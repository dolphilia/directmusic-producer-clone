import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {verify} from '../scripts/Compare-TimeSignatureConnection.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const original=path.join(root,'work/reference/time-signature-connection/20261002T114827904Z');
const destination=path.join(root,'work/reference/time-signature-connection/checks',crypto.randomUUID());
fs.mkdirSync(destination,{recursive:true});
const outcomes=[];
test('accepts observed original Timeline connection protocol',()=>{
  const result=verify(original);assert.equal(result.observedRecords,160);assert.equal(result.connections,11);assert.equal(result.unloadVerified,true);
  outcomes.push({name:'original',accepted:true});
});
const mutations=[
  ['missing-notification',rows=>rows.splice(rows.findIndex(row=>row.operation==='remove_notification'),1)],
  ['registration-order',rows=>{const index=rows.findIndex(row=>row.operation==='add_notification');[rows[index],rows[index+1]]=[rows[index+1],rows[index]];}],
  ['failed-insert-return',rows=>{const index=rows.findIndex(row=>row.phase==='failed_insert_notifications');rows.slice(index).find(row=>row.operation==='set_timeline').hr='0x80004005';}],
  ['reference-leak',rows=>{rows.find(row=>row.operation==='final_fixture_refs').first=2;}],
  ['manager-identity',rows=>{rows.find(row=>row.operation==='strip_property'&&row.property===12).managerIdentity=false;}],
];
for(const [name,mutate] of mutations)test('rejects '+name,()=>{
  const copied=path.join(destination,name);fs.cpSync(original,copied,{recursive:true});
  const rows=fs.readFileSync(path.join(copied,'probe.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse);mutate(rows);
  fs.writeFileSync(path.join(copied,'probe.jsonl'),rows.map(row=>JSON.stringify(row)).join('\r\n')+'\r\n');
  assert.throws(()=>verify(copied),/Connection observation mismatch/);outcomes.push({name,accepted:false});
});
test('rejects changed build provenance',()=>{
  const copied=path.join(destination,'build-provenance');fs.cpSync(original,copied,{recursive:true});
  const file=path.join(copied,'run.json'),run=JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));run.buildSummarySha256='0'.repeat(64);fs.writeFileSync(file,JSON.stringify(run));
  assert.throws(()=>verify(copied),/build provenance changed/);outcomes.push({name:'build-provenance',accepted:false});
});
test.after(()=>{
  fs.writeFileSync(path.join(destination,'results.json'),JSON.stringify({verifierSha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(root,'scripts/Compare-TimeSignatureConnection.mjs'))).digest('hex'),outcomes},null,2)+'\n');
  console.log('Connection checks: '+destination);
});
