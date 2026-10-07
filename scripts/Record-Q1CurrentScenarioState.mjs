import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [unit, buildPath, nativePath, driverPath] = process.argv.slice(2);
assert(driverPath, 'Usage: Record-Q1CurrentScenarioState.mjs UNIT BUILD NATIVE DRIVER');
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const write = (p, v) => fs.writeFileSync(p, JSON.stringify(v, null, 2) + '\n');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const counts = rs => rs.reduce((a, r) => { a[r.status] = (a[r.status] ?? 0) + 1; return a; }, {});
const relative = p => path.relative(process.cwd(), p).replace(/\\/g, '/');
const proof = json(unit + '/scenario-proof.json'), candidate = proof.candidate, build = json(buildPath);
const native = json(nativePath), drivers = json(driverPath), utc = new Date().toISOString();
assert(proof.passed && !proof.fullAcceptance && build.passed);
assert.equal(native.candidate, candidate); assert.equal(drivers.candidate, candidate);
assert.equal(hash('scripts/Inspect-Q1CurrentScenario.mjs'), proof.auditorSha256);
assert(!fs.existsSync(unit + '/unit-record.json'), 'Promotion is one-shot; preserve history on a subsequent unit');
const audio = proof.audioProofs.map(p => ({...p, result: json(p.proof)}));
for (const p of audio) { assert.equal(hash(p.proof), p.proofSha256); assert.equal(p.result.passed, true); }
const residuals = [
  'Q1 representative five-form/native Project/audio chain closed on this candidate only; all40 responsibilities and all8 final acceptance remain incomplete',
  'Original dynamic/reciprocal comparison remains unobserved for Lyric/Marker bulk, Wave designer and other listed responsibilities; independent RIFF and PCM checks are not a substitute',
  'Common Timeline Marker/Mute and all remaining strips, snap, graphical selection/cycling, OLE/COM clipboard and meter reanchor remain incomplete',
  'Q2 independent Windows without original binaries, COM registration and lookup paths is unavailable; current source build and local load observations do not close original independence',
  'Current ordinary core is unexecuted/blocked by the frozen historical Windows5 publication condition; dedicated blocked modes and driver unexecuted entries are excluded from passes',
  'Producer ABI, runtime/export/cache/reference repair, media/generation/live/output/deployment residuals retained in feature responsibilities and prior units'
];
const nextAction = 'Q3G StylePlayer: observe the existing original auxiliary app and bundled text contract, preserve the existing Style/ChordMap/Band/Conductor implementations, then connect missing composition, audition and motif responsibilities in the source product. Retain Q2/original-designer blocks and all40/all8 scope; no unchanged Windows5 retry.';
const beforePaths = ['docs/analysis/product-state.json', 'docs/analysis/acceptance-status.json', 'docs/analysis/regression-manifest.json'];
for (const p of beforePaths) {
  const backup=unit + '/' + path.basename(p) + '.before-promotion';
  if(fs.existsSync(backup))assert.equal(hash(p),hash(backup),'State changed after failed promotion');
  else fs.copyFileSync(p,backup);
}
const manifest = json(beforePaths[2]), supplements = [];
for (const [id, p] of [['Test-ProductGuiProjectStyleDlsAuditor', proof.audioProofs[0]], ['Test-ProductGuiTransportDlsPriorityAuditor', proof.audioProofs[1]]]) {
  const test = manifest.drivers.find(t => t.id === id); assert(test);
  const runnerHash=hash(test.runner);
  if(runnerHash!==test.sha256){test.sourceHistory??=[];test.sourceHistory.push({sha256:test.sha256,reason:'Prior inventory hash retained; current executed driver source snapshot below'});test.sha256=runnerHash;}
  const savedRunner=unit+'/'+path.basename(test.runner);fs.copyFileSync(test.runner,savedRunner);assert.equal(hash(savedRunner),runnerHash);
  test.resultHistory ??= []; test.resultHistory.push(test.lastResult);
  test.history ??= []; test.history.push(test.latest);
  const result = {candidate, id, runner: test.runner, runnerSha256: runnerHash, savedRunner, status: '合格', evidence: p.controls, evidenceSha256: hash(p.controls), sourceCapture: p.capture, producingProof: p.proof, scope: 'Positive current PCM accepted and explicit derived negative controls rejected; product full acceptance separate', fullAcceptance: false};
  test.latest = test.lastResult = result; test.status = result.status; supplements.push(result);
}
const priorSupplements = manifest.latestDriverRound?.candidate === candidate ? (manifest.latestDriverRound.supplements ?? []) : [];
const combinedSupplements = [...priorSupplements.filter(p => !supplements.some(s => s.id === p.id)), ...supplements];
for (const s of combinedSupplements) assert.equal(hash(s.evidence), s.evidenceSha256);
const effectiveResults = drivers.results.map(r => combinedSupplements.find(s => s.id === r.id) ?? r);
const driverRound = {candidate, run: driverPath, rawCounts: counts(drivers.results), counts: counts(effectiveResults), supplements: combinedSupplements, total: manifest.drivers.length, fullAcceptance: false};
manifest.latestDriverRound = driverRound; manifest.updatedUtc = utc;
manifest.helperAuditors ??= [];
manifest.helperAuditors.push({candidate, auditor: 'scripts/Inspect-Q1CurrentScenario.mjs', sha256: proof.auditorSha256, result: unit + '/scenario-proof.json', scope: proof.scope, fullAcceptance: false});
write(beforePaths[2], manifest);
const nativeRound = {candidate, run: nativePath, counts: counts(native.results), total: manifest.tests.length};
const record = {schema: 2, createdUtc: utc, candidate, phase: 'Q1', featureIds: ['DMUSProd.exe','StyleDesigner.ocx','BandEditor.ocx','DLSDesigner.ocx','AudioPathDesigner.ocx','Conductor.dll','ScriptStripMgr.dll'],
  targetGap: 'Connect fresh native five-form history/save/normal exit/distinct reload with both Style and Transport default/embedded priority/sounding Stop/quiet/full restart/tempo audio on one candidate',
  endCondition: json(unit + '/unit-start.json').endCondition, status: '代表経路限定合格・全体未完', scope: proof.scope,
  changes: ['Fresh main-created native Project owns10 documents/six forms by final save; five-form edit/UndoRedo/save and two read-only-observed normal exits0', 'Control fixture root GUID separated; every other byte preserved and all owned Project identities independently checked unique', 'Scenario auditor validates actual author and reload Watch-ProductGuiExit evidence, hashes, build and complete frozen inputs', 'Serial current-candidate Style/Transport audio and6+11 negative controls; no product-source or recorder change'],
  build: {path: buildPath, sha256: hash(buildPath), savedSources: build.sources.length, configureExitCode: build.configureExitCode, buildExitCode: build.buildExitCode, installExitCode: build.installExitCode, outputs: build.outputs},
  core: {status: '障害あり', executed: false, reason: 'Frozen historical atomic-save Windows5; no unchanged retry'}, native: nativeRound, drivers: driverRound,
  gui: {passed: true, scenarioProof: unit + '/scenario-proof.json', projectSha256: proof.project.sha256, processes: proof.processes, historyObservations: proof.observations},
  audio: {passed: true, style: proof.audioProofs[0], transport: proof.audioProofs[1], serial: true, stopResumePassed: audio[1].result.stopResumePassed, sustainPassed: audio[1].result.sustainPassed, controlQuiet: audio[1].result.controlQuiet, intervals: audio[1].result.completed[0].intervals},
  preservedFailures: ['audio-preparation-diagnostic.json','capture-path-diagnostic.json','style-first-audit.stdout.txt','style-first-audit.stderr.txt','style-negative-controls/unchanged/stderr.txt','transport-audit.stdout.txt','transport-audit.stderr.txt'].filter(p => fs.existsSync(unit+'/'+p)).map(p => unit+'/'+p),
  original: {status: '障害あり', scope: 'Current Q1 source fixtures only; original same-scenario dynamic parity unexecuted'},
  q2: {status: '障害あり', reason: 'No independent original-free Windows environment'},
  evidence: [buildPath,nativePath,driverPath,unit+'/scenario-proof.json',unit+'/final-inputs.json',unit+'/author-normal-exit.json',unit+'/reload-exit.json',unit+'/author-exit-observer.json',unit+'/reload-exit-observer.json',...proof.audioProofs.flatMap(p=>[p.proof,p.controls])].map(p=>({path:relative(p),sha256:hash(p)})),
  inputIdentityLimitations:proof.inputIdentityLimitations,residuals,nextAction,fullAcceptance:false};
write(unit + '/unit-record.json', record);
const state = json(beforePaths[0]), c = state.current;
c.guiHistory ??= []; c.guiHistory.push({candidate:c.candidate,evidence:c.gui?.evidence,unit:c.latestUnit,archive:unit+'/product-state.json.before-promotion'});
c.unitHistory ??= []; c.unitHistory.push({candidate:c.candidate,unit:c.latestUnit,archive:unit+'/product-state.json.before-promotion'});
const q1 = {candidate,status:'代表経路限定合格・全体未完',evidence:unit+'/unit-record.json',scenarioProof:unit+'/scenario-proof.json',scope:proof.scope,fullAcceptance:false};
Object.assign(c,{latestUnit:unit+'/unit-record.json',latestReport:'docs/analysis/q1-final-candidate-2026-10-07.md',updatedUtc:utc,inProgressUnit:null,residuals,nextAction,nextIndependentUnit:nextAction,gui:{candidate,...record.gui,scope:proof.scope,fullAcceptance:false},audio:{candidate,status:'限定合格',...record.audio,scope:proof.scope,fullAcceptance:false},q1CurrentIntegration:q1,q1Integration:q1,q1FiveDocumentScenario:q1,integrationProgress:q1,driverRound,drivers:driverRound,registeredDrivers:driverRound,fullAcceptance:false});
write(beforePaths[0],state);
const acceptance=json(beforePaths[1]); acceptance.history ??= [];
acceptance.history.push({candidate,unit:acceptance.latestUnit,archive:unit+'/acceptance-status.json.before-promotion'});
Object.assign(acceptance,{updatedUtc:utc,latestUnit:unit+'/unit-record.json',residuals,driverRound,regressionRound:nativeRound,fullAcceptance:false});
const scopes={
  'clean-build':`${build.sources.length}保存ソースconfigure/build/install0。全40実装・Q2独立環境未完。`,
  'startup-shutdown':`新規native Project作者${proof.processes[0].processId}、最終別再読込${proof.processes[1].processId}が通常exit0。全編集画面・全責務不足。`,
  'original-data-load':'同じ候補の専用形式試験とQ1 source fixture読込。全形式の原版最小文書・意味比較は未完。',
  'edit-save':'五形式の変更・UndoRedo・native保存、最終6形式10文書所有と一意GUIDが限定成立。全strip・clipboard・ABI不足。',
  'separate-process-reload':`最終Project10文書と全12入力byte不変、別PID${proof.processes[1].processId}で五形式保存値とScriptを復元exit0。全責務保存復元不足。`,
  'play-stop-tempo-audio':'同じ最終Project/PIDでStyle109設定・Segment120BPMで10発2秒間隔とTransport既定無音/embedded優先/発音中Stop/無音/8音再開/5→7.5BPM/DLS持続合格。全再生責務・原版動的比較不足。',
  'repeat-invalid-input':'DWORD許可/予約値・Timeline等境界拒否不変は限定成立。通常core既知保存拒否と全対象異常入力・反復不足。',
  'original-independence':'Q2独立Windows未用意。全機能の原版依存解消未証明。'
};
for(const criterion of acceptance.criteria){criterion.productSha256=build.outputs.find(o=>o.path==='install/bin/Producer.exe').sha256;criterion.currentScope=criterion.scope=scopes[criterion.id];criterion.evidence=unit+'/unit-record.json';criterion.fullAcceptance=false;criterion.remaining=['repeat-invalid-input','original-independence'].includes(criterion.id)?criterion.currentScope:'代表経路の合格のみ。全40責務・原版互換・同じ最終構成の全8受入は未完。';}
write(beforePaths[1],acceptance);
fs.writeFileSync('docs/analysis/acceptance-status.md',`# 全体8受入の現在状態\n\n候補 \`${candidate}\`。[最新単位](../../${unit}/unit-record.json)。全体未完了、\`fullAcceptance=false\`。\n\n|受入|状態|現行範囲・不足|\n|---|---|---|\n${acceptance.criteria.map(c=>`|${c.name}|${c.status}|${c.currentScope}|`).join('\n')}\n\n保存source${build.sources.length}、configure/build/install各0。native${nativeRound.total}=${JSON.stringify(nativeRound.counts)}。driver${driverRound.total}原始一巡=${JSON.stringify(driverRound.rawCounts)}、同候補の${combinedSupplements.length}件補足後=${JSON.stringify(driverRound.counts)}。障害・未実行は合格に含めない。\n\nQ1新規native五形式・保存履歴・正常終了・別PID復元・Style/Transport音声の代表経路は限定合格。Q2と全40責務・原版動的比較・全8最終受入を維持する。\n`);
console.log(JSON.stringify({candidate,q1:'representative pass',native:nativeRound.counts,drivers:driverRound.counts,fullAcceptance:false}));
