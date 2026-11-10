// Compare source-pinned original/candidate observations; no binary loading.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const reference=path.resolve(process.argv[2]??''),candidate=path.resolve(process.argv[3]??'');
function within(root,file){const relative=path.relative(root,file);return !!relative&&!relative.startsWith('..')&&!path.isAbsolute(relative);}
if(!within(path.join(repo,'work/reference/time-signature'),reference)||!within(path.join(repo,'work/candidate/time-signature'),candidate))throw Error('Expected TimeSig original and candidate runs');
const read=file=>JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const originalSummaryVerifier=path.join(repo,'scripts/Summarize-TimeSignatureProbe.mjs');
execFileSync(process.execPath,[originalSummaryVerifier,reference],{encoding:'utf8'});
const summary=read(path.join(reference,'summary.json'));
if(summary.profile!=='meter-services-v3'||!summary.ownedServicesVerified||!summary.trackMetadataAndBorrowedVerified||summary.observedRecords!==322)throw Error('Expected verified original service profile');
const runs=[reference,candidate].map(directory=>read(path.join(directory,'run.json')));
for(const [i,run] of runs.entries()){
  if(run.exitCode!==0||run.timedOut||run.launchError||run.registryRegistrationInvoked||run.implementation!==(i?'candidate':'reference')||sha(run.dll)!==run.dllSha256||!run.sources?.length)throw Error('Unsuccessful or changed native run');
  for(const source of run.sources){const root=path.join([reference,candidate][i],run.sourceSnapshot),file=path.resolve(root,source.path);if(!within(root,file)||sha(file)!==source.sha256)throw Error('Native source snapshot mismatch');}
}
if(JSON.stringify(runs[0].sources)!==JSON.stringify(runs[1].sources)||runs[0].probeSha256!==runs[1].probeSha256||runs[0].buildSummarySha256!==runs[1].buildSummarySha256||
  runs[0].dllSha256!=='898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7')throw Error('Original/candidate input or build identity mismatch');
const build=read(runs[1].buildSummary);
if(sha(runs[1].buildSummary)!==runs[1].buildSummarySha256||!build.passed||!build.sourceSnapshotUnchanged||
  build.outputs.find(output=>output.path==='build/Release/TimeSigStripMgr.dll')?.sha256!==runs[1].dllSha256||
  build.outputs.find(output=>output.path==='build/Release/time_signature_probe.exe')?.sha256!==runs[1].probeSha256)throw Error('Candidate build provenance mismatch');
for(const source of build.sources)if(sha(path.join(build.sourceRoot,source.path))!==source.sha256)throw Error('Build source snapshot changed');
const logs=[reference,candidate].map(directory=>fs.readFileSync(path.join(directory,'probe.jsonl')));
if(!logs[0].equals(logs[1]))throw Error('Native observation log mismatch');
const names=[...summary.inputs.map(file=>file.name+'-input.bin'),...summary.savedFiles.map(file=>file.name+'-saved.bin')].sort();
const fileChecks=[];
for(const directory of [reference,candidate]){
  const actual=fs.readdirSync(directory).filter(name=>name.endsWith('.bin')).sort();
  if(JSON.stringify(actual)!==JSON.stringify(names))throw Error('Incomplete or unexpected binary output set');
}
for(const name of names){
  const originalFile=path.join(reference,name),candidateFile=path.join(candidate,name);
  if(!fs.readFileSync(originalFile).equals(fs.readFileSync(candidateFile)))throw Error('Binary output mismatch: '+name);
  fileChecks.push({name,bytes:fs.statSync(originalFile).size,sha256:sha(originalFile),same:true});
}
const destination=path.join(repo,'work/comparison/time-signature-dll',new Date().toISOString().replace(/[-:.]/g,''));
fs.mkdirSync(destination,{recursive:true});
const result={schema:1,reference,candidate,passed:true,comparedRecords:322,clockQueries:196,inputFiles:13,savedFiles:14,byteIdenticalFiles:fileChecks.length,
  referenceDllSha256:runs[0].dllSha256,candidateDllSha256:runs[1].dllSha256,probeSha256:runs[0].probeSha256,sourceFiles:runs[0].sources.length,
  cleanBuildSummary:runs[1].buildSummary,cleanBuildSummarySha256:runs[1].buildSummarySha256,ownedServiceLifetimeVerified:true,borrowedLifetimeVerified:true,
  trackMetadataVerified:true,persistenceAndClockQueriesVerified:true,unloadVerified:true,normalization:'None; full native logs and binary input/output files identical',fileChecks,
  evidence:[reference,candidate].map(directory=>({directory,metadataSha256:sha(path.join(directory,'run.json')),logSha256:sha(path.join(directory,'probe.jsonl'))})),
  verifierSha256:sha(fileURLToPath(import.meta.url)),originalSummaryVerifierSha256:sha(originalSummaryVerifier),differences:[],
  limitations:['Standalone creation, parameters, metadata, persistence and IUnknown service ownership only','Service fixtures are disconnected during persistence; real runtime synchronization unverified','Timeline/strip connection, UI, editing, notifications and host replacement pending','Full Producer reconstruction incomplete']};
fs.writeFileSync(path.join(destination,'comparison.json'),JSON.stringify(result,null,2)+'\n');
fs.copyFileSync(fileURLToPath(import.meta.url),path.join(destination,'Compare-TimeSignatureDll.mjs'));
fs.copyFileSync(originalSummaryVerifier,path.join(destination,'Summarize-TimeSignatureProbe.mjs'));
console.log(JSON.stringify({destination,passed:true,comparedRecords:322,byteIdenticalFiles:fileChecks.length,candidateDllSha256:runs[1].dllSha256}));
