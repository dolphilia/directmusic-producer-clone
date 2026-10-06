#include "native_script_track.h"
#include "producer/script_track.h"
#include <objidl.h>
#include <dmerror.h>
#include <atomic>
#include <mutex>
#include <set>
#include <limits>
#include <cstring>
#include <algorithm>
#include <stdexcept>
namespace producer::app {
namespace {
const GUID trackClass={0x4108fa85,0x3586,0x11d3,{0x8b,0xd7,0,0x60,8,0x93,0xb1,0xb6}};
const GUID trackInterface={0xf96029a1,0x4282,0x11d2,{0x87,0x17,0,0x60,8,0x93,0xb1,0xbd}};
const GUID invocationInterface={0x8b7851cd,0x734b,0x4c3c,{0xa2,0x99,0xc5,0x5e,0x12,0x1a,0x35,0xf1}};
const UINT invokeMessage=WM_APP+713;
struct Token {std::atomic<bool> cancelled{false};runtime::Performance* performance=nullptr;LONG start=0,length=0;std::mutex lock;LONG offset=LONG_MIN;std::set<size_t> submitted;};
struct DispatchState {
 std::mutex lock;HWND window=nullptr;bool open=true;
 std::vector<TriggeredScriptSnapshot> sources;std::vector<runtime::Script*> scripts;
 std::vector<NativeScriptCall> calls;bool overflow=false;
 runtime::Graph* graphs[3]{};
};
class Invocation final:public IUnknown {
 std::atomic<ULONG> refs{1};
public:
 std::shared_ptr<DispatchState> state;std::shared_ptr<Token> token;ScriptEvent event;LONG dispatchClock=0;std::atomic<bool> cancelled{false};
 Invocation(std::shared_ptr<DispatchState> s,std::shared_ptr<Token> t,const ScriptEvent& e):state(std::move(s)),token(std::move(t)),event(e){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,invocationInterface))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{auto n=--refs;if(!n)delete this;return n;}
};
LRESULT CALLBACK dispatch_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
 if(message!=invokeMessage)return DefWindowProcW(window,message,wp,lp);
 auto call=reinterpret_cast<Invocation*>(lp);const auto state=call->state;runtime::Script* script=nullptr;
 {std::lock_guard<std::mutex> guard(state->lock);if(state->open&&!call->cancelled&&!call->token->cancelled){for(size_t i=0;i<state->sources.size();++i)if(state->sources[i].objectId==call->event.objectId){script=state->scripts[i];break;}}}
 if(script){NativeScriptCall result;result.routine=call->event.routine;result.objectId=call->event.objectId;result.start=call->token->start;result.logical=call->event.logical;result.physical=call->event.physical;result.timing=call->event.timing;result.dispatchClock=call->dispatchClock;result.result.error.size=sizeof(result.result.error);call->token->performance->GetTime(nullptr,&result.invokeClock);auto name=call->event.routine;result.result.result=script->CallRoutine(name.data(),&result.result.error);std::lock_guard<std::mutex> guard(state->lock);if(state->calls.size()<4096)state->calls.push_back(result);else state->overflow=true;}
 call->Release();return 0;
}
class RoutineTool final:public runtime::Tool {
 std::atomic<ULONG> refs{1};std::shared_ptr<DispatchState> state;DWORD delivery;
public:
 RoutineTool(std::shared_ptr<DispatchState> s,DWORD d):state(std::move(s)),delivery(d){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,runtime::toolId))return E_NOINTERFACE;*out=static_cast<runtime::Tool*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{auto n=--refs;if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE Init(runtime::Graph*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* out)override{if(!out)return E_POINTER;*out=delivery;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* out)override{if(!out)return E_POINTER;*out=1;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** out,DWORD n)override{if(!out||!*out)return E_POINTER;if(n!=1)return E_INVALIDARG;(*out)[0]=255;return S_OK;}
 HRESULT STDMETHODCALLTYPE ProcessPMsg(runtime::Performance* performance,runtime::Message* message)override{
  if(!message||message->type!=255||!message->user)return E_INVALIDARG;Invocation* call=nullptr;const auto hr=message->user->QueryInterface(invocationInterface,reinterpret_cast<void**>(&call));if(FAILED(hr))return hr;
  if(message->flags&0x20)call->cancelled=true;
  if(!call->cancelled&&!call->token->cancelled){performance->GetTime(nullptr,&call->dispatchClock);std::lock_guard<std::mutex> guard(state->lock);if(state->open&&state->window){call->AddRef();if(!PostMessageW(state->window,invokeMessage,0,reinterpret_cast<LPARAM>(call))){NativeScriptCall failed;failed.routine=call->event.routine;failed.objectId=call->event.objectId;failed.timing=call->event.timing;failed.physical=call->event.physical;failed.logical=call->event.logical;failed.start=call->token->start;failed.dispatchClock=call->dispatchClock;failed.result.result=HRESULT_FROM_WIN32(GetLastError());if(state->calls.size()<4096)state->calls.push_back(failed);else state->overflow=true;call->Release();call->cancelled=true;}}}
  call->Release();return DMUS_S_FREE;
 }
 HRESULT STDMETHODCALLTYPE Flush(runtime::Performance*,runtime::Message* message,LONGLONG)override{if(message&&message->user){Invocation* call=nullptr;if(SUCCEEDED(message->user->QueryInterface(invocationInterface,reinterpret_cast<void**>(&call)))){call->cancelled=true;call->Release();}}return DMUS_S_FREE;}
};
class SourceTrack final:public runtime::Track,public IPersistStream {
 std::atomic<ULONG> refs{1};Chunk track;std::shared_ptr<DispatchState> state;LONG length=0;runtime::Graph* graphs[3]{};
public:
 SourceTrack(const Chunk& t,std::shared_ptr<DispatchState> s):track(t),state(std::move(s)){for(size_t i=0;i<3;++i){graphs[i]=state->graphs[i];graphs[i]->AddRef();}}
 ~SourceTrack(){for(auto graph:graphs)graph->Release();}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(IsEqualGUID(id,IID_IUnknown)||IsEqualGUID(id,trackInterface))*out=static_cast<runtime::Track*>(this);else if(IsEqualGUID(id,IID_IPersist)||IsEqualGUID(id,IID_IPersistStream))*out=static_cast<IPersistStream*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{auto n=--refs;if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE Init(runtime::Segment* segment)override{if(!segment)return E_POINTER;return segment->GetLength(&length);}
 HRESULT STDMETHODCALLTYPE InitPlay(runtime::SegmentState* playing,runtime::Performance* performance,void** out,DWORD,DWORD)override{if(!out||!performance||!playing)return E_POINTER;*out=nullptr;try{auto token=std::make_shared<Token>();token->performance=performance;token->length=length;auto hr=playing->GetStartTime(&token->start);if(FAILED(hr))return hr;*out=new std::shared_ptr<Token>(token);return S_OK;}catch(...){return E_OUTOFMEMORY;}}
 HRESULT STDMETHODCALLTYPE EndPlay(void* data)override{if(!data)return E_POINTER;auto holder=static_cast<std::shared_ptr<Token>*>(data);LONG now=0;const auto hr=(*holder)->performance->GetTime(nullptr,&now);if(FAILED(hr)||static_cast<LONGLONG>(now)<static_cast<LONGLONG>((*holder)->start)+(*holder)->length)(*holder)->cancelled=true;delete holder;return S_OK;}
 HRESULT STDMETHODCALLTYPE Play(void* data,LONG start,LONG end,LONG offset,DWORD,runtime::Performance* performance,runtime::SegmentState*,DWORD virtualId)override{
  if(!data||!performance)return E_POINTER;if(start>end)return E_INVALIDARG;try{auto token=*static_cast<std::shared_ptr<Token>*>(data);if(token->cancelled)return S_OK;const auto events=script_events(track);std::lock_guard<std::mutex> guard(token->lock);if(offset!=token->offset){token->offset=offset;token->submitted.clear();}
   for(size_t i=0;i<events.size();++i){const auto& event=events[i];if(!event.hasId||event.routine.empty()||event.physical<start||event.physical>=end||token->submitted.count(i))continue;
    const auto time=static_cast<LONGLONG>(event.physical)+offset;if(time<LONG_MIN||time>LONG_MAX)return E_INVALIDARG;const auto graphIndex=event.timing==1?0:event.timing==2?1:event.timing==4?2:3;if(graphIndex==3)return E_INVALIDARG;
    std::unique_ptr<Invocation,void(*)(Invocation*)> invocation(new Invocation(state,token,event),[](Invocation* value){value->Release();});token->submitted.insert(i);runtime::Message* message=nullptr;auto hr=performance->AllocPMsg(sizeof(runtime::Message),&message);if(FAILED(hr)){token->submitted.erase(i);return hr;}std::memset(message,0,sizeof(*message));message->size=sizeof(*message);message->type=255;message->flags=2;message->musicTime=static_cast<LONG>(time);message->pchannel=0xffffffff;message->group=read32(track.find("trkh")->data,20);message->virtualTrack=virtualId;message->user=invocation.release();hr=graphs[graphIndex]->StampPMsg(message);if(SUCCEEDED(hr))hr=performance->SendPMsg(message);if(FAILED(hr)){performance->FreePMsg(message);token->submitted.erase(i);return hr;}
   }return S_OK;
  }catch(...){return E_OUTOFMEMORY;}
 }
 HRESULT STDMETHODCALLTYPE GetParam(REFGUID,LONG,LONG*,void*)override{return DMUS_E_TYPE_UNSUPPORTED;}
 HRESULT STDMETHODCALLTYPE SetParam(REFGUID,LONG,void*)override{return DMUS_E_TYPE_UNSUPPORTED;}
 HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
 HRESULT STDMETHODCALLTYPE AddNotificationType(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
 HRESULT STDMETHODCALLTYPE RemoveNotificationType(REFGUID)override{return DMUS_E_TYPE_UNSUPPORTED;}
 HRESULT STDMETHODCALLTYPE Clone(LONG start,LONG end,runtime::Track** out)override{if(!out)return E_POINTER;*out=nullptr;if(start<0||end<=start)return E_INVALIDARG;try{auto copy=track;auto& list=*copy.find("LIST","scrt")->find("LIST","scrl");for(auto it=list.children.begin();it!=list.children.end();){if(it->id!="LIST"||it->type!="scre"){++it;continue;}auto h=it->find("scrh");auto physical=static_cast<LONG>(read32(h->data,8));if(physical<start||physical>=end){it=list.children.erase(it);continue;}put32(h->data,8,physical-start);put32(h->data,4,static_cast<LONG>(read32(h->data,4))-start);++it;}*out=new SourceTrack(copy,state);return S_OK;}catch(...){return E_INVALIDARG;}}
 HRESULT STDMETHODCALLTYPE GetClassID(CLSID* out)override{if(!out)return E_POINTER;*out=trackClass;return S_OK;}
 HRESULT STDMETHODCALLTYPE IsDirty()override{return S_FALSE;}
 HRESULT STDMETHODCALLTYPE Load(IStream*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE Save(IStream* stream,BOOL)override{if(!stream)return E_POINTER;try{const auto bytes=track.find("LIST","scrt")->encode();ULONG written=0;auto hr=stream->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&written);return SUCCEEDED(hr)&&written!=bytes.size()?STG_E_WRITEFAULT:hr;}catch(...){return E_FAIL;}}
 HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER* out)override{if(!out)return E_POINTER;try{out->QuadPart=track.find("LIST","scrt")->encode().size();return S_OK;}catch(...){return E_FAIL;}}
};
}
struct NativeScriptTrackRuntime::Impl {
 std::shared_ptr<DispatchState> state=std::make_shared<DispatchState>();runtime::Graph* graphs[3]{};
 void close(){if(!state)return;HWND window=nullptr;{std::lock_guard<std::mutex> guard(state->lock);state->open=false;window=state->window;state->window=nullptr;state->scripts.clear();}if(window){MSG message{};while(PeekMessageW(&message,window,invokeMessage,invokeMessage,PM_REMOVE))reinterpret_cast<Invocation*>(message.lParam)->Release();DestroyWindow(window);}}
 ~Impl(){close();for(auto graph:graphs)if(graph)graph->Release();}
};
NativeScriptTrackRuntime::NativeScriptTrackRuntime(const std::vector<TriggeredScriptSnapshot>& sources,const std::vector<runtime::Script*>& scripts):impl_(std::make_unique<Impl>()){
 if(sources.size()!=scripts.size())throw std::runtime_error("Script Track runtime source count mismatch");auto state=impl_->state;state->sources=sources;state->scripts=scripts;const auto instance=GetModuleHandleW(nullptr);WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=dispatch_proc;cls.lpszClassName=L"SourceProducerNativeScriptDispatcher";if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Script dispatcher class unavailable");state->window=CreateWindowExW(0,cls.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,instance,nullptr);if(!state->window)throw std::runtime_error("Script dispatcher window unavailable");
 for(size_t i=0;i<3;++i){auto hr=CoCreateInstance(runtime::graphClass,nullptr,CLSCTX_INPROC_SERVER,runtime::graphId,reinterpret_cast<void**>(&impl_->graphs[i]));if(FAILED(hr))throw std::runtime_error("Script message graph unavailable");auto tool=new RoutineTool(state,i==0?4:i==1?8:16);hr=impl_->graphs[i]->InsertTool(tool,nullptr,0,0);tool->Release();if(FAILED(hr))throw std::runtime_error("Script message tool unavailable");state->graphs[i]=impl_->graphs[i];}
}
NativeScriptTrackRuntime::~NativeScriptTrackRuntime()=default;
void NativeScriptTrackRuntime::cancel(){impl_->close();}
std::vector<NativeScriptCall> NativeScriptTrackRuntime::calls()const{std::lock_guard<std::mutex> guard(impl_->state->lock);return impl_->state->calls;}
bool NativeScriptTrackRuntime::overflow()const{std::lock_guard<std::mutex> guard(impl_->state->lock);return impl_->state->overflow;}
Bytes NativeScriptTrackRuntime::loader_bytes(const Bytes& bytes){if(bytes.empty())return {};auto root=Chunk::parse(bytes);if(auto tracks=root.find("LIST","trkl"))tracks->children.erase(std::remove_if(tracks->children.begin(),tracks->children.end(),[](const Chunk& track){if(!track.find("LIST","scrt"))return false;(void)script_events(track);return true;}),tracks->children.end());return root.encode();}
void NativeScriptTrackRuntime::attach(runtime::Segment* segment,const Bytes& bytes){
 const auto root=Chunk::parse(bytes);const auto tracks=root.find("LIST","trkl");if(!tracks)return;size_t index=0;
 for(const auto& track:tracks->children)if(track.find("LIST","scrt")){
  const auto h=track.find("trkh"),flags=track.find("trkx");if(flags&&read32(flags->data,0)&0x40)throw std::runtime_error("Script Track clock-domain ABI remains pending");
  const auto events=script_events(track);for(const auto& e:events)if(e.hasId){if((e.timing!=1&&e.timing!=2&&e.timing!=4)||e.physical<0||e.routine.empty())throw std::runtime_error("Unsupported Script event");size_t count=0;for(const auto& s:impl_->state->sources)if(s.objectId==e.objectId)++count;if(count!=1)throw std::runtime_error("Script Track source not uniquely owned");}
  HRESULT hr=S_OK;auto source=new SourceTrack(track,impl_->state);hr=segment->InsertTrack(source,read32(h->data,20));source->Release();if(FAILED(hr))throw std::runtime_error("Cannot attach source Script Track");hr=segment->SetTrackConfig(trackClass,0xffffffff,static_cast<DWORD>(index),0x38,0x40);if(FAILED(hr))throw std::runtime_error("Cannot configure source Script Track");++index;
 }
}
}
