import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';

const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.resolve(process.argv[2]??'');
const relative=path.relative(path.join(repo,'work/integration/user-trial'),directory);
if(!relative||relative.startsWith('..')||path.isAbsolute(relative))throw Error('Supply a prepared user-trial directory');
const read=name=>JSON.parse(fs.readFileSync(path.join(directory,name),'utf8').replace(/^\uFEFF/,''));
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const user=read('install-state.json'),machine=read('machine-install-state.json'),plan=read('plan.json');
if(!user.completed||user.verifiedValues!==471||user.ownedRoots!==87||!machine.completed||machine.verifiedValues!==105)throw Error('Registration trial lacks verification');
if(user.planSha256!==sha(path.join(directory,'plan.json'))||machine.planSha256!==sha(path.join(directory,'machine-plan.json')))throw Error('Registration plans changed');
const inputs=read('ui-sample-inputs.json');
if(inputs.length!==5)throw Error('Incomplete original sample manifest');
for(const item of inputs){
  if(sha(path.join(repo,'work/producer/samples/QuickStart',item.relative))!==item.sha256)throw Error('Original sample changed');
}
const actions=fs.readFileSync(path.join(directory,'ui-actions.jsonl'),'utf8').trim().split(/\r?\n/).filter(Boolean).map(JSON.parse);
const required=[['original_tempo_properties_112','112.00'],['committed_tempo_137','137.00'],['observed_undo_112','112.00'],['observed_redo_137','137.00'],['original_project_reopened_137','137.00']];
const observations=required.map(([action,value])=>{
  const record=actions.find(r=>r.action===action);
  if(!record?.tree.includes(`Value: ${value} ID: 203`))throw Error('Missing settled UI observation: '+action);
  return {action,utc:record.utc,transportBpm:Number(value)};
});
const closed=actions.find(r=>r.action==='project_closed_confirmed');
if(!closed||closed.tree.includes('ツリー項目'))throw Error('Project unloading not observed');
const exit=read('original-ui-exit.json');
if(exit.running!==false)throw Error('Original UI did not exit');
const riff=read('ui-riff-tempo-comparison.json');
if(riff.reports.length!==3)throw Error('Incomplete saved RIFF evidence');
const segments=riff.reports.map((r,i)=>{
  if(sha(r.file)!==r.sha256||r.tempo.length!==1||r.tempo[0].events.length!==1||r.tempo[0].events[0].time!==0||r.tempo[0].events[0].bpm!==(i?137:112))throw Error('Saved RIFF evidence mismatch');
  return {file:path.relative(repo,r.file).replaceAll('\\','/'),sha256:r.sha256,bytes:r.bytes,tempo:r.tempo[0].events};
});
const originalFiles=read('machine-startup-file-checks.json');
if(originalFiles.length!==42||originalFiles.some(r=>!r.unchanged))throw Error('Original run file identities incomplete');
let candidate={acceptance:'Candidate UI evidence not recorded'};
let clipboardTrial=null;
if(fs.existsSync(path.join(directory,'tempo-clipboard-replacement.json'))){
  clipboardTrial=read('tempo-clipboard-replacement.json');
  if(!clipboardTrial.completed||path.resolve(clipboardTrial.trial)!==directory||clipboardTrial.planSha256!==sha(path.join(directory,'plan.json'))||
    clipboardTrial.originalSha256!==plan.files.find(f=>f.name==='TempoStripMgr.dll')?.sha256||
    sha(path.join(directory,'backup/TempoClipboardOriginal.dll'))!==clipboardTrial.originalSha256||
    sha(clipboardTrial.featureComparison)!==clipboardTrial.featureComparisonSha256)throw Error('Clipboard replacement ownership mismatch');
  const expectedCurrent=fs.existsSync(path.join(directory,'tempo-clipboard-restoration.json'))?clipboardTrial.originalSha256:clipboardTrial.candidateSha256;
  if(sha(path.join(plan.app,'TempoStripMgr.dll'))!==expectedCurrent)throw Error('Clipboard trial current DLL mismatch');
}
if(fs.existsSync(path.join(directory,'candidate-ui-exit.json'))){
  const replacement=read('tempo-replacement.json'),identity=read('candidate-loaded-tempo-identity.json');
  if(!replacement.completed||identity.sha256!==replacement.candidateSha256||!identity.path.toLowerCase().endsWith('\\tempostripmgr.dll'))throw Error('Candidate loaded DLL identity mismatch');
  if(sha(replacement.backup)!==replacement.originalSha256)throw Error('Original DLL backup changed');
  const candidateInputs=read('candidate-ui-sample-inputs.json');
  if(candidateInputs.length!==5)throw Error('Incomplete candidate inputs');
  for(const item of candidateInputs)if(sha(path.join(repo,'work/producer/samples/QuickStart',item.name))!==item.sha256)throw Error('Candidate input differs from original');
  const requiredCandidate=[['candidate_tempo_properties_112','112.00'],['candidate_committed_tempo_137','137.00'],['candidate_observed_undo_112','112.00'],['candidate_observed_redo_137','137.00'],['candidate_project_reopened_137','137.00']];
  const candidateObservations=requiredCandidate.map(([action,value])=>{
    const record=actions.find(r=>r.action===action);
    if(!record?.tree.includes(`Value: ${value} ID: 203`))throw Error('Missing candidate UI observation: '+action);
    return {action,utc:record.utc,transportBpm:Number(value)};
  });
  const unloaded=actions.find(r=>r.action==='candidate_project_unloaded');
  if(!unloaded||unloaded.tree.includes('Segment: heartland')||unloaded.tree.includes('heartland.sgp Value:'))throw Error('Candidate document was not unloaded');
  const candidateExit=read('candidate-ui-exit.json');
  if(candidateExit.running!==false||candidateExit.pid!==identity.pid)throw Error('Candidate process exit not verified');
  const candidateRiff=read('candidate-ui-riff-tempo-comparison.json');
  if(candidateRiff.reports.length!==2)throw Error('Incomplete candidate RIFF evidence');
  for(const [i,r] of candidateRiff.reports.entries())if(sha(r.file)!==r.sha256||r.tempo.length!==1||r.tempo[0].events.length!==1||r.tempo[0].events[0].time!==0||r.tempo[0].events[0].bpm!==(i?137:112))throw Error('Candidate saved RIFF mismatch');
  const snapshot=path.join(directory,'heartland-candidate-137.sgp');
  if(sha(snapshot)!==candidateRiff.reports[1].sha256)throw Error('Candidate saved snapshot mismatch');
  const restart=actions.find(r=>r.action==='candidate_restarted_project_reopened_137');
  if(restart&&!restart.tree.includes('Value: 137.00 ID: 203'))throw Error('Restarted candidate observation mismatch');
  if(restart){
    const restartIdentity=read('candidate-restarted-loaded-tempo-identity.json');
    const projectPath=actions.find(r=>r.action==='candidate_restart_project_path_confirmed');
    if(restartIdentity.pid===identity.pid||restartIdentity.sha256!==identity.sha256||!projectPath?.tree.includes('\\UiTest\\Candidate\\QuickStart\\QuickStart.pro'))throw Error('Restarted DLL or project identity not verified');
  }
  candidate={replacement,loadedIdentity:identity,basicTempoEditSaveProjectReload:true,processExited:true,
    restartReloadObserved:Boolean(restart),observations:candidateObservations,
    savedSegment:{sha256:sha(snapshot),bytes:fs.statSync(snapshot).size,tempo:candidateRiff.reports[1].tempo[0].events},
    acceptance:'Basic tempo edit, save, Undo/Redo and project reload verified; full module acceptance pending'};
  if(fs.existsSync(path.join(directory,'tempo-restoration.json'))){
    const restoration=read('tempo-restoration.json'),originalIdentity=read('original-restored-loaded-tempo-identity.json');
    const originalReload=actions.find(r=>r.action==='original_reloaded_candidate_saved_137');
    const originalPath=actions.find(r=>r.action==='original_reload_candidate_project_path_confirmed');
    if(!restoration.restored||restoration.originalSha256!==replacement.originalSha256||(!clipboardTrial&&sha(replacement.target)!==replacement.originalSha256)||originalIdentity.sha256!==replacement.originalSha256)throw Error('Original DLL restoration identity mismatch');
    if(!originalReload?.tree.includes('Value: 137.00 ID: 203')||!originalPath?.tree.includes('\\UiTest\\Candidate\\QuickStart\\QuickStart.pro'))throw Error('Candidate saved file was not identified in original UI');
    candidate.originalDllRestored=true;
    candidate.originalDllCurrentlyRestored=sha(replacement.target)===replacement.originalSha256;
    candidate.savedFileReadByOriginal=true;
  }
}
let clipboardReference=null;
if(fs.existsSync(path.join(directory,'clipboard-reference-riff-sequence.json'))){
  const inputs=read('clipboard-reference-inputs.json'),riff=read('clipboard-reference-riff-sequence.json');
  if(inputs.length!==5||riff.reports.length!==3)throw Error('Incomplete clipboard reference inputs/outputs');
  for(const item of inputs)if(sha(path.join(repo,'work/producer/samples/QuickStart',item.name))!==item.sha256)throw Error('Clipboard source input changed');
  const expected=[[0,3072],[0],[0,6144]];
  for(const [i,r] of riff.reports.entries())if(sha(r.file)!==r.sha256||r.tempo.length!==1||
    JSON.stringify(r.tempo[0].events.map(e=>e.time))!==JSON.stringify(expected[i])||r.tempo[0].events.some(e=>e.bpm!==112))throw Error('Clipboard saved event mismatch');
  for(const action of ['clipboard_reference_paste_saved','clipboard_reference_cut_saved','clipboard_reference_repaste_saved'])
    if(!actions.some(r=>r.action===action&&r.tree?.includes('Segment: heartland')))throw Error('Missing clipboard UI action '+action);
  clipboardReference={copyPasteCutRepasteSaved:true,saved:riff.reports.map(r=>({sha256:r.sha256,events:r.tempo[0].events}))};
}
let clipboardCandidate=null;
if(fs.existsSync(path.join(directory,'clipboard-candidate-riff-sequence.json'))){
  const inputs=read('clipboard-candidate-inputs.json'),riff=read('clipboard-candidate-riff-sequence.json');
  const identity=read('clipboard-candidate-loaded-identity.json'),exit=read('clipboard-candidate-ui-exit.json');
  if(!clipboardTrial||identity.sha256!==clipboardTrial.candidateSha256||!identity.expectedCandidateLoaded||
    ![plan.app,plan.launchApp].some(app=>path.resolve(identity.modulePath).toLowerCase()===path.join(app,'TempoStripMgr.dll').toLowerCase())||
    exit.pid!==identity.pid||exit.processStillActive!==false||exit.candidateSha256!==identity.sha256)throw Error('Clipboard candidate DLL/exit identity mismatch');
  if(inputs.length!==5||riff.reports.length!==3)throw Error('Incomplete clipboard candidate inputs/outputs');
  for(const item of inputs)if(sha(path.join(repo,'work/producer/samples/QuickStart',item.name))!==item.sha256)throw Error('Clipboard candidate input changed');
  const expected=[[0,3072],[0],[0,6144]];
  for(const [i,r] of riff.reports.entries())if(sha(r.file)!==r.sha256||r.tempo.length!==1||
    JSON.stringify(r.tempo[0].events.map(e=>e.time))!==JSON.stringify(expected[i])||r.tempo[0].events.some(e=>e.bpm!==112))throw Error('Clipboard candidate saved events mismatch');
  const names=['paste_saved','cut_saved','repaste_saved','repaste_undo','repaste_redo','project_closed','project_reload_verified'];
  const observations=names.map(name=>{
    const r=actions.find(r=>r.action==='clipboard_candidate_'+name&&r.pid===identity.pid);
    if(!r)throw Error('Missing clipboard candidate observation '+name);
    return r;
  });
  const reopened=observations.at(-1);
  if(reopened.project!=='UiTest/ClipboardCandidate/QuickStart/QuickStart.pro'||reopened.bpm!==112||
    JSON.stringify(reopened.visibleTempoMeasures)!=='[1,3]'||
    sha(path.join(plan.app,'UiTest/ClipboardCandidate/QuickStart/heartland.sgp'))!==riff.reports[2].sha256)throw Error('Clipboard candidate reload identity mismatch');
  clipboardCandidate={copyPasteCutRepasteSaved:true,undoRedoObserved:true,projectReloadObserved:true,
    processExited:true,loadedIdentity:identity,saved:riff.reports.map(r=>({sha256:r.sha256,events:r.tempo[0].events})),
    originalInteroperabilityObserved:false,processRestartReloadObserved:false};
}
const summary={schema:3,trial:path.relative(repo,directory).replaceAll('\\','/'),executionUser:plan.executionUser,
  original:{basicTempoEditSaveProjectReload:true,processExited:true,observations,segments,sourceSampleFiles:inputs.length,sourceAppFiles:originalFiles.length},
  registry:{userRoots:87,userValues:471,machineView:32,machineValues:105,userRestored:fs.existsSync(path.join(directory,'restore.json'))?read('restore.json').allOwnedRootsAbsent:false,machineRestored:fs.existsSync(path.join(directory,'machine-restore.json'))?read('machine-restore.json').restoredAbsent:false},
  candidate,
  clipboardReference,clipboardTrial,clipboardCandidate,
  limitations:['Limited tempo operations only','Audio output unverified','Drag and multiple documents unverified',
    ...(clipboardCandidate?['Clipboard candidate process restart and original interoperability pending']:['Candidate clipboard UI unverified']),
    'Original Timeline clipboard Export leaves module counter increments; full native runs fail unload acceptance','Full Producer reconstruction incomplete']};
fs.writeFileSync(path.join(directory,'integration-summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify({originalBasicTrial:summary.original.basicTempoEditSaveProjectReload,candidateBasicTrial:summary.candidate.basicTempoEditSaveProjectReload??false,candidateRestartReload:summary.candidate.restartReloadObserved??false,candidateClipboardTrial:clipboardCandidate?.copyPasteCutRepasteSaved??false,userRestored:summary.registry.userRestored,machineRestored:summary.registry.machineRestored}));
