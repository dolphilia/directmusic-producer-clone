import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {inspectFarmBirdScoreNotes,readFarmBirdScore} from './Inspect-FarmBirdScoreNotes.mjs';
const[directory,evidence]=process.argv.slice(2);assert(directory&&evidence&&!fs.existsSync(evidence),'Fresh evidence required');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const sourceNotes=directory+'/observations/notes.csv',text=fs.readFileSync(sourceNotes,'utf8'),rows=text.trim().split(/\r?\n/).map(l=>l.split(',')),head=rows.shift(),notes=rows.map(r=>Object.fromEntries(head.map((h,i)=>[h,h==='phase'||h==='observedFileTime'?r[i]:Number(r[i])])));
const baseline=inspectFarmBirdScoreNotes(path.resolve(directory));assert(baseline.scoreNotesPassed,'Actual positive run required');
const clone=()=>structuredClone(notes),solo=ns=>ns.filter(n=>n.phase==='Bird'),second=ns=>ns.find(n=>n.phase==='AfterBackgroundStop'&&n.channel===solo(ns)[0].channel),controls=[];
function reject(name,mutate){const changed=clone();mutate(changed);const result=inspectFarmBirdScoreNotes(path.resolve(directory),{observedNotesOverride:changed});assert(!result.scoreNotesPassed,name);controls.push({name,rejected:true});}
reject('Wrong MIDI pitch',ns=>solo(ns)[0].midiValue++);
reject('Wrong relative music clock',ns=>solo(ns)[1].clocks++);
reject('Wrong duration',ns=>solo(ns)[2].duration++);
reject('Wrong velocity',ns=>solo(ns)[3].velocity++);
reject('Wrong authored music value',ns=>solo(ns)[4].musicValue+=16);
reject('Missing note',ns=>ns.splice(ns.indexOf(solo(ns)[2]),1));
reject('Additional note',ns=>ns.push({...solo(ns)[0],clocks:solo(ns)[0].clocks+100}));
reject('Wrong inherited play mode',ns=>solo(ns)[0].playMode=0);
reject('Unexpected note-off flag',ns=>solo(ns)[0].flags=0);
reject('Coexistence wrong pitch',ns=>second(ns).midiValue++);
reject('Coexistence duration mismatch',ns=>second(ns).duration++);
const run=JSON.parse(fs.readFileSync(directory+'/run.json','utf8').replace(/^\uFEFF/,'')),input=run.inputs.find(i=>path.basename(i.path)==='SSBird.sgt'),bytes=fs.readFileSync(input.path);
const chunkOffset=id=>{const target=Buffer.from(id,'ascii');for(let i=0;i<bytes.length-8;i++)if(bytes.subarray(i,i+4).equals(target))return i+8;throw new Error('Missing input chunk '+id);};
for(const[name,mutate]of[
  ['Nonzero randomization unsupported',b=>b[chunkOffset('note')+4+17]=1],
  ['Changed grid resolution unsupported',b=>b.writeUInt16LE(2,chunkOffset('prth')+2)],
  ['Changed tempo unsupported',b=>b.writeDoubleLE(120,chunkOffset('tetr')+12)],
  ['Malformed note record width',b=>b.writeUInt32LE(20,chunkOffset('note'))]
]){const changed=Buffer.from(bytes);mutate(changed);assert.throws(()=>readFarmBirdScore(changed),undefined,name);controls.push({name,rejected:true});}
assert.equal(fs.readFileSync(sourceNotes,'utf8'),text);assert(fs.readFileSync(input.path).equals(bytes));
fs.mkdirSync(evidence);const result={schema:1,createdUtc:new Date().toISOString(),candidate:run.candidate,controlsPassed:true,controls,positive:{soloVariation:baseline.soloMatches[0].variationMask,soloNotes:baseline.soloMatches[0].notes.length,coexistenceVariation:baseline.coexistenceMatches[0].variationMask,coexistenceNotes:baseline.coexistenceMatches[0].notes.length},bindings:{runSha256:hash(directory+'/run.json'),inputSha256:hash(input.path),notesSha256:hash(sourceNotes),auditorSha256:hash(fileURLToPath(new URL('./Inspect-FarmBirdScoreNotes.mjs',import.meta.url))),testSha256:hash(fileURLToPath(import.meta.url))},scope:'Input-based native score note verification only. Does not validate PCM processing, Echo inversion, audible tempo, live mapping, main or whole Farm.',fullAcceptance:false};
fs.writeFileSync(evidence+'/negative-tests.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({controlsPassed:true,count:controls.length,...result.positive,fullAcceptance:false}));
