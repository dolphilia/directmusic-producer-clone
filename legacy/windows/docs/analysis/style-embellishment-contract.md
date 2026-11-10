# 所有Style装飾Patternの実選択（2026-10-03）

全体未完了。現行085934923Zを変更せず、所有Styleの装飾選択を4入力で検証した。通常Pattern60、Fill64、Break67、Intro72、End76を各固有Part GUIDに分離した。1小節4音/grid0,4,8,12/duration384/velocity96/PChannel5、variation全有効/randomizationなし。Segment長9216/Tempo120、Command時刻0/3072/6144で通常→装飾→通常。元入力は保持。

[Microsoft DMUS_EMBELLISHT_TYPES](https://learn.microsoft.com/en-gb/previous-versions/ms807636(v=msdn.10))を形式根拠として参照。Commandタイプ0/1/2/3/4（Groove/Fill/Intro/Break/End）とPattern値0/1/4/2/8を区別する。SDK保存dmusici.h/dmusicf.hの宣言も参照。公式資料の内容を入力manifestへリンク記録。

Create-StyleEmbellishmentFixture.mjsで5種類のPatternと各装飾用Segmentを生成。Inspect-StyleEmbellishmentNotes.mjsは保存ソース/EXE/driver/入力・依存hashを照合し、raw RIFFのptnh→pref GUID→Part音符、cmnd時刻/タイプから期待列を算出。通知だけでは選択を判断しない。各4入力で12音の時刻・長さ・PChannel/group/musicValue/MIDI/velocity/flags/playModeが一致。指定装飾の4音の後、6144で通常音高60へ復帰した。Endもこの明示的な後続Grooveあり入力では復帰したが、一般のEnd終了規則には拡大しない。

- fill: work/acceptance/product-notes/20261003T091059804Z/run.json; 12 notes / 57 modules / original40hash0 / passed
- intro: work/acceptance/product-notes/20261003T091106568Z/run.json; 12 notes / 57 modules / original40hash0 / passed
- break: work/acceptance/product-notes/20261003T091113288Z/run.json; 12 notes / 57 modules / original40hash0 / passed
- end: work/acceptance/product-notes/20261003T091120004Z/run.json; 12 notes / 57 modules / original40hash0 / passed

全ケースexit0/started/ended、Stop/CloseDown成功、overflow=false/forwardingFailed=false。同版Producer SHA256 8bbb75ab6ec4e3e187c12bed17cd0c3b80d8cca3d429cf88ed2bf43561a432cb。構成/compile/install結果は085934923Zに固定、今回は生成物を変更せず新規実行した。GUI/音声/core全suite未実行。Windows DirectMusic/DirectSound/GM.DLS依存は残る。原版Producer動的比較/複数装飾bit/EndAndIntro/variation選択/全40全八は未完了。

再現：生成scriptへ元style-command.sgp/Heartlnd.stpと新規outputdir。各fill/intro/break/end.sgpをTest-PlaybackNotes.ps1へ渡しInputPathsに生成Styleを指定。Inspect-ProductModules.ps1 -CaseName notes-apiで由来監査後、Inspect-StyleEmbellishmentNotes.mjsへrun.jsonとembellishmentを渡す。入力manifestはwork/analysis/style-embellishment/20261003T092000Z/inputs.json。

次は本体Style/FrameworkでPattern複製とPart共有解除の原子性/UndoRedo/保存・別Framework復元を実装し、この生成音符比較へ接続する。GUI Stop/再開/音声、JAZP書込み、他Designerと全体受入を維持する。障害なし。同条件の再試行なし。
