import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]);
const read=p=>fs.readFileSync(p),j=p=>JSON.parse(read(p).toString().replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(read(p)).digest('hex');
const state=j(dir+'/states.json'),b=j(state.build),exit=j(dir+'/exit.json'),launch=j(dir+'/reload-launch.json'),modules=j(dir+'/gui-module-provenance.json');
assert(b.passed&&modules.passed);assert.equal(modules.exeSha256,hash(state.executable));
for(const s of b.sources){assert.equal(hash(s.path),s.sha256);assert.equal(hash(path.join(b.sourceRoot,s.path)),s.sha256);}
assert.equal(exit.state,'exited');assert.equal(exit.exitCode,0);assert.equal(exit.forcedTermination,false);assert.notEqual(exit.processId,launch.processId);assert.equal(exit.exeSha256,launch.exeSha256);assert.equal(launch.exeSha256,hash(state.executable));assert(Date.parse(exit.exitUtc)<Date.parse(launch.startUtc));
function parse(buf,at=0,end=buf.length){const id=buf.toString('ascii',at,at+4),size=buf.readUInt32LE(at+4),finish=at+8+size;assert(finish<=end);const c={id,at,size,data:buf.subarray(at+8,finish),raw:buf.subarray(at,finish+(size&1))};if(['RIFF','LIST'].includes(id)){c.type=buf.toString('ascii',at+8,at+12);c.children=[];let pos=at+12;while(pos<finish){const x=parse(buf,pos,finish);c.children.push(x);pos+=x.raw.length;}assert.equal(pos,finish);}return c;}
const child=(c,id,type)=>{const a=c.children.filter(x=>x.id===id&&(!type||x.type===type));assert.equal(a.length,1);return a[0];};
const before=read(dir+'/Undone.dls'),edited=read(dir+'/Edited.dls'),root=parse(before),ins=child(root,'LIST','lins').children[0],reg=child(ins,'LIST','lrgn').children[0],art=child(child(reg,'LIST','lar2'),'art2');
assert.equal(art.data.readUInt32LE(0),8);assert.equal(art.data.readUInt32LE(4),4);assert.equal(art.data.length,56);
const constants=[];for(let n=0;n<4;n++){const p=8+n*12;constants.push([0,2,4,6].map(o=>art.data.readUInt16LE(p+o)).concat(art.data.readInt32LE(p+8)));}
assert.deepEqual(constants,[[2,0,518,0,99],[0,0,65535,0,-7],[0,0,518,0,-65536],[0,0,519,0,-78659584]]);
const offset=art.at+8+8+3*12+8,expected=Buffer.from(before);expected.writeInt32LE(-157319168,offset);assert(expected.equals(edited));
for(const n of ['Redone.dls','Collection.dls','Resaved.dls'])assert(read(dir+'/'+n).equals(edited));
assert(before.equals(read('work/analysis/dls-envelope/20261004T052200Z/Edited.dls')));
for(const [name,value,window] of [['06-decay-restored','-1200.25',2167838],['09-edited-saved','-2400.5',2167838],['11-undone-saved','-1200.25',2167838],['13-redone-saved','-2400.5',2167838],['21-decay-reloaded','-2400.5',4658224],['22-reloaded-resaved','-2400.5',4658224]]){
 const s=j(dir+'/'+name+'.json'),tree=s.accessibility.tree;assert.equal(s.window.id,window);assert(s.window.app.toLowerCase().includes('054148406z'));assert(tree.includes('Value: Region 1 override'));assert(tree.includes('Value: Connection 4 — destination 519'));assert(tree.includes('Value: EG1 Volume — Decay'));assert(tree.includes('Scale (timecents) Value: '+value));for(const button of ['Apply Connection','Add Connection','Remove Connection'])assert(tree.includes('ボタン (disabled) '+button));
 if(window===4658224)assert(Date.parse(s.time)>Date.parse(launch.startUtc));
}
const project=read(dir+'/Envelope.dmpj');assert.equal(project.toString('ascii',0,4),'RIFF');assert.equal(project.toString('ascii',8,12),'DMPJ');assert.equal(hash(dir+'/Envelope.dmpj'),launch.inputProjectSha256);
const result={schema:1,createdUtc:new Date().toISOString(),passed:true,build:state.build,buildSummarySha256:hash(state.build),exeSha256:hash(state.executable),sourceCount:b.sources.length,exitProcessId:exit.processId,reloadProcessId:launch.processId,scaleOffset:offset,outputs:['Edited.dls','Undone.dls','Redone.dls','Collection.dls','Resaved.dls','Envelope.dmpj','Initial.sgp'].map(n=>({path:dir+'/'+n,sha256:hash(dir+'/'+n)})),scope:'Named Region constant selection, edit/history/exact saves, normal exit0 and separate same-build DMPJ GUI reload/resave; native JAZP, audio and full acceptance unexecuted',fullAcceptancePassed:false};
fs.copyFileSync(new URL(import.meta.url),dir+'/selection-gui-auditor.mjs');fs.writeFileSync(dir+'/selection-gui-proof.json',JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify({passed:true,scaleOffset:offset,fullAcceptancePassed:false}));
