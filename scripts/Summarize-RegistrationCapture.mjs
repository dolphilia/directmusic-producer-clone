import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.join(root,'work/integration/registration');
const read=file=>JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const latest=new Map();
const attempts=[];
for(const name of fs.readdirSync(directory).sort()){
  const run=path.join(directory,name),metadataFile=path.join(run,'run.json');
  if(!fs.existsSync(metadataFile))continue;
  const metadata=read(metadataFile),module=path.basename(metadata.module);
  const file=path.join(run,'capture.jsonl');
  const logs=fs.existsSync(file)?fs.readFileSync(file,'utf8').trim().split(/\r?\n/).filter(Boolean).map(JSON.parse):[];
  const end=logs.find(r=>r.operation==='end_registration_capture');
  attempts.push({module,run:path.relative(root,run).replaceAll('\\','/'),exitCode:metadata.exitCode,launchError:metadata.launchError,timedOut:metadata.timedOut,
    hresult:logs.find(r=>r.operation==='private_registration')?.hresult??null,captureFinished:!!end,probeSha256:metadata.probeSha256});
  if(metadata.exitCode===0&&(!end?.registered||!end.exported||!end.restored))throw Error('Successful capture lacks completion: '+name);
  const values=logs.filter(r=>r.operation==='registry_value').map(v=>{
    const raw=Buffer.from(v.hex,'hex');if(raw.length!==v.bytes)throw Error('Registry bytes mismatch');
    const decoded=v.type===1||v.type===2||v.type===7?raw.toString('utf16le').replace(/\0+$/,''):v.type===4&&raw.length===4?raw.readUInt32LE(0):null;
    return {...v,decoded};
  });
  for(const source of metadata.sources)if(hash(path.join(run,'sources',source.path))!==source.sha256)throw Error('Saved source changed: '+name);
  if(hash(metadata.module)!==metadata.sha256||!metadata.moduleUnchanged||!metadata.globalPresenceUnchanged)throw Error('Original or global presence changed: '+name);
  const keys=logs.filter(r=>r.operation==='registry_key').map(r=>r.path);
  const components=keys.filter(k=>/^\\HKLM\\Software\\Microsoft\\DMUSProducer\\Components\\\{[^}]+\}$/i.test(k)).map(key=>{
    const clsid=key.match(/\{[^}]+\}$/)[0];
    const server=values.find(v=>v.path.toUpperCase()===(`\\HKCR\\CLSID\\${clsid}\\InProcServer32`).toUpperCase()&&v.name==='');
    return {clsid,name:values.find(v=>v.path===key&&v.name==='')?.decoded,skip:values.find(v=>v.path===key&&v.name==='Skip')?.decoded,server:server?.decoded};
  });
  const classes=keys.filter(k=>/^\\HKCR\\CLSID\\\{[^}]+\}$/i.test(k)).map(key=>({clsid:key.match(/\{[^}]+\}$/)[0],name:values.find(v=>v.path===key&&v.name==='')?.decoded,
    server:values.find(v=>v.path.toUpperCase()===(key+'\\InProcServer32').toUpperCase()&&v.name==='')?.decoded}));
  if(latest.has(module)&&!end&&latest.get(module).captureFinished)continue;
  latest.set(module,{module,sha256:metadata.sha256,run:path.relative(root,run).replaceAll('\\','/'),exitCode:metadata.exitCode,captureFinished:!!end,
    hresult:logs.find(r=>r.operation==='private_registration')?.hresult??null,probeSha256:metadata.probeSha256,
    completed:metadata.exitCode===0&&!metadata.launchError&&!metadata.timedOut,keys:keys.length,values:values.length,components,classes,
    globalPresenceUnchanged:metadata.globalPresenceUnchanged,exportedValues:values});
}
const modules=[...latest.values()].sort((a,b)=>a.module.localeCompare(b.module));
const report={schema:1,method:'Original DllRegisterServer in process-private app hive; global COM installation not performed',
  modules:modules.length,successful:modules.filter(m=>m.completed).length,failed:modules.filter(m=>!m.completed).length,
  components:modules.flatMap(m=>m.components.map(c=>({...c,module:m.module,complete:m.completed,run:m.run}))),captures:modules,attempts};
fs.writeFileSync(path.join(root,'docs/analysis/registration-capture.json'),JSON.stringify(report,null,2)+'\n');
const lines=['# 専用ハイブで取得した登録内容','','更新日：2026-10-02。DLL 登録の観測結果であり、Windows へのインストール完了ではない。',
  '',`Producer 固有の登録関数を持つ ${report.modules} モジュールを観測した。${report.successful} 件が S_OK で登録・列挙・参照先復元を完了し、${report.failed} 件は失敗した。各モジュールと保存ソースのハッシュ、結果、キー・値・元バイト列は [registration-capture.json](registration-capture.json) に残す。`,
  '', '観測方法は RegLoadAppKey(REG_PROCESS_APPKEY) と RegOverridePredefKey。HKCR/HKLM/HKCU を試験プロセス内だけで専用 capture.hiv の別サブキーへ対応付け、DLL のロード・登録・アンロード後に復元した。作業フォルダー外への実インストールは行っていない。グローバルの DMUSProducer キーと Segment の2 CLSIDについて、32/64ビット各ビューで起動前後の存在状態が同じことも確認した。全レジストリの完全な同一性を検証したという意味ではない。',
  '', '公式 API の範囲：[RegLoadAppKeyW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regloadappkeyw)、[RegOverridePredefKey](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regoverridepredefkey)。',
  '', '## Components 探索用の登録','','| モジュール | CLSID | 名前 | Skip |','| --- | --- | --- | --- |',
  ...report.components.map(c=>`| ${c.module} | ${c.clsid} | ${c.name} | ${c.skip} |`),
  '', '## モジュールごとの結果','','| モジュール | 戻り値 | キー数 | 値数 | COM クラス数 |','| --- | --- | --- | --- | --- |',
  ...modules.map(m=>`| ${m.module} | ${m.hresult??'起動未完了'} | ${m.keys} | ${m.values} | ${m.classes.length} |`),
  '', '## 再現手順','','```powershell',
  'cmake -S tests/native/registration -B work/build/registration -G "Visual Studio 17 2022" -A Win32',
  'cmake --build work/build/registration --config Release',
  '.\\scripts\\Run-RegistrationCapture.ps1 -Module SegmentDesigner.ocx',
  'node scripts/Summarize-RegistrationCapture.mjs','```',
  '', '各実行は新しいハイブを作り、元モジュールの SHA-256 を既存 PE 記録と一致させる。probe はそのハッシュをロード前に再確認する。3ソースを run に保存する。失敗した登録も保存するが、完全な登録マニフェストには含めない。MFC42、MSVCRT、MSFLXGRD とインストーラーの操作はこの33モジュールに含めていない。',
  '', '本体の起動には取得内容を使う試験環境をさらに用意する必要がある。TypeLib 登録の失敗、フォント、初期化順序、音声ランタイム、本体からの COM 生成と編集画面の表示は未検証。',
  '', 'ADSREnvelope / PanVol / RegionKeyboard の登録は 0x80040200（SELFREG_E_TYPELIB）で失敗した。COM 初期化と LoadTypeLibEx(REGKIND_NONE) による読取診断を追加した新しいプローブはビルドできたが、20261002T073350140Z / 073351148Z / 073351803Z の起動はアプリケーション制御に拒否された。この診断の結果は未取得。JSON の attempts に拒否も保持し、登録処理へ到達した先行する結果と区別する。'];
fs.writeFileSync(path.join(root,'docs/analysis/registration-capture.md'),lines.join('\n')+'\n');
console.log(JSON.stringify({modules:report.modules,successful:report.successful,failed:report.failed,components:report.components.length,classes:modules.reduce((n,m)=>n+m.classes.length,0)}));
