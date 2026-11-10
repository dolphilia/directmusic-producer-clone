# Segment File Save Asの別文書追加 — 2026-11-09

074348のFile SaveAsは別GUIDの新文書を追加し、元AName132.25/Peer120を保ち、コピーのみ137→Undo132.25→Redo137/保存を確認。関連5件493検査、作者10564/復元6724の二通常exit0、7native状態・17notes・source/Peer全bytes・copyのGUID/宣言Tempo以外・3文書Projectを照合。ただし最初PlayがLoad current DMSG 0x88781184で失敗し音声未達。録音120秒/時計4負例は保全し、同じ入力の再生は再試行しない。

原版19404とCHMの根拠に合わせ、saved SegmentのFile SaveAsだけをFramework.save_segment_copyへ接続。通常save_segmentの移動API/未保存の初回保存は維持。collision/保存失敗保全とsource独立履歴はcommon-openへ追加し、dispatcher hashを更新。最初構文C3538はログ保全後、別v2で通過。

音声は合格扱いにせず、native成功/録音時計成功と分ける。原版保存のsgdl/segh正規化は模造せず、未知bytesを保持。原版のSaveAs後Tempo disabled/dirty表示、他種別/別フォルダー/上書き等の全互換は未達。

一次入口：`work/analysis/ui-segment-saveas-copy/20261109T074237460Z/unit-record-v2.json`、`native-proof-v1.json`、`audio-failure-v1.json`。

074348の音声失敗から継続。無音声の条件変更診断でSegment生成/UNAM追加版/空UNFO除去版はS_OK。保存bytesを維持し、private loader copyから空の任意UNFOのみ除く修正を新候補で検証する。SDKヘッダー欠落の診断build失敗も保全。新候補の関連回帰/3文書GUI保存/二通常終了/別復元/137 PCMと負例/全native100節目へ進む。原版のdisabledTempo/dirty/layout厳密一致、他component/全外観/全40/全8/Q2/Style015244/49native・30driver凍結は未達のまま。 全体fullAcceptance=false、goal active。
