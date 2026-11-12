# UI12　ToolGraph Designer と PChannel Group

[調査計画](README.md) · [Container](ui-11-container.md) · [AudioPath Designer](ui-08-audiopath-designer.md) · [Parameter Control Track](seg-14-control-triggers-script.md)

図は DirectMusic Producer 5.3.0.900 の同梱 HTML Help。原版コピーでのペイン、グループ、内蔵版、保存、未登録 Tool の取り込みは [ToolGraph Designer の実機操作](ui-12-toolgraph-editing.md) に掲載した。 [図の出典と寸法](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/published-help-figures.json) · [登録の読み取り記録](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/toolgraph-dependencies.json)

## ToolGraph を開く入口

原版の [New Files の撮影](figures/harm-63-new-chordmap-selected.jpg) で `Toolgraph` が表示された。ヘルプによると、作成後または Project Tree の ToolGraph をダブルクリックしたときに Designer が開く。Segment の子ノードでは右クリックの `New > Toolgraph`、AudioPath の子ノードでは右クリックの `New Toolgraph` から空の ToolGraph を作成できる。Segment・AudioPath が内蔵できる ToolGraph はそれぞれ1つ。独立文書の Properties は `Graph` と `Info` で、Graph の Name はツリーに表示される。 [Managing Tools](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/managingtools.htm) · [Toolgraph Properties](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/toolgraphproperties.htm)

## 3つのペインと割り当て

1. 左に `PChannel Group`、中央に各 Tool の列、右に `Tool Palette`。ペインは幅を変更できると説明されている。中央は空のときにドロップ先を表示する。
2. 左の右クリックで `Insert PChannel Group`、既存グループで `Add/Remove PChannels`。右の Palette から中央へドラッグすると Tool の列が作られる。ヘルプでは同じ Tool を複数追加でき、名前には番号が付く。
3. Tool 列と PChannel Group 行が交わるセルをクリックして有効化する。図では有効セルが青。メッセージは有効な Tool を左から右の順に通る。Tool の列見出しと、割り当てを切り替えるセルで操作対象が異なる。Tool 固有の設定は列の右クリックから Properties、`Edit Tool Properties`。
4. Parameter Control Track の `Tools` から、対象の Tool とパラメーターを選ぶ入口がある。 [Toolgraph Designer Window](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/toolgraphdesignerwindow.htm) · [Tool Properties](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/toolproperties.htm) · [Add Parameter](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/toaddaparametertotheparametercontroltrack.htm)

![原版ヘルプの ToolGraph。左にグループ、中央にドロップ先、右に Palette](figures/toolgraph-help-01-designer.gif)

![原版ヘルプの ToolGraph。青いセルがグループごとの有効な Tool](figures/toolgraph-help-02-channel-assignment.gif)

## 現在の登録と追試用の入力

`ToolGraphDesigner.ocx` にある `Software\Microsoft\DirectMusic\Tools` を読み取りで調べた。HKLM・HKCU と32ビット・64ビットの各ビューでキーが見つからなかった。32ビットの Producer の Components 一覧には `Toolgraph Designer` と `Container Designer` がある。ヘルプ図の Palette の例には Duration、Echo、Quantize、Time Shift、Transpose がある。今回、独立・内蔵の各 Designer で表示された Palette は空だった。 [登録の読み取り記録](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/toolgraph-dependencies.json) · [Toolgraph Designer](../../../../../work/analysis/original-manual-session/20261010T031624075Z/help-extracted/htm/toolgraphdesigner.htm)

独立・Segment 内・AudioPath 内の入口と、PChannel Group の追加・取消・再読込を撮影した。未登録2クラスを含む試験入力 [Graph.tgp](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/missing-tool/manifest.json) は、Project へ取り込まれたコピーで2つの Tool が消えた。登録済みの Tool を用意して Palette からの追加、セルの切替、列の順序、固有 Properties と Parameter Control を追試する。 [実機の操作と保存値](ui-12-toolgraph-editing.md)
