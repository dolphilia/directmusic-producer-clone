# 2026-10-11 Sequence複数パート・ドラム・DLS（SSP07）

画像38〜43のAdd New Part、独立した複数piano roll/CC、PCh10のドラム鍵盤名/セット、Other DLS選択を実装中。全体受入は未完了で `fullAcceptance=false`。

開始記録は `work/analysis/ui-sequence-parts/20261010T215317400Z/start-v1.json`。SSP06の本体/native限定成立・PCM14/16・発音中Stop2条件の時刻不備を保持し、独立した未実装入口へ進む。

## 契約と変更

ユーザー画像とCHMのSequence Track/Imported MIDI Segment Structure/Choose DLS Instrumentを照合。新しいパートは選択group内の独立した物理Sequence trackとし、ノート/曲線なしでPChannelを保持する。DWORD PChannel16以上を許容し、予約4値と重複を編集前に拒否する。

原版の非公開 `LIST psql/psqc` は既存静的文書に観測したが、空パートPChannelの保存契約は未解明。新規クローン文書に限り、署名・版番号付きの独自 `qPCh` 設計chunkを使用する。原版chunkを推測して改変せず、runtime seqtに仮ノート/CCを置かない。クローン自身での保存復元と原版読み戻し互換性は別判定。

Add New Partダイアログは反復Create/Close、unused/in-use表示、1-based PChannel入力を接続。独立展開/CCは既存のgroup/physical track/PChannelキーへ結合。PCh10はドラム鍵盤名とセット直下メニューを表示する。鍵盤名はユーザー画像41の観測ラベルと[Windows標準GM鍵盤割当](https://learn.microsoft.com/en-us/windows/win32/multimedia/standard-midi-key-assignments)を根拠とする。

Other DLSは所有project collectionsのコンボとinstrumentリストを表示し、Bank Hi/Lo/Patchを確認できる。割当はlocale/referenceを一回のSegment履歴で変更し、解決前に所有文書へ公開しない。pan/volumeの独立有効状態と他PChannelを保持する。GUIDなしcollectionは保存済み相対参照を要求する。

## 現在の実行結果

候補は `20261010T220927675Z`。configure/build/installはexit0、保存・作業ソース276件と生成物4件が一致し、関連13モード952検査が合格した。変更一覧は `source-changes-v2.json`、同一性は `candidate-identity-v2.json` を参照する。旧220216では試験fixtureのProject名と保存フォルダ名が不一致となりSequence回帰が失敗した。製品の保存条件を緩めず、fixture名を修正して新候補で検証した。旧失敗と回帰選択IDの訂正は `preparation-correction-v1.json` に保持した。

作者PID2932でAdd Sequence Part(s)からPChannel2・10を追加し、まず空状態で保存した。空パートのnative監査11項目が合格し、ノートや仮CCを置かず所有PChannelを保存できた。続いてPChannel2へ所有DLS `Tone.dlp` のPart Tone（bank0/program8）を割り当て、MIDI60・開始0・長さ301056clocksのノートを入力した。PChannel10ではGMドラム鍵盤名を表示し、TR-808を選択してMIDI36・開始0・長さ192のノートを入力した。パート1と2のピアノロールを同時に開き、パート1のCCバーも併置した。

最終保存のnative監査14項目が合格。既存Sequenceと既存4曲線、無関係のトラック、Bandの他14チャンネルを保持し、新規2パートとそれぞれのノート、DLS参照、TR-808のpercussion patchを確認した。SegmentのSHA256は `11ace01f3cb8828d889c3b21d0a41c99426d8d74ee2ae2a6975eadcd740359f9`。作者を通常exit0で終了した後、別PID22852で同じProjectを開き、Part Toneと持続ノート、TR-808とKick Drum 1/MIDI36を復元した。再読込側も通常exit0で終了し、保存ファイルのhash不変を確認した。一次証拠は `main/gui-save-reload-proof-v1.json` と `main/actions.json`。

300秒WASAPI録音は `work/acceptance/product-project-gui/20261010T223623342Z/audio-20261010T224201610Z`。UTC/QPC/packet時計の検証と4つの破損対照を通過し、固定PCM18項目すべてが合格した。初回発音0.893秒、再開0.185秒。既存パートの基準音261.25Hz、PB時274.75Hz（+0.872半音）、Expression低下比0.165・回復比6.029に対して、新規DLSパートは各時点で440Hzを保持した。初回停止は再生から89.087秒、再開後停止は66.804秒で、どちらも196秒持続音の途中。停止区間と最終無音のRMSは約0.00005537、peak約0.00012599だった。判定器・閾値は録音前の `main/gui-audio-protocol-v1.json` から変更していない。初回のコマンドは入力配列の渡し方を誤り、録音開始前に失敗したため、修正理由を `capture-invocation-correction-v1.json` に保存した。

再生中の78モジュールについて実体・PE種別・hashを確認し、既知原版40ファイルとの一致は0。これはその再生時点の観測であり、独立Windows環境のQ2や全機能の依存解消を証明しない。`main/gui-module-proof-v1.json` に監査を保存した。

同候補の節目一巡はnative98件中51合格・2903検査、46OS凍結、既知匿名Band失敗1保留。driver170件中20合格、28OS凍結、122未実行だった。凍結nativeにprocessを起動していないこと、合格nativeの通常exit0、原始一巡へ補足結果を混ぜていないことを確認した。一次runは `work/acceptance/regression/20261010T224754198Z/run.json` と `work/acceptance/registered-drivers/20261010T225050130Z/run.json`。

## 残る不足と継続

原版psql/psqcの空パート保存形式と原版による読み戻しは未確認。qPChはクローン固有の設計情報であり、原版形式の互換完成とは扱わない。spinボタンの方向、空DLS/Cancelの実本体操作、パート間の全UndoRedo/スクロール操作、CCを含む全同時編集、ドラムキットの音色を分離したPCM判定、正確な字体・寸法・配置と原版の同入力比較も未達。GUIの最初の描画直後に一時的な密集線が見えたため、安定後の状態と区別し、原因未確定の製品障害とは断定しない。DLS別窓の再選択、画面外メニューの最大化による観測復旧も操作履歴に残した。

SSP06の旧PCM14/16とStop時刻不備はそのまま残す。今回の異なる長時間・複数パート試行を旧録音の合格へ転用しない。SSP01〜06、独立Style015244、46native/28driver OS凍結、Q2、全40責務/全8受入を保持する。SSP07は限定成立であり、`completed=false`、`fullAcceptance=false`。同候補の節目回帰を確定してから、SSP08の本体連続シナリオへ進む。
