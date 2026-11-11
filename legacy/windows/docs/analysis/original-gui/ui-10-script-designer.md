# UI10　Script Designer・Message Window の参照画面

[調査計画](README.md) · [Segment の Script Track](seg-14-control-triggers-script.md) · [Script Content の参照と埋め込み](ui-11-container.md) · [HTML Help の入口](ui-14-help.md)

図は DirectMusic Producer 5.3.0.900 の同梱 HTML Help から採取した。新規作成、ソース編集、試聴、メッセージ出力は現行の原版で追試する。 [画像対応表](../../../work/original-gui/SEG-14-control-triggers-script/20261111T081051527595Z/published-help-figures.json) · [試験素材の保存値](../../../work/original-gui/SEG-14-control-triggers-script/20261111T081051527595Z/farmgame-script-and-audiopath.json)

## Script Designer の3ペイン

原版ヘルプの入口は Script の新規作成、または Project Tree のスクリプトの子ノードをダブルクリック。左の大きい `Source` にコードと横・縦スクロール。右上に `Routines` と Refresh、右下に `Variables` と Reinitialize。Variables の表は Name と Value。 [Script Designer Window](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/scriptdesignerwindow.htm)

![原版ヘルプの Script Designer。左が Source、右上が Routines、右下が Variables](figures/script-help-01-designer.gif)

ヘルプでは Refresh がルーチン一覧と参照データを更新する。構文エラー時は一覧が空になり、Message Window にエラーが出る。Routine を実行する入口はダブルクリック、Ctrl+R、右クリックの `Call Routine(s)`。複数選択では一覧順に実行する。変数は値の列をクリックするか Ctrl+E または `Set Variable`。Reinitialize で未初期化状態に戻す。

## Script Properties とメッセージ

Project Tree の Script 文書を対象にする Properties は Script と Info の2タブ。Script ページは Name、Language と2個のチェック欄。図では `AudioVBScript` が表示され、ロード時にコンテンツを読み込む設定と、Segment をダウンロードする設定が両方オン。これは図の状態。 [Script Properties](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/scriptproperties.htm)

![原版ヘルプの Script Properties。Language とロード時の2個の設定](figures/script-help-03-script-properties.gif)

`Add-Ins > Message Window` から開く。図では Messages の下に行番号を含む Script エラー、ランタイムエラーの説明、エラーコードが3行で並び、選んだ行は青。ヘルプには Script Designer と Script Track からのエラー・Trace の入口があり、再生中の出力には PChannel 1 がある AudioPath を使用する条件が記載されている。 [Message Window](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/messagewindow.htm)

![原版ヘルプの Message Window。行番号とエラーコードの表示](figures/script-help-02-message-window.gif)

## 観測の来歴と再開条件

- 2026-10-06 の旧調査用コピーでは `ScriptMessagesOriginal.pro` の新規作成までは成功した。同じセッションの `Create New File(s)` は Project のみ、Add-Ins は空だったため Script Designer に到達できなかった。 [旧セッションの観測](../../../../../work/analysis/q3-script-messages/20261006T041300Z/original/observation-summary.json)
- 2026-11-11 の別の原版コピーでは ChordMap 作成時の New Files に Script が表示された。Add-Ins の Message Window と Script 本体の操作はこれから撮影する。 [New Files の原版撮影](figures/harm-63-new-chordmap-selected.jpg)
- [FarmGame の素材](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/inputs/farmgame/manifest.json) の `FarmMusic.spp` は `FarmMusic` という Script、`Script Content` という内蔵 Container、12個のルーチン宣言を持つ。Container の10件は `BGDawn.sgp`、`BGPredawn.sgp`、`BGNight.sgp`、`SSBird.sgp` と6件の効果音 Segment。保存された相対ファイル名と対象の GUID は10件すべて一致した。
- 追試は Project の作業コピーから FarmMusic を開く操作、Refresh 後の一覧と変数、参照表示、Properties を撮る。構文エラー、Trace、実行エラー、クリアは小さい試験スクリプトで比較し、実行入口と出力行を照合する。Document と Segment の参照を保存し、別起動で復元を確認する。
