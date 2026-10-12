#pragma once
#include "metronome.h"
#include "compat/playback_runtime.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
namespace producer::app {
// Opt-in bounded timing evidence. Audio callbacks never perform file I/O.
struct MetronomeRuntimeEvent {std::string stage;LONGLONG qpc=0,utcFileTime=0;LONG musicTime=0;DWORD pchannel=0,pitch=0,velocity=0;HRESULT result=S_OK;};
class MetronomeRuntimeTrace {
    mutable std::mutex lock_;std::vector<MetronomeRuntimeEvent> events_;bool overflow_=false;
public:
    MetronomeRuntimeTrace();
    void record(const char*,LONG=0,DWORD=0,DWORD=0,DWORD=0,HRESULT=S_OK)noexcept;
    std::vector<MetronomeRuntimeEvent> events()const;
    bool overflow()const;
};
std::string metronome_runtime_trace_json(const std::vector<MetronomeRuntimeEvent>&,bool overflow);
// A private runtime track. It never appears in serialized Segment documents.
class NativeMetronomeRuntime {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    NativeMetronomeRuntime(runtime::Segment*,runtime::AudioPath*,const Timeline&,const MetronomeSettings&,std::shared_ptr<MetronomeRuntimeTrace> = {},LONG timeOrigin=0,bool countIn=false);
    ~NativeMetronomeRuntime();
    NativeMetronomeRuntime(const NativeMetronomeRuntime&)=delete;
    void settings(const MetronomeSettings&);
    void cancel();
    bool failed()const;
};
}
