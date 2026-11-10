# 複数Bandの保存修正と公開APIの限界（2026-10-08）

全体未完了、fullAcceptance=false。最新の限定完了は0735候補のQ1であり、1056候補をその本体・PCM成功へ置き換えていない。

[単位記録](../../work/analysis/q3-style-player-multiple-band/20261008T102841039Z/unit-record.json)と[API限界](../../work/analysis/q3-style-player-multiple-band/20261008T102841039Z/band-capability-limit.json)に開始範囲、失敗、保存ソース、診断を保持した。

BandTrackのclassic GetParamは、このOSの三イベントで次の絶対位置3072/6144/0を返した。保存処理の時刻を訂正し、識別できるイベントは完全な所有Band bytesへ結び付ける。匿名の初回だけは明示的な既定Bandを使い、識別不能な後続は拒否する。本体StylePlayerの初期選択は、後続イベントが存在しても時刻0のBandを選べる。初回なし・複数BandTrackは拒否する。

通常HGLOBAL streamの読込はHRESULT 0x88781150。宣言Windows Loaderで同じ三Bandのnative SegmentはS_OKで読めた。Style.GetBandはID/NAME/class有効flags7を返すが、BandTrack.GetParamから戻るBandはflags2で、公開IPersistStream.SaveもE_NOTIMPL（0x80004001）。native埋込とStyle.GetBand→SetParamの両条件で観測した。元データとの後続Bandの対応を確定できず、三Bandの期待値は未達。匿名後続の既定Bandへの置換や、情報の欠落を合格にする変更はしていない。原版DLLは使っていない。

221保存ソースの新候補105655237Zはconfigure/build/install各0、保存/作業ソース一致。新[全native一巡](../../work/acceptance/regression/20261008T110249248Z/run.json)は91登録・53合格/37既知障害/1失敗。既存53は実通常exit0。追加三Band試験で初期選択の三条件は通るが、後続所有対応で失敗する。未対応を隠すために三イベントの期待値を減らしていない。

1038候補と診断v1/v2/v3の失敗、入力、HRESULT、実PID/通常exit1、ソース/生成物hashは保持する。1056本体の保存・別PID復元・PCM、後続Band所有対応、原版比較、全40/全8は未達。同条件API調査は繰り返さず、Farm Night/Birdの音源・100BPM・混合の独立不足を次に進める。
