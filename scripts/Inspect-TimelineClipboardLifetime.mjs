// Read-only checks of the fixed original Timeline's clipboard object lifetime.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const input=path.join(root,'work/producer/app/Timeline.dll');
const analysis=path.join(root,'work/analysis/pe/app__Timeline.dll');
const bytes=fs.readFileSync(input);
const sha256=crypto.createHash('sha256').update(bytes).digest('hex');
const expected='bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176';
if(sha256!==expected)throw Error('Original Timeline identity changed');
const pe=JSON.parse(fs.readFileSync(path.join(analysis,'pe.json'),'utf8'));
if(pe.sha256!==sha256||pe.imageBase!=='0x400000')throw Error('PE analysis identity mismatch');
function at(rva){
 const section=pe.sections.find(s=>rva>=s.rva&&rva<s.rva+s.rawSize);
 if(!section)throw Error('RVA outside raw bytes');
 return section.rawOffset+rva-section.rva;
}
function check(rva,hex){
 const offset=at(rva),actual=bytes.subarray(offset,offset+hex.length/2).toString('hex');
 if(actual!==hex)throw Error('Unexpected instruction at RVA '+rva.toString(16));
 return {rva:'0x'+rva.toString(16),fileOffset:'0x'+offset.toString(16),bytes:actual};
}
function slot(rva,index,expectedRva){
 const value=bytes.readUInt32LE(at(rva+index*4))-0x400000;
 if(value!==expectedRva)throw Error('Vtable slot differs');
 return {tableRva:'0x'+rva.toString(16),slot:index,targetRva:'0x'+value.toString(16)};
}
const evidence={
 unloadCounterCheck:check(0x5d86,'3905dce24100'),
 exportCounterAddress:check(0x1627e,'68dce24100'),
 exportIncrementCall:check(0x16283,'ff1538114000'),
 exportFactoryCall:check(0x1626f,'e84a310000'),
 dataCtorCounterAddress:check(0x15fdb,'68dce24100'),
 dataCtorIncrementCall:check(0x15fe4,'ff1538114000'),
 dataDtorCounterAddress:check(0x15913,'68dce24100'),
 dataDtorDecrementCall:check(0x15918,'ff1534114000'),
 exportObjectCtorBaseCall:check(0x193c1,'e8e3070000'),
 exportObjectDestructorBaseJump:check(0x193d6,'e94e060000'),
 exportObjectReleaseDestructorDispatch:check(0x19760,'ff5030'),
 exportObjectDeletingDestructorCall:check(0x196b6,'e815fdffff'),
 exportObjectBaseDtorEntry:check(0x19a29,'b8f7a84100'),
 exportObjectBaseDtorEnd:check(0x19a90,'c9c3'),
};
const mappings=[slot(0x2a74,10,0x16224),slot(0x2cd4,2,0x1945b),slot(0x2cd4,12,0x196b3)];
const increments=pe.imports.find(x=>x.iatRva==='0x1138'),decrements=pe.imports.find(x=>x.iatRva==='0x1134');
if(increments?.name!=='InterlockedIncrement'||decrements?.name!=='InterlockedDecrement')throw Error('Counter imports differ');
const constructor=bytes.subarray(at(0x19ba9),at(0x19bcb));
const destructor=bytes.subarray(at(0x19a29),at(0x19a92));
const globalAddress=Buffer.from('dce24100','hex');
if(constructor.includes(globalAddress)||destructor.includes(globalAddress))throw Error('Base ctor/dtor directly touches module counter');
const output={schema:1,input:path.relative(root,input).replaceAll('\\','/'),sha256,
 counterRva:'0x1e2dc',counterImports:{increment:increments,decrement:decrements},mappings,evidence,
 findings:[
  'TimelineDataObject construction/destruction directly increments/decrements the module counter.',
  'Successful Export enters an additional InterlockedIncrement after allocating CDllJazzDataObject.',
  'The exported object Release dispatches to its deleting destructor; that destructor reaches the base destructor without a matching direct module-counter decrement.',
 ],
 interpretation:'The observed S_FALSE after clipboard Export is consistent with an unbalanced original module counter. This is not proof that live COM references remain.',
 limits:['No runtime counter value read','No complete indirect-call proof','Does not alter DLL code, data, registry or clipboard'],
};
const destination=process.argv[2]?path.resolve(process.argv[2]):path.join(analysis,'clipboard-lifetime.json');
fs.writeFileSync(destination,JSON.stringify(output,null,2)+'\n');
console.log(JSON.stringify({output:destination,sha256,counterRva:output.counterRva,instructionChecks:Object.keys(evidence).length,interfaceMappings:mappings.length}));
