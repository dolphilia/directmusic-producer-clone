# UI14・ROLE01　Farm Demo、StylePlayer、導入の入口

2026-11-11。Farm Demo と StylePlayer は原版関連の配布資料に含まれる独立した試聴用アプリ。Producer の Help は原版画面を確認済み。このページの補助アプリの配置は EXE のダイアログリソース、動作の記述は同梱 README と Farm のソースで確認した。各アプリの実機操作と音声は以下の資料に掲載する。 [Farm Demo の実機記録](ui-14-farm-demo-live.md) · [StylePlayer の実機記録](ui-14-styleplayer-live.md) · [UI14 Help](ui-14-help.md) · [EXE のハッシュ、リソース ID、コントロールの寸法と位置](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/auxiliary-resources.json)

## Farm Demo

タイトルは **DirectMusic Farm Demo**。ダイアログのリソースは 308×173 DLU、8 pt の MS Shell Dlg。タイトルバーと最小化ボタンを持つ固定枠で、**File > Exit** がある。DLU はダイアログのフォントに依存する単位。背景のピクセル寸法とは別に扱う。 [Farm.rc](../../../../../work/producer/samples/FarmGame/src/Farm.rc)

![Farm.exe の埋め込み画像と一致する 466×319 px の背景素材。納屋と草地の上に実行時にボタンが描かれる](figures/ui14-farm-background.png)

| 場所 | 項目 | 配置と色の定義 |
| --- | --- | --- |
| 左列 | Sound Effects。Cougar、Cow、Rooster、Sheep、Wolf、Alarm | x=7 DLU、幅60、高さ18。オレンジ |
| 中央の上 | Backgrounds。Night、Predawn、Dawn、End | x=80 DLU、幅104、高さ18。緑 |
| 中央の下 | Secondary Segment。Bird | x=80、y=146 DLU、幅104、高さ18。黄 |
| 右下 | All Stop | x=189、y=146 DLU、幅112、高さ18。赤 |

12個とも Tab 移動の対象として宣言されたオーナー描画ボタン。ソースでは押下中に枠を沈め、中央に黒字でラベルを描く。Bird、Backgrounds、Sound Effects に淡い色の分岐もある。実機ではフォーカス枠が見えない状態で Tab・Shift+Tab 後の Space が対応する効果音を鳴らし、操作後は通常色を保持した。押下保持中の画面は未採取。 [キー操作と色の実機記録](ui-14-farm-demo-live.md) · [ボタンの描画とコマンド処理](../../../../../work/producer/samples/FarmGame/src/FARM.CPP)

操作は同梱 Script のルーチンに渡る。例は Night → dmBGNight、Bird → dmSSBird、End → dmEnding、All Stop → dmAllStop。説明書では効果音と Bird は随時演奏でき、背景を開始すると End を演奏できる。アプリの初期化コードはカレントディレクトリの res、次に ..\res で FarmMusic.spt を探す。初期化失敗時には英語のエラーダイアログを出す分岐がある。 [同梱 README](../../../../../work/producer/samples/FarmGame/README.TXT) · [Farm の起動とルーチン呼出し](../../../../../work/producer/samples/FarmGame/src/FARM.CPP)

実機では session/Farm.exe と session/res/ の構成で起動し、初期表示、Tab と Space、6つの効果音、背景切替、Bird 単独、Night 中の Cow、End、All Stop を撮影・録音した。素材なしの作業コピーではロードと初期化の警告を採取した。背景の小節境界、混合時の Bird の分離、個々の参照ファイル不足は追試対象。 [実機記録と録音](ui-14-farm-demo-live.md) · [実行用の20ファイル](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/farmgame-runtime/manifest.json) · [Container の保存値](ui-11-container.md)

## StylePlayer

EXE の主ダイアログは **StylePlayer**、163×252 DLU、8 pt の MS Shell Dlg。上の **Create Segment** 枠は Style、ChordMap、Band、Shape の4つの選択式コンボ、Intro・End、数値型の Length と Measures、Re-Compose。下の **Play Segment & Style Motifs** 枠は Play と Stop、その下に横長の進捗バー、最下段に2列2行の Motif ボタン。リソースの初期ラベルは4個とも **No Motif**。実行時は選んだ Style の Motif 名に替わり、Rumbling Point のように Motif がない場合は **No Motif** の灰色表示になる。**About StylePlayer** は Alt+Space のシステムメニューから開く。**New** ダイアログの入口は未観測。 [ダイアログの原データ](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/auxiliary-resources.json) · [選択・About の実機画面](ui-14-styleplayer-live.md)

同梱説明書では起動時に Style をランダムに選んで試聴を始める。パラメーターを変更すると先頭から再生し、Band の変更では再生位置を維持する。Shape は Groove Level の推移に対応し、Re-Compose はパターン、コード経路、バリエーションを再選択する。現在の素材フォルダーには Style 187件、Chordmap 27件がある。説明書では EXE と同じディレクトリに置く。 [StylePlayer.txt](../../../../../work/producer/style-library/StylePlayer.txt)

素材ありの作業コピーで2回起動し、初回の音と再起動時の進捗で自動再生を確認した。選択リスト、Band が1件と9件の場合、Motif の単独発音、主要設定の前後と再生位置を記録した。素材なしの作業コピーではプロセスだけが残った。再生中の Motif を混合音から切り出す試験、各 Shape の Groove Level 推移、Intro・End の音の境界は追試対象。2026-10-07 の承認待ちタイムアウトは過去の試行記録に残す。 [実機操作と録音](ui-14-styleplayer-live.md) · [過去の試行記録](../../../../../work/analysis/q3-style-player/20261007T022658Z/original-launch-block.json)

## ROLE01　インストール済み Producer

Windows の32-bit アンインストール登録に **Microsoft DirectMusic Producer** があり、旧式の InstallShield エントリからインストール先の Uninst.isu と Uninst.dll を参照する。インストール先の Producer はバージョン 5.3.0.900 で、保持した未修正 EXE とはハッシュが異なり、既存の起動修正の記録と一致する。今回の確認はファイルと登録情報の読み取りまで。インストール画面と削除ウィザードは未観測。 [導入と起動修正の記録](../../../README.md#この環境のインストール済み版) · [旧起動警告の画面記録](../producer-startup-dialog.md) · [現在のファイルと登録情報](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/installed-and-processes.json)
