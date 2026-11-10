import fs from 'node:fs';import path from 'node:path';
const target=path.resolve(process.argv[2]);fs.mkdirSync(path.dirname(target),{recursive:true});
// One 100ms, integer-cycle 440Hz period; no original sample content.
const frames=800,rate=8000,b=Buffer.alloc(44+frames*2);b.write('RIFF');b.writeUInt32LE(b.length-8,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);b.writeUInt16LE(1,20);b.writeUInt16LE(1,22);b.writeUInt32LE(rate,24);b.writeUInt32LE(rate*2,28);b.writeUInt16LE(2,32);b.writeUInt16LE(16,34);b.write('data',36);b.writeUInt32LE(frames*2,40);
for(let i=0;i<frames;i++)b.writeInt16LE(Math.round(12000*Math.sin(2*Math.PI*440*i/rate)),44+i*2);
fs.writeFileSync(target,b);console.log(target);
