# Script/Farm のANSI名参照

2141候補では所有DLS/WaveのINFO/INAMによる名前参照を修正し、本体の読み込み、名前編集・Undo/Redo、native保存、別プロセス復元まで限定成立した。238保存/作業ソース・4生成物一致、configure/build/install各0、関連15合格/既知凍結1、二通常終了0。原版の外観・操作比較は未確認で completed=false/fullAcceptance=false。

宣言Windows LoaderのGetDescriptorをDLS/Wave各6条件で独立観測した。ACP932ではINFO/INAMを使用し、UNFOの前後順序に左右されず、Unicode名だけ・空ANSI名からUnicodeへ切り替わらない。初回の不正DLSとLoader cache混入は失敗履歴を保存。有効DLSとcaseごとのfresh Loaderで12条件を確定した。厳格な不正/重複拒否は所有resolverの方針であり、原版挙動とは区別する。

本体の作者PID22316と復元PID20360は同じEXEで動作し、それぞれDLS/WaveのInitialize ScriptとLoadContentを成功させた。文書名を「DLS ANSI 音」「Wave ANSI 音」へ変更し履歴を確認、新規DlsSaved.spp/WaveSaved.sppとAnsi.proへ保存した。監査では変更が文書名だけで、完全なContainer/参照header/KEEP/音源本体/不明chunkを保持。最初の3入力と保存後6ファイルの復元前後hashは一致する。

固定protocolはContent.Loadだけの非発音routineを対象とし、音声はこの経路では非適用。Farmの再生/100BPM/primary Bird/mix/full KEEP音声を合格にする証拠ではない。UIAの古いelement cacheとfocus情報のずれは失敗操作を保存し、新鮮な画面のcaretと実際のfilenameで復旧した。SDK/Windows5/原版approval障害とは区別する。

節目の原始native96は58合格/37既知凍結/既存後続匿名Band失敗1。driver164は24合格/23障害/117未実行。両manifestは本体証拠の更新を挟んだ別hashだが、96/164の登録契約と凍結条件は一致。2048の原始95と9条件変更補足は別履歴として保持する。

[単位記録](../../work/analysis/q3-script-ansi-name/20261009T211806287Z/unit-record-v2.json)、[独立契約](../../work/analysis/q3-script-ansi-name/20261009T211806287Z/contract-v2.json)、[本体/native判定](../../work/analysis/q3-script-ansi-name/20261009T211806287Z/gui-native-proof-v1.json)、[一巡監査](../../work/analysis/q3-script-ansi-name/20261009T211806287Z/milestone-proof-v1.json)。

次は未列挙のUI05 Band Project Treeの名前Propertiesを具体化し、本体F11/context・編集/history・保存/別process復元へ接続する。Edit GUID、PChannelの全設定/graphical mixing、原版比較、Q2、個別Windows5/Clipboard5、全40/全8は未完のまま保持する。
