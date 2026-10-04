

## 2026-10-04 articulation文書編集

現行040836168Z保存63sources、configure/build/install0、DLS articulation21checks/独立raw/host smoke0。Instrument/Region所有を区別し、art1/art2・lart/lar2の複数blockを順序保持、符号付きrawScaleと可変接続件数を編集、明示block追加、ヘッダー拡張/未知tail/cdl/兄弟chunkを保持、UndoRedo/拒否原子性/別Framework保存復元全bytespassed。原版保存after-instrument.dlp内lar2/art1をfixture比較したが、原版動的編集/単位変換/GUI/音声は未実行。旧024829130Z GUI音声合格は旧版記録として保持し、新版へ転用しない。全core/full40/full8未完了。 次：Connect typed Instrument/Region articulation to current source GUI with explicit block ownership and signed raw fields; observe original envelope/pitch/filter units before presentation conversion; save/UndoRedo/separate reload, then owned-DLS digital playback comparison. Native ndsc/full40/full8 remain. 記録：work/analysis/dls-articulation/20261004T041300Z/report.md。全体範囲・受入条件を維持。


## 2026-10-04 articulation GUI接続

現行041847582Z保存65sources構成/build/install0、articulation21/raw/host smoke0。DLS画面ArticulationボタンからInstrument/Region所有・block・接続を選ぶraw編集画面を接続。PID15788 GUIでInstrument1 Scale2147483647→-65536、Save/UndoSave/RedoSave全bytes一致（Scale領域offset366のみ）。GUI104modules原版40hash0。Region CRUD/別起動復元/通常終了/現行音声・単位/曲線・原版動的比較・全core/full40/full8未完了。旧024829130Z音声成功は新版へ転用しない。 次：Retained PID15788 Articulation window55119140: verify Region override Level1/2 block and connection CRUD/UndoRedo/exact saves, close editors, save initial Segment/native Project, normal exit and separate current-build reload/resave; then destination units/envelope UI/original comparison and calibrated owned-DLS audio. Native ndsc/full40/full8 remain. 詳細：work/analysis/articulation-gui/20261004T042000Z/report.md。全対象・全体受入条件は維持する。


## 2026-10-04 Region Articulation GUI保存

現行041847582Z PID15788でRegion1 Level1接続追加/UndoSave/RedoSave、Level2空block作成/保存、独立SaveAs全bytespassed。Instrument接続/GUID/pool/PCM/他bytes保持。構成/build/installは同版前単位0、今回は製品ソース変更・再buildなし。Region接続変更/削除、GUI別起動復元/通常終了、現行音声、単位/曲線、原版動的比較、全40/全8は未完了。 次：PID15788 DLS window462816 saved at articulation-region-gui/20261004T043700Z/Collection.dls. Close DLS; save initial Segment and native Project, normal exit and separate current041847582Z GUI reload/resave. Then Region connection edit/remove and destination units/envelope/original comparison/current calibrated audio; native ndsc/full40/full8 remain. 証拠：work/analysis/articulation-region-gui/20261004T043700Z/report.md。


## 2026-10-04 Articulation別起動復元

現行041847582Zの旧PID15788でDMPJプロジェクト保存・通常終了0、新PID5740で別起動プロジェクト復元、Instrument/Region Level1接続とLevel2空blockのGUI復元、DLS再保存全bytes一致を確認。新PID GUI104依存由来passed。製品ソース変更・再buildなし、構成/build/installは同版65sources前単位0。DMPJ内部形式のみでJAZP受入ではない。現行音声、Region変更/削除、単位/曲線、原版動的比較、native ndsc、全40/全8未完了。 証拠：work/analysis/articulation-reload-gui/20261004T045300Z/report.md。次：PID5740 main34409312 / DLS1052570 / Articulation3870498 retains saved Region Level2. Close Articulation, SaveAs DLS into new independent unit before Region edit/remove. Then advance destination units/envelope and current calibrated audio/original dynamic comparison; native JAZP/ndsc and remaining full40/full8 must progress.


## 2026-10-04 Articulation単位編集

現行050923258Z保存66sourcesの構成/build/install0、Articulation関連35checks/独立raw、host smoke0、GUI cents切替/no-op全bytes/-1200.25編集/UndoSave/RedoSave全bytespassed。新PID20192 GUI104 modules由来passed、原版40hash一致0。cents/timecents/centibelsの16.16編集を実装、未知先とSustainはraw保持。現行別起動復元/音声、Region変更削除、Envelope曲線/単位全対応、原版動的比較、native ndsc、全core/full40/full8未完了。 証拠：work/analysis/articulation-units-gui/20261004T051200Z/report.md。次：Current PID20192 main200732 / DLS332180 / Articulation1577320 retained saved Instrument1 cutoff -1200.25 cents at this unit Collection.dls. Preserve frozen Edited/Redone. Next observe original envelope controls and add named EG1/EG2 editor with ownership/unknown connections retained, then integrate current owned-DLS calibrated audio and project reload. Region edit/remove, native JAZP/ndsc and remaining40/all8 stay required; do not spend next turn only expanding the raw converter auditor.


## 2026-10-04 名前付きEnvelope定数

現行052814478Z保存67sourcesの構成/build/install0、Articulation関連43checks/独立raw、host smoke0、GUI Region EG1 Decay名前付き追加/timecents -1200.25/保存UndoRedo全bytespassed。PID20084 GUI104 modules由来passed、原版40hash一致0。EG1/EG2定数を選択した所有Instrument/Region/blockへ接続、変調・未知接続を保持、重複定数/Level2のart1適用を拒否。原版Instrumentエディターunsupported operationで画面比較blocked、同条件再試行禁止。現行別起動project復元/音声、曲線編集/seconds/percent/変調設定、native JAZP/ndsc、全core/full40/full8未完了。 証拠：work/analysis/dls-envelope/20261004T052200Z/report.md。次：Preserve PID20084 main29430312 / DLS528962 / Articulation7737338 and frozen Edited/Redone. First improve named selection presentation: initial Attack label and refresh can refer to current raw Connection, require reselect; disable generic Apply while named values are staged, select actual applied constant after Set. Then save/reload current project and integrate owned-DLS calibrated WASAPI audio; verify Envelope effect in recorded signal without listening questions. Original editor failed in earlier unit and this unit: do not repeat same conditions; diagnose independently. Region edit/remove, native JAZP/ndsc and all40/all8 remain required.


## 2026-10-04 Envelope選択と本体復元

現行054148406Z保存67sources構成/build/install0、Articulation43checks/独立raw/host smoke0。名前付きEnvelopeモードと汎用Connection編集を分離し、選択した定数の値と接続番号をrefresh/Set/Save/UndoRedo後も維持。GUI Region Decay -1200.25→-2400.5 timecents保存/UndoSave/RedoSave、PID14408通常終了0、別PID14488のDMPJ復元/再保存全bytespassed。GUI123 modules原版40hash一致0。現行音声/原版動的比較/native JAZP・ndsc/fullcore/full40/full8未完了。 証拠：work/analysis/envelope-selection-gui/20261004T054200Z/report.md。次：Retain saved PID14488 main3150674 / DLS922486 / Articulation4658224 and frozen GUI outputs. Next integrate source-created owned DLS Envelope constants with playable Segment/Band and calibrated WASAPI capture; compare two explicit attack/decay settings by recorded signal, verify Stop/replay without human listening. Use fresh playable input instead of empty Initial.sgp. Original Instrument editor failed: no same-condition retries. Region generic edit/remove, native JAZP/ndsc, remaining40/all8 remain required.


## 2026-10-04 所有Envelope無人録音

現行060504851Z保存67sources構成/build/install0、関連Articulation43/raw/host smoke0。ソース新規PCM/DLS/Region Envelope・Band・Segment/DMPJを接続、EG1 Attack -12000対0 timecentsだけの変更・UndoRedo/別Framework復元を確認。校正WASAPI26秒×2、両設定各再生/再開3音の440HzとAttack立ち上がり差、baseline/Stop hold/final Stop無音passed。Fast比約1.00/Slow比0.139〜0.145、3派生反例拒否、両再生55modules原版40hash0。現行GUI/正常終了再起動/原版動的同等性/native JAZP・ndsc/fullcore/full40/full8未完了。 証拠：work/analysis/envelope-audio/20261004T061200Z/report.md。次：Next open current060504851Z source GUI with the fresh playable Slow Envelope.dmpj, edit Region EG1 Attack via named editor (0 to -12000 timecents), save/UndoRedo, calibrated GUI capture of slow versus edited fast, normal exit and separate reload. Preserve old saved PID14488 and frozen evidence; no listening questions or original unsupported-editor retries. Then advance native JAZP/ndsc document metadata/ownership and remaining40 responsibilities instead of expanding only articulation auditors.


## 2026-10-04 Envelope GUI録音・通常終了

現行060504851ZのGUI PID12460でRegion EG1 Attack0→-12000 timecents、保存/UndoSave/RedoSave全bytes一致、他3文書不変。GUI録音Slow/Fastとも440Hzと立ち上がり差（比0.176〜0.178対約1）/baseline無音を確認。Slowは自然終了後かつ録音後Stopのため不合格を保持。Fast90秒録音は26秒早期Stop/停止後RMS0 passed。本体通常終了0/強制終了なし。127modules/address由来passed/原版40hash0。製品ソース変更・再buildなし、同版保存67sources構成/build/install0、関連43/raw/hostは前単位証拠。別起動GUI復元/GUI再開/native JAZP・ndsc/fullcore/full40/full8未完了。 証拠：work/analysis/envelope-gui-audio/20261004T061400Z/report.md。次：Current PID12460 exited normally; preserve old saved PID14488 and all frozen inputs/recordings. Launch separate current060504851Z GUI and reopen this unit Envelope.dmpj, verify saved Region Attack-12000 and resave exact bytes. Then advance native JAZP/ndsc document metadata/ownership and remaining40 responsibilities; do not expand only Envelope audits. No human listening questions or original unsupported-editor retries.
