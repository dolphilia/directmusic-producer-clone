#pragma once
#include <unknwn.h>

namespace producer {
// TempoStripMgr table RVA 0x20d0. The no-argument methods have ret 4;
// GetData and SetData use ret 8. Names describe recovered roles.
struct PropPageObject : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetData(void** data) = 0; // 0x6136
    virtual HRESULT STDMETHODCALLTYPE SetData(void* data) = 0; // 0x79a5
    virtual HRESULT STDMETHODCALLTYPE ShowProperties() = 0; // 0x6203
    virtual HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() = 0; // 0x58c3, E_NOTIMPL
};
}
