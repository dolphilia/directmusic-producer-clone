# Segment music-time StartTime

041453112はSegment music-time StartTimeの限定実装。segh mtPlayStart DWORD8だけをatomic変更し、原版CHMに沿うBar/Beat/Grid/Tick入力・spin・Enter/focus-loss/CtrlTab確定・CtrlA、PlayFromStart設定点と通常Playカーソルの独立を接続。clock/reference編集は対象外。feature base031819499から7source変更285同一、直前label不合格035502578からProperties2変更290同一、292saved-working/4outputs完全一致。新StartTimeだけMS Sans Serif8point/ANSIを所有し子破棄後に解放。関連7/1069、model41契約とnative既定384/明示0/明示384/Infinite四case合格。

作者7904で全4spin上下、Bar0/Tick192拒否、Enter/Tab/CtrlTab確定、最終1:3:3:0=1920、Tree Undo二回1920→1536→0とRedo二回→1536→1920を各保存。25段階のnative照合で開始DWORD以外の全Segment byte・他2SGP・Project FILETIME以外を保持。最終差は2byteのみ。設定開始点1920とeditor cursor3332を実primary要求で区別し、元8秒後もループ範囲内のカーソル、停止安定を確認。Properties三cycleのGDI90→90（事前上限+8内、普遍的leakなしとはしない）。作者7904、復元14640、座標診断17816、別復元21284は全て通常exit0。別新設定でStart1920/Infinite3072..6912/Tempo120を復元再保存。

新baseline0固定240sは19/19、7PCM/fourclock全拒否、二episode11発音[0,1.5,4,6.5,9,11.5,14,16.5,19,21.5,24]、WAV2373cee027982cfbd662c2d5c8d1b676b4b71562a6428c989124eddbce9f94e5。新member1920固定240sも19/19、7PCM/fourclock全拒否、二episode10発音[0,2.5,5,7.5,10,12.5,15,17.5,20,22.5]、既存±.03秒内。private onset.595/.580、main.715/.725、Stop25.013/.010、停止後無音。WAVefc59b56673972f8c4a09378f3fbbc9c5f0792e03b9b46ef94fe8038167c66cd。両phaseで新PIDの二つの無入力25秒試験を通過し、入力・生成物hash/UTC-QPCを照合。閾値緩和/旧WAV流用なし。

member初回はUIA再Play入力が本体要求へ到達せず5actions、required-action監査exit1として保全。Stop25.012自体は範囲内、再送や欠落finalStop補作なし。同じUIA手順を凍結。別PIDの実画像座標診断で二primary要求1920/二Stopとnative完全保持を実証して入力方法を変更し、更に新PIDと二応答試験を経た別録音だけが合格。正確なUIA入力不達原因は未確定。最初の制限付きshell PID確認失敗は録音前・recorder未起動で、実在を読み取り照合後に同じ可視範囲で録音。protocolのmember nativeExpected.playStart残存0は録音前に1920へ訂正し原文を別保存、発音列・閾値は不変。035502578 label不合格、親031819499の86.585秒Stop PCM不合格と凍結は保持し、遡って合格にしない。初期Tree白/動く画像とUIAの時差も保全。原版同入力動的・全geometry・任意TimeSig group・Extend・embedded AudioPath・clock/reference・Info/Boundary・全40全8は未受入。6working/2blocked/0pass/fullAcceptance=false。

一次入口：`work/analysis/ui-segment-start-time/20261110T041200000Z/unit-record-v3.json`。

StartTimeの限定結果を保存し、Segment Properties Info tabの原版CHM/静止図と既存UNFO/INFO保存モデルを照合する。未知chunk/header/文書GUID/履歴を保護し、根拠のあるInfo編集をmodel→保存build→関連→本体保存通常終了→別復元→必要音声/原版比較まで進める。既知50native/30driver凍結と旧失敗を条件変更なしで再試行しない。全40全8/fullAcceptance=false、goal active。
