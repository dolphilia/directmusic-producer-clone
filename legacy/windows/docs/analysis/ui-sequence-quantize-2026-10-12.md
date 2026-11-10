# Sequence Quantizeの接続と保存復元 — 2026-10-12

訂正（原版の初期表示）：下記division2一致の記述は音符glyphの誤読。追加比較で表示4を確認し、明示的に2を指定した。factory defaultは未確認。旧一次記録は保存し、[最新の原版比較](ui-quantize-original-compare-2026-10-12.md)を現行判断とする。

170010：画像15のQuantizeをCtrl+Q/右クリックから接続し、Selected/Part/All Parts、Start/Duration、分割1〜24・Strength0〜100を実装。291ソース/4出力一致、関連4モード413検査合格。Cancel/101拒否/0無変更、選択開始・Part長さ/一UndoRedo・All開始を本体と独立全バイトSGP監査で確認。4トラック8ノート保存、Project filh限定更新、作者3320/別11184の二通常exit0。正式原版の初期値/範囲観測まで一致。丸め/境界の原版動的比較・複数Part GUI・厳密外観・新PCMは未確認。

正式原版とCHMを読み、画像15のQuantizeを既存Sequence編集へ接続した。初期値はSelected Notes、Start Time有効・Duration無効、Divide Beat=2、Strength=100。未選択ではSelectedを無効にしてPartを選ぶ。複数ノートの変更は文書の一括トランザクションで処理し、最後の項目が不正でも文書・dirty・履歴を変えない。PChannel DWORD契約、並び替え後のインデックス、opaqueデータと拡張レコードを保護した。

| 検証 | 結果 |
| --- | --- |
| configure/build/install | 各exit0、291保存/作業ソース・4生成物hash一致 |
| 関連回帰 | common-open53、timeline-range117、sequence-crud213、sequence-audition-runtime30、計413 |
| 本体 | Ctrl+Q/右メニュー、Cancel、不正Strength101拒否、Strength0でdirty/Undoなし |
| 編集 | 選択開始2784→2688、Part長さ192→384/576→768、1回UndoRedo、All開始1728→1920/3264→3456 |
| native | 独立監査9回。SGP全バイトは明示したtime/duration変更のみ、PROは既存filhのFILETIME/sizeのみ |
| 復元 | 作者3320と別11184の二通常exit0。8ノート/4トラックと変更ノートPropertiesを復元 |
| 依存点観測 | 74モジュール=68x86+6WOW64、原版40hash一致0。全体/Q2の証明ではない |

最初の165041候補は境界テストで失敗した。950/50のfixtureは最近傍768へ下がるため境界超過にならず、980/20へ訂正した。製品の境界規則とassertは変更していない。旧run、fixture、訂正記録を保存し、170010で関連回帰を再実行した。監査スクリプトの構文/インポート制限とモジュールパス解決の修正も別記録へ保存した。

原版ではダイアログ観測とCancelまでを行った。最近傍・同距離で前方・Strength整数丸め・Segment外は一括拒否という製品側規則は、原版の動的比較済み仕様ではない。原版の音符/連符glyph、厳密外観、複数物理Partの本体適用は未確認。GUIのAll Partsは単一Part入力に限定し、複数物理track/alias/reorderはcore試験と分けた。新PCMはこの数値編集単位の対象外としたが、統合シナリオでの発音タイミング検証は残る。

Ctrl+QがEditメニューを閉じた直後に開かなかった観測と、直後のアクセシビリティ値の遅れを保存した。ピアノロール再フォーカス後の操作・画面・独立native監査を根拠にし、原因は断定していない。通常終了直後のwindow列挙には閉じる窓が一時残ったため、終了はlauncherのexit0で判定した。

170010の全native100/driver172は未実行。151135の一巡、Motif Win5、将来47native/29driver凍結、SSP04親ドラッグ、原版空OK、旧PCM、独立Style015244/Q2を保持する。0735の限定採用と全体6作業中/2障害/0合格は変更せず、completed=false・fullAcceptance=false。

一次記録：[unit-record-v1.json](../../work/analysis/ui-sequence-quantize/20261011T164107592Z/unit-record-v1.json)、SHA256 `fe242fb63602bb93c504c62f213fa307d0b2fae258af5335a23e6f8c5e854c90`。

次はSSP07/画像43のChoose DLS Instrumentについて、未確認の空collectionとCancelを正式原版の観測・候補本体・文書不変/別復元で検証する。差が判明すれば既存DLS選択モデルを修正し新候補で関連回帰を行う。Quantizeの丸め/境界の原版動的比較と複数物理Part GUI、SSP04親ドラッグ、Add Tracks原版空OK、Count-In/旧PCM/Style015244/Q2/47native・29driver凍結は保持。
