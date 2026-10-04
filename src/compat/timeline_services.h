#pragma once
#include <windows.h>
#include <unknwn.h>
#include <oaidl.h>
#include <objidl.h>
namespace producer::timeline {
inline HRESULT remove_page_object(IUnknown* timeline, IUnknown* object) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[19])(timeline, object);
}
inline HRESULT get_strip_manager(IUnknown* timeline, REFGUID type, DWORD groups, DWORD index, IUnknown** out) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, DWORD, DWORD, IUnknown**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[35])(timeline, type, groups, index, out);
}
inline HRESULT create_data_object(IUnknown* timeline, IUnknown** data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[43])(timeline, data);
}
inline HRESULT draw_music_lines(IUnknown* timeline, HDC dc, DWORD groups, LONG offset) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, HDC, DWORD, DWORD, DWORD, LONG);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[9])(timeline, dc, 1, groups, 0, offset);
}
inline HRESULT measure_beat_to_position(IUnknown* timeline, DWORD groups, LONG measure, LONG beat, LONG* position) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG, LONG, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[16])(timeline, groups, 0, measure, beat, position);
}
inline HRESULT clocks_to_position(IUnknown* timeline, LONG time, LONG* position) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, LONG, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[7])(timeline, time, position);
}
inline HRESULT get_paste_mode(IUnknown* timeline, DWORD* mode) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[44])(timeline, mode);
}
inline HRESULT get_marker(IUnknown* timeline, DWORD marker, LONG* time) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[6])(timeline, marker, 0, time);
}
inline HRESULT get_strip_property(IUnknown* timeline, IUnknown* strip, DWORD property, VARIANT* value) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, DWORD, VARIANT*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[30])(timeline, strip, property, value);
}
inline HRESULT set_marker(IUnknown* timeline, DWORD marker, LONG time) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[5])(timeline, marker, 0, time);
}
}
namespace producer::timeline_data {
inline HRESULT set_boundaries(IUnknown* data, LONG start, LONG end) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, LONG, LONG);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[8])(data, start, end);
}
inline HRESULT export_data(IUnknown* data, IDataObject** destination) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IDataObject**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[10])(data, destination);
}
inline HRESULT import_data(IUnknown* data, IDataObject* source) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IDataObject*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[9])(data, source);
}
inline HRESULT get_stream(IUnknown* data, UINT format, IStream** stream) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT, IStream**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[6])(data, format, stream);
}
inline HRESULT get_boundaries(IUnknown* data, LONG* start, LONG* end) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, LONG*, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[7])(data, start, end);
}
inline HRESULT add_format(IUnknown* data, UINT format, IStream* stream) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT, IStream*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[3])(data, format, stream);
}
inline HRESULT format_available(IUnknown* data, UINT format) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[5])(data, format);
}
}
namespace producer::timeline {
inline HRESULT add_notification(IUnknown* timeline, IUnknown* manager, REFGUID type, DWORD groups) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, REFGUID, DWORD);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[40])(timeline, manager, type, groups);
}
inline HRESULT remove_notification(IUnknown* timeline, IUnknown* manager, REFGUID type, DWORD groups) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, REFGUID, DWORD);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[41])(timeline, manager, type, groups);
}
inline HRESULT get_parameter(IUnknown* timeline, REFGUID type, DWORD groups, LONG time, void* value) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, DWORD, DWORD, LONG, LONG*, void*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[33])(timeline, type, groups, 0, time, nullptr, value);
}
inline HRESULT get_property(IUnknown* timeline, DWORD property, VARIANT* value) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[11])(timeline, property, value);
}
inline HRESULT set_property(IUnknown* timeline, DWORD property, VARIANT value) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[10])(timeline, property, value);
}
inline HRESULT clocks_to_measure_beat(IUnknown* timeline, DWORD groups, LONG time, LONG* measure, LONG* beat) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG, LONG*, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[13])(timeline, groups, 0, time, measure, beat);
}
inline HRESULT measure_beat_to_clocks(IUnknown* timeline, DWORD groups, LONG measure, LONG beat, LONG* time) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG, LONG, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[15])(timeline, groups, 0, measure, beat, time);
}
inline HRESULT position_to_measure_beat(IUnknown* timeline, DWORD groups, LONG position, LONG* measure, LONG* beat) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG, LONG*, LONG*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[14])(timeline, groups, 0, position, measure, beat);
}
inline HRESULT insert_strip(IUnknown* timeline, IUnknown* strip, REFCLSID type, DWORD groups) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, REFCLSID, DWORD, DWORD);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[36])(timeline, strip, type, groups, 0);
}
inline HRESULT remove_strip(IUnknown* timeline, IUnknown* strip) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[32])(timeline, strip);
}
inline HRESULT invalidate(IUnknown* timeline, IUnknown* strip) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, const RECT*, BOOL);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[17])(timeline, strip, nullptr, TRUE);
}
inline HRESULT data_changed(IUnknown* timeline, IUnknown* manager) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[21])(timeline, manager);
}
inline HRESULT notify(IUnknown* timeline, REFGUID type, DWORD groups, void* data) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, DWORD, void*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline))[42])(timeline, type, groups, data);
}
}
