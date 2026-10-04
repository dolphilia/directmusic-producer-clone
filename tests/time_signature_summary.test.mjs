import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import test from 'node:test';

const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const root=path.join(repo,'work/reference/time-signature');
const original=path.join(root,'20261002T095342635Z');
const serviceOriginal=path.join(root,'20261002T113228581Z');
const verifier=path.join(repo,'scripts/Summarize-TimeSignatureProbe.mjs');
const checks=path.join(root,'summary-checks',crypto.randomUUID());
fs.mkdirSync(checks,{recursive:true});
const outcomes=[];
function verify(directory){return spawnSync(process.execPath,[verifier,directory],{encoding:'utf8'});}
function record(name,result){outcomes.push({name,exitCode:result.status,stdout:result.stdout,stderr:result.stderr});
  fs.writeFileSync(path.join(checks,'results.json'),JSON.stringify({verifierSha256:crypto.createHash('sha256').update(fs.readFileSync(verifier)).digest('hex'),outcomes},null,2)+'\n');}
function rejected(name,change,expected,source=original){
  const directory=path.join(checks,name);fs.cpSync(source,directory,{recursive:true});change(directory);
  const result=verify(directory);record(name,result);
  assert.notEqual(result.status,0);assert.match(result.stderr,expected);
}
test('all observed original profiles are accepted',()=>{
  for(const [name,records,queries] of [['20261002T094657214Z',176,126],['20261002T095342635Z',261,196],['20261002T113228581Z',322,196]]){
    const result=verify(path.join(root,name));record(name,result);assert.equal(result.status,0,result.stderr);
    const summary=JSON.parse(result.stdout);assert.equal(summary.observedRecords,records);assert.equal(summary.clockQueries,queries);
  }
});
test('changed input is rejected',()=>rejected('input',directory=>{
  const file=path.join(directory,'four_four-input.bin'),bytes=fs.readFileSync(file);bytes[12]=1;fs.writeFileSync(file,bytes);
},/Input\/load mismatch/));
test('query inconsistent with saved events is rejected',()=>rejected('query',directory=>{
  const file=path.join(directory,'probe.jsonl'),records=fs.readFileSync(file,'utf8').trim().split(/\r?\n/).map(JSON.parse);
  records.find(r=>r.operation==='get_meter'&&r.case==='four_four'&&r.time===0).next=1;
  fs.writeFileSync(file,records.map(JSON.stringify).join('\n')+'\n');
},/Query\/save consistency mismatch/));
test('invalid saved container length is rejected',()=>rejected('container',directory=>{
  const file=path.join(directory,'four_four-saved.bin'),bytes=fs.readFileSync(file);bytes.writeUInt32LE(1,4);fs.writeFileSync(file,bytes);
},/Invalid TIMS wrapper/));
test('changed native source snapshot is rejected',()=>rejected('source',directory=>{
  fs.appendFileSync(path.join(directory,'sources/src/time_signature/time_signature_map.cpp'),'\n// altered test snapshot\n');
},/Source snapshot mismatch/));
test('blocked native launch cannot be summarized as a successful observation',()=>{
  const result=verify(path.join(root,'20261002T105324536Z'));record('blocked-launch',result);
  assert.notEqual(result.status,0);assert.match(result.stderr,/Original run failed/);
});
function changeRecord(directory,select,change){
  const file=path.join(directory,'probe.jsonl'),records=fs.readFileSync(file,'utf8').trim().split(/\r?\n/).map(JSON.parse);
  change(records.find(select));fs.writeFileSync(file,records.map(JSON.stringify).join('\n')+'\n');
}
test('a service getter returning the noncanonical interface is rejected',()=>rejected('service-identity',directory=>{
  changeRecord(directory,r=>r.operation==='service_connection'&&r.property===1&&r.phase==='get_connected',r=>r.canonicalUnknown=false);
},/Service 1\/get_connected mismatch/,serviceOriginal));
test('missing destruction release is rejected',()=>rejected('service-release',directory=>{
  changeRecord(directory,r=>r.operation==='service_connection'&&r.property===2&&r.phase==='after_manager_release',r=>r.refs=2);
},/Owned service release mismatch/,serviceOriginal));
test('a retained borrowed reference is rejected',()=>rejected('borrowed-release',directory=>{
  changeRecord(directory,r=>r.operation==='borrowed_after_manager_release',r=>r.secondRefs=2);
},/Borrowed lifetime mismatch/,serviceOriginal));
test('changed service probe build provenance is rejected',()=>rejected('build-provenance',directory=>{
  const file=path.join(directory,'run.json'),run=JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));run.buildSummarySha256='0'.repeat(64);fs.writeFileSync(file,JSON.stringify(run));
},/Build provenance mismatch/,serviceOriginal));
console.log('Verifier evidence: '+path.relative(repo,checks).replaceAll('\\','/'));
