# UI14・ROLE01　Farm Demo、StylePlayer、導入の入口

2026-11-11。Farm Demo と StylePlayer は原版関連の配布資料に含まれる独立した試聴用アプリ。Producer の Help は [UI14 Help](ui-14-help.md) で原版画面を確認済み。このページの補助アプリの配置は EXE のダイアログリソース、動作の記述は同梱 README と Farm のソースで確認した。補助アプリのライブ画面と音声は未観測。 [EXE のハッシュ、リソース ID、コントロールの寸法と位置](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/auxiliary-resources.json)

## Farm Demo

タイトルは **DirectMusic Farm Demo**。ダイアログのリソースは 308×173 DLU、8 pt の MS Shell Dlg。タイトルバーと最小化ボタンを持つ固定枠で、**File > Exit** がある。DLU はダイアログのフォントに依存する単位。背景のピクセル寸法とは別に扱う。 [Farm.rc](../../../../../work/producer/samples/FarmGame/src/Farm.rc)

![Farm.exe の埋め込み画像と一致する 466×319 px の背景素材。納屋と草地の上に実行時にボタンが描かれる](figures/ui14-farm-background.png)

| 場所 | 項目 | 配置と色の定義 |
| --- | --- | --- |
| 左列 | Sound Effects。Cougar、Cow、Rooster、Sheep、Wolf、Alarm | x=7 DLU、幅60、高さ18。オレンジ |
| 中央の上 | Backgrounds。Night、Predawn、Dawn、End | x=80 DLU、幅104、高さ18。緑 |
| 中央の下 | Secondary Segment。Bird | x=80、y=146 DLU、幅104、高さ18。黄 |
| 右下 | All Stop | x=189、y=146 DLU、幅112、高さ18。赤 |

12個とも Tab 移動の対象として宣言されたオーナー描画ボタン。ソースでは押下中に枠を沈め、中央に黒字でラベルを描く。Bird、Backgrounds、Sound Effects に淡い色の分岐もある。現在の実行版でその表示に至る条件、フォーカス枠、ポインター形状は実機で確認する。 [ボタンの描画とコマンド処理](../../../../../work/producer/samples/FarmGame/src/FARM.CPP)

操作は同梱 Script のルーチンに渡る。例は Night → dmBGNight、Bird → dmSSBird、End → dmEnding、All Stop → dmAllStop。説明書では効果音と Bird は随時演奏でき、背景を開始すると End を演奏できる。アプリの初期化コードはカレントディレクトリの res、次に ..\res で FarmMusic.spt を探す。初期化失敗時には英語のエラーダイアログを出す分岐がある。 [同梱 README](../../../../../work/producer/samples/FarmGame/README.TXT) · [Farm の起動とルーチン呼出し](../../../../../work/producer/samples/FarmGame/src/FARM.CPP)

追試では session/Farm.exe と session/res/ の構成にし、res に [確認済みの20ファイル](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/farmgame-runtime/manifest.json) を複製する。初期表示、Tab と Space、押下と解放、背景の切替、Bird と効果音の重なり、End と All Stop、通常終了を撮る。音の開始・遷移とボタン色の変化を時系列で照合する。素材不足は別の作業コピーで試し、参照先は [Container の保存値](ui-11-container.md) と照合する。

## StylePlayer

EXE の主ダイアログは **StylePlayer**、163×252 DLU、8 pt の MS Shell Dlg。上の **Create Segment** 枠は Style、ChordMap、Band、Shape の4つの選択式コンボ、Intro・End、数値型の Length と Measures、Re-Compose。下の **Play Segment & Style Motifs** 枠は Play と Stop、その下に横長の進捗バー、最下段に2列2行の Motif ボタン。リソースの初期ラベルは4個とも **No Motif**。実行時の初期有効状態と表示名は未確認。**About StylePlayer** と **New** のリソースもある。両方とも到達経路は未確認。 [ダイアログの原データ](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/auxiliary-resources.json)

同梱説明書では起動時に Style をランダムに選んで試聴を始める。パラメーターを変更すると先頭から再生し、Band の変更では再生位置を維持する。Shape は Groove Level の推移に対応し、Re-Compose はパターン、コード経路、バリエーションを再選択する。現在の素材フォルダーには Style 187件、Chordmap 27件がある。説明書では EXE と同じディレクトリに置く。 [StylePlayer.txt](../../../../../work/producer/style-library/StylePlayer.txt)

追試では初回の選択値と進捗、Style と ChordMap のリストを開いた状態、単一 Band と複数 Band、Shape・Length の変更前後、Intro・End、Re-Compose、Play・Stop、Motif の有無を撮る。各切替の再生位置と音、進捗バーの動き、キーボード移動を記録する。2026-10-07 の原版起動試行は Computer Use の承認待ちでタイムアウトしている。 [過去の試行記録](../../../../../work/analysis/q3-style-player/20261007T022658Z/original-launch-block.json)

## ROLE01　インストール済み Producer

Windows の32-bit アンインストール登録に **Microsoft DirectMusic Producer** があり、旧式の InstallShield エントリからインストール先の Uninst.isu と Uninst.dll を参照する。インストール先の Producer はバージョン 5.3.0.900 で、保持した未修正 EXE とはハッシュが異なり、既存の起動修正の記録と一致する。今回の確認はファイルと登録情報の読み取りまで。インストール画面と削除ウィザードは未観測。 [導入と起動修正の記録](../../../README.md#この環境のインストール済み版) · [旧起動警告の画面記録](../producer-startup-dialog.md) · [現在のファイルと登録情報](../../../work/original-gui/UI13-14-transport-aux/20261111T083323468666Z/installed-and-processes.json)
