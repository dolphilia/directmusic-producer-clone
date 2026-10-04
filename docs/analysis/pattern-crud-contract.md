# 本体StyleのPattern新規・削除

更新2026-10-03。全40責務・全八受入は未完了。

## 実装と所有

`StyleDocument::new_pattern(name,pchannel)`は通常Patternと新GUIDの空Partを同時作成する。名前1〜255文字/NULなし、現在の16ch再生構成に対応するPChannel0〜15、Pattern/Part各1000の上限を検証。公開保存`dmusicf.h`のDMUS_IO_PATTERN/STYLEPART/PARTREFに基づきptnh16/prth160/prfc28を作り、Style拍子・1小節・Groove0〜100、VariationChoices全32DWORD=0xffffffff、fixed playmode、空24byte-stride note配列を用意する。PartをPatternの前へ置き、既存チャンクは変更しない。原版Producerの新規操作による全デフォルト値の動的比較は未実行であり、未知のProducer拡張を推測して生成しない。

`delete_pattern(index)`は全PatternのPart参照を事前解決し、対象Patternを削除する。対象が参照していたPartのうち、残存するどのPattern（Motifを含むLIST pttn）にも参照されないものだけを回収する。他Patternと共有するPart、対象と無関係な既存孤立Part、他チャンクは全byte保持する。既存Partの未知内容も、そのPartを最後の所有参照とともに削除する場合はPart単位で消える。壊れた/曖昧な参照は変更前に拒否する。最後のPatternを削除して空Styleにできる。一般的な未知形式の参照まで解決したとは主張しない。

作業コピーで検証してから一度adopt_editするため、新規/削除は一回のUndo/Redoで全byteを戻す。Redoは生成GUIDを再利用する。Frameworkのnew_style_pattern/delete_style_patternはapply_style_editを介して既存の依存SegmentのStyle snapshotを原子的に更新し、Segment保存bytes/dirtyは変更しない。

本体PatternメニューにNew PatternとDelete Patternを追加した。新規は追加行を選択し、既存Part/音符編集UIを使用する。削除は選択とYes/Noダイアログを要求する。メニューの非Style時/未選択時の有効化も接続した。現行版のGUI操作は未実行。

## 版別結果

最終`work/build/product-snapshot/20261003T111240720Z/build-summary.json`：保存54sources/3targets、構成/compile/install各0、warning/error0。EXE SHA `f3c795ae016373a01d3195acd3a543c1d8dabbd603ed2e99f4b62dd6edc9966c`、core SHA `880d07d3fda1ef6c802df29b924e29e94bcdef38ac4a73af45891c6a7881a84f`。

初回110729120ZはGUIコマンド612が既存PatternPropertiesと衝突しcompile失敗、614/615へ修正して新規保存/buildした。中間110849433ZのCRUD111050164Zは23件とraw全byte成功、notes111122594Zも再生成功。ただしoptional<uint32_t>とint5の比較警告2件があったため5uへ修正し最終版を再ビルド。中間の成功を最終生成物へ転用しない。

最終native `work/acceptance/pattern-crud/20261003T111400850Z/run.json`：23件、exit0。無効名/channel/indexの原子性、新規空Part/参照、全既存bytes保持、GUIDを保つ一回UndoRedo、共有コピーの片方削除でPart保持、最後の参照削除でPart回収、空Style、壊れた参照の拒否、Framework snapshot/Segment隔離、4音符を作成した新規Part、旧Pattern2件削除、保存projectを別Frameworkで復元・再保存一致を確認した。

`scripts/Inspect-PatternCrud.mjs`の`crud-proof.json`は独立RIFF読取/組立てでcreated/shared/removed/finalの全bytesを比較する。元全チャンク+新Part/Pattern、複製時の名前だけの変化、対象Patternだけの除去、旧Pattern参照GUIDから導くPart回収、新PartのGUID/24byte音符4件と保存復元の全bytesを検証した。最終Style SHA `5ee2fa4fa5628872f3ef317886ef6e54bd50881fcaafbf66a704a0b9645258d1`。元入力は旧ownership保存物のoriginal.stpとselection.sgpをread-onlyで使用し、その旧試験の成功を新試験へ流用しない。

最終本体音声 `work/acceptance/audio-loopback/20261003T111436846Z`：最終native保存/reloadしたStyleとSegmentを新PIDで再生・16秒WASAPI録音。生成12音はC4、clocks0..8448/768刻み、duration384/PChannel5/velocity96を比較、12区間の録音C4成分も確認。前/後RMS0、再生区間RMS0.02040526197、peak0.0962437093、onset4.5秒。Profile crudは新しい準備時間付き入力に対応し開始2〜5秒を要求する。既存のC4→C5判定を変更せず、新ProfileではC4×12を明示的に要求する。58modules原版40hash一致0。API成功と音声成功は別に判定する。`Inspect-LoopbackAudio`のこのprofileは固定した120 BPM/C4 strings入力用であり、任意曲の音声受入ではない。

最終host `work/acceptance/product-host/20261003T111531455Z/run.json`：exit0、23modules原版40hash一致0。Windows dmime/dmloader/dmstyle/dmband/dmusic/dmsynth/DirectSound/GM.DLSは残る宣言依存。core全suite、現行GUI、原版の新規/削除動的比較、全音源、全40責務/全八受入は未完了。今回のStyle編集はConductorを変更しないため、前版テンポ/Stop音声を無変更で再実行せず旧版結果として保持した。

## 再現と次の作業

Build-ProductSnapshot.ps1で新規保存ビルド。Test-PatternCrud.ps1にBuildSummaryPath、上記read-only入力のSegment/Styleを指定し、Inspect-PatternCrud.mjsへ新runのrun.jsonを渡す。新生成core/pattern-crud/selection.sgpをTest-LoopbackAudio.ps1へ渡し、同じsummary、録音器102354462Zの保存summary/EXE、`-Profile crud -Node <Node executable>`を指定する。Styleは同じディレクトリのHeartlnd.stpを使用する。hostはTest-ProductHostとInspect-ProductModules -CaseName host-smoke。

最終native runのunit-record.jsonに保存source/EXE/入力/native保存物/独立auditor/実録音と生成12音/module証拠/担当文書を結合する。次はGUI新規/音符編集/削除/UndoRedo/保存別起動復元の確認、Pattern clipboard/variation/Motif、空StyleからのBand/Part作成経路と原版比較を進める。新規/削除の限定経路をStyle全体の完成へ置き換えない。
