// A limited feature comparison, including failed full-run termination metadata.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const args=process.argv.slice(2);
if(args.length!==3)throw Error('Usage: Compare-SystemClipboard.mjs original-run candidate-run output.json');
const roots=args.slice(0,2).map(x=>path.resolve(x));
const json=f=>JSON.parse(fs.readFileSync(f,'utf8').replace(/^\uFEFF/,''));
const hash=f=>crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex');
const runs=roots.map(r=>json(path.join(r,'run.json')));
if(runs[0].implementation!=='original'||runs[1].implementation!=='candidate'||
 runs.some(r=>r.timedOut||r.launchError||!r.systemClipboard||!r.windowedTimeline)||
 runs[0].probeSha256!==runs[1].probeSha256||JSON.stringify(runs[0].sources)!==JSON.stringify(runs[1].sources))throw Error('Same executable, sources and fixture required');
for(let i=0;i<2;++i)for(const source of runs[i].sources){
 const snapshot=path.join(roots[i],'sources'),file=path.resolve(snapshot,source.path);
 if(!file.startsWith(snapshot+path.sep)||hash(file)!==source.sha256)throw Error('Changed source snapshot');
}
const logs=roots.map(r=>fs.readFileSync(path.join(r,'probe.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse));
const cases=['single','multiple','cut_multiple','overwrite_multiple'];
function normalizeCopy(bytes){
 const out=Buffer.from(bytes);
 if(out.toString('ascii',0,4)!=='tetr'||out.readUInt32LE(8)!==24||(out.length-12)%24)throw Error('Unexpected copied record');
 for(let at=12;at<out.length;at+=24){out.fill(0,at+4,at+8);out.fill(0,at+20,at+24);}
 return out;
}
const reports=cases.map(name=>{
 const records=logs.map(log=>{
  const a=log.findIndex(x=>x.operation==='begin_system_clipboard_case'&&x.case===name);
  const b=log.findIndex(x=>x.operation==='end_system_clipboard_case'&&x.case===name);
  if(a<0||b<=a||!log[b].passed)throw Error('Incomplete case '+name);
  return log.slice(a,b+1).filter(x=>!x.operation.startsWith('fixture_'));
 });
 if(JSON.stringify(records[0])!==JSON.stringify(records[1]))throw Error('Clipboard observations differ '+name);
 const files=['input','clipboard','after-copy','output','reload'].map(suffix=>{
  const filename=`system-clipboard-${name}-${suffix}.bin`,identities=roots.map(r=>hash(path.join(r,filename)));
  const bytes=roots.map(r=>fs.readFileSync(path.join(r,filename)));
  const sameBytes=bytes[0].equals(bytes[1]);
  const sameContent=suffix==='clipboard'?normalizeCopy(bytes[0]).equals(normalizeCopy(bytes[1])):sameBytes;
  if(!sameContent)throw Error('Clipboard content differs '+filename);
  return {filename,sha256:identities,sameBytes,sameContent,normalization:suffix==='clipboard'?'Established 24-byte record padding +4..7/+20..23 only; raw bytes retained':null};
 });
 return {name,records:records[0].length,files};
});
const termination=logs.map((log,i)=>({implementation:runs[i].implementation,exitCode:runs[i].exitCode,
 timelineUnload:log.filter(x=>x.operation.startsWith('timeline_can_unload')),tempoUnload:log.find(x=>x.operation==='can_unload_after_release')}));
const output={createdUtc:new Date().toISOString(),scope:'Four system clipboard cases only; full run acceptance is separate',
 roots,candidateSha256:runs[1].dllSha256,probeSha256:runs[0].probeSha256,cases:reports,featureCasesMatch:true,
 fullRunsPassed:runs.every(r=>r.exitCode===0),termination};
fs.writeFileSync(args[2],JSON.stringify(output,null,2)+'\n');
console.log(JSON.stringify(output,null,2));
