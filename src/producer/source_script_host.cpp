#include "source_script_host.h"
#include "script_automation.h"
#include <ActivScp.h>
#include <dmerror.h>
#include <algorithm>
#include <cstring>
#include <vector>
namespace producer::app {
namespace {
struct BridgeNames{std::wstring performance,object,load,alias,global;};
BridgeNames bridge_names(const ScriptDocument& doc){
    auto objects=doc.container().objects();for(unsigned i=0;;++i){auto prefix=L"ProducerHost"+std::to_wstring(i);BridgeNames names{prefix+L"Performance",prefix+L"Object",prefix+L"Load",prefix+L"Alias",prefix+L"Global"};
        bool used=false;for(const auto& item:objects)if(item.alias)for(const auto& name:{names.performance,names.object,names.load,names.alias,names.global})if(!_wcsicmp(name.c_str(),item.alias->c_str()))used=true;if(!used)return names;
    }
}
}
bool uses_source_script_host(const ScriptDocument& doc){return doc.source().has_value()&&(!_wcsicmp(doc.language().c_str(),L"VBScript")||!_wcsicmp(doc.language().c_str(),L"JScript"));}
Bytes source_script_loader_bytes(const ScriptDocument& doc){if(!uses_source_script_host(doc))return doc.save_bytes();auto helper=doc;helper.set_properties(doc.name(),L"VBScript",bool(doc.flags()&1),bool(doc.flags()&2));const auto names=bridge_names(doc);helper.set_source(L"Dim "+names.performance+L"\r\nDim "+names.object+L"\r\nDim "+names.alias+L"\r\nSub "+names.load+L"()\r\nEval("+names.alias+L").Load\r\nEnd Sub\r\n");return helper.save_bytes();}
namespace {
void copy_text(WCHAR* out,size_t capacity,const WCHAR* in){if(!in){out[0]=0;return;}size_t n=0;while(n+1<capacity&&in[n]){out[n]=in[n];++n;}out[n]=0;}
void clear_exception(EXCEPINFO& e){SysFreeString(e.bstrSource);SysFreeString(e.bstrDescription);SysFreeString(e.bstrHelpFile);e={};}

// Container automation Load needs the owning OS Script's execution context.
// Keep loading lazy: invoke this bridge only when the authored script calls Load.
const GUID aliasBridgeId={0x46868603,0x3c21,0x4d1b,{0x96,0x24,0xad,0x07,0x98,0x72,0x21,0x45}};
struct AliasBridge:IUnknown{virtual IDispatch* original()=0;virtual HRESULT get_object(REFIID,void**,runtime::ScriptErrorInfo*)=0;};
class AliasDispatch final:public IDispatch,public AliasBridge {
    LONG refs_=1;IDispatch* object_;runtime::Script* helper_;BridgeNames names_;std::wstring alias_;DISPID load_=DISPID_UNKNOWN;
public:
    AliasDispatch(IDispatch* object,runtime::Script* helper,BridgeNames names,std::wstring alias):object_(object),helper_(helper),names_(std::move(names)),alias_(std::move(alias)){object_->AddRef();helper_->AddRef();LPOLESTR name=const_cast<WCHAR*>(L"Load");object_->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&load_);}
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
        if(id!=load_||load_==DISPID_UNKNOWN)return object_->Invoke(id,iid,locale,flags,args,out,exception,bad);
        if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;if(!(flags&DISPATCH_METHOD))return DISP_E_MEMBERNOTFOUND;if(!args)return E_POINTER;if(args->cNamedArgs)return DISP_E_NONAMEDARGS;if(args->cArgs)return DISP_E_BADPARAMCOUNT;
        VARIANT value;VariantInit(&value);value.vt=VT_BSTR;value.bstrVal=SysAllocString(alias_.c_str());if(!value.bstrVal)return E_OUTOFMEMORY;runtime::ScriptErrorInfo error{};error.size=sizeof(error);
        auto hr=helper_->SetVariableVariant(names_.alias.data(),value,FALSE,&error);VariantClear(&value);if(SUCCEEDED(hr))hr=helper_->CallRoutine(names_.load.data(),&error);
        if(FAILED(hr)&&exception){*exception={};exception->scode=FAILED(error.result)?error.result:hr;exception->bstrSource=SysAllocString(error.component);exception->bstrDescription=SysAllocString(error.description);return DISP_E_EXCEPTION;}if(SUCCEEDED(hr)&&out)VariantInit(out);return hr;
    }
};
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
    HRESULT report(HRESULT hr,runtime::ScriptErrorInfo* error){if(error){*error=site_?site_->last:runtime::ScriptErrorInfo{};error->size=sizeof(*error);if(FAILED(hr)&&!FAILED(error->result))error->result=hr;}return FAILED(hr)?DMUS_E_SCRIPT_ERROR_IN_SCRIPT:hr;}
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
