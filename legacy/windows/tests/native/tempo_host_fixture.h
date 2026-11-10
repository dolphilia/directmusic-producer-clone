#pragma once
#include <windows.h>
#include <objidl.h>
#include <oleauto.h>
#include <cstdio>
#include <new>
#include "compat/producer_ids.h"

// An explicitly limited observation host, not a replacement Producer Framework.
// Unknown slots terminate the probe; their signatures have not been recovered.
namespace fixture {
[[noreturn]] inline void unexpected(const char* method) {
    std::printf("{\"operation\":\"unexpected_fixture_call\",\"method\":\"%s\"}\n", method);
    std::fflush(stdout);
    ExitProcess(3);
}

struct StreamInfo {
    DWORD fileType;
    GUID format;
    IUnknown* directoryNode;
};
static_assert(sizeof(StreamInfo) == 24, "Observed x86 PersistInfo layout");

struct PersistInfo : IUnknown {
    virtual void STDMETHODCALLTYPE UnknownSlot3() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamInfo(StreamInfo*) = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot5() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot6() = 0;
};

// Host CMemStream: constructor RVA 0x299b3, GetStreamInfo RVA 0x29c09.
// This wrapper implements the stream and metadata used by the tempo test only.
class MemoryStream final : public IStream, public PersistInfo {
    ULONG refs_ = 1;
    IStream* inner_;
    StreamInfo info_;
public:
    MemoryStream(IStream* inner, DWORD type, GUID format)
        : inner_(inner), info_{type, format, nullptr} {}
    ~MemoryStream() { inner_->Release(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == IID_IStream || iid == IID_ISequentialStream)
            *out = static_cast<IStream*>(this);
        else if (iid == producer::IID_IDMUSProdPersistInfo)
            *out = static_cast<PersistInfo*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = --refs_; if (!count) delete this; return count;
    }
    void STDMETHODCALLTYPE UnknownSlot3() override { unexpected("PersistInfo.slot3"); }
    void STDMETHODCALLTYPE UnknownSlot5() override { unexpected("PersistInfo.slot5"); }
    void STDMETHODCALLTYPE UnknownSlot6() override { unexpected("PersistInfo.slot6"); }
    HRESULT STDMETHODCALLTYPE GetStreamInfo(StreamInfo* out) override {
        if (!out) return E_POINTER;
        *out = info_;
        std::puts("{\"operation\":\"fixture_get_stream_info\"}");
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Read(void* p, ULONG n, ULONG* read) override {
        const HRESULT hr = inner_->Read(p, n, read);
        // Host RVA 0x29ab9 reports E_FAIL for a short read with a count pointer.
        return read && *read != n ? E_FAIL : hr;
    }
    HRESULT STDMETHODCALLTYPE Write(const void* p, ULONG n, ULONG* written) override { return inner_->Write(p, n, written); }
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER n, DWORD origin, ULARGE_INTEGER* pos) override { return inner_->Seek(n, origin, pos); }
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER n) override { return inner_->SetSize(n); }
    HRESULT STDMETHODCALLTYPE CopyTo(IStream* to, ULARGE_INTEGER n, ULARGE_INTEGER* read, ULARGE_INTEGER* written) override { return inner_->CopyTo(to, n, read, written); }
    HRESULT STDMETHODCALLTYPE Commit(DWORD flags) override { return inner_->Commit(flags); }
    HRESULT STDMETHODCALLTYPE Revert() override { return inner_->Revert(); }
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER start, ULARGE_INTEGER size, DWORD type) override { return inner_->LockRegion(start, size, type); }
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER start, ULARGE_INTEGER size, DWORD type) override { return inner_->UnlockRegion(start, size, type); }
    HRESULT STDMETHODCALLTYPE Stat(STATSTG* stat, DWORD flags) override { return inner_->Stat(stat, flags); }
    HRESULT STDMETHODCALLTYPE Clone(IStream**) override { unexpected("IStream.Clone"); }
};

struct FrameworkAbi : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE FindComponent(REFCLSID, IUnknown**) = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot4() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot5() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot6() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot7() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot8() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot9() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot10() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot11() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot12() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot13() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot14() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot15() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot16() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot17() = 0;
    virtual void STDMETHODCALLTYPE UnknownSlot18() = 0;
    virtual HRESULT STDMETHODCALLTYPE AllocMemoryStream(DWORD, GUID, IStream**) = 0;
};

class Framework final : public FrameworkAbi {
    ULONG refs_ = 1;
    IUnknown* propertySheet_ = nullptr; // Borrowed from the enclosing test.
public:
    explicit Framework(IUnknown* propertySheet = nullptr) : propertySheet_(propertySheet) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        const GUID sheetIid = {0x3095f6e0,0xc160,0x11d0,{0x89,0xae,0,0xa0,0xc9,5,0x41,0x29}};
        if (iid == sheetIid && propertySheet_) return propertySheet_->QueryInterface(iid, out);
        if (iid != IID_IUnknown && iid != producer::IID_IDMUSProdFramework) return E_NOINTERFACE;
        *out = static_cast<FrameworkAbi*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = --refs_; if (!count) delete this; return count;
    }
    HRESULT STDMETHODCALLTYPE FindComponent(REFCLSID, IUnknown** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        // Empty component list, matching the host failure at RVA 0x1a024.
        std::puts("{\"operation\":\"fixture_find_component\",\"available\":false}");
        return E_FAIL;
    }
#define UNUSED_SLOT(n) void STDMETHODCALLTYPE UnknownSlot##n() override { unexpected("Framework.slot" #n); }
    UNUSED_SLOT(4) UNUSED_SLOT(5) UNUSED_SLOT(6) UNUSED_SLOT(7) UNUSED_SLOT(8)
    UNUSED_SLOT(9) UNUSED_SLOT(10) UNUSED_SLOT(11) UNUSED_SLOT(12) UNUSED_SLOT(13)
    UNUSED_SLOT(14) UNUSED_SLOT(15) UNUSED_SLOT(16) UNUSED_SLOT(17) UNUSED_SLOT(18)
#undef UNUSED_SLOT
    HRESULT STDMETHODCALLTYPE AllocMemoryStream(DWORD type, GUID format, IStream** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (!type) return E_INVALIDARG;
        std::printf("{\"operation\":\"fixture_alloc_stream\",\"file_type\":%lu,\"known_format\":%s}\n",
            type, format == producer::GUID_TempoStreamFormat ? "true" : "false");
        IStream* inner = nullptr;
        HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &inner);
        if (FAILED(hr)) return hr;
        auto wrapper = new (std::nothrow) MemoryStream(inner, type, format);
        if (!wrapper) { inner->Release(); return E_OUTOFMEMORY; }
        *out = static_cast<IStream*>(wrapper); return S_OK;
    }
};

// TempoStripMgr slot8 RVA 0x5bd9, ret 0x18: this, DWORD, VARIANT by value.
inline HRESULT set_property(IUnknown* strip, DWORD property, IUnknown* value) {
    using SetProperty = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT);
    auto table = *reinterpret_cast<void***>(strip);
    VARIANT variant{}; variant.vt = VT_UNKNOWN; variant.punkVal = value;
    return reinterpret_cast<SetProperty>(table[8])(strip, property, variant);
}
}
