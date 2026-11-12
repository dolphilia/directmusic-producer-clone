# SEG-14　Wave Track の波形、レイヤー、Variation

2026-11-11。DirectMusic Producer 5.3.0.900 の調査用コピーで FarmGameProject を開いた。既存の **SfxCow.sgp** で Clock Time の表示を調べ、新規の **Segment1.sgp** で音楽時刻の Wave Track を編集した。親ウィンドウは1280×752 px。View は Project Tree、Standard Toolbar、Transport Controls、Status Bar を表示。画像中の矢印と青い光は操作ツールの表示。 [実行記録](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/run.json) · [原版画像47枚の対応表](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/published-wave-editing-figures.json) · [同梱ヘルプと音源](seg-14-wave-track.md)

## SfxCow の表示、選択と折りたたみ

Project Tree で **SfxCow.sgp** を展開し、子ノードの **SfxCow** をダブルクリックした。グループ1、PChannel 12 の Part が展開済みで、レイヤー **a** に青い波形が1件ある。短い Segment なので初期の倍率では右側の Variation ボタンが表示領域に収まらない。下端の横ズームを上げると波形が横に伸び、2段のボタン列が順に現れる。SfxCow では Segment の右端に折りたたみボタンがあり、倍率を上げた状態でも1段目の末尾に重なった。

波形をクリックすると項目名と波形が赤、Part の背景が薄黄色になり、赤い縦線はクリック位置に移る。ヘッダーを選ぶと波形が青に戻った。ストリップ右端のマイナスを押すと1行に縮み、青い波形は残り、名前・レイヤー・Variation の表示が消える。右端の四角で再展開できた。

![既存 SfxCow を開いた直後。幅が短い展開済み Part](figures/wave-04-cow-editor.jpg)
![横ズーム後。Variation の2段と青い波形](figures/wave-07-cow-variation-all.jpg)
![波形を選択。赤い波形と薄黄色の Part](figures/wave-08-cow-selected.jpg)
![折りたたんだ SfxCow の Part。波形と右端の展開ボタン](figures/wave-20-collapse-cow.jpg)

## 項目と Part のプロパティ

SfxCow の波形を選び **F11** を押すと、391×200 px のモデルレス **Wave Properties** が開く。Wave、Loop、Performance、Variations の4タブ。選択を Wave からヘッダーへ移すと同じ位置で **Wave Track Properties** に切り替わった。

| SfxCow の項目タブ | 観測した表示 |
| --- | --- |
| Wave | FarmGameProject: SfxCow。Start と End は Min・Sec・Grid・Ms 表示で Grid は灰色。End は1秒448 ms。Length は63,870サンプル、Source は63,868。End と Length の横に鍵。下端で Offset By の一部が途切れる |
| Loop | チェックなし、入力欄は灰色。Samples は Start 0、End 63,867、Length 63,868 |
| Performance | Attenuation と Fine Tune の値・範囲はすべて0。Don't Cut Off、Loop Independent of Segment は未チェック |
| Variations | 1〜32がすべてチェック済み。8列×4段で表示 |

Wave の参照リストには SfxAlarm、SfxCougar、SfxCow、SfxRooster、SfxSheep、SfxWolf の6件がプロジェクト名付きで並ぶ。Esc でリストを閉じると SfxCow の表示が保たれた。

![既存の Wave タブ。Clock Time の表示と Source の長さ](figures/wave-09-cow-wave-tab.jpg)
![FarmGame の Wave 参照候補6件](figures/wave-10-cow-reference-list.jpg)
![Loop オフ時のサンプル範囲と灰色の入力欄](figures/wave-12-cow-loop-tab.jpg)
![Performance の音量、音程、再生継続欄](figures/wave-13-cow-performance-tab.jpg)
![既存 SfxCow は全 Variation に所属](figures/wave-14-cow-variations-tab.jpg)

Wave Track Properties の **Wave Part** には Name **Wave**、PChannel **12 / SFX**、Attenuation **00.0 dB**、Play **in random order**、Lock ID **&lt;None&gt;** が出た。**Wave Track** の Attenuation も00.0 dB で、Variation に関する2つのフラグはオフ。**Flags** は Enable Play と Use Clocktime がオン。**Track Group Assignment** はグループ1の単一選択欄を持つ。

![Wave Part の名前と PChannel 12 / SFX](figures/wave-15-cow-header.jpg)
![Wave Track の Attenuation とフラグ](figures/wave-16-cow-track-tab.jpg)
![Flags の Enable Play と Use Clocktime](figures/wave-17-cow-flags-tab.jpg)
![単一選択の Track Group Assignment](figures/wave-18-cow-group-tab.jpg)

## 新規 Track への挿入と時刻・ループ編集

**Ctrl+N > Segment** で20小節の **Segment1** を作り、初回の **Add New Tracks** で **Wave Track** を選んだ。TimeSig 4/4 の下に、グループ1、PChannel 1 の Part が展開済みで加わる。先頭レイヤーと無名の行が見える。空の波形面の右クリックで **Insert** を選ぶと、第2小節の先頭に赤い **Empty** が入り、Wave Properties が開く。未割当時は Wave 欄が空で、End と Length は灰色。開始時刻の欄は **Bar・Beat・Grid・Tick** になる。

![Add New Tracks の Wave Track](figures/wave-26-select-wave-track.jpg)
![新規の Wave Track と空の Part](figures/wave-27-empty-track.jpg)
![空の波形面のメニュー。Insert と Add Wave Part(s) が有効](figures/wave-28-empty-insert-menu.jpg)
![参照未指定の Empty と音楽時刻の Start](figures/wave-29-empty-item.jpg)

Wave で **FarmGameProject: SfxCow** を選ぶと、赤い項目名と波形がすぐ描画される。Start は **2:1:1:0**、End と Source は **2:4:1:-79**。Start の Bar の上矢印を1回押すと波形全体が第3小節に移り、Start と End の Bar がともに3になった。Length の表示は63,882サンプルのまま。

**Loop** をオンにすると、開始・終了・長さと鍵が操作可能になる。Samples の End を下矢印で1減らすと End **63,866**、Length **63,867** となり、Start は0のまま。時刻欄の表示は同じだった。

![SfxCow 割り当て直後の Wave タブ](figures/wave-31-assign-cow.jpg)
![Start を1小節進めたあとの波形と時刻](figures/wave-32-cow-start-move.jpg)
![Loop オン直後。開始と終了の欄が有効](figures/wave-34-loop-enabled.jpg)
![サンプル単位で終端を1減らした状態](figures/wave-35-loop-end-step.jpg)

## 表示する Variation と項目の所属

項目の **Variations** タブで SfxCow の **1** だけを外した。この時点では他の Variation も表示中なので、波形は残る。Part 上端の **1** をダブルクリックして単独表示にすると、SfxCow が消える。番号列の左端の細いバーを押すと全表示になり、再度押すと全解除になる。全解除では波形が見えず、空セルの **Insert** と **Merge Variations** が灰色だった。

![SfxCow の所属から Variation 1 を外す](figures/wave-37-item-exclude-one.jpg)
![Part で Variation 1 だけを表示すると SfxCow が消える](figures/wave-39-isolate-one.jpg)
![Variation 全解除時は Insert が灰色](figures/wave-42-no-variation-menu.jpg)

## 無名レイヤー、波形のドラッグと Part の追加

**1** をクリックして単独表示にし、Project Tree から SfxSheep.wvp とその子ノードを無名行へそれぞれドラッグした。今回の2操作では項目は増えなかった。同じ行の第4小節で右クリックの **Insert** を実行すると、行名が **b** となり、**Empty** が入った。開いた Variations タブは1だけがチェック済み。Wave タブで **SfxSheep** を割り当てた。

![SfxSheep 子ノードのドラッグ操作直後](figures/wave-47-sheep-child-drop.jpg)
![無名行へ Insert。レイヤー b と Variation 1 の項目](figures/wave-49-layer-b-created.jpg)
![レイヤー b の第4小節に SfxSheep を割り当てた状態](figures/wave-52-sheep-assigned.jpg)

SfxSheep の波形中央から右へドラッグした1回目は、赤い縦線が押下位置へ移り、波形は第4小節の先頭に残った。続けて波形右側の境界付近から右へドラッグすると、波形全体が移った。Properties で Start **4:4:2:0**、Length **51,306サンプル**。移動前の Start は **4:1:1:0**、Length は同じだった。操作中のカーソル形状と、位置・長さ編集のヒット領域は追試する。

![波形中央からドラッグした直後。項目の開始位置は同じ](figures/wave-54-sheep-move-drag.jpg)
![右側からのドラッグ後。SfxSheep が右に移る](figures/wave-55-sheep-edge-drag.jpg)
![ドラッグ後の Start と Length](figures/wave-56-sheep-drag-result.jpg)

Variation 列の左端のバーで全表示に戻すと、レイヤー a の SfxCow と b の SfxSheep を同時に見られる。**b** の右クリックで **Insert Layer** を選ぶと、b の位置に無名の空行が入り、SfxSheep は下の **c** に移った。見えている波形レイヤーは2行分で、左の縦スクロールの下矢印を1回押すと c が現れる。

![全 Variation 表示で a と b に波形が見える](figures/wave-58-all-active-both-items.jpg)
![レイヤーボタンの Insert Layer と Delete Layer(s)](figures/wave-59-layer-b-menu.jpg)
![Insert Layer 直後。a の下に空行](figures/wave-60-insert-layer.jpg)
![縦スクロール後に c の SfxSheep が現れる](figures/wave-61-scroll-to-layer-c.jpg)

**Ctrl+Shift+Insert** で **Add Wave Part(s)** を開くと、初期値は未使用の **PChannel 2 / Strings**。**Create New Part** を押すと独立した空の Part が下に加わり、ダイアログは **3 / Pizz** に進んだ。番号を2に戻すと **PChannel is used** が出た。ダイアログを閉じ、ルーラーを第3小節付近から第5小節の途中までドラッグすると、ルーラーに濃紺の時間範囲ができた。見えていた SfxCow の波形は青のままだった。

![Add Wave Part(s) の初期値。PChannel 2 は未使用](figures/wave-62-add-part-dialog.jpg)
![追加済みの PChannel 2 を選んだときの表示](figures/wave-64-used-pchannel.jpg)
![2つの Wave Part。下の PChannel 2 は空](figures/wave-65-close-add-part.jpg)
![ルーラーの時間範囲と波形の表示](figures/wave-68-ruler-range.jpg)

## 保存と Producer の再起動

**Ctrl+S** で Segment1.sgp を保存した。Project を閉じると、表示操作に使った SfxCow.sgp に保存確認が出た。**No** を選び、終了前後のハッシュが一致することを確認した。最近使った項目から FarmGameProject を開き直し、Producer を通常終了した。旧ウィンドウの消失を確認して同じ実行ファイルを1回起動すると、FarmGameProject が自動で開いた。起動前後に実行パスとウィンドウを照合した。新プロセスの PID は採取できなかった。

![SfxCow.sgp の保存確認。No で閉じた](figures/wave-71-project-closed.jpg)
![Producer を起動し直した直後。FarmGameProject が自動で開く](figures/wave-75-restarted.jpg)
![Segment1 再読込。SfxCow と2つの Part](figures/wave-79-load-segment1.jpg)

Segment1.sgp では SfxCow のループ範囲と Variation 2〜32、レイヤー c の SfxSheep の時刻と Variation 1 が復元された。再起動直後はレイヤー a から表示され、下矢印で c に到達できる。View のチェックは基準の4項目だった。

![再起動後も有効な SfxCow の Loop。End 63,866](figures/wave-82-reopen-cow-loop.jpg)
![再起動後の SfxCow。Variation 2〜32](figures/wave-83-reopen-cow-variations.jpg)
![再起動後に縦スクロールして表示したレイヤー c と下の Part](figures/wave-85-reopen-sheep-visible.jpg)
![再起動後の SfxSheep。Variation 1 のみ](figures/wave-86-reopen-sheep-variation.jpg)
![再起動後の SfxSheep。Start 4:4:2:0](figures/wave-87-reopen-sheep-wave.jpg)
![再起動後の View メニュー。基準の4項目にチェック](figures/wave-89-view-baseline.jpg)

保存直後と再起動後の Segment1.sgp の SHA-256 は一致した。保存値では、Wave Track はグループ1、音楽時刻。Part は PChannel 1・2で、PChannel 2 は空。下表の物理・論理時刻と長さは DirectMusic のクロック値。項目の `DMRF` のファイル名・GUID は対応する Wave 文書と一致した。両 Wave 文書の `smpl` と `wsmp` のループ件数は0のまま。作業コピーでは FarmGameProject.pro が更新され、Segment1.sgp が追加された。既存の SfxCow.sgp と .wvp、および凍結入力21ファイルのハッシュは維持された。 [保存値の照合](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/wave-track-editing-audit.json)

| 項目 | レイヤー | 物理開始 | 論理開始 | 長さ | Variation マスク | Loop |
| --- | --- | ---: | ---: | ---: | --- | --- |
| SfxCow | a | 6144 | 6144 | 2225 | `0xFFFFFFFE` | 0〜63,866サンプル |
| SfxSheep | c | 11712 | 11520 | 1787 | `0x00000001` | オフ |

別の試行で、名前付近と波形端のドラッグ、Ctrl+A、End と Length の鍵、Offset By、Clock Time のループ範囲と発音を照合した。次の Wave 追試には、Tree ドロップが成立する条件、押下中のネイティブカーソル、同一レイヤー内の重なり、Wave 文書側でのループ編集が残る。 [Wave Track のドラッグ、端点、ループ音](seg-14-wave-track-followup.md)
