// Static evidence only. Does not load modules or change the Windows registry.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const peRoot=path.join(root,'work/analysis/pe');
const hex=n=>`0x${n.toString(16)}`;
const load=name=>{
  const dir=path.join(peRoot,`app__${name}`);
  const pe=JSON.parse(fs.readFileSync(path.join(dir,'pe.json'),'utf8'));
  const bytes=fs.readFileSync(path.join(root,pe.input));
  if(crypto.createHash('sha256').update(bytes).digest('hex')!==pe.sha256)throw Error(`Identity changed: ${name}`);
  const offset=rva=>{
    const section=pe.sections.find(s=>rva>=s.rva&&rva-s.rva<s.rawSize);
    if(!section)throw Error(`Unmapped RVA: ${hex(rva)}`);
    return section.rawOffset+rva-section.rva;
  };
  const string=rva=>{const start=offset(rva);return bytes.subarray(start,bytes.indexOf(0,start)).toString('ascii');};
  const guid=rva=>{const b=bytes.subarray(offset(rva),offset(rva)+16);return [b.readUInt32LE(0).toString(16).padStart(8,'0'),b.readUInt16LE(4).toString(16).padStart(4,'0'),b.readUInt16LE(6).toString(16).padStart(4,'0'),b.subarray(8,10).toString('hex'),b.subarray(10,16).toString('hex')].join('-').toUpperCase();};
  const resources=pe.resources.filter(r=>r.labels[0]==='6').flatMap(r=>{
    const b=fs.readFileSync(path.join(dir,r.file));let at=0;const result=[];
    for(let i=0;i<16;i++){const size=b.readUInt16LE(at);at+=2;const text=b.subarray(at,at+size*2).toString('utf16le');at+=size*2;
      if(size)result.push({id:(Number(r.labels[1])-1)*16+i,text,resource:r.file});}
    return result;
  });
  const strings=fs.readFileSync(path.join(dir,'strings.jsonl'),'utf8').trim().split('\n').map(JSON.parse);
  const assemblyFile=path.join(dir,'disassembly.txt');
  const assembly=fs.existsSync(assemblyFile)?fs.readFileSync(assemblyFile,'utf8'):'';
  const instructions=assembly.split(/\r?\n/).flatMap(line=>{
    const match=line.match(/^\s*([0-9a-f]+):\s+(.+)$/i);return match?[{va:parseInt(match[1],16),instruction:match[2].trim()}]:[];
  });
  return {name,dir,pe,bytes,offset,string,guid,resources,strings,assembly,instructions};
};
const modules=fs.readdirSync(peRoot).filter(name=>/^app__/.test(name)).map(name=>load(name.slice(5)));
const results=modules.map(m=>{
  const registryStrings=m.strings.filter(s=>s.encoding==='ascii'&&s.rva&&s.text.includes('Software\\Microsoft\\DMUSProducer'));
  const references=registryStrings.map(s=>{
    const va=Number(m.pe.imageBase)+Number(s.rva);
    return {text:s.text,rva:s.rva,uses:m.instructions.filter(i=>new RegExp(`\\b${hex(va)}\\b`,'i').test(i.instruction)).map(i=>({rva:hex(i.va-Number(m.pe.imageBase)),instruction:i.instruction}))};
  });
  return {module:m.pe.input,sha256:m.pe.sha256,disassemblyAvailable:!!m.assembly,registerExport:m.pe.exports.find(e=>e.name==='DllRegisterServer')??null,registryStrings:references,
    componentNameResource:m.resources.find(r=>r.id===201)??null};
}).filter(m=>m.registryStrings.length);
const exe=modules.find(m=>m.name==='DMUSProd.exe');
const segment=modules.find(m=>m.name==='SegmentDesigner.ocx');
// These roles follow the complete RegisterServer helper at RVA 0x1f9be:
// 0x1fc7e builds Components\<GUID>, 0x1fcd2 writes its default value;
// 0x1fcf5 writes DWORD value Skip=0. 0x1fe4c/6d/8e write associations.
const segmentRoles=[['Component',0x50dc],['RefNode',0x50ac],['Segment node',0x509c],['DirectMusic object',0x3284],['Control',0x526c],['Separate COM class',0x50ec]];
const report={schema:1,method:'Static instruction and resource extraction; no registry writes or DLL execution',
  modules:results,
  host:{module:exe.pe.input,sha256:exe.pe.sha256,componentLoaderRva:'0x18c93',registryPath:exe.string(0x3604),hive:'HKEY_LOCAL_MACHINE',
    access:'KEY_READ (0x20019), x86 default view',nameValue:exe.string(0x2b26),skipValue:exe.string(0x35fc),
    errorResource:exe.resources.find(r=>r.id===0xee76),componentIid:exe.guid(0x3d70),coCreateCallRva:'0x18e02'},
  segment:{module:segment.pe.input,sha256:segment.pe.sha256,registerHelperRva:'0x1f9be',
    defaultName:segment.resources.find(r=>r.id===201),skip:0,
    identifiers:segmentRoles.map(([role,rva])=>({role,rva:hex(rva),guid:segment.guid(rva)})),
    associationValues:[{name:'RefNode',rva:'0x35c0'},{name:'Component',rva:'0x35b4'},{name:'DMObject',rva:'0x35a8'}].map(v=>({...v,text:segment.string(Number(v.rva))}))},
  caveat:'Only Segment registration roles are traced here. String references in other modules are leads, not a complete installer or COM registration manifest.'};
fs.writeFileSync(path.join(root,'docs/analysis/producer-registration.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({modulesWithProducerRegistryStrings:results.length,host:report.host,segment:report.segment}));
