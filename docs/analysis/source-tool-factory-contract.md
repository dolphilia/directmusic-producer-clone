# 本体Source Tool供給・Parameter発見の限定契約

対象はToolGraphDesigner.ocx、ParamStripMgr.dll、Conductor.dll、DMUSProd.exe。既存graph/parameter保存モデル、explicit factory実再生、AudioPath graph接続を維持し、本体factory未供給とobject追加不能を閉じる。全体未完了。

原版help `toolgraphdesigner.htm` は標準Toolを同梱せず、custom Toolが必要と記す。`managingtools.htm` はSegment/AudioPath毎に一つのgraph、順序とPChannel routingを定める。`toaddaparametertotheparametercontroltrack.htm` はTool/DMO、stage、object、parameterを選ぶ入口。今回の原版New dialogはProjectだけでToolGraph種別を提供せず、動的相互比較は障害。証拠は `work/analysis/q3-tool-factory/20261006T215905Z/original-new-types.json`。過去の同designer障害を再試行し続けない。

製品固有のSource Velocity Toolを明示供給する。class GUIDは `{367B8933-F10B-4DCE-A5C5-7D095CC8EC35}`、原版/test Tool94の別名にしない。Note PMsgのvelocityを0..1 gainで乗算し丸める。Graphのrouting/orderをOSに委ね、StampPMsg/requeueする。DMTL `data` はversion DWORD1+float neutral gainの8bytes、payloadなしは1。異常propertiesは既存再生を止める前に拒否する。未知classの自動COM探索はしない。製品paletteとgain propertiesはnative graph履歴/保存へ接続する。

IMediaParams/IMediaParamInfo/IPersistStreamを提供する。parameter0はfloat Velocity gain、範囲0..1、jump/linear、music clock（実DirectMusicが渡す768 PPQのみ）。note時刻に評価しgap/終了後は前の終端値を保持する。標準/current/neutral開始flagsを受け入れ、両flags併記は拒否する。新しい曲線の全batchを検証してから更新する。SetParamはDWORD_ALLPARAMSを許可し、現在のenvelopeを削除しない（[Microsoft SetParam](https://learn.microsoft.com/en-us/previous-versions/ms808115(v=msdn.10))）。最初の実OS試験では誤って分解能0/1しか受け入れず、8音すべてneutral合成48となった。失敗と実OSのdata=768はruntime-diagnosis.jsonに保存し、期待する12/36を変更せずに修正する。reference time、他shape、部分flush/重なりの全SDK比較、全Stop/replayは残責務。

能力情報・範囲・curveは [Microsoft Parameter Information](https://learn.microsoft.com/en-us/windows/win32/directshow/parameter-information)、gap/開始flagsは [Microsoft Envelope Segments](https://learn.microsoft.com/en-us/windows/win32/directshow/envelope-segments) と保存Windows SDK `medparam.h` を根拠にする。これらのinterfaceはToolでも実装可能（[Media Parameters](https://learn.microsoft.com/en-us/windows/win32/directshow/media-parameters)）。Source Toolの製品GUID/propertiesは原版仕様として扱わない。

発見は選択Segmentのembedded AudioPath graphの明示source factoryだけを検査し、IMediaParamInfoからparameter名/範囲/shapeを取得する。AudioPath Tool stage0x2300とclass/instance/PChannelを保持し、同一classのinstanceを区別する。Parameter画面の追加/削除/curve操作は既存Segment単一履歴。既知source capabilityの範囲外・unsupported shape/flagsをprivate変更前に拒否し、他の未知objectは既存raw編集経路とbytesを保持する。DMO/buffer、Transport default発見、Producer custom property ABI、全Timelineはqueueに残す。

終了条件はnative不正入力/保存/history/能力と実OS automation、固定製品本体の作者保存/通常終了/別復元、WASAPI自動解析、登録群一巡。原版の動的比較障害・Q2未用意・全40/全8未完を区別し、限定成立で全体を完了にしない。
