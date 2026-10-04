import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {execFileSync} from 'node:child_process';
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
let originalPlayback=null;
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
  if(fs.existsSync(path.join(directory,'clipboard-original-reload-loaded-identity.json'))){
    const originalIdentity=read('clipboard-original-reload-loaded-identity.json');
    const originalReload=actions.find(r=>r.action==='clipboard_original_reload_verified'&&r.pid===originalIdentity.pid);
    const originalPath=actions.find(r=>r.action==='clipboard_original_reload_project_path_confirmed'&&r.pid===originalIdentity.pid);
    const restored=read('tempo-clipboard-restoration.json');
    if(!restored.restored||originalIdentity.pid===identity.pid||!originalIdentity.expectedOriginalLoaded||
      originalIdentity.sha256!==clipboardTrial.originalSha256||restored.originalSha256!==clipboardTrial.originalSha256||
      ![plan.app,plan.launchApp].some(app=>path.resolve(originalIdentity.modulePath).toLowerCase()===path.join(app,'TempoStripMgr.dll').toLowerCase())||
      !originalReload?.tree.includes('Value: 112.00 ID: 203')||JSON.stringify(originalReload.visibleTempoMeasures)!=='[1,3]'||
      originalReload.project!==reopened.project||!originalPath?.tree.includes('\\UiTest\\ClipboardCandidate\\QuickStart\\QuickStart.pro'))
      throw Error('Clipboard original interoperability identity mismatch');
    clipboardCandidate.originalInteroperabilityObserved=true;
    clipboardCandidate.originalLoadedIdentity=originalIdentity;
    const playback=actions.find(r=>r.action==='clipboard_original_playback_observed'&&r.pid===originalIdentity.pid);
    const audible=actions.find(r=>r.action==='clipboard_original_playback_human_audible'&&r.pid===originalIdentity.pid);
    const stopped=actions.find(r=>r.action==='clipboard_original_playback_stopped'&&r.pid===originalIdentity.pid);
    if(playback&&audible&&stopped){
      const originalExit=read('clipboard-original-reload-ui-exit.json');
      if(!playback.tree.includes('Elapsed 00:00:16.330')||!playback.tree.includes('Voices 17 Peak 19')||
        !audible.audibleOutputVerified||audible.source!=='User reply: 音楽が聞こえた'||
        !stopped.tree.includes('Voices 0 Peak 20')||originalExit.pid!==originalIdentity.pid||
        originalExit.processStillActive!==false||originalExit.savedSegmentSha256!==riff.reports[2].sha256||
        originalExit.originalTempoSha256!==clipboardTrial.originalSha256)throw Error('Original playback observation mismatch');
      originalPlayback={pid:originalIdentity.pid,cursorAndVoicesObserved:true,humanConfirmedAudible:true,
        stopped:true,processExited:true,scope:'Original DLLs playing candidate-saved segment; no recorded waveform comparison'};
    }
  }
  if(fs.existsSync(path.join(directory,'clipboard-candidate-restarted-loaded-identity.json'))){
    const restarted=read('clipboard-candidate-restarted-loaded-identity.json');
    const restartExit=read('clipboard-candidate-restarted-ui-exit.json');
    const samePath=(a,b)=>path.resolve(a).toLowerCase()===path.resolve(b).toLowerCase();
    if(restarted.pid===identity.pid||restarted.priorCandidatePid!==identity.pid||restarted.sha256!==identity.sha256||!restarted.expectedCandidateLoaded||
      ![plan.app,plan.launchApp].some(app=>samePath(restarted.modulePath,path.join(app,'TempoStripMgr.dll')))||
      !samePath(restarted.savedFile,path.join(plan.app,'UiTest/ClipboardCandidate/QuickStart/heartland.sgp'))||restarted.savedFileSha256!==riff.reports[2].sha256||
      restartExit.pid!==restarted.pid||restartExit.processStillActive!==false||restartExit.candidateSha256!==identity.sha256||
      restartExit.savedFileSha256!==restarted.savedFileSha256||!samePath(restartExit.savedFile,restarted.savedFile))throw Error('Clipboard restarted candidate identity mismatch');
    const reinstall=read(restarted.replacementEvidence);
    if(!reinstall.completed||reinstall.candidateSha256!==identity.sha256||reinstall.originalSha256!==clipboardTrial.originalSha256||
      reinstall.priorReplacementSha256!==sha(path.join(directory,'tempo-clipboard-replacement.json')))throw Error('Clipboard repeat replacement ownership mismatch');
    function evidence(name,logName,expectedPid=restarted.pid){
      if(!/^[A-Za-z0-9-]+$/.test(name))throw Error('Unexpected evidence directory');
      const root=path.join(directory,name),run=JSON.parse(fs.readFileSync(path.join(root,'run.json'),'utf8').replace(/^\uFEFF/,''));
      if(run.pid!==expectedPid||run.exitCode!==0||run.launchError||run.timedOut||run.signature!=='Valid'||
        run.toolSha256!=='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'||!run.sources?.length)throw Error('External-tool evidence failed');
      for(const source of run.sources){
        const snapshotRoot=path.join(root,run.sourceSnapshot),file=path.resolve(snapshotRoot,source.path),relative=path.relative(snapshotRoot,file);
        if(!relative||relative.startsWith('..')||path.isAbsolute(relative)||sha(file)!==source.sha256)throw Error('External-tool source snapshot changed');
      }
      return {run,records:fs.readFileSync(path.join(root,logName),'utf8').trim().split(/\r?\n/).map(JSON.parse)};
    }
    const autoOk=evidence(restarted.autoOkEvidence,'dialog.jsonl');
    if(autoOk.run.mode!=='click'||!autoOk.records.some(r=>r.operation==='dialog_ok'&&r.method==='AutoIt.ControlClick'&&r.result===1&&r.dialogDismissed))throw Error('Restart warning was not dismissed');
    const opened=evidence(restarted.projectOpenEvidence,'dialog.jsonl'),segment=evidence(restarted.segmentOpenEvidence,'tree.jsonl');
    const expectedProject=path.join(plan.app,'UiTest/ClipboardCandidate/QuickStart/QuickStart.pro');
    if(opened.run.mode!=='open'||!samePath(opened.run.project,expectedProject)||restartExit.projectBeforeSha256!==opened.run.projectSha256||
      sha(path.join(directory,restartExit.projectAfterSnapshot))!==restartExit.projectAfterSha256||
      restartExit.projectMetadataChanged!==(restartExit.projectBeforeSha256!==restartExit.projectAfterSha256)||
      !opened.records.some(r=>r.operation==='explicit_project_path'&&samePath(r.path,expectedProject))||
      !opened.records.some(r=>r.operation==='open_project'&&r.result===1&&r.dialogDismissed)||
      segment.run.action!=='open'||segment.run.expectedText!=='heartland.sgp'||
      !segment.records.some(r=>r.operation==='tree_item'&&r.path===segment.run.item&&r.text==='heartland.sgp')||
      !segment.records.some(r=>r.operation==='tree_action'&&r.action==='open'&&r.selected===segment.run.item))throw Error('Restarted project/segment opening not verified');
    const overview=actions.find(r=>r.action==='clipboard_candidate_restart_reloaded_overview'&&r.pid===restarted.pid);
    const meter3=actions.find(r=>r.action==='clipboard_candidate_restart_measure3_112'&&r.pid===restarted.pid);
    if(!overview?.tree.includes('Segment: heartland')||!overview.tree.includes('Value: 112.00 ID: 203')||!overview.screenshotInspected||
      JSON.stringify(overview.visibleTempoLabels)!==JSON.stringify([{measure:1,bpm:112},{measure:3,bpm:112}])||
      !meter3?.tree.includes('Tempo: Value: 112.00 ID: 223')||!meter3.tree.includes('Measure Value: 3 ID: 224')||
      sha(path.join(directory,'clipboard-candidate-restart-frame.png'))!==restartExit.overviewPngSha256)throw Error('Restarted tempo observations incomplete');
    const propertyPending=actions.find(r=>r.action==='clipboard_candidate_restart_selection_property_pending'&&r.pid===restarted.pid);
    if(propertyPending&&sha(path.join(directory,'clipboard-candidate-restart-property-selection.png'))!==restartExit.selectionPngSha256)throw Error('Pending property observation image changed');
    clipboardCandidate.processRestartReloadObserved=true;
    clipboardCandidate.restartedLoadedIdentity=restarted;
    clipboardCandidate.restartReadOnlyInputUnchanged=true;
    clipboardCandidate.restartUnchangedInputScope='heartland.sgp only; project .pro metadata changed';
    clipboardCandidate.restartProjectMetadataChanged=restartExit.projectMetadataChanged;
    clipboardCandidate.restartSelectionPropertyComparisonPending=Boolean(propertyPending?.originalComparisonPending);
    clipboardCandidate.restartScope='Same candidate in a different process reloaded unchanged saved segment; visible 112 BPM at measures 1 and 3, measure 3 properties verified. Selection/property refresh acceptance is separate.';
    if(fs.existsSync(path.join(directory,'clipboard-original-selection-loaded-identity.json'))){
      const original=read('clipboard-original-selection-loaded-identity.json'),exit=read('clipboard-original-selection-ui-exit.json');
      if(original.pid===restarted.pid||exit.pid!==original.pid||exit.processStillActive!==false||
        original.sha256!==clipboardTrial.originalSha256||exit.originalTempoSha256!==original.sha256||
        ![plan.app,plan.launchApp].some(app=>samePath(original.modulePath,path.join(app,'TempoStripMgr.dll')))||
        !samePath(original.executable,path.join(plan.app,'DMUSProd.exe'))||
        original.segmentSha256!==restarted.savedFileSha256||exit.segmentSha256!==original.segmentSha256||
        original.projectBeforeSha256!==exit.projectBeforeSha256||exit.projectBeforeSha256!==restartExit.projectAfterSha256)throw Error('Original selection comparison identity mismatch');
      const originalOk=evidence(original.autoOkEvidence,'dialog.jsonl',original.pid);
      const originalOpened=evidence(original.segmentOpenEvidence,'tree.jsonl',original.pid);
      if(!originalOk.records.some(r=>r.operation==='dialog_ok'&&r.result===1&&r.dialogDismissed)||
        originalOpened.run.action!=='open'||originalOpened.run.expectedText!=='heartland.sgp'||
        !originalOpened.records.some(r=>r.operation==='tree_action'&&r.selected===originalOpened.run.item&&r.action==='open'))throw Error('Original comparison opening failed');
      for(const [action,measure] of [['clipboard_original_selection_measure3',3],['clipboard_original_selection_measure1',1],['clipboard_original_selection_measure1_reopened',1]]){
        const record=actions.find(r=>r.action===action&&r.pid===original.pid),tree=record?.tree??record?.accessibility?.tree;
        if(!record?.screenshotInspected||record.visibleSelectedMeasure!==measure||record.propertyDisplayedMeasure!==measure||
          !tree?.includes(`Measure Value: ${measure} ID: 224`)||!tree.includes('Tempo: Value: 112.00 ID: 223'))throw Error('Original selection/property comparison incomplete');
      }
      for(const [fileKey,hashKey] of [['projectBeforeSnapshot','projectBeforeSha256'],['projectAfterSnapshot','projectAfterSha256'],['selectionPng','selectionPngSha256'],['reopenedPng','reopenedPngSha256']]){
        if(!/^[A-Za-z0-9.-]+$/.test(exit[fileKey])||sha(path.join(directory,exit[fileKey]))!==exit[hashKey])throw Error('Original selection snapshot changed');
      }
      const projectDiff=read('clipboard-original-project-metadata-comparison.json');
      const recomputed=JSON.parse(execFileSync(process.execPath,[path.join(repo,'scripts/Compare-SegmentRiff.mjs'),path.join(directory,exit.projectBeforeSnapshot),path.join(directory,exit.projectAfterSnapshot)],{encoding:'utf8'}));
      if(JSON.stringify(projectDiff)!==JSON.stringify(recomputed)||projectDiff.sameLeafChunks!==56||projectDiff.changes.length!==7)throw Error('Original project metadata comparison changed');
      clipboardCandidate.restartSelectionPropertyComparisonPending=false;
      clipboardCandidate.selectionPropertyMismatchObserved=true;
      clipboardCandidate.selectionPropertyComparison={originalPid:original.pid,candidatePid:restarted.pid,originalSelectedMeasure:1,originalDisplayedMeasure:1,candidateDisplayedMeasure:3,acceptancePassed:false,cause:'Not yet isolated; same-object SetObject contract must remain unchanged'};
      clipboardCandidate.originalProjectMetadataComparison={beforeSha256:exit.projectBeforeSha256,afterSha256:exit.projectAfterSha256,sameLeafChunks:56,changedLeafChunks:7,scope:'Original also rewrites project metadata. Candidate before bytes unavailable, so candidate changes cannot be fully explained.'};
    }
  }
}
let selectionFix=null;
if(fs.existsSync(path.join(directory,'selection-fixed-ui-exit.json'))){
  const replacement=read('tempo-selection-replacement.json'),identity=read('selection-fixed-loaded-identity.json');
  const exit=read('selection-fixed-ui-exit.json'),restoration=read('tempo-selection-restoration.json');
  const samePath=(a,b)=>path.resolve(a).toLowerCase()===path.resolve(b).toLowerCase();
  const expected='00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f';
  if(!replacement.completed||!samePath(replacement.trial,directory)||replacement.planSha256!==sha(path.join(directory,'plan.json'))||
    replacement.candidateSha256!==expected||identity.sha256!==expected||exit.candidateSha256!==expected||
    identity.pid!==exit.pid||exit.processStillActive!==false||!samePath(identity.executable,path.join(plan.app,'DMUSProd.exe'))||
    ![plan.app,plan.launchApp].some(app=>samePath(identity.modulePath,path.join(app,'TempoStripMgr.dll')))||
    !samePath(identity.savedFile,path.join(plan.app,'UiTest/ClipboardCandidate/QuickStart/heartland.sgp'))||
    !samePath(identity.savedFile,exit.savedFile)||identity.savedFileSha256!==exit.savedFileSha256||sha(exit.savedFile)!==exit.savedFileSha256||
    !restoration.restored||restoration.originalSha256!==replacement.originalSha256||restoration.replacementSha256!==sha(path.join(directory,'tempo-selection-replacement.json'))||
    sha(replacement.target)!==replacement.originalSha256||sha(replacement.backup)!==replacement.originalSha256||
    sha(replacement.candidateSnapshot)!==expected||sha(path.join(directory,replacement.sourceSnapshot))!==replacement.sourceSha256||
    sha(replacement.comparison)!==replacement.comparisonSha256)throw Error('Selection fix ownership/identity mismatch');
  const comparison=JSON.parse(fs.readFileSync(replacement.comparison,'utf8'));
  const nativeCandidate=JSON.parse(fs.readFileSync(path.join(comparison.candidate,'run.json'),'utf8').replace(/^\uFEFF/,''));
  if(!samePath(comparison.candidate,replacement.candidateRun)||nativeCandidate.dllSha256!==expected)throw Error('Selection native candidate identity mismatch');
  if(!comparison.passed||!comparison.withPageSelection||comparison.comparedRecords!==6914||comparison.byteChecks.length!==445||
    comparison.copyChecks.length!==123||comparison.imageChecks.length!==66||comparison.differences.length||
    comparison.byteChecks.some(r=>!r.same)||comparison.copyChecks.some(r=>!r.sameContent)||comparison.imageChecks.some(r=>!r.same))throw Error('Selection fix native comparison failed');
  for(const evidence of comparison.evidence){
    if(sha(path.join(evidence.directory,'run.json'))!==evidence.metadataSha256||sha(path.join(evidence.directory,'probe.jsonl'))!==evidence.logSha256)throw Error('Selection native evidence changed');
    const run=JSON.parse(fs.readFileSync(path.join(evidence.directory,'run.json'),'utf8').replace(/^\uFEFF/,''));
    if(run.exitCode!==0||run.systemClipboard||run.timedOut||run.launchError)throw Error('Selection fix native run failed');
  }
  const project=actions.find(r=>r.action==='selection_fixed_project_path'&&r.pid===identity.pid);
  if(!project?.screenshotInspected||!project.tree.includes('\\UiTest\\ClipboardCandidate\\QuickStart\\QuickStart.pro'))throw Error('Selection fix project not identified');
  for(const [action,measure] of [['selection_fixed_measure3',3],['selection_fixed_measure1',1],['selection_fixed_measure1_reopened',1]]){
    const r=actions.find(r=>r.action===action&&r.pid===identity.pid);
    if(!r?.screenshotInspected||r.visibleSelectedMeasure!==measure||r.propertyDisplayedMeasure!==measure||
      !r.tree.includes(`Measure Value: ${measure} ID: 224`)||!r.tree.includes('Tempo: Value: 112.00 ID: 223'))throw Error('Selection fix UI observation missing');
  }
  for(const [fileKey,hashKey] of [['selectionPng','selectionPngSha256'],['reopenedPng','reopenedPngSha256']]){
    if(!/^[A-Za-z0-9.-]+$/.test(exit[fileKey])||sha(path.join(directory,exit[fileKey]))!==exit[hashKey])throw Error('Selection fix screenshot changed');
  }
  for(const [name,logName] of [[identity.autoOkEvidence,'dialog.jsonl'],[identity.segmentOpenEvidence,'tree.jsonl']]){
    if(!/^[A-Za-z0-9-]+$/.test(name))throw Error('Unexpected selection tool evidence path');
    const root=path.join(directory,name),run=JSON.parse(fs.readFileSync(path.join(root,'run.json'),'utf8').replace(/^\uFEFF/,''));
    if(run.pid!==identity.pid||run.exitCode!==0||run.launchError||run.timedOut||run.signature!=='Valid'||
      run.toolSha256!=='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'||!run.sources?.length)throw Error('Selection tool execution not verified');
    for(const source of run.sources){
      const sourceRoot=path.join(root,run.sourceSnapshot),file=path.resolve(sourceRoot,source.path),relative=path.relative(sourceRoot,file);
      if(!relative||relative.startsWith('..')||path.isAbsolute(relative)||sha(file)!==source.sha256)throw Error('Selection tool source snapshot changed');
    }
    const logs=fs.readFileSync(path.join(root,logName),'utf8').trim().split(/\r?\n/).map(JSON.parse);
    if(logName==='dialog.jsonl'?!logs.some(r=>r.operation==='dialog_ok'&&r.method==='AutoIt.ControlClick'&&r.result===1&&r.dialogDismissed):
      !logs.some(r=>r.operation==='tree_item'&&r.path===run.item&&r.text==='heartland.sgp')||!logs.some(r=>r.operation==='tree_action'&&r.action==='open'&&r.selected===run.item))throw Error('Selection tool action failed');
  }
  selectionFix={candidateSha256:expected,pid:identity.pid,nativeComparisonPassed:true,comparedRecords:6914,normalFiles:445,copyFiles:123,images:66,
    hostSelectionPropertyRefreshVerified:true,propertyReopenVerified:true,segmentUnchanged:true,processExited:true,originalDllRestored:true,
    scope:'New candidate selection refresh only. Earlier clipboard/restart/audio results belong to other DLL hashes; full module acceptance remains pending.'};
}
const hostRoundRoot=path.join(directory,'tempo-host-rounds');
const selectionClipboardRounds=fs.existsSync(hostRoundRoot)?fs.readdirSync(hostRoundRoot,{withFileTypes:true})
  .filter(entry=>entry.isDirectory()&&fs.existsSync(path.join(hostRoundRoot,entry.name,'summary.json'))).sort((a,b)=>a.name.localeCompare(b.name))
  .map(entry=>JSON.parse(execFileSync(process.execPath,[path.join(repo,'scripts/Summarize-TempoHostRound.mjs'),path.join(hostRoundRoot,entry.name)],{encoding:'utf8'}))):[];
const summary={schema:7,trial:path.relative(repo,directory).replaceAll('\\','/'),executionUser:plan.executionUser,
  original:{basicTempoEditSaveProjectReload:true,processExited:true,observations,segments,sourceSampleFiles:inputs.length,sourceAppFiles:originalFiles.length},
  registry:{userRoots:87,userValues:471,machineView:32,machineValues:105,userRestored:fs.existsSync(path.join(directory,'restore.json'))?read('restore.json').allOwnedRootsAbsent:false,machineRestored:fs.existsSync(path.join(directory,'machine-restore.json'))?read('machine-restore.json').restoredAbsent:false},
  candidate,
  clipboardReference,clipboardTrial,clipboardCandidate,originalPlayback,selectionFix,selectionClipboardRounds,
  limitations:['Limited tempo operations only','Candidate DLL audio output unverified','Drag and multiple documents unverified',
    ...(clipboardCandidate?[...(!clipboardCandidate.processRestartReloadObserved?['Clipboard candidate process restart pending']:[]),
      ...(clipboardCandidate.restartSelectionPropertyComparisonPending?['Selection/property refresh after selecting first event needs original comparison']:[]),
      ...(clipboardCandidate.selectionPropertyMismatchObserved&&!selectionFix?['Candidate first-event selection leaves Measure 3; original updates to Measure 1. Cause and fix pending']:[]),
      ...(clipboardCandidate.restartProjectMetadataChanged?['Project .pro metadata also changes with original; candidate before bytes unavailable']:[]),
      ...(!clipboardCandidate.originalInteroperabilityObserved?['Clipboard original interoperability pending']:[])]:['Candidate clipboard UI unverified']),
    ...(selectionFix?[selectionClipboardRounds.length?'New selection-fixed candidate process restart, new-file original interoperability and audio acceptance pending':'New selection-fixed candidate system clipboard UI, process restart and audio acceptance pending']:[]),
    'Original Timeline clipboard Export leaves module counter increments; full native runs fail unload acceptance','Full Producer reconstruction incomplete']};
fs.writeFileSync(path.join(directory,'integration-summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify({originalBasicTrial:summary.original.basicTempoEditSaveProjectReload,candidateBasicTrial:summary.candidate.basicTempoEditSaveProjectReload??false,candidateRestartReload:summary.candidate.restartReloadObserved??false,candidateClipboardTrial:clipboardCandidate?.copyPasteCutRepasteSaved??false,clipboardCandidateRestartReload:clipboardCandidate?.processRestartReloadObserved??false,selectionPropertyComparisonPending:clipboardCandidate?.restartSelectionPropertyComparisonPending??false,newCandidateSelectionFixVerified:selectionFix?.hostSelectionPropertyRefreshVerified??false,userRestored:summary.registry.userRestored,machineRestored:summary.registry.machineRestored}));
