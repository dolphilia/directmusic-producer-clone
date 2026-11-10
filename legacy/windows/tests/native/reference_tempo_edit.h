#pragma once
#include <windows.h>
#include <unknwn.h>
#include <oaidl.h>

namespace reference_edit {
// TempoStripMgr QueryInterface RVA 0x87d7 -> subobject +8.
inline constexpr GUID TimelineEditIid =
    {0x8640f4b2, 0x2b01, 0x11d2, {0x88, 0xf9, 0x00, 0xc0, 0x4f, 0xbf, 0x8d, 0x15}};
inline HRESULT get_strip_manager(IUnknown* strip, VARIANT* out) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(strip))[4])(strip, 12, out);
}
inline HRESULT set_strip_property(IUnknown* strip, DWORD property, VARIANT value) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(strip))[5])(strip, property, value);
}
inline HRESULT select_all(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[8])(edit); // RVA 0x92ae
}
inline HRESULT can_copy(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[10])(edit);
}
inline HRESULT can_cut(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[9])(edit);
}
inline HRESULT can_paste(IUnknown* edit, IUnknown* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[11])(edit, data);
}
inline HRESULT copy(IUnknown* edit, IUnknown* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[4])(edit, data);
}
inline HRESULT paste(IUnknown* edit, IUnknown* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[5])(edit, data);
}
inline HRESULT cut(IUnknown* edit, IUnknown* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[3])(edit, data);
}
inline HRESULT insert(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[6])(edit); // RVA 0x9082
}
inline HRESULT can_insert(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[12])(edit); // RVA 0x9571
}
inline HRESULT mouse_release(IUnknown* strip, LONG x) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT, WPARAM, LPARAM, LONG, LONG);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(strip))[6])(strip, WM_LBUTTONUP, 0, 0, x, 0);
}
inline HRESULT delete_selected(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[7])(edit); // RVA 0x9226
}
inline HRESULT can_delete(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[13])(edit);
}
inline HRESULT can_select_all(IUnknown* edit) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(edit))[14])(edit);
}
inline HRESULT get_data(IUnknown* properties, void** out) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, void**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(properties))[3])(properties, out); // RVA 0x6136
}
inline HRESULT set_data(IUnknown* properties, void* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, void*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(properties))[4])(properties, data); // RVA 0x79a5
}
}
