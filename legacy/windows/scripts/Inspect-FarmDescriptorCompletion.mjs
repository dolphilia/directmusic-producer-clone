import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {inspectFarmDescriptorProject} from './Inspect-FarmDescriptorProject.mjs';

const [unitArgument, output] = process.argv.slice(2);
assert(unitArgument && output); assert(!fs.existsSync(output), 'Preserve prior completion');
const repo = process.cwd(), unit = path.resolve(unitArgument);
const hash = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
function inputHash(file) {
  if (!fs.statSync(file).isDirectory()) return hash(file);
  const files = [];
  function walk(directory) {for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {const child = path.join(directory, entry.name); if (entry.isDirectory()) walk(child); else if (entry.isFile()) files.push(child);}}
  walk(file); files.sort((a, b) => a.localeCompare(b, 'en', {sensitivity: 'base'}));
  const lines = files.map(child => path.relative(file, child).replaceAll('\\', '/') + ':' + hash(child));
  return crypto.createHash('sha256').update(lines.join('\n')).digest('hex');
}
const json = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
const local = file => path.join(unit, file);
const ref = file => ({path: path.relative(repo, file).replaceAll('\\', '/'), sha256: hash(file)});
function bound(reference) {const file = path.resolve(repo, reference.path); assert.equal(hash(file), reference.sha256); return file;}
function cli(directory, driver, exitCode = 0) {
  const file = local(directory + '/run.json'), run = json(file);
  assert.equal(run.state, 'exited'); assert.equal(run.exitCode, exitCode);
  assert.equal(run.forcedTermination, false); assert(Number.isInteger(run.processId) && run.processId > 0);
  assert.equal(path.resolve(run.arguments[0]), path.resolve(driver));
  assert.equal(run.driverSha256, hash(driver)); assert.equal(run.nodeSha256, hash(run.node));
  assert.equal(run.wrapperSha256, hash(local(directory + '/wrapper.ps1')));
  return {...ref(file), processId: run.processId, exitCode};
}
const start = json(local('unit-start.json')), record = json(local('unit-record.json'));
const buildFile = bound(record.build), build = json(buildFile), candidate = path.basename(path.dirname(buildFile));
assert(build.passed && build.sourceSnapshotUnchanged);
for (const name of ['configureExitCode', 'buildExitCode', 'installExitCode']) assert.equal(build[name], 0);
for (const source of build.sources) {
  assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256, 'Immutable build source');
  assert.equal(hash(path.join(repo, source.path)), source.sha256, 'Current product source');
}
for (const item of build.outputs) assert.equal(hash(path.join(path.dirname(buildFile), item.path)), item.sha256);
const executable = path.join(build.installDirectory, 'bin/Producer.exe'), exeSha256 = hash(executable);
const authorFile = local('main-author-exit-observer.json'), restoreFile = local('restore-process-v1/exit-observer.json');
const author = json(authorFile), restore = json(restoreFile);
for (const process of [author, restore]) {
  assert.equal(process.executable, executable); assert.equal(process.exeSha256, exeSha256);
  assert.equal(process.state, 'exited'); assert.equal(process.exitCode, 0); assert.equal(process.forcedTermination, false);
  assert(Date.parse(process.startUtc) < Date.parse(process.exitUtc));
}
assert.notEqual(author.processId, restore.processId);
assert(Date.parse(author.exitUtc) < Date.parse(restore.startUtc), 'Author exits before distinct reload');
const savedFile = local('author-saved-inputs.json'), saved = json(savedFile); assert.equal(saved.inputs.length, 23);
for (const input of saved.inputs) {
  bound(input); assert.equal(hash(local('author-saved-v1/' + path.basename(input.path))), input.sha256, 'Preserved native input');
}
const native = inspectFarmDescriptorProject(local('native-inputs-v1'));
const authorActions = json(local('main-actions.json')), restoreActions = json(local('restore-process-v1/main-actions.json'));
const observations = json(local('restore-process-v1/main-observations.json'));
assert(authorActions.some(a => a.label === 'project-save-native'));
assert(authorActions.some(a => a.label === 'Author undo Script name change'));
assert(authorActions.some(a => a.label === 'Author redo Script name change'));
const open = restoreActions.find(a => a.label === 'restore-project-path');
assert.equal(open.args.text, native.project.path);
assert(restoreActions.some(a => a.label === 'restore-native-project'));
assert(restoreActions.some(a => a.label === 'restored-script-initialize'));
assert(observations.some(o => o.state.accessibility?.tree.includes('Value: FarmDescriptorSaved')));
assert(observations.some(o => o.state.accessibility?.tree.includes('Script operation succeeded.')));
assert(observations.some(o => o.state.accessibility?.tree.includes('Score initialized.')));
const related = json(local('related-regression-proof.json')), nativeRuns = [json(bound(related.relatedRun)), json(bound(related.namedRuntimeRun))];
const results = nativeRuns.flatMap(run => {
  assert.equal(run.candidate, candidate); assert.equal(run.buildSummarySha256, hash(buildFile));
  assert.equal(run.exeSha256, build.outputs.find(o => o.path.endsWith('producer_core_tests.exe')).sha256);
  for (const result of run.results) {
    assert.equal(result.status, '合格'); assert.equal(result.exitCode, 0); assert.equal(result.timedOut, false);
    assert(Number.isInteger(result.processId) && result.processId > 0 && result.checks > 0);
    for (const input of result.inputs) assert.equal(inputHash(input.path), input.sha256);
  }
  return run.results.map(r => ({id: r.id, processId: r.processId, checks: r.checks}));
});
assert.equal(results.length, 13); assert.equal(new Set(results.map(r => r.id)).size, 13);
const cliRuns = [
  cli('native-script-audit-cli-v1', 'scripts/Inspect-FarmDescriptorNative.mjs'),
  cli('native-project-audit-cli-v1', 'scripts/Inspect-FarmDescriptorProject.mjs'),
  cli('native-auditor-controls-cli-v1', 'scripts/Test-FarmDescriptorNativeAuditors.mjs'),
  cli('sfx-audio-audit-cli-v2', 'scripts/Inspect-FarmGuiAudio.mjs'),
  cli('sfx-audio-controls-cli-v2', 'scripts/Test-FarmGuiAudioAuditor.mjs'),
  cli('lifecycle-audio-audit-cli-v1', 'scripts/Inspect-FarmGuiLifecycle.mjs'),
  cli('audio-expired-cli-v1', 'scripts/Inspect-FarmGuiAudio.mjs', 1)
];
const controls = json(local('native-auditor-controls.json')); assert(controls.passed);
assert.equal(controls.controls.filter(c => c.accepted).length, 1); assert.equal(controls.controls.filter(c => !c.accepted).length, 9);
const sfx = json(local('sfx-audio-proof-v2.json')), life = json(local('lifecycle-audio-proof-v1.json'));
assert(sfx.isolatedSfxPassed && life.activeStopSilenceRestartPassed); assert.equal(life.phases.length, 8); assert(life.phases.every(p => p.passed));
assert.equal(sfx.bindings.processId, restore.processId); assert.equal(life.bindings.processId, restore.processId);
assert.equal(sfx.bindings.producerSha256, exeSha256); assert.equal(life.bindings.producerSha256, exeSha256);
const captures = ['audio-20261008T081848930Z', 'audio-20261008T082516768Z'].map((name, index) => {
  const dir = local('restore-process-v1/' + name), file = path.join(dir, 'run.json'), run = json(file), proof = index ? life : sfx;
  assert.equal(run.state, 'exited'); assert.equal(run.captureExitCode, 0);
  assert.equal(run.processId, restore.processId); assert.equal(run.exeSha256, exeSha256);
  assert.equal(run.buildSummarySha256, hash(buildFile)); assert.equal(run.recorderSha256, hash(run.recorder));
  assert.equal(run.inputs.length, 23); assert.deepEqual(run.inputs.map(i => i.sha256).sort(), saved.inputs.map(i => i.sha256).sort());
  for (const input of run.inputs) assert.equal(hash(input.path), input.sha256);
  for (const [key, name] of [['runSha256', 'run.json'], ['actionsSha256', 'actions.json'], [index ? 'pcmSha256' : 'wavSha256', 'output.wav'], ['packetSha256', 'packets.csv'], ['endpointSha256', 'endpoint.txt']]) assert.equal(proof.bindings[key], hash(path.join(dir, name)));
  return ref(file);
});
const sfxControls = json(local('sfx-audio-controls-v2.json'));
assert(sfxControls.passed && sfxControls.positiveRecordedControl); assert.equal(sfxControls.controls.length, 5); assert(sfxControls.controls.every(c => c.rejected));
const evidence = ['unit-start.json', 'author-saved-inputs.json', 'main-actions.json', 'main-observations.json', 'restore-process-v1/main-actions.json', 'restore-process-v1/main-observations.json', 'native-script-proof.json', 'native-project-proof.json', 'native-auditor-controls.json', 'related-regression-proof.json', 'sfx-audio-proof-v2.json', 'sfx-audio-controls-v2.json', 'lifecycle-audio-proof-v1.json', 'compile-failure-v1.json'].map(file => ref(local(file)));
const proof = {schema: 1, createdUtc: new Date().toISOString(), boundedCompletionPassed: true, candidate, build: ref(buildFile), executable: {path: executable, sha256: exeSha256}, sourcesMatched: build.sources.length, outputsMatched: build.outputs.length, inputsUnchangedAfterBothExits: saved.inputs.length, author: {...ref(authorFile), processId: author.processId, exitCode: 0}, reload: {...ref(restoreFile), processId: restore.processId, exitCode: 0}, native, relatedResults: results, captures, cliRuns, evidence, auditor: ref(process.argv[1]), scope: 'Owned exact Unicode NAME/CATEGORY class selection: new product, native Script/Project persistence, distinct reload, two normal exits, isolated six-SFX PCM and sounding Stop/quiet/restart. Controls and failed attempts retained.', remaining: start.remaining ?? record.remaining, originalComparisonVerified: false, wholeCandidateQ1Verified: false, fullAcceptance: false};
fs.mkdirSync(path.dirname(output), {recursive: true}); fs.writeFileSync(output, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
console.log(JSON.stringify({boundedCompletionPassed: true, candidate, sources: build.sources.length, inputs: saved.inputs.length, related: results.length, author: author.processId, reload: restore.processId, fullAcceptance: false}));
