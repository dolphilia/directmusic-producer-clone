# TempoStripMgr 代替実装の比較

最新の追加は末尾の実ページ選択更新回帰。新候補00c3はシステムクリップボードを含まない6,914観測・445通常ファイル・123コピー用データ・66画像で一致した。候補690のシステムクリップボード4ケースは機能結果が一致したが、原版・候補ともTimelineのアンロード確認に失敗した記録を保持する。

後続の静的・動的調査で、原版TimelineのExportにアンロード用カウンターの追加増分と、破棄経路の直接減算の不一致を確認した。原版・候補とも4回のExport後、全所有者とOLEの解放後にもカウンター4を観測した。S_FALSEを生存COM参照の漏れとは同一視しない。[カウンターと寿命の調査](timeline-clipboard-lifetime.md) に14命令のバイト照合、異常終了と復元処理の修正、同一36ソースでの機能再比較を記録する。候補690の本体UIでもコピー・切り取り・貼付け、保存、Undo/Redo、プロジェクト再読込を確認した。元DLLへ戻した別プロセスでも候補保存物を開き、1・3小節目の112と音声出力を確認した。この候補のプロセス再起動と候補をロードした状態での音声出力は継続中。

更新日：2026-10-02。工程4は作業中。原版 Timeline と TimeSigStripMgr を接続し、境界位置・複数選択の編集、拍子変更通知、通知の登録・解除、イベント追加、明示したデータオブジェクトへのコピー・切り取り・貼付け、Undo 用操作名の取得、カーソル位置でのテンポ変更通知、範囲選択、非表示の実ウィンドウを使った横スクロールと描画20ケース、クリック選択13ケース、プロパティページ管理の接続、実ページの生成・入力・スピン通知、OLEの継続・中止コールバックと左ボタンドロップ、DoDragDrop境界を差し替えた開始・終了・セルフドロップ、右メニュー閉鎖7ケースと実OLE中止2ケースまで比較できた。貼付け・通知試験では原版 Timeline 内の TimeStripMgr も接続する。システムクリップボード連携、Undo 履歴、実OLEの成功転送、ドラッグ画像、右メニューのコマンド選択、本体でのページ接続とモジュール置換は未完了。

## ビルドと実行

```powershell
cmake -S . -B work/build/probes -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/probes --config Release --parallel 4
.\scripts\Run-ReferenceProbe.ps1 -Connected
.\scripts\Run-ReferenceProbe.ps1 -Connected -Candidate
node scripts/Compare-TempoDll.mjs <原版の実行ディレクトリ> <代替版の実行ディレクトリ>
```

Timeline 接続と編集まで比較する場合は、両実行の `-Connected` を `-Timeline` に置き換える。両方とも、ハッシュで識別した原版 `work/producer/app/Timeline.dll` の絶対パスをプローブへ渡す。代替 DLL の隣に原版をコピーする必要はない。Timeline のパス・版・ハッシュも run.json に記録する。

非0位置の編集と移動を含める場合は `-TimeSignature`、代替版では `-TimeSignature -Candidate` を使う。原版 TimeSigStripMgr に自作の4/4拍子データを読み込ませ、Timeline の長さを30720 clocksとする。初期拍子イベントは生成直後には存在しないため、DLL を接続するだけでは位置変換は成功しない。境界6ケースと拍子変更3ケース、通知登録のチェックも実行する。run 内の `sources/` に参照ソースをコピーし、ハッシュも保存する。

代替 DLL は `work/build/probes/Release/TempoStripMgr.dll`。元の `work/producer/app/TempoStripMgr.dll` は上書きしていない。登録せず、同じ `com_probe.exe` が指定 DLL のクラスファクトリーを直接呼ぶ。PE は x86。公開シンボルは DllGetClassObject、DllCanUnloadNow、DllRegisterServer、DllUnregisterServer で、登録・登録解除は現時点で `E_NOTIMPL`。

主要なソース：

- `src/tempo/tempo_track.{h,cpp}`：イベント順序、通常の `tetr`、編集側のテンポ検索。
- `src/tempo/tempo_dll.cpp`：クラスファクトリー、非集約オブジェクト、参照管理、StripMgr、IPersistStream、Framework・Timeline・ランタイム接続、strip と限定的な編集操作。
- `src/compat/strip.h` / `timeline_services.h`：strip・TimelineEdit の ABI と、原版 Timeline を呼ぶ復元済みスロット。
- `src/compat/host_services.h`：復元した Framework のスロット19と PersistInfo のスロット4だけを呼ぶアダプター。
- `tests/native/com_probe.cpp`：原版と代替版に同じ入力を与えるネイティブ試験。
- `scripts/Compare-TempoDll.mjs`：実行条件・プローブ同一性・ケース網羅を確認し、ログとバイト列を照合する。

## 確認済みの結果

| 項目 | 証拠 |
| --- | --- |
| 原版の実行 | `work/reference/tempo/20261002T070346148Z/` |
| 代替版の実行 | `work/candidate/tempo/20261002T070351228Z/` |
| 照合結果 | `work/comparison/tempo-dll/20261002T070406250Z/comparison.json` |
| 原版 DLL SHA-256 | `bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95` |
| 代替 DLL SHA-256 | `9d73a68d5460571aa63e02d78b3fa65da261a35b51e14ca7a0e74159a192d12d` |
| 共通プローブ SHA-256 | `638930e30fd16da807278c1b1e7288229957bb03825a47e684ba421ecec2b519` |
| プロセス終了 | 両方0、タイムアウトなし |
| API 等の観測レコード | 6887件一致、差分0 |
| 通常の入出力ファイル | 443件すべて SHA-256 一致。WM_COMMAND 11ケースの33件、ドロップ24ケースの96件、開始境界・実中止14ケースの56件を含む |
| コピー用データ | 123件は24バイトレコードの未初期化領域だけを正規化して一致。元バイト列・ハッシュと内容一致を別々に記録。コピー用データの未初期化領域までバイト一致したという意味ではない |

確認内容は、非集約生成、IUnknown 同一性、StripMgr / PropPageObject / IPersistStream の取得、初期状態、接続プロパティ、通常データの保存・読込、テンポ取得、実ランタイム同期、Timeline の strip 登録・委譲・列挙、選択、時刻0の100→164.25 BPM変更、削除、非0位置の118→133.5 BPM変更と移動、切断・解放。Timeline、TimeSigStripMgr、TempoStripMgr の3 DLL で DllCanUnloadNow が `S_OK` になる。

6ケースは単一137、小数93.75、複数、順不同、同時刻、単一100への置換。各イベント位置とその直後で編集側・ランタイムの値を調べる。実ランタイムは原版と代替版の両方で同じ Windows の `dmime.dll` を使用し、実ファイルのパス・版・ハッシュは run.json に記録する。実際に音を出す試験ではない。

strip は管理オブジェクトと別の IUnknown 同一性・参照数を持つ。管理オブジェクトが strip の初期参照を所有し、strip 側の管理オブジェクトへの参照は借用とする。管理オブジェクトの終了時にはこの借用参照を切る。接続プロパティ0の取得は IUnknown の基底位置に変換せず、原版と同様に Timeline インターフェースのポインターを AddRef して返すことを確認した。

## 編集状態の観測

両 DLL で次が一致した。

- strip 名は `1-32: Tempo`。属性1は VT_BOOL 値1、属性6/8/9は VT_INT 値20、属性7/10は VT_BOOL 値0。
- 未選択時は CanDelete が `S_FALSE`、CanSelectAll が `S_OK`、GetData は `S_OK` と null。
- 選択後は CanDelete が `S_OK`。GetData の借用データを複製し、テンポだけ変更すると保存・編集側・Timeline・ランタイムへ反映される。
- 変更後と削除後の IsDirty は、今回の接続条件では原版・代替版とも `S_FALSE`。文書全体の変更有無はこれだけでは判定できない。
- 削除後の保存は12 bytes、編集側 GetParam は `S_FALSE` と120 BPM。ランタイム GetParam は `0x88781161` で失敗し、編集側の既定値と同じ結果にはならない。
- 削除後は CanDelete / CanSelectAll が `S_FALSE`、GetData は `S_OK` と null、SetData は `S_FALSE`。

初期の限定試験では HWND を伴う UI と Draw を呼んでいなかった。後続の実ウィンドウ・描画・クリック選択の比較は末尾の節を参照。追加試験は OnWMMessage に WM_LBUTTONUP を直接渡し、挿入位置を設定する限定経路を含む。実画面でのマウス操作ではない。モデル単体の24件比較は `work/comparison/tempo/20261002T021855016Z/` に追加実装前の成功を記録した。直前の `20261002T020947860Z` は起動拒否だったが、同じ実行ファイルで再試行すると実行できた。

## イベントの追加

TimelineEdit の CanInsert / Insert と、選択状態、保存・再読込、編集側・Timeline・実ランタイムのテンポを比較した。拍子は4/4、長さ30720 clocks。Timeline の表示倍率は SetTimelineProperty(8, VT_R8=0.125) で明示する。

| ケース | 観測した結果 |
| --- | --- |
| 位置未設定 | CanInsert は S_FALSE、Insert は E_FAIL |
| empty / between | 時刻900に対応する位置への追加は時刻768、120 BPM。新規イベントを選択する |
| occupied | 時刻768に150 BPMがあると CanInsert は S_FALSE。それでも Insert を直接呼ぶと同時刻の後ろに120 BPMを追加する |
| occupied_tick | 時刻900に150 BPMがある場合も CanInsert は S_FALSE。直接 Insert は時刻768に120 BPMを追加し、900のイベントを保持する |
| later_measure | 時刻4708に対応する位置への追加は4608、120 BPM |
| negative | x=-1では S_FALSE / E_FAIL、保存イベントを変更しない |
| beyond_length | 時刻40000に対応する位置への追加は39936。移動処理と異なり、曲の長さ30720へ制限しない |

7ケースとも再読込後の保存バイトが一致した。マーカー1・2は追加後に0と観測したが、非0マーカーからの解除や既選択状態の維持・解除は今回の入力では確認していない。0テンポの内部プレースホルダー、ドラッグ中のボタン解放、プロパティ画面の生成・表示は未対応。

表示倍率を明示する前の比較 `work/comparison/tempo-dll/20261002T023506895Z/` は、76ファイルが一致した一方、X座標に6件の差が出た。原版 Timeline の倍率設定をプローブへ追加して両方を再実行すると全項目が一致した。座標レコードは除外していない。未指定倍率が異なった原因までは確定していない。

## 音楽上の位置と移動

コピー・切り取りと Undo 操作名の確認範囲は後述の専用節を参照。

4/4拍子での結果。measure・beat は0始まりの API 値である。

| 入力時刻 | GetData の measure / beat / tick | measure を1増やした後の時刻 |
| --- | --- | --- |
| 768 | 0 / 1 / 0 | 3840 |
| 900 | 0 / 1 / 132 | 3972 |
| 4708 | 1 / 2 / 100 | 7780 |

各位置でテンポだけを変更し、保存・編集側・Timeline・実ランタイムへの反映を確認した後、小節フィールドを1増やして同じ項目を照合した。代替実装は1小節を3072に固定せず、Timeline のスロット13 / 15で変換する。後続では下記3種類への拍子変更も比較した。途中に複数の拍子イベントがあるケースは未比較。原版 TimeSigStripMgr は拍子の問い合わせに使用しており、そのランタイムトラックの同期は試験していない。

境界用プローブの初版は Smart App Control に起動を拒否されたが、拍子変更試験を追加した上記ビルドでは実行できた。セキュリティ設定は変更していない。原版・代替版で次の6ケースが一致し、各出力の保存・再読込も一致した。

| ケース | 確認結果 |
| --- | --- |
| 負の tick | 小節0・拍0・tick=-100への移動は時刻0になる |
| 曲末を超える小節 | 長さ30720に対し小節20へ指定すると30719になる |
| 途中小節で終わる曲 | 長さ1000に対し小節0・拍2へ指定すると1536になる。原版は小節で上限判定するため、時刻の単純な範囲制限とは異なる |
| 複数選択のテンポ変更 | 時刻0/768/1536を全選択しても、変更されるのは先頭のテンポだけ |
| 同じ位置への移動 | 先頭を既存の時刻768へ移すと、元からあった150 BPMの後に移動した118 BPMが残る |
| 順序が変わる移動 | 先頭を小節1へ移すと、時刻768/1536/3072の順になる |

位置とテンポを同時指定したケースでは、位置変更が優先されテンポ118を維持した。複数選択の GetData は先頭イベントと flags=2を返す。

## 拍子変更通知

位置をイベント内に保持し、Load と位置更新通知で再計算する実装へ改めた。拍子データを読み替えただけの段階では、原版と同様に GetData の小節・拍・tick は変わらない。Timeline 経由で拍子変更通知を送ると、保持した位置を基に絶対時刻を更新する。必要なら元の小節に収まる拍まで戻し、同じ拍にある他のイベントの tick+1へ調整する。処理後はランタイム同期と変更通知・再描画要求を行う。

入力時刻900/2304/2404/4708/7528に対し、次の保存結果が原版と一致した。各時刻のテンポ取得、Timeline 経由の取得、実ランタイムも照合した。

| 変更後の拍子 | 変更後の時刻列 |
| --- | --- |
| 3/4 | 900, 1536, 1537, 3940, 5992 |
| 3/8 | 516, 768, 769, 2020, 3304 |
| 5/4 | 900, 2304, 2404, 5476, 9064 |

4/4へ戻して位置更新通知だけを送ると、絶対時刻は維持し、GetData の位置を再計算する。3/8で516になった先頭イベントは、4/4では小節0・拍0・tick516となる。未対応 GUID の直接 OnUpdate は原版・代替版とも `E_FAIL`。

6種類の通知リストへの登録・逆順の解除も実装した。2種類については Timeline 経由の通知結果を検証し、6 GUIDすべてで登録の存在・重複なし・切断後の不在を確認した。登録チェックを追加した同じプローブが一度起動拒否されたが、上記の再試行では成功した。セキュリティ設定は変更していない。拒否記録は `work/reference/tempo/20261002T021055436Z/code-integrity-events.json` に残し、成功記録と区別する。

## 保存したソースからのビルド

成功した原版 run の `sources/` には全 CMake 入力と比較スクリプトを保存している。作業中の `src/` を参照せず、新しい出力先で構成・ビルドできた。

```powershell
cmake -S work/reference/tempo/20261002T021855247Z/sources -B work/build/repro-20261002T021855247Z -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/repro-20261002T021855247Z --config Release --parallel 4
```

両コマンドの終了コードは0、MSVC 19.44.35228.0、Windows SDK 10.0.26100.0。tempo_core、TempoStripMgr.dll、com_probe.exe、tempo_core_compare.exeが生成された。この再構成先の実行ファイルを改めて動作比較した結果ではなく、保存したソースの構成・ビルド可能性の確認である。Producer 全体のクリーンビルド達成を意味しない。

比較から除くのは、配置が実装ごとに変わるオブジェクト内位置・仮想関数テーブル RVA と、限定ホストの内部コール列 `fixture_*`。元ログにはすべて残す。内部コール列の除外には、代替版が Conductor 探索をまだ実装していないという差が含まれるため、ホストサービスの呼び出し順まで互換とは判断しない。戻り値、取得値、保存バイト、IUnknown 同一性、解放結果は除外していない。

## プロパティで追加確認した契約

原版 RVA `0x5a96` / `0x5bd9` と、上記の動的試験を照合した。

| プロパティ | 観測値・動作 |
| --- | --- |
| 0 / 1 / 2 の未接続取得 | `E_FAIL`、`VT_UNKNOWN`、null |
| 3：トラックヘッダー | 32 bytes。ランタイム CLSID、位置0、group bits `0xffffffff`、`tetr`、list type 0 |
| 3 の設定 | 正しい BYREF と非 null を検証するが、指定した group bits 8 は保存されず、再取得は `0xffffffff` |
| 4 | 初期値 `0x18`。設定値は `0x11c58` でマスクされる |
| 5 | 初期値0。32ビット設定値を保持する |
| 6 | `VT_I4`、`0x11c58` |
| GetProperty の出力 null | `E_POINTER` |
| プロパティ1へ VT_EMPTY を設定 | `E_INVALIDARG` |
| 未知のプロパティ7を取得 | `E_INVALIDARG` |
| テンポ GetParam の出力 null | `E_POINTER` |

## 未対応と既知の差

1. strip の Draw は横スクロールを含む20ケースを実装・比較したが、マウス操作後の代表イベント切替は未比較。StripFunctionBar、Undo 履歴は未実装。SetStripProperty の選択関連2/3/4は通常経路を実装したが、ドラッグ中の抑制と本体上の実ページの表示更新は未検証。Copy / Cut / Paste / CanPaste は非 null の TimelineDataObject を渡す経路を実装し、システムクリップボードを使う null 引数経路は未実装。OnWMMessage は WM_LBUTTONDOWN / UP の通常クリック、Ctrl追加、Shift範囲、空拍の選択を実装。Ctrlで既選択イベントを押すと始まるOLEドラッグ、移動中の処理、プロパティ表示は未実装。表示名は現在の全グループ指定に限る。
2. PropPageObject の GetData / SetData は非0位置と移動まで実装し、境界・複数選択を比較した。途中に複数の拍子イベントがある場合、長大データや時刻のオーバーフロー、変換失敗時の部分更新は未比較。変換失敗時は代替版の拍子再配置を確定せず、エラーを返す。ShowPropertiesのTimelineへの引渡しとページ管理の対象切替・解除は実装済み。実ページ生成と画面内の直接入力は末尾の範囲で実装・比較済み。本体のページ更新・入力配送は未検証。
3. Timeline の変更通知（21 / 42）、再描画要求（17）、6種類の登録・解除（40 / 41）、3種類の OnUpdate の通常編集経路を実装。登録・重複なし・切断後不在は比較済み。残る3種類の通知処理と、テンポ変更通知で再生中に送るメッセージは未実装。Undo 用操作名の取得は比較済みだが、通知網全体と履歴管理の互換性は未確認。
4. Framework のストリーム生成と PersistInfo は使用するが、Conductor 探索と再生系サービスは未対応。
5. COM 集約は `CLASS_E_NOAGGREGATION`。原版の集約クラスとの比較、LockServer の詳細な比較、複数オブジェクト・複数文書は未実施。
6. ストリームは単一 `tetr`・16 bytes レコード・正の有限テンポに限定。未知のチャンク、拡張レコード、負値・0・非有限値は未対応。読込には暫定64 MiB制限がある。
7. 未接続 Load は原版のアクセス違反を再現せず `E_UNEXPECTED` を返す。不正入力の状態保持や PersistInfo 失敗時の処理も、原版との異常系比較は未実施。
8. DllRegisterServer / DllUnregisterServer は未実装で、本体への登録・置換は行っていない。現在の成功を工程5の統合検証や Producer 全体の完了とは扱わない。

実際のプロパティページの生成・入力は末尾の範囲で比較した。次はドラッグ、Undo 履歴・残る通知処理と、原版本体の実行環境整備を進める。

## コピー・切り取りと Undo 用操作名

Timeline のスロット43で原版 TimelineDataObject を作り、選択境界を明示して Copy / Cut へ渡した。システムクリップボードの読書きは行っていない。

| ケース | 確認内容 |
| --- | --- |
| unselected | イベント未選択では CanCopy / CanCut は S_FALSE、Copy / Cut は E_UNEXPECTED。コピー形式は追加されず、元データは保持される |
| single | 時刻900・137 BPM、境界開始900。コピーは相対時刻132・tick132の1件。元データ保持、切り取り後は空トラックへ同期 |
| multiple | 時刻900/2304/4708・137/150/93.75 BPM、境界開始900。相対時刻132/1536/3940、tick132/0/100 |
| early_boundary | 同じ3イベント、境界開始0。相対時刻900/2304/4708、tickは上と同じ |

CanPaste は空のデータオブジェクトに S_FALSE、コピー形式が入ると S_OKを返す。これは形式の受入可否だけで、貼付け実行の確認ではない。範囲選択だけでイベントが未選択の場合、部分選択、失敗する外部ストリーム、0テンポの内部項目は未比較。

コピー形式名は `Jazz v.1 Tempolist`。`tetr` のレコードサイズは24で、相対時刻 +0、double BPM +8、tick +16。原版の書込処理 RVA `0x6362..0x6383` は +4..7 と +20..23 を初期化しない。コピー元の実行で前者にスタック由来の値が入り、代替版は両方を0で保存する。比較はこの8バイトだけを正規化し、ヘッダーとその他すべてのバイトを照合する。元ファイルは変更しない。検証では、paddingだけの変更を許容する一方、時刻・BPM・tickの変更を検出し、16バイトなど異なるレコード形式を拒否することを確認した。

GUID `178633A6-4452-11D2-890C-00C04FBF8D15` は IsParamSupported で S_OK、GetParam の出力は呼出し側が SysFreeString する BSTR。次イベント出力は変更しない。初期値は空文字列、テンポ変更で `Change Tempo`、移動で `Move Tempo(s)`、追加で `Insert Tempo`、削除・成功した切り取りで `Delete Tempo(s)` となる。コピー、失敗した追加・切り取り、Loadだけでは直前の操作名を保持する。21回の観測が一致した。これは Undo の表示用情報であり、編集を元へ戻す履歴処理は未実装。

## 貼付けと時刻カーソル

コピー元は時刻900/2304/4708、137/150/93.75 BPM。原版 TimeStripMgr を接続し、各ケースで設定したカーソルを読み戻してから実行した。貼付け7ケースはすべて S_OK。選択された先頭イベント、操作名 `Paste Tempo(s)`、編集側・Timeline・ランタイムの取得結果、保存と再読込後のバイト列が原版と一致した。

| ケース | 条件と観測結果 |
| --- | --- |
| merge_empty | 境界開始900、カーソル900。拍頭768を基準にし、900/2304/4708へ貼付け |
| snap_existing | 同じ位置に貼り付けると900の既存90 BPMは137へ置換。0と1000のイベントは保持 |
| merge_duplicates | カーソル0。同時刻132の先頭だけ137へ置換し、2件目の95 BPMは保持 |
| overwrite | 境界900～6000、カーソル0、上書きモード。0～5100を両端込みで削除し、5101と6000は保持 |
| near_end | 長さ30720、カーソル30000。基準位置を補正し、26244/27648/30052へ貼付け |
| before_start | コピー境界開始3072、カーソル0。負になる相対位置を補正し、132/1536/3940へ貼付け |
| short_target | 長さ1000、カーソル900。補正後は132/1536/3940となり、全イベントを曲内へ切り詰める動作ではない |

TimelineDataObject のストリーム取得は形式の利用可能状態を消費する。入力の事前検査は別にコピーしたデータオブジェクトで行い、実行用は未消費のまま渡す。比較スクリプトは7ケース完了、TimeStripMgr の接続・解除、カーソル読戻し、事前検査後の S_FALSE と実行前の S_OK も必須にする。

不正・途中で切れたコピー用ストリーム、空形式、0テンポの内部プレースホルダー、整数オーバーフロー、処理途中の失敗による部分更新、プロパティ画面の表示は未比較。代替版は妥当性確認後に変更を確定するため、破損入力時の原版と同じ部分更新を保証しない。

## カーソル位置でのテンポ変更通知

GUID 1528EAB8-C518-11D2-B0E7-00105A26620B の OnUpdate を8ケース比較した。入力は先頭にダミー時刻−1234567、+8に新しいdoubleテンポを置く。変更位置には入力時刻を使わず、原版 TimeStripMgr を接続した Timeline のカーソルを使う。カーソルの設定値と読戻し値は毎回一致を検証した。

- 空トラック、先頭イベントより前では S_OK のまま変更しない。
- カーソル0では時刻0、カーソル1000では時刻900、カーソル30000では最後の時刻2304のイベントを変更する。
- 同時刻900に137と142 BPMがあると、最後の142だけを164.25へ変更する。選択の有無には依存しない。
- 同じテンポなら保存内容を変えない。入力nullはE_POINTER。
- Timelineの通知配送を通すケースでも171.5 BPMへの変更・保存・ランタイム同期が一致する。
- 選択状態と、Timelineプロパティ12の0/1が維持される。操作名は変更時にChange Tempoとなる。

原版単独の追加観測は20261002T032151973Z、代替版は20261002T032208076Z、比較は20261002T032220898Z。後続の範囲選択を含む冒頭の比較でもすべて通る。再生中のSegmentState・Performanceを伴うメッセージ送信、既存プロパティページの更新、0・負値・非有限テンポは対象外であり、この通知の全経路が完成したわけではない。

## 範囲選択のプロパティ

strip の SetStripProperty へ3（開始時刻）、4（終了時刻）、2（ガター選択）を渡す。3/4はVT_I4、2はVT_BOOLの0/1を使う。通常状態での直接COM呼出しを比較し、実画面のドラッグ操作はまだ試していない。

| ケース | 選択してコピーされた時刻 |
| --- | --- |
| 同じ拍内900～1000 | 768, 900, 1000, 1535 |
| 終了が次の拍900～1536 | 768, 900, 1000, 1535, 1536 |
| 小節をまたぐ3000～5100 | 2304, 3072, 4708, 5000 |
| 同じ開始・終了900 | なし |
| ガター選択無効 | なし |
| 逆順4708～900 | 0, 768, 900, 1000, 1535, 4708, 5000 |
| 開始−1、終了768 | 0, 768, 900, 1000, 1535 |
| 範囲選択後にガター選択解除 | なし |

開始−1は0として変換され、両端の拍は丸ごと含まれる。逆順の結果を空へ変更せず、原版の小節境界ごとの条件を保持した。各ケースで全選択を先行させ、以前の選択の解除も検査した。選択だけでは保存内容が変わらず、コピー内容と選択イベント削除後の保存・再読込・同期が一致した。型が不適切なプロパティ3と未知のプロパティ99はE_FAIL。ドラッグ中の選択変更抑制、ページ更新、Timelineのガター操作からの配送は未比較。

## メモリ上での描画比較

原版 `work/reference/tempo/20261002T033720095Z/` と代替版 `work/candidate/tempo/20261002T034049481Z/` の比較は `work/comparison/tempo-dll/20261002T034111577Z/comparison.json` に保存した。2654レコード、通常197ファイル、コピー用23ファイル、画像24枚が一致し、差分は0。両実行は終了コード0だった。

空、単一、小数テンポ、複数イベント、同じ拍内の複数イベント、全選択、同じ拍内の全選択、範囲反転、描画オフセット、クリッピング、縮小、別view値の12ケースを各2回描いた。640×20、上から格納する32ビットDIB、Arial高さ−12、非アンチエイリアス、透明背景モードを固定し、使用フォントのデータも保存・比較した。BMP全体を正規化なしで比較し、繰り返しの一致、DCのフォント・文字色・背景色・背景モードの維持、描画前後の保存内容不変も確認した。

描画オフセット192のケースでも、Timelineの表示開始マーカーは0のままである。実際に横スクロールした状態や左端の直前テンポ表示、本体UIの操作、マウス操作後の代表イベント切替はこの試験では未確認。描画全体の互換性が完成したとは扱わない。

## 範囲選択実装時の保存ソースからの再構成

貼付け・テンポ通知・範囲選択を含む代替版実行20261002T032636899Zのsourcesだけを使用し、以下の両コマンドが終了コード0となった。

```powershell
cmake -S work/candidate/tempo/20261002T032636899Z/sources -B work/build/repro-20261002T032636899Z -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/repro-20261002T032636899Z --config Release --parallel 4
```

MSVC 19.44.35228.0、Windows SDK 10.0.26100.0で、tempo_core、TempoStripMgr.dll、com_probe.exe、tempo_core_compare.exeの全4ターゲットが生成された。保存した23ソースのハッシュは現ソースと一致することも確認した。これは当該モジュールと試験プログラムの再ビルド確認であり、この別ディレクトリの生成物を使った再比較やProducer全体のビルドを意味しない。

## 実ウィンドウでの横スクロールと直前テンポ表示

最新比較は上表の実行。Run-ReferenceProbe.ps1 の -Windowed（代替版は -Windowed -Candidate）で再現する。このオプションは -TimeSignature の全試験を含み、com_window_probe.exe を選択する。非表示の親ウィンドウへ原版 Timeline の IOleObject を接続し、実在する子 HWND を確認してから描画する。ウィンドウ有無、DEP の実行時設定、表示開始時刻の読戻しも比較条件とする。

従来の12ケースに、スクロール4ケースと灰色の直前テンポ表示4ケースを追加した。192ピクセルで1536 clocks、200ピクセルで1600 clocksの表示開始を確認した。拍境界、拍途中、同じ拍の複数テンポ、選択状態を比較する。直前表示の4ケースでは、原版・代替版とも RGB(168,168,168) の画素が88、91、88、88個あり、繰り返し画像も一致した。同じ拍に120・130・140がある直前表示は140.00の斜体となる。灰色画素が0なら、画像同士が一致していても比較を合格にしない。

全20ケースを各2回描画し、画像40枚はBMP全バイト一致。ウィンドウの無効化、Close、サイト切断後も、既存の通知解除・参照解放・3 DLLのアンロード確認が通った。これは原版 Timeline を使う限定ホストでの検証であり、DMUSProd.exe の本体UI上の試験は未実施。

## 描画比較時の保存ソースからの再構成

代替版20261002T040059698Zのsourcesに保存した26ファイルは、比較時の作業ソースとすべてハッシュ一致した。次のコマンドを新規ビルド先で実行し、両方終了コード0を確認した。

    cmake -S work/candidate/tempo/20261002T040059698Z/sources -B work/build/repro-20261002T040059698Z -G "Visual Studio 17 2022" -A Win32
    cmake --build work/build/repro-20261002T040059698Z --config Release --parallel 4

MSVC 19.44.35228.0、Windows SDK 10.0.26100.0で、tempo_core、tempo_strip_manager、tempo_core_compare、com_probe、com_window_probe、timeline_window_probeの全6ターゲットを生成した。この別ビルドの生成物での再比較とProducer全体のビルドは未実施。

## クリック選択の比較

13ケースはfirst、second、beat_end、replace、control_add、shift_forward、shift_reverse、same_beat、empty_beat、collapse、empty_track、shift_from_empty、empty_then_select_all。押下・解放後の選択データ、編集可否、コピー内容、描画2回、選択による保存内容不変を比較する。empty_beatの後の追加とshift_from_emptyの後のプロパティ変更は、保存・再読込も照合する。

原版20261002T042155801Z・代替版20261002T042200666Zの比較20261002T042219651Zでは3197レコード、通常243ファイル、コピー用36ファイル、画像66枚が一致した。試験はStrip::OnWMMessageを直接呼び、WM_LBUTTONDOWN / UPを配送する。本体のウィンドウからの入力配送やOLEドラッグは含まない。

実装時には二つの差異を解消した。マーカー更新のコールバックによって選択が消える差異は、原版の再入抑制を再現して修正した。空拍を含む選択後の編集では、GetDataとSetDataが異なる選択イベントを参照することを確認し、SetDataの探索を修正した。空拍の内部イベントを保存やコピーへ混入させないことも検査した。

## プロパティページ接続の比較

冒頭の最新比較には、tests/native/property_page_probe.hによる40件の追加観測を含む。原版Timelineへ限定Frameworkを接続し、QueryInterfaceで限定プロパティシートを返す。非表示条件では管理オブジェクトを設定せず、表示条件では設定し、同じ管理オブジェクトを再使用することを確認する。シート側の設定失敗を与えてもShowPropertiesはS_OKを返す。

タイトル、null引数、IUnknown同一性、未対応IID、編集対象の切替・比較・解除、GetData失敗、解除通知、最終参照数を比較した。編集対象二つはともに参照数1を保持し、GetData呼出しは1回と2回、解除通知は各1回。シート設定は4回、表示状態問い合わせは5回。終了後にFrameworkの参照は0、限定シートの参照は元の1へ戻り、3 DLLともアンロード可能となった。比較器はこの網羅性と寿命管理も検査する。

限定シートはページ管理を受け取るが、実際のプロパティページを生成しない。したがって画面表示、コントロールからの入力、RefreshDataによる画面更新を検証した結果ではない。これらは次の実装・観測対象である。

最新の代替版sourcesに保存した28ファイルは現ソースとハッシュ一致した。次の構成・ビルドはともに終了コード0で、全6ターゲットを生成した。

    cmake -S work/candidate/tempo/20261002T043816216Z/sources -B work/build/repro-20261002T043816216Z -G "Visual Studio 17 2022" -A Win32
    cmake --build work/build/repro-20261002T043816216Z --config Release --parallel 4

使用したコンパイラーはMSVC 19.44.35228.0、Windows SDKは10.0.26100.0。この別ビルドの生成物での再比較と、Producer全体のビルドは未実施。


## 実プロパティページの比較（2026-10-02）

最新の上表の実行では、GetPropertySheetPagesから実ページを1枚生成し、非表示の子シートへ接続した。初期・単一選択・複数選択・未選択の表示、18入力（通常、同値、0、範囲超過、負値、空欄、非数値、数値接尾辞、空白、原点の負tick）、5スピン通知、PSN_RESETを比較した。テンポ欄のEN_CHANGE、全編集欄のEN_KILLFOCUS、スピンのUDN_DELTAPOSは実在するコントロールへ配送する。

実モデルへページを接続した5ケースは、時刻900・137.25 BPMを入力にし、テンポ145.5、0入力の補正1 BPM、2小節目への移動3972、3拍目への移動1668、tick42への移動810を確認した。選択データ、実ランタイム、保存バイト列、再読込後の再保存まで一致し、15ファイルを保存した。シート破棄・解除後の参照と3 DLLのアンロードも成功した。

範囲外テンポの0・1001・−3で原版だけがSetDataを呼ぶ差を、比較20261002T045503093Zに保存した。代替版は比較前に補正値を保持していたため通知を失っていた。変更有無を保持してから補正・送信するよう修正した。スピン処理未対応の5差分は20261002T045640178Zに保持し、RVA 0x49d6 / 0x51ef / 0x5189 / 0x4bbeを根拠に実装した。失敗結果は削除していない。

最終比較20261002T050337839Zは3529レコード、通常258ファイル、コピー用36ファイル、画像66枚が一致し、差分0。今回のページ操作ではコピーの正規化範囲を広げていない。比較器はページHWND、各ケースの網羅性、解除・参照管理とruntimeModulesの一致を検査する。完了レコードを意図的に除いたログが拒否されることもwork/comparison/page-coverage-validation-20261002/rejection.logに記録した。

保存した29ソースは現ソースおよびrun.jsonのハッシュと一致した。新規ディレクトリで次の構成・ビルドが終了コード0となり、全6ターゲットを生成した。

    cmake -S work/candidate/tempo/20261002T050322356Z/sources -B work/build/repro-pages-20261002T050322356Z -G "Visual Studio 17 2022" -A Win32
    cmake --build work/build/repro-pages-20261002T050322356Z --config Release --parallel 4

MSVC 19.44.35228.0、Windows SDK 10.0.26100.0。configure.logとbuild.logは上記ビルドディレクトリに保存した。これらの新規生成物での再比較は未実施。Producer全体のビルド成功を示す結果でもない。

今回の範囲は限定ホストに実ページを生成した直接メッセージ試験。画面上のキーボード・フォーカス配送、本体のページ配置、実モデルでのスピン入力、文書Undo、ドラッグ、音声出力は未検証。PSN_RESETは確定済みモデル値を戻さない観測であり、本体の取り消し履歴の検証とは区別する。次はドラッグ開始・中止とデータ所有権を調べる。

## OLEコールバック・左ドロップの比較

最新の上表の比較は、ドラッグ継続・中止20条件、GiveFeedback 5条件、データ受け入れと効果選択120条件、左ボタンドロップ17ケースを含む。tests/native/drag_probe.hは独立したIDataObjectとIEnumFORMATETCを用意し、TYMED_ISTREAMまたはTYMED_HGLOBALを提示する。原版はISTREAMだけを受け入れ、形式名が一致してもHGLOBALとQueryGetDataのS_FALSEを拒否する。入口からDragLeaveまたはDropまで参照が1→2→1に戻り、両インターフェースのIUnknown同一性も一致した。

17ケースはcopy、move、control、control_without_allowed_copy、snap、origin、near_end、short_target、negative、unsupported、no_effect、release_after_enter、unselected、single_selected、release_negative_over、unsupported_medium、query_sfalse。コピー元の通常イベントは900/137、2304/150、4708/93.75。コピー開始を768とし、対象に0/80、1668/95、5500/101を読み込む。倍率0.125のx=192では、137 BPMが1668、150が3072、93.75が5476へ入り、1668の95 BPMは置換される。カーソル100・貼付けモード1を設定してもドロップの位置には使用せず、操作後も値が変わらない。選択解除の有無はプロパティデータに加え、選択したイベントのコピー内容を比較する。

初回比較20261002T052449716Zは、保存値と同期は一致したが選択データに9差分があった。代替版が通常Pasteと同様に既存選択を解除していたため、Drop用の共有処理では既存選択を保持するよう修正した。失敗記録は保持している。媒体形式の初期調査は20261002T051403067Z・20261002T051515142Zに保存したが、HGLOBALのため有効な受け入れに達していない。ISTREAMに修正した原版20261002T051912109Z以降で効果選択を観測できた。

この作業単位の比較は4980レコード、通常326ファイル、コピー用70ファイル、画像66枚、差分0。ドロップ17ケースのsource / target / output / reloadの68ファイルはバイト一致、入力と選択状態のコピー34ファイルは既存の8バイトpaddingのみ正規化して内容一致。画像の追加はなく、従来の66枚が回帰確認された。各保存イベントで編集側・原版Timeline・実dmimeのテンポを確認している。Undo用の操作名も照合するが、文書Undo履歴を実行した結果ではない。

比較器は、追加ケースの順序・件数、インターフェース取得、参照数、ドロップ後のカーソル・モード、保存・再読込の完了を必須とする。また両run.jsonの30ソース一覧が一致し、sources内の全保存ファイルが記録ハッシュと一致することを要求する。work/comparison/drag-coverage-validation-20261002/{rejection.log,drag-rejection.log,source-rejection.log}に、ドロップ完了記録の欠落、ドラッグ完了記録の欠落、保存ソースの意図的な改変が拒否される結果を残した。検証用のコピーだけを変更し、元のrunは保持した。

## ドラッグ比較の保存ソースからのビルドと実行

作業ソースと同一ハッシュの30ソースを保存した代替版20261002T053100438Zから、次の新規構成・ビルドを行った。両終了コード0、全6ターゲット生成、MSVC 19.44.35228.0、Windows SDK 10.0.26100.0。configure.logとbuild.logはビルドディレクトリに保存した。

```powershell
cmake -S work/candidate/tempo/20261002T053100438Z/sources -B work/build/repro-drag-20261002T053100438Z -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/repro-drag-20261002T053100438Z --config Release --parallel 4
.\scripts\Run-ReferenceProbe.ps1 -Windowed -BuildDirectory work/build/repro-drag-20261002T053100438Z
.\scripts\Run-ReferenceProbe.ps1 -Windowed -Candidate -BuildDirectory work/build/repro-drag-20261002T053100438Z
node scripts/Compare-TempoDll.mjs work/reference/tempo/20261002T053143638Z work/candidate/tempo/20261002T053148670Z
```

Run-ReferenceProbe.ps1のBuildDirectoryを追加し、上記の新規生成物で原版・代替版を実行した。この作業単位の4980件比較はこのビルドから得た結果であり、新規ビルドの成功だけで再現性を判定していない。既定のBuildDirectoryはwork/build/probesで従来コマンドも使える。原版DLLの識別は固定ハッシュのままで、登録・原版ファイルの上書き・システムクリップボード変更は行っていない。

今回のMOVE(2)はドロップ先の交渉とイベント挿入を比較した結果。実際のドラッグ元の削除、選択済みイベントからの開始、WindowsのDoDragDropループ、右ボタンのドロップメニュー、セルフドロップは未検証。代替版の右ドロップメニューとドラッグ開始は未対応で、Producer本体の置換試験へ進める状態とは判定しない。次は開始ヘルパーとエクスポートデータ、中止時の参照解放を原版で観測する。

## DoDragDrop境界を使った開始・終了・セルフドロップ比較

最新の5905件比較は、tests/native/ole_drag_boundary.hの12ケースを追加した結果である。control_single_cancel、control_all_cancel、move_single_cancel、move_all_cancel、control_all_copy、move_all_copy、control_single_move、move_all_move、control_all_failure、self_single_move、self_all_copy、self_same_positionを実行する。各ケースで倍率0.125と横スクロール0を再設定し、同一の0/120、900/137、2304/150、4708/93.75を読み込む。試験の初期調査20261002T054242947Zは直前の短い曲による倍率変更を引き継いでいたため、位置の契約は倍率を明示した後続の結果を根拠とする。

原版・代替版のロード済みDLLで、名前がole32.dll!DoDragDropと一致するインポートのみを一時的に差し替える。PEの名前付きimportを解析して位置を得るため、代替版のRVAを固定しない。ページ保護を元へ戻し、最後にIATも元の値へ復元する。元DLLのファイルは変更しない。OSのループへ入る地点で、実際のIDataObject、IDropSourceと許可効果を観測し、外部転送のケースは指定した戻り値・効果を返す。セルフドロップ3ケースは、同じStripから実IDropTargetを取得し、実DragEnter / Dropを呼んでその結果を返す。

各開始中にQueryContinueDrag 20条件、GiveFeedback、IUnknown同一性、EnumFormatEtcの1形式と終端、ISTREAM/HGLOBALのQueryGetData、GetDataの初期カーソルと独立性を確認する。転送データをAddRefして保持し、ヘルパー返却後も同じバイトを取得でき、最後のReleaseで0になる。二回目のWM_MOUSEMOVEでは境界が再び呼ばれない。返却直後とボタン解放後の選択、選択コピー、保存・再読込、残った各イベントの編集側／Timeline／実dmime値、Undo用操作名を比較した。空になった場合はランタイム取得が失敗することも確認する。

ストリーム末尾位置の9差分は比較20261002T054935039Zに保存した。代替版がGetDataでSeek(0)していたためであり、原版と同じ書込終了位置で返すよう修正した。セルフドロップを追加した比較20261002T055528273Zでは、移動のUndo名、同位置の返却効果とその後のUndo名に3差分があった。開始位置の記録と同位置抑止、元イベントの標識、移動終了後のMove Tempo(s)を実装して解消した。MOVEのDrop直後の実ランタイムが以前の150 BPMを保持し、元削除後に137 BPMになることも追加観測した。

通常48ファイルはdrag-start-{case}-{input,returned,up,reload}.bin、コピー用24ファイルはdrag-start-{case}-{clipboard,selected-clipboard}.bin。これらを比較器に追加し、既存の326通常／70コピーから374通常／94コピーへ増えた。24-byteレコードのpadding8バイトだけを正規化する従来条件は変更していない。画像66枚は既存の描画・クリック選択回帰試験である。

## 開始処理の保存ソースからの再現

保存32ソースの代替版20261002T055947351Zだけを使用して新規構成し、全6ターゲットを生成した。構成・ビルドは終了コード0、警告・エラーなし。MSVC 19.44.35228.0、Windows SDK 10.0.26100.0、ログは下記ビルドディレクトリのconfigure.log / build.log。その生成物で得た原版20261002T060034989Z・代替版20261002T060040582Zが最新の比較対象であり、保存ソースは作業ソースとも32件一致する。

```powershell
cmake -S work/candidate/tempo/20261002T055947351Z/sources -B work/build/repro-drag-start-20261002T055947351Z -G 'Visual Studio 17 2022' -A Win32
cmake --build work/build/repro-drag-start-20261002T055947351Z --config Release --parallel 4
.\scripts\Run-ReferenceProbe.ps1 -Windowed -BuildDirectory work/build/repro-drag-start-20261002T055947351Z
.\scripts\Run-ReferenceProbe.ps1 -Windowed -Candidate -BuildDirectory work/build/repro-drag-start-20261002T055947351Z
node scripts/Compare-TempoDll.mjs work/reference/tempo/20261002T060034989Z work/candidate/tempo/20261002T060040582Z
```

比較器は開始12ケースの順序と完了、形式、保持データ、継続条件、二重開始抑止、返却直後／解放後の状態、差し替えの復元、セルフドロップ中のランタイム観測を必須にする。ソースに試験ヘッダーがあるWindowed実行では試験全体の欠落も許容しない。work/comparison/drag-start-coverage-validation-20261002のmissing-case、missing-restore、missing-self-runtime-rejection.logは、完了ケース、復元レコード、セルフドロップ中のランタイムレコードを除いて拒否された記録。変更したのは検証用コピーだけで、元runを保持した。

原版ファイルは試験後も固定SHA-256で一致する。3 DLLは終了時にアンロード可能となり、システムクリップボードや登録・物理入力は変更していない。実OLEループ、外部の受領先、ドラッグ画像、右開始と右ドロップメニュー、別ストリップ／別文書への移動、本体UI・文書Undo・音声出力は未検証。次は画像・右メニューと実OLEループの試験条件を整え、本体の実行環境へ接続する。今回の境界差し替えによる比較を、Producer本体の統合完了とは判定しない。

## 右ドロップメニューの閉鎖と実OLEループの中止

20261002T062859215Zが最新比較。6449レコード、通常410ファイル、コピー用112ファイル、画像66枚に差分なし。新しい試験は右メニュー閉鎖7ケースと実OLE中止2ケースで、通常36ファイル・コピー用18ファイルを追加した。保存33ソースだけから新規構成・全6ターゲットを生成し、そのビルドで原版20261002T062839769Zと代替版20261002T062845815Zを実行した。

右ドロップはallowed=0/1/2/3/4、非対応形式、負のXを入力する。原版DLLが組み立てた実HMENUを、名前付きuser32!TrackPopupMenuインポートの一時差替えで観測し、FALSEを返して閉じる。項目はMove(0x8026)、Copy(0x8028)、区切り、Cancel(0x8027)。Moveのみ直前の交渉効果にMOVEがない場合にGRAYEDとなる。非対応形式や負のXでもメニューが要求される。フラグ2、座標、所有ウィンドウの存在・同一プロセス・非表示も検査する。閉鎖後はS_OK、効果0、参照数1、保存内容不変で、選択・コピー・再読込・ランタイムも比較する。メニューを実際に表示した証拠ではなく、項目の選択・コマンド配送は未検証。

代替版にMENU/243と閉鎖処理を追加した。Timelineスロット30のプロパティ1はHDCであり、WindowFromDCで所有HWNDを取得し、ReleaseDCしてからTrackPopupMenuを呼ぶ。初回の代替版20261002T062322325ZはHDCをHWNDとして誤って扱ったためメニュー境界を通らなかった。修正後の記録を最新根拠とする。メニューコマンドからCOPY/MOVEへ反映する経路は引き続き未実装・検証待ち。

real_control_all_cancelとreal_move_all_cancelではDoDragDrop境界内でWindowsの実DoDragDropを呼ぶ。IDropSource代理は実ストリップを保持し、Windowsが呼ぶQueryContinueDragへescape=TRUEを渡す。試験自身のSTAスレッドへ別スレッドからWM_KEYDOWN/VK_ESCAPEをPostThreadMessageする。入力デバイスや他プロセスのキューは操作しない。Windowsの戻り値DRAGDROP_S_CANCEL、効果0、継続コールバック1回、代理の残参照1、転送データの終了後保持・最終解放0、選択・保存・再読込・ランタイムを比較した。物理入力、中止以外の実OLE転送、外部ターゲット、画像は証明していない。

入力配送なしの20261002T061843622Zとログflush追加後の20261002T062038363Zは15秒でタイムアウトした。WM_TIMERによる同スレッドの起床を試した20261002T062144928Zもタイムアウトした。別スレッドからの自身へのメッセージ配送で20261002T062305233Z以降は成功している。失敗run.jsonと途中ログは保持し、成功へ書き換えていない。代理・起床スレッドの寿命とログ順序は終了時のjoinで確定させる。

```powershell
cmake -S work/candidate/tempo/20261002T062717325Z/sources -B work/build/repro-right-menu-20261002T062717325Z -A Win32
cmake --build work/build/repro-right-menu-20261002T062717325Z --config Release
.\scripts\Run-ReferenceProbe.ps1 -Windowed -BuildDirectory work/build/repro-right-menu-20261002T062717325Z
.\scripts\Run-ReferenceProbe.ps1 -Windowed -Candidate -BuildDirectory work/build/repro-right-menu-20261002T062717325Z
node scripts/Compare-TempoDll.mjs work/reference/tempo/20261002T062839769Z work/candidate/tempo/20261002T062845815Z
node work/comparison/right-menu-validation-20261002/validate.mjs
```

新規ビルドのログはwork/build/repro-right-menu-configure.logとrepro-right-menu-build.log。MSVC 19.44.35228.0／SDK 10.0.26100.0、全6ターゲット成功、警告・エラーなし。比較器は実中止コールバック、右メニュー項目、インポート復元の記録を除いた三つのログを拒否する。負例の終了コード1と理由はwork/comparison/right-menu-validation-20261002/*-rejection.logとresults.jsonに保存した。同じ場所のsource-identities.jsonで両実行・ビルド元・現在の33ソースの同一性を検査した。原版ファイルハッシュ不変、3 DLLのアンロードS_OKも維持した。

次は右メニューのコマンド経路とドラッグ画像、本体への接続条件を調べる。右ボタンによる開始、実OLEでの正常なCOPY/MOVE、システムクリップボード、文書Undo、本体UI・音声と全体の再構築は未完了。工程全体の完了判定は変更しない。
## WM_COMMAND 11ケースの比較（2026-10-02）

最新の成功比較は原版20261002T070346148Z、候補20261002T070351228Z、比較20261002T070406250Z。6,887レコード、通常443ファイル、コピー用123ファイル、画像66枚に差分なし。WM_COMMAND の削除・挿入・全選択・プロパティ・右メニュー ID の単独配送・未知 ID を11ケース追加した。正常な終了と3 DLL のアンロード S_OK を確認した。

今回の成功比較は既存の `work/build/probes` の生成物による。候補の保存35ソースだけから `work/build/repro-command-20261002T070351228Z` を構成し、全7ターゲットのビルドは成功した（`work/build/repro-command-configure.log`、`work/build/repro-command-build.log`）。その新規生成物は Smart App Control が起動を拒否したため、クリーンビルドの生成物による6887件比較は未確認。拒否した実行は原版20261002T070707138Z/070814538Z、候補20261002T070710604Z/070817768Zとして保持した。本体のエラーとは区別し、[統合試験](tempo-integration.md) にイベントログとパスを記録した。

`work/comparison/command-validation-20261002/validate.mjs` は成功した両 run の35ソースとその時点の作業ソースをハッシュ照合した。コマンド完了境界、実行結果、選択コピー、コマンド区間のランタイム記録をそれぞれ欠落させた4ログを、比較器がすべて拒否することも確認した。記録は source-identities.json、results.json と4件の rejection.log。原版 TempoStripMgr のハッシュは不変。

挿入位置はコマンド引数の座標999を使わず、先行するマウス操作が保持した位置を使う。通知の上位ワードを持つ削除も一致した。Copy/Paste のシステムクリップボード、文書 Undo、本体からの入力配送、右メニューでの実選択と成功した実OLE転送は未検証。

## システムクリップボードの機能比較（2026-10-02）

候補DLL `69029024df3a082e14343cc8684d2baeb9162b3087fa8f310a2d29e3b5bdd733` はnull引数のCopy・Cut・CanPaste・Pasteを実装した。原版Copy RVA 0x8e5dは選択先頭時刻を拍へ丸めてコピー原点とし、Timeline slot43でデータオブジェクトを生成、data slot8で生の選択先頭・最終時刻を設定、slot3へストリームを追加、slot10でIDataObjectを取り出してOleSetClipboardへ渡す。manager+0x3cは成功したIDataObjectを保持する。CanPaste RVA 0x948dとPaste RVA 0xa7f2はOleGetClipboard、Timeline slot43、data slot9のImport、slot5の形式検査を経由する。manager終了RVA 0x714bはOleIsCurrentClipboardがS_OKの場合だけOleFlushClipboardし、保持参照を解放する。API名はPEの名前付きインポート、slotと引数は呼出し側の命令列で確認した。

`Run-ReferenceProbe.ps1 -Windowed -Clipboard -BuildDirectory work/build/clipboard` は明示した場合だけWindowsのクリップボードを使う。単一選択（time=900）、全選択、全選択の切り取り、上書き貼付けの4ケースを追加した。元データはtimes=0/900/2304/4708、BPM=120/137/150/93.75、貼付け先は6144。既存OLEオブジェクトを内容を記録せず保持し、試験側の更新以降に別の変更がなければ復元する。後続ソースは復元後のデータの確定も追加しているが、この最終版EXEの実行はブロックされ、動作確認前である。

同一probe SHA-256 `59e17ddb0a7171db437a9abc3257288e9929f26cd88435a1d7f8bc5c8781b3f4` と同一35ソースの原版 `work/reference/tempo/20261002T084855747Z`、候補 `work/candidate/tempo/20261002T084906659Z` を限定比較した。4ケースの戻り値・選択範囲・操作名・再読込が一致する。通常保存16ファイルは無正規化で一致。コピー4ファイルは24-byteレコードの既に解析済みのpadding +4..7/+20..23だけを正規化して内容が一致する。生のハッシュ・raw一致のfalseも `work/build/clipboard/feature-comparison.json` に残した。

両runはexitCode=1であり、全体試験成功へ書き換えていない。TempoのDllCanUnloadNowはS_OKだが、Timelineはmanager解放後もS_FALSEを返す。メッセージ配送とOleUninitialize後まで観測した原版 `20261002T085046178Z` でもS_FALSEで、原因は未確定。`Compare-SystemClipboard.mjs` は4ケースだけを判定し、fullRunsPassed=falseと終了時の観測を報告する。従来のCompare-TempoDllはこれらの失敗runを拒否する。参照管理の調査を終えるまで、この候補の全体回帰・終了条件を合格とは扱わない。

ビルドは新しいwork/build/clipboardでMSVC19.44／SDK10.0.26100、全7ターゲットが成功した。追加の終了処理を再ビルドしたEXEはSmart App Controlにより起動拒否（原版run `20261002T085159886Z`）。WindowsのCodeIntegrityイベント3077はcom_window_probe.exeの制御ポリシー拒否を示す。制御設定は変更していない。

```powershell
node scripts/Compare-SystemClipboard.mjs work/reference/tempo/20261002T084855747Z work/candidate/tempo/20261002T084906659Z work/build/clipboard/feature-comparison.json
```
## 選択後の実ページ更新回帰（2026-10-02追加）

候補690の本体試験で先頭イベントを選んでもMeasure 3が残ったため、実ページを表示したまま選択する回帰を追加した。3→1小節目、Ctrl追加、ボタン解放時の単一選択への縮小、空拍の5操作／押下・解放10段階を、選択モデルとページの実コントロールで照合する。試験からRefreshDataを補助呼出しせず、入力／出力ファイルの不変も検査する。

修正前候補 `work/candidate/tempo/20261002T103841783Z` は表示不一致で終了1。原版のShowProperties後のRefreshData呼出しを復元した新候補00c3と原版は、`work/reference/tempo/20261002T103949808Z`、`work/candidate/tempo/20261002T104000544Z` で同じプローブ・36ソースを使い両方終了0。比較 `work/comparison/tempo-dll/20261002T104031478Z/comparison.json` は6,914観測・445通常ファイル・123コピー用データ・66画像一致、差異0である。ページ接続の追加呼出しに応じて比較器の期待数も更新した。

この成功比較はシステムクリップボードを含まない（run.systemClipboard=false）。同機能の限定4ケース一致と原版Timelineのカウンター残存による全体失敗は、従来の別記録を保持する。新候補の本体選択更新試験と復元は [統合試験](tempo-integration.md) の末尾を参照。全機能の互換性や全体再構築の合格ではない。
