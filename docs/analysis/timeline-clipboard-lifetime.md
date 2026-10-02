# Timeline のクリップボード寿命とアンロード判定

対象は原版Timeline.dll 5.3.0.900、SHA-256 `bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176`。2026-10-02に静的なカウンター更新の非対称を確認した。生存COM参照の漏れを動的に証明したという意味ではない。

原版と候補Tempoを同じTimelineへ接続したクリップボード試験では、4機能ケースが完了しTempoのDllCanUnloadNowはS_OKだった。一方、TimelineはmanagerのRelease後もS_FALSEを返した。原版試験 `work/reference/tempo/20261002T085046178Z` ではOleUninitialize後もS_FALSE。失敗runはそのまま保持する。

## 静的に確認した関係

TimelineのDllCanUnloadNow RVA0x5d6aはMFCのordinal1131を呼び、その結果が0の場合、RVA0x1e2dcのDWORDが0であることも要求する（比較命令RVA0x5d86）。したがってS_FALSEだけで生存COM参照があるとは判定できない。

| 経路 | 根拠RVA | 観測した命令と責務 |
| --- | --- | --- |
| TimelineDataObjectの生成 | 0x15fdb、0x15fe4 | カウンターのアドレスをInterlockedIncrementへ渡す |
| TimelineDataObjectの破棄 | 0x15913、0x15918 | 同じアドレスをInterlockedDecrementへ渡す |
| Export、data vtable0x2a74 slot10 | 0x16224 | 新しいCDllJazzDataObjectを生成する |
| Export内の追加増分 | 0x1627e、0x16283 | カウンターをInterlockedIncrementする |
| ExportしたIDataObjectのRelease | 0x1945b→0x19750 | オブジェクト内+4の参照数を減らし、0ならvtable slot12へ進む |
| CDllJazzDataObjectの削除デストラクター | 0x196b3→0x193d0 | CBaseJazzDataObjectのデストラクター0x19a29へ進み、必要ならメモリを解放 |
| ベースの生成・破棄 | 0x19ba9、0x19a29 | オブジェクト内参照数、列挙器、形式・ストリームを管理するが、同じグローバルカウンターの直接更新はない |

この追加増分に対する直接の減算がExportしたオブジェクトの破棄経路にないため、今回のS_FALSEは原版のアンロード用カウンターが残る挙動と整合する。後述の動的観測で残るカウンター値も確認した。間接呼出し先すべて、メモリの最終解放はまだ観測していない。「COM参照漏れの確定」とは区別して調べる。

根拠の再確認は読取専用 `scripts/Inspect-TimelineClipboardLifetime.mjs` で行う。固定バイナリのSHA-256、14箇所の命令バイト、3つのvtable slot、名前付きInterlockedインポートを照合し、`work/analysis/pe/app__Timeline.dll/clipboard-lifetime.json` を出力する。原版DLLのコード・データは書き換えない。

```powershell
node scripts/Inspect-TimelineClipboardLifetime.mjs
```

## 動的観測と復元処理の修正

比較ハーネスに各Copy/Cutの前後とmanager解放後のカウンター読取を追加した。固定した原版TimelineのRVA0x1e2dcを読むだけで、減算や値の補正はしない。生成物SHA-256 `7d38bee268ec7d1691ec377f14a36695c89541250ec5bae926a3d8e5e12b3fea` を実行した原版run `20261002T091106613Z` では4回とも増分1を観測したが、クリップボードを復元した直後、追加したOleFlushClipboardの完了ログ前にexitCode=-1073740771で異常終了した。過去の起動拒否と今回の実行・異常終了を区別して保持する。

OleGetClipboardから得たIDataObjectを再登録してFlushする復元方法を廃止した。同オブジェクトは現在のクリップボードを読むビューであり、独立したバックアップではない。`tests/native/clipboard_snapshot.h` は対応するメモリ形式をプロセス内に複製し、データ内容をログやファイルへ出さず復元する。bitmap/metafile等の非対応ハンドルを含む場合は、試験開始前に失敗してクリップボードを変更しない。復元時はOpenClipboard下でシーケンス番号と所有プロセスを確認し、他プロセスが置き換えたデータを上書きしない。

修正版プローブSHA-256 `8385809115a48b9e26507535267742ecc3c78b8bbfc4487ddc304f22d651b76b`、原版 `work/reference/tempo/20261002T091325057Z`、候補 `work/candidate/tempo/20261002T091342859Z`。保存36ソースと実行プローブは同一。両runともsnapshotとrestoreがS_OK、4機能ケースが完了し、カウンターは1→2→3→4→5、managerとクリップボード所有者の解放後は4だった。TempoのDllCanUnloadNowはS_OK、TimelineはOLE終了後もS_FALSE。両runのexitCode=1は従来のアンロード受入を失敗した結果で、異常終了ではない。

`work/build/clipboard/counter-feature-comparison.json` は4ケースのログと16通常ファイルの完全一致、4コピーファイルの既知padding以外の一致を確認する。全run成功はfalseのままとする。Export回数に対応する原版カウンターの残存を動的に確認したが、生存COMオブジェクト数やメモリ解放全体の証明とはしない。将来の互換TimelineではExportしたIDataObjectの寿命をソース側のカウンターへ正しく反映させ、原版固有モジュールへの依存を解消する。
