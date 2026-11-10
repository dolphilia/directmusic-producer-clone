#pragma once
#include <windows.h>
#include <unknwn.h>
#include <oaidl.h>
#include <objidl.h>
#include <string>
#include "compat/producer_ids.h"

// Calls into the original Timeline.dll, never a simulated timeline.
// Table RVA 0x245c: insert slot3 (0xe1e2), remove slot31 (0x104ac),
// GetParam slot33 (0xb487), EnumStrip slot37 (0xbb60).
class ReferenceTimeline {
    HMODULE module_ = nullptr;
    IUnknown* timeline_ = nullptr;
    IUnknown* timeManager_ = nullptr;
public:
    HRESULT load(const wchar_t* tempoDll, const wchar_t* explicitPath = nullptr) {
        std::wstring path;
        if (explicitPath) path = explicitPath;
        else {
            path = tempoDll;
            path.resize(path.find_last_of(L"/\\") + 1);
            path += L"Timeline.dll";
        }
        module_ = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module_) return HRESULT_FROM_WIN32(GetLastError());
        using GetClass = HRESULT (STDMETHODCALLTYPE*)(REFCLSID, REFIID, void**);
        const auto getClass = reinterpret_cast<GetClass>(GetProcAddress(module_, "DllGetClassObject"));
        if (!getClass) return E_NOINTERFACE;
        IClassFactory* factory = nullptr;
        HRESULT hr = getClass(producer::CLSID_Timeline, IID_IClassFactory, reinterpret_cast<void**>(&factory));
        if (SUCCEEDED(hr)) {
            hr = factory->CreateInstance(nullptr, producer::IID_IDMUSProdTimeline, reinterpret_cast<void**>(&timeline_));
            factory->Release();
        }
        return hr;
    }
    bool loaded() const { return timeline_ != nullptr; }
    // Read-only diagnostic for the pinned 5.3.0.900 Timeline used by the runner.
    // DllCanUnloadNow RVA 0x5d86 compares this global; Export increments it.
    LONG module_counter() const {
        return module_ ? *reinterpret_cast<const volatile LONG*>(reinterpret_cast<const unsigned char*>(module_) + 0x1e2dc) : -1;
    }
    HRESULT query(REFIID iid, void** out) { return timeline_->QueryInterface(iid, out); }
    HRESULT set_horizontal_scroll(LONG pixels) {
        // SetTimelineProperty(9), RVA 0x14343. Its scroll-bar path needs a window.
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
        VARIANT value{}; value.vt = VT_I4; value.lVal = pixels;
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[10])(timeline_, 9, value);
    }
    HRESULT connect_time_strip() {
        if (timeManager_) return S_FALSE;
        const GUID classId = {0x884f3f04,0xbfe0,0x11d0,{0xbb,0xdb,0,0xa0,0xc9,0x22,0xe6,0xeb}};
        using GetClass = HRESULT (STDMETHODCALLTYPE*)(REFCLSID, REFIID, void**);
        const auto getClass = reinterpret_cast<GetClass>(GetProcAddress(module_, "DllGetClassObject"));
        IClassFactory* factory = nullptr;
        HRESULT hr = getClass(classId, IID_IClassFactory, reinterpret_cast<void**>(&factory));
        if (SUCCEEDED(hr)) {
            hr = factory->CreateInstance(nullptr, producer::IID_IDMUSProdStripMgr, reinterpret_cast<void**>(&timeManager_));
            factory->Release();
        }
        if (SUCCEEDED(hr)) {
            using Set = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
            VARIANT value{}; value.vt = VT_UNKNOWN; value.punkVal = timeline_;
            hr = reinterpret_cast<Set>((*reinterpret_cast<void***>(timeManager_))[8])(timeManager_, 0, value);
        }
        return hr;
    }
    HRESULT disconnect_time_strip() {
        if (!timeManager_) return S_FALSE;
        using Set = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
        VARIANT value{}; value.vt = VT_UNKNOWN;
        const HRESULT hr = reinterpret_cast<Set>((*reinterpret_cast<void***>(timeManager_))[8])(timeManager_, 0, value);
        timeManager_->Release(); timeManager_ = nullptr;
        return hr;
    }
    HRESULT create_data_object(IUnknown** out) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown**);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[43])(timeline_, out);
    }
    static HRESULT set_data_boundaries(IUnknown* data, LONG start, LONG end) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, LONG, LONG);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[8])(data, start, end);
    }
    static HRESULT data_format_available(IUnknown* data, UINT format) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[5])(data, format);
    }
    static HRESULT data_stream(IUnknown* data, UINT format, IStream** stream) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, UINT, IStream**);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(data))[6])(data, format, stream);
    }
    HRESULT insert(IUnknown* manager) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, DWORD);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[3])(timeline_, manager, 1);
    }
    HRESULT remove(IUnknown* manager) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[31])(timeline_, manager);
    }
    HRESULT get(REFGUID type, LONG time, LONG* next, void* value) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, DWORD, DWORD, LONG, LONG*, void*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[33])(timeline_, type, 1, 0, time, next, value);
    }
    HRESULT enum_strip(DWORD index, IUnknown** strip) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, IUnknown**);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[37])(timeline_, index, strip);
    }
    HRESULT find_manager_strip(IUnknown* manager, IUnknown** out) {
        *out = nullptr;
        IUnknown* identity = nullptr;
        HRESULT hr = manager->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&identity));
        if (FAILED(hr)) return hr;
        for (DWORD index = 0; index < 256; ++index) {
            IUnknown* strip = nullptr;
            hr = enum_strip(index, &strip);
            if (FAILED(hr) || !strip) break;
            VARIANT property{};
            using GetProperty = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT*);
            const HRESULT queried = reinterpret_cast<GetProperty>((*reinterpret_cast<void***>(strip))[4])(strip, 12, &property);
            IUnknown* owner = nullptr;
            if (SUCCEEDED(queried) && property.vt == VT_UNKNOWN && property.punkVal)
                property.punkVal->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&owner));
            const bool same = owner && owner == identity;
            if (owner) owner->Release();
            if (property.vt == VT_UNKNOWN && property.punkVal) property.punkVal->Release();
            if (same) { *out = strip; identity->Release(); return S_OK; }
            strip->Release();
        }
        identity->Release(); return E_FAIL;
    }
    HRESULT set_length(LONG length) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
        VARIANT value{}; value.vt = VT_I4; value.lVal = length;
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[10])(timeline_, 1, value);
    }
    HRESULT clocks_to_measure_beat(LONG time, LONG* measure, LONG* beat) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG, LONG*, LONG*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[13])(timeline_, 0xffffffff, 0, time, measure, beat);
    }
    HRESULT clocks_to_position(LONG time, LONG* position) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, LONG, LONG*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[7])(timeline_, time, position);
    }
    HRESULT set_zoom(double zoom) {
        // SetTimelineProperty slot10, property8: RVA 0x14313 checks VT_R8
        // and stores the scale consumed by ClocksToPosition at RVA 0xbcc6.
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
        VARIANT value{}; value.vt = VT_R8; value.dblVal = zoom;
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[10])(timeline_, 8, value);
    }
    HRESULT get_marker(DWORD marker, LONG* time) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[6])(timeline_, marker, 0, time);
    }
    HRESULT set_marker(DWORD marker, LONG time) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, DWORD, LONG);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[5])(timeline_, marker, 0, time);
    }
    HRESULT get_paste_mode(DWORD* mode) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[44])(timeline_, mode);
    }
    HRESULT set_paste_mode(DWORD mode) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[45])(timeline_, mode);
    }
    HRESULT set_boolean_property(DWORD property, bool enabled) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
        VARIANT value{}; value.vt = VT_BOOL; value.boolVal = enabled ? 1 : 0;
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[10])(timeline_, property, value);
    }
    HRESULT get_property(DWORD property, VARIANT* value) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[11])(timeline_, property, value);
    }
    HRESULT notify(REFGUID type, DWORD groups, void* data) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, DWORD, void*);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[42])(timeline_, type, groups, data);
    }
    HRESULT add_notification(IUnknown* manager, REFGUID type, DWORD groups) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, REFGUID, DWORD);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[40])(timeline_, manager, type, groups);
    }
    HRESULT remove_notification(IUnknown* manager, REFGUID type, DWORD groups) {
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, REFGUID, DWORD);
        return reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_))[41])(timeline_, manager, type, groups);
    }
    HRESULT close() {
        if (timeManager_) disconnect_time_strip();
        if (timeline_) { timeline_->Release(); timeline_ = nullptr; }
        if (!module_) return S_OK;
        using CanUnload = HRESULT (STDMETHODCALLTYPE*)();
        auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(module_, "DllCanUnloadNow"));
        const HRESULT hr = canUnload ? canUnload() : E_NOINTERFACE;
        if (hr == S_OK) { FreeLibrary(module_); module_ = nullptr; }
        return hr;
    }
    ~ReferenceTimeline() { close(); }
};
