# Segment SequenceノートCRUD

全体は未完了。SDK固定ヘッダー `work/analysis/sources/dmusicf.h` の `DMUS_IO_SEQ_ITEM` を形式の根拠とする。Producer固有の編集動作との動的比較は未実行。

`SegmentDocument::edit_note/delete_note` は、選択group-maskとSequence型内indexのトラックを編集する。ノートindexはstatus上位nibbleが0x90かつvelocity非ゼロのレコードだけを数える。controller、note-off、velocity0 note-onは削除対象に数えない。変更はtime/duration/PChannel/pitch/velocityを扱う。現在のPChannel編集は既存追加と同じ0..15の限定範囲であり、全DWORD PChannel対応は未完了。

表示timeはraw mtTime+signed nOffset。変更時はnOffsetを保持し、64bit計算で新raw timeを得る。raw timeが変わったレコードだけを既存追加と同じraw-clock順へ挿入し、他レコード間の順は保持する。timeが同じ変更は位置も保持する。status下位nibble・reserved/拡張bytes・curve・未知subchunks・padding・trkh・兄弟track・root metadataを保持する。raw time overflow、範囲外値、曖昧/不正payloadはcommit前に拒否し、文書bytes/cache/dirty/historyを変更しない。無変更・範囲外indexはfalse。削除は対象一recordとevtlサイズだけを変える。最後のnoteを消してもcontroller/note-offは残る。

現行 `work/build/product-snapshot/20261003T043148553Z/build-summary.json` は保存50sources/3targets、構成・compile・install各exit0、warning/errorなし。Producer SHA `e747d3d80ef3d20b3dd8680a6de8f04af3159b0fe5fcc0ac350a09cb65f417f2`、core SHA `a54357cb33320fe75932b8b7b113d2dabaeff7b425584e7b24a3cf339aa9993b`。targeted `work/acceptance/sequence-crud/20261003T043309507Z/run.json` は23検査exit0。標準20byte fixtureと、offset/status/controller/unknown/padding/24byte stride付き保存fixtureを独立監査。別Framework project reload、全bytes UndoRedo、失敗原子性を確認。旧版670全suiteを現行成功へ流用しない。

標準CRUD生成 `playable.sgp` は7note、first0clocks/duration192/pitch65/velocity88、その後768刻み、last note削除。runtime `work/acceptance/product-group/20261003T043324805Z/run.json` は9group/6Tempo、全bytes snapshot、PlayStop。56moduleの原版40hash一致0。拡張synthetic `saved.sgp` はWindows loaderの非S_OK `0x08781091` を受け、現行ConductorはDownload/Play前に拒否。拒否試験 `043340697Z` のdriverはexit1/failedで保持し、独立 `sequence-crud-playback-proof.json` は期待した拒否と標準再生を区別して監査する。拡張入力の再生対応は未完了であり、全bytes保存成功を再生互換と呼ばない。

途中失敗：restricted構成042216304ZはSDK探索先アクセス拒否。通常環境の042242278Z構成/build/installは成功したが、restricted実行042355648Zは15s以内に終了せず0byte stdout/locked-file記録失敗。runを補足保存、driverはtimeout時のstdout hashを未確認nullとするよう修正。通常環境042550087Zはアクセス違反exit0xC0000005。診断付き042707246Z/run042817715Zで17検査の後、試験fixtureがvector増加前の参照を再利用した箇所を特定。参照再取得後042847060Z/run043019729Z21件成功。この版の拡張入力runtime043037256Zでは部分ロード後のDownload/PlayとGetTrack失敗を観測し、現行の部分ロード拒否につながった。最終結合auditor初稿はHRESULTのhex転記を誤りassertion失敗、raw decimal142086289→0x08781091へ訂正。失敗記録は保持する。

再現は通常の許可されたWindows環境で次を実行する。

```powershell
./scripts/Build-ProductSnapshot.ps1
./scripts/Test-SequenceCrud.ps1 -BuildSummaryPath <build-summary.json>
node scripts/Inspect-SequenceCrud.mjs <sequence-crud/run.json>
./scripts/Test-GroupRuntime.ps1 -BuildSummaryPath <summary> -Segment <generated>/playable.sgp
node scripts/Inspect-GroupPlayback.mjs <runtime/run.json>
./scripts/Inspect-ProductModules.ps1 -RunPath <runtime/run.json> -CaseName group-playback-api
# Expected rejection: this driver exits1; inspect rather than rerun unchanged.
./scripts/Test-GroupRuntime.ps1 -BuildSummaryPath <summary> -Segment <generated>/saved.sgp
node scripts/Inspect-SequenceCrudPlayback.mjs <native-proof> <runtime-proof> <partial-run.json>
```

次はmainの選択note一覧・フィールド編集/削除へ公開し、group/文書切替のcontext・保存・通常閉鎖・別process再読込を同版で検証する。拡張Sequence runtime互換、offset排序・PChannel/curve/原版比較、現在の音声、他40責務/全八受入は継続する。原版PID5400は窓非公開と停止拒否の既存障害として記録し、同条件の再起動・停止再試行は行わない。


## 2026-10-03 Sequence CRUD本体UI・検証継続（044136915Z）

mainへ選択note一覧、専用clock/channelとChange Note/Delete Sequence Note確認を追加。移動したraw recordの結果note indexをAPIで返し、値一致による推測を使わずGUIの選択を追従。選択は文書index/group-mask/Sequence型内indexをkeyとして保持し、新規project/成功project openで消去する。失敗/無変更時は出力indexを変更しない。

保存50sources/3targets `work/build/product-snapshot/20261003T044136915Z/build-summary.json` 構成・compile・install各exit0。Producer SHA 8c5ba80d4b9d8952fc096cd9ebea6bec560f2f94cffe62d519380e873a0a3683、core SHA 045cfb7651483a7904e33db7300b4bebf8e3edf01ff320edb72a2b26893e8d3d。`sequence-crud/20261003T044302166Z/run.json` 26検査成功。新しいhost-only driver `Test-ProductHost.ps1`/`product-host/20261003T044300327Z/run.json` は--smoke exit0、現行full suiteは実行していない。

GUI `product-ui/20261003T044500Z/states.json` の34 capturesではnative同版playable.sgpをowned.sgpにコピーして読込、note1を5000clocks/duration288/pitch72/velocity100へ変更し、Note7への選択追従を確認。別名edited.sgpの全bytesは独立auditor `Inspect-SequenceCrudGui.mjs` が期待する1recordのpatch+moveのみ。group2の空状態は変更/削除無効、group1へ戻るとNote7と全field復元。削除確認のNoでNotes7と選択/clean表示保持（取消後別名保存全bytesはまだ未試験）。進捗proof `sequence-crud-gui-progress-proof.json` はこの限定範囲だけ合格。

同版GUI保存edited.sgpを `product-group/20261003T062720774Z/run.json` で再生、9group/6Tempo、wholebytes snapshot、Play/Stop exit0。55module原版40hash一致0。これはruntime時点のinventoryでGUIは未取得。現行音声は未確認、旧聴取回答を転用しない。拡張synthetic非S_OK拒否は前版043148553Zの証拠であり現行で未再実行。

GUI途中でstale filename indexがcached stateで使用不可、File menu indexの座標がwindow bounds外となったため各々再観測して画面座標/Alt+nへ変更。file dialogのfocused_elementが検索boxと報告される一方、スクリーンのfilename caret/入力された絶対パスを確認。Apply直後accessibilityが旧値でスクリーンだけ先に更新される遅延を観測し、追加取得したsettled記録を判定に使用。API副作用や製品不具合と混同しない。auditor初稿はmodule proof filenameを誤りENOENT、group-playback-module-provenance.jsonへ修正後pass。module inspector初回はRunJsonという存在しない引数で起動前失敗、RunPathへ修正後pass。

ユーザーが旧core crash error窓のOKを閉じたと回答、現行GUIを再観測し遮蔽解消を確認。原版PID5400の停止拒否/窓非公開は同条件で再試行していない。

現行メイン224135258のDelete Sequence Note確認を表示している。対象は保存済み作業コピーedited.sgp、Note7/5000/pitch72のみ。Computer Use confirmationsの「Always Confirm at Action-Time」「Delete data」に従い、Yes前の確認を質問済み。未回答は承認扱いしない。削除確定/UndoRedo GUI/文書切替/保存正常終了/別process project復元は未完了、次は回答後fresh modalを再観測してYes/Undo/保存比較。その後現行GUI由来と保存入力再生・音声へつなぐ。全40責務/全八受入は未達で目標activeを維持。

追記：同版GUI確認modal表示中の一意processをbase address付きでread-only capture。gui-modules.json/gui-module-provenance.jsonは109module・原版40hash一致0。独立GUI進捗auditorもGUI/runtime両inventoryのoriginalHashMatches空を検査、最終proofへGUI109/hashを結合した。modalは削除確定待ち、未実行結果は保持。


## 2026-10-04 AudioPath範囲編集／Sequence PChannel

現行20261004T092316964Z保存71sources構成/build/install0。AudioPath port/route PChannel範囲と既存buffer接続編集・UndoRedo/独立Segment copy/native保存復元/runtime全bytes保持を実装。Sequenceの誤った0～15制限をDWORD PChannelへ修正、予約broadcast拒否、UI接続。専用20＋文書20＋Sequence26/raw/host passed。編集APFarm local22の明示AudioPath実再生、元Source不在WASAPI440Hz/Slow Attack/Stop/再開/停止無音、派生PCM3拒否passed。再生56/host24modules原版40hash0。旧save Windows error5未解決、現行全core/GUI/原版動的比較/全40/全八未完了。 証拠：work/analysis/audiopath-range/20261004T091800Z/report.md。旧Sequence0～15制限はMIDIチャンネルとの混同であり、現行DWORD PChannelとreserved broadcast拒否へ訂正。
