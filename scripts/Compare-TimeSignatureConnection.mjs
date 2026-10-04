// Verify observed connection semantics and compare unchanged native logs.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const read=file=>JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const within=(root,file)=>{const relative=path.relative(root,file);return !!relative&&!relative.startsWith('..')&&!path.isAbsolute(relative);};
const ok='0x00000000',fail='0x80004005',invalid='0x80070057';
const timelineIid='22b5869d-523e-11d2-8913-00c04fbf8d15',trackClass='d2ac2888-b39b-11d1-8704-00600893b1bd';
const notifications=['96a0a26c-f4e7-11d1-88cb-00c04fbf8d15','d2ac28a1-b39b-11d1-8704-00600893b1bd','d2ac28a4-b39b-11d1-8704-00600893b1bd'];
function equal(actual,expected,label){if(JSON.stringify(actual)!==JSON.stringify(expected))throw Error('Connection observation mismatch: '+label);}
export function verify(directory,kind='reference'){
  directory=path.resolve(directory);
  if(!within(path.join(repo,'work',kind,'time-signature-connection'),directory))throw Error('Expected owned TimeSig connection evidence');
  const run=read(path.join(directory,'run.json'));
  if(!run.connectionProbe||run.implementation!==kind||run.exitCode!==0||run.timedOut||run.launchError||run.registryRegistrationInvoked||sha(run.dll)!==run.dllSha256)throw Error('Unsuccessful or changed connection run');
  if(kind==='reference'&&run.dllSha256!=='898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7')throw Error('Original identity mismatch');
  for(const source of run.sources??[]){const root=path.join(directory,run.sourceSnapshot),file=path.resolve(root,source.path);if(!within(root,file)||sha(file)!==source.sha256)throw Error('Connection source snapshot changed');}
  const required=['tests/native/time_signature_connection_probe.cpp','src/compat/strip.h','src/compat/timeline_services.h','src/time_signature/time_signature_dll.cpp'];
  if(required.some(file=>!run.sources?.some(source=>source.path===file)))throw Error('Missing connection sources');
  const build=read(run.buildSummary);
  if(sha(run.buildSummary)!==run.buildSummarySha256||!build.passed||!build.sourceSnapshotUnchanged||build.configureExitCode!==0||build.buildExitCode!==0)throw Error('Connection build provenance changed');
  for(const source of build.sources){const file=path.resolve(build.sourceRoot,source.path);if(!within(build.sourceRoot,file)||sha(file)!==source.sha256)throw Error('Build source snapshot changed');}
  const probe=build.outputs.find(output=>output.path==='build/Release/time_signature_connection_probe.exe');
  if(probe?.sha256!==run.probeSha256||sha(path.join(build.buildDirectory,'Release/time_signature_connection_probe.exe'))!==run.probeSha256)throw Error('Connection probe changed');
  if(kind==='candidate'&&build.outputs.find(output=>output.path==='build/Release/TimeSigStripMgr.dll')?.sha256!==run.dllSha256)throw Error('Candidate not from saved build');
  for(const source of run.sources){if(build.sources.find(item=>item.path===source.path)?.sha256!==source.sha256)throw Error('Native run/build source mismatch');}
  const records=fs.readFileSync(path.join(directory,'probe.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse);
  let cursor=0;
  const take=(row,label=row.operation)=>equal(records[cursor++],row,label);
  for(const operation of ['initialize','class_factory','create_manager'])take({operation,hr:ok});
  const addref=(timeline,refs)=>take({operation:'timeline_addref',timeline,refs});
  const release=(timeline,refs)=>take({operation:'timeline_release',timeline,refs});
  function disconnect(timeline,groups,hr=ok){
    take({operation:'remove_page_object',timeline,nonNull:true});
    for(const type of notifications.toReversed())take({operation:'remove_notification',timeline,sameManager:true,type,groups,hr});
    take({operation:'remove_page_object',timeline,nonNull:true});take({operation:'remove_strip',timeline,sameStrip:true});release(timeline,1);
  }
  function connect(timeline,groups,hr=ok){
    take({operation:'timeline_query',timeline,iid:timelineIid,hr:ok});addref(timeline,2);
    take({operation:'insert_strip',timeline,nonNull:true,class:trackClass,groups,index:0,hr});
    for(const type of notifications)take({operation:'add_notification',timeline,sameManager:true,type,groups,hr});
    take({operation:'get_track_manager',timeline,type:notifications[1],groups,index:0,hr:fail});
  }
  function connection(phase,{old=null,incoming=null,current=null,groups=1,serviceHr=ok,setHr=ok,sameInput=false,rejected=false}={}){
    take({operation:'begin_connection',phase});
    if(old)disconnect(old,groups,serviceHr);
    if(rejected)take({operation:'timeline_query',timeline:'rejected',iid:timelineIid,hr:'0x80004002'});
    else if(incoming)connect(incoming,groups,serviceHr);
    take({operation:'set_timeline',hr:setHr});
    if(current)addref(current,3);
    take({operation:'get_timeline',hr:current?ok:fail,vt:13,sameInput,isNull:!current});
    if(current)release(current,2);
    take({operation:'end_connection',firstRefs:current==='first'?2:1,secondRefs:current==='second'?2:1,rejectedRefs:1});
  }
  function properties(phase,title){
    take({operation:'begin_strip_properties',phase,present:true});take({operation:'strip_null_property',hr:'0x80004003'});
    for(const property of [0,1,2,6,7,8,9,10,12,99]){
      const vt=property===0?8:property===1||property===7||property===10?11:property===6||property===8||property===9?22:property===12?13:0;
      take({operation:'strip_property',property,hr:vt?ok:fail,vt,value:property===1?1:vt===22?20:0,managerIdentity:property===12,utf16:property===0?Array.from(title,char=>char.charCodeAt(0)):[]});
    }
    take({operation:'end_strip_properties'});
  }
  connection('wrong_type_empty',{setHr:invalid});
  connection('connect_first',{incoming:'first',current:'first',sameInput:true});properties('groups_one','1: TimeSig');
  connection('wrong_type_connected',{current:'first',setHr:invalid});
  connection('repeat_first',{old:'first',incoming:'first',current:'first',sameInput:true});
  connection('reject_replacement',{old:'first',rejected:true,setHr:fail});
  connection('failed_insert_notifications',{incoming:'first',current:'first',serviceHr:fail,sameInput:true});
  connection('disconnect_failed_insert',{old:'first',serviceHr:fail});
  take({operation:'change_groups',hr:ok});
  connection('connect_second',{incoming:'second',current:'second',groups:66,sameInput:true});properties('groups_two_seven','2, 7: TimeSig');
  connection('replace_second_first',{old:'second',incoming:'first',current:'first',groups:66,sameInput:true});
  connection('disconnect',{old:'first',groups:66});connection('repeat_disconnect');
  take({operation:'release_manager',remaining:0});take({operation:'final_fixture_refs',first:1,second:1,rejected:1,retainedStrips:0});take({operation:'can_unload',hr:ok});
  if(cursor!==records.length||cursor!==160)throw Error('Incomplete or extra connection observations');
  return {run,observedRecords:cursor,connections:11,stripProperties:20,notificationsAdded:15,notificationsRemoved:15,unloadVerified:true};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const original=path.resolve(process.argv[2]??''),reference=verify(original);
  if(!process.argv[3]){console.log(JSON.stringify({observedRecords:reference.observedRecords,connections:11,unloadVerified:true}));}
  else{
    const candidate=path.resolve(process.argv[3]),replacement=verify(candidate,'candidate');
    if(JSON.stringify(reference.run.sources)!==JSON.stringify(replacement.run.sources)||reference.run.probeSha256!==replacement.run.probeSha256||reference.run.buildSummarySha256!==replacement.run.buildSummarySha256)throw Error('Different connection inputs or build');
    if(!fs.readFileSync(path.join(original,'probe.jsonl')).equals(fs.readFileSync(path.join(candidate,'probe.jsonl'))))throw Error('Connection native log bytes differ');
    const destination=path.join(repo,'work/comparison/time-signature-connection',new Date().toISOString().replace(/[-:.]/g,''));fs.mkdirSync(destination,{recursive:true});
    const output={schema:1,passed:true,reference:original,candidate,observedRecords:160,connections:11,stripProperties:20,notificationsAdded:15,notificationsRemoved:15,unloadVerified:true,normalization:'None; full native log bytes identical',
      candidateDllSha256:replacement.run.dllSha256,probeSha256:reference.run.probeSha256,buildSummary:reference.run.buildSummary,buildSummarySha256:reference.run.buildSummarySha256,
      sourceFiles:reference.run.sources.length,verifierSha256:sha(fileURLToPath(import.meta.url)),
      evidence:[original,candidate].map(directory=>({directory,metadataSha256:sha(path.join(directory,'run.json')),logSha256:sha(path.join(directory,'probe.jsonl'))})),
      limitations:['Timeline fixture only; missing Style manager lookup path only','Style time-signature import, real Timeline connection and notifications delivery pending','Drawing, editing, property pages, runtime synchronization and host replacement pending','Destructor with live connection not compared; tested explicit disconnection','Full Producer reconstruction incomplete']};
    fs.writeFileSync(path.join(destination,'comparison.json'),JSON.stringify(output,null,2)+'\n');fs.copyFileSync(fileURLToPath(import.meta.url),path.join(destination,'Compare-TimeSignatureConnection.mjs'));
    console.log(JSON.stringify({destination,passed:true,observedRecords:160}));
  }
}
