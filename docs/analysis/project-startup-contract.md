# 本体の保存project直接起動（2026-10-03）

対象版は `work/build/product-snapshot/20261003T082511918Z/build-summary.json`。全体は未完了。

GUI起動時に必ずUntitledを作る旧導線は、保存projectの別起動復元に不要な未保存文書を混在させた。本体に `Producer.exe --open-project <absolute-project-path>` を追加。Frameworkで先に読込み、文書種別を選択してからGUIを生成する。WM_CREATEはproject_pathが空の場合にだけ新規Segmentを作る。通常の引数なし起動は従来どおり。空projectの主なSegment操作を無効化、読込み警告を表示する。引数不足/未知引数/不正projectはGUI作成前にexit1。原版Producerの起動引数互換とは主張しない。

保存54sources、Win32/MSVC2022/SDK10.0.26100.0、3targets。configure/compile/install各exit0、build.log warning/errorなし。Producer927744bytes SHA256 `d5a2ecbf611897b5d3543de941c8c55040848ba12decccb63903454e469611a7`。core tests1180160bytes SHA256 `0c8c820c23111bfaf9b83fe15d8e22f2a8a70096bceae98d9c0ab88b5e2d49db`。core fullsuite/通知APIをこの版では実行していない。旧0813通知API/0742 Commandの成功を転用しない。

host082701698Z成功・依存監査成功。GUI起動はproduct-project-gui配下082648179Z/083103049Z/083752032Z。launcher.ps1はビルド/ソース/EXE/入力hashを先行照合、引数とPIDを記録し通常Closeのexitを待つ。timeoutでは強制終了しない。3つのPID6556/16608/10100は各exit0。

初回は旧0742 GUIが保存したcommand-project.dmpjを現行EXEで復元。コンボはstartup.sgpとsaved-moved.sgpの2文書、余分なUntitledなし。saved-movedのTempo0=120、3072=180と8音符を表示、Play後自然終了表示。ユーザー回答「聞こえた。途中から速くなった」はこの版/入力/play-startだけに限定してaudio-confirmation.jsonへ保存。GUI74modules原版40hash一致0。

2回目は通知表示のため、同じ744bytesのSegmentのTempo doubleを2か所だけ30へ変更したslow.sgpと単一文書DMPJを使用。入力由来/オフセット/変更値はwork/analysis/project-startup/20261003T082900Z/inputs.json。スクリーンショットでPlaying document snapshot、Playing — embellishment changed、Stop後Stoppedを観測。Stop直前のaccessibilityはPlaying — Groove changed。accessibilityは画面更新より遅れるので同一captureの旧textと新画面を混同しない。Play capture08:35:14.787Z→Stop capture08:35:30.352Z、名目16秒に対して約15.6秒。厳密な停止時刻や十分な余裕を持つ途中停止はまだ未検証。通知は実Pattern選択の証拠ではない。slow音声はユーザー未確認。

GUIでresaved-slow.sgpとresaved-slow.dmpjを新規保存。元と再保存Segmentは744bytes全bytes一致SHA256 `0bbce3338c82efe670bad34f6435a3879e2ce50e4637ed47621d93c59633a53b`。DMPJのvers/nameは保持、fileだけslow.sgp→resaved-slow.sgp。通常Close後、同じ現行EXEの別PIDで再保存projectを起動し、相対参照からresaved-slow.sgp/Tempo両30/8音符を復元、通常Close exit0。2回目GUI127modules原版40hash一致0、address/PID付きの一点観測。OS DirectMusic/DirectSound/GM.DLS依存は残る。

不正起動3ケースはproduct-startup-failures/20261003T083956333Z/run.json。--open-project引数不足/未知option/不正DMPJすべてexit1。不正fixtureのhash不変。拒否理由のUI表示/全異常入力/すべての文書種別は未検証。

独立監査scripts/Inspect-ProjectStartupGui.mjsは保存54sources/EXE/3launch/PID/exit/入力/32枚画像hash/2依存監査/ユーザー回答/Segment全bytes/RIFF DMPJ参照変更/拒否3ケースを照合。証拠は083103049Z/project-startup-proof.json。画像の意味はComputer Useで直接観測、機械監査は画像hashと対応textの範囲だけ。全体受入はfalse。

再現手順：Build-ProductSnapshot.ps1、Test-ProductHost.ps1、Inspect-ProductModules.ps1へBuildSummaryPath/RunPath。Test-ProductProjectGui.ps1へBuildSummaryPath、Project、依存InputPathsを渡してGUI起動し、Computer Useで観測・保存・通常Close。各runのlaunch.json/states.jsonを保持。Capture-ProductGuiModules.ps1とInspect-ProductGuiModules.ps1で実行中依存を監査。Test-ProductStartupFailures.ps1へBuildSummaryPath。最後にNodeでInspect-ProjectStartupGui.mjsへ初回dir、slow dir、reload dir、failure run.jsonの4引数を渡す。現行証拠へ新しい再実行結果を上書きしない。

ユーザーが多数の検証アプリを閉じたと回答したため、旧GUI HWNDや削除modalは再利用しない。削除自体は未実行。初回Close操作はユーザー入力検出により入力前に拒否されたので、現行状態を再観測して通常Closeした。OS拒否の迂回/セキュリティ設定変更なし。

次手は所有Styleの異なるGroove範囲と識別可能なPattern音符列を準備し、Command通知と実際に出る音符/Pattern選択を対応させる。本体再生中のStop/再開を余裕ある入力で検証する。JAZP書込み、meter再整列、未整列/同時刻/拡張stride、他Designerの責務、全40PE/全8受入は維持。
