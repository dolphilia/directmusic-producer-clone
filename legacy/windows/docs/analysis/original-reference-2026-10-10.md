# 原版の優先参照先の更新

2026-10-10（日本時間）。ユーザーがリポジトリ外へインストールした原版を、今後の外観・操作・文書・再生の優先比較基準とする。今回の作業は参照方針の調整と読み取り確認であり、引き継ぎ中の製品実装や原版の動的比較は再開していない。

## 確認した参照先

- 実行ファイル: C:/Program Files (x86)/Microsoft DirectMusic Producer/DMUSProd.exe
- 版: 5.3.0.900、x86、518144bytes。
- SHA256: fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011。
- インストールディレクトリ直下39ファイルのうちPEは35件。対応するwork/producer/appの35件と全てhash一致。全40責務の範囲は維持し、この35件を全対象数にしない。
- ホストのアンインストール一覧にMicrosoft DirectMusic Producerを確認。32bit FileOutputDMOのInProcServer32がインストール先を参照している。全COM登録/実ロード元の監査は未実施。
- 読取観測時の原版processはPID24452、開始2026-10-10T02:43:52.1256587Z。この値は履歴であり、入力前に再観測する。

[ファイル一覧](../../work/analysis/original-installed-reference/20261010T024611017Z/installed-files-v1.json)、[ホスト観測](../../work/analysis/original-installed-reference/20261010T024611017Z/host-observation-v1.json)、[COM参照先](../../work/analysis/original-installed-reference/20261010T024611017Z/installed-path-com-query.txt)、[機械可読方針](../../work/analysis/original-installed-reference/20261010T024611017Z/reference-policy-v1.json)。ユーザーの「こちらのほうがオリジナルに忠実」という評価を基準選択に採用した。EXE/DLL/OCXのhash一致だけでは、登録・探索先・設定・文書状態を含む表示/操作の同等性を証明できない。

## 比較と既存障害の扱い

新しい原版のmain frame/メニュー/Project/Timeline/Properties/Transportを、同じ入力・状態・4K/200%で新たに観測する。起動前/入力前に実EXE/hash/PID/開始時刻/返却windowと未保存文書を確認し、ユーザー文書とインストールファイルを保護する。編集比較は検証用コピーで行い、画像と操作列、native保存/別process復元、必要PCMを別判定にする。

旧リポジトリコピーのapp approval timeout・window未取得・Script/Add-Ins不足等は、その対象・環境での履歴として保持する。インストール先/登録/起動対象が変わったため、新対象の動的観測は検証待ちとして評価できる。古い障害を新対象に無条件転記せず、新しい許可拒否が出た場合も許可ゲートの迂回はしない。個別Windows5/Clipboard5・凍結37件は、それぞれの解除条件を確認するまで維持する。

## 製品検証と引き継ぎへの影響

現在のホストは原版インストール済み・一部COM登録ありの比較環境である。これからの015244本体/音声/回帰では、環境版・登録/探索先・実ロード元を採取し、旧インストール前runと区別する。保存ソース・生成物が一致すれば既存buildは利用できるが、旧実行結果を新環境の実行済み結果に転用しない。

原版なしの依存解消(Q2)は、原版PE・Producer COM登録・原版探索先のない別Windowsで行う。比較用原版を削除/登録解除して代用しない。再構築製品が原版DLL/OCXへ暗黙fallbackしていないかも実ロード元で確認する。Q2は未用意のまま、全体受入は6作業中/2障害/0合格・fullAcceptance=false。

[計画](../analysis-plan.md)、[継続プロンプト](continuation-prompt-2026-10-10.md)、[引き継ぎ資料](handoff-2026-10-10.md)、[UI契約](ui-compatibility.md)へ反映する。旧候補・失敗・録音・比較証拠と引き継ぎ021834の記録は履歴として保全する。今回の更新前文書は[索引](../../work/analysis/original-installed-reference/20261010T024611017Z/before-docs-index-v1.json)、新しい引き継ぎ入口は[更新記録](../../work/analysis/original-installed-reference/20261010T024611017Z/reference-update-record-v1.json)、整合性確認は[記録](../../work/analysis/original-installed-reference/20261010T024611017Z/integrity-v1.json)を参照。

## 用途分担と限定起動確認の追加

ユーザーの追加指示により、旧コピーは実験・解析・分析・改造に自由利用できる。改造前hash/バックアップと変更差分を保持し、改造したコピーの動作を未改造原版の仕様根拠にしない。正式インストール版はオリジナルの動作確認用とし、インストールファイルを保護する。[更新方針](../../work/analysis/original-installed-reference/20261010T025959223Z/reference-policy-v2.json)。

指定EXEのhash/518144bytesを再確認してComputer Useで起動を試したが、起動APIは `launched app did not expose a targetable window` を返した。その後のwindow列挙と通常process列挙でDMUSProdが見つからず、スプラッシュのレジストリメッセージ観測/OK、本体表示、スクリーンショット取得はいずれも未確認。手動起動・最低限動作は確認済みというユーザー報告を保持する。起動API/ウィンドウ取得の問題の原因は未確定であり、原版の起動不能・OS拒否・app approval拒否とは断定しない。追加起動・入力・設定変更・強制終了は行っていない。

[起動確認記録](../../work/analysis/original-installed-reference/20261010T025959223Z/startup-check-v1.json)、[今回の更新記録](../../work/analysis/original-installed-reference/20261010T025959223Z/reference-update-record-v2.json)、[整合性](../../work/analysis/original-installed-reference/20261010T025959223Z/integrity-v2.json)。

## 手動起動済みセッションとヘルプの限定確認

前節の起動API失敗の後、ユーザーが手動で起動した正式インストール版を[確認](../../work/analysis/original-manual-session/20261010T031624075Z/session-check.json)した。EXEパス/hashをprocessと結び付け、本体の撮影、Help→Contents、CHMの文字/図の取得、Helpのみの通常閉鎖、本体が残ることを確認した。新たな起動API実行は行っておらず、API起動の問題の解決を意味しない。

CHM SHA256 cc99ff2d87602444c0f9962fe1685e5f0899b49ef3562ba7eae6bae977c13eed。863抽出ファイル中586 HTMLと252 GIFをデコード/検査し、参照画像欠落なし。[話題索引](../../work/analysis/original-manual-session/20261010T031624075Z/help-index.json)、[画像索引](../../work/analysis/original-manual-session/20261010T031624075Z/help-images.json)、[監査](../../work/analysis/original-manual-session/20261010T031624075Z/help-audit.json)を必要な仕様調査に使う。代表話題と4画像の読解/目視までで、全文理解済み・全画面比較済みではない。

200%環境でelement-indexクリックと実位置にずれがあり、原因は未確定。新しい画面の座標指定ではHelpメニューを開けた。一般的な編集操作の成功へ拡張せず、操作前に対象window/画面を再取得する。文書編集/保存/再生は実施していない。visibleなProject名は維持したが入力ファイル全hash監査は非実施。

[番号付き画像の優先計画](priority-segment-sequence-2026-10-10.md)は、この限定観測とユーザーの画像/操作報告を別根拠として扱い、各段階で機能/外観/操作を照合する。
