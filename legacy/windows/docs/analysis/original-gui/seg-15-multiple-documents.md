# SEG-15　複数 Segment の編集、再生対象、参照更新

2026-11-11。DirectMusic Producer 5.3.0.900 の調査用コピーで、FarmGameProject の **BGDawn.sgp** と **BGPredawn.sgp** を開いた。View は Project Tree、Standard Toolbar、Transport Controls、Status Bar を表示。親ウィンドウは1280×752 px。青い光と矢印は操作ツールの表示。 [実行記録](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/run.json) · [原版画像36枚の対応表](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/published-multi-figures.json)

## 同じ小節の Script と Segment Trigger

**Window > Tile Horizontally** で上段を BGPredawn、下段を BGDawn にした。双方の拍子は1/4、先頭テンポは100.00 BPM。保存してあった横ズームの違いで、同じ小節番号の画面上の位置はずれる。上段の第3小節にグループ1〜32の **Script**、下段の第3小節にグループ1の **Segment Trigger** がある。アクティブな子ウィンドウはタイトルバーの色が濃く、操作中のストリップは薄黄色になる。

上段の斜体 **dmSfxAlarm** を選び **F11** を押すと、**Script Routine Properties** に FarmMusic、Before Time Stamp、3:1:0 が出る。モデルレスのシートを開いたまま下段の **SSBird** をクリックすると、下段がアクティブになり、Transport は BGDawn、シートは **Segment Trigger Track Properties** のグループ割り当てへ切り替わった。上段には直前のセル選択と位置線が残った。下段の同じ項目をもう一度クリックすると **Segment/Motif Trigger Properties** に切り替わり、SSBird、Secondary、Controlling Segment、3:1:1 が表示された。

![Tile Horizontally 後。上段が BGPredawn、下段が BGDawn](figures/multi-09-tiled.jpg)
![上段の Script 項目と Script Routine Properties](figures/multi-11-script-properties.jpg)
![下段へ切替直後。Segment Trigger Track Properties とグループ1](figures/multi-12-trigger-properties.jpg)
![下段の同じ項目を再選択。SSBird の参照と時刻](figures/multi-13-trigger-item.jpg)

## Project Tree と Transport の再生対象

Project Tree の **SSBird.sgp** ファイルノードを選択すると、ファイルの Design と Runtime を持つ **Segment File Properties** が出た。子の **SSBird** 文書ノードを単一選択すると、**SSBird Segment Properties** と Transport の **SSBird.sgp: SSBird** に切り替わった。アクティブな編集文書は BGDawn のまま。SSBird の編集ウィンドウを開かずに、Transport の左の再生ボタンから開始できた。ボタンは停止用に切り替わり、背面の2つの Timeline の位置線はその場に残った。ここで確認した再生対象は Transport の表示とボタン状態で、出力音は採取していない。

![SSBird.sgp のファイル側の Properties](figures/multi-15-expand-ssbird.jpg)
![SSBird の文書ノード選択。アクティブな BGDawn と Transport の SSBird が同時に見える](figures/multi-16-ssbird-transport.jpg)
![SSBird の再生中。編集面は上下の背景 Segment](figures/multi-17-play-ssbird.jpg)

## 文書をまたぐ Clipboard と Undo／Redo

BGDawn の SSBird トリガーを選び **Ctrl+C**。BGPredawn の Script 行の空セルでは右クリックの **Paste** が灰色だった。

BGDawn の Tempo 行の第3小節に **Insert** で項目を作ると、初期値は **120.00 BPM、3:1:0**。この項目を Ctrl+C でコピーし、BGPredawn の Tempo 行の第4小節を右クリックすると **Paste** が有効になり、サブメニューに **Merge** と **Overwrite** が出た。Merge で上段の第4小節先頭に120.00が入った。下段の第3小節と、双方の第1小節にあった100.00が残る。

![Trigger をコピーした状態の Script 行。Paste は灰色](figures/multi-22-script-clipboard-menu.jpg)
![下段の Tempo に挿入。120.00 BPM と3:1:0](figures/multi-25-source-tempo-insert.jpg)
![上段の Tempo にフォーカスを移すと Paste が有効](figures/multi-29-target-tempo-paste-menu.jpg)
![Paste の Merge と Overwrite](figures/multi-30-tempo-paste-submenu.jpg)
![Merge 後。上段の第4小節と下段の第3小節に120.00](figures/multi-31-tempo-merge.jpg)

下段の Tempo 行にフォーカスを戻すと **Edit > Undo Insert Tempo** が表示された。これを実行してから、上段と下段で順にショートカットを試した。

| フォーカスと操作 | BGPredawn 第4小節 | BGDawn 第3小節 |
| --- | --- | --- |
| Merge 直後 | 120.00 | 120.00 |
| 下段で Undo Insert Tempo | 120.00 | 空 |
| 上段で Ctrl+Z | 空 | 空 |
| 下段で Ctrl+Y | 空 | 120.00 |
| 上段で Ctrl+Y | 120.00 | 120.00 |
| 上段を Ctrl+S で保存し、下段で Ctrl+Z | 120.00 | 空 |

![下段がアクティブなときの Edit メニュー](figures/multi-33-source-undo-menu.jpg)
![下段で Undo。上段には貼り付け結果が残る](figures/multi-34-undo-source.jpg)
![上段でも Undo。両方の試験項目が消える](figures/multi-36-undo-target.jpg)
![下段で Redo。第3小節だけに120.00](figures/multi-38-redo-source.jpg)
![上段でも Redo。各文書の項目が復元される](figures/multi-40-redo-target.jpg)
![上段を保存したあと、下段を試験前の状態へ戻す](figures/multi-43-restore-dawn-tempo.jpg)

## 参照名の即時更新と別エディターの開閉

Project Tree で SSBird の子を選び **F2** を押すとインライン編集枠が現れた。この回の文字入力と Enter では名前が変わらなかった。**F11 > Segment** の Name 欄で名前を選び、**SSBirdCheck** と入力して **Tab** で確定すると、子ノード、Transport のコロン以降、BGDawn の第3小節のトリガーラベルが更新された。ファイルノードの表記は **SSBird.sgp**。トリガーの Properties を開き直すと、参照欄は **FarmGameProject: SSBirdCheck**、時刻は3:1:1だった。

![SSBird 文書ノードで F2 を押したときの編集枠](figures/multi-45-rename-inline.jpg)
![変更前の SSBird Segment Properties](figures/multi-48-ssbird-name-properties.jpg)
![Name 確定直後。Tree、Transport、Timeline に SSBirdCheck](figures/multi-54-name-commit.jpg)
![同じトリガーの参照欄と時刻](figures/multi-55-renamed-trigger-reference.jpg)

BGPredawn の Script が参照する **FarmMusic** を Tree の子ノードから開くと、Script Designer が2つの Timeline の手前に別の子ウィンドウとして重なる。親タイトルは **FarmMusic.spp**、Transport には直前の **BGDawn** が表示される。Script Designer の閉じるボタンで、下段の BGDawn と選択済みのトリガーに戻った。共有の **FarmGame** Style も、子ノードから開くと別の子ウィンドウとして重なった。

FarmGame Style の子ノードを選び、**F11 > Style** の Name を **FarmGameCheck** に変更した。確定直後、BGDawn・BGPredawn の両方の **Styles** 行のラベルが新しい名前に変わり、Tree のファイルノードは **FarmGame.stp** と表示された。

![FarmMusic の Script Designer を開く。Transport は BGDawn](figures/multi-58-script-designer.jpg)
![Script Designer を閉じたあと。下段の BGDawn がアクティブ](figures/multi-59-return-after-script.jpg)
![FarmGame Style Designer が別の子ウィンドウとして開く](figures/multi-61-style-designer.jpg)
![変更前の FarmGame Style Properties](figures/multi-64-style-properties.jpg)
![共有 Style の Name 確定後。上下の Styles 行に新しい名前](figures/multi-67-shared-style-renamed.jpg)

## 保存確認、再起動とファイルの照合

Tempo を保存したあと、参照名の試験を行った状態で **File > Close Project FarmGameProject** を選んだ。保存確認は **FarmGame.stp → BGPredawn.sgp → BGDawn.sgp → SSBird.sgp** の順。すべて **No** を選んだ。BGPredawn の保存確認は、Ctrl+S で Tempo を保存してから参照名の操作を行ったあとに出た。

![最初の保存確認。FarmGame.stp](figures/multi-70-close-project-prompt.jpg)
![FarmGame.stp を No で閉じた次の確認。BGPredawn.sgp](figures/multi-71-discard-style.jpg)
![BGPredawn.sgp を No で閉じた次の確認。BGDawn.sgp](figures/multi-72-discard-predawn-after-save.jpg)
![BGDawn.sgp を No で閉じた次の確認。SSBird.sgp](figures/multi-73-discard-dawn.jpg)

Producer を通常終了し、旧ウィンドウの消失を確認して同じ実行ファイルを1回起動した。実行パスと別ウィンドウを照合した。起動時は Project が閉じた状態で、File の履歴から FarmGameProject を開いた。BGDawn の第3小節は Tempo が空でトリガー名は **SSBird**。両方の Style は **FarmGame**。BGPredawn の第4小節には保存した120.00が復元された。

![Project を閉じた状態から再起動。空の作業領域](figures/multi-75-restarted.jpg)
![再起動後の File メニューに FarmGameProject.pro](figures/multi-76-reopen-mru-menu.jpg)
![再読込した BGDawn。SSBird、FarmGame、第3小節の Tempo は空](figures/multi-79-dawn-after.jpg)
![再読込した BGPredawn。第4小節の120.00と FarmGame](figures/multi-80-predawn-after.jpg)

保存直後と再起動後の試験出力は SHA-256 が一致した。`tetr` に **2304クロック、120.00 BPM** が1件増えた。BGDawn の `DMRF` の SSBird と両文書の FarmGame は対象文書の GUID と一致し、保存した BGPredawn の FarmMusic と Script5 への参照も試験前と同一だった。

試験出力では、同一ペイロードの Marker の `MARK/play` が4組から6組に増え、Chord の `crdb` の11バイトにも差が出た。差が生じる操作の切り分けと再生への影響は追試する。試験出力を採取先に保全し、Project を閉じてから作業用 BGPredawn を試験前のバイト列に戻した。作業用ファイルの最終照合で試験前とのハッシュ差が残ったのは **FarmGameProject.pro**。凍結入力21ファイルのハッシュは一致した。 [ファイルと差分の照合](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/multi-document-audit.json) · [貼り付けを保存した試験出力](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/captures/multi-pasted-BGPredawn.sgp)

![試験前の BGPredawn に戻した作業用 Project。次に開く APFarm.aup がツリー先頭に見える](figures/multi-85-ready-audiopath.jpg)

Script 同士の別文書への Merge・Overwrite は後続試験で照合した。Script の入力グループ1・2は原版で双方1〜32となった。Tempo などグループを指定できるトラックの別グループへの貼り付け、同時再生時の優先順、参照先のファイル移動・欠落時の挙動は追試対象。 [Script Track の別文書への貼り付け](seg-14-script-track-cross-document.md)
