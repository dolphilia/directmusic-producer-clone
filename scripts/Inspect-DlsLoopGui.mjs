import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const dir=path.resolve(process.argv[2]), repo=path.resolve(import.meta.dirname,'..');
const bytes=p=>fs.readFileSync(p), json=p=>JSON.parse(bytes(p).toString('utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(bytes(p)).digest('hex');
const state=json(path.join(dir,'states.json')),run=json(state.run),build=json(state.build);
const host=run.cases.find(c=>c.name==='host-smoke');
assert(build.passed && run.cases.every(c=>c.passed));
assert.equal(hash(state.executable),host.sha256);
assert(build.outputs.some(o=>path.resolve(path.dirname(state.build),o.path)===path.resolve(state.executable)&&o.sha256===host.sha256));
for(const s of build.sources){assert.equal(hash(path.join(build.sourceRoot,s.path)),s.sha256);assert.equal(hash(path.join(repo,s.path)),s.sha256);}
assert.equal(state.firstWindow.app,'process:'+state.executable);
assert.equal(state.restartWindow.app,state.firstWindow.app);
assert.notEqual(state.firstWindow.id,state.restartWindow.id);
assert.equal(state.firstClosed,true);
assert(!json(path.join(dir,'first-close-windows.json')).some(w=>w.app===state.firstWindow.app));
for(const r of state.records)assert.equal(hash(path.join(dir,r.screenshot)),r.sha256);
const reload=state.records.find(r=>r.name==='30-reloaded-values');
for(const [id,value] of [[72,1],[73,10],[74,1000],[82,0],[83,200],[84,500]])assert(reload.tree.split('\n').some(l=>l.includes('編集 ')&&l.includes('Value: '+value+' ID: '+id)));
const core=path.join(path.dirname(state.run),'core/dls-editor/loops');
const proof=json(path.join(core,'loop-proof.json'));assert.equal(proof.passed,true);
assert.equal(json(path.join(dir,'gui-module-provenance.json')).passed,true);
const comparisons=[];
for(const [gui,native] of [['saved-before-restart.dls','region-edited.dls'],['undo-saved.dls','wave-edited.dls'],['redo-saved.dls','region-edited.dls'],['resaved-after-restart.dls','region-edited.dls'],['source.dls','region-edited.dls']]){
  assert(bytes(path.join(dir,gui)).equals(bytes(path.join(core,native))),gui+' whole bytes');
  comparisons.push({gui,native:path.join(core,native),sha256:hash(path.join(dir,gui)),wholeBytesEqual:true});
}
const result={schema:1,passed:true,createdUtc:new Date().toISOString(),build:state.build,run:state.run,exeSha256:host.sha256,coreChecks:556,sourceCount:build.sources.length,statesSha256:hash(path.join(dir,'states.json')),auditorSha256:hash(import.meta.filename),nativeProof:{path:path.join(core,'loop-proof.json'),sha256:hash(path.join(core,'loop-proof.json'))},comparisons,guiModuleCount:json(path.join(dir,'gui-module-provenance.json')).moduleCount,scope:'Existing Wave loop 0 and explicit Region loop 0 GUI edits, Save, Undo/Save, Redo/Save, normal first exit, separate process reload values and resave whole bytes. Native independently audited fixture has extended WSMP and another Wave loop. GUI accessibility can lag one action; screenshots retained and manually inspected, not machine OCR.',remaining:['GUI loop append/enable/disable and whole-sample inheritance unexecuted','GUI project lifecycle unexecuted in this case','Current edited-loop runtime/API/audio unexecuted','Original dynamic comparison, Producer/SMPL synchronization, per-field override/articulation/group and all40/all8 gates incomplete'],fullAcceptance:false};
fs.copyFileSync(import.meta.filename,path.join(dir,'audit-dls-loop-gui.mjs'));
fs.writeFileSync(path.join(dir,'loop-gui-proof.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({passed:true,proof:path.join(dir,'loop-gui-proof.json'),comparisons:comparisons.length}));
