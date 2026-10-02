#pragma once
#include <windows.h>
#include <unknwn.h>
#include <oaidl.h>

namespace producer {
// TempoStripMgr 5.3.0.900 table RVA 0x2114. Names follow observed roles;
// this is a recovered ABI, not an acquired Producer SDK header.
struct StripManager : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID type) = 0; // 0x5893
    virtual HRESULT STDMETHODCALLTYPE GetParam(REFGUID type, LONG time, LONG* next, void* value) = 0; // 0x7191
    virtual HRESULT STDMETHODCALLTYPE SetParam(REFGUID type, LONG time, void* value) = 0; // 0xf62c
    virtual HRESULT STDMETHODCALLTYPE OnUpdate(REFGUID type, DWORD groups, void* data) = 0; // 0x769c
    virtual HRESULT STDMETHODCALLTYPE GetStripMgrProperty(DWORD property, VARIANT* value) = 0; // 0x5a96
    virtual HRESULT STDMETHODCALLTYPE SetStripMgrProperty(DWORD property, VARIANT value) = 0; // 0x5bd9
};
}
