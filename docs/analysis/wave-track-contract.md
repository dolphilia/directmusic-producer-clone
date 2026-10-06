# Wave track placement / trim

対象はWaveStripMgr.dllの既存外部参照Waveイベントの位置、trim、音量、音高編集。全Wave責務の完成ではない。

原版配布SfxCow.sgp / SfxCow.wvp / runtime SfxCow.sgtを独立RIFF解析し、`work/analysis/q3-wave/20261005T010000Z/original-sample-observation.json`へhashと全chunk配置を保存した。保持済み一次資料`work/analysis/sources/dmusicf.h`のDMUS_IO_WAVE_ITEM_HEADERと照合。原版GUIの同一入力動的比較はInstrument Editor警告後のプロセス消失と再起動画面未取得により未実行。静的監査を動的比較の合格としない。

原版配置はDMTK → LIST wavt → wath / LIST wavp → waph / LIST wavi → LIST wave → waih / LIST DMRF。waihは64 bytes。volume/pitch/variationsのoffsetは0/4/8、time/startOffset/reserved/durationは16/24/32/40、logical/loopStart/loopEnd/flagsは48/52/56/60。12..15はalignmentを含む保持領域。packed 60-byte構造として読まない。SfxCowのtime=0/startOffset=0/duration=14483039、waph PChannel=11、DMRF file=SfxCow.wvp。

保持済みdmusici.hのDMUS_TRACKCONFIG_PLAY_CLOCKTIME=0x40をtrkxから判別する。wath flag0x2はPERSIST_CONTROLでありclock domainではない。SfxCow trkx=0x50はclock-time。現編集はclock-timeだけへ限定し、music-timeはlogical time/tempo契約が未完のため文書不変で拒否する。編集は既存trackのtime domainを変更しない。開始offsetは100 ns単位。位置とdurationは既存trackのunitsを保持し、音楽時刻とreference timeの変換を推測しない。現単位の入力制約はtime/offset非負、duration正、volume<=0、time+durationがsigned64 overflowしないこと。pitchはsigned32。元イベントのpickupや未対応flagを読み込み時に消去しない。reference、reserved、logical、loops、flags、variations、未知extensions、Producer chunksは保持する。

SegmentDocumentは全treeを所有し、編集は私有コピーを検証して一回のhistoryへcommitする。不正入力・破損header・不正selectionでは元文書不変。グループ選択以外のtrackへ変更を漏らさない。

次の残責務は本体GUI操作・保存/正常終了/別プロセス復元、音声でtrim/位置/音量/音高確認、原版相互編集、clock domain変更、WVP単独文書所有/native Project参照catalog、参照解決と実音源長に対するtrim/loop検証、新規イベント/削除/clipboard/variation/streamingとTimeline操作。独立したtyped編集だけでWave全機能を対応済みにしない。

## Q3 WaveイベントCRUDの現行限定契約

単位: work/analysis/q3-wave-events/20261005T102647Z。保存済み原版Help wavetrack/insertingwavesinthewavetrack とSDK wavp/wavi/wave構造を根拠とする。WaveイベントはWVP参照のinstanceであり、イベントコピー・削除で音源そのものを変更しない。コピーはwaih/DMRF/未知子を含む一イベント全部とpart設定/clock domainを保持。現行clipboard WVCP v1はclock-timeの既存part設定全体が一致する先だけへ貼付け、指定time以外のtrim/variation/loop/flags/参照を変えない。新規参照・空instance・別PChannel/partへの移送・music-time変換は未完了で明示拒否する。削除は対象waveだけを除き、空part/list/trackや未知siblingsを保持する。Paste/DeleteはSegment全tree一回historyでcommitし、失敗/overflow/破損/設定不一致時は文書とhistory不変。clipboardは文書editor contextで保持しProject交換時に破棄する。原版動的比較の障害を継続し静的観測をその合格にしない。
