import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';

const [unit, buildPath, nativePath, driverPath, guiPath] = process.argv.slice(2);
if (!guiPath) throw new Error('Usage: Record-LyricRangeState.mjs UNIT BUILD NATIVE DRIVER GUI_RESULT');
const read = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const write = (p, v) => fs.writeFileSync(p, JSON.stringify(v, null, 2) + '\n');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const relative = p => path.relative(process.cwd(), p).replace(/\\/g, '/');
const candidate = path.basename(path.dirname(buildPath));
const build = read(buildPath), native = read(nativePath), drivers = read(driverPath), gui = read(guiPath);
if (!build.passed || !build.sourceSnapshotUnchanged || native.candidate !== candidate || drivers.candidate !== candidate) throw new Error('Candidate identity mismatch');
for (const id of ['timeline-range','lyric-document','lyric-runtime','sequence-crud']) if (native.results.find(r => r.id === id)?.status !== '合格') throw new Error('Required regression did not pass: ' + id);
for (const source of build.sources) {
  if (hash(source.path) !== source.sha256 || hash(path.join(build.sourceRoot, source.path)) !== source.sha256) throw new Error('Source identity mismatch: ' + source.path);
}
for (const output of build.outputs) if (hash(path.join(path.dirname(buildPath), output.path)) !== output.sha256) throw new Error('Output identity mismatch');
const counts = results => results.reduce((a, r) => { a[r.status] = (a[r.status] ?? 0) + 1; return a; }, {});
const scope = 'Selected-group/nth Lyric joins Tempo/Sequence source Timeline range; full original bulk/Timeline/40 responsibilities incomplete';
const residuals = [
  'Original Lyric bulk selection/duplicate/overlap/meter reanchor dynamic comparison remains unobserved; independent byte checks are not an original oracle',
  'All Timeline strip types, snap, OLE/COM clipboard, selection cycling and original drag semantics remain incomplete',
  'Current candidate Q1 native five-format author/history/save/normal exit/distinct reload/Style/Transport/Stop/restart/tempo chain must be executed; old185539 Transport contamination failure retained',
  'Q2 independent original-free Windows build/install/Q1 environment unavailable',
  'Frozen ordinary core and other registered Windows5 publication refusals retained; no unchanged retries or pass claims',
  'Remaining all40 responsibility and all8 final configuration acceptance incomplete'
];
if (!gui.passed) residuals.push('Current candidate main Lyric range edit/history/save/separate-process GUI restore not verified: ' + gui.reason);
if (!gui.normalAuthorExitVerified) residuals.push('Author window/process disappeared after saved normal close; OS refused exit-code handle. Do not claim author exit0 or retry unchanged.');
const nextAction = 'Execute Q1 on candidate20261006T202431545Z: fresh native five-format author/history/save/normal exit/distinct reload, then serial WASAPI Style and Transport default/embedded priority with active Stop, quiet and full restart. Keep Q2 independent Windows and original dynamic blockers; next independent Q3C is Marker/Mute shared range from existing CRUD.';
const entry = {
  schema: 2, createdUtc: new Date().toISOString(), candidate, phase: 'Q3C',
  featureIds: ['LyricStripMgr.dll', 'Timeline.dll', 'DMUSProd.exe'],
  targetGap: 'Lyric absent from shared selection/copy/delete/merge/overwrite/relocation and single multi-strip history',
  endCondition: 'Saved-source build, related and registered native regressions, main UI connection and persistence; GUI/original unavailable steps explicitly blocked',
  scope, status: gui.passed ? '限定成立・原版bulk比較未完' : 'native限定成立・GUI/原版bulk比較障害',
  build: {path: buildPath, sha256: hash(buildPath), savedSources: build.sources.length, outputs: build.outputs, configureExitCode: build.configureExitCode, buildExitCode: build.buildExitCode, installExitCode: build.installExitCode},
  changes: [
    'Preserved and completed pre-existing unpromoted Lyric range implementation and interrupted saved builds',
    'Lyric physical half-open selection translates both independent clocks by one delta, preserving Unicode/delivery/extensions and opaque entry children',
    'Private multi-strip edits retain bytes/dirty/Redo on corrupt or out-of-range Lyric input, and commit one history entry',
    'Explicit selected-group/nth Lyric track and source TRNG lyrc content connected to main Timeline dialog',
    'Eight-acceptance status table regenerated from current JSON rather than stale Wave candidate Markdown'
  ],
  native: {run: relative(nativePath), counts: counts(native.results), results: native.results.filter(r => ['timeline-range','lyric-document','lyric-runtime','sequence-crud','core'].includes(r.id))},
  drivers: {run: relative(driverPath), counts: counts(drivers.results)},
  gui, original: {status: '障害あり', evidence: unit + '/original-window.json', reason: 'Original existing Segment designer did not open; no new dynamic bulk oracle'},
  evidence: [buildPath, nativePath, driverPath, guiPath, unit + '/unit-start.json', unit + '/original-window.json', ...(gui.evidence ?? [])].map(p => ({path: relative(p), sha256: hash(p)})),
  residuals, nextAction, fullAcceptance: false
};
write(unit + '/unit-record.json', entry);
const manifest = read('docs/analysis/regression-manifest.json');
const ordinary = manifest.tests.find(t=>t.id==='core');
if (ordinary.blockedByKnownOsRefusal?.reason.includes('Product runner same candidate')) {
  ordinary.blockedByKnownOsRefusal.previousReason = ordinary.blockedByKnownOsRefusal.reason;
  ordinary.blockedByKnownOsRefusal.reason = 'Historical candidate20261005T051458521Z atomic Chordmap Project replacement Windows5 after690 checks; destination retained. Unchanged publication condition frozen. Current candidate ordinary core unexecuted; historical separate product-run success is not transferred.';
}
for (const [kind, runPath, run] of [['tests', nativePath, native], ['drivers', driverPath, drivers]]) {
  for (const result of run.results) {
    const t = manifest[kind].find(t => t.id === result.id);
    if (!t) throw new Error('Unregistered result: ' + result.id);
    if (t.lastResult) { t.resultHistory ??= []; t.resultHistory.push(t.lastResult); }
    const normalized = result.id==='core' && result.processId===null ? {...result,error:ordinary.blockedByKnownOsRefusal.reason,rawRecordedReason:result.error} : result;
    t.lastResult = {candidate, run: relative(runPath), ...normalized}; t.status = result.status;
    if(t.latest) {t.history ??= []; t.history.push(t.latest);} t.latest = t.lastResult;
  }
}
manifest.updatedUtc = entry.createdUtc;
manifest.latestNativeRound = {candidate, run: relative(nativePath), counts: entry.native.counts};
manifest.latestDriverRound = {candidate, run: relative(driverPath), counts: entry.drivers.counts};
write('docs/analysis/regression-manifest.json', manifest);
for (const p of ['docs/analysis/product-state.json', 'docs/analysis/acceptance-status.json']) {
  const backup = unit + '/' + path.basename(p) + '.before-promotion';
  if (!fs.existsSync(backup)) fs.copyFileSync(p, backup);
}
const state = read('docs/analysis/product-state.json'), c = state.current;
c.unitHistory ??= []; c.unitHistory.push({candidate: c.candidate, unit: c.latestUnit, archive: unit + '/product-state.json.before-promotion'});
const previousGui = c.gui, previousAudio = c.audio;
Object.assign(c, {candidate, build: buildPath, latestUnit: unit + '/unit-record.json', latestReport: 'docs/analysis/q3-lyric-range-2026-10-07.md', updatedUtc: entry.createdUtc, residuals, nextAction, fullAcceptance: false});
for (const [field, exitCode] of [['configuration',build.configureExitCode],['compilation',build.buildExitCode],['install',build.installExitCode]]) c[field] = {candidate, status: exitCode === 0 ? '合格' : '失敗', exitCode, evidence: buildPath, scope: 'Saved-source Win32 Release, reference tools OFF; whole40/Q2 acceptance separate'};
c.core = {candidate, status: '障害あり', evidence: relative(nativePath), reason: 'Recorded unchanged Windows5 publication condition frozen', scope: 'Not executed and not included as pass'};
c.nativeDedicated = {candidate, run: relative(nativePath), counts: entry.native.counts, timelineRangeChecks: entry.native.results.find(r => r.id === 'timeline-range')?.checks, sequenceChecks: entry.native.results.find(r => r.id === 'sequence-crud')?.checks, fullAcceptance: false};
c.driverRound = manifest.latestDriverRound;
c.registeredDrivers = {...c.driverRound,fullAcceptance:false}; c.drivers = c.driverRound;
const smoke = drivers.results.find(r=>r.id==='Test-ProductHost');
c.smoke = {candidate,status:smoke?.status??'未実行',evidence:smoke?.evidence,scope:'Bounded product host smoke only; all40 not complete'};
c.gui = {candidate, ...gui, previousEvidence: previousGui?.evidence, scope};
c.audio = {candidate, status: '未実行', scope: 'New candidate; previous candidate audio retained as history only', historical: previousAudio, fullAcceptance: false};
c.q1CurrentIntegration = {candidate, status: '未実行', historicalUnit: 'work/analysis/q1-current-integration/20261006T192000Z/unit-record.json', fullAcceptance: false};
c.q1FiveDocumentScenario = c.q1CurrentIntegration; c.q1Integration = c.q1CurrentIntegration;
c.integrationProgress = {candidate,status:'作業中',scope}; c.nextIndependentUnit = nextAction;
const lyric = c.featureResponsibilities['LyricStripMgr.dll'];
lyric.history ??= []; lyric.history.push({evidence: lyric.evidence, scope: lyric.scope, remaining: lyric.remaining});
Object.assign(lyric, {candidate, status: '作業中', scope, evidence: unit + '/unit-record.json', remaining: residuals.slice(0,2)});
write('docs/analysis/product-state.json', state);
const acceptance = read('docs/analysis/acceptance-status.json');
acceptance.history ??= []; acceptance.history.push({candidate: acceptance.candidate, latestUnit: acceptance.latestUnit, archive: unit + '/acceptance-status.json.before-promotion'});
Object.assign(acceptance, {candidate, build: buildPath, latestUnit: unit + '/unit-record.json', updatedUtc: entry.createdUtc, fullAcceptance: false, residuals, regressionRound: manifest.latestNativeRound, driverRound: manifest.latestDriverRound});
for (const field of ['configuration','compilation','install']) acceptance[field] = c[field];
for (const criterion of acceptance.criteria) {
  criterion.candidate = candidate; criterion.evidence = unit + '/unit-record.json';
  criterion.status = ['repeat-invalid-input','original-independence'].includes(criterion.id) ? '障害あり' : '作業中';
  criterion.scope = criterion.id === 'play-stop-tempo-audio' ? 'New candidate audio unexecuted; prior Style success and Transport contamination failure archived' : scope;
  criterion.remaining = criterion.id === 'original-independence' ? residuals.find(r=>r.startsWith('Q2 ')) : criterion.id === 'play-stop-tempo-audio' ? residuals.find(r=>r.startsWith('Current candidate Q1')) : 'Current bounded results do not complete all40 duties or this all8 criterion; see unit residuals';
}
write('docs/analysis/acceptance-status.json', acceptance);
fs.writeFileSync('docs/analysis/acceptance-status.md', `# 全体8受入の現在状態\n\n候補 \`${candidate}\`。最新単位 [Lyric共通Timeline範囲](../../${unit}/unit-record.json)。全体未完了、\`fullAcceptance=false\`。\n\n|受入|状態|\n|---|---|\n${acceptance.criteria.map(c => `|${c.name}|${c.status}|`).join('\n')}\n\n構成/build/install各exit0。native ${JSON.stringify(entry.native.counts)}、driver ${JSON.stringify(entry.drivers.counts)}。障害・未実行は合格に含めない。\n\n${gui.passed ? '本体Lyric範囲操作と保存・別プロセス復元の限定証拠あり。' : '本体GUI操作は対象取得障害で未確認。'}新候補のQ1全経路・音声・原版bulk比較・Q2独立Windows・全40責務は未完。旧185539のStyle成功・Transport録音混入失敗は履歴として保持し、新生成物へ転用しない。\n`);
console.log(JSON.stringify({candidate, native: entry.native.counts, drivers: entry.drivers.counts, gui: gui.passed, fullAcceptance: false}));
