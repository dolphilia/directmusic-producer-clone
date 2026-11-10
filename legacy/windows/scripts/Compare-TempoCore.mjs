// Compare the implementation with retained outputs from an original DLL run.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
if (process.argv.length !== 3) throw new Error('Usage: node scripts/Compare-TempoCore.mjs <reference run directory>');
const reference = path.resolve(process.argv[2]);
const metadata = JSON.parse(fs.readFileSync(path.join(reference, 'run.json'), 'utf8').replace(/^\uFEFF/, ''));
if (metadata.exitCode !== 0 || metadata.timedOut || !metadata.originalTimeline || metadata.launchError ||
    metadata.dllSha256 !== 'bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95')
  throw new Error('A successful original Timeline reference run is required');
const cases = new Map([
  ['single_137', 'single'], ['fractional_93_75', 'fractional'], ['multiple_events', 'multiple'],
  ['unsorted_events', 'unsorted'], ['duplicate_times', 'duplicate'], ['replace_with_single', 'replace'],
]);
const lines = file => fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, '').trim().split(/\r?\n/).filter(Boolean).map(JSON.parse);
const original = lines(path.join(reference, 'probe.jsonl'));
const exe = path.join(root, 'work/build/probes/Release/tempo_core_compare.exe');
const output = path.join(root, 'work/comparison/tempo', new Date().toISOString().replace(/[-:.]/g, ''));
fs.mkdirSync(output, { recursive: true });
const result = spawnSync(exe, [reference], { encoding: 'utf8', windowsHide: true, timeout: 15000 });
fs.writeFileSync(path.join(output, 'core.jsonl'), result.stdout ?? '');
fs.writeFileSync(path.join(output, 'stderr.txt'), result.stderr ?? '');
const checks = [];
let error = result.error?.message ?? null;
try {
  if (error || result.status !== 0) throw new Error(error ?? `Core comparison exited ${result.status}`);
  const core = lines(path.join(output, 'core.jsonl'));
  let current = null;
  let previousHr = null;
  const seen = new Set();
  for (const record of original) {
    if (record.operation === 'begin_stream_case') current = cases.get(record.case);
    if (record.operation === 'get_editor_tempo') previousHr = record.hresult;
    if (current && record.operation === 'editor_tempo') {
      const found = core.find(value => value.case === current && value.at === record.at);
      const same = previousHr === '0x00000000' && record.matches === true && found?.found === true &&
        found.time === record.time && found.tempo === record.tempo && found.next === record.next;
      checks.push({ case: current, at: record.at, same });
    }
    if (record.operation === 'end_stream_case') {
      if (current && record.passed) seen.add(current);
      current = null;
    }
  }
  if (seen.size !== cases.size || checks.length !== 24 || checks.some(check => !check.same))
    throw new Error('Missing or different original editor GetParam observations');
  if (!core.some(record => record.passed === true)) throw new Error('Core output lacks a successful completion record');
} catch (failure) { error = failure.message; }
const hash = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const inputs = ['run.json', 'probe.jsonl', ...fs.readdirSync(reference).filter(name => name.endsWith('.bin'))]
  .map(name => ({ path: path.join(reference, name), sha256: hash(path.join(reference, name)) }));
const sources = ['CMakeLists.txt', 'src/tempo/tempo_track.h', 'src/tempo/tempo_track.cpp',
  'tests/native/tempo_core_compare.cpp', 'scripts/Compare-TempoCore.mjs']
  .map(name => ({ path: name, sha256: hash(path.join(root, name)) }));
const report = { reference, createdUtc: new Date().toISOString(), exitCode: result.status, error,
  passed: error === null, queryChecks: checks, sources, inputs, executableSha256: hash(exe) };
fs.writeFileSync(path.join(output, 'comparison.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify({ passed: report.passed, queryChecks: checks.length, error, evidence: output }, null, 2));
if (error) process.exitCode = 1;
