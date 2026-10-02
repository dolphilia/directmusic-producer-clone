#pragma once
#include <windows.h>
#include <oaidl.h>
#include "prop_page_object.h"

namespace producer {
// TempoStripMgr RVA 0x2624; Timeline RVA 0x1980.
inline constexpr GUID IID_PropPageManager =
    {0x3095f6e1,0xc160,0x11d0,{0x89,0xae,0,0xa0,0xc9,5,0x41,0x29}};
inline constexpr GUID IID_PropSheet =
    {0x3095f6e0,0xc160,0x11d0,{0x89,0xae,0,0xa0,0xc9,5,0x41,0x29}};
// Recovered table RVA 0x19f8. Slot11 is a C++ deleting destructor,
// not a callable COM method. Property page handles occupy pointer-sized cells.
struct PropPageManager : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetPropertySheetTitle(BSTR*, BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertySheetPages(IUnknown*, HANDLE*, SHORT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnRemoveFromPropertySheet() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetObject(PropPageObject*) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveObject(PropPageObject*) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsEqualObject(PropPageObject*) = 0;
    virtual HRESULT STDMETHODCALLTYPE RefreshData() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsEqualPageManagerGUID(REFGUID) = 0;
};
}
