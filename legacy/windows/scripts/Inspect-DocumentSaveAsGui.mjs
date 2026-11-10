import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const bytes=p=>fs.readFileSync(p),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>JSON.parse(bytes(p).toString().replace(/^\uFEFF/,''));
if(!process.argv[2])throw Error('Supply GUI evidence directory');
const dir=path.resolve(process.argv[2]),state=read(path.join(dir,'states.json')),run=read(state.run),build=read(state.build);
if(!build.passed||run.cases.some(c=>!c.passed)||hash(bytes(state.build))!==run.buildSummarySha256)throw Error('Build/run identity');
for(const s of build.sources)if(hash(bytes(s.path))!==s.sha256||hash(bytes(path.join(build.sourceRoot,s.path)))!==s.sha256)throw Error('Source drift');
const host=run.cases.find(c=>c.name==='host-smoke');
if(path.resolve(host.executable)!==path.resolve(state.executable)||hash(bytes(state.executable))!==host.sha256)throw Error('EXE identity');
if(!state.firstClosed||!state.finalClosed||state.firstWindow.id===state.restartWindow.id||state.firstWindow.app!==state.restartWindow.app)throw Error('Restart lifecycle');
const modules=read(path.join(dir,'gui-module-provenance.json'));
if(!modules.passed||modules.exeSha256!==host.sha256)throw Error('GUI dependency identity');
for(const r of state.records)if(hash(bytes(path.join(dir,r.screenshot)))!==r.sha256||r.window.app!==state.firstWindow.app)throw Error('Capture identity');
const tree=name=>{const r=state.records.find(r=>r.name===name);if(!r)throw Error('Missing capture '+name);return r.tree;};
for(const [name,expected] of [['style-before','Style tempo: 120.000000'],['style-edited','Style tempo: 137.000000'],['style-saveas-dialog','Value: Style ID: FileTypeControlHost'],['dependent-dirty','song.sgp *'],['segment-saved','moved.sgp'],['band-saved','renamed.bnp'],['restart-loaded','768 clocks 137 BPM'],['style-resaved','Style: renamed.stp'],['band-resaved-settled','PChannel 0 patch 256']])if(!tree(name).includes(expected))throw Error('Capture value '+name+' '+expected);
function parse(b,a=0,z=b.length){const out=[];for(let at=a;at<z;){if(at+8>z)throw Error('RIFF header');const id=b.toString('ascii',at,at+4),n=b.readUInt32LE(at+4),stop=at+8+n,next=stop+(n&1),container=id==='RIFF'||id==='LIST';if(next>z||container&&n<4)throw Error('RIFF bounds');out.push({id,type:container?b.toString('ascii',at+8,at+12):'',data:container?null:Buffer.from(b.subarray(at+8,stop)),children:container?parse(b,at+12,stop):[],padding:n&1?b[stop]:0});at=next;}return out;}
function encode(c){const d=c.data??Buffer.concat([Buffer.from(c.type),...c.children.map(encode)]),h=Buffer.alloc(8);h.write(c.id);h.writeUInt32LE(d.length,4);return Buffer.concat([h,d,...(d.length&1?[Buffer.from([c.padding])]:[])]);}
const root=b=>{const r=parse(b);if(r.length!==1||!encode(r[0]).equals(b))throw Error('Lossless RIFF');return r[0];};
const text=c=>c.data.toString('utf16le').replace(/\0$/,'');
function replaceFiles(c,from,to){let count=0;for(const x of c.children){if(x.id==='file'&&text(x)===from){x.data=Buffer.from(to+'\0','utf16le');count++;}count+=replaceFiles(x,from,to);}return count;}
const native=path.join(path.dirname(state.run),'core/style-save-as'),files={};
const equal=(a,b,label)=>{if(!a.equals(b))throw Error(label);};
for(const [gui,input] of [['style.stp','before-style.stp'],['song.sgp','edited-song.sgp'],['guid-song.sgp','guid-song.sgp'],['band.bnp','band.bnp'],['project.dmpj','before.dmpj'],['assets/sound.dls','assets/sound.dls']])equal(bytes(path.join(dir,gui)),bytes(path.join(native,input)),'Seed modified '+gui);
equal(bytes(path.join(dir,'assets/renamed.stp')),bytes(path.join(native,'assets/renamed.stp')),'Style complete edited bytes');
const song=root(bytes(path.join(dir,'song.sgp')));if(replaceFiles(song,'style.stp','renamed.stp')!==2)throw Error('Two unsorted Style references');equal(encode(song),bytes(path.join(dir,'assets/moved.sgp')),'Segment filename-only change');
const band=root(bytes(path.join(dir,'band.bnp')));if(replaceFiles(band,'assets\\sound.dls','sound.dls')!==1)throw Error('Band DLS reference');equal(encode(band),bytes(path.join(dir,'assets/renamed.bnp')),'Band filename-only change');
const project=root(bytes(path.join(dir,'project.dmpj')));for(const [from,to] of [['song.sgp','assets\\moved.sgp'],['style.stp','assets\\renamed.stp'],['band.bnp','assets\\renamed.bnp']])if(replaceFiles(project,from,to)!==1)throw Error('Project reference '+from);equal(encode(project),bytes(path.join(dir,'moved-project.dmpj')),'Project active paths only');
for(const name of ['renamed.stp','moved.sgp','renamed.bnp'])equal(bytes(path.join(dir,'saved-'+name)),bytes(path.join(dir,'assets',name)),'Restart/resave all bytes '+name);
equal(bytes(path.join(dir,'saved-moved-project.dmpj')),bytes(path.join(dir,'moved-project.dmpj')),'Project retained');
for(const f of ['style.stp','song.sgp','band.bnp','guid-song.sgp','project.dmpj','assets/renamed.stp','assets/moved.sgp','assets/renamed.bnp','moved-project.dmpj','assets/sound.dls','states.json','gui-module-provenance.json']){const b=bytes(path.join(dir,f));files[f]={sha256:hash(b),bytes:b.length};}
const proof={schema:1,passed:true,build:state.build,buildSha256:hash(bytes(state.build)),run:state.run,runSha256:hash(bytes(state.run)),exeSha256:host.sha256,sourceCount:build.sources.length,captureCount:state.records.length,moduleCount:modules.moduleCount,files,styleEditedAndSavedAs:true,segmentAndBandSavedAs:true,dependentActivePathsOnly:true,seedFilesPreserved:true,normalExitRestartThreeDocumentResavesAllBytes:true,guiUndoRedo:'unexecuted',guiPlay:state.guiPlay,audio:state.audio,originalDynamicComparison:'unexecuted',fullAcceptance:false,limitations:state.limitations};
fs.writeFileSync(path.join(dir,'document-save-as-proof.json'),JSON.stringify(proof,null,2)+'\n');fs.copyFileSync(new URL(import.meta.url),path.join(dir,'document-save-as-auditor.mjs'));console.log(JSON.stringify({passed:true,sourceCount:proof.sourceCount,captureCount:proof.captureCount,moduleCount:proof.moduleCount}));
