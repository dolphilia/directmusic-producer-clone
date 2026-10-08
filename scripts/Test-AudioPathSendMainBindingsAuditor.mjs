import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {chunks, inspectNative} from './Inspect-AudioPathSendNative.mjs';
import {checkedReference, inspectSendCase, serializeChunk, verifySendLifecycle, verifySendObservation} from './Inspect-AudioPathSendMainBindings.mjs';

const [authorRootArg, scopePath, output, observationIndexPath] = process.argv.slice(2); assert(output, 'Usage: actual-author-root binding-scope fresh-output [observation-index]');
assert(!fs.existsSync(output), 'Preserve earlier controls'); fs.mkdirSync(output);
const authorRoot = path.resolve(authorRootArg), author = path.join(authorRoot, 'author', 'MultiCapture');
const read = p => fs.readFileSync(p), json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex'), ref = p => ({path: path.resolve(p), sha256: hash(p)});
const write = (p, o) => fs.writeFileSync(p, JSON.stringify(o, null, 2) + '\n', {flag: 'wx'});
const observationFiles = observationIndexPath
  ? ['observation', 'png', 'action'].map(k => checkedReference(json(observationIndexPath)[k]))
  : ['AuthorSendSegmentSaved.json', 'AuthorSendSegmentSaved-0.png', 'AuthorSendSegmentSaved-action.json'].map(n => path.join(authorRoot, n));
const scope = json(scopePath), protocol = json(scope.protocol.path); assert.equal(hash(scope.protocol.path), scope.protocol.sha256);
const one = (xs, id, type = '') => { const f = xs.filter(c => c.id === id && c.type === type); assert.equal(f.length, 1); return f[0]; };
const native = (p, type) => one(chunks(read(p)), 'RIFF', type);
function leaf(id, data) { const h = Buffer.alloc(8); h.write(id); h.writeUInt32LE(data.length, 4); return one(chunks(Buffer.concat([h, data, ...(data.length & 1 ? [Buffer.alloc(1)] : [])])), id); }
function fixture(label, name, mutation) {
  const dir = path.join(output, label, 'MultiCapture'); fs.mkdirSync(dir, {recursive: true});
  const ap = native(path.join(author, 'SendMinus600.aup'), 'DMAP');
  const groups = ap.children.filter(c => c.id === 'LIST' && c.type === 'dbfl');
  const fxs = one(one(groups[1].children, 'RIFF', 'DSBC').children, 'LIST', 'fxls'), send = fxs.children[1];
  one(send.children, 'data').data.writeInt32LE(name === 'dry-zero' ? 0 : -600, 0);
  if (name.includes('waves')) {
    const h = Buffer.alloc(56); Buffer.from('6802fc87559a604395aa004a1d9de26c', 'hex').copy(h, 4);
    const fx = {id: 'RIFF', type: 'DSFX', children: [leaf('fxhr', h)], raw: Buffer.alloc(12)};
    fxs.children.splice(name.startsWith('send-before') ? 2 : 1, 0, fx);
  }
  if (mutation) mutation({ap, groups, fxs, send});
  const audioPath = path.join(dir, 'Input.aup'); fs.writeFileSync(audioPath, serializeChunk(ap), {flag: 'wx'});
  const segment = native(path.join(author, 'SendMinus600.sgp'), 'DMSG');
  segment.children[segment.children.indexOf(one(segment.children, 'RIFF', 'DMAP'))] = one(chunks(read(audioPath)), 'RIFF', 'DMAP');
  const segmentPath = path.join(dir, 'Input.sgp'); fs.writeFileSync(segmentPath, serializeChunk(segment), {flag: 'wx'});
  for (const name of ['RouteBand.bnp', 'RouteSource.dls']) fs.copyFileSync(path.join(author, name), path.join(dir, name), fs.constants.COPYFILE_EXCL);
  const names = ['Input.sgp', 'RouteBand.bnp', 'Input.aup', 'RouteSource.dls'];
  const project = native(path.join(authorRoot, 'before', 'MultiCapture', 'MultiCapture.pro'), 'JAZP');
  const entries = project.children.filter(c => c.id === 'LIST' && c.type === 'file'); assert.equal(entries.length, 4);
  for (let i = 0; i < entries.length; i++) {
    const e = entries[i], oldName = one(e.children, 'name');
    e.children[e.children.indexOf(oldName)] = leaf('name', Buffer.from(names[i] + '\0', 'utf16le'));
    const p = path.join(dir, names[i]), document = native(p, ['DMSG', 'DMBD', 'DMAP', 'DLS '][i]);
    const filh = one(e.children, 'filh').data; filh.writeUInt32LE(read(p).length, 24);
    one(document.children, i === 3 ? 'dlid' : 'guid').data.copy(filh, 28);
  }
  const projectPath = path.join(dir, 'MultiCapture.pro'); fs.writeFileSync(projectPath, serializeChunk(project), {flag: 'wx'});
  return [projectPath, segmentPath, path.join(dir, names[1]), audioPath, path.join(dir, names[3])].map((p, i) => ({role: ['project', 'segment', 'band', 'audiopath', 'dls'][i], ...ref(p)}));
}
const results = [], positive = (name, fn) => { fn(); results.push({name, expected: 'accept', passed: true}); };
const negative = (name, fn, pattern) => { let failure; try { fn(); } catch (e) { failure = e; } assert(failure, 'Must reject ' + name); assert.match(failure.message, pattern); results.push({name, expected: 'reject', passed: true, reason: failure.message}); };
let error = null;
try {
  for (const name of protocol.cases) {
    const files = fixture('positive-' + name, name);
    positive('Allowed native delta: ' + name, () => inspectSendCase(files, name, authorRoot));
  }
  const nativeNegative = (label, mutation, pattern) => negative(label, () => inspectSendCase(fixture(label, 'dry-minus600', mutation), 'dry-minus600', authorRoot), pattern);
  nativeNegative('wrong-attenuation', ({send}) => one(send.children, 'data').data.writeInt32LE(-601, 0), /Case attenuation/);
  nativeNegative('wrong-send-class', ({send}) => one(send.children, 'fxhr').data[4] ^= 1, /Literal Send\/Waves effect order/);
  nativeNegative('wrong-destination-guid', ({send}) => one(send.children, 'fxhr').data[36] ^= 1, /Send local mix-in destination/);
  nativeNegative('changed-existing-source-metadata', ({groups}) => one(one(groups[1].children, 'RIFF', 'DSBC').children, 'dsbd').data[8] ^= 1, /Only allowed case gain\/order\/default-Waves delta/);
  nativeNegative('changed-untouched-control-buffer', ({groups}) => one(groups[0].children, 'ddah').data[16] ^= 1, /Only allowed case gain\/order\/default-Waves delta/);
  const badOrder = fixture('wrong-order', 'send-before-waves-minus600');
  negative('Waves before Send under after-Send case identity', () => inspectSendCase(badOrder, 'send-after-waves-minus600', authorRoot), /Literal Send\/Waves effect order/);
  const badEmbedded = fixture('wrong-embedded', 'dry-minus600');
  let bytes = read(badEmbedded[1].path), song = one(chunks(bytes), 'RIFF', 'DMSG'), embedded = one(song.children, 'RIFF', 'DMAP'); bytes[embedded.offset + 20] ^= 1;
  fs.writeFileSync(badEmbedded[1].path, bytes); badEmbedded[1].sha256 = hash(badEmbedded[1].path);
  negative('Segment embedded AP differs from standalone input', () => inspectSendCase(badEmbedded, 'dry-minus600', authorRoot), /Case embedded AudioPath exact/);
  const shortNote = fixture('wrong-note-duration', 'dry-minus600'); bytes = read(shortNote[1].path); song = one(chunks(bytes), 'RIFF', 'DMSG');
  const all = xs => xs.flatMap(c => [c, ...all(c.children)]), seqt = one(all(song.children), 'seqt'), events = one(chunks(seqt.data), 'evtl');
  seqt.data.writeInt32LE(3072, events.offset + 16); fs.writeFileSync(shortNote[1].path, bytes); shortNote[1].sha256 = hash(shortNote[1].path);
  negative('Short diagnostic notes substituted for long main notes', () => inspectSendCase(shortNote, 'dry-minus600', authorRoot), /Long notes, tempo and all non-AP bytes retained/);
  const wrongDls = fixture('wrong-dls', 'dry-minus600'); bytes = read(wrongDls[4].path); bytes[bytes.length - 2] ^= 1; fs.writeFileSync(wrongDls[4].path, bytes); wrongDls[4].sha256 = hash(wrongDls[4].path);
  negative('Owned DLS changed', () => inspectSendCase(wrongDls, 'dry-minus600', authorRoot), /Owned DLS unchanged/);
  for (const [label, offset, pattern] of [['wrong-catalog-guid', 28, /Catalog native GUID/], ['wrong-catalog-size', 24, /Catalog native size/]]) {
    const files = fixture(label, 'dry-minus600'); bytes = read(files[0].path);
    const project = one(chunks(bytes), 'RIFF', 'JAZP'), entry = project.children.find(c => c.id === 'LIST' && c.type === 'file');
    one(entry.children, 'filh').data[offset] ^= 1; fs.writeFileSync(files[0].path, bytes); files[0].sha256 = hash(files[0].path);
    negative(label, () => inspectSendCase(files, 'dry-minus600', authorRoot), pattern);
  }
  const launch = json(path.join(authorRoot, 'author-launch.json'));
  // Fabricated lifecycle records exercise identity/exit checks without querying,
  // starting or terminating any app. They are never supplied to the full auditor.
  const syntheticLaunch = {...launch, processId: 70001}, identity = {pid: 70001, path: launch.executable, sha256: launch.exeSha256,
    startUtc: '2026-10-08T02:00:00Z', observedUtc: '2026-10-08T02:00:01Z', responding: true};
  const exit = {processId: 70001, executable: launch.executable, exeSha256: launch.exeSha256, startUtc: identity.startUtc,
    state: 'exited', exitCode: 0, forcedTermination: false, exitUtc: '2026-10-08T02:02:00Z'};
  const close = {beforeUtc: '2026-10-08T02:01:59Z', afterUtc: '2026-10-08T02:01:59.1Z', window: {app: 'process:' + launch.executable, id: 70002}};
  const expected = {exeSha256: launch.exeSha256, buildSummarySha256: launch.buildSummarySha256, windowId: 70002};
  const life = (i = identity, e = exit, a = close) => verifySendLifecycle(syntheticLaunch, i, e, a, expected);
  positive('Matching fabricated PID/start/image/hash and normal Close/exit0', () => life());
  negative('Exit from another PID', () => life(identity, {...exit, processId: 70003}), /Exit belongs to selected PID/);
  negative('PID reused with a different start instance', () => life({...identity, startUtc: '2026-10-08T01:59:00Z'}), /Same process start instance/);
  negative('Wrong live executable hash', () => life({...identity, sha256: '0'.repeat(64)}), /Live process executable hash/);
  negative('Wrong live executable path', () => life({...identity, path: path.join(output, 'Producer.exe')}), /Live process image path/);
  negative('Watcher still running is not normal exit', () => life(identity, {...exit, state: 'watching'}), /Normal-exit observation completed/);
  negative('Actual exit code nonzero', () => life(identity, {...exit, exitCode: 1}), /Actual normal exit0/);
  negative('Forced close flag', () => life(identity, {...exit, forcedTermination: true}), /No forced termination/);
  negative('Recorded exit predates Close action', () => life(identity, {...exit, exitUtc: '2026-10-08T02:01:58Z'}), /Actual normal GUI Close before exit/);
  negative('Close another window', () => life(identity, exit, {...close, window: {...close.window, id: 70004}}), /Close selected window/);
  const [observationPath, pngPath, actionPath] = observationFiles;
  const observation = json(observationPath), action = json(actionPath), png = read(pngPath), window = observation.window;
  positive('Existing real saved-Segment observation/PNG/action shape', () => verifySendObservation(observation, png, action, window, 'Value: SendMinus600.sgp ID: 207'));
  negative('Stale observation before input', () => verifySendObservation({...observation, observedUtc: '2026-10-08T00:00:00Z'}, png, action, window, 'Value: SendMinus600.sgp ID: 207'), /Fresh observation after input/);
  negative('Observed another window', () => verifySendObservation({...observation, window: {...window, id: 70004}}, png, action, window, 'Value: SendMinus600.sgp ID: 207'), /Observed selected window/);
  negative('Missing required Playing observation', () => verifySendObservation(observation, png, action, window, 'Playing musical input'), /Actual GUI state/);
  const wrongDimensions = structuredClone(observation); wrongDimensions.screenshots[0].width += 1;
  negative('Native image dimensions contradict snapshot', () => verifySendObservation(wrongDimensions, png, action, window, 'Value: SendMinus600.sgp ID: 207'), /Screenshot width/);
  positive('Exact saved file reference', () => checkedReference(ref(path.join(author, 'SendMinus600.aup'))));
  negative('Modified file reference hash', () => checkedReference({path: path.join(author, 'SendMinus600.aup'), sha256: '0'.repeat(64)}), /Evidence reference hash/);
  // A real live watcher must not become a successful author closure simply
  // because numerical controls or three saved documents already passed.
  if (json(path.join(authorRoot, 'author-exit.json')).state === 'watching')
    negative('Actual incomplete author lifecycle cannot satisfy native full mode', () => inspectNative(authorRoot), /watching|exited/);
} catch (e) { error = {name: e.name, message: e.message, stack: e.stack}; }
const proof = {schema: 1, createdUtc: new Date().toISOString(), passed: !error, kind: 'native-and-lifecycle-validator-controls',
  positiveCount: results.filter(r => r.expected === 'accept').length, negativeCount: results.filter(r => r.expected === 'reject').length,
  results, error, scope: ref(scopePath), protocol: scope.protocol, authorRoot,
  sourceDocuments: ['SendZero.aup', 'SendMinus600.aup', 'SendMinus600.sgp', 'RouteBand.bnp', 'RouteSource.dls'].map(n => ref(path.join(author, n))),
  realObservationShapeOnly: observationFiles.map(ref), observationIndex: observationIndexPath ? ref(observationIndexPath) : null,
  auditor: ref('scripts/Inspect-AudioPathSendMainBindings.mjs'), controlDriver: ref(fileURLToPath(import.meta.url)),
  dependencies: ['scripts/Inspect-AudioPathSendNative.mjs', 'scripts/Inspect-AudioPathSendGuiAudio.mjs', 'scripts/AudioCaptureClock.mjs'].map(ref),
  productSourceChanged: false, productCaptureExecuted: false, syntheticNormalExitCount: 0,
  limitations: ['Derived native fixtures and fabricated lifecycle records validate only checker logic; actual author save/exit, distinct restore and four main captures remain pending'], fullAcceptance: false};
write(path.join(output, 'controls.json'), proof); console.log(JSON.stringify({passed: proof.passed, positiveCount: proof.positiveCount, negativeCount: proof.negativeCount, error: error?.message}));
if (error) process.exitCode = 1;
