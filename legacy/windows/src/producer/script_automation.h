#pragma once
#include "compat/script_runtime.h"
namespace producer::app {
// Owns the public OS automation object. All documented Performance methods
// except Trace delegate unchanged, including optional-duration volume changes.
// Trace uses the public PMSG graph, never source-text reconstruction.
class ScriptPerformanceAutomation final:public IDispatch {
    LONG references_=1;
    runtime::Performance* performance_;
    IDispatch* delegate_;
    DISPID traceId_;
public:
    ScriptPerformanceAutomation(runtime::Performance*,IDispatch*,DISPID);
    ~ScriptPerformanceAutomation();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override;
    ULONG STDMETHODCALLTYPE AddRef()override;
    ULONG STDMETHODCALLTYPE Release()override;
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT*)override;
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT,LCID,ITypeInfo**)override;
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID,LPOLESTR*,UINT,LCID,DISPID*)override;
    HRESULT STDMETHODCALLTYPE Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*)override;
};
}
