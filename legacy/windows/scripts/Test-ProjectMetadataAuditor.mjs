import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
const [runPath, outputDirectory] = process.argv.slice(2);
if (!runPath || !outputDirectory) throw new Error('Usage: run.json output-directory');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const directory = path.resolve(outputDirectory), auditor = path.resolve('scripts/Inspect-ProjectMetadata.mjs');
fs.mkdirSync(path.dirname(directory), { recursive: true }); fs.mkdirSync(directory, { recursive: false });
const sourceDirectory = path.join(path.dirname(path.resolve(runPath)), 'core', 'Metadata');
const cases = [];
for (const [name, mutate] of [
  ['unchanged', null],
  ['wrong-size', folder => { const file = path.join(folder, 'Metadata.pro'), bytes = fs.readFileSync(file), offset = bytes.indexOf(Buffer.from('filh')); if (offset < 0) throw new Error('Header missing'); bytes.writeUInt32LE(bytes.readUInt32LE(offset + 8 + 24) + 1, offset + 8 + 24); fs.writeFileSync(file, bytes); }],
  ['unknown-tail-changed', folder => { const file = path.join(folder, 'after-2.pro'), bytes = fs.readFileSync(file), offset = bytes.indexOf(Buffer.from('filh')); if (offset < 0) throw new Error('Header missing'); bytes[offset + 8 + 44] ^= 1; fs.writeFileSync(file, bytes); }],
]) {
  const caseDirectory = path.join(directory, name), folder = path.join(caseDirectory, 'core', 'Metadata');
  fs.mkdirSync(path.dirname(folder), { recursive: true }); fs.cpSync(sourceDirectory, folder, { recursive: true });
  const derivedRun = path.join(caseDirectory, 'run.json'); fs.copyFileSync(runPath, derivedRun); if (mutate) mutate(folder);
  const result = spawnSync(process.execPath, [auditor, derivedRun, path.join(caseDirectory, 'proof.json')], { encoding: 'utf8', windowsHide: true, timeout: 15000 });
  fs.writeFileSync(path.join(caseDirectory, 'stdout.txt'), result.stdout ?? ''); fs.writeFileSync(path.join(caseDirectory, 'stderr.txt'), result.stderr ?? '');
  if (result.error || (mutate ? result.status !== 1 : result.status !== 0)) throw new Error(`Unexpected auditor outcome: ${name}`);
  cases.push({ name, exitCode: result.status, expected: mutate ? 'rejected' : 'passed', derivedRunSha256: hash(derivedRun) });
}
const record = { schema: 1, createdUtc: new Date().toISOString(), passed: true, originalRun: path.resolve(runPath), originalRunSha256: hash(runPath), auditorSha256: hash(auditor), driverSha256: hash(process.argv[1]), cases, scope: 'Synthetic copies only; no product rerun or original-file modification', fullAcceptance: false };
fs.writeFileSync(path.join(directory, 'controls.json'), JSON.stringify(record, null, 2));
console.log(JSON.stringify(record));
