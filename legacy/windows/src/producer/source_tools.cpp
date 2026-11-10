#include "source_tools.h"
#include "audio_path.h"
#include <dmerror.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>
namespace producer::app {
const ParamGuid source_velocity_tool_class={0x33,0x89,0x7b,0x36,0x0b,0xf1,0xce,0x4d,0xa5,0xc5,0x7d,0x09,0x5c,0xc8,0xec,0x35};
namespace {
bool gain_valid(float v){return std::isfinite(v)&&v>=0&&v<=1;}
GUID guid(const ParamGuid& b){GUID g;std::memcpy(&g,b.data(),16);return g;}
class VelocityTool final:public runtime::Tool,public IMediaParams,public IMediaParamInfo,public IPersistStream {
    LONG refs_=1;std::mutex lock_;float neutral_,value_,base_;GUID format_=guid(param_time_music);MP_TIMEDATA timeData_=768;
    std::vector<MP_ENVELOPE_SEGMENT> envelopes_;
    float at(REFERENCE_TIME time)const {
        float v=base_;
        for(const auto& e:envelopes_){if(time<e.rtStart)break;v=e.valEnd;if(time<e.rtEnd){v=e.valStart;if(e.iCurve==MP_CURVE_LINEAR)v+=static_cast<float>((static_cast<double>(time)-e.rtStart)/(static_cast<double>(e.rtEnd)-e.rtStart))*(e.valEnd-e.valStart);}}
        return v;
    }
public:
    explicit VelocityTool(float gain):neutral_(gain),value_(gain),base_(gain){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override {if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==runtime::toolId)*out=static_cast<runtime::Tool*>(this);else if(id==__uuidof(IMediaParams))*out=static_cast<IMediaParams*>(this);else if(id==__uuidof(IMediaParamInfo))*out=static_cast<IMediaParamInfo*>(this);else if(id==IID_IPersist||id==IID_IPersistStream)*out=static_cast<IPersistStream*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release()override{const auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(runtime::Graph*)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* out)override{if(!out)return E_POINTER;*out=8;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* out)override{if(!out)return E_POINTER;*out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** out,DWORD count)override{if(!out||!*out)return E_POINTER;if(count!=1)return E_INVALIDARG;**out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessPMsg(runtime::Performance*,runtime::Message* p)override {
        if(!p)return E_POINTER;
        if(p->type==1){if(p->size<sizeof(runtime::NoteMessage))return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);value_=at(p->musicTime);auto& n=*reinterpret_cast<runtime::NoteMessage*>(p);n.velocity=static_cast<BYTE>(std::clamp(std::lround(n.velocity*value_),0l,127l));}
        return p->graph&&SUCCEEDED(p->graph->StampPMsg(p))?DMUS_S_REQUEUE:DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(runtime::Performance*,runtime::Message*,LONGLONG)override{std::lock_guard<std::mutex> guard(lock_);envelopes_.clear();base_=value_=neutral_;return DMUS_S_FREE;}
    HRESULT STDMETHODCALLTYPE GetParam(DWORD index,MP_DATA* out)override{if(!out)return E_POINTER;if(index)return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);*out=value_;return S_OK;}
    HRESULT STDMETHODCALLTYPE SetParam(DWORD index,MP_DATA v)override{if((index&&index!=DWORD_ALLPARAMS)||!gain_valid(v))return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);base_=value_=v;return S_OK;}
    HRESULT STDMETHODCALLTYPE AddEnvelope(DWORD index,DWORD count,MP_ENVELOPE_SEGMENT* curves)override {
        if(index||(count&&!curves))return E_INVALIDARG;
        for(DWORD i=0;i<count;++i){const auto& e=curves[i];if(e.rtEnd<e.rtStart||!gain_valid(e.valStart)||!gain_valid(e.valEnd)||(e.iCurve!=MP_CURVE_JUMP&&e.iCurve!=MP_CURVE_LINEAR)||e.flags>2||(i&&curves[i-1].rtStart>e.rtStart))return E_INVALIDARG;}
        try {std::lock_guard<std::mutex> guard(lock_);auto next=envelopes_;for(DWORD i=0;i<count;++i){auto e=curves[i];if(e.flags==MPF_ENVLP_BEGIN_CURRENTVAL)e.valStart=value_;else if(e.flags==MPF_ENVLP_BEGIN_NEUTRALVAL)e.valStart=neutral_;next.push_back(e);}std::stable_sort(next.begin(),next.end(),[](const auto& a,const auto& b){return a.rtStart<b.rtStart;});envelopes_=std::move(next);return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}
    }
    HRESULT STDMETHODCALLTYPE FlushEnvelope(DWORD index,REFERENCE_TIME start,REFERENCE_TIME end)override {
        if((index&&index!=DWORD_ALLPARAMS)||end<start)return E_INVALIDARG;
        std::lock_guard<std::mutex> guard(lock_);envelopes_.erase(std::remove_if(envelopes_.begin(),envelopes_.end(),[&](const auto& e){return e.rtStart<=end&&e.rtEnd>=start;}),envelopes_.end());base_=value_=neutral_;return S_OK;
    }
    // DirectMusic supplies DMUS_PPQ (768) as the music-time data. PMsg times
    // and Parameter Track envelopes use that same clock scale.
    HRESULT STDMETHODCALLTYPE SetTimeFormat(GUID format,MP_TIMEDATA data)override{if(format!=guid(param_time_music)||data!=768)return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);format_=format;timeData_=data;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetParamCount(DWORD* n)override{if(!n)return E_POINTER;*n=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetParamInfo(DWORD index,MP_PARAMINFO* info)override{if(!info)return E_POINTER;if(index)return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);*info={};info->mpType=MPT_FLOAT;info->mopCaps=MP_CAPS_CURVE_JUMP|MP_CAPS_CURVE_LINEAR;info->mpdMinValue=0;info->mpdMaxValue=1;info->mpdNeutralValue=neutral_;wcscpy_s(info->szLabel,L"Velocity gain");wcscpy_s(info->szUnitText,L"ratio");return S_OK;}
    HRESULT STDMETHODCALLTYPE GetParamText(DWORD index,WCHAR** out)override{if(!out)return E_POINTER;*out=nullptr;return index?E_INVALIDARG:E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetNumTimeFormats(DWORD* n)override{if(!n)return E_POINTER;*n=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetSupportedTimeFormat(DWORD index,GUID* out)override{if(!out)return E_POINTER;if(index)return E_INVALIDARG;*out=guid(param_time_music);return S_OK;}
    HRESULT STDMETHODCALLTYPE GetCurrentTimeFormat(GUID* out,MP_TIMEDATA* data)override{if(!out||!data)return E_POINTER;std::lock_guard<std::mutex> guard(lock_);*out=format_;*data=timeData_;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID* out)override{if(!out)return E_POINTER;*out=guid(source_velocity_tool_class);return S_OK;}
    HRESULT STDMETHODCALLTYPE IsDirty()override{return S_FALSE;}
    HRESULT STDMETHODCALLTYPE Load(IStream* stream)override {if(!stream)return E_POINTER;BYTE bytes[8];ULONG n=0;const auto hr=stream->Read(bytes,8,&n);if(FAILED(hr))return hr;if(n!=8)return STG_E_READFAULT;DWORD version;float gain;std::memcpy(&version,bytes,4);std::memcpy(&gain,bytes+4,4);if(version!=1||!gain_valid(gain))return E_INVALIDARG;std::lock_guard<std::mutex> guard(lock_);neutral_=base_=value_=gain;envelopes_.clear();return S_OK;}
    HRESULT STDMETHODCALLTYPE Save(IStream* stream,BOOL)override {if(!stream)return E_POINTER;std::lock_guard<std::mutex> guard(lock_);const auto p=source_velocity_payload(neutral_);ULONG n=0;const auto hr=stream->Write(p.data.data(),8,&n);return FAILED(hr)?hr:n==8?S_OK:STG_E_WRITEFAULT;}
    HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER* n)override{if(!n)return E_POINTER;n->QuadPart=8;return S_OK;}
};
bool same_address(const ParamObject& a,const ParamObject& b){return a.timeFormat==b.timeFormat&&a.pchannel==b.pchannel&&a.stage==b.stage&&a.buffer==b.buffer&&a.objectClass==b.objectClass&&a.index==b.index;}
}
float source_velocity_gain(const GraphTool& tool){if(tool.classId!=source_velocity_tool_class)throw std::runtime_error("Not a source Velocity Tool");if(!tool.payload)return 1;const auto& p=*tool.payload;if(p.id!="data"||p.container()||p.data.size()!=8||read32(p.data,0)!=1)throw std::runtime_error("Invalid source Velocity Tool properties");float gain;std::memcpy(&gain,p.data.data()+4,4);if(!gain_valid(gain))throw std::runtime_error("Velocity gain must be finite and within 0..1");return gain;}
Chunk source_velocity_payload(float gain){if(!gain_valid(gain))throw std::runtime_error("Velocity gain must be finite and within 0..1");Chunk p;p.id="data";p.data.resize(8);put32(p.data,0,1);std::memcpy(p.data.data()+4,&gain,4);return p;}
std::vector<ToolFactory> source_tool_factories(){return {{source_velocity_tool_class,[](const GraphTool& tool){return new VelocityTool(source_velocity_gain(tool));},[](const GraphTool& tool){(void)source_velocity_gain(tool);}}};}
std::vector<ToolParameterObject> discover_audio_path_tool_parameters(const Bytes& bytes){
    if(bytes.empty())return {};AudioPathDocument path;path.load(bytes);const auto graphBytes=path.tool_graph();if(graphBytes.empty())return {};ToolGraphDocument graph;graph.load(graphBytes);const auto tools=graph.tools();std::vector<ToolParameterObject> out;const auto factories=source_tool_factories();
    for(size_t i=0;i<tools.size();++i){const auto& t=tools[i];const auto f=std::find_if(factories.begin(),factories.end(),[&](const auto& factory){return factory.classId==t.classId;});if(f==factories.end())continue;const auto channel=t.channels.empty()?0:t.channels.front();
        ToolParameterObject object;object.address.timeFormat=param_time_music;object.address.objectClass=t.classId;object.address.pchannel=channel;
        for(size_t before=0;before<i;++before){const auto& prev=tools[before];if(prev.classId==t.classId&&(prev.channels.empty()||std::find(prev.channels.begin(),prev.channels.end(),channel)!=prev.channels.end()))++object.address.index;}
        object.name=L"Velocity Tool "+std::to_wstring(i+1)+L" / PChannel "+std::to_wstring(static_cast<std::uint64_t>(channel)+1);
        std::unique_ptr<runtime::Tool,void(*)(runtime::Tool*)> tool(f->create(t),[](runtime::Tool* p){if(p)p->Release();});IMediaParamInfo* raw=nullptr;
        if(!tool||FAILED(tool->QueryInterface(__uuidof(IMediaParamInfo),reinterpret_cast<void**>(&raw))))continue;
        std::unique_ptr<IMediaParamInfo,void(*)(IMediaParamInfo*)> info(raw,[](IMediaParamInfo* p){if(p)p->Release();});DWORD count=0;if(FAILED(info->GetParamCount(&count))||count>1024)throw std::runtime_error("Source Tool parameter discovery failed");
        for(DWORD index=0;index<count;++index){MP_PARAMINFO parameter{};if(FAILED(info->GetParamInfo(index,&parameter)))throw std::runtime_error("Source Tool capability discovery failed");object.parameters.push_back({index,parameter});}out.push_back(std::move(object));
    }return out;
}
const ToolParameterCapability* tool_parameter_capability(const std::vector<ToolParameterObject>& objects,const ParamObject& address,std::uint32_t index){for(const auto& object:objects)if(same_address(object.address,address))for(const auto& p:object.parameters)if(p.index==index)return &p;return nullptr;}
bool valid_tool_parameter_curve(const ToolParameterCapability& capability,const ParamCurve& c){const auto& p=capability.info;return valid_param_curve(c)&&(p.mopCaps&c.type)!=0&&c.flags<=2&&c.startValue>=p.mpdMinValue&&c.startValue<=p.mpdMaxValue&&c.endValue>=p.mpdMinValue&&c.endValue<=p.mpdMaxValue;}
void validate_source_parameter_controls(const std::vector<ParamControlObject>& controls,const std::vector<ToolParameterObject>& capabilities){for(const auto& o:controls)if(o.address.objectClass==source_velocity_tool_class){for(const auto& p:o.parameters){const auto c=tool_parameter_capability(capabilities,o.address,p.index);if(!c)throw std::runtime_error("Source Tool parameter address is not available in the embedded AudioPath");for(const auto& curve:p.curves)if(!valid_tool_parameter_curve(*c,curve))throw std::runtime_error("Source Tool parameter curve exceeds its supported values, shape or flags");}}}
}
