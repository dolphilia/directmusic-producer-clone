# 全体再構築の検証状態

更新日：2026-10-04。取得済み40 PEモジュールの暫定管理表。対象範囲の削減や工程6の完了を示す一覧ではない。

製品の最新状態は [本体実装と引継ぎ](product-host.md)、[機能台帳](feature-map.csv)、[機械可読状態](product-state.json)。原版不要の本体 producer と producer_core、producer_core_tests の構成・コンパイル・install成功は限定製品構成についてのみ。ユーザーの環境変更後、native実行と限定GUI試験を実施した。音声と全機能の受入は未完了。

最新製品ビルド: work/build/product-snapshot/20261002T232641307Z/build-summary.json。現行native記録: work/acceptance/product/20261002T232742232Z/run.json。旧拒否記録: work/acceptance/product/20261002T132157931Z/run.json。passed: current471 checks; current GUI integration rebuilt and same-run deletion output playback API passed。全体完成は未達。

識別情報と個別状態は [implementation-status.csv](implementation-status.csv)。依存ライブラリ・同梱サンプルも台帳に残し、必要なビルド・配布条件を今後確定する。

| 群 | 現在の状態 | 次の判定 |
| --- | --- | --- |
| TempoStripMgr | 候補690のクリップボード・終了再起動と原版相互読込・聴取を確認。新候補00c3は選択更新のネイティブ6914件と本体表示が一致。別の本体試験でコピー・切り取り・再貼付け保存とUndo/Redoバイト一致を確認。別試験の古いUIツリーは証拠から除外。原版TimelineのExportカウンター残存は失敗履歴として保持 | 新しい保存物の候補再起動・原版読込、現在のUI記録、候補音声、実ドラッグ・複数文書、非テンポ保存差分を調べる |
| TimeSigStripMgr | 保存17ソースからビルド・実行し、原版と322観測・27ファイル、試験用Timeline接続160観測が完全一致。接続比較器7テストと既存検証器10テスト成功。Style管理対象の検索失敗経路のみ。修正した実TimelineプローブはOS起動拒否 | 実Timeline・Style拍子データ取込み・描画・編集・UI・通知配送・ランタイム同期と本体置換を確立する |
| DMUSProd 本体・Framework・Timeline・Segment文書 | 型付きfactory/所有権、RIFF/metadata保持、Tempo/明示拍子/最小Sequence編集、保存、Undo/Redoを実装。passed: current471 checks; current GUI integration rebuilt and same-run deletion output playback API passed。partial: current471 native Framework reload and deleted-DLS API passed. GUI delete/save/restart pending; audio/original dynamic comparison/all40/all eight gates incomplete. | Style参照/拍子、残るtrack/COM通知と全体受入を接続 |
| Conductor・最小Sequence/BandTrack | passed: current232641307Z same-run deleted.dls exact bytes register/download/Play/Stop/Unload with identity restart. Current GUI Play/audio unverified.。Unverified for current232641307Z; old piano listening confirmation remains separate. | 全音符/Band/DLS/AudioPath編集、現在版の音声測定/endpoint、原版相互読込を拡張 |
| BandEditor・DLSDesigner・その他の Producer モジュール | passed: current471 checks; current GUI integration rebuilt and same-run deletion output playback API passed。partial: current471 native Framework reload and deleted-DLS API passed. GUI delete/save/restart pending; audio/original dynamic comparison/all40/all eight gates incomplete. | Wave参照付き削除/loop編集、Style/Segment自身SaveAs/履歴、articulation/group-aware tracks・他40責務 |
| MFC42 / MSVCRT / MSFLXGRD | 原版の依存ライブラリとして識別 | 配布と呼出し規約、使用範囲を確認する |
| Farm / StylePlayer | 同梱サンプル・補助アプリとして識別 | 全体受入での役割と依存を確認する |

現行の共通CMakeを保存49ファイルから全12ターゲット構成・ビルドした記録は work/build/recovery-snapshot/20261002T120616172Z/build-summary.json。比較プローブ・静的ライブラリを含む12ターゲットであり、Producer12モジュールの再構築ではない。TimeSig単独の現行19ソース・5ターゲットは work/build/time-signature-snapshot/20261002T120513711Z/build-summary.json。今回生成物は全て未実行。TimeSigの成功比較は旧候補630bと保存17ソースのビルド work/build/time-signature-snapshot/20261002T114801406Z/build-summary.json を使用した。Styleプローブの修正前の不完全な保存観測は互換基準から除外し、修正後の別ハッシュの起動拒否を work/reference/time-signature-style/20261002T120234201Z/run.json に保持する。

現行の Tempo 比較は原版 Timeline・TimeSigStripMgr と Windows の dmime.dll に依存する。Producer全体を原版固有モジュールなしで動かす構成と、実プロジェクトの作成・編集・保存・再読込・音声出力の受入は未完了。全体完成の判定は [計画書](../analysis-plan.md) の受入試験を維持する。


最新AudioPath単位：現行081843304Z保存67sources構成/build/install0。FrameworkはAudioPath .aup→.audとSegment root DMAP保持、限定編集metadata除去を実装。Conductorはembedded設定取得/明示AudioPath作成/同path Download・Play・UnloadとSequence接続確認を実装。関連247＋専用7checks/独立raw/host passed。元Source不在でAPFarm設定PChannel10→Performance16のAPI・音符一致、WASAPI440Hz/Slow Attack/途中Stop/再開/停止無音passed、派生PCM3拒否。同版AudioPathなし回帰録音passed。再生56/host23modules原版40hash0。AudioPath ownedモデル/編集GUI・全core/全40/全八は未完了。 [記録](../../work/analysis/runtime-audiopath/20261004T081400Z/report.md)。CSVは40行を保持。旧表の限定候補成功はその版だけの履歴。
