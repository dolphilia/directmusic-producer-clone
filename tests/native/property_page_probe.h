#pragma once
#include "compat/prop_page_manager.h"
#include "compat/timeline_services.h"
#ifdef PRODUCER_WINDOWED_PROBE
#include <commctrl.h>
#include <prsht.h>
#endif

namespace property_probe {
// A headless property-sheet service: captures the page manager, but deliberately
// does not create page HWNDs. Only slots called by the observed Timeline path
// are implemented; any unexpected ABI call terminates the probe.
struct SheetAbi : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetPageManager(producer::PropPageManager*) = 0;
    virtual void STDMETHODCALLTYPE Slot4() = 0;
    virtual void STDMETHODCALLTYPE Slot5() = 0;
    virtual void STDMETHODCALLTYPE Slot6() = 0;
    virtual void STDMETHODCALLTYPE Slot7() = 0;
    virtual void STDMETHODCALLTYPE Slot8() = 0;
    virtual HRESULT STDMETHODCALLTYPE RefreshTitle() = 0;
    virtual HRESULT STDMETHODCALLTYPE RefreshPage() = 0;
    virtual void STDMETHODCALLTYPE Slot11() = 0;
    virtual void STDMETHODCALLTYPE Slot12() = 0;
    virtual void STDMETHODCALLTYPE Slot13() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsShowing() = 0;
};
class Sheet final : public SheetAbi {
public:
    ULONG refs = 1;
    unsigned setCalls = 0, visibleCalls = 0;
    HRESULT setResult = S_OK, visibleResult = S_OK;
    producer::PropPageManager* page = nullptr;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != IID_IUnknown && iid != producer::IID_PropSheet) return E_NOINTERFACE;
        *out = static_cast<SheetAbi*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE SetPageManager(producer::PropPageManager* value) override {
        ++setCalls;
        if (FAILED(setResult)) return setResult;
        if (page != value) { clear(); page = value; page->AddRef(); }
        return setResult;
    }
    HRESULT clear() {
        HRESULT hr = S_OK;
        if (page) { auto old = page; page = nullptr; hr = old->OnRemoveFromPropertySheet(); old->Release(); }
        return hr;
    }
    HRESULT STDMETHODCALLTYPE RefreshTitle() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE RefreshPage() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE IsShowing() override { ++visibleCalls; return visibleResult; }
#define PAGE_UNUSED(n) void STDMETHODCALLTYPE Slot##n() override { fixture::unexpected("PropSheet.slot" #n); }
    PAGE_UNUSED(4) PAGE_UNUSED(5) PAGE_UNUSED(6) PAGE_UNUSED(7) PAGE_UNUSED(8)
    PAGE_UNUSED(11) PAGE_UNUSED(12) PAGE_UNUSED(13)
#undef PAGE_UNUSED
};
class Object final : public producer::PropPageObject {
public:
    ULONG refs = 1;
    unsigned gets = 0, removals = 0;
    HRESULT getResult = S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != IID_IUnknown && iid != producer::IID_IDMUSProdPropPageObject) return E_NOINTERFACE;
        *out = static_cast<producer::PropPageObject*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE GetData(void** out) override { ++gets; *out = nullptr; return getResult; }
    HRESULT STDMETHODCALLTYPE SetData(void*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ShowProperties() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() override { ++removals; return E_NOTIMPL; }
};

#ifdef PRODUCER_WINDOWED_PROBE
struct PageData {
    LONG time = 900; double tempo = 137.25;
    LONG measure = 2, beat = 3, tick = 132; DWORD extra = 0; WORD flags = 0;
};
static_assert(offsetof(PageData, flags) == 32);
class DataObject final : public producer::PropPageObject {
public:
    PageData data{};
    bool present = true;
    unsigned sets = 0, removals = 0;
    ULONG refs = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER; *out = nullptr;
        if (iid != IID_IUnknown && iid != producer::IID_IDMUSProdPropPageObject) return E_NOINTERFACE;
        *out = this; AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE GetData(void** out) override { *out = present ? &data : nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetData(void* value) override { ++sets; std::memcpy(&data,value,0x22); return S_OK; }
    HRESULT STDMETHODCALLTYPE ShowProperties() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() override { ++removals; return E_NOTIMPL; }
};
inline int CALLBACK sheet_callback(HWND, UINT message, LPARAM value) {
    if (message == PSCB_PRECREATE) {
        auto bytes = reinterpret_cast<BYTE*>(value);
        const bool extended = reinterpret_cast<WORD*>(bytes)[1] == 0xffff;
        auto style = reinterpret_cast<DWORD*>(bytes + (extended ? 12 : 0));
        *style = (*style & ~(WS_VISIBLE | WS_POPUP)) | WS_CHILD;
    }
    return 0;
}
using PageExercise = bool (*)(HWND, producer::PropPageManager*, producer::PropPageObject*, void*);
inline bool native_page(producer::PropPageManager* manager, Sheet& sheet,
    producer::PropPageObject* track, PageExercise exercise, void* context) {
    std::puts("{\"operation\":\"begin_native_page_probe\"}"); std::fflush(stdout);
    INITCOMMONCONTROLSEX init{sizeof(init), ICC_UPDOWN_CLASS | ICC_TAB_CLASSES}; InitCommonControlsEx(&init);
    HANDLE raw = nullptr; SHORT count = -1;
    auto hr = manager->GetPropertySheetPages(&sheet, &raw, &count);
    std::printf("{\"operation\":\"native_page_create\",\"hresult\":\"0x%08lx\",\"count\":%d,\"handle\":%s}\n",hr,count,raw?"true":"false"); std::fflush(stdout);
    if(hr != S_OK || count != 1 || !raw) return false;
    HWND parent = CreateWindowExW(0,L"STATIC",L"Property probe",WS_POPUP,0,0,600,400,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HPROPSHEETPAGE pageHandle = reinterpret_cast<HPROPSHEETPAGE>(raw);
    PROPSHEETHEADERW header{}; header.dwSize=sizeof(header); header.dwFlags=PSH_MODELESS|PSH_NOAPPLYNOW|PSH_USECALLBACK;
    header.hwndParent=parent; header.pszCaption=L"Tempo probe"; header.nPages=1; header.phpage=&pageHandle; header.pfnCallback=sheet_callback;
    HWND window=reinterpret_cast<HWND>(PropertySheetW(&header));
    HWND page=IsWindow(window)?PropSheet_GetCurrentPageHwnd(window):nullptr;
    bool ok=parent && IsWindow(window) && IsWindow(page) && IsChild(parent,window) && !IsWindowVisible(window);
    std::printf("{\"operation\":\"native_page_window\",\"valid\":%s,\"hidden\":%s,\"child\":%s}\n",IsWindow(page)?"true":"false",!IsWindowVisible(window)?"true":"false",IsChild(parent,window)?"true":"false");std::fflush(stdout);
    DataObject object;
    if(ok) {
        auto observe=[&](const char* name) {
            std::printf("{\"operation\":\"native_page_state\",\"case\":\"%s\"}\n",name);
            for(int id : {223,224,225,233,232,229,230,234}) {
                HWND control=GetDlgItem(page,id); char text[256]{},className[64]{};
                GetWindowTextA(control,text,256);GetClassNameA(control,className,64);
                std::printf("{\"operation\":\"native_page_control\",\"id\":%d,\"class\":\"%s\",\"text\":\"%s\",\"enabled\":%s}\n",id,className,text,IsWindowEnabled(control)?"true":"false");
                ok = control && ok;
            }
            std::fflush(stdout);
        };
        observe("initial");
        hr=manager->SetObject(&object);ok=hr==S_OK&&ok;observe("single");
        object.data.flags=2; hr=manager->RefreshData();ok=hr==S_OK&&ok;observe("multiple");
        object.present=false; hr=manager->RefreshData();ok=hr==S_OK&&ok;observe("none");
        struct Edit {const char* name;int id;const char* text;bool origin=false;};
        for(const auto& edit : {
            Edit{"tempo_normal",223,"145.50"},Edit{"measure_normal",224,"4"},
            Edit{"beat_normal",225,"2"},Edit{"tick_negative",233,"-42"},
            Edit{"tempo_same",223,"137.25"},Edit{"tempo_zero",223,"0"},
            Edit{"tempo_high",223,"1001"},Edit{"tempo_negative",223,"-3"},
            Edit{"tempo_empty",223,""},Edit{"tempo_text",223,"abc"},
            Edit{"tempo_suffix",223,"42abc"},Edit{"tempo_space",223," 42 "},
            Edit{"measure_zero",224,"0"},Edit{"measure_high",224,"40000"},
            Edit{"measure_empty",224,""},Edit{"beat_high",225,"999"},
            Edit{"tick_high",233,"40000"},Edit{"tick_origin",233,"-42",true}}) {
            object.present=true;object.data=PageData{};object.sets=0;
            if(edit.origin){object.data.measure=0;object.data.beat=0;}
            hr=manager->RefreshData();ok=hr==S_OK&&ok;
            HWND control=GetDlgItem(page,edit.id);SetWindowTextA(control,edit.text);
            SendMessageW(page,WM_COMMAND,MAKEWPARAM(edit.id,EN_KILLFOCUS),reinterpret_cast<LPARAM>(control));
            std::printf("{\"operation\":\"native_page_edit\",\"case\":\"%s\",\"id\":%d,\"input\":\"%s\",\"sets\":%u,\"tempo\":%.17g,\"measure\":%ld,\"beat\":%ld,\"tick\":%ld}\n",edit.name,edit.id,edit.text,object.sets,object.data.tempo,object.data.measure,object.data.beat,object.data.tick);
            observe(edit.name);
        }
        struct Spin {int id;int position;int delta;};
        for(const auto& spin:{Spin{232,137,-1},Spin{232,137,1},Spin{229,3,1},Spin{230,4,-1},Spin{234,132,1}}) {
            object.data=PageData{};object.sets=0;
            hr=manager->RefreshData();ok=hr==S_OK&&ok;
            NMUPDOWN message{};message.hdr.hwndFrom=GetDlgItem(page,spin.id);
            message.hdr.idFrom=spin.id;message.hdr.code=UDN_DELTAPOS;
            message.iPos=spin.position;message.iDelta=spin.delta;
            auto result=SendMessageW(page,WM_NOTIFY,spin.id,reinterpret_cast<LPARAM>(&message));
            std::printf("{\"operation\":\"native_page_spin\",\"id\":%d,\"delta\":%d,\"result\":%lld,\"sets\":%u,\"tempo\":%.17g,\"measure\":%ld,\"beat\":%ld,\"tick\":%ld}\n",spin.id,spin.delta,static_cast<long long>(result),object.sets,object.data.tempo,object.data.measure,object.data.beat,object.data.tick);
        }
        const auto beforeCancel=object.data;const auto setsBeforeCancel=object.sets;
        PSHNOTIFY cancel{};cancel.hdr.hwndFrom=window;cancel.hdr.code=PSN_RESET;
        const auto cancelResult=SendMessageW(page,WM_NOTIFY,0,reinterpret_cast<LPARAM>(&cancel));
        const bool cancelUnchanged=std::memcmp(&beforeCancel,&object.data,0x22)==0&&setsBeforeCancel==object.sets;
        std::printf("{\"operation\":\"native_page_cancel\",\"result\":%lld,\"unchanged\":%s}\n",static_cast<long long>(cancelResult),cancelUnchanged?"true":"false");
        ok=cancelUnchanged&&ok;
        hr=manager->RemoveObject(&object);ok=hr==S_OK&&ok;
        if(ok && exercise)ok=exercise(page,manager,track,context)&&ok;
    }
    if(IsWindow(window))DestroyWindow(window);
    if(parent)DestroyWindow(parent);
    std::printf("{\"operation\":\"end_native_page_probe\",\"passed\":%s,\"object_refs\":%lu,\"removals\":%u}\n",ok?"true":"false",object.refs,object.removals);std::fflush(stdout);
    return ok;
}
#endif

inline bool run(IUnknown* manager, ReferenceTimeline& timeline
#ifdef PRODUCER_WINDOWED_PROBE
    , PageExercise exercise=nullptr, void* context=nullptr
#endif
) {
    bool ok = true;
    auto check = [&](const char* name, HRESULT hr, HRESULT expected) {
        std::printf("{\"operation\":\"page_%s\",\"hresult\":\"0x%08lx\"}\n", name, hr);
        std::fflush(stdout); ok = ok && hr == expected;
    };
    std::puts("{\"operation\":\"begin_property_page_probe\"}");
    producer::PropPageObject* object = nullptr;
    check("query_object", manager->QueryInterface(producer::IID_IDMUSProdPropPageObject,
        reinterpret_cast<void**>(&object)), S_OK);
    if (!object) return false;
    check("show_without_sheet", object->ShowProperties(), S_OK);
    Sheet sheet;
    auto framework = new fixture::Framework(&sheet);
    IUnknown* tl = nullptr;
    check("query_timeline", timeline.query(producer::IID_IDMUSProdTimeline, reinterpret_cast<void**>(&tl)), S_OK);
    if (!tl) { framework->Release(); object->Release(); return false; }
    VARIANT value{}; value.vt = VT_UNKNOWN; value.punkVal = framework;
    check("connect_framework", producer::timeline::set_property(tl, 3, value), S_OK);
    sheet.visibleResult = S_FALSE;
    check("show_hidden", object->ShowProperties(), S_OK);
    ok = ok && sheet.setCalls == 0 && sheet.visibleCalls == 1;
    sheet.visibleResult = S_OK;
    check("show_visible", object->ShowProperties(), S_OK);
    auto page = sheet.page;
    ok = ok && page && sheet.setCalls == 1;
    if (page) {
        IUnknown* identity = nullptr;
        check("query_identity", page->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&identity)), S_OK);
        ok = ok && identity == page;
        if (identity) identity->Release();
        IUnknown* unsupported = nullptr;
        check("unsupported_interface", page->QueryInterface(GUID_NULL, reinterpret_cast<void**>(&unsupported)), E_NOINTERFACE);
        ok = ok && !unsupported;
        BSTR title = nullptr; BOOL appendProperties = FALSE;
        check("title", page->GetPropertySheetTitle(&title, &appendProperties), S_OK);
        const bool tempoTitle = title && wcscmp(title, L"Tempo") == 0;
        std::printf("{\"operation\":\"page_title_value\",\"tempo\":%s,\"append_properties\":%ld}\n",
            tempoTitle ? "true" : "false", appendProperties);
        ok = ok && tempoTitle && appendProperties == TRUE;
        SysFreeString(title);
        check("title_null", page->GetPropertySheetTitle(nullptr, &appendProperties), E_POINTER);
        check("title_flag_null", page->GetPropertySheetTitle(&title, nullptr), E_POINTER);
        HANDLE handle = nullptr; SHORT count = -1;
        check("pages_null_output", page->GetPropertySheetPages(&sheet, nullptr, &count), E_POINTER);
        check("pages_null_count", page->GetPropertySheetPages(&sheet, &handle, nullptr), E_POINTER);
        check("pages_null_sheet", page->GetPropertySheetPages(nullptr, &handle, &count), E_INVALIDARG);
        check("equal_current", page->IsEqualObject(object), S_OK);
        check("equal_null", page->IsEqualObject(nullptr), E_INVALIDARG);
        check("set_null", page->SetObject(nullptr), E_INVALIDARG);
        check("refresh_without_page", page->RefreshData(), E_FAIL);
        check("show_again", object->ShowProperties(), S_OK);
        ok = ok && sheet.page == page && sheet.setCalls == 2;
        Object first, second;
        check("set_first", page->SetObject(&first), S_OK);
        check("equal_first", page->IsEqualObject(&first), S_OK);
        check("equal_old", page->IsEqualObject(object), S_FALSE);
        check("set_first_again", page->SetObject(&first), S_OK);
        check("set_second", page->SetObject(&second), S_OK);
        second.getResult = E_INVALIDARG;
        check("refresh_failed_get", page->RefreshData(), E_FAIL);
        check("remove_other", page->RemoveObject(&first), E_INVALIDARG);
        check("remove_null", page->RemoveObject(nullptr), E_INVALIDARG);
        check("remove_second", page->RemoveObject(&second), S_OK);
        check("remove_second_again", page->RemoveObject(&second), E_INVALIDARG);
        std::printf("{\"operation\":\"page_object_lifecycle\",\"first_refs\":%lu,\"second_refs\":%lu,\"first_gets\":%u,\"second_gets\":%u,\"first_removals\":%u,\"second_removals\":%u}\n",
            first.refs, second.refs, first.gets, second.gets, first.removals, second.removals);
        ok = ok && first.refs == 1 && second.refs == 1 && first.gets == 1 && second.gets == 2 && first.removals == 1 && second.removals == 1;
        check("show_restore", object->ShowProperties(), S_OK);
        sheet.setResult = E_FAIL;
        check("show_sheet_failure", object->ShowProperties(), S_OK);
        check("equal_after_failure", page->IsEqualObject(object), S_OK);
#ifdef PRODUCER_WINDOWED_PROBE
        ok = native_page(page,sheet,object,exercise,context) && ok;
#endif
        check("remove_sheet", sheet.clear(), S_OK);
    }
    value.punkVal = nullptr;
    check("disconnect_framework", producer::timeline::set_property(tl, 3, value), S_OK);
    tl->Release();
    const auto remaining = framework->Release();
    std::printf("{\"operation\":\"page_sheet_lifecycle\",\"framework_refs\":%lu,\"sheet_refs\":%lu,\"set_calls\":%u,\"visible_calls\":%u}\n",
        remaining, sheet.refs, sheet.setCalls, sheet.visibleCalls);
    unsigned expectedSetCalls=4,expectedVisibleCalls=5;
#ifdef PRODUCER_WINDOWED_PROBE
    if(exercise){expectedSetCalls+=6;expectedVisibleCalls+=6;}
#endif
    ok = ok && remaining == 0 && sheet.refs == 1 && sheet.setCalls == expectedSetCalls && sheet.visibleCalls == expectedVisibleCalls;
    check("object_remove_callback", object->OnRemoveFromPageManager(), E_NOTIMPL);
    object->Release();
    std::printf("{\"operation\":\"end_property_page_probe\",\"passed\":%s}\n", ok ? "true" : "false");
    return ok;
}
}
