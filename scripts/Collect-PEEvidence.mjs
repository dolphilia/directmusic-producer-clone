// Static evidence only. No inspected executable or DLL is loaded/executed.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const input = path.join(root, 'work/producer');
const output = path.join(root, 'work/analysis/pe');
const docs = path.join(root, 'docs/analysis');
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const hex = n => `0x${n.toString(16)}`;
const json = (p, data) => fs.writeFileSync(p, JSON.stringify(data, null, 2) + '\n');
const csv = value => '"' + String(value ?? '').replaceAll('"', '""') + '"';
function run(exe, args) {
  const r = spawnSync(exe, args, { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  if (r.error || r.status !== 0) throw new Error(`${exe}: ${r.error ?? r.stderr}`);
  return r.stdout;
}
const llvmVersion = run('llvm-readobj.exe', ['--version']);
const manifest = JSON.parse(fs.readFileSync(path.join(input, 'manifest.json'), 'utf8').replace(/^\uFEFF/, ''));
for (const f of manifest.files) {
  const actual = fs.readFileSync(path.join(input, f.destination));
  if (actual.length !== f.bytes || sha(actual) !== f.sha256) throw new Error(`Changed input: ${f.destination}`);
}
fs.mkdirSync(output, { recursive: true });
fs.mkdirSync(docs, { recursive: true });

function inspect(file, destination) {
  const b = fs.readFileSync(file);
  const check = (at, len) => {
    if (!Number.isSafeInteger(at) || at < 0 || at + len > b.length) throw new Error(`Invalid offset ${at} in ${file}`);
    return at;
  };
  const u16 = at => b.readUInt16LE(check(at, 2));
  const u32 = at => b.readUInt32LE(check(at, 4));
  const textZ = at => { check(at, 1); const end = b.indexOf(0, at); if (end < 0) throw new Error('Unterminated string'); return b.toString('latin1', at, end); };
  if (u16(0) !== 0x5a4d) throw new Error('Missing MZ signature');
  const pe = u32(0x3c);
  if (u32(pe) !== 0x4550) throw new Error('Missing PE signature');
  const opt = pe + 24;
  if (u16(opt) !== 0x10b) throw new Error('This collector currently requires PE32');
  const imageBase = u32(opt + 28), sectionCount = u16(pe + 6);
  const sections = [];
  for (let i = 0, off = opt + u16(pe + 20); i < sectionCount; i++, off += 40) {
    sections.push({ name: b.toString('ascii', off, off + 8).replace(/\0.*$/, ''),
      virtualSize: u32(off + 8), rva: u32(off + 12), rawSize: u32(off + 16),
      rawOffset: u32(off + 20), characteristics: hex(u32(off + 36)) });
  }
  function rvaOffset(rva) {
    if (rva < u32(opt + 60)) return check(rva, 1);
    const s = sections.find(s => rva >= s.rva && rva - s.rva < s.rawSize);
    if (!s) throw new Error(`RVA has no file bytes: ${hex(rva)}`);
    return check(s.rawOffset + rva - s.rva, 1);
  }
  const offsetRva = off => {
    const s = sections.find(s => off >= s.rawOffset && off - s.rawOffset < s.rawSize);
    return s ? hex(s.rva + off - s.rawOffset) : null;
  };
  const directory = n => ({ rva: u32(opt + 96 + 8 * n), size: u32(opt + 100 + 8 * n) });
  const imports = [], exports = [], resources = [], debug = [];
  const imp = directory(1);
  if (imp.rva) for (let p = rvaOffset(imp.rva); u32(p + 12); p += 20) {
    const dll = textZ(rvaOffset(u32(p + 12))), lookup = u32(p) || u32(p + 16), iat = u32(p + 16);
    for (let j = 0, t = rvaOffset(lookup); u32(t + j * 4); j++) {
      const entry = u32(t + j * 4);
      imports.push({ dll, name: entry & 0x80000000 ? null : textZ(rvaOffset(entry) + 2),
        ordinal: entry & 0x80000000 ? entry & 0xffff : null, iatRva: hex(iat + j * 4), iatVa: hex(imageBase + iat + j * 4) });
    }
  }
  const exp = directory(0);
  if (exp.rva) {
    const p = rvaOffset(exp.rva), base = u32(p + 16), functions = u32(p + 20), names = u32(p + 24), byIndex = new Map();
    for (let i = 0; i < names; i++) byIndex.set(u16(rvaOffset(u32(p + 36)) + i * 2), textZ(rvaOffset(u32(rvaOffset(u32(p + 32)) + i * 4))));
    for (let i = 0; i < functions; i++) {
      const rva = u32(rvaOffset(u32(p + 28)) + i * 4);
      if (rva) exports.push({ ordinal: base + i, name: byIndex.get(i) ?? null, rva: hex(rva),
        forwarder: rva >= exp.rva && rva < exp.rva + exp.size ? textZ(rvaOffset(rva)) : null });
    }
  }
  const res = directory(2), visited = new Set();
  let version = null;
  if (res.rva) {
    const start = rvaOffset(res.rva);
    function walk(relative, labels = []) {
      if (labels.length > 8 || visited.has(relative)) throw new Error('Invalid resource tree');
      visited.add(relative);
      const p = start + relative, count = u16(p + 12) + u16(p + 14);
      for (let i = 0; i < count; i++) {
        const name = u32(p + 16 + i * 8), target = u32(p + 20 + i * 8);
        const label = name & 0x80000000 ? b.toString('utf16le', start + (name & 0x7fffffff) + 2, start + (name & 0x7fffffff) + 2 + u16(start + (name & 0x7fffffff)) * 2) : String(name);
        const next = [...labels, label];
        if (target & 0x80000000) walk(target & 0x7fffffff, next);
        else {
          const data = start + target, rva = u32(data), size = u32(data + 4), off = rvaOffset(rva);
          check(off, size);
          const bytes = b.subarray(off, off + size), resourceFile = `resource-${resources.length.toString().padStart(4, '0')}.bin`;
          fs.writeFileSync(path.join(destination, resourceFile), bytes);
          const resource = { labels: next, rva: hex(rva), fileOffset: hex(off), size, codepage: u32(data + 8), file: resourceFile, sha256: sha(bytes) };
          if (next[0] === '6') {
            resource.strings = [];
            for (let j = 0, at = 0; j < 16; j++) {
              const length = bytes.readUInt16LE(at); at += 2;
              if (at + length * 2 > bytes.length) throw new Error('Truncated string resource');
              if (length) resource.strings.push({ id: (Number(next[1]) - 1) * 16 + j, text: bytes.toString('utf16le', at, at + length * 2) });
              at += length * 2;
            }
          }
          if (next[0] === 'REGISTRY') resource.text = bytes.toString('latin1').replace(/\0+$/, '');
          resources.push(resource);
          if (next[0] === '16') {
            const signature = bytes.indexOf(Buffer.from([0xbd, 0x04, 0xef, 0xfe]));
            if (signature >= 0 && signature + 16 <= bytes.length) {
              const ms = bytes.readUInt32LE(signature + 8), ls = bytes.readUInt32LE(signature + 12);
              version = `${ms >>> 16}.${ms & 0xffff}.${ls >>> 16}.${ls & 0xffff}`;
            }
          }
        }
      }
    }
    walk(0);
  }
  const dbg = directory(6);
  if (dbg.rva) for (let i = 0; i + 28 <= dbg.size; i += 28) {
    const p = rvaOffset(dbg.rva) + i, type = u32(p + 12), size = u32(p + 16), off = u32(p + 24);
    if (size) check(off, size);
    const signature = size >= 4 ? b.toString('ascii', off, off + 4) : null;
    let pdbPath = null;
    if (type === 2 && signature === 'RSDS' && size > 24) pdbPath = textZ(off + 24);
    if (type === 2 && signature === 'NB10' && size > 16) pdbPath = textZ(off + 16);
    debug.push({ type, size, rva: hex(u32(p + 20)), fileOffset: hex(off), signature, pdbPath });
  }
  const strings = [];
  for (let i = 0; i < b.length;) {
    const start = i;
    while (i < b.length && b[i] >= 32 && b[i] <= 126) i++;
    if (i - start >= 5) strings.push({ encoding: 'ascii', fileOffset: hex(start), rva: offsetRva(start), text: b.toString('ascii', start, i) });
    i++;
  }
  for (let parity = 0; parity < 2; parity++) for (let i = parity; i + 1 < b.length;) {
    const start = i;
    while (i + 1 < b.length && b[i] >= 32 && b[i] <= 126 && b[i + 1] === 0) i += 2;
    if (i - start >= 10) strings.push({ encoding: 'utf16le-ascii-range', fileOffset: hex(start), rva: offsetRva(start), text: b.toString('utf16le', start, i) });
    i += 2;
  }
  fs.writeFileSync(path.join(destination, 'strings.jsonl'), strings.map(s => JSON.stringify(s)).join('\n') + '\n');
  const record = { schema: 1, input: path.relative(root, file).replaceAll('\\', '/'), sha256: sha(b), bytes: b.length,
    machine: hex(u16(pe + 4)), architecture: u16(pe + 4) === 0x14c ? 'x86' : 'unknown', version,
    imageBase: hex(imageBase), entryRva: hex(u32(opt + 16)), coffTimestamp: u32(pe + 8),
    sections, imports, exports, resources, debug, stringCount: strings.length,
    stringScope: 'Printable ASCII runs and UTF-16LE ASCII-range runs; excludes non-ASCII text' };
  // Independent standard-tool view retained for cross-checking the parser.
  const llvm = run('llvm-readobj.exe', ['--file-headers', '--sections', '--coff-exports', '--coff-debug-directory', file]);
  fs.writeFileSync(path.join(destination, 'llvm-readobj.txt'), llvm);
  const importResult = spawnSync('llvm-readobj.exe', ['--coff-imports', file], { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  if (importResult.error || importResult.signal) throw new Error(`LLVM import execution failed: ${importResult.error ?? importResult.signal}`);
  fs.writeFileSync(path.join(destination, 'llvm-imports.txt'), importResult.stdout);
  record.llvmImportCheck = { exitCode: importResult.status, stderr: importResult.stderr.trim() };
  record.importScope = 'Normal import table; delay imports retained only in LLVM output when supported';
  if (!llvm.includes('Format: COFF-i386')) throw new Error('Unexpected LLVM architecture');
  const llvmImports = (importResult.stdout.match(/^Import \{/gm) ?? []).length;
  const llvmExports = [...llvm.matchAll(/^Export \{([\s\S]*?)^\}/gm)]
    .filter(m => Number.parseInt(m[1].match(/RVA: 0x([0-9a-f]+)/i)?.[1] ?? '0', 16) !== 0).length;
  if ((importResult.status === 0 && llvmImports !== new Set(imports.map(i => i.dll)).size) || llvmExports !== exports.length) throw new Error('LLVM directory count mismatch');
  json(path.join(destination, 'pe.json'), record);
  return record;
}

const records = [];
for (const entry of manifest.files.filter(f => /\.(exe|dll|ocx)$/i.test(f.destination))) {
  const relative = entry.destination.replaceAll('\\', '/');
  const destination = path.join(output, relative.replaceAll('/', '__'));
  fs.mkdirSync(destination, { recursive: true });
  const record = inspect(path.join(input, relative), destination);
  records.push(record);
  console.log(`${relative}: ${record.imports.length} imports, ${record.exports.length} exports, ${record.resources.length} resources`);
}
const columns = ['input', 'sha256', 'bytes', 'architecture', 'version', 'imageBase', 'entryRva', 'importDlls', 'importSymbols', 'exports', 'resources', 'debugTypes', 'pdbPaths'];
const rows = records.map(r => ({ ...r, importDlls: new Set(r.imports.map(i => i.dll)).size, importSymbols: r.imports.length,
  exports: r.exports.length, resources: r.resources.length, debugTypes: r.debug.map(d => d.type).join(';'), pdbPaths: r.debug.map(d => d.pdbPath).filter(Boolean).join(';') }));
fs.writeFileSync(path.join(docs, 'modules.csv'), columns.map(csv).join(',') + '\n' + rows.map(r => columns.map(c => csv(r[c])).join(',')).join('\n') + '\n');
json(path.join(output, 'collection.json'), { schema: 1, createdUtc: new Date().toISOString(), node: process.version,
  llvmVersion: llvmVersion.trim(), collectorSha256: sha(fs.readFileSync(fileURLToPath(import.meta.url))),
  manifestSha256: sha(fs.readFileSync(path.join(input, 'manifest.json'))), verifiedInputs: manifest.files.length,
  moduleCount: records.length, llvmImportExceptions: records.filter(r => r.llvmImportCheck.exitCode !== 0).map(r => ({ input: r.input, ...r.llvmImportCheck })),
  modules: records.map(r => ({ input: r.input, sha256: r.sha256 })) });
console.log(`Verified ${manifest.files.length} inputs and collected ${records.length} PE modules.`);
