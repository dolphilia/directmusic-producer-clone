# 2026-10-11 Sequence CC編集（SSP06）

画像29〜37のPBend/CC11 Expression追加・独立展開・曲線入力を候補210412で限定確認。保存・別process復元は成立し、固定PCMは14/16条件。全体受入は未完了で `fullAcceptance=false`。

開始記録: `work/analysis/ui-sequence-cc/20261010T205006515Z/start-v1.json`。SSP05候補201607の限定成立と未達を保持し、独立したCCプレースホルダーの解消へ移った。

## 契約と実装

Microsoft CHMと固定SDKヘッダーを照合。SDK Summer2004文書の転載一次資料で、PBend native値は14bit/中心8192、RPN/NRPN番号はMSB/LSBを別々の7bitバイトとして保存すると確認。画面PBend −8192〜8191は保存時8192を加算する。CHM中心0をnative値と同一視した初稿は `contract-correction-v1.json` に保存して訂正。

型付きcurl CRUDと一操作一Undoを追加。signed nOffset、28/32/拡張stride、未知subchunk/padding、ノート、別group/physical track/PChannelを保持。DWORD PChannel16以上/最大許可値と予約4値拒否を保護。不正入力はbytes/dirty/history不変。

本体へAdd New CC Track、種類/パラメーター、個別lane展開、Draw Curve/Insert、ドラッグ破線preview/確定/取消し、選択曲線変更/削除、数値Curve Propertiesを接続。同種laneを重複追加せず、PBendとExpressionを異なる値域で同時表示。

## 候補と実行状態

- 205708867はbuild/install成功。契約訂正前なのでGUI/native/PCM受入には使用しない。旧候補と誤推論を保持。
- 訂正候補 `20261010T210412367Z` はconfigure/build/install各0。271保存/作業ソースと4出力一致。関連13モード914検査、全登録native98一巡は51合格2865検査/46OS凍結/既知匿名Band失敗1保留。driver170一巡は20合格/28OS凍結/122未実行。
- 11変更を `source-changes-v3.json` に保存。GUI/PCM期待値は操作前に `main/gui-audio-protocol-v2.json` へ固定。4曲線でPBend上昇/復帰、Expression減衰/復帰を分離。

## 本体・保存・音声

作者9072と別復元2784は同一EXEで通常exit0。Add取消しでdirty不変、既存PBの重複追加なし、Insert/Draw Curve、曲線数値Properties、一回Undo/Redo、PBとExpressionの同時展開を確認した。native監査は4曲線の32byteレコードを独立読取し、変更curlのみ・他31 leaf不変（全32）を確認。保存後の再読込でも曲線と入力hashを保持した。

固定300秒WASAPIは時計監査と4負例を通過。初回発音0.984秒、PBの約+0.857半音と復帰、Expression低音量比0.182と回復比5.495、再開0.286秒・音高・停止後無音は固定基準を満たした。初回Stopは開始87.789秒後で63秒ノート/64秒Segmentの自然終了後、再開Stopは12.939秒後で要求4〜8秒を超過した。この2条件は操作時刻不備として不合格のまま保持し、製品Stop不具合とは断定しない。閾値変更や同条件再録音で救済しない。旧SSP03〜05の初回発音失敗は旧候補の結果として維持する。

一次証拠は `main/gui-save-reload-proof-v1.json`、`main/native-proof-v1.json`、`main/audio-proof-v1.json`、`main/audio-procedure-failure-v1.json`。録音は `work/acceptance/product-project-gui/20261010T213341158Z/audio-20261010T213704485Z`。全一巡はnative `20261010T214318124Z`、driver `20261010T214838693Z`。単位v2へhash結合する。

## 残責務

空laneは曲線作成までsession表示状態のみ。原版design metadataでの空lane保存は未確認。PolyAT key-specific mappingは未達。Exponential/Log画面形状は近似。原版厳密形状/pixel/DPI/全gesture/clipboard/多選択/各型実音/リセット割込み、Skyが途中停止できないdragの取消し本体/PCM確認は未達。

全98native/170driverの選択一巡は実施済みだが、全合格ではない。発音中Stopの固定手順、原版比較、Q2、全40/全8は未達。46native/28driverの既知OS拒否は変化証拠なしに再試行しない。SSP01〜05・独立Style015244の残責務を維持。

次は未達を保持してSSP07画像38〜43の複数パート/ドラム/DLS選択へ進む。空パートのPChannel保存契約を確定してからAdd New Partを接続し、独立piano roll/CC、PCh10の鍵盤名/ドラムセット、所有プロジェクトDLS選択を検証する。全体統合SSP08で未達音声手順も再設計する。
