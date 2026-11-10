#include "source_script_host.h"
#include "script_automation.h"
#include <ActivScp.h>
#include <dmerror.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
#include <vector>
namespace producer::app {
namespace {
struct BridgeNames{std::wstring performance,object,load,alias,global,play,stop,mask,result,constant,playingCheck,playingStop;std::array<std::wstring,4> arguments;};
BridgeNames bridge_names(const ScriptDocument& doc){
    auto objects=doc.container().objects();for(unsigned i=0;;++i){auto prefix=L"ProducerHost"+std::to_wstring(i);BridgeNames names{prefix+L"Performance",prefix+L"Object",prefix+L"Load",prefix+L"Alias",prefix+L"Global",prefix+L"Play",prefix+L"Stop",prefix+L"Mask",prefix+L"Result",prefix+L"Constant",prefix+L"PlayingCheck",prefix+L"PlayingStop",{prefix+L"Argument0",prefix+L"Argument1",prefix+L"Argument2",prefix+L"Argument3"}};
        bool used=false;for(const auto& item:objects)if(item.alias){for(const auto& name:{names.performance,names.object,names.load,names.alias,names.global,names.play,names.stop,names.mask,names.result,names.constant,names.playingCheck,names.playingStop})if(!_wcsicmp(name.c_str(),item.alias->c_str()))used=true;for(const auto& name:names.arguments)if(!_wcsicmp(name.c_str(),item.alias->c_str()))used=true;}if(!used)return names;
    }
}
}
bool uses_source_script_host(const ScriptDocument& doc){return doc.source().has_value()&&(!_wcsicmp(doc.language().c_str(),L"VBScript")||!_wcsicmp(doc.language().c_str(),L"JScript"));}
Bytes source_script_loader_bytes(const ScriptDocument& doc){
    if(!uses_source_script_host(doc))return doc.save_bytes();auto helper=doc;helper.set_properties(doc.name(),L"VBScript",bool(doc.flags()&1),bool(doc.flags()&2));const auto n=bridge_names(doc);
    std::wstring source;for(const auto& name:{n.performance,n.object,n.alias,n.mask,n.result})source+=L"Dim "+name+L"\r\n";for(const auto& name:n.arguments)source+=L"Dim "+name+L"\r\n";
    source+=L"Sub "+n.load+L"()\r\nEval("+n.alias+L").Load\r\nEnd Sub\r\nSub "+n.play+L"()\r\nSelect Case "+n.mask+L"\r\n";
    // Preserve omitted optional slots, including explicit missing arguments.
    // The OS owns flags, embedded AudioPath choice, transitions and the result.
    for(unsigned mask=0;mask<16;++mask){source+=L"Case "+std::to_wstring(mask)+L"\r\nSet "+n.result+L" = Eval("+n.alias+L").Play(";unsigned count=4;while(count&&!(mask&(1u<<(count-1))))--count;for(unsigned i=0;i<count;++i){if(i)source+=L",";if(mask&(1u<<i))source+=n.arguments[i];}source+=L")\r\n";}
    source+=L"End Select\r\nEnd Sub\r\nSub "+n.stop+L"()\r\nIf "+n.mask+L" = 0 Then\r\nEval("+n.alias+L").Stop\r\nElse\r\nEval("+n.alias+L").Stop "+n.arguments[0]+L"\r\nEnd If\r\nEnd Sub\r\n";
    source+=L"Sub "+n.constant+L"()\r\n"+n.result+L" = Eval("+n.alias+L")\r\nEnd Sub\r\n";
    source+=L"Sub "+n.playingCheck+L"()\r\n"+n.result+L" = "+n.object+L".IsPlaying\r\nEnd Sub\r\nSub "+n.playingStop+L"()\r\nIf "+n.mask+L" = 0 Then\r\n"+n.object+L".Stop\r\nElse\r\n"+n.object+L".Stop "+n.arguments[0]+L"\r\nEnd If\r\nEnd Sub\r\n";
    helper.set_source(source);return helper.save_bytes();
}
namespace {
void copy_text(WCHAR* out,size_t capacity,const WCHAR* in){if(!in){out[0]=0;return;}size_t n=0;while(n+1<capacity&&in[n]){out[n]=in[n];++n;}out[n]=0;}
void clear_exception(EXCEPINFO& e){SysFreeString(e.bstrSource);SysFreeString(e.bstrDescription);SysFreeString(e.bstrHelpFile);e={};}
// Public DirectMusic Producer help: Segment.Play/Stop and PlayingSegment.Stop.
// These are scripting names, whose values differ from DMUS_SEGF_* values.
constexpr const wchar_t* scriptConstants[]={L"IsControl",L"IsSecondary",L"AlignToBar",L"AlignToBeat",L"AlignToSegment",L"AtBeat",L"AtFinish",L"AtGrid",L"AtImmediate",L"AtMarker",L"AtMeasure",L"PlayFill",L"PlayIntro",L"PlayBreak",L"PlayEnd",L"PlayEndAndIntro",L"PlayModulate",L"NoCutoff"};
using ConstantValues=std::vector<std::pair<std::wstring,LONG>>;
HRESULT read_script_constants(runtime::Script* helper,const BridgeNames& names,ConstantValues& values,runtime::ScriptErrorInfo* error){
    // Performance automation has no type information (GetTypeInfo E_NOTIMPL).
    // Ask the same owning OS Script for library values, without guessed flags
    // or changing authored source. Only documented constant names are queried.
    HRESULT hr=S_OK;for(const auto name:scriptConstants){
        VARIANT argument,value,number;VariantInit(&argument);VariantInit(&value);VariantInit(&number);argument.vt=VT_BSTR;argument.bstrVal=SysAllocString(name);if(!argument.bstrVal)return E_OUTOFMEMORY;
        hr=helper->SetVariableVariant(const_cast<WCHAR*>(names.alias.c_str()),argument,FALSE,error);VariantClear(&argument);
        if(SUCCEEDED(hr))hr=helper->CallRoutine(const_cast<WCHAR*>(names.constant.c_str()),error);
        if(SUCCEEDED(hr))hr=helper->GetVariableVariant(const_cast<WCHAR*>(names.result.c_str()),&value,error);
        if(SUCCEEDED(hr)&&(value.vt==VT_EMPTY||value.vt==VT_NULL))hr=DISP_E_UNKNOWNNAME;
        if(SUCCEEDED(hr))hr=VariantChangeType(&number,&value,0,VT_I4);if(SUCCEEDED(hr))values.emplace_back(name,number.lVal);VariantClear(&number);VariantClear(&value);if(FAILED(hr))break;
    }
    VARIANT empty;VariantInit(&empty);runtime::ScriptErrorInfo cleanup{};cleanup.size=sizeof(cleanup);helper->SetVariableVariant(const_cast<WCHAR*>(names.alias.c_str()),empty,FALSE,&cleanup);helper->SetVariableVariant(const_cast<WCHAR*>(names.result.c_str()),empty,FALSE,&cleanup);return hr;
}
class GlobalDispatch final:public IDispatch {
    LONG refs_=1;IDispatch* performance_;ConstantValues constants_;
    static constexpr DISPID firstConstant=-1000;
public:
    GlobalDispatch(IDispatch* performance,ConstantValues values):performance_(performance),constants_(std::move(values)){performance_->AddRef();}
    ~GlobalDispatch(){performance_->Release();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id!=IID_IUnknown&&id!=IID_IDispatch)return E_NOINTERFACE;*out=this;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* out)override{return performance_->GetTypeInfoCount(out);}
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT index,LCID locale,ITypeInfo** out)override{return performance_->GetTypeInfo(index,locale,out);}
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID iid,LPOLESTR* names,UINT count,LCID locale,DISPID* out)override{
        if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;if(!names||!out||!count)return E_INVALIDARG;
        if(count==1&&names[0])for(size_t i=0;i<constants_.size();++i)if(!_wcsicmp(names[0],constants_[i].first.c_str())){out[0]=firstConstant-static_cast<DISPID>(i);return S_OK;}
        return performance_->GetIDsOfNames(iid,names,count,locale,out);
    }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID id,REFIID iid,LCID locale,WORD flags,DISPPARAMS* args,VARIANT* out,EXCEPINFO* error,UINT* bad)override{
        const auto index=static_cast<long long>(firstConstant)-id;if(index<0||index>=static_cast<long long>(constants_.size()))return performance_->Invoke(id,iid,locale,flags,args,out,error,bad);
        if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;if(!(flags&DISPATCH_PROPERTYGET))return DISP_E_MEMBERNOTFOUND;if(!args||!out)return E_POINTER;if(args->cNamedArgs)return DISP_E_NONAMEDARGS;if(args->cArgs)return DISP_E_BADPARAMCOUNT;
        VariantInit(out);out->vt=VT_I4;out->lVal=constants_[static_cast<size_t>(index)].second;return S_OK;
    }
};

// Container automation uses its owning OS Script's execution context.
// Keep realization lazy: only authored Load/Play/Stop invokes the bridge.
const GUID aliasBridgeId={0x46868603,0x3c21,0x4d1b,{0x96,0x24,0xad,0x07,0x98,0x72,0x21,0x45}};
struct AliasBridge:IUnknown{virtual IDispatch* original()=0;virtual HRESULT get_object(REFIID,void**,runtime::ScriptErrorInfo*)=0;};
void unwrap_alias(VARIANT&);
void wrap_playing(VARIANT&,runtime::Script*,const BridgeNames&);
class AliasDispatch final:public IDispatch,public AliasBridge {
    LONG refs_=1;IDispatch* object_;runtime::Script* helper_;BridgeNames names_;std::wstring alias_;DISPID load_=DISPID_UNKNOWN,play_=DISPID_UNKNOWN,stop_=DISPID_UNKNOWN;
public:
    AliasDispatch(IDispatch* object,runtime::Script* helper,BridgeNames names,std::wstring alias):object_(object),helper_(helper),names_(std::move(names)),alias_(std::move(alias)){object_->AddRef();helper_->AddRef();for(auto [text,id]:{std::pair{L"Load",&load_},std::pair{L"Play",&play_},std::pair{L"Stop",&stop_}}){LPOLESTR name=const_cast<WCHAR*>(text);object_->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,id);}}
    ~AliasDispatch(){object_->Release();helper_->Release();}
    IDispatch* original()override{object_->AddRef();return object_;}
    HRESULT get_object(REFIID id,void** out,runtime::ScriptErrorInfo* error)override{return helper_->GetVariableObject(alias_.data(),id,out,error);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IDispatch)*out=static_cast<IDispatch*>(this);else if(id==aliasBridgeId)*out=static_cast<AliasBridge*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* out)override{return object_->GetTypeInfoCount(out);}
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT i,LCID l,ITypeInfo** out)override{return object_->GetTypeInfo(i,l,out);}
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID id,LPOLESTR* n,UINT c,LCID l,DISPID* out)override{return object_->GetIDsOfNames(id,n,c,l,out);}
    HRESULT STDMETHODCALLTYPE Invoke(DISPID id,REFIID iid,LCID locale,WORD flags,DISPPARAMS* args,VARIANT* out,EXCEPINFO* exception,UINT* bad)override{
        if(id==DISPID_UNKNOWN||(id!=load_&&id!=play_&&id!=stop_))return object_->Invoke(id,iid,locale,flags,args,out,exception,bad);
        if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;if(!(flags&DISPATCH_METHOD))return DISP_E_MEMBERNOTFOUND;if(!args)return E_POINTER;if(args->cNamedArgs)return DISP_E_NONAMEDARGS;
        const auto maximum=id==play_?4u:id==stop_?1u:0u;if(args->cArgs>maximum)return DISP_E_BADPARAMCOUNT;if(args->cArgs&&!args->rgvarg)return E_POINTER;
        VARIANT value;VariantInit(&value);value.vt=VT_BSTR;value.bstrVal=SysAllocString(alias_.c_str());if(!value.bstrVal)return E_OUTOFMEMORY;runtime::ScriptErrorInfo error{};error.size=sizeof(error);
        auto hr=helper_->SetVariableVariant(names_.alias.data(),value,FALSE,&error);VariantClear(&value);LONG mask=0;
        for(UINT i=0;SUCCEEDED(hr)&&i<args->cArgs;++i){hr=VariantCopyInd(&value,&args->rgvarg[args->cArgs-1-i]);if(SUCCEEDED(hr)&&!(value.vt==VT_ERROR&&value.scode==DISP_E_PARAMNOTFOUND)){unwrap_alias(value);mask|=1<<i;hr=helper_->SetVariableVariant(names_.arguments[i].data(),value,value.vt==VT_DISPATCH||value.vt==VT_UNKNOWN,&error);}VariantClear(&value);}
        if(SUCCEEDED(hr))hr=helper_->SetVariableNumber(names_.mask.data(),mask,&error);
        auto& routine=id==play_?names_.play:id==stop_?names_.stop:names_.load;if(SUCCEEDED(hr))hr=helper_->CallRoutine(routine.data(),&error);
        VARIANT result;VariantInit(&result);if(SUCCEEDED(hr)&&id==play_)hr=helper_->GetVariableVariant(names_.result.data(),&result,&error);
        // Release temporary parameter/result references even when execution fails.
        runtime::ScriptErrorInfo cleanup{};cleanup.size=sizeof(cleanup);for(auto& argument:names_.arguments)helper_->SetVariableVariant(argument.data(),value,FALSE,&cleanup);helper_->SetVariableVariant(names_.result.data(),value,FALSE,&cleanup);
        if(FAILED(hr)){VariantClear(&result);if(exception){*exception={};exception->scode=FAILED(error.result)?error.result:hr;exception->bstrSource=SysAllocString(error.component);exception->bstrDescription=SysAllocString(error.description);return DISP_E_EXCEPTION;}return hr;}
        if(id==play_)wrap_playing(result,helper_,names_);if(out)*out=result;else VariantClear(&result);return hr;
    }
};
// Returned PlayingSegment methods also require their owning OS Script context.
// The original automation/interface is retained, including from-instance use.
class PlayingDispatch final:public IDispatch,public AliasBridge {
    LONG refs_=1;IDispatch* object_;runtime::Script* helper_;BridgeNames names_;DISPID check_=DISPID_UNKNOWN,stop_=DISPID_UNKNOWN;
public:
    PlayingDispatch(IDispatch* object,runtime::Script* helper,BridgeNames names):object_(object),helper_(helper),names_(std::move(names)){object_->AddRef();helper_->AddRef();for(auto [text,id]:{std::pair{L"IsPlaying",&check_},std::pair{L"Stop",&stop_}}){LPOLESTR name=const_cast<WCHAR*>(text);object_->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,id);}}
    ~PlayingDispatch(){object_->Release();helper_->Release();}
    IDispatch* original()override{object_->AddRef();return object_;}
    HRESULT get_object(REFIID id,void** out,runtime::ScriptErrorInfo* error)override{VARIANT value;VariantInit(&value);value.vt=VT_DISPATCH;value.pdispVal=object_;auto hr=helper_->SetVariableVariant(names_.object.data(),value,TRUE,error);if(SUCCEEDED(hr))hr=helper_->GetVariableObject(names_.object.data(),id,out,error);VariantInit(&value);runtime::ScriptErrorInfo cleanup{};cleanup.size=sizeof(cleanup);helper_->SetVariableVariant(names_.object.data(),value,FALSE,&cleanup);return hr;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IDispatch)*out=static_cast<IDispatch*>(this);else if(id==aliasBridgeId)*out=static_cast<AliasBridge*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* out)override{return object_->GetTypeInfoCount(out);}
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT i,LCID locale,ITypeInfo** out)override{return object_->GetTypeInfo(i,locale,out);}
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID iid,LPOLESTR* names,UINT count,LCID locale,DISPID* out)override{return object_->GetIDsOfNames(iid,names,count,locale,out);}
    HRESULT STDMETHODCALLTYPE Invoke(DISPID id,REFIID iid,LCID locale,WORD flags,DISPPARAMS* args,VARIANT* out,EXCEPINFO* exception,UINT* bad)override{
        if(id==DISPID_UNKNOWN||(id!=check_&&id!=stop_))return object_->Invoke(id,iid,locale,flags,args,out,exception,bad);
        if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;if(!(flags&(id==check_?DISPATCH_METHOD|DISPATCH_PROPERTYGET:DISPATCH_METHOD)))return DISP_E_MEMBERNOTFOUND;if(!args)return E_POINTER;if(args->cNamedArgs)return DISP_E_NONAMEDARGS;if(args->cArgs>(id==check_?0u:1u))return DISP_E_BADPARAMCOUNT;if(args->cArgs&&!args->rgvarg)return E_POINTER;
        runtime::ScriptErrorInfo error{};error.size=sizeof(error);VARIANT value;VariantInit(&value);value.vt=VT_DISPATCH;value.pdispVal=object_;auto hr=helper_->SetVariableVariant(names_.object.data(),value,TRUE,&error);VariantInit(&value);LONG mask=0;
        if(SUCCEEDED(hr)&&args->cArgs){hr=VariantCopyInd(&value,args->rgvarg);if(SUCCEEDED(hr)&&!(value.vt==VT_ERROR&&value.scode==DISP_E_PARAMNOTFOUND)){unwrap_alias(value);mask=1;hr=helper_->SetVariableVariant(names_.arguments[0].data(),value,value.vt==VT_DISPATCH||value.vt==VT_UNKNOWN,&error);}VariantClear(&value);}
        if(SUCCEEDED(hr))hr=helper_->SetVariableNumber(names_.mask.data(),mask,&error);auto& routine=id==check_?names_.playingCheck:names_.playingStop;if(SUCCEEDED(hr))hr=helper_->CallRoutine(routine.data(),&error);
        VARIANT result;VariantInit(&result);if(SUCCEEDED(hr)&&id==check_)hr=helper_->GetVariableVariant(names_.result.data(),&result,&error);
        runtime::ScriptErrorInfo cleanup{};cleanup.size=sizeof(cleanup);for(auto name:{names_.object,names_.arguments[0],names_.result})helper_->SetVariableVariant(name.data(),value,FALSE,&cleanup);
        if(FAILED(hr)){VariantClear(&result);if(exception){*exception={};exception->scode=FAILED(error.result)?error.result:hr;exception->bstrSource=SysAllocString(error.component);exception->bstrDescription=SysAllocString(error.description);return DISP_E_EXCEPTION;}return hr;}
        if(out)*out=result;else VariantClear(&result);return hr;
    }
};
void wrap_playing(VARIANT& value,runtime::Script* helper,const BridgeNames& names){if(value.vt==VT_DISPATCH&&value.pdispVal){auto* wrapped=new PlayingDispatch(value.pdispVal,helper,names);value.pdispVal->Release();value.pdispVal=wrapped;}}
void unwrap_alias(VARIANT& value){IUnknown* object=value.vt==VT_DISPATCH?static_cast<IUnknown*>(value.pdispVal):value.vt==VT_UNKNOWN?value.punkVal:nullptr;if(!object)return;AliasBridge* bridge=nullptr;if(SUCCEEDED(object->QueryInterface(aliasBridgeId,reinterpret_cast<void**>(&bridge)))){auto* original=bridge->original();bridge->Release();VariantClear(&value);value.vt=VT_DISPATCH;value.pdispVal=original;}}

struct Site final:IActiveScriptSite {
    LONG refs=1;IDispatch* global;runtime::Script* helper;BridgeNames names;runtime::ScriptErrorInfo last{};
    Site(IDispatch* g,runtime::Script* h,BridgeNames n):global(g),helper(h),names(std::move(n)){reset();}
    void reset(){last={};last.size=sizeof(last);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IActiveScriptSite){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}
    ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetLCID(LCID* out)override{if(!out)return E_POINTER;*out=LOCALE_USER_DEFAULT;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetItemInfo(LPCOLESTR name,DWORD flags,IUnknown** item,ITypeInfo** type)override{
        if(item)*item=nullptr;if(type)*type=nullptr;IDispatch* object=nullptr;
        if(!_wcsicmp(name,names.global.c_str())){object=global;object->AddRef();}
        else{VARIANT v;VariantInit(&v);auto n=std::wstring(name);runtime::ScriptErrorInfo error{};error.size=sizeof(error);auto hr=helper->GetVariableVariant(n.data(),&v,&error);if(SUCCEEDED(hr)&&v.vt==VT_DISPATCH&&v.pdispVal){object=v.pdispVal;object->AddRef();}else if(SUCCEEDED(hr)&&v.vt==VT_UNKNOWN&&v.punkVal)v.punkVal->QueryInterface(IID_IDispatch,reinterpret_cast<void**>(&object));VariantClear(&v);if(!object)return TYPE_E_ELEMENTNOTFOUND;auto* wrapped=new AliasDispatch(object,helper,names,name);object->Release();object=wrapped;}
        HRESULT hr=S_OK;if(flags&SCRIPTINFO_IUNKNOWN){if(!item)hr=E_POINTER;else{*item=object;object->AddRef();}}
        if(SUCCEEDED(hr)&&(flags&SCRIPTINFO_ITYPEINFO)){if(!type)hr=E_POINTER;else hr=object->GetTypeInfo(0,LOCALE_USER_DEFAULT,type);}object->Release();return hr;
    }
    HRESULT STDMETHODCALLTYPE GetDocVersionString(BSTR* out)override{if(!out)return E_POINTER;*out=SysAllocString(L"1");return *out?S_OK:E_OUTOFMEMORY;}
    HRESULT STDMETHODCALLTYPE OnScriptTerminate(const VARIANT*,const EXCEPINFO*)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE OnStateChange(SCRIPTSTATE)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE OnScriptError(IActiveScriptError* error)override{EXCEPINFO e{};DWORD cookie=0;ULONG line=0;LONG character=0;BSTR source=nullptr;error->GetExceptionInfo(&e);if(e.pfnDeferredFillIn)e.pfnDeferredFillIn(&e);error->GetSourcePosition(&cookie,&line,&character);error->GetSourceLineText(&source);last.result=e.scode?e.scode:DISP_E_EXCEPTION;last.line=line+1;last.character=character+1;copy_text(last.component,260,e.bstrSource);copy_text(last.description,260,e.bstrDescription);copy_text(last.sourceLine,260,source);SysFreeString(source);clear_exception(e);return S_OK;}
    HRESULT STDMETHODCALLTYPE OnEnterScript()override{return S_OK;}
    HRESULT STDMETHODCALLTYPE OnLeaveScript()override{return S_OK;}
};
class Host final:public runtime::Script {
    LONG refs_=1;IActiveScript* engine_=nullptr;IActiveScriptParse* parser_=nullptr;IDispatch* dispatch_=nullptr;Site* site_=nullptr;IDispatch* global_=nullptr;runtime::Script* helper_;BridgeNames names_;
    HRESULT report(HRESULT hr,runtime::ScriptErrorInfo* error){
        // JScript can return S_OK from SetScriptState after OnScriptError has
        // reported a global initialization failure. Preserve that failure so
        // an aborted new Script never replaces the previous valid session.
        if(SUCCEEDED(hr)&&site_&&FAILED(site_->last.result))hr=site_->last.result;
        if(error){*error=site_?site_->last:runtime::ScriptErrorInfo{};error->size=sizeof(*error);if(FAILED(hr)&&!FAILED(error->result))error->result=hr;}return FAILED(hr)?DMUS_E_SCRIPT_ERROR_IN_SCRIPT:hr;
    }
    HRESULT member(WCHAR* name,DISPID& id){if(!dispatch_)return DMUS_E_NOT_INIT;if(!name)return E_POINTER;LPOLESTR n=name;return dispatch_->GetIDsOfNames(IID_NULL,&n,1,LOCALE_USER_DEFAULT,&id);}
    HRESULT invoke(WCHAR* name,WORD flags,DISPPARAMS* args,VARIANT* result,runtime::ScriptErrorInfo* error){site_->reset();DISPID id=0;auto hr=member(name,id);if(FAILED(hr))return report(hr,error);EXCEPINFO e{};UINT bad=0;hr=dispatch_->Invoke(id,IID_NULL,LOCALE_USER_DEFAULT,flags,args,result,&e,&bad);if(e.pfnDeferredFillIn)e.pfnDeferredFillIn(&e);if(FAILED(hr)&&!FAILED(site_->last.result)){site_->last.result=e.scode?e.scode:hr;copy_text(site_->last.description,260,e.bstrDescription);copy_text(site_->last.component,260,e.bstrSource);}clear_exception(e);return report(hr,error);}
    HRESULT enumerate(bool routines,DWORD index,WCHAR* out){if(!out)return E_POINTER;if(!dispatch_)return DMUS_E_NOT_INIT;ITypeInfo* info=nullptr;auto hr=dispatch_->GetTypeInfo(0,LOCALE_USER_DEFAULT,&info);if(FAILED(hr))return hr;TYPEATTR* attr=nullptr;hr=info->GetTypeAttr(&attr);if(FAILED(hr)){info->Release();return hr;}std::vector<std::wstring> names;
        auto add=[&](MEMBERID id){BSTR name=nullptr;UINT n=0;if(SUCCEEDED(info->GetNames(id,&name,1,&n))&&n&&name){std::wstring text(name,SysStringLen(name));if(std::find(names.begin(),names.end(),text)==names.end())names.push_back(text);}SysFreeString(name);};
        for(WORD i=0;i<attr->cFuncs;++i){FUNCDESC* f=nullptr;if(SUCCEEDED(info->GetFuncDesc(i,&f))){if((f->invkind==INVOKE_FUNC)==routines)add(f->memid);info->ReleaseFuncDesc(f);}}
        if(!routines)for(WORD i=0;i<attr->cVars;++i){VARDESC* v=nullptr;if(SUCCEEDED(info->GetVarDesc(i,&v))){add(v->memid);info->ReleaseVarDesc(v);}}
        info->ReleaseTypeAttr(attr);info->Release();if(index>=names.size())return S_FALSE;if(names[index].size()>=260)return E_INVALIDARG;copy_text(out,260,names[index].c_str());return S_OK;
    }
public:
    explicit Host(runtime::Script* helper):helper_(helper){helper_->AddRef();}
    ~Host(){if(engine_)engine_->Close();if(dispatch_)dispatch_->Release();if(parser_)parser_->Release();if(engine_)engine_->Release();if(site_)site_->Release();if(global_)global_->Release();helper_->Release();}
    HRESULT initialize(const ScriptDocument& doc,runtime::Performance* performance,runtime::ScriptErrorInfo* error){
        names_=bridge_names(doc);auto& bridge=names_.performance;runtime::ScriptErrorInfo local{};local.size=sizeof(local);auto hr=helper_->SetVariableObject(bridge.data(),performance,&local);if(FAILED(hr)){if(error)*error=local;return hr;}VARIANT object;VariantInit(&object);hr=helper_->GetVariableVariant(bridge.data(),&object,&local);if(FAILED(hr)||object.vt!=VT_DISPATCH||!object.pdispVal){VariantClear(&object);if(error)*error=local;return FAILED(hr)?hr:E_NOINTERFACE;}
        LPOLESTR trace=const_cast<WCHAR*>(L"Trace");DISPID traceId=0;hr=object.pdispVal->GetIDsOfNames(IID_NULL,&trace,1,LOCALE_USER_DEFAULT,&traceId);if(SUCCEEDED(hr))global_=new ScriptPerformanceAutomation(performance,object.pdispVal,traceId);VariantClear(&object);if(FAILED(hr))return hr;
        ConstantValues constants;hr=read_script_constants(helper_,names_,constants,&local);if(FAILED(hr)){if(error){*error=local;if(!FAILED(error->result))error->result=hr;}return hr;}auto* globals=new GlobalDispatch(global_,std::move(constants));global_->Release();global_=globals;
        site_=new Site(global_,helper_,names_);CLSID language{};hr=CLSIDFromProgID(doc.language().c_str(),&language);if(SUCCEEDED(hr))hr=CoCreateInstance(language,nullptr,CLSCTX_INPROC_SERVER,IID_IActiveScript,reinterpret_cast<void**>(&engine_));if(SUCCEEDED(hr))hr=engine_->QueryInterface(IID_IActiveScriptParse,reinterpret_cast<void**>(&parser_));if(SUCCEEDED(hr))hr=engine_->SetScriptSite(site_);if(SUCCEEDED(hr))hr=parser_->InitNew();
        if(SUCCEEDED(hr))hr=engine_->AddNamedItem(names_.global.c_str(),SCRIPTITEM_GLOBALMEMBERS);
        for(const auto& item:doc.container().objects())if(SUCCEEDED(hr)&&item.alias&&!item.alias->empty())hr=engine_->AddNamedItem(item.alias->c_str(),SCRIPTITEM_ISVISIBLE);
        EXCEPINFO e{};const auto source=doc.source();if(SUCCEEDED(hr)&&!source)hr=E_NOTIMPL;if(SUCCEEDED(hr))hr=parser_->ParseScriptText(source->c_str(),nullptr,nullptr,nullptr,0,0,SCRIPTTEXT_ISVISIBLE,nullptr,&e);if(SUCCEEDED(hr))hr=engine_->SetScriptState(SCRIPTSTATE_CONNECTED);clear_exception(e);if(SUCCEEDED(hr))hr=engine_->GetScriptDispatch(nullptr,&dispatch_);return report(hr,error);
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==runtime::scriptId){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs_);}
    ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(runtime::Performance*,runtime::ScriptErrorInfo*)override{return DMUS_E_ALREADY_INITED;}
    HRESULT STDMETHODCALLTYPE CallRoutine(WCHAR* name,runtime::ScriptErrorInfo* error)override{DISPPARAMS args{};return invoke(name,DISPATCH_METHOD,&args,nullptr,error);}
    HRESULT STDMETHODCALLTYPE SetVariableVariant(WCHAR* name,VARIANT value,BOOL reference,runtime::ScriptErrorInfo* error)override{DISPID put=DISPID_PROPERTYPUT;DISPPARAMS args{&value,&put,1,1};return invoke(name,reference?DISPATCH_PROPERTYPUTREF:DISPATCH_PROPERTYPUT,&args,nullptr,error);}
    HRESULT STDMETHODCALLTYPE GetVariableVariant(WCHAR* name,VARIANT* value,runtime::ScriptErrorInfo* error)override{if(!value)return E_POINTER;DISPPARAMS args{};return invoke(name,DISPATCH_PROPERTYGET,&args,value,error);}
    HRESULT STDMETHODCALLTYPE SetVariableNumber(WCHAR* name,LONG number,runtime::ScriptErrorInfo* error)override{VARIANT value;VariantInit(&value);value.vt=VT_I4;value.lVal=number;return SetVariableVariant(name,value,FALSE,error);}
    HRESULT STDMETHODCALLTYPE GetVariableNumber(WCHAR* name,LONG* number,runtime::ScriptErrorInfo* error)override{if(!number)return E_POINTER;VARIANT value,converted;VariantInit(&value);VariantInit(&converted);auto hr=GetVariableVariant(name,&value,error);if(SUCCEEDED(hr))hr=VariantChangeType(&converted,&value,0,VT_I4);if(SUCCEEDED(hr))*number=converted.lVal;VariantClear(&value);VariantClear(&converted);return hr;}
    HRESULT STDMETHODCALLTYPE SetVariableObject(WCHAR* name,IUnknown* object,runtime::ScriptErrorInfo* error)override{auto& bridge=names_.object;auto hr=helper_->SetVariableObject(bridge.data(),object,error);VARIANT value;VariantInit(&value);if(SUCCEEDED(hr))hr=helper_->GetVariableVariant(bridge.data(),&value,error);if(SUCCEEDED(hr))hr=SetVariableVariant(name,value,TRUE,error);VariantClear(&value);return hr;}
    HRESULT STDMETHODCALLTYPE GetVariableObject(WCHAR* name,REFIID id,void** out,runtime::ScriptErrorInfo* error)override{if(!out)return E_POINTER;VARIANT value;VariantInit(&value);auto hr=GetVariableVariant(name,&value,error);if(SUCCEEDED(hr)){IUnknown* candidate=value.vt==VT_DISPATCH?static_cast<IUnknown*>(value.pdispVal):value.vt==VT_UNKNOWN?value.punkVal:nullptr;AliasBridge* alias=nullptr;if(candidate&&SUCCEEDED(candidate->QueryInterface(aliasBridgeId,reinterpret_cast<void**>(&alias)))){hr=alias->get_object(id,out,error);alias->Release();VariantClear(&value);return hr;}unwrap_alias(value);auto& bridge=names_.object;hr=helper_->SetVariableVariant(bridge.data(),value,TRUE,error);if(SUCCEEDED(hr))hr=helper_->GetVariableObject(bridge.data(),id,out,error);}VariantClear(&value);return hr;}
    HRESULT STDMETHODCALLTYPE EnumRoutine(DWORD i,WCHAR* out)override{return enumerate(true,i,out);}
    HRESULT STDMETHODCALLTYPE EnumVariable(DWORD i,WCHAR* out)override{return enumerate(false,i,out);}
};
}
HRESULT create_source_script_host(const ScriptDocument& doc,runtime::Script* helper,runtime::Performance* performance,runtime::Script** out,runtime::ScriptErrorInfo* error){if(!helper||!performance||!out||!error)return E_POINTER;*out=nullptr;auto* host=new Host(helper);auto hr=host->initialize(doc,performance,error);if(SUCCEEDED(hr))*out=host;else host->Release();return hr;}
}
