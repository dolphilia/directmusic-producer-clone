import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {verifySendProjectOpen} from './Inspect-AudioPathSendMainBindings.mjs';

const [launchPath, output] = process.argv.slice(2);
assert(output && !fs.existsSync(output), 'Usage: actual-visible-launch.json fresh-output');
fs.mkdirSync(output);
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const ref = p => ({path: path.resolve(p), sha256: crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex')});
const write = (p, v) => fs.writeFileSync(p, JSON.stringify(v, null, 2) + '\n', {flag: 'wx'});
const launch = json(launchPath), window = launch.selectedWindow, results = [];
const positive = (name, fn) => { fn(); results.push({name, expected: 'accept', passed: true}); };
const negative = (name, fn, pattern) => { let error; try { fn(); } catch (e) { error = e; } assert(error, 'Must reject ' + name); assert.match(error.message, pattern); results.push({name, expected: 'reject', passed: true, reason: error.message}); };
const changed = (label, key, mutate) => { const l = structuredClone(launch), v = json(l.projectOpen[key].path); mutate(v); const p = path.join(output, label + '.json'); write(p, v); l.projectOpen[key] = ref(p); return l; };
let error = null;
try {
  positive('Actual clean native GUI Open observations', () => verifySendProjectOpen(launch, launch.inputs, window));
  const wrongFiles = structuredClone(launch.inputs); wrongFiles[0].path = path.join(output, 'Wrong.pro');
  negative('Wrong Project input path', () => verifySendProjectOpen(launch, wrongFiles, window), /Exact native Project path typed/);
  negative('Invented command-line arguments', () => verifySendProjectOpen({...launch, arguments: ['--open-project', launch.inputs[0].path]}, launch.inputs, window), /Sky launch has no invented CLI arguments/);
  negative('Another selected window', () => verifySendProjectOpen({...launch, selectedWindow: {...window, id: window.id + 1}}, launch.inputs, window), /Launch selected window/);
  negative('Open before filename was observed', () => verifySendProjectOpen(changed('early-open', 'openAction', v => {v.beforeUtc = '2000-01-01T00:00:00Z';}), launch.inputs, window), /Open follows filename observation/);
  negative('Stale filename observation', () => verifySendProjectOpen(changed('stale-filename', 'filenameObservation', v => {v.observedUtc = '2000-01-01T00:00:00Z';}), launch.inputs, window), /Fresh observation after input/);
  negative('Restored another window', () => verifySendProjectOpen(changed('wrong-restored-window', 'restored', v => {v.window.id += 1;}), launch.inputs, window), /Observed selected window/);
  negative('Missing two-note restore', () => verifySendProjectOpen(changed('missing-notes', 'restored', v => {v.accessibility.tree = v.accessibility.tree.replaceAll('Notes: 2.', 'Notes: 0.');}), launch.inputs, window), /Native Project restored two notes/);
  negative('Native screenshot contradicts restored dimensions', () => verifySendProjectOpen(changed('wrong-dimensions', 'restored', v => {v.screenshots[0].width += 1;}), launch.inputs, window), /Screenshot width/);
  negative('Typed path hash mismatch', () => verifySendProjectOpen({...launch, projectOpen: {...launch.projectOpen, typedAction: {...launch.projectOpen.typedAction, sha256: '0'.repeat(64)}}}, launch.inputs, window), /Evidence reference hash/);
} catch (e) {error = {name: e.name, message: e.message, stack: e.stack};}
const proof = {schema: 1, createdUtc: new Date().toISOString(), passed: !error, kind: 'gui-project-open-validator-controls', results,
  positiveCount: results.filter(r => r.expected === 'accept').length, negativeCount: results.filter(r => r.expected === 'reject').length,
  actualObservationShape: ref(launchPath), auditor: ref('scripts/Inspect-AudioPathSendMainBindings.mjs'), controlDriver: ref('scripts/Test-AudioPathSendProjectOpenAuditor.mjs'),
  error, productCaptureExecuted: false, fullAcceptance: false, scope: 'Actual GUI Open record validates positive shape; mutated copies validate rejections only, never product acceptance'};
write(path.join(output, 'controls.json'), proof);
console.log(JSON.stringify({passed: proof.passed, positiveCount: proof.positiveCount, negativeCount: proof.negativeCount, error: error?.message}));
if (error) process.exitCode = 1;
