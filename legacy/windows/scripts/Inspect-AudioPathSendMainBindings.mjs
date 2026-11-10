import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {chunks, inspectNative} from './Inspect-AudioPathSendNative.mjs';
import {readSendPcm, verifySendPackets, analyzeSendPcm, compareSendCases} from './Inspect-AudioPathSendGuiAudio.mjs';

const read = p => fs.readFileSync(p), json = p => JSON.parse(read(p).toString().replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(read(p)).digest('hex');
const ref = p => ({path: path.resolve(p), sha256: hash(p)});
const samePath = (a, b) => path.resolve(a).toLowerCase() === path.resolve(b).toLowerCase();
export function checkedReference(r) { assert(r && typeof r.path === 'string'); assert.match(r.sha256, /^[a-f0-9]{64}$/); assert.equal(hash(r.path), r.sha256, 'Evidence reference hash'); return path.resolve(r.path); }
const one = (xs, id, type = '') => { const f = xs.filter(c => c.id === id && c.type === type); assert.equal(f.length, 1, 'Unique native ' + id + '/' + type); return f[0]; };
const flat = xs => xs.flatMap(c => [c, ...flat(c.children)]);
const native = (p, type) => one(chunks(read(p)), 'RIFF', type);
export function serializeChunk(c) {
  if (!['RIFF', 'LIST'].includes(c.id)) return c.raw;
  const data = Buffer.concat([Buffer.from(c.type, 'ascii'), ...c.children.map(serializeChunk)]), h = Buffer.alloc(8);
  h.write(c.id); h.writeUInt32LE(data.length, 4);
  return Buffer.concat([h, data, ...(data.length & 1 ? [c.raw.subarray(c.raw.length - 1)] : [])]);
}

// Literal class/order/4B LONG checks are independent of the product parser.
// Removing just the added Waves and neutralizing the gain must recover the
// GUI-saved author AudioPath byte for byte, including routes and unknown data.
export function inspectSendCase(files, name, authorRoot) {
  assert.equal(files.length, 5, 'Exactly five native case files');
  assert.deepEqual(files.map(f => f.role), ['project', 'segment', 'band', 'audiopath', 'dls'], 'Native role order');
  assert.equal(new Set(files.map(f => path.resolve(checkedReference(f)))).size, 5);
  const [projectFile, segmentFile, bandFile, audioFile, dlsFile] = files.map(f => path.resolve(f.path));
  assert(files.every(f => samePath(path.dirname(f.path), path.dirname(projectFile))), 'Single owned native Project directory');
  assert.equal(path.basename(projectFile, '.pro'), path.basename(path.dirname(projectFile)), 'Native Project name/folder');
  const author = path.join(authorRoot, 'author', 'MultiCapture'), ap = native(audioFile, 'DMAP');
  const Send = '762160efbbbce0498ccae09a5a152b33', FileOutput = '11146d2dd7dce745addeacac85a2425d', Waves = '6802fc87559a604395aa004a1d9de26c';
  const expected = ({'dry-zero': [FileOutput, Send], 'dry-minus600': [FileOutput, Send],
    'send-before-waves-minus600': [FileOutput, Send, Waves], 'send-after-waves-minus600': [FileOutput, Waves, Send]})[name];
  assert(expected, 'Known frozen case');
  const buffers = ap.children.filter(c => c.id === 'LIST' && c.type === 'dbfl'); assert.equal(buffers.length, 3);
  const fxs = one(one(buffers[1].children, 'RIFF', 'DSBC').children, 'LIST', 'fxls');
  const classes = fxs.children.map(f => one(f.children, 'fxhr').data.subarray(4, 20).toString('hex'));
  assert.deepEqual(classes, expected, 'Literal Send/Waves effect order');
  const send = fxs.children[classes.indexOf(Send)], parameter = one(send.children, 'data').data;
  assert.equal(parameter.length, 4, 'Send signed LONG width');
  assert.equal(parameter.readInt32LE(0), name === 'dry-zero' ? 0 : -600, 'Case attenuation');
  const header = one(send.children, 'fxhr').data; assert.equal(header.length, 56);
  assert.deepEqual(header.subarray(36, 52), one(buffers[2].children, 'ddah').data.subarray(0, 16), 'Send local mix-in destination');
  const wave = fxs.children.find((_, i) => classes[i] === Waves);
  if (wave) {
    assert.equal(wave.id, 'RIFF'); assert.equal(wave.type, 'DSFX'); assert.equal(wave.children.length, 1, 'Waves factory default has no custom parameter data');
    const h = Buffer.alloc(56); Buffer.from(Waves, 'hex').copy(h, 4);
    assert.deepEqual(one(wave.children, 'fxhr').data, h, 'Public Waves class and factory defaults');
  }
  const segment = native(segmentFile, 'DMSG');
  assert.deepEqual(one(segment.children, 'RIFF', 'DMAP').raw, read(audioFile), 'Case embedded AudioPath exact');
  const nonAp = xs => xs.filter(c => !(c.id === 'RIFF' && c.type === 'DMAP')).map(c => c.raw);
  assert.deepEqual(nonAp(segment.children), nonAp(native(path.join(author, 'SendMinus600.sgp'), 'DMSG').children), 'Long notes, tempo and all non-AP bytes retained');
  const notes = one(chunks(one(flat(segment.children), 'seqt').data), 'evtl').data;
  assert.equal(notes.length, 44); assert.equal(notes.readUInt32LE(0), 20);
  assert.deepEqual([4, 24].map(p => [notes.readInt32LE(p), notes.readInt32LE(p + 4), notes.readUInt32LE(p + 8), notes[p + 15], notes[p + 16]]), [[0, 122880, 0, 69, 96], [0, 122880, 8, 60, 96]]);
  assert.deepEqual(read(bandFile), read(path.join(author, 'RouteBand.bnp')), 'Owned Band unchanged');
  assert.deepEqual(read(dlsFile), read(path.join(author, 'RouteSource.dls')), 'Owned DLS unchanged');
  // Serialize after editing only our private parse tree/data copy.
  if (wave) fxs.children.splice(fxs.children.indexOf(wave), 1);
  parameter.writeInt32LE(-600, 0);
  assert.deepEqual(serializeChunk(ap), read(path.join(author, 'SendMinus600.aup')), 'Only allowed case gain/order/default-Waves delta');
  const project = native(projectFile, 'JAZP'), entries = project.children.filter(c => c.id === 'LIST' && c.type === 'file');
  assert.equal(entries.length, 4, 'Native Project four entries');
  const names = entries.map(e => one(e.children, 'name').data.toString('utf16le').replace(/\0+$/, ''));
  assert.deepEqual(names, [segmentFile, bandFile, audioFile, dlsFile].map(p => path.basename(p)), 'Native catalog names/order');
  for (let i = 0; i < entries.length; i++) {
    assert.equal(path.basename(names[i]), names[i], 'Contained native catalog name');
    const p = [segmentFile, bandFile, audioFile, dlsFile][i], document = native(p, ['DMSG', 'DMBD', 'DMAP', 'DLS '][i]);
    const guid = one(document.children, i === 3 ? 'dlid' : 'guid').data, h = one(entries[i].children, 'filh').data;
    assert.equal(h.length, 44); assert.equal(h.readUInt32LE(24), read(p).length, 'Catalog native size');
    assert.deepEqual(h.subarray(28, 44), guid, 'Catalog native GUID');
  }
  const old = native(path.join(authorRoot, 'before', 'MultiCapture', 'MultiCapture.pro'), 'JAZP');
  const rootOnly = xs => xs.filter(c => !(c.id === 'LIST' && c.type === 'file')).map(c => c.raw);
  assert.deepEqual(rootOnly(project.children), rootOnly(old.children), 'Project root/runtime/unknown metadata retained');
  return {passed: true, name, classes, attenuation: name === 'dry-zero' ? 0 : -600, catalog: names, files, longNoteSeconds: 160, runtimeTempoMeasured: false};
}

export function verifySendLifecycle(launch, identity, exit, close, expected) {
  assert.equal(launch.processId, identity.pid, 'Actual selected PID');
  assert.equal(launch.processId, exit.processId, 'Exit belongs to selected PID');
  assert(Number.isSafeInteger(launch.processId) && launch.processId > 0);
  assert(samePath(launch.executable, identity.executable ?? identity.path), 'Live process image path');
  assert(samePath(launch.executable, exit.executable), 'Exit process image path');
  assert.equal(launch.exeSha256, identity.sha256 ?? identity.exeSha256, 'Live process executable hash');
  assert.equal(launch.exeSha256, exit.exeSha256, 'Exit process executable hash');
  assert.equal(launch.exeSha256, expected.exeSha256, 'Adopted executable hash');
  assert.equal(launch.buildSummarySha256, expected.buildSummarySha256, 'Adopted build summary hash');
  assert.equal(identity.startUtc, exit.startUtc, 'Same process start instance');
  assert.equal(identity.responding, true, 'Observed live selected process');
  assert.equal(exit.state, 'exited', 'Normal-exit observation completed');
  assert.equal(exit.exitCode, 0, 'Actual normal exit0'); assert.equal(exit.forcedTermination, false, 'No forced termination');
  const start = Date.parse(identity.startUtc), observed = Date.parse(identity.observedUtc), ended = Date.parse(exit.exitUtc);
  assert(Number.isFinite(start) && Number.isFinite(observed) && Number.isFinite(ended));
  assert(start <= observed && observed <= ended, 'Live identity before actual exit');
  const before = Date.parse(close.beforeUtc), after = Date.parse(close.afterUtc);
  assert(before >= observed && after >= before && after - before <= 2000 && ended >= before, 'Actual normal GUI Close before exit');
  assert(samePath(close.window.app.replace(/^process:/, ''), launch.executable), 'Close selected executable window');
  assert.equal(close.window.id, expected.windowId, 'Close selected window');
  return {passed: true, processId: launch.processId, startUtc: identity.startUtc, exitUtc: exit.exitUtc, exitCode: 0};
}

export function screenshotDimensions(b) {
  if (b.subarray(0, 8).toString('hex') === '89504e470d0a1a0a') {
    assert(b.length >= 33 && b.toString('ascii', 12, 16) === 'IHDR', 'PNG screenshot header');
    return {format: 'PNG', width: b.readUInt32BE(16), height: b.readUInt32BE(20)};
  }
  assert(b.length >= 4 && b.readUInt16BE(0) === 0xffd8, 'JPEG or PNG native screenshot');
  // Read only JPEG headers, not pixels. Preserve the recorder's original bytes.
  const sof = new Set([0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf]);
  for (let p = 2; p < b.length;) {
    assert.equal(b[p++], 0xff, 'JPEG marker'); while (p < b.length && b[p] === 0xff) p++;
    assert(p < b.length); const marker = b[p++]; assert(marker !== 0xda && marker !== 0xd9, 'JPEG dimensions before scan/end');
    assert(p + 2 <= b.length); const n = b.readUInt16BE(p); assert(n >= 2 && p + n <= b.length, 'JPEG segment bounds');
    if (sof.has(marker)) { assert(n >= 8); return {format: 'JPEG', width: b.readUInt16BE(p + 5), height: b.readUInt16BE(p + 3)}; }
    p += n;
  }
  assert.fail('JPEG screenshot dimensions missing');
}

export function verifySendObservation(observation, png, action, window, requiredText) {
  assert.equal(observation.window.id, window.id, 'Observed selected window');
  assert.equal(observation.window.app.toLowerCase(), window.app.toLowerCase(), 'Observed selected app');
  assert.equal(action.window.id, window.id, 'Input selected window');
  assert.equal(action.window.app.toLowerCase(), window.app.toLowerCase(), 'Input selected app');
  assert(Date.parse(observation.observedUtc) >= Date.parse(action.afterUtc), 'Fresh observation after input');
  assert(observation.accessibility && typeof observation.accessibility.tree === 'string');
  assert(observation.accessibility.tree.includes(requiredText), 'Actual GUI state: ' + requiredText);
  const dimensions = screenshotDimensions(png);
  assert.equal(dimensions.width, observation.screenshots[0].width, 'Screenshot width');
  assert.equal(dimensions.height, observation.screenshots[0].height, 'Screenshot height');
  return true;
}

export function verifySendProjectOpen(launch, files, window) {
  if (launch.launchMethod !== 'sky.launch_app+FileOpen') {
    assert.equal(launch.arguments[0], '--open-project');
    assert(samePath(launch.arguments[1].replace(/^"|"$/g, ''), files[0].path), 'GUI opened exact case Project');
    return {passed: true, method: '--open-project'};
  }
  assert.deepEqual(launch.arguments, [], 'Sky launch has no invented CLI arguments');
  assert.equal(launch.selectedWindow.id, window.id, 'Launch selected window');
  assert.equal(launch.selectedWindow.app.toLowerCase(), window.app.toLowerCase(), 'Launch selected app');
  const e = launch.projectOpen;
  const typed = json(checkedReference(e.typedAction)), filename = json(checkedReference(e.filenameObservation));
  const open = json(checkedReference(e.openAction)), restored = json(checkedReference(e.restored));
  assert.equal(typed.kind, 'type_text', 'Actual native Project filename input');
  assert.equal(typed.payload.text, files[0].path, 'Exact native Project path typed');
  verifySendObservation(filename, read(checkedReference(e.filenamePng)), typed, window, files[0].path);
  assert(/編集.*ファイル名.*Value:/.test(filename.accessibility.tree), 'Native filename edit observed');
  assert.equal(open.kind, 'click', 'Actual native Open submit');
  assert(Date.parse(open.beforeUtc) >= Date.parse(filename.observedUtc), 'Open follows filename observation');
  assert(Date.parse(open.afterUtc) >= Date.parse(open.beforeUtc) && Date.parse(open.afterUtc) - Date.parse(open.beforeUtc) <= 2000, 'Actual bounded Open input');
  verifySendObservation(restored, read(checkedReference(e.restoredPng)), open, window, 'Value: ' + path.basename(files[1].path) + ' ID: 207');
  assert(restored.accessibility.tree.includes('Notes: 2.'), 'Native Project restored two notes');
  return {passed: true, method: launch.launchMethod, openUtc: open.afterUtc, restoredUtc: restored.observedUtc};
}

export function inspectSendBindings(indexPath) {
  const index = json(indexPath), protocolPath = checkedReference(index.protocol), protocol = json(protocolPath);
  const scopePath = checkedReference(index.bindingScope), scope = json(scopePath), numericScopePath = checkedReference(index.numericScope), numericScope = json(numericScopePath);
  assert.equal(scope.protocol.sha256, index.protocol.sha256); assert.equal(numericScope.protocol.sha256, index.protocol.sha256);
  assert.equal(index.candidate, protocol.candidate); assert.equal(index.kind, 'actual-main-audio-case-evidence');
  assert(Date.parse(scope.createdUtc) < Date.parse(index.createdUtc), 'Binding audit scope fixed before actual capture evidence');
  const authorRoot = path.resolve(index.authorRoot), authorNative = inspectNative(authorRoot);
  const authorLaunch = json(path.join(authorRoot, 'author-launch.json')), authorExit = json(path.join(authorRoot, 'author-exit.json'));
  const authorIdentity = json(checkedReference(index.authorIdentity)), authorClose = json(checkedReference(index.authorClose));
  const expected = {exeSha256: authorLaunch.exeSha256, buildSummarySha256: protocol.build.sha256};
  assert.equal(hash(authorLaunch.buildSummary), protocol.build.sha256);
  const authorLife = verifySendLifecycle(authorLaunch, authorIdentity, authorExit, authorClose, {...expected, windowId: authorClose.window.id});
  const build = json(authorLaunch.buildSummary); assert(build.passed && build.sourceSnapshotUnchanged); assert.equal(build.sources.length, 218);
  for (const s of build.sources) { assert.equal(hash(path.join(build.sourceRoot, s.path)), s.sha256, 'Saved product source'); assert.equal(hash(path.resolve(s.path)), s.sha256, 'Current product source'); }
  for (const o of build.outputs) assert.equal(hash(path.join(path.dirname(authorLaunch.buildSummary), o.path)), o.sha256, 'Saved product output');
  assert.deepEqual(index.cases.map(c => c.name), protocol.cases, 'Four actual cases in frozen order');
  const cases = index.cases.map(c => {
    const inputPath = checkedReference(c.inputs), input = json(inputPath);
    assert.equal(input.name, c.name); assert.equal(input.candidate, index.candidate); assert.equal(input.protocolSha256, index.protocol.sha256);
    const fixture = inspectSendCase(input.files, c.name, authorRoot);
    const runPath = checkedReference(c.captureRun), run = json(runPath), dir = path.dirname(runPath);
    const launchPath = checkedReference(c.launch), launch = json(launchPath), launchAtCapture = json(path.join(dir, 'gui-launch-at-capture.json'));
    const identityPath = checkedReference(c.identity), identity = json(identityPath), exitPath = checkedReference(c.exit), exit = json(exitPath);
    assert.equal(run.state, 'exited'); assert.equal(run.captureExitCode, 0); assert.equal(run.processId, launch.processId);
    assert.equal(launchAtCapture.processId, launch.processId); assert.equal(launchAtCapture.exeSha256, expected.exeSha256);
    assert.equal(run.exeSha256, expected.exeSha256); assert.equal(run.buildSummarySha256, expected.buildSummarySha256);
    assert.equal(run.durationSeconds, protocol.recorder.durationSeconds); assert.equal(run.silentKeepAlive, protocol.recorder.silentKeepAlive);
    assert.equal(run.timingBasis, 'recorder-start-utc-qpc');
    assert(Date.parse(input.createdUtc) < Date.parse(run.captureStartUtc), 'Exact native input record before capture');
    assert(Date.parse(scope.createdUtc) < Date.parse(run.captureStartUtc)); assert(Date.parse(protocol.createdUtc) < Date.parse(run.captureStartUtc));
    const projectOpen = verifySendProjectOpen(launch, input.files, {app: 'process:' + launch.executable, id: c.windowId});
    if (launch.launchMethod === 'sky.launch_app+FileOpen') {
      assert.deepEqual(launchAtCapture.projectOpen, launch.projectOpen, 'Capture binds actual GUI Project open');
      assert.equal(c.restored.sha256, launch.projectOpen.restored.sha256, 'Same actual restored observation');
      assert.equal(c.restoredPng.sha256, launch.projectOpen.restoredPng.sha256, 'Same actual restored screenshot');
    }
    assert.equal(launchAtCapture.inputs.length, 5); assert.equal(launch.inputs.length, 5);
    for (const f of input.files) {
      for (const xs of [launch.inputs, launchAtCapture.inputs, run.inputs]) assert(xs.some(x => samePath(x.path, f.path) && x.sha256 === f.sha256), 'Same five actual GUI/capture case inputs');
    }
    for (const f of [index.protocol, index.bindingScope, index.numericScope, c.inputs]) assert(run.inputs.some(x => samePath(x.path, f.path) && x.sha256 === f.sha256), 'Capture binds frozen protocol/scope/input record');
    for (const x of run.inputs) checkedReference(x);
    for (const [p, h] of [[run.executable, run.exeSha256], [run.buildSummary, run.buildSummarySha256], [run.recorder, run.recorderSha256],
      [run.recorderBuildSummary, run.recorderBuildSummarySha256], [path.join(dir, 'driver.ps1'), run.driverSha256], [path.join(dir, 'ready.json'), run.captureReadySha256]]) assert.equal(hash(p), h, 'Actual capture dependency');
    assert.equal(run.driverSha256, protocol.recorder.script.sha256); assert.equal(run.recorderBuildSummarySha256, protocol.recorder.build.sha256);
    const recorder = json(run.recorderBuildSummary); assert(recorder.passed); assert.equal(recorder.sha256, run.recorderSha256);
    for (const s of recorder.sources) assert.equal(hash(path.join(path.dirname(run.recorderBuildSummary), 'sources', s.path)), s.sha256, 'Saved recorder source');
    const actions = json(checkedReference(c.actions)); assert.deepEqual(actions.map(a => a.name), scope.operationSchema.names, 'Actual recording/transport/Close actions');
    const window = {app: 'process:' + launch.executable, id: c.windowId}; assert(Number.isSafeInteger(window.id) && window.id > 0);
    for (let i = 0; i < actions.length - 1; i++) {
      const a = actions[i], observation = json(checkedReference(a.observation)), png = read(checkedReference(a.png));
      verifySendObservation(observation, png, a, window, scope.operationSchema.requiredObservationText[i]);
    }
    const restored = json(checkedReference(c.restored)), restoredPng = read(checkedReference(c.restoredPng));
    const selection = 'Value: ' + path.basename(input.files[1].path) + ' ID: 207';
    verifySendObservation(restored, restoredPng, {window, afterUtc: identity.observedUtc}, window, selection);
    assert(restored.accessibility.tree.includes('Notes: 2.'), 'Actual main restored two-note Segment');
    assert(Date.parse(restored.observedUtc) < Date.parse(actions[0].beforeUtc), 'Restored document observation before recording');
    const life = verifySendLifecycle(launch, identity, exit, actions.at(-1), {...expected, windowId: c.windowId});
    assert.notEqual(life.processId, authorLife.processId, 'Distinct native restore process');
    const inventory = json(checkedReference(c.preRecordingInventory)); assert.equal(inventory.files.length, 3);
    assert(Date.parse(inventory.observedUtc) < Date.parse(actions[0].beforeUtc), 'Absent outputs observed before Start Recording');
    assert.equal(c.taps.length, 3); assert.deepEqual(c.taps.map(t => path.basename(t.path)), Object.keys(protocol.outputRoles), 'Actual FileOutput roles/order');
    for (const f of c.taps) { checkedReference(f); const before = inventory.files.find(x => samePath(x.path, f.path)); assert(before && before.exists === false, 'Fresh FileOutput target'); }
    const outputDirectory = path.dirname(c.taps[0].path); assert(c.taps.every(f => samePath(path.dirname(f.path), outputDirectory)));
    assert.deepEqual(fs.readdirSync(outputDirectory).filter(n => /^Record\d*\.wav$/i.test(n)).sort(), Object.keys(protocol.outputRoles).sort(), 'Exactly three finalized tap WAV files');
    const endpoint = readSendPcm(path.join(dir, 'output.wav')), taps = c.taps.map(f => readSendPcm(f.path));
    const ready = json(path.join(dir, 'ready.json')), capture = json(path.join(dir, 'capture.json')); assert(capture.passed);
    assert.equal(capture.seconds, protocol.recorder.durationSeconds);
    const packets = read(path.join(dir, 'packets.csv')).toString().trim().split(/\r?\n/).slice(1).map(l => l.split(',').map(Number));
    const verified = verifySendPackets(ready, capture, packets, endpoint), clock = verified.clock;
    assert(Math.abs(clock.utcToSeconds(run.captureStartUtc)) <= .001, 'Actual recorder UTC start');
    const readyPollSeconds = clock.utcToSeconds(run.readyUtc); assert(readyPollSeconds >= 0 && readyPollSeconds < 1, 'Ready poll after calibrated start');
    const keys = ['startRecording', 'first', 'stopFirst', 'replay', 'stopReplay', 'stopRecording', 'close'];
    const times = Object.fromEntries(actions.map((a, i) => [keys[i], {before: clock.utcToSeconds(a.beforeUtc), after: clock.utcToSeconds(a.afterUtc)}]));
    const audio = analyzeSendPcm({endpoint, taps}, times, protocol, numericScope);
    return {name: c.name, passed: true, fixture, life, audio, times, packets: verified.evidence, readyPollSeconds,
      bindings: {inputs: c.inputs, run: c.captureRun, launch: c.launch, identity: c.identity, exit: c.exit, actions: c.actions,
        endpoint: ref(path.join(dir, 'endpoint.txt')), pcm: ref(path.join(dir, 'output.wav')), packets: ref(path.join(dir, 'packets.csv')),
        ready: ref(path.join(dir, 'ready.json')), capture: ref(path.join(dir, 'capture.json')), taps: c.taps, restored: c.restored, restoredPng: c.restoredPng}};
  });
  assert.equal(new Set(cases.map(c => c.life.processId)).size, 4, 'Four independently launched case processes');
  assert(cases.every(c => c.bindings.endpoint.sha256 === cases[0].bindings.endpoint.sha256), 'Same selected render endpoint');
  return {schema: 1, createdUtc: new Date().toISOString(), passed: true, kind: 'actual-adopted-main-send-bounded', candidate: index.candidate,
    authorNative, authorLife, cases, comparison: compareSendCases(cases, protocol, numericScope), index: ref(indexPath),
    protocol: index.protocol, bindingScope: index.bindingScope, numericScope: index.numericScope, auditor: ref(fileURLToPath(import.meta.url)),
    dependencies: ['./Inspect-AudioPathSendNative.mjs', './Inspect-AudioPathSendGuiAudio.mjs', './AudioCaptureClock.mjs'].map(p => ref(fileURLToPath(new URL(p, import.meta.url)))),
    normalExits: 5, limitations: ['Bounded local Send/default Waves only; external destination lifetime/original comparison/all40/all8 remain',
      'Digital mixed endpoint; physical speaker and sustained-note runtime tempo not measured'], fullAcceptance: false};
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [index, output] = process.argv.slice(2); assert(output, 'Usage: actual-evidence-index fresh-proof-output'); assert(!fs.existsSync(output));
  const proof = inspectSendBindings(index); fs.writeFileSync(output, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
  console.log(JSON.stringify({passed: true, kind: proof.kind, candidate: proof.candidate, cases: proof.cases.length, normalExits: proof.normalExits}));
}
