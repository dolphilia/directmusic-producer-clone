# 専用ハイブで取得した登録内容

更新日：2026-10-02。DLL 登録の観測結果であり、Windows へのインストール完了ではない。

Producer 固有の登録関数を持つ 33 モジュールを観測した。30 件が S_OK で登録・列挙・参照先復元を完了し、3 件は失敗した。各モジュールと保存ソースのハッシュ、結果、キー・値・元バイト列は [registration-capture.json](registration-capture.json) に残す。

観測方法は RegLoadAppKey(REG_PROCESS_APPKEY) と RegOverridePredefKey。HKCR/HKLM/HKCU を試験プロセス内だけで専用 capture.hiv の別サブキーへ対応付け、DLL のロード・登録・アンロード後に復元した。作業フォルダー外への実インストールは行っていない。グローバルの DMUSProducer キーと Segment の2 CLSIDについて、32/64ビット各ビューで起動前後の存在状態が同じことも確認した。全レジストリの完全な同一性を検証したという意味ではない。

公式 API の範囲：[RegLoadAppKeyW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regloadappkeyw)、[RegOverridePredefKey](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regoverridepredefkey)。

## Components 探索用の登録

| モジュール | CLSID | 名前 | Skip |
| --- | --- | --- | --- |
| AudioPathDesigner.ocx | {04ADC2AD-7EA5-4260-A45B-75A6EF856E99} | Audiopath Designer | 0 |
| BandEditor.ocx | {44207724-487B-11D0-89AC-00A0C9054129} | Band Editor | 0 |
| ChordMapDesigner.ocx | {6D432E20-B5E2-11D0-9EDC-00AA00A21BA9} | Chordmap Editor | 0 |
| Conductor.dll | {36F6DDE8-46CE-11D0-B9DB-00AA00C08146} | Conductor | 0 |
| ContainerDesigner.ocx | {1CA84B10-7D17-11D3-B472-00105A2796DE} | Container Designer | 0 |
| DLSDesigner.ocx | {7B5F1BE1-96FC-11D0-89AA-00A0C9054129} | DLS Designer | 0 |
| ScriptDesigner.ocx | {BEC19C60-66FD-11D3-B45D-00105A2796DE} | Script Designer | 0 |
| SegmentDesigner.ocx | {DFCE860B-A6FA-11D1-8881-00C04FBF8D15} | Segment Designer | 0 |
| StyleDesigner.ocx | {44207721-487B-11D0-89AC-00A0C9054129} | Style Designer | 0 |
| ToolGraphDesigner.ocx | {EAB971EE-6601-4F70-9434-32CE568AE3F3} | Toolgraph Designer | 0 |

## モジュールごとの結果

| モジュール | 戻り値 | キー数 | 値数 | COM クラス数 |
| --- | --- | --- | --- | --- |
| ADSREnvelope.ocx | 0x80040200 | 4 | 0 | 0 |
| AudioPathDesigner.ocx | 0x00000000 | 16 | 12 | 2 |
| BandEditor.ocx | 0x00000000 | 16 | 12 | 2 |
| BandStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| ChordMapDesigner.ocx | 0x00000000 | 16 | 12 | 2 |
| ChordMapRefStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| ChordMapStripMgr.dll | 0x00000000 | 21 | 14 | 1 |
| ChordStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| CommandStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| Conductor.dll | 0x00000000 | 12 | 5 | 1 |
| ContainerDesigner.ocx | 0x00000000 | 14 | 9 | 1 |
| DLSDesigner.ocx | 0x00000000 | 19 | 19 | 3 |
| FileOutputDMO.dll | 0x00000000 | 20 | 14 | 1 |
| LyricStripMgr.dll | 0x00000000 | 18 | 11 | 1 |
| MarkerStripMgr.dll | 0x00000000 | 18 | 11 | 1 |
| MIDIStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| MuteStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| PanVol.ocx | 0x80040200 | 4 | 0 | 0 |
| ParamStripMgr.dll | 0x00000000 | 18 | 11 | 1 |
| RegionKeyboard.ocx | 0x80040200 | 4 | 0 | 0 |
| ScriptDesigner.ocx | 0x00000000 | 16 | 12 | 2 |
| ScriptStripMgr.dll | 0x00000000 | 18 | 11 | 1 |
| SegmentDesigner.ocx | 0x00000000 | 18 | 15 | 3 |
| SegmentStripMgr.dll | 0x00000000 | 18 | 11 | 1 |
| SequenceStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| SignPostStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| StyleDesigner.ocx | 0x00000000 | 20 | 18 | 4 |
| StyleRefStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| TempoStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| Timeline.dll | 0x00000000 | 21 | 18 | 2 |
| TimeSigStripMgr.dll | 0x00000000 | 18 | 12 | 1 |
| ToolGraphDesigner.ocx | 0x00000000 | 16 | 12 | 2 |
| WaveStripMgr.dll | 0x00000000 | 18 | 11 | 1 |

## 再現手順

```powershell
cmake -S tests/native/registration -B work/build/registration -G "Visual Studio 17 2022" -A Win32
cmake --build work/build/registration --config Release
.\scripts\Run-RegistrationCapture.ps1 -Module SegmentDesigner.ocx
node scripts/Summarize-RegistrationCapture.mjs
```

各実行は新しいハイブを作り、元モジュールの SHA-256 を既存 PE 記録と一致させる。probe はそのハッシュをロード前に再確認する。3ソースを run に保存する。失敗した登録も保存するが、完全な登録マニフェストには含めない。MFC42、MSVCRT、MSFLXGRD とインストーラーの操作はこの33モジュールに含めていない。

本体の起動には取得内容を使う試験環境をさらに用意する必要がある。TypeLib 登録の失敗、フォント、初期化順序、音声ランタイム、本体からの COM 生成と編集画面の表示は未検証。

ADSREnvelope / PanVol / RegionKeyboard の登録は 0x80040200（SELFREG_E_TYPELIB）で失敗した。COM 初期化と LoadTypeLibEx(REGKIND_NONE) による読取診断を追加した新しいプローブはビルドできたが、20261002T073350140Z / 073351148Z / 073351803Z の起動はアプリケーション制御に拒否された。この診断の結果は未取得。JSON の attempts に拒否も保持し、登録処理へ到達した先行する結果と区別する。
