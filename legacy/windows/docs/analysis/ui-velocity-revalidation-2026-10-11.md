# Sequenceベロシティ試聴の固定300秒再検証

2026-10-11（日本時間）。画像16・17に対応する既存4段階PCMの限定再検証。画像20の数値編集ダイアログは開閉確認だけで、今回新たな数値変更試験は行っていない。全SSP04操作の成立とはしない。

080641・追加ソース変更なし。既存4段階ベロシティ300秒PCM12/12、初回80=0.495秒/96=0.480秒/上端preview=0.505秒/40=0.485秒、音量比96/80=1.437・40/80=0.248、最終無音成立。Undo80・8notes保存復元・作者22876/別21724の二通常exit0、独立監査20/20。SSP04全gesture/原版外観操作、Band300秒等は未達。

候補は `20261011T080641928Z`、Producer SHA256 `c5e42c241f7782b75d01699c0ec6ec85dbcb7bd7635c1b6ed68943657c3a37d9`。初回発音修正の保存ソース288件・生成/配置4出力と作業木の一致を再確認した。追加製品ソース変更はない。同候補のnative100件=53合格2965検査/46OS凍結/既知Band1保留、driver172件=20合格/28OS凍結/124未実行は直前の初回発音単位の一巡を参照し、今回再実行した扱いにはしない。

| 操作 | 発音まで | 音高 | RMS | 判定 |
| --- | --- | --- | --- | --- |
| 初回80クリック | 0.495秒 | MIDI61 | 0.033342 | 合格 |
| 96クリック | 0.480秒 | MIDI61 | 0.047918 | 合格 |
| 上端80→40ドラッグ | 0.505秒 | MIDI61 | 0.024885 | 合格 |
| 40クリック | 0.485秒 | MIDI61 | 0.008272 | 合格 |

事前保存したプロトコルの順序・300秒・発音1秒・音高・音量差・無音・非clip条件を従来と一致させた。音声設定は64 voices/22050Hz/55ms/既定Synth・永続化off。最後の試聴は録音開始70.108秒、以後約228秒の無音を含み、baseline/tail/診断尾部外RMSはいずれも0。packet gap最大2frames、peak0.100299。12条件が成立した。ドラッグ途中はprivate試聴2件を記録したが、全ての中間velocityを個別PCM分離した証明ではない。Transport再生/停止はこの4段階には含めず、前単位の別証拠を参照する。

ドラッグ後の40とUndo後80を本体値で確認。Save File→Save Project後のSeed.sgpは入力とbyte一致し8ノートを保持。Project差分は44byte filhのFILETIME位置16〜21だけ。別process21724でtime2784/pitch61/duration576/velocity80と8ノートを復元し、復元前後の入力hashを照合。作者22876と復元21724はいずれも通常exit0。80ロードmoduleを74x86アプリ用/6AMD64 WoW64ホスト支援に分けPE/hash照合、正式原版インストール先依存0件。この点観測をQ2/全40成立へ転用しない。

最初の300秒試行は最後の入力が約297秒となり、従来の入力<296秒assertionに失敗した。録音と操作記録を保持して新しい入力コピー・新しいprocessでやり直した。旧候補の初回1.215秒/1.15秒、初回発音単位baseline1.13秒の不合格記録は変更していない。

一次証拠は [`unit-record-v1.json`](../../work/analysis/ui-velocity-revalidation/20261011T090428025Z/unit-record-v1.json)（SHA256 `2a8df31dd72d98c38abe940027d0db12bf7cfed0b94295292ec55c53a0757724`）、[`artifact-audit-v1.json`](../../work/analysis/ui-velocity-revalidation/20261011T090428025Z/artifact-audit-v1.json)、[固定PCM](../../work/acceptance/product-project-gui/20261011T090459709Z/audio-20261011T090708724Z/velocity-pcm-proof-v1.json)、[失敗試行](../../work/analysis/ui-velocity-revalidation/20261011T085344008Z/attempt1-timing-diagnosis-v1.json)。監査20/20合格。screenshot47枚のhashと正式原版EXE hashを維持。

次単位は画像25〜28のBand mixer固定300秒・19条件再検証。Band live-gesture-followup/main-repeatの固定300秒プロトコル・19条件を読み、新しい検証コピーと作者processで初回発音、pan/volume対照、再生中Undo/Redo、停止/再開と最終無音を同じ基準で再検証する。入力時刻を前半へ配置し、native保存/別process復元/二通常exitを確認。旧1.15秒失敗、Count-In15/16（操作窓80.906秒>80秒）、Echo停止中/実機/Advanced/録音、SSP04全gesture/原版比較、Style015244/Q2/OS凍結を保持。

SSP04の左右端/中央移動/Grid/None/保持キー/取消し/AllParts/percent・原版同入力/厳密外観は今回再証明していない。旧Band18/19、Count-In15/16、独立Style015244、OS凍結、全40/全体8受入を保持する。限定採用0735と全体6作業中/2障害/0合格、fullAcceptance=falseを維持。
