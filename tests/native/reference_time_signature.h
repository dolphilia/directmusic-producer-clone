#pragma once
#include <windows.h>
#include <unknwn.h>
#include <objidl.h>
#include <vector>
#include "compat/producer_ids.h"

// Original TimeSigStripMgr supplies the timeline's meter map for position tests.
// Class identity is recorded in docs/analysis/com-classes.csv.
class ReferenceTimeSignature {
    HMODULE module_ = nullptr;
    IUnknown* manager_ = nullptr;
public:
    static std::vector<unsigned char> meter_stream(BYTE beats, BYTE denominator) {
        // 'tims', payload12, record size8, time0, numerator4/denominator4/grids4.
        // Load RVA 0x7cc6 -> reader 0x7884; this is synthetic input.
        return {'t','i','m','s',12,0,0,0,8,0,0,0,0,0,0,0,beats,denominator,4,0};
    }
    static std::vector<unsigned char> default_meter_stream() { return meter_stream(4, 4); }
    HRESULT load(const wchar_t* path) {
        module_ = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module_) return HRESULT_FROM_WIN32(GetLastError());
        using GetClass = HRESULT (STDMETHODCALLTYPE*)(REFCLSID, REFIID, void**);
        const auto getClass = reinterpret_cast<GetClass>(GetProcAddress(module_, "DllGetClassObject"));
        if (!getClass) return E_NOINTERFACE;
        constexpr GUID classId = {0x8c6005d2,0xabda,0x11d2,{0xb0,0xd9,0,0x10,0x5a,0x26,0x62,0x0b}};
        IClassFactory* factory = nullptr;
        HRESULT hr = getClass(classId, IID_IClassFactory, reinterpret_cast<void**>(&factory));
        if (SUCCEEDED(hr)) {
            hr = factory->CreateInstance(nullptr, producer::IID_IDMUSProdStripMgr, reinterpret_cast<void**>(&manager_));
            factory->Release();
        }
        return hr;
    }
    IUnknown* manager() const { return manager_; }
    HRESULT seed_meter(BYTE beats, BYTE denominator) {
        IPersistStream* persist = nullptr;
        HRESULT hr = manager_->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&persist));
        if (FAILED(hr)) return hr;
        IStream* stream = nullptr;
        hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
        if (SUCCEEDED(hr)) {
            const auto bytes = meter_stream(beats, denominator);
            ULONG written = 0;
            hr = stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written);
            if (SUCCEEDED(hr) && written != bytes.size()) hr = E_FAIL;
            LARGE_INTEGER zero{};
            if (SUCCEEDED(hr)) hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
            if (SUCCEEDED(hr)) hr = persist->Load(stream);
            stream->Release();
        }
        persist->Release(); return hr;
    }
    HRESULT seed_default_meter() { return seed_meter(4, 4); }
    HRESULT close() {
        if (manager_) { manager_->Release(); manager_ = nullptr; }
        if (!module_) return S_OK;
        using CanUnload = HRESULT (STDMETHODCALLTYPE*)();
        const auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(module_, "DllCanUnloadNow"));
        const HRESULT hr = canUnload ? canUnload() : E_NOINTERFACE;
        if (hr == S_OK) { FreeLibrary(module_); module_ = nullptr; }
        return hr;
    }
    ~ReferenceTimeSignature() { close(); }
};
