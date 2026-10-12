# Mute拍セルの本体接続

2026-10-10（日本時間）。最新試験候補2309（`20261009T230910277Z`）。限定採用0735と全40責務・全8受入を保持。`completed=false`、`fullAcceptance=false`。

原版helpの拍セルクリックと保存済み原版SGPを基準に、Timelineへ1拍のmute/unmuteを接続した。選択group/nth Mute trackを使い、一操作を一つの文書履歴に記録する。原版の768 mute／1536 restoreを生成し、隣接するMuteは境界を統合する。初期16行に加え、入力にあるPChannel 17以上も表示する。DWORD許可範囲・予約4値、未知chunk・拡張stride、拒否時のbytes/dirty/history不変を保護する。拍内部に混在イベントがある場合の一般編集方針は未完のまま、今回の操作は変更を拒否する。

[build](../../work/build/product-snapshot/20261009T230910277Z/build-summary.json)は238保存/作業ソース・4出力一致、configure/build/install0。関連11 modeは合格、Mute専用99項目。本体ではPChannel 1の2拍目、PChannel 17の3拍目をクリックし、unmute・Undo/Redo・Ctrl+S・Save Projectを確認した。別processでXと連続Muteを復元し、作者5108・復元19624・音声25120は通常exit0。独立解析では空入力に追加したMute payloadが原版1拍データと一致し、PChannel 17では復帰時刻1536→2304以外のbytesを保持した。4保存ファイルは復元/音声processで不変。[保存復元監査](../../work/analysis/ui-mute-beat-grid/20261009T230035741Z/gui-native-proof-v2.json)は799照合、32観測・40画像を結ぶ。画像数や照合数を完成率にしない。

Fileメニューのindexed自動操作で、Save Projectの代わりに新しいStyleが作成された。原因は未確定で、OS拒否や製品不具合とは断定しない。Styleを検証フォルダーへ保存して保持し、画面で確認したSave Projectを座標指定して実行した。このStyleは機能検証の合格対象に含めない。[操作失敗](../../work/analysis/ui-mute-beat-grid/20261009T230035741Z/menu-action-failure-v1.json)を保持する。

160秒の固定WASAPI録音では、1拍Muteの無音、次の拍からの復帰、7音の音高と5→7.5 BPMの時刻間隔が限定成立。mute/baseline/tail RMSは0、packet最大gapは2frame、初回発音遅延は約1.130秒。再開・発音中Stop・停止後無音を含む拡張protocolは、Play開始と自然終了観測の遅延で録音枠に収まらず失敗した。未実施を合格にせず、閾値変更・同条件再試行をしていない。[固定条件と失敗](../../work/analysis/ui-mute-beat-grid/20261009T230035741Z/audio-protocol-result-v1.json)と[限定Mute音声](../../work/acceptance/product-project-gui/20261009T233302520Z/audio-20261009T233353758Z/mute-audio-proof.json)を区別する。

原始native96は58合格/37既知凍結/既存後続匿名Band失敗1。原始driver164は23合格/23障害/117未実行/Mute driver失敗1。Mute native自体は99項目合格だが、driverの旧scope完全一致が新しい拍区間試験の説明文を拒否していた。説明文2形式を明示認識し、native passed・exit・hash・入力不変の条件を維持して同候補の専用driverを補足実行し合格。原始失敗を保持し、補足適用後24合格/23障害/117未実行を別表示する。manifestは再生成せず変更したdriver hashのみ更新した。[節目監査](../../work/analysis/ui-mute-beat-grid/20261009T230035741Z/milestone-proof-v1.json)に原始一巡と補足を結ぶ。

Whole Segmentのcontext、Add/Remove PChannelsと表示永続化、remap GUI、Mute Track Properties、拍内部混在方針、原版の配置・字体・状態・動的操作比較は残る。4K/200%は変更せず、保護対象の原版process21252へ入力していない。必要比較未確認のため単位を完成へ昇格しない。[現行単位記録](../../work/analysis/ui-mute-beat-grid/20261009T230035741Z/unit-record-v2.json)から入力・ソース・生成物・観測・PCM・失敗を辿れる。
