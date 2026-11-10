#pragma once
#include "producer/script_runtime.h"
#include "producer/segment_trigger_playback.h"
#include "compat/playback_runtime.h"
#include <memory>
namespace producer::runtime {
struct Track:IUnknown {
 virtual HRESULT STDMETHODCALLTYPE Init(Segment*)=0;
 virtual HRESULT STDMETHODCALLTYPE InitPlay(SegmentState*,Performance*,void**,DWORD,DWORD)=0;
 virtual HRESULT STDMETHODCALLTYPE EndPlay(void*)=0;
 virtual HRESULT STDMETHODCALLTYPE Play(void*,LONG,LONG,LONG,DWORD,Performance*,SegmentState*,DWORD)=0;
 virtual HRESULT STDMETHODCALLTYPE GetParam(REFGUID,LONG,LONG*,void*)=0;
 virtual HRESULT STDMETHODCALLTYPE SetParam(REFGUID,LONG,void*)=0;
 virtual HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID)=0;
 virtual HRESULT STDMETHODCALLTYPE AddNotificationType(REFGUID)=0;
 virtual HRESULT STDMETHODCALLTYPE RemoveNotificationType(REFGUID)=0;
 virtual HRESULT STDMETHODCALLTYPE Clone(LONG,LONG,Track**)=0;
};
}
namespace producer::app {
struct NativeScriptCall {std::uint64_t playbackId=0;std::array<std::uint8_t,16> objectId{};std::wstring routine;LONG start=0,physical=0,logical=0,dispatchClock=0,invokeClock=0;DWORD timing=0;ScriptResult result;bool messageVisible=false;};
class NativeScriptTrackRuntime {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 NativeScriptTrackRuntime(const std::vector<TriggeredScriptSnapshot>&,const std::vector<runtime::Script*>&);
 ~NativeScriptTrackRuntime();
 NativeScriptTrackRuntime(const NativeScriptTrackRuntime&)=delete;
 void attach(runtime::Segment*,const Bytes&);
 static Bytes loader_bytes(const Bytes&);
 bool overflow()const;
 void cancel();
 std::vector<NativeScriptCall> calls()const;
};
}
