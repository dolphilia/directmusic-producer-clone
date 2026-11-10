# 現候補のQ1代表統合

候補 **20261006T222153066Z**、198保存ソース。configure/build/install各exit0。Producer.exe SHA256 `1229c2ba21b29c73223d929e03f785aaf5025d4a9f4a53ca8c4fabe6fb5a81b3`、build-summary `9cdb5e73ba8b99afe7b4b833f3caa6f0c9829513c377704e336ed1259e5f21b5`。製品ソースは変更せず、今回の本体操作と録音を同じ生成物へ接続した。[最新単位](../../work/analysis/q1-current-integration/20261006T225403Z/unit-record.json)、[統合監査](../../work/analysis/q1-current-integration/20261006T225403Z/scenario-proof.json)。全40責務・全8受入は未完、`fullAcceptance=false`。

作者PID12768で新規native Projectを作成し、初回保存から6形式10所有文書を含めた。InitialのNote追加、Sequenceの5→6→Undo/Redo→最終5、Style108→109、Band100→101、DLS Key low0→1、AudioPath名変更の履歴と保存を本体で確認。作者は正常exit0。15分の監視期限超過記録は障害として保持し、その後の実際の正常終了を別の読取専用監視で記録した。期限超過を合格へ変更していない。

別PID6232が同じ最終Projectと全12入力を読み込み、Note1、テンポ5/7.5、Style109、Band101、DLS Key low1/Region loop320+11987、Conflictの1..15、Routeの0..16、VBScript言語とUnicode本文を復元。最後に正常exit0。作者保存コピーと全12入力はbyte一致。Project SHA256 `dbb2bb3dc99c6ecca03bd39e5d130fc912c267a3b6d0eaa1e63e9d3dd6829c42`。

| 同じ再読込PIDの録音 | 結果 |
| --- | --- |
| Style参照Segment | Style設定109、Segment120 BPM。MIDI60の10発、2秒間隔、前後無音RMS0、パケット整合合格。6対照合格 |
| Transport優先・停止再開 | 未接続Conflict defaultは無音。埋込Routeで2発を発音中Stop、保持無音RMS0、先頭から全8音再開。間隔12/12/12/12/8/8/8秒、5→7.5 BPM、DLS持続と音高合格。11対照合格 |

録音器は保存ソース版 `20261006T211007927Z`、SHA256 `9e53dacd2b5793935b459fd04233330065d35edc7cbbbfff019fbeede0ec94d2`。StyleとTransportは直列、endpoint/QPC時刻・操作UTC・入力/依存hash・全packet/WAV・判定器を各captureに保存。人の聴取待ちは用いていない。

監査の不整合も修正した。Segment内Bandは元100の私有コピーで、所有Band101とは異なる。Style判定器は今回の入力台帳でhash固定した元Bandの全非参照byteと直下GUIDを照合し、所有Bandとの差を記録する。所有Band101の音声反映は合格としていない。Transport判定器は正常終了で書き直されるlaunchの代わりに録音時の固定snapshotを読む。初回失敗と旧判定器を単位に保持した。

SourceSequenceとControlのfixtureコピーには同じGUIDが残る。選択文書での対照再生は検証したが、曖昧なGUID-only参照解決を合格に含めない。次のfixture整備責務としてqueueに残す。原版動的比較、原版なし独立Windows/Q2、全Producer ABI/全Timeline/全40責務は別の不足である。

固定候補の登録回帰はnative77=47合格30障害。driver107の原始一巡20合格21障害66未実行に、今回の同候補の音声対照2件を補足し22合格21障害64未実行。通常coreは既知Windows5条件で未実行・障害。未実行/障害は合格に含めない。全8受入は6作業中・2障害、合格0。

再現は `Build-ProductSnapshot.ps1` のWin32 Release/reference tools OFFで保存ソースをbuild/installし、同summaryで `Test-RegressionManifest.ps1` と `Test-RegisteredNativeDrivers.ps1`。`Test-ProductProjectGui.ps1` から作業コピーを起動し上記操作・保存・終了、同じProjectを別起動する。`Capture-ProductGuiAudio.ps1 -SilentKeepAlive` のready後にPlay/Stopを記録し、`Inspect-ProductGuiProjectStyleDlsAudio.mjs --style-tempo 109` と `Inspect-ProductGuiTransportDlsPriorityAudio.mjs --slow --require-lifecycle`、各対照試験を実行する。今回の全パス・入力hash・操作state・再監査コマンドはunit配下。新生成物へ今回の成功を転用しない。

次はQ3DのWave default loop inheritanceとtrack/Region override。既存WSMP/PCM/DLSモデルを保持し、契約とruntime接続の不足を調べ、本体履歴・保存・別復元・PCMへ閉じる。Q2と原版比較の障害、既知OS拒否を保持し、同条件再試行は行わない。
