import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
const [sourceDirectory, preparation, outputDirectory] = process.argv.slice(2).map(p => path.resolve(p));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
fs.mkdirSync(path.dirname(outputDirectory), { recursive: true }); fs.mkdirSync(outputDirectory);
const auditor = path.resolve('scripts/Inspect-StyleProjectGui.mjs'), cases = [];
for (const [name, mutate] of [
  ['unchanged', null],
  ['style-unrelated-byte', dir => { const p = path.join(dir, 'edited.stp'), bytes = fs.readFileSync(p); bytes[bytes.length - 1] ^= 1; fs.writeFileSync(p, bytes); }],
  ['missing-dirty-prompt', dir => { const p = path.join(dir, '15-project-dirty-close-prompt.json'), record = JSON.parse(fs.readFileSync(p, 'utf8')); record.accessibility.tree = record.accessibility.tree.replaceAll('Discard unsaved documents?', 'No pending save'); fs.writeFileSync(p, JSON.stringify(record)); }],
]) {
  const dir = path.join(outputDirectory, name); fs.cpSync(sourceDirectory, dir, { recursive: true, filter: p => path.basename(p) !== 'style-project-gui-proof.json' }); if (mutate) mutate(dir);
  const result = spawnSync(process.execPath, [auditor, dir, preparation], { encoding: 'utf8', windowsHide: true, timeout: 15000 });
  fs.writeFileSync(dir + '/control.stdout.txt', result.stdout ?? ''); fs.writeFileSync(dir + '/control.stderr.txt', result.stderr ?? '');
  if (result.error || result.status !== (mutate ? 1 : 0)) throw Error('Unexpected control outcome: ' + name);
  if (fs.existsSync(dir + '/style-project-gui-proof.json') !== !mutate) throw Error('Stale proof in control: ' + name);
  cases.push({ name, exitCode: result.status, expected: mutate ? 'rejected' : 'passed' });
}
const record = { schema: 1, createdUtc: new Date().toISOString(), passed: true, sourceDirectory, preparation, preparationSha256: hash(preparation), auditorSha256: hash(auditor), driverSha256: hash(process.argv[1]), cases, scope: 'Synthetic evidence copies only; no GUI or product reexecution', fullAcceptance: false };
fs.writeFileSync(outputDirectory + '/controls.json', JSON.stringify(record, null, 2)); console.log(JSON.stringify(record));
