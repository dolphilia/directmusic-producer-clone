import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {captureClock} from './AudioCaptureClock.mjs';
import {chunks} from './Inspect-AudioPathSendNative.mjs';

const json = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const ref = p => ({path: path.resolve(p), sha256: hash(p)});
const one = (cs, id) => { const f = cs.filter(c => c.id === id); assert.equal(f.length, 1, 'Unique ' + id); return f[0]; };

// Reads both the actual float32 loopback and integer/float FileOutput files.
// Container/format lengths and every channel's sample are checked independently.
export function readSendPcm(file) {
  const root = one(chunks(fs.readFileSync(file)), 'RIFF');
  assert.equal(root.type, 'WAVE');
  const fmt = one(root.children, 'fmt ').data, data = one(root.children, 'data').data;
  assert(fmt.length >= 16, 'PCM format header');
  let tag = fmt.readUInt16LE(0);
  const channels = fmt.readUInt16LE(2), rate = fmt.readUInt32LE(4);
  const align = fmt.readUInt16LE(12), bits = fmt.readUInt16LE(14);
  if (tag === 65534) {
    assert(fmt.length >= 40 && fmt.readUInt16LE(16) >= 22, 'Extensible PCM format');
    assert.equal(fmt.readUInt16LE(18), bits, 'All stored bits valid');
    assert.equal(fmt.subarray(28, 40).toString('hex'), '00001000800000aa00389b71', 'PCM subtype GUID suffix');
    tag = fmt.readUInt32LE(24);
  }
  assert([1, 3].includes(tag), 'PCM or IEEE float');
  assert(Number.isInteger(channels) && channels >= 1 && channels <= 32);
  assert(rate >= 8000 && rate <= 192000);
  assert([8, 16, 24, 32].includes(bits));
  if (tag === 3) assert.equal(bits, 32);
  assert.equal(align, channels * bits / 8);
  assert.equal(fmt.readUInt32LE(8), rate * align);
  assert.equal(data.length % align, 0);
  const frames = data.length / align, mono = new Float64Array(frames), energy = new Float64Array(frames);
  let peak = 0;
  for (let i = 0; i < frames; i++) for (let c = 0; c < channels; c++) {
    const p = i * align + c * bits / 8;
    const x = tag === 3 ? data.readFloatLE(p) : bits === 8 ? (data[p] - 128) / 128 :
      bits === 16 ? data.readInt16LE(p) / 32768 : bits === 24 ? data.readIntLE(p, 3) / 8388608 : data.readInt32LE(p) / 2147483648;
    assert(Number.isFinite(x), 'Finite PCM sample');
    mono[i] += x / channels; energy[i] += x * x / channels; peak = Math.max(peak, Math.abs(x));
  }
  return {rate, channels, bits, tag, frames, seconds: frames / rate, mono, energy, peak};
}

function rms(w, a, z) {
  assert(a >= 0 && z > a && z <= w.seconds, 'PCM window in recording');
  const from = Math.ceil(a * w.rate), to = Math.floor(z * w.rate);
  assert(to > from);
  let total = 0; for (let i = from; i < to; i++) total += w.energy[i];
  return Math.sqrt(total / (to - from));
}
function tone(w, a, z, hz) {
  assert(a >= 0 && z > a && z <= w.seconds);
  const step = Math.max(1, Math.floor(w.rate / 8000)), k = 2 * Math.cos(2 * Math.PI * hz / (w.rate / step));
  let s1 = 0, s2 = 0, n = 0;
  for (let i = Math.ceil(a * w.rate); i < Math.floor(z * w.rate); i += step) {
    const hann = .5 - .5 * Math.cos(2 * Math.PI * (i / w.rate - a) / (z - a));
    const s = w.mono[i] * hann + k * s1 - s2; s2 = s1; s1 = s; n++;
  }
  return 2 * Math.sqrt(Math.max(0, s1 * s1 + s2 * s2 - k * s1 * s2)) / n;
}
function onsets(w, c) {
  const found = []; let active = false, quietSince = 0;
  for (let bin = 0; (bin + 1) * .01 <= w.seconds; bin++) {
    const t = bin * .01, level = rms(w, t, t + .01);
    if (level < c.stopSilenceRmsMaximum) { if (active) { active = false; quietSince = t; } }
    else if (level > c.activeRmsMinimum && !active) {
      if (t - quietSince >= .2) found.push(t); active = true;
    }
  }
  return found;
}

export function validateSendTiming(times, protocol, scope, seconds) {
  const r = protocol.operationRules, limit = scope.additionalAuditRules.actionIntervalSecondsMaximum;
  const names = ['startRecording', 'first', 'stopFirst', 'replay', 'stopReplay', 'stopRecording', 'close'];
  for (let i = 0; i < names.length; i++) {
    const t = times[names[i]]; assert(t && Number.isFinite(t.before) && Number.isFinite(t.after));
    assert(t.before >= 0 && t.after >= t.before && t.after - t.before <= limit, 'Bounded input interval');
    if (i) assert(times[names[i - 1]].after < t.before, 'Ordered recording/transport/close actions');
  }
  assert(times.first.before >= r.baselineSecondsMinimum, 'Endpoint baseline hold');
  assert(times.first.before - times.startRecording.after >= r.baselineSecondsMinimum, 'FileOutput baseline hold');
  for (const [play, stop, hold] of [[times.first, times.stopFirst, r.firstActiveHoldSecondsMinimum],
    [times.replay, times.stopReplay, r.replayActiveHoldSecondsMinimum]]) {
    assert(stop.before - play.after >= hold, 'Active hold before Stop');
    assert(stop.after - play.before <= r.maximumPlayToStopSeconds, 'Stop before fixed active-note deadline');
  }
  assert(times.replay.before - times.stopFirst.after >= r.stoppedHoldSecondsMinimum, 'Stopped hold before replay');
  assert(times.stopRecording.before - times.stopReplay.after >= r.finalStoppedHoldSecondsMinimum, 'Final stopped hold');
  assert(times.close.after + r.finalStoppedHoldSecondsMinimum <= seconds, 'Capture retains normal-close quiet');
  return true;
}

// Endpoint time is UTC/QPC-calibrated. FileOutput has its own frame origin;
// the first source onset aligns waveforms, not a fabricated DMO UTC timestamp.
// The second onset spacing, all three route starts, Stop windows and capture
// duration must corroborate that alignment independently.
export function analyzeSendPcm(recordings, times, protocol, scope) {
  const {endpoint, taps} = recordings, c = protocol.criteria, rules = scope.additionalAuditRules;
  assert.equal(taps.length, 3, 'Three FileOutput taps');
  assert.equal(endpoint.frames, endpoint.rate * protocol.recorder.durationSeconds);
  validateSendTiming(times, protocol, scope, endpoint.seconds);
  for (const w of [endpoint, ...taps]) {
    assert(w.frames > 0 && w.energy.length === w.frames && w.mono.length === w.frames);
    assert(w.peak < c.peakMaximum, 'Clipping peak');
  }
  assert(taps.every(w => w.channels === 2 && w.rate === taps[0].rate), 'Three stereo taps with common rate');
  assert(Math.max(...taps.map(w => w.seconds)) - Math.min(...taps.map(w => w.seconds)) <= protocol.pcmWindows.maximumRouteOnsetDifferenceSeconds, 'Tap recording duration agreement');
  const starts = [endpoint, ...taps].map(w => onsets(w, c));
  assert(starts.every(s => s.length === 2), 'Exactly two musical PCM starts on endpoint and every tap');
  for (const [i, play] of [times.first, times.replay].entries())
    assert(starts[0][i] >= play.before - .01 && starts[0][i] <= play.after + rules.onsetAfterPlaySecondsMaximum, 'PCM onset tied to actual GUI Play');
  const alignment = starts[0][0] - starts[1][0];
  const routeLimit = protocol.pcmWindows.maximumRouteOnsetDifferenceSeconds;
  for (let route = 1; route <= 3; route++) for (let i = 0; i < 2; i++)
    assert(Math.abs(starts[route][i] + alignment - starts[0][i]) <= routeLimit, 'Simultaneous tap routes and independent replay spacing');
  assert(alignment >= times.startRecording.before - rules.onsetAfterPlaySecondsMaximum &&
    alignment <= times.startRecording.after + rules.onsetAfterPlaySecondsMaximum, 'File frame origin consistent with recording start');
  for (const w of taps) assert(Math.abs(w.seconds + alignment - times.stopRecording.before) <=
    rules.onsetAfterPlaySecondsMaximum + times.stopRecording.after - times.stopRecording.before, 'File frames span recording lifecycle');
  const silenceRanges = [[rules.baselineWindowStartSeconds, times.first.before - .05],
    [times.stopFirst.after + protocol.pcmWindows.stoppedAfterAction[0], times.replay.before - .05],
    [times.stopReplay.after + protocol.pcmWindows.stoppedAfterAction[0], times.stopRecording.before - .05]];
  const silence = [endpoint, ...taps].map((w, index) => {
    const offset = index ? alignment : 0;
    return silenceRanges.map(([a, z], n) => {
      const level = rms(w, Math.max(rules.baselineWindowStartSeconds, a - offset), z - offset);
      assert(level <= c.stopSilenceRmsMaximum, 'Baseline/inter-Stop/final Stop silence');
      return {name: ['baseline', 'stop-hold', 'final-stop'][n], from: Math.max(rules.baselineWindowStartSeconds, a - offset), to: z - offset, rms: level};
    });
  });
  const plays = [times.stopFirst, times.stopReplay].map((stop, i) => {
    const from = starts[0][i] + protocol.pcmWindows.sourceSteadyAfterEachActualPcmOnset[0];
    const to = starts[0][i] + protocol.pcmWindows.sourceSteadyAfterEachActualPcmOnset[1];
    assert(to < stop.before, 'Steady PCM window before Stop');
    const spectral = [endpoint, ...taps].map((w, index) => {
      const a = from - (index ? alignment : 0), z = to - (index ? alignment : 0);
      const level = rms(w, a, z); assert(level >= c.activeRmsMinimum, 'Every tap and endpoint audible');
      const hz = index === 2 ? c.routedPChannel8ToneHz : c.rawSourceToneHz;
      const own = tone(w, a, z, hz), lower = tone(w, a, z, hz / 2 ** (1 / 12)), upper = tone(w, a, z, hz * 2 ** (1 / 12));
      assert(own > rules.toneAmplitudeMinimum && own >= Math.max(lower, upper) * rules.adjacentSemitoneRatioMinimum, 'Expected source/control pitch vs adjacent semitones');
      const cross = tone(w, a, z, index === 2 ? c.rawSourceToneHz : c.routedPChannel8ToneHz);
      if (index) assert(own > cross * c.pitchRatioMinimum, 'Exclusive routed tap pitch');
      else {
        const adjacent = Math.max(tone(w, a, z, c.routedPChannel8ToneHz / 2 ** (1 / 12)), tone(w, a, z, c.routedPChannel8ToneHz * 2 ** (1 / 12)));
        assert(cross > rules.toneAmplitudeMinimum && cross >= adjacent * rules.adjacentSemitoneRatioMinimum, 'Endpoint contains both owned pitches');
      }
      const beforeStop = rms(w, stop.before + protocol.pcmWindows.beforeStopBeforeAction[0] - (index ? alignment : 0), stop.before + protocol.pcmWindows.beforeStopBeforeAction[1] - (index ? alignment : 0));
      assert(beforeStop >= c.activeRmsMinimum, 'Stop interrupts active PCM on every route');
      const quiet = rms(w, stop.after + protocol.pcmWindows.stoppedAfterAction[0] - (index ? alignment : 0), stop.after + protocol.pcmWindows.stoppedAfterAction[1] - (index ? alignment : 0));
      assert(quiet <= c.stopSilenceRmsMaximum, 'Stop silence window');
      return {activeRms: level, tone: own, lower, upper, cross, beforeStopRms: beforeStop, quietRms: quiet};
    });
    return {onset: starts[0][i], spectral, ratio: spectral[3].tone / spectral[1].tone};
  });
  const controlReplayRelativeChange = Math.abs(plays[1].spectral[2].tone / plays[0].spectral[2].tone - 1);
  assert(controlReplayRelativeChange <= rules.unchangedControlRelativeTolerance, 'Untouched control stable across replay');
  const normalCloseQuiet = rms(endpoint, times.close.after, endpoint.seconds - .3);
  assert(normalCloseQuiet <= c.stopSilenceRmsMaximum, 'Normal close remains silent');
  return {passed: true, starts, silence, plays, normalCloseQuiet, controlReplayRelativeChange,
    peak: [endpoint, ...taps].map(w => w.peak), fileFrameAlignmentToEndpointSeconds: alignment,
    fileTimeBasis: 'First source PCM onset correlation; own frame origins and independent second-onset spacing; no absolute FileOutput UTC calibration',
    runtimeTempoMeasured: false};
}

export function compareSendCases(cases, protocol, scope) {
  assert.deepEqual(cases.map(x => x.name), protocol.cases, 'Four cases in frozen order');
  const [zero, minus, before, after] = cases.map(x => x.audio), c = protocol.criteria;
  for (const p of zero.plays) assert(Math.abs(p.ratio - 1) <= c.zeroRatioTolerance, 'Zero attenuation unity');
  for (const r of [minus, before]) for (const p of r.plays)
    assert(Math.abs(p.ratio - c.attenuationAmplitudeExpected) <= c.attenuationRatioTolerance, 'Minus600 is -6dB amplitude');
  const comparisons = zero.plays.map((_, i) => {
    const control = cases.map(x => x.audio.plays[i].spectral[2].tone);
    const changes = control.map(x => Math.abs(x / control[0] - 1));
    assert(changes.every(x => x <= scope.additionalAuditRules.unchangedControlRelativeTolerance), 'Untouched route stable across cases');
    const orderDifference = Math.abs(after.plays[i].ratio / before.plays[i].ratio - 1);
    assert(orderDifference >= c.afterWavesVsBeforeDifferenceMinimum, 'After/before Waves Send branch difference on both plays');
    return {zeroRatio: zero.plays[i].ratio, minusRatio: minus.plays[i].ratio,
      beforeWavesRatio: before.plays[i].ratio, afterWavesRatio: after.plays[i].ratio, orderDifference, controlRelativeChanges: changes};
  });
  return {passed: true, comparisons};
}

export function verifySendPackets(ready, capture, packets, recorded) {
  assert.equal(recorded.rate, ready.sampleRate); assert.equal(recorded.channels, ready.channels);
  assert.equal(recorded.frames, recorded.rate * capture.seconds);
  assert(packets.length > 100, 'Packet inventory');
  assert.equal(capture.packets, packets.length); assert.equal(capture.timestampErrors, 0);
  let maxGapFrames = 0, maxOverlapFrames = 0;
  for (let i = 0; i < packets.length; i++) {
    const p = packets[i]; assert(p.length === 5 && p.every(Number.isFinite));
    assert(p.every(Number.isSafeInteger) && p[1] > 0, 'Integer packet data');
    assert.equal(p[2] & 4, 0, 'Packet timestamp error');
    if (p[2] & 1) assert(p[0] / recorded.rate < .05, 'Only startup discontinuity');
    if (i) {
      assert(p[0] >= packets[i - 1][0] && p[3] >= packets[i - 1][3], 'Ordered packet positions');
      const gap = p[0] - packets[i - 1][0] - packets[i - 1][1];
      maxGapFrames = Math.max(maxGapFrames, gap); maxOverlapFrames = Math.max(maxOverlapFrames, -gap);
    }
  }
  assert(maxGapFrames <= recorded.rate * .002 && maxOverlapFrames <= recorded.rate * .002, 'Packet coverage gaps/overlap');
  assert(packets[0][0] <= recorded.rate * .05, 'Packet baseline coverage');
  assert(packets.at(-1)[0] + packets.at(-1)[1] >= recorded.frames - recorded.rate * .05, 'Packet final quiet coverage');
  const clock = captureClock(ready, capture, packets, recorded.rate);
  return {clock, evidence: {passed: true, maxGapFrames, maxOverlapFrames, clock: clock.evidence}};
}

// This CLI is intentionally a numerical layer. A separate actual-main dependency,
// native-input, two-exit and screenshot audit is still required before acceptance.
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [input, protocolPath, scopePath, output] = process.argv.slice(2); assert(output, 'Usage: numeric-input protocol scope output');
  assert(!fs.existsSync(output), 'Preserve previous numeric audit');
  const protocol = json(protocolPath), scope = json(scopePath), record = json(input);
  assert.equal(scope.protocol.sha256, hash(protocolPath)); assert.equal(record.protocolSha256, hash(protocolPath));
  assert.equal(record.scopeSha256, hash(scopePath)); assert.equal(record.candidate, protocol.candidate);
  assert(Date.parse(scope.createdUtc) < Date.parse(record.createdUtc), 'Fixed numerical scope precedes actual captures');
  const cases = record.cases.map(c => {
    for (const f of [c.endpoint, ...c.taps, c.ready, c.capture, c.packets, c.actions]) assert.equal(hash(f.path), f.sha256, 'Numeric input hash');
    const endpoint = readSendPcm(c.endpoint.path), taps = c.taps.map(f => readSendPcm(f.path));
    const ready = json(c.ready.path), capture = json(c.capture.path);
    assert(capture.passed); assert.equal(capture.seconds, protocol.recorder.durationSeconds);
    const packets = fs.readFileSync(c.packets.path, 'utf8').trim().split(/\r?\n/).slice(1).map(l => l.split(',').map(Number));
    const verified = verifySendPackets(ready, capture, packets, endpoint), actions = json(c.actions.path);
    const actionNames = ['StartRecording', 'PlayFirst', 'StopFirst', 'PlayReplay', 'StopReplay', 'StopRecording', 'NormalClose'];
    const names = ['startRecording', 'first', 'stopFirst', 'replay', 'stopReplay', 'stopRecording', 'close'];
    const times = Object.fromEntries(names.map((n, i) => {
      const found = actions.filter(a => a.name === actionNames[i]); assert.equal(found.length, 1, 'Unique action ' + actionNames[i]);
      return [n, {before: verified.clock.utcToSeconds(found[0].beforeUtc), after: verified.clock.utcToSeconds(found[0].afterUtc)}];
    }));
    return {name: c.name, audio: analyzeSendPcm({endpoint, taps}, times, protocol, scope), packets: verified.evidence, times, inputs: c};
  });
  const comparison = compareSendCases(cases, protocol, scope);
  const proof = {schema: 1, createdUtc: new Date().toISOString(), passed: true, kind: 'numeric-audio-only',
    candidate: protocol.candidate, cases, comparison, input: ref(input), protocol: ref(protocolPath), scope: ref(scopePath),
    auditor: ref(fileURLToPath(import.meta.url)), dependencies: ['./AudioCaptureClock.mjs', './Inspect-AudioPathSendNative.mjs'].map(p => ref(fileURLToPath(new URL(p, import.meta.url)))),
    limitations: ['Actual main native input/build/process/window/two-exit/screenshot and recording finalization binding is a separate prerequisite',
      'FileOutput onset correlation does not measure its absolute UTC start; runtime tempo and physical speakers not measured'], fullAcceptance: false};
  fs.writeFileSync(output, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'});
  console.log(JSON.stringify({passed: true, kind: proof.kind, cases: cases.length, comparison}));
}
