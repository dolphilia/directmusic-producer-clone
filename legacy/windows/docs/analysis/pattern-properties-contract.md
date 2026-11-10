# Pattern名・装飾種別編集（2026-10-03）

全体未完了。最新保存54sources/3targets 094600314Zは構成・compile・install各exit0、ログwarning/errorなし。Producer953344bytes SHA ac0f17480bb25ed0303ec42209712229c84858701290adb8592720171a7bf8a3、core1219584bytes SHA 79dce2b4b65e8aa1a0d4a609aa22a1f94032900797cfa0cb3b9c134de0885ac4。対象20件、host094718542Z、編集後Fill/Introの生成12音×2を同版で検証。GUI・音声・core全suiteは未実行。0920所有権GUI結果を転用しない。

## 形式根拠と実装

凍結work/analysis/sources/dmusicf.h DMUS_IO_PATTERNのwEmbellishment WORDはptnh offset6..7、名前はLIST UNFO/UNAM UTF16。既存原版Heartlndの全RIFF・Pattern/Part配置観測と0859装飾選択記録を再利用。今回公式Microsoft旧資料URLは取得失敗（cache miss）で新しい資料の成功として扱わない。原版Producerの同GUI編集・保存比較は未実行。

StyleDocument::set_pattern_propertiesは名前/装飾を一つのbytes履歴へ採用。名前1..255 UTF16 code units・埋込NULなし。選択範囲・空/256/NUL/同値は無変更false。新規装飾値は既知Normal0/Fill1/Break2/Intro4/End8のbit範囲0..15。既存の他値は同値でのrenameを保持するが通常との相互変換は拒否する。Motif新規・変換は別途mtfs/再生契約が必要で未完了。bit組合せ3は保存readbackのみ、このruntimeでは単独種別だけ確認。

ptnhの2bytesと変更した名前以外、PartGUID/pref/音符/variation/flags/未知chunk/padding/tailを保持。UNFO/UNAMなしは作成する経路が実装済みだが今回境界試験は未実行。UNFO/UNAM重複は既存unique検証により変更前に例外。名前変更だけでは元装飾bytes不変。同名Patternは形式上拒否していない（名前をPart identityにしない）。

FrameworkはStyleコピーの編集後、依存Segment contextを先行準備してapply_style_editで文書とcacheをまとめて更新。Segment保存bytes/dirtyを変更しない。UndoRedoもcacheを更新する。本体右列にPattern name / type、Normal/Fill/Intro/Break/End combobox、Set Pattern Propertiesを追加。未対応既存typeはOther (unchanged)でrename維持。Style/Pattern選択で表示/enable/値を復元。これらGUI入力は今回compileのみで未操作。

## 同版の検証

native094717628Z20件：不正値/同値でbytes履歴不変、Fill64をConverted Intro 64/値4へ一編集、UNAM/ptnh以外全bytes比較、一回UndoRedo、Intro72をConverted Fill 72/値1へ編集、Part identity維持、未知値rename/変換拒否、組合せ3、255文字、重複名前chunk拒否、Framework snapshot/history/Segment bytes/dirty不変、Style/project保存・別Framework復元・全byte再保存。元5Pattern入力と全生成ファイルhashはrun.jsonへ保存。

同nativeで保存したHeartlnd.stpを通常の本体Framework再生経路で使用。fill.sgpは元native入力、intro.sgpは元fixtureの無変更コピー（新入力として証拠にhash固定）。Fill run094736359Zは60×4→72×4→60×4、Intro run094803393Zは60×4→64×4→60×4。各12音の全属性・Command通知時刻・source/runtime Style全bytesが独立raw RIFF oracle一致。自然終了/Stop/CloseDown、overflow=false/forwardingFailed=false。各57modulesとhost24modulesは原版40hash一致0、独立監査passed。Windows DirectMusic/DirectSound/GM.DLSは依存物として残る。

Inspect-PatternProperties.mjsは入力Style treeから2つの名前/WORDだけを書き換えた期待treeを構成し、first/edited/Heartlnd/resaved全bytes一致、保存source54/EXE/coredriver/input/stdout20/全生成物hash、2notes proof/同版EXE/依存Stylehash/実音高を照合。properties-proof.jsonとauditorコピーに固定。

監査ツール初回は第2引数styleを渡し、必要値embellishmentのassertで失敗。そのため後続properties監査もnotes-proof不足で失敗。引数を修正して既に記録された同じrunを比較し成功した。本体を再試行していない。初期auditor件数24の仮記載は実行前に完成traceの20へ修正。host module proofは初回read時にまだ生成しておらずENOENT、独立監査後に実結果を採用した。これらを製品失敗やOS拒否と混同しない。

## 再現・残作業

Build-ProductSnapshot.ps1、Test-PatternProperties.ps1 -BuildSummaryPath -Segment 元fill.sgp -Style 元Heartlnd.stp。生成core/pattern-properties/fill.sgp/Heartlnd.stpをTest-PlaybackNotes.ps1へ渡す。元intro.sgpを同生成directoryへ全byteコピーして別run。各Inspect-ProductModules.ps1 -CaseName notes-api、Inspect-StyleEmbellishmentNotes.mjs run.json embellishment、最後Inspect-PatternProperties.mjs nativeRun fillRun introRun。本体hostはTest-ProductHost.ps1とInspect-ProductModules.ps1。新規runを使用し古い成功を転用しない。

次は現行GUIでPattern名/種別変更・UndoRedo・保存・通常Close・別launch復元・全byte再保存、その後長い入力でStop/再開と音声確認。Pattern新規/削除/clipboard/variation、Motif変換・未知type/UI対応、欠落UNFO境界、原版動的比較、JAZP書込み・他Designer、全40責務/全8受入は残る。


## GUIプロパティの同版受入追記（2026-10-03）

ソース/生成物094600314Zを変更せず、専用コピーprojectをGUI095313400Z/PID17092で起動。Fill64をGUI Intro 64/Introへ変更、一回UndoでFill64/Fillへ戻りRedoで両方復元。Intro72をGUI Fill 72/Fillへ変更しSave Document、dirty解除。通常Close exit0。別launch095750876Z/PID16476で両名前・Intro/Fill種別を文字列と画像で復元確認、Save Document再保存、通常Close exit0。Style SHA e808287707e4954697870f693e26903bc16fc49b8b60d078136f69330c715e5d、別launch入力と再保存が全bytes一致。各GUI45modules原版40hash一致0。

Inspect-PatternPropertiesGui.mjsは元Style treeのPattern2/4の名前と装飾WORDだけを変更した期待treeとGUI保存全bytesを比較し、他Part/Pattern/unknown/tail/paddingを保持。2launch同版source54/EXE/異なるPID/exit0、projectとSegmentbytes不変、GUI復元文字列、14画像hashとmodule proofを独立照合。proofはproduct-project-gui/20261003T095313400Z/properties-gui-proof.json。過去native unit-recordは当時のGUI未実行という正しい記録のまま保持する。

ツール障害：popup中のtext-only観測はaccessibility null、Fileメニューindexは外部座標(-68,-38)、別launchの初期text-only観測ではgeometry unavailable。各失敗後に現行窓の新しいscreenshotを観測し、現在画像の選択座標で継続した。アクセシビリティ一覧ラベルは即時snapshotで古い値を返したため、再観測したプロパティ値と画像を根拠に採用した。OS拒否はなく、閉じた窓へ入力しない。GUI再生・音声/長入力Stop再開は今回未実行で次手に残す。
