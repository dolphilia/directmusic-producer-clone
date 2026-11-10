# Q3F 発音中の録音Stop

候補 `20261007T043410164Z` の本体で、録音Stopが演奏を中断せず、2本のWAVを確定する限定経路を確認した。全40責務・全体8受入は未完、`fullAcceptance=false`。

本体作者PID5624で、2音のdurationを3072→122880 clocksへ変更し、Undoで3072、Redoで122880を確認した。native SegmentとJAZP Projectを保存して通常exit0。別PID18808で同じ5入力をFile/Openから復元し、60 BPM・両duration122880・PChannel1/9・FileOutput effects2を確認して通常exit0。独立RIFF監査では、保存Segmentの変更は2音のdurationだけで、Projectの4文書のGUID/サイズ/参照が一致した。長いcarrierのsegh184320は作者起動前に準備した入力変更であり、GUI編集とは分けている。

240秒のWASAPIと2本のFileOutput PCMを取得した。録音開始→Play→音楽Stop→Play→発音中の録音Stop→約30秒の発音継続→音楽Stop→Play→音楽Stopを操作時刻と結合した。C4/A4は録音Stop前後とも存在し、移行区間の最小10ms RMSは0.043238。各音楽Stop後、再開まで無音。録音WAVは両方69.649秒、番号順にA4/C4の排他的経路を持ち、確定後と再開後もbytes/hash不変だった。DMOにはUTC/QPCがないため、2回の発音間隔一致と実測約0.72秒の相対offsetを記録し、絶対時刻一致を主張しない。packet最大gap2 frames、timestamp error0。テンポ変更はこの単位の範囲外で、Q1/各経路に残る。

元の短音PCM条件を維持して長音の明示モードを追加した。短音・長音の各7対照と、本体の12対照（録音Stop後無音/片音/40ms隙間、Stop後混入、再開欠落、遅い録音Stop、確定後増加、clock誤指定、timestamp error、同作者PID、操作時刻誤差）を検証した。これらは判定器の回帰であり、旧候補の録音を現候補の実行結果へ転用していない。

一次証拠は [単位証明](../../work/analysis/q3-file-output-stop/20261007T050605Z/unit-proof.json)、[作者保存入力](../../work/analysis/q3-file-output-stop/20261007T050605Z/saved-inputs.json)、[PCM/時刻判定](../../work/analysis/q3-file-output-stop/20261007T050605Z/recovery-session/audio-20261007T053525528Z/file-output-active-stop-proof.json)、[2出力PCM](../../work/analysis/q3-file-output-stop/20261007T050605Z/recovery-session/recordings/file-output-multi-pcm-proof.json)、[対照](../../work/analysis/q3-file-output-stop/20261007T050605Z/active-stop-controls/negative-tests.json)。build summary hashは`1ab980a1330131ef15267df0e3a6a8e729dcb2e7f2b28bb19ff92eec27fd8c2e`、Producer.exeは`e7a41478123c090e6ceb0c37820e60930560e7c59b5337c07414a4b18a3e8239`。

再判定コマンド（リポジトリrootで実行）:

```powershell
node scripts/Inspect-FileOutputMultiPcm.mjs work/analysis/q3-file-output-stop/20261007T050605Z/recovery-session/recordings work/build/product-snapshot/20261007T043410164Z/build-summary.json work/analysis/q3-file-output-stop/20261007T050605Z/author/MultiCapture --active-stop-long-notes
node scripts/Inspect-FileOutputActiveStop.mjs work/analysis/q3-file-output-stop/20261007T050605Z/recovery-session/audio-20261007T053525528Z work/analysis/q3-file-output-stop/20261007T050605Z
node scripts/Inspect-FileOutputActiveStopUnit.mjs work/analysis/q3-file-output-stop/20261007T050605Z work/analysis/q3-file-output-stop/20261007T050605Z/recovery-session/audio-20261007T053525528Z
```

新規実行では同候補のinstall、保存入力、明示した録音器 `work/build/audio-capture/20261006T211007927Z/build-summary.json` を使い、Computer Useの観測したウィンドウに対して上の操作を行う。各操作をUTC前後時刻付きで保存し、通常終了をread-only observerで取得する。既存録音を上書きせず、新しい録音先と対照ディレクトリを使う。依存は宣言済みWindows DirectMusic/DirectSound、所有DLS音源、default render endpoint。原版実行物はこの経路で使用していないが、Q2独立環境の証明にはならない。

失敗履歴を保存した。PID21012のGUI観測失敗は`foreground window did not report a process id`で、OS拒否/製品故障とは判定していない。所定のComputer Use一覧で見えなかったPID5836は900秒後も残存し、終了成功に含めない。所定のexplicit saved executable起動で取得したPID18808だけを今回の復元/音声に結合した。初期空文書はscratch-recovery.sgp/dmpjへ保全した。旧録音器に`--silent-keepalive`を指定した実行はargc不一致exit2として保存し、対応済み録音器へ変更した。既定録音器も対応済み版へ修正した。録音中のGet-FileHash共有エラーとLength44の点観測は合格根拠にせず、確定後のRIFF/PCM/hashで判定した。

原版StylePlayerのapproval timeout、designer不在、Q2未用意、core/Script Reference.spp/Containerの別々のWindows5凍結は保持する。Send/未接続録音group、同時session、live切替、legacy ABI等はFileOutput残責務。次は独立した継承DLS runtime exportへ進み、既存編集・private playback解決を再実装せず、本体のruntime保存経路を修正・検証する。
