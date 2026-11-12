# DirectMusic Producer 原版 GUI 調査計画

開始日 2026-11-11。調査成果は、原版の画面と操作を後から再現し、実装と照合するための参照資料として使う。Segment エディターを最優先とし、アプリ全体の画面・操作も段階的に調べる。新製品への採用は、各挙動の根拠と製品側の採用判断に基づいて決める。

現在の再開点は **Pattern Part と Runtime の追加小節**。ピックアップと末尾の追加小節に別々の音を置き、Pattern の自然終了と本体に音を含めた場合を録音で比べる。Runtime 保存で音符と境界の情報がどう出力されるかも確認する。直前の直接 Transition では、保存済みの切替先 Boundary と Timing、明示指定、要求位置によるピックアップ音の差を8回の録音で比較した。初回の異常終了から復旧し、Producer は Project を閉じた状態で1件、標準4項目を表示している。 [Transition の Boundary・Timing とピックアップ](seg-10-transition-pickup.md)

- [UI01 View とツールバー表示](ui-01-view-toolbars.md) は表示前後、項目ごとの切替、再起動後の保持を掲載。
- [初回観測](pilot.md) は New Files の3状態と過去の Segment 画面を掲載。
- [SEG-01 作成とトラック追加](seg-01-creation.md) は Length の切替・確定・取消し、17種類のトラック一覧、範囲選択、追加結果を掲載。
- [SEG-02・03 表示範囲と文書切替](seg-02-03-viewport.md) は Sequence 展開、最大化、横ズーム、スクロール、2文書の切替を掲載。
- [SEG-02 子ウィンドウ操作](seg-02-mdi.md) は上下・左右・重ね整列、移動、サイズ変更、最小化・復元・最大化を掲載。
- [SEG-04 トラックヘッダー](seg-04-track-headers.md) は選択色、Sequence の折りたたみ・展開、境界と並べ替えの試行を掲載。
- [SEG-05 選択とノート入力](seg-05-selection.md) はルーラーと空セルの違い、時間範囲、矩形・単独・全選択、混在値の表示を掲載。
- [SEG-05 貼付けと履歴](seg-05-clipboard-history.md) は Merge、Cut、Overwrite、フォーカスと Undo/Redo、文書別の履歴と保存値を掲載。
- [SEG-06 Sequence Part と楽器選択](seg-06-sequence-parts.md) は鍵盤、縦ズーム、PChannel 2・10、打楽器名、Band0 と楽器メニューを掲載。
- [SEG-07 ノートのポインターと Snap](seg-07-note-gestures.md) は中央と端の形状、ドラッグの試行、Grid・None の挿入と保存値を掲載。
- [SEG-08 CC レーンと曲線](seg-08-cc-curves.md) は RPN の取消、PBend と Expression の描画・開閉・選択、伸縮、再起動時の復元を掲載。
- [SEG-09 Tempo と Time Sig](seg-09-tempo-timesig.md) は同一拍の異なる Tick、Cycle Tempos、6/8 とグリッド、表示範囲外のラベルと保存値を掲載。
- [SEG-09 Bands と Band 子エディター](seg-09-band-editor.md) は時刻と楽器欄、子ウィンドウ、パン・音量、Organ 1・Room、DLS の取消、再読込を掲載。
- [SEG-10 Segment Properties](seg-10-properties.md) は4タブ、入力欄の有効条件、Length と GUID の取消し、選択に応じた対象切替を掲載。
- [SEG-10 Length・Start Time・Loop の入力境界](seg-10-properties-bounds.md) は44枚と保存前後の `.sgp` 22点で、小節指定と時間指定、短縮時の表示と保存値、Repeat、Loop 端点、再読込と異常終了を掲載。
- [SEG-10 追加小節とピックアップ](seg-10-length-pickup.md) は49枚と `.sgp` 22点で、スピンの折り返し、Sequence の有無、前後の音符、縮小時の保持、複数パートと再読込を掲載。
- [SEG-10 Boundary の Timing とピックアップ再生](seg-10-boundary-prepare-time.md) は21枚、保存ファイル6点、先頭再生4回の録音で、下端のキー操作と Timing による発音差を掲載。
- [SEG-10 Boundary の Cut Off と再生中の切替](seg-10-boundary-cutoff.md) は23枚、.sgp 15点、再生中の切替5回の録音で、Cut Off の3択と試聴設定の優先順位を掲載。
- [SEG-10・12 ピックアップと末尾の追加小節の再生](seg-10-extra-bar-playback.md) は30枚、.sgp 8点、先頭再生7回の録音で、有限・無限ループ、自然終了、Clocktime、5小節に延ばした対照条件を掲載。
- [SEG-10 Segment Properties の Tab 順と Info・GUID](seg-10-info-keyboard.md) は47枚、.sgp 14点で、4タブのキーボード操作、GUID の入力形式・重複、Info の保存と別プロセスの再読込を掲載。
- [SEG-10 拍子に応じた Start at と Clocktime の丸め](seg-10-meter-clock.md) は43枚、.sgp 40点で、6/8と混在拍子の入力補正、1 ms刻みの長さ、別プロセスの再読込と先頭再生の差を掲載。
- [SEG-10 Transition の Boundary・Timing とピックアップ](seg-10-transition-pickup.md) は32枚、.sgp 18点、録音8回で、既定設定の適用先、Immediate と Barline、要求位置による発音差を掲載。
- [SEG-11 再生・停止・位置](seg-11-transport-position.md) は2つの再生入口、現在テンポ、横追従、停止・再開、Play Cursor を掲載。
- [SEG-12 ループ・端点・境界停止](seg-12-loop-boundary.md) は有限・Infinite・Clock Time の折り返し、端点の入力と保存、Barline 停止、別起動後の復元を掲載。
- [SEG-13 保存と別プロセスでの再読込](seg-13-reopen.md) は Project の自動再オープン、Segment の表示と値の復元、再保存の照合を掲載。
- [SEG-14 Groove と Lyric](seg-14-groove-lyric.md) は4トラックの追加、Groove Level と繰り返し、歌詞の時刻と同じ拍の切り替え、日本語表示と保存値を掲載。
- [SEG-14 Marker と Mute](seg-14-marker-mute.md) は Marker と Enter Switch Point の上下段、Mark Range メニュー、PChannel の拍・行操作、再割り当て、折りたたみと再読込を掲載。
- [SEG-14 Chord の拍セルと鍵盤](seg-14-chord.md) は拍単位の挿入、Chord/Scale・Inversion、Hide と Undo、トラックの Key と再読込を掲載。
- [SEG-14 Chordmap 参照と Signpost](seg-14-chordmap-signpost.md) は参照が空の状態、候補の選択と履歴、ChordMap Designer の入口、Cadence、Activity Level、Compose Chords の依存不足を掲載。
- [SEG-14 Pattern と Style](seg-14-pattern-style.md) は原版の Pattern 展開・入力、Style の参照と Tempo・Bands の連動、Seed Variations を掲載。
- [SEG-14 Pattern Part の表示、リンク、移動と Variation](seg-14-pattern-part.md) は40枚で、PartRef の別表示とリンク、リンク解除、PChannel 順、Variation、Keep Constant、別起動での復元を掲載。
- [SEG-14 リンク中の Pattern Part の CC と Var Switch Point](seg-14-pattern-controls.md) は38枚、3本の MIDI、2本の録音で、共有編集と独立編集、マーカーの条件、Variation 別の出力、別起動と Part タブ下部を掲載。
- [SEG-14 Pattern の Variation 再生順と途中切替](seg-14-variation-playback.md) は29枚と6本の MIDI で、Play と Reset、条件付きと無条件の Exit、コード変更と対照試験、番号とカーソルのずれを掲載。
- [SEG-14 Pattern の Reset Variation Order on Play と単独試聴](seg-14-variation-order.md) は35枚と17本の MIDI で、1 Part・2 Variation、実際の Segment Loop、単独試聴後の Reset の差、Variation Choices を掲載。
- [SEG-14 Style Track の Seed Variations](seg-14-style-seed.md) は36枚と17本の MIDI で、有効・無効、手入力、生成、取消し、Project 再読込後の表示と試聴の差を掲載。
- [SEG-14 Compose Chords](seg-14-compose-chords.md) は ChordMap の Signpost Group、候補不足の警告、Pattern・Style での生成と保存・Undo/Redo を掲載。
- [SEG-14 複数 Signpost、Cadence、再生時の Compose Chords](seg-14-compose-chords-followup.md) は33枚と2本の録音で、C1 の挿入、選択が外れたときの Properties、再生・ループ時の再生成、別起動での復元を掲載。
- [SEG-14 Chord Path の接続、Cadence 2件と発音](seg-14-chord-path.md) は33枚、4本の MIDI、2本の録音で、固定長・可変長・重み0の生成結果と C1・C2 による音高差を掲載。
- [SEG-14 Parameter Control](seg-14-parameter-control.md) は埋め込み・Performance 側・Tools の候補差、Echo1 の描画・反転・移動、折りたたみ、保存と再読込を掲載。
- [SEG-14 Parameter Control の複数曲線、時刻と Echo の発音](seg-14-parameter-control-followup.md) は38枚と1本の録音で、重なり、全選択と一括編集、単独ハンドルと一括移動の境界、Clocktime、Echo1 の遅延音を掲載。
- [SEG-14 Segment Trigger](seg-14-segment-trigger.md) は空状態、Segment・Motif の候補、境界と再生種別、時刻の変更、保存と再読込を掲載。
- [SEG-14 Segment Trigger の重なり、発音境界、Primary](seg-14-segment-trigger-followup.md) は29枚と3本の録音で、同一 Tick の表示と Cycle Items、再起動後の選択、Immediate・Beat の発音差、Primary とトラックの Enable Play を掲載。
- [SEG-14 Segment Trigger のドラッグ、貼り付け、同じ拍での選択](seg-14-trigger-drag.md) は27枚と SGP 8件で、ラベルの所属拍、通常ドラッグの選択、Tick が一致する場合と異なる場合の Merge・Overwrite を掲載。
- [SEG-14 Segment Trigger の同一 Tick にある2件の発音](seg-14-trigger-simultaneous.md) は29枚、MIDI 6本と WAV 4本で、同時・単独・トラック無効・保存順の逆転を掲載。
- [SEG-14 Segment Trigger の Align と Start at next marker](seg-14-trigger-alignment.md) は36枚と MIDI 23本で、通常境界・Align の起点と省かれる音・後続 Marker と代替境界を掲載。
- [SEG-14 子 Segment の Enter Switch Point と Align](seg-14-trigger-enter.md) は16枚と MIDI 5本で、Marker トラックの上下段、前後の Enter と代替境界、後続2点の選択を掲載。
- [SEG-14 Pattern Part の Variation の Enter と Align](seg-14-trigger-variation-enter.md) は23枚と MIDI 8本で、Enter の追加と切り取り・貼り付け、子の発音開始と Chord Change による Variation 選択を掲載。
- [SEG-14 親の Chord と子の条件付き Variation Enter](seg-14-trigger-parent-chord.md) は25枚と MIDI 8本で、コードの前・同時・後、Flags、トラック削除、無条件の対照を掲載。
- [SEG-14 Segment Trigger の Primary、Secondary、Controlling Segment](seg-14-trigger-primary-control.md) は23枚と MIDI 9本で、Type の表示、3つの発音境界、親の発音停止とテンポを掲載。
- [SEG-14 Controlling Segment のテンポ競合と実際の音の間隔](seg-14-trigger-tempo-audio.md) は22枚、WAV 6本と MIDI 5本で、Beat の競合、子の終了時の復帰、親イベントを遅らせた対照、Transport 表示を掲載。
- [SEG-14 同じ Tick にある Segment と Motif の再生](seg-14-trigger-motif-overlap.md) は26枚と MIDI 6本で、参照欄の切り替え、前面と保存順、単独・同時・Enable Play オフの出力を掲載。
- [SEG-14 元の SSBird と accordion の同時発音](seg-14-trigger-original-audio.md) は22枚、MIDI 5本と WAV 5本で、内蔵 Pattern と Motif の表示、単独と同時、Enable Play オフ、音の重なりを掲載。
- [SEG-14 Script Track](seg-14-script-track.md) は Script と Routine の割り当て、Timing、同じ拍での切替、手動・再生時の呼出し、保存と再読込を掲載。
- [SEG-14 Script Track の複数選択と別起動](seg-14-script-track-followup.md) は28枚と1本の録音で、全選択と Cycle Routines、メニューと Ctrl+R の差、ドラッグ試行、負の Tick の履歴と復元を掲載。
- [SEG-14 Script Track の Timing と発音時刻](seg-14-script-track-timing.md) は22枚と2本の録音で、同一ルーチンの3方式、基準音との間隔、Trace の表示区間、保存時の1バイト差、FarmMusic の手動・再生経路の違いを掲載。
- [SEG-14 Script Routine の Belongs To と発音・Trace](seg-14-script-belongs.md) は27枚、録音1本の有効12試行と保存値で、開始時刻を固定し所属時刻を1小節後にした場合の3方式、Quick response の Trace の遅れ、開き直しを掲載。
- [SEG-14 Script Track の FarmMusic とスクリプト言語](seg-14-farmmusic-playback.md) は17枚と4本の録音で、手動と再生、言語だけを替えた構文エラー、互換構文にした場合の Trace と Alarm、Language 一覧と Escape を掲載。
- [SEG-14 Script Properties の Language 切替](seg-14-script-language-ui.md) は32枚と8保存時点で、下キーと Escape、マウスでの切替、手動と再生、別起動、構文エラー時の空の候補一覧と復帰を掲載。
- [SEG-14 Script Track のドラッグと時刻編集](seg-14-script-track-gestures.md) は24枚で、通常ドラッグ、Tree からのドロップ、Start Time と Belongs To の移動、同じ拍の3件と別起動での選択順を掲載。
- [SEG-14 Script Track の同一拍の選択、ルーラーとドラッグ](seg-14-script-selection-drag.md) は24枚と11状態の保存値で、同一拍の単独コピー、位置線と範囲、全選択の移動と Undo/Redo、移動しなかった再試行を掲載。
- [SEG-14 Script Track の別文書への貼り付け](seg-14-script-track-cross-document.md) は23枚と保存ファイルで、読み込み時のグループ、Merge と Overwrite、間の空セル、Undo と Redo を掲載。
- [SEG-14 Script Track の複数ルーチンの貼り付けと範囲端](seg-14-script-paste-range.md) は32枚と保存ファイルで、Merge の同一 Tick、Overwrite の消去端、コピー元を7小節にした対照、Undo/Redo を掲載。
- [SEG-14 別 Project へ貼り付けた Script Routine の参照](seg-14-script-project-reference.md) は16枚と保存ファイルで、同名 Script の解決、GUID の変化、ファイル指定とキャンセル後の再読込を掲載。
- [SEG-14 貼り付け時に指定した Script ファイルの取込](seg-14-script-file-picker.md) は27枚と保存値で、同じ Script と別名 Script の取込、Undo 後のファイル、欠損 Routine のエラーと修復を掲載。
- [SEG-14 別 Project の Script 候補の自動選択](seg-14-script-candidate-order.md) は22枚と10保存状態で、ファイル名・Script 名・2種類の GUID の各一致条件、同名2候補の登録順、再読込と再貼り付けを掲載。
- [SEG-14 Runtime Script の取込](seg-14-script-runtime-picker.md) は23枚と2保存状態で、`.spt` の書き出し、貼り付け時の `.spp` としての取込、GUID、設計用ファイルとの対照、再読込後の手動呼出しと再生を掲載。
- [SEG-14 貼り付けた Script Routine の発音と Trace](seg-14-script-paste-audio.md) は21枚と録音4本で、貼り付け前、同じ Project と別 Project の手動・先頭再生、Trace の更新区間、保存された両 GUID を掲載。
- [SEG-14 同じ Segment 内の Script Track 間のドラッグと貼り付け](seg-14-script-cross-track.md) は27枚と保存ファイルで、2行の追加、ドラッグ後の選択と Properties、貼り付け時の時刻差、Undo/Redo、グループマスクの正規化を掲載。
- [SEG-14 Parameter Control、Segment Trigger、Script の入口と試験素材](seg-14-control-triggers-script.md) は原版ヘルプの専用画面、参照候補を用意するための保存値、実機で確認する状態を掲載。
- [SEG-14 Wave Track の波形、レイヤー、Variation](seg-14-wave-track-editing.md) は原版での選択色、時刻とループの編集、表示と所属の切替、Part・レイヤーの追加、再起動後の復元を掲載。
- [SEG-14 Wave Track のドラッグ、端点、ループ音](seg-14-wave-track-followup.md) は32枚と1本の録音で、名前付近のドラッグ、隠れたレイヤーの全選択、End と Length の鍵、Offset By、ループの発音と再起動後の表示を掲載。
- [SEG-14 Wave Track の Project Tree ドロップと同一レイヤーの重なり](seg-14-wave-drop-overlap.md) は32枚と SGP 6条件で、ファイル行からの追加、Variation とレイヤー、重なる波形の選択と再読込を掲載。
- [SEG-14 Wave Track の Project Tree ドロップと複数 Variation での挿入](seg-14-wave-tree-variation-drop.md) は27枚と保存済み SGP 8状態で、ファイル行と子ノードのドロップ試行、Insert の所属、表示中と非表示の波形上での右クリック、重なりの再読込を掲載。
- [SEG-14 Wave Track の重なり、選択順、同時発音](seg-14-wave-overlap-audio.md) は22枚と WAV 2本で、同時刻の表示と選択順、同じレイヤーと別レイヤー、保存順の逆転を掲載。
- [SEG-14 Wave Track の重なりと Variation の発音](seg-14-wave-variation-audio.md) は30枚と WAV 5本で、同じ Variation と別々の Variation、定順ループ、途中で表示を外した場合の発音と赤い番号、保存と再読込を掲載。
- [SEG-14 Wave Track の再生途中で Variation 1 を外す](seg-14-wave-live-variation-boundary.md) は30枚と WAV 8本で、発音前・発音中・折り返しの前後の操作を、赤い番号、波形、各周の発音と照合した記録を掲載。
- [SEG-14 Tempo Track を含まない Wave Segment の再生速度](seg-14-wave-tempo-context.md) は47枚と WAV 7本、12条件で、先行テンポ、Alternate Tempo の切り替え順、明示 Tempo、画面時刻と Project 再読込を比較。
- [SEG-14 Producer 再起動後の再生テンポと Wave 選択](seg-14-wave-tempo-restart.md) は36枚と WAV 5本で、Tempo Track のない Segment の初回再生、Alternate Tempo の保持、Snap To Zero の初期化、Project の自動復元を掲載。
- [SEG-14 Wave 文書のループ端と Segment 項目への継承](seg-14-wave-document-loop.md) は31枚と WAV 2本で、端の移動、Undo、既存・新規項目の初期値、保存後の反復周期を掲載。
- [SEG-14 Wave エディターのループ方式、長さの固定、終点の検索](seg-14-wave-editor-loop-options.md) は43枚、WVP 8状態、録音2本で、2つの Lock、Snap To Zero、Best Loop End の境界、Forward と Loop and Release、再読込を掲載。
- [SEG-14 DLS Region のループ方式と Note Off 後の再生](seg-14-dls-note-release.md) は37枚と WAV 5本で、Instrument と Region の入口、保存時の1バイト差、終端の先の PCM、読み直した画面と発音の差を掲載。
- [SEG-14 Wave Track の灰色の参照元ループ値が更新される条件](seg-14-wave-reference-refresh.md) は48枚とディスク上の12時点で、ドラッグ、Wave の保存、Undo・Redo、数値確定、Properties・文書・Project の開き直しを比較。
- [SEG-14 Wave Track と Wave Properties](seg-14-wave-track.md) は原版ヘルプの Part・Layer、Wave 項目と文書の設定、FarmGame の6組の参照・PCM・時刻を掲載。
- [SEG-15 複数 Segment の編集と参照](seg-15-multiple-documents.md) は上下整列、Properties と Transport の対象、Tempo の文書間貼り付けと履歴、参照名の即時更新と再起動を掲載。
- [UI04 Style Designer と Motif](ui-04-style-motif.md) は過去の原版撮影から Style の作成・読込、Pattern と Motif の一覧、accordion の右クリックと3つのプロパティタブを掲載。
- [UI07 DLS Collection、Instrument、Wave エディター](ui-07-dls-wave-instrument.md) は空の Collection、Wave 挿入、Instrument 作成の依存条件、波形の初期表示、保存とエラーを掲載。
- [UI08 AudioPath Designer](ui-08-audiopath-designer.md) は原版ヘルプのペイン、バスとバッファ、エフェクトの追加と順序、Send の宛先を掲載。
- [UI08 AudioPath Designer の実機操作](ui-08-audiopath-editing.md) はペインと列のリサイズ、Bus と Buffer の選択、Chorus と Echo の順序、Send の宛先と音量、再起動を掲載。
- [UI10 Script Designer・Message Window](ui-10-script-designer.md) は原版の3ペイン、Script の作成と Refresh、Message Window の移動と手動呼出しを掲載。
- [UI11 Container](ui-11-container.md) は原版ヘルプの参照と埋め込み、Add/Remove Files、FarmGame の設計用・実行用データを掲載。
- [UI11 Container の実機操作](ui-11-container-editing.md) は Script・独立文書・Segment 内蔵版、一覧と Tree の操作、Cache、設計用と実行用の保存差を掲載。
- [UI12 ToolGraph Designer](ui-12-toolgraph-designer.md) は原版ヘルプの3ペイン、Tool 割り当て、現行ホストの登録と未登録クラスの素材を掲載。
- [UI12 ToolGraph Designer の実機操作](ui-12-toolgraph-editing.md) はペインと PChannel Group、Segment・AudioPath 内蔵版、設計用と実行用の保存、未登録 Tool の取り込みを掲載。
- [UI13 Transport と Wave・MIDI 書き出し](ui-13-transport-export.md) は原版で撮影した空の Secondary と出力名、ヘルプのメーター、設定、書き出し条件を掲載。
- [UI13 Secondary と書き出しの実機操作](ui-13-transport-export-live.md) は45枚で、枠と境界設定、Primary との同時再生、MIDI の待機と自然終了、Wave の経路・警告・停止、ファイル照合を掲載。
- [UI13 Synth Status と Output Status](ui-13-synth-output-status.md) は46枚で、Voices・Peak・Reset、PChannel の追加・消去、ミュート、ソロ、改名、All Notes Off、浮動と復帰、調査中のアクセス違反を掲載。
- [UI13 Transport Options と試聴速度](ui-13-transport-options-live.md) は63枚で、2小節のカウントインと MIDI の照合、MIDI 設定の有効条件、レイテンシーの保持、既定 AudioPath と再生可否、Tempo Modifier の時間と入力範囲を掲載。
- [UI14 Help メニューと HTML Help](ui-14-help.md) はインストール版の Contents、独立ウィンドウ、Help を閉じた後の本体とヘルプ索引を掲載。
- [UI14・ROLE01 補助アプリと導入の入口](ui-14-companion-apps.md) は Farm Demo と StylePlayer の配置、試聴手順、インストール済み版の登録を掲載。
- [UI14 Farm Demo の実機操作と音声](ui-14-farm-demo-live.md) は39枚と3本の録音で、キー操作、効果音、背景と Cow の重なり、End と All Stop、素材不足時の警告を掲載。
- [UI14 StylePlayer の実機操作と音声](ui-14-styleplayer-live.md) は54枚と2本の録音で、自動再生、Style と Band・Motif の連動、キーボード選択、Length 0、再構成と再生位置、About と移動を掲載。

## 成果物と保存場所

| 成果物 | 保存場所と使い方 |
| --- | --- |
| 調査計画と進捗 | このページ。各回で観測済みのケース、保留理由、次の操作を更新する |
| 画面別・操作別の Markdown | このディレクトリにケース ID ごとに追加する。画像を本文に埋め込み、入口、操作、状態の変化、終了時の状態を説明する |
| Markdown 掲載画像 | [figures/](figures/) に保持する。画像は出典・撮影条件を記録し、相対リンクで埋め込む |
| 撮影原本・操作記録・試験入出力 | `legacy/windows/work/original-gui/<ケースID>/<実施日時>/` に保存する。撮影画像、観測ログ、入力・出力ファイル、ハッシュをひも付ける |
| 既存の互換性台帳 | [UI 互換性台帳](../ui-compatibility.json) の該当ケースに資料を結ぶ。原版の観測結果と候補側の比較結果をそれぞれの証拠で更新する |

既存の [外観と操作互換の契約](../ui-compatibility.md)、[画像1〜47の要件索引](../priority-segment-sequence.json)、[優先実装計画](../priority-segment-sequence-2026-10-10.md) を画面一覧の出発点にする。各画面の入口は原版のメニュー、ツリー、右クリック、ヘルプで洗い出し、資料が見つかったケースはその取得条件を確認する。既存画像のうち今回の資料に必要なものは、元のパス・観測日・バイナリと画像のハッシュを記録して引用する。

## 実施順序

| 段階 | 対象 | 次へ進む条件 | 現在 |
| --- | --- | --- | --- |
| 0 | 起動環境、証拠の保存方法、New Files の予備観測 | 参照バイナリを特定し、操作前後の画像を Markdown から参照できる | 調査用コピーと専用 Project2 で確認済み。Windows 11、1280×800、96 DPI |
| 1 | Segment の作成、開閉、プロパティ、Timeline 共通操作 | SEG-01〜05、SEG-10、SEG-13 の基本ケースを同一入力から再実行できる | 作成、横ズーム、子ウィンドウ操作、Properties、別起動、選択、貼付け、Undo/Redo を観測。ヘッダーの境界操作、長さやサイズの上下限、Save As と閉じる分岐を継続 |
| 2 | Sequence・Part・CC、Band、再生中の Timeline | SEG-06〜09 と SEG-11・12 の基本ケースと主要な境界を撮影する | 複数 Part、Snap、CC、Tempo・Time Sig、埋め込み Band、再生の追従、有限・Infinite・Clock Time ループ、Barline 停止を観測。個別の追試条件は各資料に記録 |
| 3 | その他の Segment トラック、参照文書と複合操作 | SEG-14・15 の全入口を確認し、個別動作と文書間の状態を記録する | 17種類すべてで追加または編集を撮影。Groove・Lyric・Marker・Mute・Chord・Chordmap・Signpost・Wave・Script・Segment Trigger・Parameter Control・Pattern・Style 参照は別起動で復元を観測。Script の Timing 3方式は同一ルーチンの発音と Trace を測定し、Start Time を固定して Belongs To を1小節後にした場合も照合。FarmMusic は VBScript の手動呼び出しと互換構文の AudioVBScript での再生を録音で照合。Script の通常ドラッグ、Start Time と Belongs To、同じ拍の3件と保存順に沿った表示切替、グループ1〜32への正規化、別文書への Merge・Overwrite と複数選択の消去端、同じ Segment 内の2本の Script Track へのドラッグと貼り付けを照合。Wave はドラッグ、時刻の鍵、Offset By、ループ音、Tree のファイル行からのドロップ、Insert での複数 Variation への所属と既存波形上での挿入、同一レイヤー内の重なりの保存と2音の出力、同時刻の選択と順序、Variation 別の発音と定順ループ、途中切り替えの赤い番号、波形の再描画、発音前・発音中・折り返しの前後の発音差、Tempo Track なしの再生速度と倍率の切り替え順、Producer 再起動後の初回テンポと倍率の保持、Wave 文書からのループ継承と再生周期、灰色の参照元ループ値が更新される条件、Wave エディターの Loop と Selection の Lock、Snap To Zero の別起動時の初期化、Best Loop End、Loop and Release の保存と単独試聴を照合。DLS Region は Instrument Editor、Region のループ上書き、Volume Release、Note Off 後の終端側の PCM、再読込した画面と録音を照合。Segment Trigger は同一 Tick の選択と保存、Immediate・Beat・Primary の発音、同時刻の2つの Segment の個別出力、Secondary での Align と Start at next marker の発音境界、子の Marker トラック下段にある Enter の優先、Pattern Part の Enter と初回の Variation 選択、Beat・Align・Marker での3つの Type の発音とテンポ、同一 Tick にある Segment と Motif の個別 MIDI 出力、元の SSBird と accordion の音声の重なり、親の Chord と Enable Control が子の初回 Variation と音高に及ぼす影響、Beat で Controlling Segment と親のテンポが競合したときの再生速度と復帰、通常ドラッグの選択と保存時刻、Paste での同一 Tick と同一拍内の異なる Tick の扱いを照合。Parameter Control は複数選択と移動、Clocktime、Echo1 の発音を照合。Pattern Part はリンク、CC と Var Switch Point の共有と独立編集、行の順序、Variation 別の MIDI と途中切替、単独試聴後の Reset の差、Keep Constant を照合。Pattern・Style の Compose Chords は複数 Signpost、C1・C2、再生・ループ時の生成、MIDI 音高を照合。Chord Path は固定長・可変長と重み0、Style の録音は実際の音高を照合。Seed Variations は生成・手入力・再読込後の試聴を MIDI で比較。複数文書の貼り付け・履歴・再生対象・参照名を観測し、異なるグループと再生音は追試へ |
| 4 | 本体共通、各専用エディター、Transport と補助画面 | アプリ全体の調査表に観測・保留の根拠と次の手順がそろう | 新規作成、プロジェクトツリー、Window、選択対象と Transport の連動を一部観測。Style・Motif、DLS・Wave、Help の過去の原版撮影を照合。AudioPath、Script Designer、Container、ToolGraph を実機観測。Transport 補助機能と書き出し、Farm Demo と StylePlayer の操作と音声を記録。導入・解除の実機画面は未観測 |
| 5 | 主要ケースの撮り直しと相互照合 | 別の作業コピーで追試し、状態・操作・画像の記述とリンクを確認する | 未着手 |

1回の作業は同じ入力を使う1〜3ケース程度を目安とする。まず通常経路を一通り確定し、同じ画面の境界操作を続けて調べる。Segment の観測を優先し、調査中に通過する本体共通画面も同時に記録する。撮影枚数は、Segment でおおむね150〜250枚、全体では数百枚を想定する。遷移の意味が異なる状態を揃えることを優先し、枚数は観測した状態数に応じて見直す。

## 実行対象と比較条件

- 原版 5.3.0.900 のインストール済み版、起動修正済みインストール版、作業用コピーは、実行パスと SHA-256 で特定する。[原版の履歴](../original-reference-2026-10-10.md) と [起動修正の記録](../../../README.md#この環境のインストール済み版) を参照する。原版の機能を変える差分がある場合は、影響する画面・操作をケースに記録する。起動時の登録に関係する操作には未改造時の証拠も確認する。
- このローカルマシンで本計画を実施する間の Computer Use は、2026-11-11 にユーザーから継続承認を受けている。計画内の画面操作・撮影・調査用ファイルの編集や通常終了は、その都度ユーザーに確認せず進める。
- 原版または作業用コピーを起動する前に、開いている Producer のウィンドウとプロセスを実行パスで照合する。使う実行ファイルと専用 Project がすでに開いていれば、そのウィンドウを再利用する。不要な Producer は通常終了し、未保存の変更があれば保存先と内容を確認してから閉じる。
- 別の実行ファイルまたは別プロセスでの検証が必要なときは、既存の Producer の終了を確認してから、対象の実行ファイルと作業ディレクトリを指定して起動する。起動直後に PID・実行パス・ウィンドウを再照合し、目的の1プロセスで操作する。
- 新規作成・編集・再読込は、調査用の INI と作業用プロジェクトだけが開く状態で実行する。コピー元の入力を保全し、試験ごとに `inputs/` から `session/` へ複製する。保存・再保存先と元データへの変更有無をハッシュで確認する。既存プロジェクトが開いている場合は、作成や保存を始める前に専用セッションを確保する。
- 基準環境は最初のセッションで OS、表示言語、テーマ、フォント、ディスプレイ解像度、DPI、作業領域、ウィンドウ全体とクライアント寸法を採取して固定する。旧資料の 1280×800・96 DPI や 4K・200% は別条件として記録する。標準サイズに加え、狭いウィンドウ、最大化、高 DPI の代表例を調べる。
- 通常の Segment 検証では、調査用コピーの `View` で `Project Tree`、`Standard Toolbar`、`Transport Controls`、`Status Bar` を表示する。観測に必要なツールバーをその都度表示し、撮影時の表示項目を記録する。切替と復元は [UI01 の手順](ui-01-view-toolbars.md) を参照する。
- 撮影前に表示中の文書、フォーカス、ツリー展開、ウィンドウの配置、トラック表示順、ズーム、横・縦スクロール、再生位置と入力デバイスの条件を記録する。音源、AudioPath、MIDI 入力などが操作可能状態に関係するケースでは接続条件も記録する。

## Segment エディターの調査表

| ケース | 画面と入口 | 確認する操作・境界 | 主な撮影点 |
| --- | --- | --- | --- |
| SEG-01 作成・追加 | File New、Standard New、Ctrl+N、Project ツリー、New Files、Length、Add New Tracks | 既定/指定名、単独/複数選択、選択解除、無選択、OK/Cancel/Esc、長さの単位と最小/最大、初期トラック、空選択での確定可否 | 初期、Segment 選択、Length の各単位、Add New Tracks 初期/選択中/無選択、確定後、取消後 |
| SEG-02 ウィンドウと画面構成 | ツリーから開く、MDI 子、Window メニュー、Properties | 最大化/復元/最小化、移動、四辺と四隅のリサイズ、最小寸法、重なり、Tile/Cascade、複数文書切替、子ウィンドウを閉じた後のフォーカス | 初期/狭幅/最大化、ドラッグ前後、フォーカス有無、切替前後、閉鎖後 |
| SEG-03 時間軸と表示範囲 | 小節・拍・グリッド・時刻ルーラー、横縦スクロールバー、Zoom | ホイール・矢印・ページ送り・つまみ、ズーム中心、最小/最大、全体表示、端でのクランプ、長い Segment、拍子変更、スクロール位置の保持 | 通常/拡大/縮小、開始端/中間/終端、狭幅、目盛り切替、グリッドの変化 |
| SEG-04 ヘッダーとトラック配置 | トラック名、グループ番号、上下境界、表示メニュー | 選択/非選択、グループ表記と順序、表示名省略、開閉、個別高さと境界リサイズ、ヘッダー幅、並べ替え可否、アクティブ行、スクロール中の固定部 | 折りたたみ/展開、短/長ラベル、複数グループ、境界ホバー、ドラッグ前/中/後 |
| SEG-05 選択と共通編集 | ルーラー、トラック名、空セル、イベント、右クリック、Edit | 単一/複数/範囲、Shift/Ctrl/キーボード、解除、Select All、メニューの有効状態、挿入/削除、コピー/切り取り/貼付け、取り消しとやり直し、別トラック・別文書 | 選択の段階、右クリック位置ごとのメニュー、空/選択あり、貼付け前後、Undo/Redo 後 |
| SEG-06 Sequence・Part | Sequence ヘッダー、Part、ピアノロール、鍵盤、楽器選択 | 開閉、パート追加・切替、グループと PChannel、縦ズーム、半音単位の移動、鍵盤試聴、複数パート、ドラム表示、Instrument メニューと Cancel | 折りたたみ/展開、通常/狭幅、縦ズーム前後、鍵盤移動、Part 切替、楽器階層 |
| SEG-07 ノートとプロパティ | ノート中央/左右端、空グリッド、Note Properties | 挿入、選択、矩形、移動・伸縮、ピッチ変更、Snap、複製、無効範囲、オートスクロール、Velocity、Start/End/Duration、Belongs to Beat、複数選択時の混在値 | ポインター形状、ドラッグ前/中/後、Snap の結果、単一/複数のプロパティ、取消/Undo |
| SEG-08 CC と曲線 | CC レーン、CC の追加/削除、Curve Properties | レーン開閉・高さ、CC 種別、点/線選択、値と時刻の移動、端点と曲線形状、複数選択、ノート選択との連動、再読込 | レーンなし/あり、点選択、ハンドルホバー/ドラッグ、表示切替、プロパティ |
| SEG-09 Tempo・Time Sig・Bands | トラック名、空セル、イベント、Band 子エディター | テンポ変更、拍子とグリッド、同じ拍の複数イベント、先頭/直前値、Bands の参照/所有・ダブルクリック、音色/PChannel・Pan・Volume、閉鎖後の選択 | 基本表示、スクロール境界、グループ差、選択/入力/エラー、Band 子の開閉と親の変化 |
| SEG-10 Segment Properties | ツリー/エディターの Properties、F11、各ページ | Info、長さ、Start Time、Loop、Boundary の表示と既定値、スピン方向、Tab 順、相互依存・無効化、適用/取消、範囲外、再表示/保存復元 | 全ページ、コントロール別のフォーカス/無効/入力値、ページ切替、適用直後、再表示 |
| SEG-11 再生と位置 | Transport、再生位置、現在テンポ、ステータスバー | 停止中の位置変更、再生・停止・再開、Play Cursor の追従有無、横端での追従、表示位置と再生位置の分離、終了時の位置 | 停止中、開始直後、端の手前/後、停止直後、自然終了、再開直後 |
| SEG-12 ループ・境界 | Start/Loop、境界設定、ループマーカー、Transport | 開始位置とループ区間、反復、端点の移動、music/reference time、拍子・テンポ切替、ループ折返し、境界停止/遷移、無効な範囲 | 端点ホバー/移動、開始前後、折返し前後、停止要求時/確定時、エラー |
| SEG-13 保存と閉鎖 | File Save/Save As、閉じる、文書切替、別起動 | 編集済み表示、参照の差し替え、保存取消し、破棄/継続、元入力の保持、Undo 履歴と選択・ズーム・スクロールの再読込 | 編集前/後、確認の各分岐、保存後、閉鎖後、別起動で復元した直後 |
| SEG-14 その他のトラック | Add New Tracks、各ストリップと Properties | Chord、ChordMap Reference、SignPost、Command、Lyric、Marker、Mute、Parameter Control、Wave、Segment Trigger、Script、Style/Pattern など、一覧で確認できた種類を個別に調査する | 空行、追加後、選択、専用メニュー/ページ、移動/取消/境界、依存不足 |
| SEG-15 複合・文書間 | 複数トラック、複数 Segment、参照先のエディター | 同じ小節での種類別動作、グループの違い、複数文書の Undo・Clipboard・再生の対象、参照更新と独立性 | 操作対象と非対象、フォーカス切替、前後、別起動 |

SEG-01〜05 を先に追試し、同じ Segment の状態を SEG-06〜12 へ引き継ぐ。追加したトラックは、共通操作が同じかを一つずつ照合する。グループや音源に依存する差は操作名と発生条件を記録する。

## アプリ全体の調査表

| 既存台帳 | 画面・機能 | 収集する主な状態 |
| --- | --- | --- |
| UI01 | 起動、メインフレーム、Project ツリー、File/Edit/View/Add-Ins/Window/Help、Standard・Bookmarks、共通ダイアログ | 何も開いていない状態、作成/読込後、ツリーの展開・折りたたみ、名前変更、分割幅、表示/非表示、ツールバー移動・ドッキング、メニュー項目の無効/有効、キーボード移動 |
| UI02・03・09 | Timeline、Segment と時刻付きトラック | 上の Segment 調査表を使用する |
| UI04 | Style、Pattern、Motif | パターン一覧、Part・Variation、試聴、共有/所有表示、子画面、選択・移動・複製 |
| UI05・06 | Band、Chord、ChordMap、SignPost | 鍵盤・音色、グラフ・節点、リンクの作成/解除、複数選択、プロパティ・移動、Undo/Redo |
| UI07 | DLS Collection、Instrument、Wave、Region、Envelope、PanVol | ツリー/波形/鍵盤の連動、分割位置、鍵域・速度域・ループ端のハンドル、重なり、エラー、試聴 |
| UI08 | AudioPath、バス、送信、効果 | 経路と順序、値の直接入力とスピン、挿入・並べ替え、宛先切替、プロパティ・取消 |
| UI10〜12 | Script、Container、ToolGraph | 参照/埋込/変数、追加と切替、選択/移動、グラフ接続、読込不足と実行エラー |
| UI13 | Transport、Synth Status、録音/書き出し | 停止/再生/録音、Primary/Secondary、速度・メトロノーム・MIDI、音源切替、出力前後とエラー |
| UI14・ROLE01 | 補助アプリ、Help、導入・解除関連 | 実用上の画面と入口、操作可能状態、参照専用の画面。機能の入口を確認できない場合は理由を台帳に記録する |

## 撮影と操作の記録

1. **操作前を固定する。** 原版の実行パスとハッシュ、ケース ID、入力ハッシュ、開始状態、対象ウィンドウと表示条件を記録する。メニューや独立したプロパティ・ダイアログは本体画面と対象だけの画像を取得する。
2. **実際の入力と結果を結ぶ。** 操作対象を名前・階層・位置で特定し、クリック回数・ボタン、押したキーと修飾キー、ドラッグの始点/終点、ホイールの位置と方向、操作順を記録する。入力の直後に画像と必要な UIA を採り、選択、フォーカス、スクロール、値、利用可能な操作を記述する。UIA と画像が食い違う場合は採取順と再観測結果を残す。
3. **形状と時間を観測する。** ポインターは平常時、ヒット領域へ入った直後、押下中、移動中、ドロップ可能/不可、解放後を区別する。対象はヘッダーの境界、スプリッター、スクロールバー、ノート端、ルーラー、ループマーカー、ウィンドウ枠。マウスポインターと再生カーソルを個別に記録する。ホバーや押下中を取得できない場合は観測待ちにし、手動の保持・撮影など再現できる方法を試す。自動操作ツールの青い強調表示はツールの表示として注記する。再生中は開始時点からの経過時刻と音楽時刻を併記し、連続フレームで折返し・追従・一時表示を記録する。
4. **外観を記述する。** 画面全体、対象の拡大、前後比較を使う。部品名、整列、寸法、余白、字体・サイズ・色、アイコン、切り詰め、選択・非アクティブ・無効状態を記録する。寸法はウィンドウ相対座標と物理ピクセル、DPI を明記する。OS の透かし、描画遅延、ツールの強調表示があれば撮影条件に記す。
5. **終了状態を確かめる。** Cancel/Esc、範囲外、エラー、Undo/Redo、フォーカス移動、閉じて再表示した状態も必要に応じて撮る。文書の保存や復元が関係する場合は試験コピーを別プロセスで開き、値と表示状態を読み戻す。再生表示と音声の実出力はそれぞれ確認内容を記録する。
6. **画像を照合する。** 原本は撮影時の形式で保存し、ファイル名・寸法・SHA-256 を記録する。切り抜きや注釈を使う場合は元画像と範囲を示す。Markdown の各画像に代替テキストと「何を操作して何が変わったか」の短い説明を添える。既存プロジェクト名、個人パス、他アプリを含むフレームは掲載前に見直す。

### 1ケースの Markdown 構成

| 欄 | 内容 |
| --- | --- |
| 対象と入口 | ケース ID、台帳 ID、画面名、メニュー/キー/クリックからの手順 |
| 条件 | 原版の版・SHA-256、実行パス、OS/DPI/寸法、入力の来歴とハッシュ、選択・再生状態 |
| 操作列と画像 | 番号順に前状態、入力、観測結果、画像 ID、カーソル/フォーカス、確認できた終了状態 |
| 例外と範囲 | 無効/取消/範囲外、時間依存、既知の再現条件、未観測項目、元資料へのリンク |
| 実装時の照合 | 操作と見た目をどう比較できるか、対応する保存値やログ、候補側の比較への入口 |

各画像は、例として `seg-07-03-note-left-edge-hover.png` のようにケース、順番、状態が分かる名前にする。同名の取り直しは新しい実施記録に保存し、根拠を更新した日付を記す。

## 試験素材の準備

| 素材 | 用途 |
| --- | --- |
| 空の作業用 Project と新規 Segment | 作成、既定値、取消、追加可能なトラック一覧、初期の選択/Transport |
| 20小節程度の複数トラック Segment | 一覧と順序、グループ、プロパティ、編集、Undo、保存復元。Sequence の短い音と長い音、CC、Tempo/Time Sig/Band を入れる |
| 長尺・複数拍子の Segment | 目盛り、省略、横縦スクロール、端のズーム、追従、ループ折返し。開始・中間・終端を同一入力で再現する |
| 複数 Part、複数グループ、ドラムを含む Segment | 鍵盤・パート・グループ間の差、PChannel、CC 種別、選択と複製 |
| 参照先を含む Project | Style、Band、DLS、Wave、AudioPath、Script の接続と独立した編集画面。既存 [Q1DlsOracle](../../../Q1DlsOracle/) は来歴を調べて作業コピーに利用する |
| 原版サンプルの Project | [QuickStart](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/inputs/quickstart/manifest.json) の既存 Style 参照、[FarmGame](../../../work/original-gui/SEG-14-pattern-style/20261111T074828536163Z/inputs/farmgame/manifest.json) の内蔵 Pattern と Wave 参照。保存値を照合済みの素材から作業コピーを作る |
| Wave の参照・取込素材 | [6組の照合記録](../../../work/original-gui/SEG-14-wave-audiopath/20261111T081723992826Z/farmgame-wave-audit.json) と [元 WAV](../../../work/original-gui/SEG-14-wave-audiopath/20261111T081723992826Z/inputs/wave-import/manifest.json)。既存項目から Wave 文書へ移る操作と、別 Project への取込・追加を確認する |
| Script と AudioPath の参照素材 | [FarmGame の保存値](../../../work/original-gui/SEG-14-control-triggers-script/20261111T081051527595Z/farmgame-script-and-audiopath.json) に Script の12ルーチンと10件の Segment 参照、AudioPath の Echo を記録。Script Track、Script Designer、Parameter Control の候補を出すために使う |
| Container と ToolGraph の依存素材 | [FarmGame 実行用ファイル](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/farmgame-runtime/manifest.json) は外部参照と GUID の差を保持。[未登録の Tool クラスを含む Graph](../../../work/original-gui/UI11-12-container-toolgraph/20261111T082559380067Z/inputs/missing-tool/manifest.json) は読込時の表示とエラー確認に使う |
| DLS の PCM 素材 | [Wave エディターの Project](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/wave-editor/manifest.json) は Wave を開く操作、[Instrument 入り Project](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/instrument-saved/manifest.json) は Region と楽器の読込、[Tone.wav](../../../work/original-gui/UI07-14-archive/20261111T080241498238Z/inputs/pcm-import/manifest.json) は空の Collection への挿入に使う |
| 2文書・無効値・欠損参照の専用コピー | 対象文書の切替、Undo の対象、取消やエラー時の回復、確認ダイアログ |

## 完了条件と継続手順

- ケースは「未観測」「観測中」「原版確認済み」「追試待ち」「障害あり」「非該当」で記録する。原版確認済みには、入力と環境、画像付きの手順、前後状態、該当する取消し・境界の結果が必要。画面の見た目と操作の挙動をそれぞれ再現できる記述にする。
- 段階1〜3の完了時に、Segment のトラック一覧と Properties の全ページを照合し、未確認の入口、描画、カーソル、ドロップ、境界、文書間差を明記する。段階4では各 UI 群で到達可能な画面を照合し、見つからない入口は確認した操作と依存条件を記録する。
- 1回の作業を終えるたび、画像のリンク・表示、サイズ、撮影時の状態とキャプションの一致、同じ入力からの再現、試験コピーへの保存先を確認する。次回の入口、入力、狙う状態をこのページに追記する。ツールや素材に障害が出た場合は、起きた箇所と影響範囲を記録し、独立して進められるケースを続ける。

### 原版での再調査が残る項目

[Add New Tracks の17種類](seg-01-creation.md#3-add-new-tracks-の選択と取り消し) はすべて追加または編集した画面がある。各資料に境界条件と次の操作を記録した。[Segment Properties](seg-10-properties.md) の4ページは撮影済み。Length、Start Time、Loop の主な入力境界と保存値を照合した。Extension bars とピックアップは Sequence Part の描画、音符の編集、再読込を確認。Boundary 下端の Timing と Cut Off の選択を保存値と照合し、After Prepare Time での先頭再生ではピックアップ音を録音した。Immediate・Quick Response での Cut Off は3択を録音で比較した。Sequence の先頭再生では、自然終了・有限 Repeat・Infinite を録音し、ピックアップ、ループの内外、末尾音の扱いを比較した。4タブの Tab 順を有効欄が変わる条件で比較し、Info と GUID の入力・保存・再読込を確認した。6/8と混在拍子で Start at と Clocktime の丸めを比較し、Clocktime の端数小節で終端を越えた Start が保存されること、同じ Start で小節指定と時間指定の再生が異なることを確認した。直接 Transition では Immediate・Barline と Timing を組み合わせ、切替位置とピックアップ音を録音した。Pattern Part と Runtime での追加小節、Marker や Align を使う切替、テンポが変わる素材や Clocktime の分欄の極値、重複 GUID の2文書を別プロセスで開く場合、原版の応答停止・異常終了の再現条件は追試に残る。 [入力境界の記録](seg-10-properties-bounds.md) · [追加小節とピックアップ](seg-10-length-pickup.md) · [Boundary 下部と再生の記録](seg-10-boundary-prepare-time.md) · [Cut Off と切替時の発音](seg-10-boundary-cutoff.md) · [先頭再生とループ時の録音](seg-10-extra-bar-playback.md) · [Tab 順と Info・GUID](seg-10-info-keyboard.md) · [拍子と Clocktime の境界](seg-10-meter-clock.md) · [切替位置とピックアップの録音](seg-10-transition-pickup.md)

| 順 | 原版で確認すること | 再開に使う資料 |
| --- | --- | --- |
| 1 | Pattern の他の Play モード・別起動時の Reset の比較・Variation Choices の修飾キーと貼り付け・Enter の Chord Change 条件と前後位置・Part ごとの録音と CC の発音差・修飾キーと押下中のカーソル・Part タブ下部の値変更、Style の Seed の上限・複数 Style とループ・同値の OK による再適用、Chord Path の複数分岐の優先順・押下中のカーソル・新規接続の別起動後の復元、Parameter Control の修飾キー・押下中のカーソル・Start From Current・対象のない AudioPath と対象のある ToolGraph・曲線ごとの発音と重なりの適用順、Segment Trigger の修飾キー付きドラッグと押下中のカーソル、Wave の Tree ドロップでの複数 Variation と子ノードの成立条件・押下中のカーソル・隠れた波形とドロップの関係・Variation 途中切替の正確な発音境界（発音前・発音中・折り返し前後の8条件は照合済み）・DLS Region の短いノートと別の Volume Release での終端側の発音、Script の修飾キーによる追加選択とドラッグ、押下中のカーソル、通常ドラッグの成立条件、設計用と内容が異なる .spt および依存を持つ .spt の取込 | [Reset と単独試聴](seg-14-variation-order.md) · [Variation 再生順と途中切替](seg-14-variation-playback.md) · [CC と Var Switch Point](seg-14-pattern-controls.md) · [Pattern Part](seg-14-pattern-part.md) · [Pattern と Style](seg-14-pattern-style.md) · [Seed Variations](seg-14-style-seed.md) · [Chord Path](seg-14-chord-path.md) · [Parameter Control](seg-14-parameter-control-followup.md) · [Segment Trigger](seg-14-segment-trigger-followup.md) · [ドラッグと貼り付け](seg-14-trigger-drag.md) · [同一 Tick の発音](seg-14-trigger-simultaneous.md) · [Align と Start at next marker](seg-14-trigger-alignment.md) · [子の Enter Switch Point](seg-14-trigger-enter.md) · [Pattern Part の Enter](seg-14-trigger-variation-enter.md) · [Primary と Controlling Segment](seg-14-trigger-primary-control.md) · [Segment と Motif の同時刻の出力](seg-14-trigger-motif-overlap.md) · [Wave](seg-14-wave-track-followup.md) · [Tree ドロップと重なり](seg-14-wave-drop-overlap.md) · [複数 Variation と重なりへの挿入](seg-14-wave-tree-variation-drop.md) · [重なりと同時発音](seg-14-wave-overlap-audio.md) · [Variation の発音](seg-14-wave-variation-audio.md) · [発音前・発音中・折り返し前後の切り替え](seg-14-wave-live-variation-boundary.md) · [先行テンポと Alternate Tempo](seg-14-wave-tempo-context.md) · [再起動後のテンポと Snap To Zero](seg-14-wave-tempo-restart.md) · [Wave 文書のループと継承](seg-14-wave-document-loop.md) · [Wave エディターの設定と試聴](seg-14-wave-editor-loop-options.md) · [DLS Region の Note Off](seg-14-dls-note-release.md) · [参照元ループ値の更新](seg-14-wave-reference-refresh.md) · [Script の選択](seg-14-script-track-followup.md) · [Script の Timing](seg-14-script-track-timing.md) · [Belongs To と発音・Trace](seg-14-script-belongs.md) · [FarmMusic とスクリプト言語](seg-14-farmmusic-playback.md) · [Script の言語切替](seg-14-script-language-ui.md) · [ドラッグと時刻編集](seg-14-script-track-gestures.md) · [同一拍の選択、ルーラーとドラッグ](seg-14-script-selection-drag.md) · [別文書への貼り付け](seg-14-script-track-cross-document.md) · [複数ルーチンの貼り付けと範囲端](seg-14-script-paste-range.md) · [別 Project の参照解決](seg-14-script-project-reference.md) · [Script ファイルの取込](seg-14-script-file-picker.md) · [Script 候補の自動選択](seg-14-script-candidate-order.md) · [Runtime Script の取込](seg-14-script-runtime-picker.md) · [貼り付け後の発音と Trace](seg-14-script-paste-audio.md) · [同じ Segment 内の2本の Script Track](seg-14-script-cross-track.md) |
| 2 | ノート・CC の押下中と移動、ヘッダー境界と複数グループ、ループ端点、画面幅・DPI の差、Save As、異なるグループへの貼り付け、参照先の移動・欠落と複数文書の再生音 | [ノート](seg-07-note-gestures.md) · [CC](seg-08-cc-curves.md) · [ヘッダー](seg-04-track-headers.md) · [ループ](seg-12-loop-boundary.md) · [再読込](seg-13-reopen.md) · [複数文書](seg-15-multiple-documents.md) |
| 3 | 本体のメニューとドッキング、専用エディターの選択・移動・取消、Transport 補助画面と出力・実音声、補助アプリ、導入・解除画面の入口 | [アプリ全体の調査表](#アプリ全体の調査表) · [Transport](ui-13-transport-export.md) · [補助アプリ](ui-14-companion-apps.md) |
| 4 | 別の作業コピーで主要ケースの画面、操作、保存・音声を照合して、各ケースの状態を更新 | [実施順序](#実施順序) の段階5 |
