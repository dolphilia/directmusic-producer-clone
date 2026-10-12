# Tempo Properties と空拍への挿入（2026-10-09）

0735限定採用を保持。0401候補は232保存／作業ソース・4生成物一致、関連34合格／4凍結／0失敗と4製品driverが成立。本体のF11／右click Properties、選択追従、190.25 BPM・Beat／Tick移動、3段UndoRedo、空拍Insert120 BPM・137.75への編集、native保存、別process復元、二通常exit0を実施した。独立GUI6／native6は合格。全体完了と原版外観・操作比較は未達。

一次記録は [0401限定記録](../../work/analysis/ui-tempo-properties/20261009T035721816Z/unit-0401-limited-v1.json)、[GUI解析](../../work/analysis/ui-tempo-properties/20261009T035721816Z/gui-proof-v1.json)、[native解析](../../work/analysis/ui-tempo-properties/20261009T035721816Z/native-proof-v2.json)。native解析v1は監査器のフィールド誤指定を保存し、watcherの実フィールドforcedTerminationへ訂正したv2が合格。Tempo3イベント以外のノート8件、拍子、Band、埋込AudioPath、unknown bytesは保全された。ProjectはSongの書込日時／サイズのみ変化し、別process用コピーは同一hash。

[固定WASAPI解析](../../work/analysis/ui-tempo-properties/20261009T035721816Z/audio-proof-v1.json)は不合格。2回目Stopの保守的区間が6.328秒で、上限6.3秒を越えた。録音・操作時刻・解析器を保持し、閾値は変更していない。音声の合格証拠は未成立。旧候補0239の成功を転用しない。

HelpのTempo Propertiesは1..1000 BPM・2小数、Measure／Beat／Tick、共有modeless表示を根拠とする。Help画像sgtempo.gifと原版TempoManager／Stripの復元コードを参照したが、原版実GUI64100706は既存approval timeoutで凍結。原版Timeline全体、font／color／icon／配置、負Tick／pickup、zoom／scroll、drag／OLE／clipboard、多選択等は比較未確認・未実装を残す。200%表示設定は変更していない。

実GUIでCtrl+AがEdit全文選択にならずBeat1への入力が12になった。無効座標として拒否・復元できたが操作不足のため、0430でProperties Editの全文選択を修正した。F11が同じTree選択へfocusだけ戻した場合と、Shift+F10でTempo listを開いた場合の対象判定も修正した。[修正記録](../../work/analysis/ui-tempo-properties/20261009T035721816Z/keyboard-correction-start-v1.json)に、記録セル失敗後に実装した順序上の誤りと修正前ソースを残した。

0430は232保存／作業ソース・4生成物一致、関連34合格／4凍結、4製品driver成立。本体でName／BPM／Beat／TickのCtrl+A置換、同じTree選択でのF11、TabでTempo listへ戻した後のShift+F10を確認した。191.50 BPM・2:3:48（4656 clocks）への編集、3段UndoRedo、native保存・別process復元・二通常exit0が成立。[0430限定記録](../../work/analysis/ui-tempo-properties/20261009T035721816Z/unit-0430-limited-v1.json)、[GUI7](../../work/analysis/ui-tempo-properties/20261009T035721816Z/keyboard-0430/gui-proof-v1.json)、[native6](../../work/analysis/ui-tempo-properties/20261009T035721816Z/keyboard-0430/native-proof-v1.json)に新候補自身の証拠を保存した。SongのTempo以外はbyte不変、ProjectはSongの書込日時／サイズだけ変化した。

0430の64秒録音は[固定解析不合格](../../work/analysis/ui-tempo-properties/20261009T035721816Z/keyboard-0430/audio-proof-v1.json)。初回Stopの保守的上限は6.103秒、再開後は6.905秒で固定上限6.3秒を超えた。context引継ぎ中に操作なく終了した別録音も未資格として保存した。閾値を変更せず、音声対照試験は未実行。通常終了時のGUI再取得エラーはwatcherのexit0／forcedTermination=falseと区別して保存した。

Tempo単位を原版比較・音声未達のまま保持し、次は別責務UI09のLyric時刻付き文字列とProperties／track入口・拍位置へ進む。原版比較の障害、inventoryComplete=false、fullAcceptance=falseを維持する。
