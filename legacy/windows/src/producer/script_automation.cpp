#include "script_automation.h"
#include <cstring>
#include <limits>
namespace producer::app {
ScriptPerformanceAutomation::ScriptPerformanceAutomation(runtime::Performance* p,IDispatch* d,DISPID id):performance_(p),delegate_(d),traceId_(id){performance_->AddRef();delegate_->AddRef();}
ScriptPerformanceAutomation::~ScriptPerformanceAutomation(){delegate_->Release();performance_->Release();}
HRESULT ScriptPerformanceAutomation::QueryInterface(REFIID id,void** out){if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IDispatch){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
ULONG ScriptPerformanceAutomation::AddRef(){return InterlockedIncrement(&references_);}
ULONG ScriptPerformanceAutomation::Release(){auto n=InterlockedDecrement(&references_);if(!n)delete this;return n;}
HRESULT ScriptPerformanceAutomation::GetTypeInfoCount(UINT* n){return delegate_->GetTypeInfoCount(n);}
HRESULT ScriptPerformanceAutomation::GetTypeInfo(UINT i,LCID locale,ITypeInfo** out){return delegate_->GetTypeInfo(i,locale,out);}
HRESULT ScriptPerformanceAutomation::GetIDsOfNames(REFIID id,LPOLESTR* names,UINT n,LCID locale,DISPID* out){return delegate_->GetIDsOfNames(id,names,n,locale,out);}
HRESULT ScriptPerformanceAutomation::Invoke(DISPID id,REFIID iid,LCID locale,WORD flags,DISPPARAMS* args,VARIANT* result,EXCEPINFO* error,UINT* argumentError){
    if(id!=traceId_)return delegate_->Invoke(id,iid,locale,flags,args,result,error,argumentError);
    if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;
    if(!(flags&DISPATCH_METHOD))return DISP_E_MEMBERNOTFOUND;
    if(!args)return E_POINTER;
    if(args->cNamedArgs)return DISP_E_NONAMEDARGS;
    if(args->cArgs!=1)return DISP_E_BADPARAMCOUNT;
    if(!args->rgvarg)return E_POINTER;
    VARIANT text;VariantInit(&text);
    auto hr=VariantChangeTypeEx(&text,&args->rgvarg[0],locale,0,VT_BSTR);
    if(FAILED(hr)){if(argumentError)*argumentError=0;return hr;}
    const auto length=SysStringLen(text.bstrVal);
    // Frozen SDK DMUS_LYRIC_PMSG: string at56, structure64 on Win32.
    // Use the full structure size plus additional characters, including NUL.
    if(length>(std::numeric_limits<ULONG>::max()-64)/sizeof(WCHAR)){VariantClear(&text);return E_OUTOFMEMORY;}
    for(UINT i=0;i<length;++i)if(!text.bstrVal[i]){VariantClear(&text);return DISP_E_TYPEMISMATCH;}
    const ULONG bytes=64+length*sizeof(WCHAR);
    runtime::Message* message=nullptr;hr=performance_->AllocPMsg(bytes,&message);
    if(FAILED(hr)||!message){VariantClear(&text);return FAILED(hr)?hr:E_OUTOFMEMORY;}
    std::memset(message,0,bytes);message->size=bytes;message->flags=0x41;message->type=14;message->group=0xffffffff;
    hr=performance_->GetTime(&message->referenceTime,nullptr);
    if(SUCCEEDED(hr)){
        auto* destination=reinterpret_cast<WCHAR*>(reinterpret_cast<BYTE*>(message)+sizeof(runtime::Message));
        if(length)std::memcpy(destination,text.bstrVal,length*sizeof(WCHAR));destination[length]=0;
        runtime::Graph* graph=nullptr;hr=performance_->GetGraph(&graph);
        if(SUCCEEDED(hr)&&graph){hr=graph->StampPMsg(message);graph->Release();}
        else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
        if(SUCCEEDED(hr))hr=performance_->SendPMsg(message);
    }
    if(FAILED(hr))performance_->FreePMsg(message);
    VariantClear(&text);if(SUCCEEDED(hr)&&result)VariantInit(result);return hr;
}
}
