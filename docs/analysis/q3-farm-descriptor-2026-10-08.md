# Farm の名前・カテゴリ参照（2026-10-08）

候補 20261008T073500338Z で、所有カタログ内の正確な Unicode NAME / NAME+CATEGORY と class による参照を実装し、本体の保存・別プロセス復元・音声まで限定合格を確認した。原版比較、全40責務、同じ最終構成での全体8受入は未完了、fullAcceptance=false。

[開始契約](../../work/analysis/q3-farm-descriptor/20261008T072556077Z/unit-start.json)、[単位記録](../../work/analysis/q3-farm-descriptor/20261008T072556077Z/unit-record.json)、[結合監査](../../work/analysis/q3-farm-descriptor/20261008T072556077Z/completion-v2/proof.json)から一次記録へ辿れる。結合監査 SHA-256 は 6d12d4f4e8e40dbe5949de579e1e78035e754843dcdc50dcea9126f128236205。

SDK Loader の選択順位に従い、GUID object、stream、memory、full path、name/category、name、local filename の扱いを分離した。未対応 stream/memory/URL は明示的に拒否する。使用する名前とカテゴリは有効な descriptor flag の値であり、残存する無効な GUID/filename は選択に使用しない。既存の file/GUID 所有、class 制約、native bytes、KEEP flag、header tail の保全を維持した。case 同値・名前衝突・ANSI metadata・identity-less/cycles/config/graph/KEEP 寿命は未確認として残す。

218保存ソースと作業ソース、4生成物が一致し、configure/build/install は各0。Producer.exe SHA-256 は fd0684f26a3b722a6e7294c3f405507c9935e1fd55115c11459c696805a705be。関連13試験は実PID・通常exit0で合格し、依存試験200 checks、20 alias を使う Farm runtime32 checks を含む。

検証入力は原版のデータ19ファイルを保持した上で、10個の埋込 owner と10個の NAME 参照を持つ新しい Script を作った。うち5参照は CATEGORY も指定する。本体author PID8660で Script 名称を変更し、Undo/Redo、FarmDescriptorSaved.spp、DescriptorHost.sgp、native-inputs-v1.pro を保存した。author通常exit0後、同じEXEの別PID15508でProject・Script・Farmを復元・初期化し、通常exit0を確認した。23入力は保存コピーと二つの終了後のhashが一致する。

独立RIFF監査は、要求した Script のUNAM以外の source/container/header/GUID/opaque bytes 不変と、Projectの二文書のpath/size/GUID/runtime filenameを確認した。変更なし正例と9反例の判定器試験も合格。これを原版の相互編集互換性の証明とは扱わない。

同じ復元PID・同じ入力の実WASAPI録音で、Cougar/Cow/Rooster/Sheep/Wolf/Alarmの六種は固定相関0.98と音高条件に合格した。停止後RMSは0。別録音ではNight発音中Stop、停止後無音、再開、secondary操作後の継続、再Stop、private Farmウィンドウを閉じた後の無音が成立した。音声正例と5反例も合格。Night/Birdの音源・音高・100BPM、一般的な同時音源の分離、Predawn/Dawn/Endは未確認。

120秒の初回録音は後半操作が収録時間外となり監査exit1だった。録音と操作時刻を保持し、300秒の新規録音で検証した。閾値は変更していない。最初のcompileでの試験補助変数重複と、結合監査の入力形式誤指定も失敗記録・元判定器を保持して修正した。

新候補native登録90の原始一巡は53合格/37既知障害/失敗0。driver登録159の原始一巡は24合格/23障害/112未実行。同候補10補足を加えた34合格/23障害/102未実行は原始結果と分離する。既知拒否の条件変更を確認できない通常core、各native保存Windows5、原版approval timeout、Q2独立Windows未用意は凍結する。

新候補採用を[adoption.json](../../work/analysis/q3-farm-descriptor/20261008T072556077Z/adoption.json)へ保存した。次は固定候補Q1代表経路を実行する。旧030413のQ1・Send、本体Farm183/Waves183/Style210の成功は版付き履歴であり、新EXEの実行結果へ転用しない。
