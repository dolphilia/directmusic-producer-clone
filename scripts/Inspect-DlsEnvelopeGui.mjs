import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]),native=path.resolve(process.argv[3]);
const read=p=>fs.readFileSync(p),j=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),h=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const state=j(dir+'/states.json'),build=j(state.build),run=j(native+'/run.json'),modules=j(dir+'/gui-module-provenance.json');
assert(build.passed&&run.passed&&modules.passed);assert.equal(h(state.build),run.buildSummarySha256);
const exe=build.outputs.find(x=>x.path==='install/bin/Producer.exe');assert.equal(h(state.executable),exe.sha256);
for(const s of build.sources){assert.equal(h(s.path),s.sha256);assert.equal(h(path.join(build.sourceRoot,s.path)),s.sha256);}
function parse(b,at=0,end=b.length){const id=b.toString('ascii',at,at+4),size=b.readUInt32LE(at+4),finish=at+8+size;assert(finish<=end);const c={id,at,size,data:b.subarray(at+8,finish),raw:b.subarray(at,finish+(size&1))};if(['RIFF','LIST'].includes(id)){c.type=b.toString('ascii',at+8,at+12);c.children=[];for(let pos=at+12;pos<finish;){const child=parse(b,pos,finish);c.children.push(child);pos+=child.raw.length;}assert.equal(c.children.reduce((n,x)=>n+x.raw.length,0),size-4);}return c;}
const child=(c,id,type)=>{const a=c.children.filter(x=>x.id===id&&(!type||x.type===type));assert.equal(a.length,1);return a[0];};
const before=read(native+'/core/Articulation/EnvelopeBefore.dls'),edited=read(dir+'/Edited.dls'),root=parse(before),ins=child(root,'LIST','lins').children[0],region=child(ins,'LIST','lrgn').children[0],list=child(region,'LIST','lar2'),art=child(list,'art2');
assert.equal(art.data.readUInt32LE(0),8);assert.equal(art.data.readUInt32LE(4),3);assert.equal(art.data.length,44);
const end=art.at+8+art.size,record=Buffer.alloc(12);record.writeUInt16LE(0x207,4);record.writeInt32LE(-78659584,8);
const expected=Buffer.concat([before.subarray(0,end),record,before.subarray(end)]);
for(const c of [root,child(root,'LIST','lins'),ins,child(ins,'LIST','lrgn'),region,list,art])expected.writeUInt32LE(c.size+12,c.at+4);
expected.writeUInt32LE(4,art.at+12);assert(expected.equals(edited));assert(read(dir+'/Undone.dls').equals(before));assert(read(dir+'/Redone.dls').equals(edited));assert(read(dir+'/Collection.dls').equals(edited));
for(const name of ['03-articulation','05-decay-selected','06-decay-entered','08-edited-saved','10-undone-saved','12-redone-saved']){const s=j(dir+'/'+name+'.json');assert(s.window.app.toLowerCase().includes('052814478z'));assert.equal(s.window.id,7737338);}
assert(j(dir+'/05-decay-selected.json').accessibility.tree.includes('No constant value in this block'));
assert(j(dir+'/06-decay-entered.json').accessibility.tree.includes('Value: -1200.25'));
const proof={schema:1,createdUtc:new Date().toISOString(),passed:true,build:state.build,buildSummarySha256:h(state.build),exeSha256:exe.sha256,processId:state.processId,scope:'Current GUI Region EG1 Decay named explicit add, timecents -1200.25, exact whole-file save/UndoSave/RedoSave; unchanged Instrument and existing modulation/opaque Region records. No original dynamic equivalence, audio or full acceptance.',inputs:[{path:native+'/core/Articulation/EnvelopeBefore.dls',sha256:h(native+'/core/Articulation/EnvelopeBefore.dls')}],outputs:['Edited.dls','Undone.dls','Redone.dls','Collection.dls'].map(n=>({path:dir+'/'+n,sha256:h(dir+'/'+n)})),fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/envelope-gui-auditor.mjs');fs.writeFileSync(dir+'/envelope-gui-proof.json',JSON.stringify(proof,null,2)+'\n');console.log(JSON.stringify({passed:true,processId:state.processId,fullAcceptancePassed:false}));
