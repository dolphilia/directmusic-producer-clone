# Runtime Save As保存先記憶 — 2026-10-07

候補 **20261006T173721618Z**。194保存ソースのconfigure/build/installはすべて0。全体完了はfalse。

原版helpはRuntime Save Asで既定名・フォルダーを変更できると明記する。既存132353単位の原版UNFO/rdir・rnam編集/保存復元と現行15試験を照合し、実装済みの文書別設定は作り直さず、本体Save Asが設定を記憶しない不足を修正した。原版の同メニューが無効な動的比較障害は残る。

Frameworkはメタデータを先に検証し、出力成功後にProjectへ反映する。6文書種のUI保存を接続。保存拒否は設定・Project変更状態・Segment文書と履歴を保持する。低レベルexportは既存契約のまま。

| 検証 | 結果 |
| --- | --- |
| 専用native | 保存先記憶25 / 既存文書別フォルダー15 合格 |
| 登録native76 | 46合格・30障害。失敗/未実行0。通常coreは既知拒否のまま |
| ドライバー107 | 20合格・21障害・66未実行。未実行を合格に含めない |
| 本体 | author12016 exit0 → reload22360 exit0。Initial.sgpの名称GuiRemembered.sgt・フォルダー..\Export\復元 |
| native RIFF | 選択文書のrnam/rdirだけ変更。他leaf/padding保持 |
| 新候補音声 | 未実行。旧候補PCMを転用しない |
| 全8 | 6作業中・2障害、合格0 |

独立監査の初回はcatalog ordinal0を仮定したため拒否。実ファイル名Initial.sgpで所有を照合して修正し、初回証拠も保持。原版動的比較の代わりにはしない。

Standalone記憶・dialog初期値・正確な原版Save Asのpath表現、5種の追加GUI操作、Q2、全40責務はqueueへ残す。Runtime群を際限なく細分化せず、次はQ3D Wave PCMの範囲選択・cut/copy/paste/cue/loop補正を、現実装と原版help/既存最小入力の契約で照合し、不足する1編集単位をnative履歴/本体保存/別起動まで実装する。原版designer障害は保持。機能群の節目で新候補Q1音声をWASAPI再確認する。

再現: scripts/Build-ProductSnapshot.ps1、scripts/Test-RegressionManifest.ps1 -BuildSummaryPath <新summary>、scripts/Test-RegisteredNativeDrivers.ps1 -BuildSummaryPath <同summary>。GUIはTest-ProductProjectGuiで作業用native Projectを起動し、Runtime Save As→Runtime Properties→Save Project As→正常終了→別起動→Runtime Propertiesを行う。

詳細・hash・入力・PID・証拠は [unit-record](../../work/analysis/q3-runtime-saveas-memory/20261006T173215Z/unit-record.json) を参照。
