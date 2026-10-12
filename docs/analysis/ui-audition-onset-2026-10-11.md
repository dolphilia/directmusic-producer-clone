# Sequence初回試聴の発音開始 — 2026-10-11

画像16・17と24〜28に関係する初回発音の不足を調査し、私有ノート試聴の即時primary再生時刻を修正した。新候補の初回ノートは0.485秒、温間0.570秒、通常再生0.470秒、再開0.460秒。既存1秒基準と音高・音量・停止後無音を含む限定PCM12/12が成立した。旧Sequence全操作1.215秒、Band全操作1.15秒の失敗記録は保持し、全操作300秒検証の成功には置き換えていない。全40責務・全8受入は6作業中/2障害/0合格、fullAcceptance=false。

## 証跡

- [単位記録](../../work/analysis/ui-audition-onset/20261011T075312250Z/unit-record-v2.json) SHA256 d650ce1f86c24f1c7e4f424611b53608171383f9cd2ae4d5318e9c2d6874e9e0
- [開始契約](../../work/analysis/ui-audition-onset/20261011T075312250Z/start-v1.json)、[原因調査](../../work/analysis/ui-audition-onset/20261011T075312250Z/baseline-diagnosis-v1.json)、[API契約](../../work/analysis/ui-audition-onset/20261011T075312250Z/scheduling-contract-v1.json)
- [2ソース差分](../../work/analysis/ui-audition-onset/20261011T075312250Z/source-changes-v1.json)、[288ソース/4出力照合](../../work/analysis/ui-audition-onset/20261011T075312250Z/candidate-identity-v1.json)、[独立監査20条件](../../work/analysis/ui-audition-onset/20261011T075312250Z/artifact-audit-v2.json)
- [固定180秒手順](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/audio-protocol-v1.json)、[PCM12条件](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/audio-proof-v1.json)、[製品QPC照合](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/runtime-timing-proof-v1.json)
- [native保存](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/native-save-proof-v1.json)、[別プロセス復元](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/reload-proof-v1.json)、[実ロード照合](../../work/analysis/ui-audition-onset/20261011T075312250Z/main/module-proof-v2.json)

## 原因と実装

071630の実Mainを同じ入力で診断し、初回ノート1.130秒と通常再生0.450秒を記録した。初回入力操作の幅は約116msで、すべて差し引いても1秒を超える。通常Transportに既に適用していた明示開始時刻を私有ノート試聴が通っていなかったため、その経路を修正した。旧候補には私有試聴traceがなく、旧合成器の内部待ち時間そのものを直接測定できたとは主張しない。

AFTERPREPARETIMEは準備完了後の開始を指定する。[MicrosoftのDirectMusic API資料](https://learn.microsoft.com/en-nz/previous-versions/ms808252(v=msdn.10))を確認し、即時primaryの私有試聴ではpublic ReferenceToMusicTimeによる約300ms先の開始時刻を使う。遅延指定、secondary、boundary、共有/保持済み再生、複合Script/Graphは既存条件を維持する。download-onlyはこの分岐の前に終了する。これは資料と実測に基づく実装判断であり、全条件で300msが最適という判定ではない。

Conductorの私有試聴にopt-in QPC traceを追加し、Mainの既存runtime traceとは別のnote-audition JSONへ正常終了時に保存する。UI入力前からConductor入口までには操作注入とモデル・依存解決が含まれるため、Windowsメッセージdispatch時間を単独で測った値として扱わない。

## 同候補の検証

候補 **20261011T080641928Z**。configure/build/installすべてexit0、保存/作業ソース288件・出力4件一致。Producer.exe SHA256 **c5e42c241f7782b75d01699c0ec6ec85dbcb7bd7635c1b6ed68943657c3a37d9**。

| 項目 | 結果と範囲 |
|---|---|
| 原始native100 | 53合格2965検査、46既存OS凍結、既知匿名Band1検証待ち |
| 関連6 | Timeline117/Transport57/Meter65/Sequence試聴30/Metronome12/Count-In19、計300成立 |
| 原始driver172 | 20合格/28既存OS凍結/124前提不足未実行 |
| 専用3run | Echo、MIDI Input、Performance Optionsはexit0。原始driver20へ加算しない。物理機器0 |
| Main音声 | 180秒、6操作を開始後94秒以内に実行。音高72・既存音量閾値・1秒発音・初期/停止/最終無音の12条件成立 |
| 製品時刻 | 私有試聴2回/Transport2回の明示・解決開始clock一致。要求待ち約300ms、提出からPCM約308〜312ms |
| native保存 | Segment/DLSのbytes不変。Projectは既存filh44byteのFILETIME領域だけ更新。3ファイル全bytes不変ではない |
| 別復元 | 作者22320/復元18968、ともに通常exit0。120 BPM/1ノート/pitch72/duration184320/velocity96/Organ 1を復元。再読込後3ファイルhash不変 |
| 実ロード | 80件中x86アプリ74件とWoW64ホスト補助6件をPE/hash照合。正式原版インストール先0件。全40/Q2の依存解消判定ではない |
| 独立監査 | 20条件成立。正式原版EXE/旧失敗/全体採用候補/Style015244/全8状態の保全を照合 |

[native round](../../work/acceptance/regression/20261011T081444323Z/run.json)、[driver round](../../work/acceptance/registered-drivers/20261011T081718106Z/run.json)、[Echo](../../work/acceptance/midi-echo/20261011T081435492Z/run.json)、[MIDI Input](../../work/acceptance/midi-input/20261011T081438518Z/run.json)、[Performance Options](../../work/acceptance/performance-options/20261011T081439353Z/run.json)を保存した。

## 失敗の保全と次

071630診断PCMは初回1.130秒と最終無音時間不足の10/12でfalseのまま保持する。新候補180秒は操作を前半で終え、停止後約85秒の余白を確保した。録音時間の延長は無音の観測時間を確保するためであり、1秒・RMS・音高・無音の基準は変更していない。

モジュール照合の初回試行はWoW64ホスト補助を一律にSysWOW64へ解決して失敗した。v2は64bitアドレスとwow64*.dllをホスト補助として分け、残りをx86ファイルへ照合した。独立監査v1は保護されたid/status投影と受入オブジェクト全体を比較して1条件falseになった記録を残し、v2で同じ投影同士へ修正した。製品検証の失敗を成功に書き換えていない。

次は画像16〜20のSequence全操作PCMを従来の固定手順・基準で再検証する。ベロシティ入力/高さ変更/音量比/停止再開/無音/native保存/別復元を新しい検証コピーで確認する。Band全操作300秒、Count-In静音失敗、Echo停止中発音/実入力/Advanced/録音、原版同入力音声・外観比較、Style015244、Q2とOS凍結をqueueに保持する。[優先順位レビュー](../../work/analysis/ui-audition-onset/20261011T075312250Z/priority-review-v2.json)を保存した。
