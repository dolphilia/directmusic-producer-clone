import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [directory, hostRun] = process.argv.slice(2).map(p => path.resolve(p));
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const launch = json(directory + '/launch.json'), build = json(launch.buildSummary);
assert(launch.passed && launch.exitCode === 0 && !launch.timedOut && !launch.launchError);
assert(build.passed && build.configureExitCode === 0 && build.buildExitCode === 0 && build.installExitCode === 0);
assert.equal(hash(launch.buildSummary), launch.buildSummarySha256);
assert.equal(hash(launch.executable), launch.exeSha256);
assert.equal(hash(directory + '/launcher.ps1'), launch.driverSha256);
for (const source of build.sources) {
  assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256);
  assert.equal(hash(path.resolve(source.path)), source.sha256);
}
for (const input of launch.inputs) assert.equal(hash(input.path), input.sha256);
const states = json(directory + '/playback-session-states.json');
const state = label => {
  const matches = states.filter(s => s.label === label);
  assert.equal(matches.length, 1, label);
  return matches[0];
};
const tree = label => state(label).accessibility.tree;
let previous = 0;
for (const s of states) {
  assert(Date.parse(s.createdUtc) >= previous);
  previous = Date.parse(s.createdUtc);
  assert.equal(s.window.app, states[0].window.app);
  assert(s.captures.length > 0);
  for (const capture of s.captures) assert.equal(hash(directory + '/' + capture.file), capture.sha256);
}
const primary = 'Primary — primary.stp / Authored Motif — Playing';
const secondary = 'Secondary — secondary.stp / Authored Motif — Playing';
for (const label of ['sessions-both-playing', 'sessions-primary-selected']) {
  assert.equal(state(label).window.title, 'Playback Sessions');
  assert(tree(label).includes(primary) && tree(label).includes(secondary));
  assert.equal((tree(label).match(/一覧項目/g) ?? []).length, 2);
}
assert(tree('sessions-secondary-retained').includes(secondary));
assert(!tree('sessions-secondary-retained').includes(primary));
assert.equal((tree('sessions-secondary-retained').match(/一覧項目/g) ?? []).length, 1);
const stopped = tree('sessions-all-stopped-ready');
assert(!stopped.includes('一覧項目'));
assert(stopped.includes('ボタン (disabled) Stop Selected ID: 101'));
assert(stopped.includes('ボタン (disabled) Stop All ID: 102'));
assert(stopped.includes('No playback sessions.'));
for (const [before, after] of [['restart-primary-play', 'restart-secondary-play'], ['restart-secondary-play', 'sessions-both-playing'], ['sessions-primary-selected', 'sessions-secondary-retained'], ['sessions-secondary-retained', 'sessions-all-stopped-ready']]) {
  assert(Date.parse(state(after).createdUtc) > Date.parse(state(before).createdUtc));
}
const modules = json(directory + '/gui-modules.json');
assert.equal(modules.processId, launch.processId);
assert.equal(modules.executable, launch.executable);
const host = json(hostRun);
assert(host.cases.find(c => c.name === 'host-smoke').passed);
assert.equal(host.cases.find(c => c.name === 'host-smoke').sha256, launch.exeSha256);
fs.writeFileSync(directory + '/states.json', JSON.stringify({run: hostRun, build: launch.buildSummary, executable: launch.executable}, null, 2) + '\n');
const proof = {
  schema: 1, createdUtc: new Date().toISOString(), passed: true,
  scope: 'Current-product simultaneous primary/secondary display, selected nonlatest primary Stop retaining secondary, all Stop and empty disabled controls',
  build: launch.buildSummary, exeSha256: launch.exeSha256, sourceCount: build.sources.length, processId: launch.processId,
  statesSha256: hash(directory + '/playback-session-states.json'), auditorSha256: hash(process.argv[1]),
  selectionEvidence: 'Primary row blue selection and secondary checkbox checked were inspected in saved screenshots. UIA does not expose these checked/selected states; this auditor checks the surrounding rows and resulting state transitions.',
  initialAttempt: 'The first finite playbacks ended before the list was observed. Its empty result and misleading retained-playback timeout remain recorded, without being counted as simultaneous verification.',
  limitations: ['No GUI audio recording', 'No GUI actual start clock measurement', 'Main status after list Stop All says segment ended rather than user stop', 'Retained playback start/timeout history is not tracked per instance', 'No original simultaneous GUI comparison', 'No full acceptance'],
  fullAcceptance: false
};
fs.copyFileSync(process.argv[1], directory + '/playback-sessions-gui-auditor.mjs');
fs.writeFileSync(directory + '/playback-sessions-gui-proof.json', JSON.stringify(proof, null, 2) + '\n');
console.log(JSON.stringify(proof, null, 2));
