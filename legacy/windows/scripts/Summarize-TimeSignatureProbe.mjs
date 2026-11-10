import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.resolve(process.argv[2]??'');
const relative=path.relative(path.join(repo,'work/reference/time-signature'),directory);
if(!relative||relative.startsWith('..')||path.isAbsolute(relative))throw Error('Expected TimeSig reference run');
const read=name=>JSON.parse(fs.readFileSync(path.join(directory,name),'utf8').replace(/^\uFEFF/,''));
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const run=read('run.json');
if(run.exitCode!==0||run.timedOut||run.launchError||run.dllSha256!=='898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7')throw Error('Original run failed');
for(const source of run.sources){
  const root=path.join(directory,run.sourceSnapshot),file=path.resolve(root,source.path),relative=path.relative(root,file);
  if(!relative||relative.startsWith('..')||path.isAbsolute(relative)||sha(file)!==source.sha256)throw Error('Source snapshot mismatch');
}
const records=fs.readFileSync(path.join(directory,'probe.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse);
const base=['empty','four_four','three_eight','five_four','zero_fields','first_late','changes','list_extended','empty_chunk'];
const extra=['first_unaligned','changes_unaligned','duplicate_measure','unsorted','negative_first'];
const extended=records.some(r=>r.operation==='load'&&r.case==='negative_first');
const services=records.some(r=>r.operation==='begin_service_probe');
const cases=extended?[...base,...extra]:base;
const release=records.at(services?-5:-2);
if((services&&!extended)||records.length!==(services?322:extended?261:176)||records.at(-1).operation!=='can_unload'||records.at(-1).hr!=='0x00000000'||
  release.operation!=='release_manager'||release.remaining!==0)throw Error('Incomplete or unsupported probe profile');
function exact(actual,expected,label){
  if(!actual||Object.keys(actual).sort().join('|')!==Object.keys(expected).sort().join('|')||Object.entries(expected).some(([key,value])=>JSON.stringify(actual[key])!==JSON.stringify(value)))throw Error(label+' mismatch');
}
if(services){
  const ok='0x00000000',invalid='0x80070057',pointer='0x80004003',fail='0x80004005';
  const metadata=[{operation:'begin_metadata_probe',version:1},{operation:'supports_borrowed',hr:ok},{operation:'supports_undo_label',hr:ok},
    {operation:'borrowed_initial',hr:ok,isNull:true,next:0x12345678},{operation:'undo_label',hr:ok,utf16:[]}];
  for(const property of [0,1,2,3,4,5,6,99])metadata.push({operation:'property_arguments',property,nullHr:pointer,emptyHr:property<=2?fail:property===6?ok:invalid,nullByrefHr:property<=2?fail:property===6?ok:property===99?invalid:pointer});
  function track(phase,groups,flags,producerFlags){metadata.push({operation:'track_header',phase,hr:ok,vt:16384,expectedTrackClass:true,position:0,groups,chunk:0,list:0x534d4954},
    {operation:'track_flags',phase,property:4,hr:ok,flags},{operation:'track_flags',phase,property:5,hr:ok,flags:producerFlags});}
  track('initial',1,56,0);
  for(const property of [3,4,5,6,99])metadata.push({operation:'set_property_arguments',property,emptyHr:invalid,nullByrefHr:property<=5?pointer:invalid});
  for(const operation of ['set_track_header','set_track_flags','set_producer_flags'])metadata.push({operation,hr:ok});
  track('changed',66,0x30038,0x89abcdef);
  metadata.push({operation:'set_borrowed_first',hr:ok},{operation:'borrowed_first',hr:ok,identity:true,refs:2,adds:1,releases:0},
    {operation:'set_borrowed_second',hr:ok},{operation:'borrowed_second',hr:ok,identity:true,firstRefs:1,firstReleases:1,refs:2,adds:1,releases:0},
    {operation:'set_borrowed_null',hr:pointer});
  for(const operation of ['reset_track_header','reset_track_flags','reset_producer_flags'])metadata.push({operation,hr:ok});
  metadata.push({operation:'end_metadata_probe'});
  const metadataStart=records.findIndex(r=>r.operation==='begin_metadata_probe');
  metadata.forEach((expected,i)=>exact(records[metadataStart+i],expected,'Metadata observation '+i));
  exact(records.at(-4),{operation:'borrowed_after_manager_release',firstRefs:1,firstAdds:1,firstReleases:1,secondRefs:1,secondAdds:1,secondReleases:1},'Borrowed lifetime');
  const phases=[['wrong_type',invalid,1,0,0,0,0,0,0,false,0,0],['connect',ok,2,1,0,1,0,1,0,false,0,0],
    ['get_connected',ok,2,2,1,2,1,1,0,true,13,3],['repeat_same_input',ok,2,3,2,3,1,2,0,false,0,0],
    ['reject_replacement',ok,1,3,3,3,1,2,1,false,0,0],['get_after_rejection',fail,1,3,3,3,1,2,1,false,13,1],
    ['reconnect',ok,2,4,3,4,1,3,1,false,0,0],['disconnect',ok,1,4,4,4,1,3,1,false,0,0],
    ['get_disconnected',fail,1,4,4,4,1,3,1,false,13,1],['retain_until_destruction',ok,2,5,4,5,1,4,1,false,0,0]];
  const serviceStart=records.findIndex(r=>r.operation==='begin_service_probe');
  exact(records[serviceStart],{operation:'begin_service_probe',version:1},'Service start');
  for(const property of [1,2])for(const [i,row] of phases.entries()){
    const [phase,hr,refs,adds,releases,queries,unknownQueries,serviceQueries,rejectedQueries,canonicalUnknown,vt,duringGetRefs]=row;
    exact(records[serviceStart+1+(property-1)*10+i],{operation:'service_connection',property,phase,hr:property===2&&phase==='reject_replacement'?'0x80004002':hr,
      refs,adds,releases,queries,unknownQueries,serviceQueries,rejectedRefs:1,rejectedQueries:rejectedQueries+property-1,canonicalUnknown,vt,duringGetRefs},'Service '+property+'/'+phase);
  }
  exact(records[serviceStart+21],{operation:'end_service_probe'},'Service end');
  for(const property of [1,2])exact(records.at(property-4),{operation:'service_connection',property,phase:'after_manager_release',hr:ok,refs:1,adds:5,releases:5,queries:5,unknownQueries:1,serviceQueries:4,rejectedRefs:1,rejectedQueries:2,canonicalUnknown:false,vt:0,duringGetRefs:0},'Owned service release');
  if(!run.buildSummary||sha(run.buildSummary)!==run.buildSummarySha256)throw Error('Build provenance mismatch');
  const build=JSON.parse(fs.readFileSync(run.buildSummary,'utf8').replace(/^\uFEFF/,''));
  if(!build.passed||!build.sourceSnapshotUnchanged||build.outputs.find(r=>r.path==='build/Release/time_signature_probe.exe')?.sha256!==run.probeSha256)throw Error('Built probe mismatch');
  for(const source of build.sources)if(sha(path.join(build.sourceRoot,source.path))!==source.sha256)throw Error('Build source snapshot mismatch');
}
const tuples={four_four:[[0,4,4,4]],three_eight:[[0,3,8,4]],five_four:[[0,5,4,4]],zero_fields:[[0,0,0,0]],
  first_late:[[3072,4,4,4]],changes:[[0,4,4,4],[3072,3,4,4],[7680,5,8,4]],list_extended:[[0,4,4,4],[3072,3,4,4]],empty_chunk:[],
  first_unaligned:[[1500,3,8,3]],changes_unaligned:[[0,4,4,4],[4100,3,4,4],[8000,5,8,4]],
  duplicate_measure:[[0,4,4,4],[0,3,4,2]],unsorted:[[6144,4,4,4],[0,3,4,4]],negative_first:[[-3072,4,4,4]]};
const savedTuples={...tuples,empty:[],zero_fields:[[0,4,2,2]],first_unaligned:[[1152,3,8,3]],
  changes_unaligned:tuples.changes,duplicate_measure:[[0,3,4,2]],unsorted:[[0,3,4,4],[4608,4,4,4]]};
const event=([time,beats,denominator,grids])=>({time,beats,denominator,grids});
function input(name){
  const size=name==='list_extended'?12:8,rows=tuples[name],bytes=Buffer.alloc(12+size*rows.length,0xa5);
  bytes.write('tims',0,'ascii');bytes.writeUInt32LE(4+size*rows.length,4);bytes.writeUInt32LE(size,8);
  rows.forEach(([time,beats,denominator,grids],i)=>{const offset=12+i*size;bytes.writeInt32LE(time,offset);bytes[offset+4]=beats;bytes[offset+5]=denominator;bytes.writeUInt16LE(grids,offset+6);});
  if(name!=='list_extended')return bytes;
  const wrapper=Buffer.alloc(12);wrapper.write('LIST',0,'ascii');wrapper.writeUInt32LE(4+bytes.length,4);wrapper.write('TIMS',8,'ascii');return Buffer.concat([wrapper,bytes]);
}
function saved(name){
  const file=path.join(directory,name+'-saved.bin'),bytes=fs.readFileSync(file),events=[];
  if(bytes.length<12||bytes.toString('ascii',0,4)!=='LIST'||bytes.toString('ascii',8,12)!=='TIMS'||bytes.readUInt32LE(4)!==bytes.length-8)throw Error('Invalid TIMS wrapper');
  if(bytes.length>12){
    if(bytes.length<24||bytes.toString('ascii',12,16)!=='tims'||bytes.readUInt32LE(16)!==bytes.length-20||bytes.readUInt32LE(20)!==8||(bytes.length-24)%8)throw Error('Invalid tims record layout');
    for(let o=24;o<bytes.length;o+=8)events.push({time:bytes.readInt32LE(o),beats:bytes[o+4],denominator:bytes[o+5],grids:bytes.readUInt16LE(o+6)});
  }
  if(JSON.stringify(events)!==JSON.stringify(savedTuples[name].map(event)))throw Error('Saved event semantics mismatch: '+name);
  return {name,bytes:bytes.length,sha256:sha(file),events};
}
const files=cases.map(saved),inputFiles=[];
for(const file of files){
  const observations=records.filter(r=>r.operation==='save'&&r.case===file.name);
  if(observations.length!==1||observations[0].hr!=='0x00000000'||!observations[0].fileWritten||observations[0].bytes!==file.bytes)throw Error('Saved observation mismatch');
  if(file.name!=='empty'){
    const filename=path.join(directory,file.name+'-input.bin'),bytes=fs.readFileSync(filename),loads=records.filter(r=>r.operation==='load'&&r.case===file.name);
    if(!bytes.equals(input(file.name))||loads.length!==1||loads[0].hr!=='0x00000000')throw Error('Input/load mismatch: '+file.name);
    inputFiles.push({name:file.name,bytes:bytes.length,sha256:sha(filename),events:tuples[file.name].map(event)});
  }
}
const times=[-1,0,1,1535,1536,3071,3072,5375,5376,6143,6144,7679,7680,10000];
for(const file of files){
  const queries=records.filter(r=>r.operation==='get_meter'&&r.case===file.name);
  if(queries.length!==times.length||queries.some((r,i)=>r.time!==times[i]))throw Error('Missing clock observations');
  for(const r of queries){
    if(!file.events.length){
      if(r.hr!=='0x88781161'||r.next!==0||r.returnedTime!==0x5a5a5a5a||r.beats!==90||r.denominator!==90||r.grids!==0x5a5a)throw Error('Empty query modified outputs');
    }else{
      const applicable=file.events.filter(e=>e.time<=r.time).at(-1)??file.events[0],next=file.events.find(e=>e.time>r.time);
      if(r.hr!=='0x00000000'||r.returnedTime!==0||r.next!==(next?next.time-r.time:0)||
        r.beats!==applicable.beats||r.denominator!==applicable.denominator||r.grids!==applicable.grids)throw Error('Query/save consistency mismatch: '+file.name);
    }
  }
}
const expectedHr={initialize:'0x00000000',class_factory:'0x00000000',create_manager:'0x00000000',query_persist:'0x00000000',class_id:'0x00000000',
  class_id_null:'0x80004003',dirty_initial:'0x00000001',size_max:'0x80004001',supports_meter:'0x00000000',supports_unknown:'0x00000001',
  get_unknown:'0x80070057',get_meter_null:'0x80004003',set_meter:'0x80070057',set_meter_null:'0x80004003',load_null:'0x80070057',save_null:'0x80070057'};
for(const [operation,hr] of Object.entries(expectedHr)){
  const found=records.filter(r=>r.operation===operation);if(found.length!==1||found[0].hr!==hr)throw Error('ABI result mismatch: '+operation);
}
const properties=records.filter(r=>r.operation==='get_property');
if(properties.length!==7||new Set(properties.map(r=>r.property)).size!==7)throw Error('Missing properties');
for(const r of properties){
  const expected={0:['0x80004005',13,0],1:['0x80004005',13,0],2:['0x80004005',13,0],4:['0x00000000',16384,56],5:['0x00000000',16384,0],6:['0x00000000',3,196664],99:['0x80070057',0,0]}[r.property];
  if(!expected||r.hr!==expected[0]||r.vt!==expected[1]||(r.property===4||r.property===5?r.flags:r.value)!==expected[2])throw Error('Property result mismatch');
}
const dirty=records.filter(r=>r.operation==='dirty_after_save');
if(dirty.length!==cases.length-1||dirty.some(r=>r.hr!=='0x00000001'))throw Error('Dirty state mismatch');
const getMeter=records.filter(r=>r.operation==='get_meter');
if(getMeter.length!==cases.length*times.length)throw Error('Unexpected query cases');
const output={schema:3,run:path.relative(repo,directory).replaceAll('\\','/'),dllSha256:run.dllSha256,probeSha256:run.probeSha256,
  metadataSha256:sha(path.join(directory,'run.json')),logSha256:sha(path.join(directory,'probe.jsonl')),verifierSha256:sha(fileURLToPath(import.meta.url)),
  profile:services?'meter-services-v3':extended?'meter-boundaries-v2':'meter-basic-v1',observedRecords:records.length,clockQueries:getMeter.length,sourceFiles:run.sources.length,
  inputFiles:inputFiles.length,inputs:inputFiles,savedFiles:files,querySaveConsistencyVerified:true,unloadVerified:true,trackMetadataAndBorrowedVerified:services,ownedServicesVerified:services,
  limitations:['Original standalone parameters and persistence only','Replacement DLL behavior not established by this reference run',
    ...(services?['Owned service tests use separate canonical/service IUnknown views; fake services disconnected during persistence']:['Track metadata/borrowed-object dynamic probe unverified']),
    'Timeline connection, UI, editing, notification, runtime synchronization and host acceptance pending']};
fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(output,null,2)+'\n');
console.log(JSON.stringify({profile:output.profile,observedRecords:output.observedRecords,clockQueries:output.clockQueries,sourceFiles:output.sourceFiles,inputFiles:output.inputFiles,savedFiles:files.length,unloadVerified:true}));
