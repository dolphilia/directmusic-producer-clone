# ToolGraph 契約と未完了責務

対象は ToolGraphDesigner.ocx。原版 help toolgraphdesigner.htm／toolgraphdesignerwindow.htm／toolproperties.htm／managingtools.htm はtool順序、PChannel群、同一toolの複数instance、独立文書、Segment/AudioPathへ一個のembedded graph、tool固有properties/Param制御を定義する。標準palette toolは同梱されず、登録されたcustom toolのABIと実行時効果は残責務。

保存SDK work/analysis/sources/dmusicf.h:573-643 のDMTG→LIST toll→RIFF DMTL→tolhを実装。GUID16、signed LONG index、DWORD channel count、ckid/type、DWORD配列のheader32+4countを、countの割当て前に残長検証する。tool dataは省略可能。複数の一致payload/重複headerは採用前拒否し、未知payload/拡張header/padding/orderを保持する。補足根拠は[Microsoft SDK Tool header](https://documentation.help/DirectMusic/dmusiotoolheader.htm)、[Microsoft SDK InsertTool](https://documentation.help/DirectMusic/idirectmusicgraph8inserttool.htm)（SDK本文保存mirror）。

loadはsigned indexとDWORD値をそのまま所有し、Sequenceのbroadcast拒否規則をGraphへ無根拠に転用しない。channels編集はarray/countのみ、移動/削除はtool列とindexを一括更新。追加instanceはGUID入力と保存表現で、外部classを自動実行しない。空配列の実行時routing解釈は今後のABI試験で確認する。GUI表示は実番号で、1-based PChannel表記との原版照合は未完了。

原版サンプル23件のDMTG存在検索は0件。原版同一操作保存/再読込/実行は既存startup/window障害で未実施。自作SDKfixture成功を原版相互互換としない。Producer独自tool名/PChannel group設計chunk、paletteとproperties ABI、Segment/AudioPath embedded editing、runtime order/message routing/Param/録音、original comparisonは全対象に残す。

今回の単位はowned graph/順序/channelsとnative Project・本体保存復元。不正入力、履歴、disk/native Project復元を試験し、新候補GUI/回帰と別プロセス復元へ接続する。最新進行は[progress.json](../../work/analysis/q3-toolgraph/20261005T031700Z/progress.json)。全40/全8受入は未完了。

## 現行検証境界

候補 20261005T032253934Z、129保存ソース、build/install0、EXE a983677c4903f70b5419776986f8affb373dec67f5db4f837baba3be61f95bf5。専用32/core791/Script70と本体Graph/native Project保存・別プロセス復元/正常終了0。固定回帰47=30合格17障害、driver94=29合格11障害54未実行。詳細 [単位](../../work/analysis/q3-toolgraph/20261005T031700Z/report.md)。原版動的比較・source custom tool ABI/runtime/embedded graph/Paramは未完了。独立RIFF監査をその代替にしない。
