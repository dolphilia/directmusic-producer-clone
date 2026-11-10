#pragma once
#include <windows.h>
#include <oleidl.h>
#include <cstdio>

// A windowed OLE container for native integration probes. The parent never
// becomes visible. No COM registration or original-module patching is required.
class HiddenOleSite final : public IOleClientSite, public IOleInPlaceSite,
                            public IOleInPlaceFrame {
    ULONG references_ = 1;
    HWND window_ = nullptr;
public:
    HiddenOleSite() {
        window_ = CreateWindowExW(0, L"STATIC", L"Producer integration probe",
            WS_OVERLAPPED, 0, 0, 800, 300, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    }
    ~HiddenOleSite() { if (window_) DestroyWindow(window_); }
    HWND window() const { return window_; }
    RECT rectangle() const { RECT value{}; GetClientRect(window_, &value); return value; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == IID_IOleClientSite) *out = static_cast<IOleClientSite*>(this);
        else if (iid == IID_IOleWindow || iid == IID_IOleInPlaceSite) *out = static_cast<IOleInPlaceSite*>(this);
        else if (iid == IID_IOleInPlaceUIWindow || iid == IID_IOleInPlaceFrame) *out = static_cast<IOleInPlaceFrame*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG left = --references_; if (!left) delete this; return left;
    }
    HRESULT STDMETHODCALLTYPE SaveObject() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, IMoniker** out) override {
        if (!out) return E_POINTER; *out = nullptr; return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer** out) override {
        if (!out) return E_POINTER; *out = nullptr; return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE ShowObject() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestNewObjectLayout() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetWindow(HWND* out) override {
        if (!out) return E_POINTER; *out = window_; return window_ ? S_OK : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CanInPlaceActivate() override { std::puts("{\"operation\":\"site_can_activate\"}"); std::fflush(stdout); return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceActivate() override { std::puts("{\"operation\":\"site_activated\"}"); std::fflush(stdout); return S_OK; }
    HRESULT STDMETHODCALLTYPE OnUIActivate() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame** frame, IOleInPlaceUIWindow** document,
        LPRECT position, LPRECT clip, LPOLEINPLACEFRAMEINFO info) override {
        if (!frame || !document || !position || !clip || !info) return E_POINTER;
        std::puts("{\"operation\":\"site_window_context\"}"); std::fflush(stdout);
        *frame = static_cast<IOleInPlaceFrame*>(this); AddRef(); *document = nullptr;
        *position = *clip = rectangle();
        *info = {}; info->cb = sizeof(*info); info->hwndFrame = window_;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Scroll(SIZE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE DiscardUndoState() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE DeactivateAndUndo() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetBorder(LPRECT out) override {
        if (!out) return E_POINTER; *out = rectangle(); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS) override { return INPLACE_E_NOTOOLSPACE; }
    HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject*, LPCOLESTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetMenu(HMENU, HOLEMENU, HWND) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, WORD) override { return S_FALSE; }
};

class HiddenOleHost {
    HiddenOleSite* site_ = nullptr;
    IOleObject* object_ = nullptr;
    IOleInPlaceObject* inPlace_ = nullptr;
public:
    HRESULT activate(IOleObject* object) {
        site_ = new HiddenOleSite;
        if (!site_->window()) return HRESULT_FROM_WIN32(GetLastError());
        object_ = object; object_->AddRef();
        HRESULT hr = object_->SetClientSite(site_);
        RECT rect = site_->rectangle();
        if (SUCCEEDED(hr)) hr = object_->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, site_, 0, site_->window(), &rect);
        if (SUCCEEDED(hr)) hr = object_->QueryInterface(IID_IOleInPlaceObject, reinterpret_cast<void**>(&inPlace_));
        HWND child = nullptr;
        if (SUCCEEDED(hr)) hr = inPlace_->GetWindow(&child);
        if (SUCCEEDED(hr) && (!child || !IsWindow(child) || !IsChild(site_->window(), child) || IsWindowVisible(site_->window()))) hr = E_FAIL;
        return hr;
    }
    HRESULT close() {
        HRESULT first = S_OK;
        const auto keep = [&](HRESULT hr) { if (FAILED(hr) && SUCCEEDED(first)) first = hr; };
        if (inPlace_) { keep(inPlace_->InPlaceDeactivate()); inPlace_->Release(); inPlace_ = nullptr; }
        if (object_) { keep(object_->Close(OLECLOSE_NOSAVE)); keep(object_->SetClientSite(nullptr)); object_->Release(); object_ = nullptr; }
        if (site_) { site_->Release(); site_ = nullptr; }
        return first;
    }
    ~HiddenOleHost() { close(); }
};
