# Current Tempoのprimary有無表示 — 2026-11-09

103640はprimaryなしの無効120、Tempoなしprimaryの無効空欄、Tempoあり132.25の編集可能表示を原版の契約に合わせた。関連4件238検査。空2676/作者6452/復元21216は通常exit0。primaryだけ切替えても編集文書/空履歴を保持し、Save Fileは編集中Seedだけを書いた。Transport再表示とNoTempo編集窓Close後も空欄を保持。2文書native保存/別復元/再保存を通し、両Segment全bytesとSeed17音符、Projectの既存参照を保持。native100は50合格2855検査、既知OS49件と匿名Band1件はPIDなし。入力30（7directory）/292ソース/4出力一致。表示修正なので新PCMなし・過去音声合格の転用なし。

変更はTransportToolbarの2ソースだけ。既存primary bindingのcanPlayからnoPrimaryを保持し、optional BPMがない場合の120 fallbackをprimaryなしに限定した。Tempoなしのprimaryは空欄。positive BPMのformat・edit/spin enabled条件・選択/runtime/編集モデル/keyboardは維持。100901との292 saved-source照合で290は同一。

作者は既存TimeSig-only Segment1の専用ProjectへGUI InsertFilesで132.25/17音符のSeedを追加した。Transport Listとeditor pickerが独立したまま、Save Fileの前後mtimeではSeedだけが更新された。NoTempo editorを開いてView Transport hide/show、その最後のeditorCloseも空欄を保持。最後Closeでlegacy formが出る既知の見た目差は、この単位の合格には含めない。

初期・作者・復元のSegment全bytesが一致。Project初期の1参照を保存時刻以外保持してSeed参照を追加し、作者/復元の2参照ProjectはFILETIME以外一致。表示変更だけで音楽データや再生経路を変えていないため、新しい音声検証は不要。既存PCMをこの候補の音声合格へ転用しない。

一次入口：`work/analysis/ui-tempo-target-display/20261109T103529762Z/unit-record-v2.json`、`gui-proof-v1.json`、`native-proof-v1.json`、`native-milestone-proof-v1.json`。

SSP01→SSP02のエディター開閉を継続。原版15872の固定証拠では、Project初期はprimary未登録、TimeSig-only編集窓を開くとAddNewTracks、最後Close後はgray workspaceのままprimary/Play/空欄を保持し、再OpenでもAddNewTracks。次は最後Closeでlegacy formを出さずgrayを保持する独立修正を優先し、関連回帰/本体/保存/通常終了/別復元まで検証する。初期primary登録とTimeSig-only reopen promptは別未達として扱う。全40/全8/fullAppearance/fullAcceptance=false、49native/30driver凍結と匿名Bandの同条件再試行なし。正式原版15872通常exit0・通知手動対応待ちは解消済み。 goal active。
