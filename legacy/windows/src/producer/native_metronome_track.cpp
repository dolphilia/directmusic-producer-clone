#include "native_metronome_track.h"
#include "native_script_track.h" // frozen public IDirectMusicTrack prefix
#include "compat/tempo_runtime.h"
#include <dmerror.h>
#include <atomic>
#include <mutex>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <sstream>
namespace producer::app {
MetronomeRuntimeTrace::MetronomeRuntimeTrace(){events_.reserve(4096);}
void MetronomeRuntimeTrace::record(const char* stage,LONG musicTime,DWORD pchannel,DWORD pitch,DWORD velocity,HRESULT result)noexcept{
    try{LARGE_INTEGER qpc{};FILETIME utc{};QueryPerformanceCounter(&qpc);GetSystemTimePreciseAsFileTime(&utc);const auto ticks=(static_cast<LONGLONG>(utc.dwHighDateTime)<<32)|utc.dwLowDateTime;
        std::lock_guard<std::mutex> guard(lock_);if(events_.size()<4096)events_.push_back({stage,qpc.QuadPart,ticks,musicTime,pchannel,pitch,velocity,result});else overflow_=true;
    }catch(...){std::lock_guard<std::mutex> guard(lock_);overflow_=true;}
}
std::vector<MetronomeRuntimeEvent> MetronomeRuntimeTrace::events()const{std::lock_guard<std::mutex> guard(lock_);return events_;}
bool MetronomeRuntimeTrace::overflow()const{std::lock_guard<std::mutex> guard(lock_);return overflow_;}
std::string metronome_runtime_trace_json(const std::vector<MetronomeRuntimeEvent>& events,bool overflow){
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);std::ostringstream out;out<<"{\"schema\":1,\"qpcFrequency\":"<<frequency.QuadPart<<",\"overflow\":"<<(overflow?"true":"false")<<",\"events\":[";bool comma=false;
    for(const auto& e:events){if(comma)out<<',';comma=true;out<<"{\"stage\":\"";for(const auto c:e.stage){if(c=='\\'||c=='\"')out<<'\\';out<<c;}out<<"\",\"qpc\":"<<e.qpc<<",\"utcFileTime\":"<<e.utcFileTime<<",\"musicTime\":"<<e.musicTime<<",\"pchannel\":"<<e.pchannel<<",\"pitch\":"<<e.pitch<<",\"velocity\":"<<e.velocity<<",\"result\":"<<e.result<<'}';}out<<"],\"scope\":\"Runtime scheduling only; PCM and original comparison separate\",\"fullAcceptance\":false}\n";return out.str();
}
namespace {
const GUID clickInterface={0x84f208fa,0xd341,0x4a86,{0xaf,0x48,0x83,0xab,0x76,0xb2,0xea,0x15}};
struct Control {
    std::mutex lock;MetronomeSettings settings;runtime::AudioPath* path=nullptr;
    std::atomic<bool> cancelled{false},failed{false};std::shared_ptr<MetronomeRuntimeTrace> trace;bool countIn=false;
    ~Control(){if(path)path->Release();}
};
struct PlayToken {std::mutex lock;LONG offset=LONG_MIN,covered=0;};
class Click final:public IUnknown {
    std::atomic<ULONG> refs{1};
public:
    bool first;explicit Click(bool value):first(value){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,clickInterface))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
};
class ClickTool final:public runtime::Tool {
    std::atomic<ULONG> refs{1};std::shared_ptr<Control> control;
public:
    explicit ClickTool(std::shared_ptr<Control> value):control(std::move(value)){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,runtime::toolId))return E_NOINTERFACE;*out=static_cast<runtime::Tool*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(runtime::Graph*)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* out)override{if(!out)return E_POINTER;*out=16;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* out)override{if(!out)return E_POINTER;*out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** out,DWORD n)override{if(!out||!*out)return E_POINTER;if(n!=1)return E_INVALIDARG;(*out)[0]=255;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessPMsg(runtime::Performance* performance,runtime::Message* message)override{
        if(!performance||!message||!message->user)return E_INVALIDARG;
        if((message->flags&0x20)||control->cancelled)return DMUS_S_FREE;
        Click* raw=nullptr;if(FAILED(message->user->QueryInterface(clickInterface,reinterpret_cast<void**>(&raw))))return E_INVALIDARG;
        const bool first=raw->first;raw->Release();MetronomeSettings settings;{std::lock_guard<std::mutex> guard(control->lock);settings=control->settings;}
        if(control->trace)control->trace->record(control->countIn?"countin-click-tool-arrived":"click-tool-arrived",message->musicTime,settings.pchannel,first?settings.firstPitch:settings.otherPitch,first?settings.firstVelocity:settings.otherVelocity);
        if(!settings.enabled||control->cancelled)return DMUS_S_FREE;const auto velocity=first?settings.firstVelocity:settings.otherVelocity;if(!velocity)return DMUS_S_FREE;
        DWORD mapped=0;auto hr=control->path->ConvertPChannel(settings.pchannel,&mapped);
        // An intentionally disconnected local PChannel remains silent.
        if(hr==DMUS_E_NOT_FOUND)return DMUS_S_FREE;
        if(hr!=S_OK||mapped>=0xfffffffcu){control->failed=true;return DMUS_S_FREE;}
        runtime::Message* note=nullptr;hr=performance->AllocPMsg(sizeof(runtime::NoteMessage),&note);
        if(SUCCEEDED(hr)){
            std::memset(note,0,sizeof(runtime::NoteMessage));note->size=sizeof(runtime::NoteMessage);note->type=1;note->flags=6;note->musicTime=message->musicTime;note->pchannel=mapped;note->group=0xffffffff;note->virtualTrack=message->virtualTrack;
            auto n=reinterpret_cast<runtime::NoteMessage*>(note);n->duration=48;n->velocity=static_cast<BYTE>(velocity);n->noteFlags=1;n->midiValue=static_cast<BYTE>(first?settings.firstPitch:settings.otherPitch);n->musicValue=n->midiValue;n->playMode=0;
            runtime::Graph* graph=nullptr;hr=performance->GetGraph(&graph);if(SUCCEEDED(hr)&&graph)hr=graph->StampPMsg(note);else if(SUCCEEDED(hr))hr=E_UNEXPECTED;if(graph)graph->Release();
            // The dispatch gate is checked again immediately before transfer.
            {std::lock_guard<std::mutex> guard(control->lock);if(SUCCEEDED(hr)&&!control->cancelled)hr=performance->SendPMsg(note);else if(SUCCEEDED(hr))hr=E_ABORT;}
            if(FAILED(hr))performance->FreePMsg(note);
            if(control->trace)control->trace->record(control->countIn?"countin-click-note-sent":"click-note-sent",message->musicTime,mapped,first?settings.firstPitch:settings.otherPitch,velocity,hr);
        }
        if(FAILED(hr)&&hr!=E_ABORT)control->failed=true;return DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(runtime::Performance*,runtime::Message*,LONGLONG)override{return DMUS_S_FREE;}
};
class ClickTrack final:public runtime::Track {
    std::atomic<ULONG> refs{1};Timeline timeline;runtime::Graph* graph;std::shared_ptr<Control> control;LONG length=0,origin=0;
public:
    ClickTrack(const Timeline& t,runtime::Graph* g,std::shared_ptr<Control> c,LONG timeOrigin):timeline(t),graph(g),control(std::move(c)),origin(timeOrigin){graph->AddRef();}
    ~ClickTrack(){graph->Release();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,producer::IID_DirectMusicTrack))return E_NOINTERFACE;*out=static_cast<runtime::Track*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(runtime::Segment* segment)override{return segment?segment->GetLength(&length):E_POINTER;}
    HRESULT STDMETHODCALLTYPE InitPlay(runtime::SegmentState*,runtime::Performance*,void** out,DWORD,DWORD)override{if(!out)return E_POINTER;*out=nullptr;try{*out=new PlayToken;return S_OK;}catch(...){return E_OUTOFMEMORY;}}
    HRESULT STDMETHODCALLTYPE EndPlay(void* data)override{delete static_cast<PlayToken*>(data);return S_OK;}
    HRESULT STDMETHODCALLTYPE Play(void* data,LONG start,LONG end,LONG offset,DWORD,runtime::Performance* performance,runtime::SegmentState*,DWORD virtualId)override{
        if(!data||!performance)return E_POINTER;if(start>end)return E_INVALIDARG;if(control->cancelled)return S_OK;
        try{auto& token=*static_cast<PlayToken*>(data);std::lock_guard<std::mutex> guard(token.lock);if(offset!=token.offset){token.offset=offset;token.covered=std::max<LONG>(0,start);}
            const LONG begin=std::max<LONG>({origin,start,token.covered}),limit=std::min(end,length);if(begin>=limit)return S_OK;
            for(const auto& beat:timeline.beat_ticks(begin-origin,limit-origin)){
                const auto time=static_cast<LONGLONG>(beat.clocks)+origin+offset;if(time<LONG_MIN||time>LONG_MAX)return E_INVALIDARG;
                runtime::Message* message=nullptr;auto hr=performance->AllocPMsg(sizeof(runtime::Message),&message);if(FAILED(hr))return hr;
                std::memset(message,0,sizeof(*message));message->size=sizeof(*message);message->type=255;message->flags=2;message->musicTime=static_cast<LONG>(time);message->pchannel=0xffffffff;message->group=0xffffffff;message->virtualTrack=virtualId;
                try{message->user=new Click(beat.beat==0);}catch(...){performance->FreePMsg(message);return E_OUTOFMEMORY;}
                hr=graph->StampPMsg(message);if(SUCCEEDED(hr))hr=performance->SendPMsg(message);if(control->trace)control->trace->record(control->countIn?"countin-click-scheduled":"click-scheduled",static_cast<LONG>(time),0,0,beat.beat==0?1:0,hr);if(FAILED(hr)){performance->FreePMsg(message);control->failed=true;return hr;}token.covered=beat.clocks+origin+1;
            }token.covered=limit;return S_OK;
        }catch(...){control->failed=true;return E_FAIL;}
    }
    HRESULT STDMETHODCALLTYPE GetParam(REFGUID,LONG,LONG*,void*)override{return DMUS_E_TYPE_UNSUPPORTED;}
    HRESULT STDMETHODCALLTYPE SetParam(REFGUID,LONG,void*)override{return DMUS_E_TYPE_UNSUPPORTED;}
    HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
    HRESULT STDMETHODCALLTYPE AddNotificationType(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
    HRESULT STDMETHODCALLTYPE RemoveNotificationType(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
    HRESULT STDMETHODCALLTYPE Clone(LONG,LONG,runtime::Track** out)override{if(!out)return E_POINTER;*out=nullptr;return E_NOTIMPL;}
};
void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Private Metronome track initialization failed");}
}
struct NativeMetronomeRuntime::Impl {std::shared_ptr<Control> control=std::make_shared<Control>();runtime::Graph* graph=nullptr;~Impl(){control->cancelled=true;if(graph)graph->Release();}};
NativeMetronomeRuntime::NativeMetronomeRuntime(runtime::Segment* segment,runtime::AudioPath* path,const Timeline& timeline,const MetronomeSettings& settings,std::shared_ptr<MetronomeRuntimeTrace> trace,LONG timeOrigin,bool countIn):impl_(std::make_unique<Impl>()){
    if(!segment||!path)throw std::runtime_error("Metronome requires the primary Segment AudioPath");validate_metronome_settings(settings);impl_->control->settings=settings;impl_->control->path=path;path->AddRef();
    if(timeOrigin<0)throw std::runtime_error("Metronome time origin is negative");impl_->control->trace=std::move(trace);impl_->control->countIn=countIn;
    check(CoCreateInstance(runtime::graphClass,nullptr,CLSCTX_INPROC_SERVER,runtime::graphId,reinterpret_cast<void**>(&impl_->graph)));
    auto tool=new ClickTool(impl_->control);auto hr=impl_->graph->InsertTool(tool,nullptr,0,0);tool->Release();check(hr);
    auto track=new ClickTrack(timeline,impl_->graph,impl_->control,timeOrigin);hr=segment->InsertTrack(track,1);track->Release();check(hr);
}
NativeMetronomeRuntime::~NativeMetronomeRuntime()=default;
void NativeMetronomeRuntime::settings(const MetronomeSettings& value){validate_metronome_settings(value);std::lock_guard<std::mutex> guard(impl_->control->lock);impl_->control->settings=value;}
void NativeMetronomeRuntime::cancel(){std::lock_guard<std::mutex> guard(impl_->control->lock);impl_->control->cancelled=true;}
bool NativeMetronomeRuntime::failed()const{return impl_->control->failed;}
}
