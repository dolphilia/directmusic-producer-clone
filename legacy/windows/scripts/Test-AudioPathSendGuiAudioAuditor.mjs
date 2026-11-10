import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {analyzeSendPcm, compareSendCases, readSendPcm, verifySendPackets} from './Inspect-AudioPathSendGuiAudio.mjs';

const [protocolPath, scopePath, output] = process.argv.slice(2);
assert(output, 'Usage: protocol scope fresh-output-directory'); assert(!fs.existsSync(output), 'Preserve prior controls');
fs.mkdirSync(output);
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const ref = p => ({path: path.resolve(p), sha256: hash(p)});
const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const write = (p, o) => fs.writeFileSync(p, JSON.stringify(o, null, 2) + '\n', {flag: 'wx'});
const protocol = json(protocolPath), scope = json(scopePath);
assert.equal(scope.protocol.sha256, hash(protocolPath));
const rate = 8000, epoch = Date.parse('2026-10-08T02:00:00Z');
const times = {startRecording: {before: 10, after: 10.1}, first: {before: 14, after: 14.1},
  stopFirst: {before: 22, after: 22.1}, replay: {before: 28, after: 28.1},
  stopReplay: {before: 36, after: 36.1}, stopRecording: {before: 40, after: 40.1}, close: {before: 44, after: 44.1}};
const first = 14.2, replay = 28.2, fileOrigin = 10.15, fileSeconds = 29.9;
function signal(role, gain, opt = {}) {
  const origin = role === 'endpoint' ? 0 : fileOrigin, seconds = role === 'endpoint' ? 300 : fileSeconds;
  const frames = Math.round(seconds * rate), mono = new Float64Array(frames), energy = new Float64Array(frames);
  let peak = 0;
  for (let i = 0; i < frames; i++) {
    const t = i / rate + origin, delay = role === 'destination' ? (opt.destinationDelay ?? 0) : 0;
    let x = 0;
    const starts = [first + delay, replay + delay + (role === 'destination' ? (opt.replayDelay ?? 0) : 0)];
    for (let n = 0; n < 2; n++) {
      const end = (n ? 36.05 : 22.05) - (opt.earlyEnd ? 2 : 0);
      if (t >= starts[n] && t < end && !(opt.missingReplay && n === 1)) {
        const source = .03 * Math.sin(2 * Math.PI * (opt.wrongPitch ? 440 * 2 ** (1 / 12) : 440) * (t - starts[n]));
        const control = .025 * (opt.controlScale ?? 1) * Math.sin(2 * Math.PI * 261.6255653005986 * (t - starts[n]));
        if (role === 'endpoint') x += source + control + .03 * gain * Math.sin(2 * Math.PI * 440 * (t - starts[n]));
        if (role === 'source') x += source + (opt.crossLeak ? control : 0);
        if (role === 'control') x += control;
        if (role === 'destination') x += .03 * gain * Math.sin(2 * Math.PI * 440 * (t - starts[n]));
      }
    }
    if (opt.afterStop && t >= 37 && t < 39) x += .01 * Math.sin(2 * Math.PI * 440 * t);
    if (opt.extraStart && t >= 24 && t < 25) x += .01 * Math.sin(2 * Math.PI * 440 * t);
    if (opt.clipping && t >= 20 && t < 21) x += 1.1;
    mono[i] = x; energy[i] = x * x; peak = Math.max(peak, Math.abs(x));
  }
  return {rate, channels: 2, bits: 32, tag: 3, seconds, frames, mono, energy, peak};
}
const recordings = (gain, opt = {}) => ({endpoint: signal('endpoint', gain, opt),
  taps: ['source', 'control', 'destination'].map(role => signal(role, gain, opt))});
const analyze = (gain, opt, actionTimes = times) => analyzeSendPcm(recordings(gain, opt), actionTimes, protocol, scope);
function writeWave(p, w, tag = 3, bits = 32) {
  const align = w.channels * bits / 8, b = Buffer.alloc(44 + w.frames * align);
  b.write('RIFF', 0); b.writeUInt32LE(b.length - 8, 4); b.write('WAVEfmt ', 8); b.writeUInt32LE(16, 16);
  b.writeUInt16LE(tag, 20); b.writeUInt16LE(w.channels, 22); b.writeUInt32LE(w.rate, 24);
  b.writeUInt32LE(w.rate * align, 28); b.writeUInt16LE(align, 32); b.writeUInt16LE(bits, 34);
  b.write('data', 36); b.writeUInt32LE(b.length - 44, 40);
  for (let i = 0; i < w.frames; i++) for (let c = 0; c < w.channels; c++) {
    if (tag === 3) b.writeFloatLE(w.mono[i], 44 + i * align + c * 4);
    else b.writeInt16LE(Math.max(-32768, Math.min(32767, Math.round(w.mono[i] * 32768))), 44 + i * align + c * 2);
  }
  fs.writeFileSync(p, b, {flag: 'wx'});
}
const ready = {schema: 2, sampleRate: rate, channels: 2, qpcFrequency: '10000000', startQpcTicks: '1000000000', startQpc100ns: 1000000000,
  utcBeforeFileTime: (BigInt(epoch) * 10000n + 116444736000000000n).toString(),
  utcAfterFileTime: (BigInt(epoch) * 10000n + 116444736000000000n + 10n).toString()};
const capture = {schema: 2, passed: true, seconds: 300, sampleRate: rate, channels: 2, packets: 30000, timestampErrors: 0,
  endQpcTicks: '4000100000', endUtcBeforeFileTime: (BigInt(ready.utcBeforeFileTime) + 3000100000n).toString(),
  endUtcAfterFileTime: (BigInt(ready.utcAfterFileTime) + 3000100000n).toString()};
const packets = Array.from({length: 30000}, (_, i) => [i * 80, 80, 0, i * 80, 1000000000 + i * 100000]);
const results = [], positive = (name, fn) => { fn(); results.push({name, expected: 'accept', passed: true}); };
const negative = (name, fn, message) => {
  let failure; try { fn(); } catch (e) { failure = e; }
  assert(failure, 'Must reject ' + name); assert.match(failure.message, message);
  results.push({name, expected: 'reject', passed: true, reason: failure.message});
};
let error = null, cliRun = null;
try {
  const gains = [1, protocol.criteria.attenuationAmplitudeExpected, protocol.criteria.attenuationAmplitudeExpected,
    protocol.criteria.attenuationAmplitudeExpected * .4];
  const cases = protocol.cases.map((name, i) => ({name, audio: analyze(gains[i])}));
  positive('Four fixed cases: two active Stops, exclusive pitches, silence/replay, unity/-6dB and order difference', () => compareSendCases(cases, protocol, scope));
  positive('Calibrated packet coverage and clocks', () => verifySendPackets(ready, capture, packets, signal('endpoint', 1)));
  const integerFile = path.join(output, 'integer-16bit.wav'); writeWave(integerFile, signal('source', 1), 1, 16);
  positive('Independent integer FileOutput WAVE decode', () => { const w = readSendPcm(integerFile); assert.equal(w.frames, Math.round(fileSeconds * rate)); assert(w.peak > .02 && w.peak < .04); });
  negative('Missing replay', () => analyze(.5011872336272722, {missingReplay: true}), /Exactly two musical PCM starts/);
  negative('Third musical start between Stops', () => analyze(.5011872336272722, {extraStart: true}), /Exactly two musical PCM starts/);
  negative('Wrong adjacent semitone on raw source', () => analyze(.5011872336272722, {wrongPitch: true}), /Expected source\/control pitch/);
  negative('Control pitch leaks into raw source tap', () => analyze(.5011872336272722, {crossLeak: true}), /Exclusive routed tap pitch/);
  negative('Post-Stop audible interval', () => analyze(.5011872336272722, {afterStop: true}), /Exactly two musical PCM starts|Stop.*silence/);
  negative('Notes end before GUI Stop', () => analyze(.5011872336272722, {earlyEnd: true}), /Stop interrupts active PCM/);
  negative('Destination starts late', () => analyze(.5011872336272722, {destinationDelay: .12}), /Simultaneous tap routes/);
  negative('Destination replay spacing differs despite first alignment', () => analyze(.5011872336272722, {replayDelay: .12}), /Simultaneous tap routes/);
  negative('Clipped recording', () => analyze(.5011872336272722, {clipping: true}), /Clipping peak/);
  const late = structuredClone(times); late.stopFirst = {before: 55, after: 55.1}; late.replay = {before: 60, after: 60.1}; late.stopReplay = {before: 68, after: 68.1}; late.stopRecording = {before: 72, after: 72.1}; late.close = {before: 76, after: 76.1};
  negative('Stop exceeds fixed 40 second deadline', () => analyze(.5011872336272722, {}, late), /Stop before fixed active-note deadline/);
  const changedGain = structuredClone(cases); changedGain[1].audio.plays[1].ratio = .6;
  negative('Replay gain wrong even if first -6dB passes', () => compareSendCases(changedGain, protocol, scope), /Minus600 is -6dB/);
  const identicalOrder = structuredClone(cases); identicalOrder[3].audio.plays[1].ratio = identicalOrder[2].audio.plays[1].ratio;
  negative('Waves order difference missing on replay', () => compareSendCases(identicalOrder, protocol, scope), /After\/before Waves/);
  const changedControl = structuredClone(cases); changedControl[2].audio.plays[0].spectral[2].tone *= 1.2;
  negative('Untouched control changes across cases', () => compareSendCases(changedControl, protocol, scope), /Untouched route stable/);
  negative('Reordered case identity', () => compareSendCases([...cases].reverse(), protocol, scope), /Four cases in frozen order/);
  const timestampError = packets.map(p => [...p]); timestampError[900][2] = 4;
  negative('Packet timestamp error', () => verifySendPackets(ready, capture, timestampError, signal('endpoint', 1)), /Packet timestamp error/);
  const gap = packets.filter((_, i) => i !== 900);
  negative('Omitted packet hidden as silence', () => verifySendPackets(ready, {...capture, packets: gap.length}, gap, signal('endpoint', 1)), /Packet coverage gaps/);
  const drifting = {...capture, endUtcBeforeFileTime: (BigInt(capture.endUtcBeforeFileTime) + 1000000n).toString(), endUtcAfterFileTime: (BigInt(capture.endUtcAfterFileTime) + 1000000n).toString()};
  negative('UTC/QPC drift', () => verifySendPackets(ready, drifting, packets, signal('endpoint', 1)), /UTC\/QPC drift/);
  const malformed = path.join(output, 'malformed-wave.wav'), bad = fs.readFileSync(integerFile); bad.writeUInt32LE(bad.length + 16, 40); fs.writeFileSync(malformed, bad, {flag: 'wx'});
  negative('WAVE data exceeds RIFF', () => readSendPcm(malformed), /Chunk exceeds parent/);

  const record = {schema: 1, createdUtc: new Date().toISOString(), candidate: protocol.candidate,
    protocolSha256: hash(protocolPath), scopeSha256: hash(scopePath), kind: 'fabricated-control-input', cases: []};
  for (let i = 0; i < protocol.cases.length; i++) {
    const dir = path.join(output, protocol.cases[i]); fs.mkdirSync(dir);
    const endpointPath = path.join(dir, 'endpoint.wav'); writeWave(endpointPath, signal('endpoint', gains[i]));
    const tapPaths = ['source', 'control', 'destination'].map((role, n) => {
      const p = path.join(dir, ['Record.wav', 'Record1.wav', 'Record2.wav'][n]); writeWave(p, signal(role, gains[i])); return p;
    });
    for (const [name, obj] of [['ready.json', ready], ['capture.json', capture]]) write(path.join(dir, name), obj);
    fs.writeFileSync(path.join(dir, 'packets.csv'), 'frame,frames,flags,position,qpc\n' + packets.map(p => p.join(',')).join('\n') + '\n', {flag: 'wx'});
    const actionNames = ['StartRecording', 'PlayFirst', 'StopFirst', 'PlayReplay', 'StopReplay', 'StopRecording', 'NormalClose'];
    write(path.join(dir, 'actions.json'), Object.entries(times).map(([name, t], n) => ({name: actionNames[n],
      beforeUtc: new Date(epoch + t.before * 1000).toISOString(), afterUtc: new Date(epoch + t.after * 1000).toISOString()})));
    record.cases.push({name: protocol.cases[i], endpoint: ref(endpointPath), taps: tapPaths.map(ref),
      ready: ref(path.join(dir, 'ready.json')), capture: ref(path.join(dir, 'capture.json')),
      packets: ref(path.join(dir, 'packets.csv')), actions: ref(path.join(dir, 'actions.json'))});
  }
  const recordPath = path.join(output, 'numeric-input.json'); write(recordPath, record);
  const cliOutput = path.join(output, 'numeric-proof.json'), startedUtc = new Date().toISOString();
  const cli = spawnSync(process.execPath, ['scripts/Inspect-AudioPathSendGuiAudio.mjs', recordPath, protocolPath, scopePath, cliOutput], {encoding: 'utf8'});
  fs.writeFileSync(path.join(output, 'cli.stdout.txt'), cli.stdout ?? '', {flag: 'wx'});
  fs.writeFileSync(path.join(output, 'cli.stderr.txt'), cli.stderr ?? '', {flag: 'wx'});
  cliRun = {processId: cli.pid, startedUtc, completedUtc: new Date().toISOString(), exitCode: cli.status, signal: cli.signal, error: cli.error?.message ?? null};
  write(path.join(output, 'cli-run.json'), cliRun);
  positive('Complete numeric CLI on independently fabricated four WAVE/packet/action sets', () => { assert.equal(cli.status, 0, cli.stderr); assert(json(cliOutput).passed); });
} catch (e) { error = {name: e.name, message: e.message, stack: e.stack}; }
const report = {schema: 1, createdUtc: new Date().toISOString(), passed: !error, kind: 'fabricated-numeric-controls',
  positiveCount: results.filter(r => r.expected === 'accept').length, negativeCount: results.filter(r => r.expected === 'reject').length,
  results, error, cliRun, recipe: {sampleRate: rate, channels: 2, endpointSeconds: 300, fileSeconds, first, replay, fileOrigin, sourceAmplitude: .03, controlAmplitude: .025, times},
  protocol: ref(protocolPath), scope: ref(scopePath), auditor: ref('scripts/Inspect-AudioPathSendGuiAudio.mjs'),
  dependencies: [ref('scripts/AudioCaptureClock.mjs'), ref('scripts/Inspect-AudioPathSendNative.mjs')],
  controlDriver: ref(fileURLToPath(import.meta.url)), productCaptureExecuted: false,
  limitations: ['Fabricated PCM/packet/action controls validate only numeric logic; actual main and normal GUI lifecycles remain pending'], fullAcceptance: false};
write(path.join(output, 'controls.json'), report);
console.log(JSON.stringify({passed: report.passed, positiveCount: report.positiveCount, negativeCount: report.negativeCount, error: error?.message, cliRun}));
if (error) process.exitCode = 1;
