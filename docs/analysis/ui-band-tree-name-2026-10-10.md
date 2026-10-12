# Band Treeの名前と共有Properties — 2026-10-10

候補2221で、所有Bandの名前をProject TreeからF11または右クリックPropertiesで編集する経路を追加した。既存の共有modeless PropertiesがBand・Segment・Styleの選択に追従する。BandのUNFO/UNAMだけを編集し、文書全体のUndo/Redoと保存済み状態へ接続した。Project保存時には既存Band参照の表示名も更新する。

原版helpのBand Name、単一Properties窓、F11・右クリック入口を根拠とする。原版本体の動的比較は既存approval障害が残り、配置・字体・色・アイコン・手順の等価は未確認。Edit GUID、PChannel Properties、多重選択、pan/volume mixerは未実装のqueueに保持する。200%設定は変更していない。

候補は `20261009T222110833Z`。保存/作業238ソースと4出力を照合し、configure/build/installはexit0。Producer.exeのSHA256は `c193e78f78e7fbf957261dc9a04d3f59ab9dfee48d940ec2abf31945bcd5a460`。関連14試験が合格、2既知障害は凍結。component-nameは70チェックであり、開始時の台帳にあった74という記述は訂正した。

本体author PID22124で「Project Band 新名」→「Studio Band 音」、Undoによる元名/clean復帰、Redo、Properties欄からCtrl+Sを確認した。名前未設定の別Bandには「Session Band」を設定し、BandとProjectを保存した。PID23648で両名を復元し、PChannel17/997の楽器設定とPChannel0の別Bandを確認した。両processは通常exit0。保存5入力は再読込後もhash一致した。保護対象の原版PID21252は操作していない。

独立RIFF監査でBandのGUID・楽器・参照・opaque bytesを保持し、Segment/Styleファイルの不変とProjectの既存参照/identityを確認した。GUI結合監査は43観測/76画像をhash照合した。modeless窓を含むUIA cache不一致2件はunknown操作として保持し、再観測後の通常クリック/キー操作で結果を確定した。OS拒否とは分類していない。

共有Properties変更の共通再生回帰は未達。最初の64秒録音では画像なし観測後の入力が `coordinate input geometry is unavailable` となり、実Playに到達しなかった。画像付き観測へ変更した別録音ではPlay/Stop入力は返ったが、Play後からStop入力まで12.145秒となり固定4.2〜6.3秒を超えた。replayは継続せず、6必要観測中3のみで判定器も不合格。入力・閾値を変えず失敗録音を保持し、音声合格へ昇格していない。

全体一巡は同候補の原始native96を58合格/37凍結/既存後続匿名Band失敗1として保持する。driver164の原始結果は単位記録のmilestone proofを参照する。旧2141/2048の結果は履歴であり、この候補の成功へ加算しない。

`boundedFinished=false`、`completed=false`、`fullAcceptance=false`。Band名前編集と保存復元の不足は減ったが、必要音声、原版外観/操作、全40責務、独立Windows/Q2、同最終構成の全8受入は未完。音声の同条件再試行はせず、次は原版helpにあるMuteトラックの拍セルクリックによるmute/unmute操作を既存nativeモデルへ接続する。

一次証拠は `work/analysis/ui-band-tree-name/20261009T221904681Z/` のstart、source-changes、build-binding、related-regression、gui-protocol、gui-bindings、gui-native-proof、native-proof-v1/v2、audio-attempt-1/2、audio-proof-v2、milestone-proof、unit-recordを参照する。Git対象外workの保全が必要。
