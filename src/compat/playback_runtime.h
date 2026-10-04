#pragma once
#include <windows.h>
#include <objbase.h>
#include <cstddef>

// Public DirectMusic ABI, checked against the frozen SDK references listed in
// docs/analysis/sdk-reference-sources.json. These are interface prefixes: no
// implementation, guessed slots, or Producer COM interfaces are used here.
namespace producer::runtime {
// Frozen public dmusici.h: DMUS_SEGF_AFTERPREPARETIME (1 << 10).
inline constexpr DWORD playAfterPrepareTime=0x400;
inline constexpr DWORD playSecondary=0x80,playGrid=0x800,playBeat=0x1000,playMeasure=0x2000,playDefault=0x4000;
// Frozen public dmusici.h, GUID_NOTIFICATION_SEGMENT options.
inline constexpr DWORD segmentStarted=0,segmentEnded=1;
inline constexpr GUID performanceClass={0xd2ac2881,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID loaderClass={0xd2ac2892,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID segmentClass={0xd2ac2882,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID styleClass={0xd2ac288a,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID styleId={0xd2ac28bd,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID collectionClass={0x480ff4b0,0x28b2,0x11d1,{0xbe,0xf7,0,0xc0,0x4f,0xbf,0x8f,0xef}};
inline constexpr GUID collectionId={0xd2ac287c,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID allTypes={0xd2ac2893,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID commandParam={0xd2ac289d,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID commandParam2={0x28f97ef7,0x9538,0x11d2,{0x97,0xa9,0,0xc0,0x4f,0xa3,0x6e,0x58}};
struct CommandParam {BYTE type,groove,range,repeat;};
static_assert(sizeof(CommandParam)==4 && offsetof(CommandParam,repeat)==3);
struct CommandParam2 {LONG time;BYTE type,groove,range,repeat;};
static_assert(sizeof(CommandParam2)==8 && offsetof(CommandParam2,type)==4 && offsetof(CommandParam2,repeat)==7);
inline constexpr GUID performance8Id={0x679c4137,0xc62e,0x4147,{0xb2,0xb4,0x9d,0x56,0x9a,0xcb,0x25,0x4c}};
inline constexpr GUID loader8Id={0x19e7c08c,0x0a44,0x4e6a,{0xa1,0x16,0x59,0x5a,0x7c,0xd5,0xde,0x8c}};
inline constexpr GUID segment8Id={0xc6784488,0x41a3,0x418f,{0xaa,0x15,0xb3,0x50,0x93,0xba,0x42,0xd4}};
struct Music;struct Sound;struct Track;struct Graph;struct Port;struct Instrument;struct DownloadedInstrument;
struct Message;struct NotificationMessage;struct NoteRange;struct ChordKey;struct TimeSignature;struct AudioParams;
struct Performance;struct Segment;
#pragma pack(push,8)
struct Message {
    DWORD size;LONGLONG referenceTime;LONG musicTime;
    DWORD flags,pchannel,virtualTrack;IUnknown* tool;Graph* graph;
    DWORD type,voice,group;IUnknown* user;
};
struct NotificationMessage {
    Message message;GUID notificationType;DWORD option,field1,field2;
};
struct NoteMessage {
    Message message;LONG duration;WORD musicValue,measure;short offset;
    BYTE beat,grid,velocity,noteFlags,timeRange,durationRange,velocityRange,playMode,subChord,midiValue;
    char transpose;
};
#pragma pack(pop)
static_assert(sizeof(Message)==56 && offsetof(Message,referenceTime)==8 && offsetof(Message,user)==52);
static_assert(sizeof(NotificationMessage)==88 && offsetof(NotificationMessage,notificationType)==56 && offsetof(NotificationMessage,option)==72);
static_assert(sizeof(NoteMessage)==80 && offsetof(NoteMessage,duration)==56 && offsetof(NoteMessage,midiValue)==75);
inline constexpr GUID graphClass={0xd2ac2884,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID graphId={0x2befc277,0x5497,0x11d2,{0xbc,0xcb,0,0xa0,0xc9,0x22,0xe6,0xeb}};
inline constexpr GUID toolId={0xd2ac28ba,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
struct Tool: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Init(Graph*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD**,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE ProcessPMsg(Performance*,Message*)=0;
    virtual HRESULT STDMETHODCALLTYPE Flush(Performance*,Message*,LONGLONG)=0;
};
struct Graph: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE StampPMsg(Message*)=0;
    virtual HRESULT STDMETHODCALLTYPE InsertTool(Tool*,DWORD*,DWORD,LONG)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTool(DWORD,Tool**)=0;
    virtual HRESULT STDMETHODCALLTYPE RemoveTool(Tool*)=0;
};
inline constexpr GUID commandNotification={0xd2ac289c,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID segmentNotification={0xd2ac2899,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
struct Collection: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInstrument(DWORD,Instrument**)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumInstrument(DWORD,DWORD*,WCHAR*,DWORD)=0;
};
struct SegmentState: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetRepeats(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetSegment(Segment**)=0;
    virtual HRESULT STDMETHODCALLTYPE GetStartTime(LONG*)=0;
};
#pragma pack(push,8)
struct ObjectDesc {
    DWORD size,valid;GUID objectId,classId;FILETIME date;DWORD versionMS,versionLS;
    WCHAR name[64],category[64],filename[MAX_PATH];LONGLONG memoryLength;BYTE* memory;IStream* stream;
};
#pragma pack(pop)
static_assert(sizeof(void*)==4 && sizeof(ObjectDesc)==848 && offsetof(ObjectDesc,memoryLength)==832 && offsetof(ObjectDesc,memory)==840);
struct Band;struct ChordMap;
struct StyleTimeSignature {LONG time;BYTE beats,denominator;WORD grids;};
static_assert(sizeof(StyleTimeSignature)==8 && offsetof(StyleTimeSignature,grids)==6);
struct Style: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetBand(WCHAR*,Band**)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumBand(DWORD,WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetDefaultBand(Band**)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumMotif(DWORD,WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetMotif(WCHAR*,Segment**)=0;
    virtual HRESULT STDMETHODCALLTYPE GetDefaultChordMap(ChordMap**)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumChordMap(DWORD,WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetChordMap(WCHAR*,ChordMap**)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTimeSignature(StyleTimeSignature*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetEmbellishmentLength(DWORD,DWORD,DWORD*,DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTempo(double*)=0;
};
struct MusicObject;
struct Loader: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetObject(ObjectDesc*,REFIID,void**)=0;
    virtual HRESULT STDMETHODCALLTYPE SetObject(ObjectDesc*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetSearchDirectory(REFGUID,WCHAR*,BOOL)=0;
    virtual HRESULT STDMETHODCALLTYPE ScanDirectory(REFGUID,WCHAR*,WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE CacheObject(MusicObject*)=0;
    virtual HRESULT STDMETHODCALLTYPE ReleaseObject(MusicObject*)=0;
    virtual HRESULT STDMETHODCALLTYPE ClearCache(REFGUID)=0;
    virtual HRESULT STDMETHODCALLTYPE EnableCache(REFGUID,BOOL)=0;
};
struct Segment: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetLength(LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetLength(LONG)=0;
    virtual HRESULT STDMETHODCALLTYPE GetRepeats(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetRepeats(DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetDefaultResolution(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultResolution(DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTrack(REFGUID,DWORD,DWORD,Track**)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTrackGroup(Track*,DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE InsertTrack(Track*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE RemoveTrack(Track*)=0;
    virtual HRESULT STDMETHODCALLTYPE InitPlay(SegmentState**,Performance*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetGraph(Graph**)=0;
    virtual HRESULT STDMETHODCALLTYPE SetGraph(Graph*)=0;
    virtual HRESULT STDMETHODCALLTYPE AddNotificationType(REFGUID)=0;
    virtual HRESULT STDMETHODCALLTYPE RemoveNotificationType(REFGUID)=0;
    virtual HRESULT STDMETHODCALLTYPE GetParam(REFGUID,DWORD,DWORD,LONG,LONG*,void*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetParam(REFGUID,DWORD,DWORD,LONG,void*)=0;
    virtual HRESULT STDMETHODCALLTYPE Clone(LONG,LONG,Segment**)=0;
    virtual HRESULT STDMETHODCALLTYPE SetStartPoint(LONG)=0;
    virtual HRESULT STDMETHODCALLTYPE GetStartPoint(LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetLoopPoints(LONG,LONG)=0;
    virtual HRESULT STDMETHODCALLTYPE GetLoopPoints(LONG*,LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetPChannelsUsed(DWORD,DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetTrackConfig(REFGUID,DWORD,DWORD,DWORD,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetAudioPathConfig(IUnknown**)=0;
    virtual HRESULT STDMETHODCALLTYPE Compose(LONG,Segment*,Segment*,Segment**)=0;
    virtual HRESULT STDMETHODCALLTYPE Download(IUnknown*)=0;
    virtual HRESULT STDMETHODCALLTYPE Unload(IUnknown*)=0;
};
struct AudioPath: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetObjectInPath(DWORD,DWORD,DWORD,REFGUID,DWORD,REFGUID,void**)=0;
    virtual HRESULT STDMETHODCALLTYPE Activate(BOOL)=0;
    virtual HRESULT STDMETHODCALLTYPE SetVolume(LONG,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE ConvertPChannel(DWORD,DWORD*)=0;
};
struct Performance: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Init(Music**,Sound*,HWND)=0;
    virtual HRESULT STDMETHODCALLTYPE PlaySegment(Segment*,DWORD,LONGLONG,SegmentState**)=0;
    virtual HRESULT STDMETHODCALLTYPE Stop(Segment*,SegmentState*,LONG,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetSegmentState(SegmentState**,LONG)=0;
    virtual HRESULT STDMETHODCALLTYPE SetPrepareTime(DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetPrepareTime(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetBumperLength(DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetBumperLength(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SendPMsg(Message*)=0;
    virtual HRESULT STDMETHODCALLTYPE MusicToReferenceTime(LONG,LONGLONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE ReferenceToMusicTime(LONGLONG,LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE IsPlaying(Segment*,SegmentState*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetTime(LONGLONG*,LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE AllocPMsg(ULONG,Message**)=0;
    virtual HRESULT STDMETHODCALLTYPE FreePMsg(Message*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetGraph(Graph**)=0;
    virtual HRESULT STDMETHODCALLTYPE SetGraph(Graph*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetNotificationHandle(HANDLE,LONGLONG)=0;
    virtual HRESULT STDMETHODCALLTYPE GetNotificationPMsg(NotificationMessage**)=0;
    virtual HRESULT STDMETHODCALLTYPE AddNotificationType(REFGUID)=0;
    virtual HRESULT STDMETHODCALLTYPE RemoveNotificationType(REFGUID)=0;
    virtual HRESULT STDMETHODCALLTYPE AddPort(Port*)=0;
    virtual HRESULT STDMETHODCALLTYPE RemovePort(Port*)=0;
    virtual HRESULT STDMETHODCALLTYPE AssignPChannelBlock(DWORD,Port*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE AssignPChannel(DWORD,Port*,DWORD,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE PChannelInfo(DWORD,Port**,DWORD*,DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE DownloadInstrument(Instrument*,DWORD,DownloadedInstrument**,NoteRange*,DWORD,Port**,DWORD*,DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE Invalidate(LONG,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetParam(REFGUID,DWORD,DWORD,LONG,LONG*,void*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetParam(REFGUID,DWORD,DWORD,LONG,void*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetGlobalParam(REFGUID,void*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE SetGlobalParam(REFGUID,void*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE GetLatencyTime(LONGLONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetQueueTime(LONGLONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE AdjustTime(LONGLONG)=0;
    virtual HRESULT STDMETHODCALLTYPE CloseDown()=0;
    virtual HRESULT STDMETHODCALLTYPE GetResolvedTime(LONGLONG,LONGLONG*,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE MIDIToMusic(BYTE,ChordKey*,BYTE,BYTE,WORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE MusicToMIDI(WORD,ChordKey*,BYTE,BYTE,BYTE*)=0;
    virtual HRESULT STDMETHODCALLTYPE TimeToRhythm(LONG,TimeSignature*,WORD*,BYTE*,BYTE*,short*)=0;
    virtual HRESULT STDMETHODCALLTYPE RhythmToTime(WORD,BYTE,BYTE,short,TimeSignature*,LONG*)=0;
    virtual HRESULT STDMETHODCALLTYPE InitAudio(Music**,Sound**,HWND,DWORD,DWORD,DWORD,AudioParams*)=0;
    virtual HRESULT STDMETHODCALLTYPE PlaySegmentEx(IUnknown*,WCHAR*,IUnknown*,DWORD,LONGLONG,SegmentState**,IUnknown*,IUnknown*)=0;
    virtual HRESULT STDMETHODCALLTYPE StopEx(IUnknown*,LONGLONG,DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE ClonePMsg(Message*,Message**)=0;
    // Public IDirectMusicPerformance8 slots, after ClonePMsg.
    virtual HRESULT STDMETHODCALLTYPE CreateAudioPath(IUnknown*,BOOL,AudioPath**)=0;
};
}
