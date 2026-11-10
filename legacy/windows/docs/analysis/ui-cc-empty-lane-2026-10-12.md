# 空のCCトラックの保存・復元 — 2026-10-12

235833 空CC保存を修正：原版は空PBend/Expressionをnative保存・Project再open後も保持。候補のsession-only状態を文書のdesign metadata・dirty・UndoRedoへ接続。291保存/作業ソース・4出力一致、native22/閉鎖26、作者7536・別復元22856・原版native import11244が通常exit0。関連4/471合格、segment-creationは36検査後Mixer.sgpのatomic replace Windows5で失敗し新規凍結。原版へのqCCL読戻し・全CC・厳密外観・短時間Stopは未達。 全体fullAcceptance=false。

原版の専用コピーでSaveだけの対照、空PBend追加、空Expression追加を分離して保存した。Projectを閉じて再openすると両レーンが復元され、EditメニューのUndo Add CC Trackも有効だった。runtime seqtは全段階で同一、1ノート・曲線0件を保持。原版が生成するLIST psqlのpsqc/cvau/cvsuを独立RIFF解析した。psqcの未知フィールドは解釈しない。原版比較は同じ原版processでのProject unload/reopenであり、原版の別process復元ではない。

候補ではレーン一覧をsession mapだけで保持していた。DocumentへCCレーンの追加と列挙を接続し、1操作のUndo/Redo・dirty・保存を実装した。source-owned qCCLは16byte識別子、version1、件数、DWORD PChannelとcontroller識別子を持つ。未知version・不正長・重複・無効controllerを部分適用せず拒否する。原版psqlの読取は今回観測したPChannel0のPBend/CC11だけに限定し、未知のpsqc/cvsuは元bytesのまま保持する。高DWORD PChannel・予約値拒否・既存note/curve/他track保持のassertを削除していない。

新候補235833（Producer SHA256 7e5ae292585034c8053f0b96c6f75ad6fbad60c4575a09fbf4b7b673133f553e）は291 sourcesでconfigure/build/install0。sequence-crud259（26条件追加）、timeline-range117、meter-properties65、sequence-audition-runtime30が合格、計471。segment-creationはBand mixer保存中に36条件後、Mixer.sgpのatomic replaceでWindows error5。失敗候補・出力を保持し、同条件で再実行しない。関連5件の全合格・単位完了とはしない。新規凍結は当該native1件だけで、49native/30driver。224147の原始一巡は歴史として保持し、新候補の一巡へ転用しない。

本体作者7536で空PB追加→Undo clean→Redo dirty→Save、Expression追加→Undoで保存済みPBだけ→Redo→Saveを確認。作者通常exit0、別22856で両レーンがcleanに復元し通常exit0。さらに原版保存データを別11244で読み、PBend/Expressionをclean表示して保存せず通常exit0。native22/閉鎖26の監査が合格。候補のqCCLを除きRIFF長を戻すとSegment全bytesはbaselineと一致する。Project差分はFILETIME/sizeの12bytesだけ。音声はdesign metadataだけの変更のため非適用であり、旧短時間Stop15/16の未達を取り消さない。

現在の画面は原版961×752、候補1207×794。変更後に再観測して操作した。OS倍率は未測定で旧4K/200%は歴史。原版が候補qCCLを読んだときのレーン復元、全PChannelのlegacy design形式、全CC/PolyAT、字体/寸法・開閉状態の永続化は未確認。正式原版25548と未保存Projectの6640は開いたまま保持した。

[単位](../../work/analysis/ui-cc-empty-lane/20261011T234100Z/unit-record-v1.json)、[native22](../../work/analysis/ui-cc-empty-lane/20261011T234100Z/native-proof-v1.json)、[閉鎖26](../../work/analysis/ui-cc-empty-lane/20261011T234100Z/final-audit-v1.json)、[関連回帰](../../work/acceptance/regression/20261012T000541526Z/run.json)、[新規OS拒否](../../work/analysis/ui-cc-empty-lane/20261011T234100Z/segment-creation-os-refusal-v1.json)。

次：SSP02/UI01.priority-05のCreate New File(s)空選択OKを、正式原版→235833候補の順に観測する。既定Segmentの選択を実UIで解除できるか、OKの有効性・確定後の警告/文書/dirty/historyを区別する。差が実在すれば修正し、新候補関連回帰・本体/native保存/別復元/通常終了へ進む。49native/30driverの既知OS拒否、CC短時間Stop15/16、SSP04親/6640破棄回答待ち、独立Style015244/Q2、全40/全8を保持する。
