# Waves Reverb カスタム編集の本体統合

2026-10-09。候補`20261009T012731786Z`。4つの設定値を編集・取消し・Undo/Redoし、AudioPath/Segment/Projectへ保存して別プロセスで復元する経路が限定成立した。必要WASAPIは発音遅延と停止後の残音で不合格。原版画面・操作比較も未完のため、単位`completed=false`、全体`fullAcceptance=false`。0735限定採用を維持する。

一次記録は[単位](../../work/analysis/q3-waves-custom/20261009T004429121Z/unit-record.json)、[開始条件](../../work/analysis/q3-waves-custom/20261009T004429121Z/unit-start.json)、[契約](../../work/analysis/q3-waves-custom/20261009T004429121Z/persistence-and-ui-protocol-v1.json)。対象はAudioPathDesigner/Conductor/DMUSProd、UI08.waves-effect-properties。

## 実装と限定検証

公開DSFXWavesReverbの範囲・有限値を検証し、宣言OSのdsdmo.dllが返すIPersistStreamを不透明な保存データとして扱う。4floatのメモリ配置をnative dataの仕様にしない。選択したWaves以外のchunk・padding・順序を保持し、一回の履歴取引として編集する。非canonical payloadと不正値は文書・dirty・履歴を変更せず拒否する。既存pathのAudioPath Saveも本体へ接続した。

[build](../../work/build/product-snapshot/20261009T012731786Z/build-summary.json)は231保存/作業ソース・4生成物が一致し、configure/build/install各0。Producer.exe SHA256は`f228e0db1930758d0b0e734272c93148d7f7cd57fd410135d2eafa452d347f22`。[関連19](../../work/acceptance/regression/20261009T013250636Z/run.json)は15通常exit0/既知凍結4。Waves opaque persistence/custom/history/native/actualDSPは42checks、Waves runtime39、Send runtime89を含む。登録93native/164driverの全一巡へ加算しない。

本体でdefaults・入力取消し・範囲外エラー・カスタム`-6/-12/750/0.5`・一回のUndo/Redo・Segmentへの埋込・3ファイル保存を操作した。author PID5860とreload PID6804は別開始時刻、同じexe、各通常exit0。reload画面で4値を直接確認した。[GUI監査6](../../work/analysis/q3-waves-custom/20261009T004429121Z/gui-proof-v2.json)と[独立native監査7](../../work/analysis/q3-waves-custom/20261009T004429121Z/native-proof-v2.json)は合格。native差分は選択Waves data、Segmentの同一DMAP、Project二fileのfilh日時/サイズだけ。Sequence/Tempo/Band/未知bytesは一致。

旧0059のモーダル対象公開/focus不足、初期GUI監査のdirty表示空白の誤指定、幾何情報・cached indexの観測エラーは原始証拠を保持した。正常に閉じた子windowへの直後のrefresh失敗は親画面とexit0で終了を確認し、OSアクセス拒否とは扱わない。

## 音声と残差

[固定音声契約](../../work/analysis/q3-waves-custom/20261009T004429121Z/audio-protocol-v2.json)と[実行した監査](../../work/analysis/q3-waves-custom/20261009T004429121Z/audio-proof-v2.json)を保持する。現在候補の既定/カスタム各64秒・WASAPI float32・recorder UTC/QPC・packet・入力/driver/生成物hashを結合したが、onsetが固定1秒を超えた。独立診断は既定1.27/1.09秒、取り直したcustom1.11/1.03秒。停止後0.35〜2.35秒のRMSも固定1e-7を超えた。最初のcustomはStopが7.49秒で4.2〜6.3秒条件も外れ、失敗録音を保存した。音高/tempo/差分/negative controlsの全合格は未成立。閾値を緩めず、必要音声を未達とする。

原版helpはEffects List→Effect Properties→Edit Effect Propertiesの入口を示す。現UIのEffects List/propertypage、InstanceName、AlwaysProcessSoftware、配置/字体/色/アイコンの再現差分は残る。現在の追加taskbar表示とfocusも原版の動的比較は未確認。原版window64100706のapproval timeoutは条件変更がなく凍結。4K/200%は維持した。別名保存は別責務としてqueueへ保持する。

次は音声失敗で判明した通常AudioPathの停止処理とprepare遅延をSDK根拠から修正し、新候補の関連回帰・共有寿命・本体WASAPIを検証する。比較障害を残したまま、独立したUI/機能責務へ継続する。全40/全8/Q2/原版依存解消は未完。
