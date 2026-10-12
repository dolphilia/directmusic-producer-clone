# Wave項目の明示ループ指定と再生

2026-10-10（日本時間）。候補1825（20261009T182543095Z）で、Segment内の選択Wave項目にループを設定するAPIとWave／Loopタブを接続した。関連8試験、本体操作、native保存、別process復元、三通常exit0は限定成立した。固定音声は初回発音が1.014秒で1秒基準を超えたため不合格。原版との外観・操作比較も未完で、単位未完・fullAcceptance=falseを維持する。

[一次単位記録](../../work/analysis/ui-wave-item-loop/20261009T181700320Z/unit-record-v2.json)に保存ソース、236ソースのbuild/install、4出力、入力、操作、録音、判定器、processをhashで結び付けた。v1の音声件数表記18/19はv2で17/18へ訂正した。判定や閾値は変更していない。

HelpのWave／Loop／Performanceタブと公開SDKのwaihに基づき、Loop checkbox、開始／終了sample、長さを既存Wave配置・Trim画面へ追加した。0/0は明示offとして扱い、source WaveのWSMPを自動継承しない。inclusive endはHelpの「最後のsample」からの推論で、原版の動的観測ではない。等しいstart/endのsingle-frame encodingやdefault継承は未確定として残す。

編集は選択group／nth track／part／itemを保持し、waihの52／56 DWORDのみを変更する。参照、flags、reserved、拡張、source PCM／WSMPは不変。不正順序・数字・PCM範囲を文書公開前に拒否し、dirty／UndoRedoを保つ。wave-trackの103項目（旧68）でatomic変更、履歴、native保持、境界、group／clipboard／保存復元を確認した。

GUI14、native5、200%表示の候補確認6は限定成立。40秒duration、44100〜88199sampleのループ、enable／disable／UndoRedo、no-op、三不正入力と既存Redo保持、Applyせず閉じた値の取消しを操作し、保存後の別processで値と空の履歴を確認した。author PID12032、reload PID452、対照PID20320は各launch記録で正常終了0に結び付く。

固定WASAPIの18判定中17項目は成立した。初回は固定1秒窓でonset未検出、別の非合格化診断で1.014秒を観測。replayは0.223秒、441Hzループはsourceの5秒を超えて6／12秒でも続き、発音中Stopと停止後無音を確認した。0/0対照は441Hzから882Hz tailへ進み、source終端後は無音になった（対照10項目成立）。既存1446／1713／1734の失敗を上書きしない。

原版の選択項目から共有modeless PropertiesへF11／右クリックで開く入口と、現在のFileメニューmodalには差分がある。Performance、音楽時刻、Length Lock、shared Timeline、字体／色／icon／配置、圧縮／streaming／playlist／SMPL、Q2、全40／全8は残る。原版approval timeoutと個別Windows5／clipboard障害は条件変更なしで保持する。次はLyric／Markerの子control focusからF11／contextで共有Propertiesへ到達する操作不足を検証し、Waveの初回遅延は独立した未達として残す。
