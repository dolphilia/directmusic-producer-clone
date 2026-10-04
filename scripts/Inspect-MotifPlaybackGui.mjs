import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const [directory, hostRun] = process.argv.slice(2).map(p => path.resolve(p));
const read = p => fs.readFileSync(p);
const json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const launch = json(directory + '/launch.json');
assert(launch.passed && launch.exitCode === 0 && !launch.timedOut && !launch.launchError);
assert.equal(hash(launch.buildSummary), launch.buildSummarySha256);
assert.equal(hash(launch.executable), launch.exeSha256);
assert.equal(hash(directory + '/launcher.ps1'), launch.driverSha256);
const build = json(launch.buildSummary);
assert(build.passed && build.sourceSnapshotUnchanged);
for (const source of build.sources) {
  assert.equal(hash(path.join(build.sourceRoot, source.path)), source.sha256);
  assert.equal(hash(path.resolve(source.path)), source.sha256);
}
for (const input of launch.inputs) assert.equal(hash(input.path), input.sha256);
const states = json(directory + '/motif-band-states.json');
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
  for (const capture of s.captures) assert.equal(hash(directory + '/' + capture.file), capture.sha256);
}
assert(tree('dialog-second-refreshed').includes('Start boundary Value: Saved Motif boundary'));
assert(tree('dialog-second-refreshed').includes('Delay (music clocks) Value: 0 ID: 103'));
assert(tree('cancelled').includes('Stopped'));
assert(tree('delay-set').includes('Delay (music clocks) Value: 7680 ID: 103'));
assert(tree('play-submitted').includes('Scheduled Motif: Authored Motif'));
assert(tree('natural-end-refreshed').includes('Stopped (segment ended)'));
assert(Date.parse(state('natural-end-refreshed').createdUtc) > Date.parse(state('play-submitted').createdUtc));
const modules = json(directory + '/gui-modules.json');
assert.equal(modules.processId, launch.processId);
assert.equal(modules.executable, launch.executable);
assert.equal(json(hostRun).cases.find(c => c.name === 'host-smoke').sha256, launch.exeSha256);
fs.writeFileSync(directory + '/states.json', JSON.stringify({run: hostRun, build: launch.buildSummary, executable: launch.executable}, null, 2) + '\n');
const proof = {
  schema: 1, createdUtc: new Date().toISOString(), passed: true,
  scope: 'Current-product Motif playback dialog defaults/cancel/delay submission/natural end; source, executable, input, process and capture identity',
  build: launch.buildSummary, exeSha256: launch.exeSha256, processId: launch.processId,
  statesSha256: hash(directory + '/motif-band-states.json'), auditorSha256: hash(process.argv[1]),
  checkboxEvidence: 'Screenshot observation: preparation unchecked and secondary checked at delay-set. Checked states are not present in the UIA tree and are not machine-classified by this auditor.',
  limitations: ['No GUI audio recording', 'No actual start-clock measurement in GUI', 'No simultaneous primary/secondary playback', 'No saved-boundary rounding proof', 'No full acceptance'],
  fullAcceptance: false
};
fs.copyFileSync(process.argv[1], directory + '/playback-gui-auditor.mjs');
fs.writeFileSync(directory + '/motif-playback-gui-proof.json', JSON.stringify(proof, null, 2) + '\n');
console.log(JSON.stringify(proof, null, 2));
