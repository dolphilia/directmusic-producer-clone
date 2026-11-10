import assert from 'node:assert/strict';

// FILETIME values exceed Number's exact integer range. Subtract with BigInt
// before converting intervals to seconds; UTC and QPC are different clocks.
export function captureClock(ready,capture,packets,rate){
  assert.equal(ready.schema,2,'Recorder start UTC/QPC calibration required');
  assert.equal(capture.schema,2,'Recorder end UTC/QPC calibration required');
  const integer=x=>{assert.match(x,/^\d+$/);return BigInt(x);};
  const before=integer(ready.utcBeforeFileTime),after=integer(ready.utcAfterFileTime);
  const endBefore=integer(capture.endUtcBeforeFileTime),endAfter=integer(capture.endUtcAfterFileTime);
  assert(after>=before&&endAfter>=endBefore);
  const startUtc=(before+after)/2n,endUtc=(endBefore+endAfter)/2n;
  const frequency=integer(ready.qpcFrequency),startQpc=integer(ready.startQpcTicks),endQpc=integer(capture.endQpcTicks);
  assert(frequency>0n&&endQpc>startQpc);
  const bracketSeconds=Math.max(Number(after-before),Number(endAfter-endBefore))/1e7;
  assert(bracketSeconds<=.002,'UTC/QPC sample bracket too wide');
  const qpc100ns=Number(startQpc)*1e7/Number(frequency);
  assert(Math.abs(qpc100ns-ready.startQpc100ns)<.001);
  assert(packets.length>0);
  let maxPacketOriginErrorSeconds=0;
  for(const p of packets){
    assert(p.length===5&&p.every(Number.isFinite));
    // Recorder truncates the QPC-relative frame offset toward zero.
    maxPacketOriginErrorSeconds=Math.max(maxPacketOriginErrorSeconds,Math.abs((p[4]-qpc100ns)/1e7-p[0]/rate));
  }
  assert(maxPacketOriginErrorSeconds<=1/rate+.000001,'Packet frame/QPC origin mismatch');
  const qpcDuration=Number(endQpc-startQpc)/Number(frequency),utcDuration=Number(endUtc-startUtc)/1e7;
  const driftSeconds=Math.abs(qpcDuration-utcDuration);
  assert(driftSeconds<=.005,'UTC/QPC drift or system clock change');
  assert(qpcDuration>=capture.seconds&&qpcDuration<capture.seconds+1);
  const utcToSeconds=iso=>{
    const ms=Date.parse(iso);assert(Number.isSafeInteger(ms));
    // Date.parse retains milliseconds only; PowerShell's round-trip UTC
    // format includes seven fractional digits (100ns FILETIME precision).
    const fraction=iso.match(/\.(\d{1,7})(?:Z|[+-]\d\d:\d\d)$/)?.[1];
    const remainder=fraction?BigInt(fraction.padEnd(7,'0').slice(3)):0n;
    return Number(BigInt(ms)*10000n+remainder+116444736000000000n-startUtc)/1e7;
  };
  return {utcToSeconds,evidence:{schema:1,startUtcFileTime:startUtc.toString(),bracketSeconds,driftSeconds,qpcDuration,utcDuration,maxPacketOriginErrorSeconds}};
}
