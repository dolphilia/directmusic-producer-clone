// Verify a closed selection-fixed Tempo clipboard host round without loading DLLs.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const round=path.resolve(process.argv[2]??'');
const trial=path.dirname(path.dirname(round));
const within=(root,file)=>{const r=path.relative(root,file);return !!r&&!r.startsWith('..')&&!path.isAbsolute(r);};
if(!within(path.join(repo,'work/integration/user-trial'),round)||path.basename(path.dirname(round))!=='tempo-host-rounds')throw Error('Expected prepared host round');
const read=file=>JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const json=name=>read(path.join(round,name));
const sha=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const same=(a,b)=>path.resolve(a).toLowerCase()===path.resolve(b).toLowerCase();
const original='bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95';
const candidate='00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f';
const plan=read(path.join(trial,'plan.json')),replacement=json('replacement.json'),restoration=json('restoration.json'),identity=json('loaded-identity.json'),exit=json('exit.json');
if(plan.files.length!==42||plan.files.some(file=>sha(path.join(plan.app,file.name))!==file.sha256))throw Error('Original app manifest mismatch');
if(!replacement.completed||!same(replacement.trial,trial)||!same(replacement.round,round)||!same(replacement.target,path.join(plan.app,'TempoStripMgr.dll'))||
  replacement.planSha256!==sha(path.join(trial,'plan.json'))||replacement.originalSha256!==original||replacement.candidateSha256!==candidate||
  sha(path.join(round,'OriginalTempo.dll'))!==original||sha(path.join(round,'CandidateTempo.dll'))!==candidate||sha(replacement.target)!==original||
  sha(path.join(round,'Set-TempoHostRound.ps1'))!==replacement.scriptSha256||!restoration.restored||restoration.originalSha256!==original||
  restoration.replacementSha256!==sha(path.join(round,'replacement.json'))||identity.tempoSha256!==candidate||identity.pid!==exit.pid||exit.running!==false||
  !same(identity.exe,path.join(plan.app,'DMUSProd.exe'))||![plan.app,plan.launchApp].some(app=>same(identity.tempoPath,path.join(app,'TempoStripMgr.dll'))))throw Error('Round ownership, DLL identity, exit or restoration mismatch');
if(sha(replacement.comparison)!==replacement.comparisonSha256)throw Error('Native comparison changed');
const comparison=read(replacement.comparison);
if(!comparison.passed||!comparison.withPageSelection||comparison.comparedRecords!==6914||comparison.differences.length)throw Error('Native comparison did not pass');
for(const evidence of comparison.evidence){if(sha(path.join(evidence.directory,'run.json'))!==evidence.metadataSha256||sha(path.join(evidence.directory,'probe.jsonl'))!==evidence.logSha256)throw Error('Native evidence changed');}
const inputs=json('inputs.json');
if(inputs.length!==5||new Set(inputs.map(r=>r.name)).size!==5)throw Error('Incomplete input manifest');
for(const input of inputs){if(!same(input.source,path.join(repo,'work/producer/samples/QuickStart',input.name))||sha(input.source)!==input.sha256||!same(input.destination,path.join(plan.app,'UiTest/SelectionClipboard/QuickStart',input.name)))throw Error('Input identity mismatch');}
const actions=fs.readFileSync(path.join(round,'ui-actions.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse);
const quality=json('ui-log-quality.json');
if(quality.uiActionLogSha256!==sha(path.join(round,'ui-actions.jsonl'))||quality.entries!==actions.length||quality.treeEvidenceUsable!==false||quality.distinctTrees!==1||new Set(actions.map(r=>r.tree)).size!==1)throw Error('Unexpected UI log quality profile');
function action(name){const r=actions.find(r=>r.action===name&&r.pid===identity.pid);if(!r)throw Error('Missing UI action '+name);return r;}
const startup=action('frame_after_auto_ok'),project=action('selection_clipboard_project_opened'),segment=action('fresh_segment_opened');
// The retained UI action trees are stale; project/segment identity is checked
// against the independent AutoIt logs below. Do not use these trees for properties.
const toolRecords=[];
for(const [name,logName,operation] of [[startup.autoOkEvidence,'dialog.jsonl','dialog_ok'],[project.autoItEvidence,'dialog.jsonl','open_project'],[segment.treeEvidence,'tree.jsonl','tree_action']]){
  if(!/^[A-Za-z0-9-]+$/.test(name))throw Error('Invalid tool evidence path');
  const root=path.join(trial,name),run=read(path.join(root,'run.json'));
  if(run.pid!==identity.pid||run.exitCode!==0||run.launchError||run.timedOut||run.signature!=='Valid'||run.toolSha256!=='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'||!run.sources?.length)throw Error('External tool did not succeed');
  for(const source of run.sources){const sourceRoot=path.join(root,run.sourceSnapshot),file=path.resolve(sourceRoot,source.path);if(!within(sourceRoot,file)||sha(file)!==source.sha256)throw Error('Tool source snapshot changed');}
  const logs=fs.readFileSync(path.join(root,logName),'utf8').trim().split(/\r?\n/).map(JSON.parse);
  if(!logs.some(r=>r.operation===operation&&(operation==='tree_action'?r.action==='open':r.result===1&&r.dialogDismissed)))throw Error('External tool operation missing');
  if(operation==='open_project'&&(!same(run.project,inputs.find(r=>r.name==='QuickStart.pro').destination)||sha(path.join(root,run.projectBeforeSnapshot))!==inputs.find(r=>r.name==='QuickStart.pro').sha256))throw Error('Project input snapshot changed');
  if(operation==='tree_action'&&(!logs.some(r=>r.operation==='tree_item'&&r.path===run.item&&r.text==='heartland.sgp')||run.item!=='#5|#2'))throw Error('Observed fresh segment not identified');
  toolRecords.push({directory:name,metadataSha256:sha(path.join(root,'run.json')),logSha256:sha(path.join(root,logName))});
}
const files=['paste-second.sgp','cut-second.sgp','paste-third.sgp','undo-third.sgp','redo-third.sgp'];
const expected=[[0,3072],[0],[0,6144],[0],[0,6144]];
const riff=JSON.parse(execFileSync(process.execPath,[path.join(repo,'scripts/Inspect-SegmentRiff.mjs'),...files.map(f=>path.join(round,f))],{encoding:'utf8'}));
for(const [i,r] of riff.reports.entries()){
  action(['save_paste_second','save_cut_second','save_paste_third','save_undo_third','save_redo_third'][i]);
  if(r.tempo.length!==1||r.tempo[0].events.length!==expected[i].length||r.tempo[0].events.some((e,j)=>e.time!==expected[i][j]||e.bpm!==112))throw Error('Saved tempo events mismatch');
}
if(riff.reports[1].sha256!==riff.reports[3].sha256||riff.reports[2].sha256!==riff.reports[4].sha256||exit.segmentSha256!==riff.reports[4].sha256||sha(inputs.find(r=>r.name==='heartland.sgp').destination)!==exit.segmentSha256)throw Error('Undo/Redo or final file mismatch');
const summary={schema:1,round:path.relative(repo,round).replaceAll('\\','/'),candidateSha256:candidate,pid:identity.pid,toolRecords,
  copyPasteCutRepasteSaved:true,undoRedoBytesVerified:true,selectionPropertyRefreshVerified:false,uiTreeEvidenceUsable:false,processExited:true,originalDllRestored:true,
  snapshots:riff.reports.map((r,i)=>({file:files[i],sha256:r.sha256,bytes:r.bytes,events:r.tempo[0].events})),
  limitations:['UI action trees were stale; new-round property verification pending','Candidate process restart and original interoperability for these new files pending','Candidate audio output unverified','Non-tempo saved metadata equivalence unverified','Other original Producer modules required; full reconstruction incomplete']};
fs.writeFileSync(path.join(round,'summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify(summary));
