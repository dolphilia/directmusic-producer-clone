import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [directory, hostRun, mode = 'complete'] = process.argv.slice(2);
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
assert(['complete', 'natural-only', 'manual-only'].includes(mode));
const launch = json(directory + '/launch.json'), build = json(launch.buildSummary);
assert(launch.passed && launch.exitCode === 0 && !launch.timedOut && !launch.launchError);
assert(build.passed && build.configureExitCode === 0 && build.buildExitCode === 0 && build.installExitCode === 0);
assert.equal(hash(launch.buildSummary), launch.buildSummarySha256);
assert.equal(hash(launch.executable), launch.exeSha256);
assert.equal(hash(directory + '/launcher.ps1'), launch.driverSha256);
for (const source of build.sources) assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256);
for (const input of launch.inputs) assert.equal(hash(input.path), input.sha256);
const states = json(directory + '/playback-session-states.json');
const state = label => {const found = states.filter(s => s.label === label); assert.equal(found.length, 1, label); return found[0];};
const tree = label => {assert(state(label).accessibility, label + ' requires accessibility evidence'); return state(label).accessibility.tree;};
let previous = 0;
for (const s of states) {
  assert(Date.parse(s.createdUtc) >= previous); previous = Date.parse(s.createdUtc);
  assert.equal(s.window.app, states[0].window.app); assert(s.captures.length > 0);
  for (const c of s.captures) assert.equal(hash(directory + '/' + c.file), c.sha256);
}
const primary = 'Primary — primary.stp / Authored Motif — Playing';
const secondary = 'Secondary — secondary.stp / Authored Motif — Playing';
if (mode !== 'manual-only') {
assert(tree('both-playing-list').includes(primary) && tree('both-playing-list').includes(secondary));
const ended = tree('secondary-ended-main-ready');
assert(ended.includes(primary) && !ended.includes(secondary));
assert(ended.includes('Stopped (segment ended); other playback retained.'));
assert(!ended.includes('Playback did not start'));
assert(Date.parse(state('secondary-ended-main-ready').createdUtc) > Date.parse(state('both-playing-list').createdUtc));
}
if (mode !== 'natural-only') {
  if (mode === 'manual-only') assert(tree('primary-playing-list').includes(primary));
  const stopped = tree(mode === 'manual-only' ? 'manual-stop-main-final' : 'manual-stop-main-ready');
  assert(stopped.includes('テキスト Stopped. Style tempo:'));
  assert(!stopped.includes('other playback retained') && !stopped.includes('segment ended'));
  assert(!stopped.includes('一覧項目 (selectable) Primary') && !stopped.includes('一覧項目 (selectable) Secondary'));
  const list = mode === 'manual-only' ? tree('all-stopped-list-ready') : stopped;
  assert(!list.includes('一覧項目'));
  assert(list.includes('ボタン (disabled) Stop All ID: 102'));
  assert(list.includes('ボタン (disabled) Stop Selected ID: 101'));
}
const modules = json(directory + '/gui-modules.json'), host = json(hostRun);
assert.equal(modules.processId, launch.processId); assert.equal(modules.executable, launch.executable);
assert(host.cases.find(c => c.name === 'host-smoke').passed);
assert.equal(host.cases.find(c => c.name === 'host-smoke').sha256, launch.exeSha256);
fs.writeFileSync(directory + '/states.json', JSON.stringify({run: path.resolve(hostRun), build: launch.buildSummary, executable: launch.executable}, null, 2) + '\n');
const scope = {complete: 'Per-instance GUI natural end retaining primary and list Stop updating main status', 'natural-only': 'GUI natural end retaining primary only; manual Stop status failed and excluded', 'manual-only': 'GUI playing primary, list Stop All and main Stopped status only; no concurrent natural-end claim'};
const proof = {schema: 1, createdUtc: new Date().toISOString(), passed: true, scope: scope[mode], mode, build: launch.buildSummary, exeSha256: launch.exeSha256, sourceCount: build.sources.length, processId: launch.processId, statesSha256: hash(directory + '/playback-session-states.json'), auditorSha256: hash(process.argv[1]), workspaceIdentityChecked: false, limitations: ['Snapshot sources checked; workspace may have advanced', 'No GUI audio recording or actual start-clock measurement', 'Checkbox checked state inspected in screenshot, not exposed by UIA', 'No original comparison or full acceptance'], fullAcceptance: false};
fs.copyFileSync(process.argv[1], directory + '/playback-monitor-gui-auditor.mjs');
fs.writeFileSync(directory + '/playback-monitor-gui-proof.json', JSON.stringify(proof, null, 2) + '\n');
console.log(JSON.stringify(proof, null, 2));
