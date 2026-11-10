#pragma once
#include <windows.h>
#include <unknwn.h>
#include <oaidl.h>
namespace producer {
inline constexpr GUID IID_Strip = {0x893ee17a, 0x04a3, 0x11d3, {0x89,0x4c,0,0xc0,0x4f,0xbf,0x8d,0x15}};
inline constexpr GUID IID_TimelineEdit = {0x8640f4b2, 0x2b01, 0x11d2, {0x88,0xf9,0,0xc0,0x4f,0xbf,0x8d,0x15}};
// Recovered x86 ABI. Draw and message argument roles still need UI validation.
struct Strip : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Draw(HDC, DWORD view, LONG xOffset) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStripProperty(DWORD, VARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStripProperty(DWORD, VARIANT) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnWMMessage(UINT, WPARAM, LPARAM, LONG x, LONG y) = 0;
};
struct TimelineEdit : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Cut(IUnknown* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE Copy(IUnknown* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE Paste(IUnknown* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE Insert() = 0;
    virtual HRESULT STDMETHODCALLTYPE Delete() = 0;
    virtual HRESULT STDMETHODCALLTYPE SelectAll() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanCut() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanCopy() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanPaste(IUnknown* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanInsert() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanDelete() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanSelectAll() = 0;
};
}
