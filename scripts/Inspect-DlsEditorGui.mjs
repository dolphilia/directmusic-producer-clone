import fs from 'node:fs';import path from 'node:path';import crypto from 'node:crypto';
const dir=path.resolve(process.argv[2]??'');if(!process.argv[2])throw Error('Supply GUI evidence directory');
const bytes=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),read=p=>JSON.parse(bytes(p).toString().replace(/^\uFEFF/,''));
const state=read(path.join(dir,'states.json')),run=read(state.run),build=read(state.build);
if(!build.passed||run.cases.some(c=>!c.passed)||run.buildSummarySha256!==hash(bytes(state.build)))throw Error('Build/run identity');
for(const s of build.sources)if(hash(bytes(s.path))!==s.sha256||hash(bytes(path.join(build.sourceRoot,s.path)))!==s.sha256)throw Error('Source drift '+s.path);
const base=path.join(path.dirname(state.run),'core/dls-editor'),proof=read(path.join(base,'editor-proof.json'));
const before=bytes(path.join(dir,'before.dls')),edited=bytes(path.join(dir,'edited.dls'));
if(hash(before)!==run.referenceCollection.sha256||hash(edited)!==proof.editedSha256)throw Error('Independently audited field/PCM expectation');
// The native auditor already derives all fields and every PCM sample from the
// observed original. Undoing only the final Wave operation leaves locale/Region.
const undoExpected=Buffer.from(before);undoExpected.writeUInt32LE(2,100);undoExpected.writeUInt32LE(7,104);undoExpected.writeUInt16LE(60,140);undoExpected.writeUInt16LE(12,144);undoExpected.writeUInt16LE(3,150);
if(!bytes(path.join(dir,'undo.dls')).equals(undoExpected))throw Error('Wave Undo changed other data');
for(const name of ['redo.dls','resaved.dls','owned.dls'])if(!bytes(path.join(dir,name)).equals(edited))throw Error('Full-byte Redo/restart resave '+name);
const captures=state.records.map(r=>({...r,imageSha256:hash(bytes(path.join(dir,r.image)))}));
if(new Set(captures.map(c=>c.imageSha256)).size!==captures.length)throw Error('Stale capture');
if(!captures.every(c=>c.window.app.includes(path.dirname(state.build).replaceAll('/','\\'))))throw Error('GUI executable path');
if(state.mainWindows.length!==2||state.mainWindows[0]===state.mainWindows[1])throw Error('Restart main-window identity');
const reloaded=captures.find(c=>c.step==='reloaded');for(const text of ['Value: 2 ID: 11','Value: 7 ID: 12','Value: 60 ID: 21','Value: 12 ID: 23','Value: 3 ID: 25'])if(!reloaded?.tree.includes(text))throw Error('Reloaded field '+text);
for(const name of ['first-closed-windows.json','closed-windows.json'])if(read(path.join(dir,name)).some(w=>w.app===captures[0].window.app))throw Error('GUI still open '+name);
const result={schema:1,build:state.build,buildSha256:hash(bytes(state.build)),run:state.run,runSha256:hash(bytes(state.run)),exeSha256:run.cases.find(c=>c.name==='host-smoke').sha256,sourceCount:build.sources.length,workspaceSourcesMatched:true,originalSha256:hash(before),editedSha256:hash(edited),undoSha256:hash(undoExpected),instrumentBank2Program7:true,regionKeyLow60VelocityLow12Group3:true,allPcmSamplesHalf:true,loopsPoolAndUnknownChunksUnchanged:true,guiWaveUndoRedoSavedAllBytes:true,restartFieldsAndLosslessResave:true,documentSwitchReopensOwnedDls:true,captures,mainWindows:state.mainWindows,closedWindowsObserved:true,guiExitCodes:'unverified',guiModuleInventory:'unverified',editedDlsPlayback:'unexecuted',fullAcceptance:false,auditorSha256:hash(bytes(new URL(import.meta.url)))};
fs.writeFileSync(path.join(dir,'unit-audit.json'),JSON.stringify(result,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),path.join(dir,'editor-gui-auditor.mjs'));console.log(JSON.stringify({passed:true,captures:captures.length,proof:path.join(dir,'unit-audit.json')}));
