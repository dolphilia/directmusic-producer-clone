# Mute文書契約（限定実装・全責務未完了）

一次根拠: `work/analysis/sources/dmusicf.h`のDMUS_IO_MUTE（時刻、DWORD PChannel、DWORD map、stride付きmute chunk）、`dmplugin.h`のCLSID_DirectMusicMuteTrack、Producer helpのmutetrack.htmと拍Mute/remap操作。原版動的入力は `work/analysis/q3-mute/20261004T222600Z/original-observation.json`。

原版GUIでソース製Songを開きMute追加。空trackはmute payloadなし、GUID d2ac2898-b39b-11d1-8704-00600893b1bd、group1、trkh ckid mute、trkx8。PChannel1の第2拍クリックはclock768/channel0/map0xffffffff、clock1536/channel0/map0の2レコード。PChannel2→1の先頭remapはclock0/channel1/map0とSegment末端clock6144/channel1/map1。原版はPChannelごとにレコードをまとめ、全時刻順ではない。

画面PChannelは1基点、格納は0基点DWORD。0xffffffff mapは消音、self mapは解除。0xfffffffc〜0xffffffffのsource channelはbroadcast予約値であり個別対象にしない。mapのbroadcast3値も対象外、0xffffffffだけ消音に用いる。16以上を拒否しない。非負時刻、文書長と等しい末端restoreを許可する。上向きremapは形式上DWORDとして保持するが、原版helpは下位channelへのremapだけを有効とする。形式読込と再生互換を区別する。

所有編集はCRUD/clipboard/全文書UndoRedo、group/nth-track選択。不正入力、衝突する同PChannel/同時刻、無変更はbytes/選択/redo不変。stride>=12と整除を確認し、未知record extension/兄弟chunk/paddingを保持。clipboard extensionがdestination strideへ入らない場合は切り捨てず拒否。原版の未編集順序は保持し、変更後はPChannel/time順で編成する。

消音中はnotes/waves/bandイベントを無視、remapはSequence/Style/Bandのイベントを対象channelへ送り、instrument patch変更は元channelへ残す、という原版help契約を維持する。候補20261004T223943407Zの本体文書編集・保存・別プロセス復元と、同一Sequence入力の原版/ソースWASAPI消音・解除・pitch・テンポ比較は限定合格。原版の連続2拍Muteは768消音/2304解除へ合併し、相互編集で復元した。原版PChannel17の拍消音も観測した。clear remap、meter変更、Wave/Style/Band効果、live remap、共通Timelineは未完了。Stop/再開と波形/effects一致はこの単位の合格範囲に含めない。未知data保持やnative parserだけを全責務の対応済みにしない。
