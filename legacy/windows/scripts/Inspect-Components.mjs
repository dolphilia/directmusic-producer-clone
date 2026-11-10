import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const peRoot = path.join(root, 'work/analysis/pe');
const hex = n => `0x${n.toString(16)}`;
const csv = x => '"' + String(x ?? '').replaceAll('"', '""') + '"';
const modules = fs.readdirSync(peRoot, { withFileTypes: true }).filter(f => f.isDirectory()).map(f => {
  const dir = path.join(peRoot, f.name), pe = JSON.parse(fs.readFileSync(path.join(dir, 'pe.json')));
  return { dir, pe, bytes: fs.readFileSync(path.join(root, pe.input)) };
});
const classes = [];
for (const { pe } of modules) for (const r of pe.resources.filter(r => r.text)) {
  for (const match of r.text.matchAll(/ForceRemove\s+\{([0-9a-f-]{36})\}\s*=\s*s\s*'([^']*)'/ig)) {
    classes.push({ module: pe.input, sha256: pe.sha256, clsid: match[1].toUpperCase(), description: match[2],
      threadingModel: r.text.match(/ThreadingModel\s*=\s*s\s*'([^']*)'/i)?.[1] ?? '',
      resourceRva: r.rva, resourceFile: path.relative(root, path.join(peRoot, pe.input.slice('work/producer/'.length).replaceAll('/', '__'), r.file)).replaceAll('\\', '/') });
  }
}
const guidBytes = s => {
  const h = s.replaceAll('-', ''), result = Buffer.from(h, 'hex');
  result.subarray(0, 4).reverse(); result.subarray(4, 6).reverse(); result.subarray(6, 8).reverse(); return result;
};
const locations = [];
for (const { pe, bytes } of modules) {
  const base = Number(pe.imageBase), sections = pe.sections;
  const offset = va => {
    const rva = va - base, section = sections.find(s => rva >= s.rva && rva - s.rva < s.rawSize);
    if (!section) throw new Error('Pointer outside mapped bytes');
    return section.rawOffset + rva - section.rva;
  };
  const rvaOf = at => {
    const s = sections.find(s => at >= s.rawOffset && at - s.rawOffset < s.rawSize);
    return s ? s.rva + at - s.rawOffset : null;
  };
  const u32 = at => bytes.readUInt32LE(at);
  const ptrRefs = value => {
    const needle = Buffer.alloc(4); needle.writeUInt32LE(value);
    const found = [];
    for (let at = bytes.indexOf(needle); at >= 0; at = bytes.indexOf(needle, at + 1)) if (at % 4 === 0) found.push(at);
    return found;
  };
  for (const c of classes) {
    const needle = guidBytes(c.clsid);
    for (let at = bytes.indexOf(needle); at >= 0; at = bytes.indexOf(needle, at + 1)) {
      const rva = rvaOf(at);
      locations.push({ module: pe.input, clsid: c.clsid, owner: c.module, rva: rva === null ? null : hex(rva), fileOffset: hex(at) });
    }
  }
  const stringsPath = path.join(peRoot, pe.input.slice('work/producer/'.length).replaceAll('/', '__'), 'strings.jsonl');
  const strings = fs.readFileSync(stringsPath, 'utf8').trim().split('\n').filter(Boolean).map(s => JSON.parse(s));
  const types = strings.filter(s => s.encoding === 'ascii' && /^\.\?A[UV]/.test(s.text) && s.rva)
    .map(s => ({ name: s.text, offset: Number(s.fileOffset) - 8, rva: Number(s.rva) - 8 }));
  const nameByVa = new Map(types.map(t => [base + t.rva, t.name]));
  const findings = [];
  for (const t of types) for (const ref of ptrRefs(base + t.rva)) {
    const col = ref - 12;
    try {
      if (col < 0 || u32(col) !== 0 || u32(col + 4) > 0xffff || u32(col + 8) > 0xffff) continue;
      const ch = offset(u32(col + 16)), count = u32(ch + 8);
      if (u32(ch) !== 0 || count < 1 || count > 128) continue;
      const array = offset(u32(ch + 12)), bases = [];
      for (let i = 0; i < count; i++) {
        const bc = offset(u32(array + i * 4));
        bases.push({ name: nameByVa.get(u32(bc)) ?? hex(u32(bc)), containedBases: u32(bc + 4),
          memberDisplacement: bytes.readInt32LE(bc + 8), vbtableDisplacement: bytes.readInt32LE(bc + 12),
          virtualDisplacement: bytes.readInt32LE(bc + 16), attributes: hex(u32(bc + 20)) });
      }
      const colRva = rvaOf(col);
      if (colRva === null) continue;
      const tables = [];
      for (const loc of ptrRefs(base + colRva)) {
        const functions = [];
        for (let at = loc + 4; at + 4 <= bytes.length && functions.length < 128; at += 4) {
          const candidate = u32(at) - base;
          if (!sections.some(s => (Number(s.characteristics) & 0x20000000) && candidate >= s.rva && candidate - s.rva < s.virtualSize)) break;
          const target = offset(u32(at));
          // Old images put read-only data in executable .text. Do not consume
          // the next RTTI locator and adjacent vtable as method pointers.
          if (target + 20 <= bytes.length && u32(target) === 0 && nameByVa.has(u32(target + 12))) break;
          functions.push(hex(candidate));
        }
        if (functions.length) tables.push({ rva: hex(rvaOf(loc + 4)), candidateFunctionRvas: functions,
          caveat: 'Consecutive executable-address pointers; actual vtable length requires interface/disassembly verification' });
      }
      if (tables.length) findings.push({ class: t.name, typeDescriptorRva: hex(t.rva), completeObjectLocatorRva: hex(colRva),
        subobjectOffset: u32(col + 4), bases, tables });
    } catch { /* Reject a pointer pattern that does not form a valid RTTI hierarchy. */ }
  }
  const dir = path.join(peRoot, pe.input.slice('work/producer/'.length).replaceAll('/', '__'));
  fs.writeFileSync(path.join(dir, 'rtti.json'), JSON.stringify({ input: pe.input, sha256: pe.sha256, findings }, null, 2) + '\n');
}
const columns = ['module', 'sha256', 'clsid', 'description', 'threadingModel', 'resourceRva', 'resourceFile'];
fs.writeFileSync(path.join(root, 'docs/analysis/com-classes.csv'), columns.map(csv).join(',') + '\n' + classes.map(c => columns.map(k => csv(c[k])).join(',')).join('\n') + '\n');
fs.writeFileSync(path.join(peRoot, 'class-guid-locations.json'), JSON.stringify(locations, null, 2) + '\n');
console.log(`Extracted ${classes.length} registered class records; ${locations.length} binary GUID occurrences.`);
