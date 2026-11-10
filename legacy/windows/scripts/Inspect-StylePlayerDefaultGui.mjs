import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {inspectNative} from './Inspect-StylePlayerDefaultNative.mjs';
const root = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
assert(!fs.existsSync(output), 'Use a fresh proof path');
const read = p => fs.readFileSync(p), json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const native = inspectNative(root);
const author = json(root + '/author-launch.json'), reload = json(root + '/reload-launch.json');
const firstExit = json(root + '/author-exit.json'), secondExit = json(root + '/reload-exit.json');
assert.equal(secondExit.state, 'exited'); assert.equal(secondExit.exitCode, 0); assert(!secondExit.forcedTermination);
assert.equal(secondExit.processId, reload.processId); assert.notEqual(reload.processId, author.processId);
assert(Date.parse(firstExit.exitUtc) < Date.parse(reload.createdUtc));
assert.equal(reload.exeSha256, author.exeSha256); assert.equal(secondExit.exeSha256, author.exeSha256);
assert.equal(reload.buildSummarySha256, author.buildSummarySha256);
const observation = name => {
  const p = path.join(root, name + '.json'), s = json(p);
  assert.equal(s.window.app.toLowerCase(), ('process:' + reload.executable).toLowerCase());
  assert(Date.parse(s.observedUtc) > Date.parse(reload.createdUtc));
  return {path: p, sha256: hash(p), tree: s.accessibility.tree, window: s.window};
};
const generated = observation('restore/RestoredDefaultComposedConfirmed');
assert.match(generated.tree, /Value: Style default ChordMap/);
const status = /(Playing|Stopped); compositions (\d+); restarts (\d+); band changes (\d+); motifs (\d+); primary (\d+)/.exec(generated.tree);
assert(status && +status[2] >= 1 && +status[6] > 0, 'Restored unnamed Style must generate through its public default selection');
const selected = observation('restore/RestoredComposedSegment');
assert.equal(selected.window.id, reload.windowId);
assert.match(selected.tree, /Value: Composed\.sgp ID: 207/); assert.match(selected.tree, /120\.000000/);
const protocolPath = path.join(root, 'audio-protocol.json'), protocol = json(protocolPath);
assert.equal(protocol.exeSha256, reload.exeSha256);
for (const x of [protocol.build, protocol.nativeProof, protocol.savedInputs, ...protocol.inputs]) assert.equal(hash(path.resolve(x.path)), x.sha256);
assert(Date.parse(protocol.createdUtc) < Date.parse(json(root + '/playing-confirmed.json').observedUtc), 'Protocol must precede recording playback');
const transport = [];
for (const [name, pattern] of [['playing-confirmed', /Playing/], ['stopped-confirmed', /Stopped\./], ['replaying-confirmed', /Playing/], ['final-stopped-confirmed', /Stopped\./]]) {
  const s = observation(name); assert.equal(s.window.id, reload.windowId); assert.match(s.tree, pattern);
  transport.push({path: s.path, sha256: s.sha256});
}
const proof = {...native, kind: 'default-map-native-and-distinct-GUI-restore', reloadPid: reload.processId,
  reloadLaunchSha256: hash(root + '/reload-launch.json'), reloadExitSha256: hash(root + '/reload-exit.json'),
  observations: [{path: generated.path, sha256: generated.sha256}, {path: selected.path, sha256: selected.sha256}, ...transport],
  generation: {compositions: +status[2], primary: +status[6]}, protocolSha256: hash(protocolPath),
  nativeAuditorSha256: native.auditorSha256, auditorSha256: hash(fileURLToPath(import.meta.url)),
  scope: 'Source main author native save/exit0, distinct native project reload, actual default-map composition and transport, second normal exit0; PCM separately required',
  limitations: ['PCM independently required', 'Original dynamic parity and Q2 remain unverified'], fullAcceptance: false};
fs.writeFileSync(output, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
console.log(JSON.stringify({passed: true, authorPid: author.processId, reloadPid: reload.processId, files: native.ownedFiles.length, fullAcceptance: false}));
