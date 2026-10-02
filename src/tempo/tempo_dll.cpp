#include <windows.h>
#include <objidl.h>
#include <oleidl.h>
#include <new>
#include <cstring>
#include <oleauto.h>
#include <string>
#include <algorithm>
#include <cstdio>
#include <commctrl.h>
#include <prsht.h>
#include <cmath>
#include <cwctype>
#include "compat/producer_ids.h"
#include "compat/strip_manager.h"
#include "compat/prop_page_object.h"
#include "compat/prop_page_manager.h"
#include "compat/tempo_runtime.h"
#include "compat/host_services.h"
#include "compat/strip.h"
#include "compat/timeline_services.h"
#include "compat/tempo_notifications.h"
#include "tempo_track.h"
#include "tempo_data_object.h"

namespace {
LONG objectCount = 0;
LONG lockCount = 0;
constexpr DWORD supportedFlags = 0x11c58;
constexpr GUID undoLabelParam = {0x178633a6,0x4452,0x11d2,{0x89,0x0c,0,0xc0,0x4f,0xbf,0x8d,0x15}};
constexpr GUID alternateStreamFormat =
    {0x102125e0, 0x98b7, 0x11d1, {0x89, 0xaf, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0x29}};
struct TrackHeader { GUID classId; DWORD position; DWORD groupBits; DWORD chunkId; DWORD listType; };
static_assert(sizeof(TrackHeader) == 32);

template<class T> class ComOwner {
    T* pointer_ = nullptr;
public:
    ComOwner() = default;
    ComOwner(const ComOwner&) = delete;
    ComOwner& operator=(const ComOwner&) = delete;
    ~ComOwner() { reset(); }
    T* get() const { return pointer_; }
    T* operator->() const { return pointer_; }
    T** put() { reset(); return &pointer_; }
    void reset() { if (pointer_) { auto old = pointer_; pointer_ = nullptr; old->Release(); } }
};

// Page-manager behavior recovered from table RVA 0x19f8 and dialog105.
class TempoPageManager final : public producer::PropPageManager {
    LONG references_ = 1;
    producer::PropPageObject* object_ = nullptr; // Borrowed, RVA 0xf02d.
    ComOwner<IUnknown> sheet_;
    HWND window_ = nullptr;
    bool pageCreated_ = false, editing_ = false, single_ = false, multiple_ = false;
    struct Data { LONG time; double tempo; LONG measure, beat, tick; DWORD extra; WORD flags; } data_{};
    void notify_sheet() {
        if (!sheet_.get()) return;
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*);
        auto table = *reinterpret_cast<void***>(sheet_.get());
        reinterpret_cast<Call>(table[9])(sheet_.get());
        reinterpret_cast<Call>(table[10])(sheet_.get());
    }
    void number(int id, LONG value) {
        wchar_t text[32]{}; swprintf_s(text,L"%ld",value); SetDlgItemTextW(window_,id,text);
    }
    void tempo_text() {
        wchar_t text[64]{}; swprintf_s(text,L"%.2f",data_.tempo); SetDlgItemTextW(window_,223,text);
    }
    void paint_data() {
        if (!window_) return;
        const bool before = editing_; editing_ = true;
        for (int id : {223,224,225,233,232,229,230,234}) EnableWindow(GetDlgItem(window_,id),single_);
        if (single_) { tempo_text(); number(224,data_.measure+1);number(225,data_.beat+1);number(233,data_.tick); }
        else {
            SetDlgItemTextW(window_,223,multiple_?L"Multiple Tempos Selected":L"None");
            for(int id:{224,225,233})SetDlgItemTextW(window_,id,L"");
        }
        editing_ = before;
    }
    void submit() { if(object_)object_->SetData(&data_); }
    void edit(int id, bool final) {
        if(editing_ || !single_)return;
        wchar_t text[256]{};GetDlgItemTextW(window_,id,text,256);
        auto start=text;while(iswspace(*start))++start;
        auto tail=start+wcslen(start);while(tail>start && iswspace(tail[-1]))*--tail=0;
        const bool before=editing_;editing_=true;
        if(id==223) {
            wchar_t* end=nullptr;double value=wcstod(start,&end);
            if(end==start || !std::isfinite(value)) { if(final)tempo_text(); }
            else if(final || (value>=1 && value<=1000)) {
                const double clamped=(std::max)(1.0,(std::min)(1000.0,value));
                const bool changed=clamped!=data_.tempo;
                data_.tempo=clamped;
                if(final && (*end || clamped!=value))tempo_text();
                if(changed)submit();
            }
        } else if(final && (id==224 || id==225 || id==233)) {
            LONG* member=id==224?&data_.measure:id==225?&data_.beat:&data_.tick;
            LONG value=*member;
            if(*start) {
                value=wcstol(start,nullptr,10);
                if(id==224 || id==225)value=(std::max)(1L,(std::min)(id==224?32767L:256L,value))-1;
                else {value=(std::min)(32767L,value);if(value<0 && (data_.measure>0 || data_.beat>0))value=0;}
            }
            number(id,value+(id==233?0:1));
            if(value!=*member){*member=value;submit();}
        }
        editing_=before;
    }
    static UINT CALLBACK page_callback(HWND, UINT message, LPPROPSHEETPAGEW page) {
        auto self=reinterpret_cast<TempoPageManager*>(page->lParam);
        if(message==PSPCB_CREATE)self->AddRef();
        else if(message==PSPCB_RELEASE)self->Release();
        return 1;
    }
    static INT_PTR CALLBACK dialog_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto self=reinterpret_cast<TempoPageManager*>(GetWindowLongPtrW(window,GWLP_USERDATA));
        if(message==WM_INITDIALOG) {
            self=reinterpret_cast<TempoPageManager*>(reinterpret_cast<PROPSHEETPAGEW*>(lparam)->lParam);
            self->window_=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
            for(const auto pair:{std::pair{223,6},std::pair{224,5},std::pair{225,3},std::pair{233,5}})
                SendDlgItemMessageW(window,pair.first,EM_LIMITTEXT,pair.second,0);
            SendDlgItemMessageW(window,229,UDM_SETRANGE32,1,32767);
            SendDlgItemMessageW(window,230,UDM_SETRANGE32,1,256);
            SendDlgItemMessageW(window,234,UDM_SETRANGE32,static_cast<WPARAM>(-32768),32767);
            SendDlgItemMessageW(window,232,UDM_SETRANGE32,1,1000);
            self->paint_data();return TRUE;
        }
        if(!self)return FALSE;
        if(message==WM_COMMAND) {
            const auto id=LOWORD(wparam),code=HIWORD(wparam);
            if(code==EN_KILLFOCUS || (id==223 && code==EN_CHANGE)) {self->edit(id,code==EN_KILLFOCUS);return TRUE;}
        }
        if(message==WM_NOTIFY && lparam) {
            const auto spin=reinterpret_cast<NMUPDOWN*>(lparam);
            if(spin->hdr.code==UDN_DELTAPOS) {
                const auto id=spin->hdr.idFrom;
                if(id==232 || id==229 || id==230 || id==234) {
                    if(self->single_) {
                        const int editId=id==232?223:id==229?224:id==230?225:233;
                        self->edit(editId,true);
                        if(id==232) {
                            const double value=(std::max)(1.0,(std::min)(1000.0,std::floor(self->data_.tempo+spin->iDelta)));
                            if(value!=self->data_.tempo) {
                                self->data_.tempo=value;
                                const bool before=self->editing_;self->editing_=true;
                                self->tempo_text();self->editing_=before;
                                self->submit();
                            }
                        } else if(spin->iDelta) {
                            LONG& value=id==229?self->data_.measure:id==230?self->data_.beat:self->data_.tick;
                            value+=spin->iDelta;self->submit();
                        }
                    }
                    SetWindowLongPtrW(window,DWLP_MSGRESULT,id==232?0:1);
                    return TRUE;
                }
            }
        }
        if(message==WM_DESTROY)self->window_=nullptr;
        return FALSE;
    }
    void remove_object() {
        if (object_) { auto old = object_; object_ = nullptr; old->OnRemoveFromPageManager(); }
    }
public:
    TempoPageManager() { InterlockedIncrement(&objectCount); }
    ~TempoPageManager() { InterlockedDecrement(&objectCount); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != IID_IUnknown && iid != producer::IID_PropPageManager) return E_NOINTERFACE;
        *out = static_cast<producer::PropPageManager*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = InterlockedDecrement(&references_); if (!count) delete this; return count;
    }
    HRESULT STDMETHODCALLTYPE GetPropertySheetTitle(BSTR* title, BOOL* append) override {
        if (!title || !append) return E_POINTER;
        *append = TRUE; *title = SysAllocString(L"Tempo");
        return *title ? S_OK : E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE GetPropertySheetPages(IUnknown* sheet, HANDLE* pages, SHORT* count) override {
        if (!pages || !count) return E_POINTER;
        if (!sheet) return E_INVALIDARG;
        *pages = nullptr; *count = 0;
        sheet->AddRef(); sheet_.reset(); *sheet_.put()=sheet;
        INITCOMMONCONTROLSEX init{sizeof(init),ICC_UPDOWN_CLASS};InitCommonControlsEx(&init);
        HMODULE module = nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&dialog_proc),&module))return E_FAIL;
        PROPSHEETPAGEW page{};page.dwSize=sizeof(page);page.dwFlags=PSP_USECALLBACK;
        page.hInstance=module;page.pszTemplate=MAKEINTRESOURCEW(105);page.pfnDlgProc=dialog_proc;
        page.lParam=reinterpret_cast<LPARAM>(this);page.pfnCallback=page_callback;
        pageCreated_=true;
        auto handle=CreatePropertySheetPageW(&page);
        if(handle){*pages=reinterpret_cast<HANDLE>(handle);*count=1;}
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnRemoveFromPropertySheet() override { remove_object(); sheet_.reset(); return S_OK; }
    HRESULT STDMETHODCALLTYPE SetObject(producer::PropPageObject* object) override {
        if (!object) return E_INVALIDARG;
        if (object != object_) { remove_object(); object_ = object; RefreshData(); }
        notify_sheet();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RemoveObject(producer::PropPageObject* object) override {
        if (!object || object != object_) return E_INVALIDARG;
        remove_object(); RefreshData(); notify_sheet(); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE IsEqualObject(producer::PropPageObject* object) override {
        if (!object) return E_INVALIDARG;
        return object == object_ ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE RefreshData() override {
        void* data = nullptr;
        if (object_ && FAILED(object_->GetData(&data))) return E_FAIL;
        if(!pageCreated_)return E_FAIL;
        multiple_=data && (reinterpret_cast<Data*>(data)->flags & 2);
        single_=data && !multiple_;
        if(single_)std::memcpy(&data_,data,0x22);else data_={};
        paint_data();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE IsEqualPageManagerGUID(REFGUID) override {
        return E_NOTIMPL; // Constructor does not establish a known GUID; unverified.
    }
};

class TempoManager;
class TempoStrip final : public producer::Strip, public producer::TimelineEdit, public IDropSource, public IDropTarget {
    LONG references_ = 1;
    TempoManager* manager_; // Borrowed: manager owns the initial strip reference.
    LONG insertionPosition_ = -1; // Strip constructor RVA 0x8688, member +0x3c.
    LONG selectionStart_ = 0, selectionEnd_ = 0;
    bool gutterSelected_ = false;
    producer::tempo::Position selectionAnchor_{};
    bool collapseSelectionOnRelease_ = false;
    bool resettingMarkers_ = false;
    DWORD sourceKeys_ = 0, dropKeys_ = 0, dropEffect_ = 0;
    UINT dragFormat_ = 0;
    IDataObject* dropData_ = nullptr;
    bool dragPending_ = false;
    bool sourceDragging_ = false;
    LONG dragOriginX_ = 0;
    void reset_markers();
    HRESULT insertion_time(LONG& time);
    bool range_selected();
    HRESULT paste_data(IUnknown* data, LONG cursor, DWORD mode, bool clearSelection = true, bool synchronize = true, bool* modified = nullptr);
    HRESULT start_drag(DWORD keys, LONG x);
public:
    explicit TempoStrip(TempoManager* manager) : manager_(manager) { InterlockedIncrement(&objectCount); }
    ~TempoStrip() { if (dropData_) dropData_->Release(); InterlockedDecrement(&objectCount); }
    void detach() { manager_ = nullptr; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    ULONG STDMETHODCALLTYPE Release() override { const auto count = InterlockedDecrement(&references_); if (!count) delete this; return static_cast<ULONG>(count); }
    HRESULT STDMETHODCALLTYPE Draw(HDC, DWORD, LONG) override;
    HRESULT STDMETHODCALLTYPE GetStripProperty(DWORD, VARIANT*) override;
    HRESULT STDMETHODCALLTYPE SetStripProperty(DWORD, VARIANT) override;
    HRESULT STDMETHODCALLTYPE OnWMMessage(UINT, WPARAM, LPARAM, LONG, LONG) override;
    HRESULT STDMETHODCALLTYPE Cut(IUnknown*) override;
    HRESULT STDMETHODCALLTYPE Copy(IUnknown*) override;
    HRESULT STDMETHODCALLTYPE Paste(IUnknown*) override;
    HRESULT STDMETHODCALLTYPE Insert() override;
    HRESULT STDMETHODCALLTYPE Delete() override;
    HRESULT STDMETHODCALLTYPE SelectAll() override;
    HRESULT STDMETHODCALLTYPE CanCut() override;
    HRESULT STDMETHODCALLTYPE CanCopy() override;
    HRESULT STDMETHODCALLTYPE CanPaste(IUnknown*) override;
    HRESULT STDMETHODCALLTYPE CanInsert() override;
    HRESULT STDMETHODCALLTYPE CanDelete() override;
    HRESULT STDMETHODCALLTYPE CanSelectAll() override;
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escape, DWORD keys) override;
    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override { return DRAGDROP_S_USEDEFAULTCURSORS; }
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject*, DWORD, POINTL, DWORD*) override;
    HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD*) override;
    HRESULT STDMETHODCALLTYPE DragLeave() override;
    HRESULT STDMETHODCALLTYPE Drop(IDataObject*, DWORD, POINTL, DWORD*) override;
};

class TempoManager final : public producer::StripManager, public IPersistStream, public producer::PropPageObject {
    friend class TempoStrip;
    LONG references_ = 1;
    producer::tempo::Track track_;
    ComOwner<IUnknown> framework_;
    ComOwner<IUnknown> runtime_;
    ComOwner<IUnknown> timeline_;
    ComOwner<producer::PropPageManager> pageManager_;
    ComOwner<IDataObject> clipboard_;
    TempoStrip* strip_;
    struct PropertyData {
        LONG time; double tempo; LONG measure; LONG beat; LONG tick; DWORD unknown; WORD flags;
    } propertyData_{};
    static_assert(offsetof(PropertyData, tempo) == 8 && offsetof(PropertyData, flags) == 32);
    DWORD groupBits_ = 0xffffffff;
    DWORD flags_ = 0x18; // Original constructor RVA 0x7033.
    DWORD extras_ = 0;
    bool dirty_ = false;
    const wchar_t* undoLabel_ = L"";

    HRESULT connect_timeline(IUnknown* value) {
        if (timeline_.get()) {
            producer::timeline::remove_strip(timeline_.get(), static_cast<producer::Strip*>(strip_));
            for (size_t i = std::size(producer::tempo_notifications::registered); i > 0; --i)
                producer::timeline::remove_notification(timeline_.get(), static_cast<producer::StripManager*>(this),
                    producer::tempo_notifications::registered[i - 1], groupBits_);
            timeline_.reset();
        }
        if (!value) return S_OK;
        HRESULT hr = value->QueryInterface(producer::IID_IDMUSProdTimeline, reinterpret_cast<void**>(timeline_.put()));
        if (FAILED(hr)) return E_FAIL;
        hr = producer::timeline::insert_strip(timeline_.get(), static_cast<producer::Strip*>(strip_), producer::CLSID_DirectMusicTempoTrack, groupBits_);
        if (FAILED(hr)) timeline_.reset();
        else for (const auto& type : producer::tempo_notifications::registered)
            producer::timeline::add_notification(timeline_.get(), static_cast<producer::StripManager*>(this), type, groupBits_);
        return hr;
    }
    void invalidate() {
        if (timeline_.get()) producer::timeline::invalidate(timeline_.get(), static_cast<producer::Strip*>(strip_));
    }
    void changed() {
        synchronize();
        if (timeline_.get()) {
            producer::timeline::data_changed(timeline_.get(), static_cast<producer::StripManager*>(this));
            producer::timeline::notify(timeline_.get(), producer::GUID_TempoParam, 0xffffffff, nullptr);
        }
    }
    HRESULT describe_position(LONG time, PropertyData& result) {
        result.time = time;
        result.measure = result.beat = 0;
        result.tick = time;
        if (time <= 0) return S_OK;
        if (!timeline_.get()) return E_UNEXPECTED;
        HRESULT hr = producer::timeline::clocks_to_measure_beat(timeline_.get(), groupBits_, time, &result.measure, &result.beat);
        LONG base = 0;
        if (SUCCEEDED(hr)) hr = producer::timeline::measure_beat_to_clocks(timeline_.get(), groupBits_, result.measure, result.beat, &base);
        if (SUCCEEDED(hr)) result.tick = time - base;
        return hr;
    }
    static producer::tempo::Position cached_position(const PropertyData& data) {
        return {data.measure, data.beat, data.tick};
    }
    static LONG add_clocks(LONG a, LONG b) {
        const DWORD sum = static_cast<DWORD>(a) + static_cast<DWORD>(b);
        LONG result; std::memcpy(&result, &sum, sizeof(result)); return result;
    }
    void refresh_positions() {
        if (!timeline_.get()) return;
        for (size_t i = 0; i < track_.events().size(); ++i) {
            PropertyData position{};
            if (SUCCEEDED(describe_position(track_.events()[i].time, position))) track_.cache_position(i, cached_position(position));
        }
    }
    HRESULT clocks_for_position(producer::tempo::Position position, LONG& time) {
        LONG base = 0;
        const HRESULT hr = producer::timeline::measure_beat_to_clocks(timeline_.get(), groupBits_, position.measure, position.beat, &base);
        if (SUCCEEDED(hr)) time = add_clocks(base, position.tick);
        return hr;
    }
    HRESULT avoid_meter_collision(const std::vector<producer::tempo::Event*>& active, LONG& time) {
        // RVA 0x6764 examines stored musical positions of the remaining list.
        // An event in the same beat moves the candidate after that event's tick,
        // even when the proposed tick was later than the existing one.
        PropertyData target{};
        HRESULT hr = describe_position(time, target);
        if (FAILED(hr)) return hr;
        for (const auto* other : active) {
            if (other->position.measure > target.measure) break;
            LONG measureStart = 0;
            hr = producer::timeline::measure_beat_to_clocks(timeline_.get(), groupBits_, other->position.measure, 0, &measureStart);
            if (FAILED(hr)) return hr;
            producer::tempo_notifications::TimeSignature meter{};
            hr = producer::timeline::get_parameter(timeline_.get(), producer::tempo_notifications::timeSignature, groupBits_, measureStart, &meter);
            if (FAILED(hr)) return hr;
            if (other->position.beat > meter.beats) break;
            if (other->position.measure == target.measure && other->position.beat == target.beat) {
                target.tick = add_clocks(other->position.tick, 1);
                hr = clocks_for_position(cached_position(target), time);
                if (SUCCEEDED(hr)) hr = describe_position(time, target);
                if (FAILED(hr)) return hr;
            }
        }
        return clocks_for_position(cached_position(target), time);
    }
    HRESULT meter_changed(bool& changed) {
        if (!timeline_.get()) return E_UNEXPECTED;
        // Stable pointers allow original-order processing while each changed
        // event is removed and reinserted into the live time-ordered list.
        auto events = track_.events();
        std::vector<producer::tempo::Event*> active;
        active.reserve(events.size());
        for (auto& event : events) active.push_back(&event);
        for (auto& event : events) {
            if (event.time <= 0) continue;
            LONG time = 0;
            HRESULT hr = clocks_for_position(event.position, time);
            PropertyData position{};
            if (SUCCEEDED(hr)) hr = describe_position(time, position);
            if (FAILED(hr)) return hr;
            if (time == event.time && position.measure == event.position.measure &&
                position.beat == event.position.beat && position.tick == event.position.tick) continue;
            active.erase(std::find(active.begin(), active.end(), &event));
            // Shrinking a measure must not push an old beat into a later
            // measure. RVA 0x6a17 walks backward by beats until it fits.
            while (position.measure != event.position.measure) {
                position.beat = add_clocks(position.beat, -1);
                const LONG previous = time;
                hr = clocks_for_position(cached_position(position), time);
                if (SUCCEEDED(hr)) hr = describe_position(time, position);
                if (FAILED(hr)) return hr;
                if (time >= previous) return E_UNEXPECTED; // Invalid/non-progressing host conversion.
            }
            hr = avoid_meter_collision(active, time);
            if (SUCCEEDED(hr)) hr = describe_position(time, position);
            if (FAILED(hr)) return hr;
            event.time = time;
            event.position = cached_position(position);
            event.position.beat &= 0xff; // Original RVA 0x6b00 stores the low byte.
            changed = true;
            const auto insertion = std::upper_bound(active.begin(), active.end(), time,
                [](LONG at, const producer::tempo::Event* existing) { return at < existing->time; });
            active.insert(insertion, &event);
        }
        if (changed) {
            std::vector<producer::tempo::Event> ordered;
            ordered.reserve(active.size());
            for (const auto* event : active) ordered.push_back(*event);
            track_.replace_events(std::move(ordered));
        }
        return S_OK;
    }
    HRESULT position_from_input(const PropertyData& input, LONG& time) {
        if (!timeline_.get()) return E_UNEXPECTED;
        LONG base = 0;
        HRESULT hr = producer::timeline::measure_beat_to_clocks(timeline_.get(), groupBits_, input.measure, input.beat, &base);
        if (FAILED(hr)) return hr;
        // MUSIC_TIME addition wraps at 32 bits in the original x86 code.
        const DWORD sum = static_cast<DWORD>(base) + static_cast<DWORD>(input.tick);
        std::memcpy(&time, &sum, sizeof(time));
        if (time < 0) time = 0;
        PropertyData normalized{};
        hr = describe_position(time, normalized);
        if (FAILED(hr)) return hr;
        VARIANT length{};
        hr = producer::timeline::get_property(timeline_.get(), 1, &length);
        if (FAILED(hr)) return hr;
        if (length.vt != VT_I4) return E_UNEXPECTED;
        LONG endMeasure = 0;
        hr = producer::timeline::clocks_to_measure_beat(timeline_.get(), groupBits_, length.lVal, &endMeasure, nullptr);
        if (FAILED(hr)) return hr;
        const LONG lastMeasure = endMeasure > 0 ? endMeasure - 1 : 0;
        // The original bounds by the final allowed measure, not by clock alone.
        if (normalized.measure > lastMeasure) time = length.lVal - 1;
        return S_OK;
    }

    HRESULT synchronize() {
        if (!framework_.get() || !runtime_.get()) return E_UNEXPECTED;
        ComOwner<IStream> stream;
        HRESULT hr = producer::host::alloc_memory_stream(framework_.get(), 2, producer::GUID_TempoStreamFormat, stream.put());
        if (FAILED(hr)) return hr;
        if (!stream.get()) return E_UNEXPECTED;
        hr = Save(stream.get(), FALSE);
        LARGE_INTEGER zero{};
        if (SUCCEEDED(hr)) hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
        ComOwner<IPersistStream> persist;
        if (SUCCEEDED(hr)) hr = runtime_->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(persist.put()));
        if (SUCCEEDED(hr)) hr = persist->Load(stream.get());
        return hr;
    }
public:
    TempoManager() : strip_(new TempoStrip(this)) { InterlockedIncrement(&objectCount); }
    ~TempoManager() {
        // Keep the DLL live while releasing host objects, which can re-enter COM.
        if (pageManager_.get()) pageManager_->RemoveObject(static_cast<producer::PropPageObject*>(this));
        pageManager_.reset();
        connect_timeline(nullptr);
        strip_->detach(); strip_->Release();
        runtime_.reset(); framework_.reset();
        // Original RVA 0x714b: materialize only our current clipboard before
        // releasing the retained exported Timeline data object.
        if (clipboard_.get() && OleIsCurrentClipboard(clipboard_.get()) == S_OK) OleFlushClipboard();
        clipboard_.reset();
        InterlockedDecrement(&objectCount);
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == producer::IID_IDMUSProdStripMgr) *out = static_cast<producer::StripManager*>(this);
        else if (iid == IID_IPersist || iid == IID_IPersistStream) *out = static_cast<IPersistStream*>(this);
        else if (iid == producer::IID_IDMUSProdPropPageObject) *out = static_cast<producer::PropPageObject*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = InterlockedDecrement(&references_);
        if (!count) delete this;
        return static_cast<ULONG>(count);
    }
    HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID type) override {
        return type == producer::GUID_TempoParam || type == undoLabelParam ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE GetParam(REFGUID type, LONG at, LONG* next, void* data) override {
        if (type == undoLabelParam) {
            if (!data) return E_POINTER;
            auto out = static_cast<BSTR*>(data);
            *out = SysAllocString(undoLabel_);
            return *out ? S_OK : E_OUTOFMEMORY;
        }
        if (type != producer::GUID_TempoParam) return E_INVALIDARG;
        if (!data) return E_POINTER;
        const auto value = track_.query(at);
        auto out = static_cast<producer::TempoParam*>(data);
        out->time = value.eventTime;
        out->tempo = value.bpm;
        if (next) *next = value.next;
        return value.found ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE SetParam(REFGUID, LONG, void*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnUpdate(REFGUID type, DWORD, void* data) override {
        try {
            if (type == producer::tempo_notifications::changeTempoAtCursor) {
                if (!data) return E_POINTER;
                if (!timeline_.get()) return S_OK;
                LONG cursor = 0;
                if (FAILED(producer::timeline::get_marker(timeline_.get(), 0, &cursor))) return S_OK;
                double tempo = 0;
                std::memcpy(&tempo, static_cast<const unsigned char*>(data) + 8, sizeof(tempo));
                if (!track_.change_tempo_at(cursor, tempo)) return S_OK;
                // RVA 0x74ec updates the event active at the cursor without
                // changing selection. Playback-message and property-page
                // refresh branches require services not yet implemented.
                undoLabel_ = L"Change Tempo";
                invalidate();
                VARIANT property{};
                const bool restore = SUCCEEDED(producer::timeline::get_property(timeline_.get(), 12, &property)) && property.boolVal;
                if (restore) {
                    VARIANT disabled{}; disabled.vt = VT_BOOL;
                    producer::timeline::set_property(timeline_.get(), 12, disabled);
                }
                changed();
                if (restore) {
                    VARIANT enabled{}; enabled.vt = VT_BOOL; enabled.boolVal = 1;
                    producer::timeline::set_property(timeline_.get(), 12, enabled);
                }
                return S_OK;
            }
            if (type == producer::tempo_notifications::refreshPositions) {
                refresh_positions(); invalidate(); return S_OK;
            }
            if (type == producer::tempo_notifications::timeSignature) {
                bool moved = false;
                const HRESULT hr = meter_changed(moved);
                if (FAILED(hr)) return hr;
                if (moved) changed();
                invalidate(); return S_OK;
            }
            // Other registered notification handlers remain to be recovered.
            return E_FAIL;
        } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }
    HRESULT STDMETHODCALLTYPE GetStripMgrProperty(DWORD property, VARIANT* value) override {
        if (!value) return E_POINTER;
        if (property <= 2) {
            value->vt = VT_UNKNOWN; value->punkVal = nullptr;
            IUnknown* stored = property == 1 ? runtime_.get() : property == 2 ? framework_.get() : timeline_.get();
            if (property == 0 && stored) { value->punkVal = stored; stored->AddRef(); return S_OK; }
            return stored ? stored->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&value->punkVal)) : E_FAIL;
        }
        if (property == 6) { value->vt = VT_I4; value->lVal = supportedFlags; return S_OK; }
        if (property > 5 || value->vt != VT_BYREF) return E_INVALIDARG;
        if (!value->byref) return E_POINTER;
        if (property == 3) {
            *static_cast<TrackHeader*>(value->byref) = {producer::CLSID_DirectMusicTempoTrack, 0, groupBits_, 0x72746574, 0};
        } else *static_cast<DWORD*>(value->byref) = property == 4 ? flags_ : extras_;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetStripMgrProperty(DWORD property, VARIANT value) override {
        if (property <= 2) {
            if (value.vt != VT_UNKNOWN) return E_INVALIDARG;
            if (property == 0) return connect_timeline(value.punkVal);
            auto& stored = property == 1 ? runtime_ : framework_;
            const GUID& iid = property == 1 ? producer::IID_DirectMusicTrack : producer::IID_IDMUSProdFramework;
            // QI before releasing permits setting the same pointer without a
            // dangling reference. Original ignores a failed QI in this setter.
            IUnknown* incoming = nullptr;
            if (value.punkVal) value.punkVal->QueryInterface(iid, reinterpret_cast<void**>(&incoming));
            *stored.put() = incoming;
            return S_OK;
        }
        if (property > 5 || value.vt != VT_BYREF) return E_INVALIDARG;
        if (!value.byref) return E_POINTER;
        // Original property3 validates but does not copy the supplied header.
        if (property == 4) flags_ = *static_cast<DWORD*>(value.byref) & supportedFlags;
        if (property == 5) extras_ = *static_cast<DWORD*>(value.byref);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID* out) override {
        if (!out) return E_POINTER;
        *out = producer::CLSID_TempoMgr; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE IsDirty() override { return dirty_ ? S_OK : S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Save(IStream* stream, BOOL clearDirty) override {
        if (!stream) return E_INVALIDARG;
        try {
            ComOwner<IUnknown> metadata;
            if (SUCCEEDED(stream->QueryInterface(producer::IID_IDMUSProdPersistInfo, reinterpret_cast<void**>(metadata.put())))) {
                producer::host::StreamInfo info{};
                const HRESULT hr = producer::host::get_stream_info(metadata.get(), &info);
                if (FAILED(hr)) return hr;
                if (info.format != producer::GUID_TempoStreamFormat && info.format != alternateStreamFormat) return E_INVALIDARG;
            }
            const auto bytes = track_.save();
            ULONG written = 0;
            const HRESULT hr = stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written);
            if (FAILED(hr) || written != bytes.size()) return E_FAIL;
            if (clearDirty) dirty_ = false;
            return S_OK;
        } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }
    HRESULT STDMETHODCALLTYPE Load(IStream* stream) override {
        if (!stream) return E_INVALIDARG;
        // The original crashes without these host connections. Report a
        // failure instead; no compatibility claim is made for invalid setup.
        if (!framework_.get() || !runtime_.get()) return E_UNEXPECTED;
        try {
            std::vector<std::uint8_t> bytes(12);
            ULONG read = 0;
            HRESULT hr = stream->Read(bytes.data(), 12, &read);
            if (FAILED(hr) || read != 12) return E_FAIL;
            if (std::memcmp(bytes.data(), "tetr", 4)) return E_NOTIMPL;
            DWORD payload = 0; std::memcpy(&payload, bytes.data() + 4, 4);
            // Provisional resource limit; not an observed reference limit.
            if (payload < 4 || payload > 64 * 1024 * 1024) return E_FAIL;
            bytes.resize(static_cast<size_t>(payload) + 8);
            hr = stream->Read(bytes.data() + 12, payload - 4, &read);
            if (FAILED(hr) || read != payload - 4) return E_FAIL;
            const auto loaded = track_.load(bytes);
            if (loaded == producer::tempo::LoadResult::unsupported) return E_NOTIMPL;
            if (loaded != producer::tempo::LoadResult::ok) return E_FAIL;
            refresh_positions();
            // Original Load (0x7436) does not propagate synchronization HRESULT.
            synchronize();
            return S_OK;
        } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }
    HRESULT STDMETHODCALLTYPE GetData(void** out) override {
        if (!out) return E_INVALIDARG;
        *out = nullptr;
        const auto event = track_.first_selected();
        if (!event) return S_OK;
        propertyData_ = {};
        propertyData_.time = event->time;
        propertyData_.measure = event->position.measure;
        propertyData_.beat = event->position.beat;
        propertyData_.tick = event->position.tick;
        propertyData_.tempo = event->bpm;
        propertyData_.flags = track_.selected_count() > 1 ? 2 : 0;
        *out = &propertyData_;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetData(void* data) override {
        if (!data) return E_INVALIDARG;
        // SetData uses 0x64d2 (first selected, including empty placeholders),
        // unlike GetData's 0x6136 scan which skips placeholders.
        const auto selected = track_.first_selected(true);
        if (!selected) return S_FALSE;
        // Read only the recovered prefix; callers need not allocate tail padding.
        PropertyData input{}; std::memcpy(&input, data, 0x22);
        const auto current = selected->position;
        if (input.measure == current.measure && input.beat == current.beat && input.tick == current.tick) {
            if (input.tempo == selected->bpm) return S_OK;
            if (!track_.change_selected_tempo(input.tempo)) return E_INVALIDARG;
            undoLabel_ = L"Change Tempo";
        } else {
            LONG time = 0;
            HRESULT hr = position_from_input(input, time);
            if (FAILED(hr)) return hr;
            PropertyData position{};
            hr = describe_position(time, position);
            if (FAILED(hr)) return hr;
            // Position changes take precedence over a simultaneous tempo change.
            track_.move_first_selected(time, cached_position(position));
            undoLabel_ = L"Move Tempo(s)";
        }
        invalidate(); changed();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ShowProperties() override {
        if (!timeline_.get()) return E_FAIL; // Original dereferences a missing Timeline.
        if (!pageManager_.get()) {
            auto page = new (std::nothrow) TempoPageManager;
            if (!page) return E_OUTOFMEMORY;
            *pageManager_.put() = page;
        }
        using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, IUnknown*, IUnknown*);
        const auto call = reinterpret_cast<Call>((*reinterpret_cast<void***>(timeline_.get()))[18]);
        call(timeline_.get(), pageManager_.get(), static_cast<producer::PropPageObject*>(this));
        // RVA 0x6294 deliberately does not propagate the Timeline result.
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() override { return E_NOTIMPL; }
};

HRESULT TempoStrip::QueryInterface(REFIID iid, void** out) {
    if (!out) return E_INVALIDARG;
    *out = nullptr;
    if (iid == IID_IUnknown || iid == producer::IID_Strip) *out = static_cast<producer::Strip*>(this);
    else if (iid == producer::IID_TimelineEdit) *out = static_cast<producer::TimelineEdit*>(this);
    else if (iid == IID_IDropSource) *out = static_cast<IDropSource*>(this);
    else if (iid == IID_IDropTarget) *out = static_cast<IDropTarget*>(this);
    else return E_NOTIMPL; // Observed original strip's unsupported-QI result.
    AddRef(); return S_OK;
}
HRESULT TempoStrip::GetStripProperty(DWORD property, VARIANT* out) {
    if (!out) return E_POINTER;
    switch (property) {
    case 0:
        // The currently implemented manager keeps its initial all-groups mask.
        out->vt = VT_BSTR; out->bstrVal = SysAllocString(L"1-32: Tempo");
        return out->bstrVal ? S_OK : E_OUTOFMEMORY;
    case 1: out->vt = VT_BOOL; out->boolVal = 1; return S_OK;
    case 6: case 8: case 9: out->vt = VT_INT; out->intVal = 20; return S_OK;
    case 7: case 10: out->vt = VT_BOOL; out->boolVal = 0; return S_OK;
    case 12:
        out->vt = VT_UNKNOWN; out->punkVal = nullptr;
        return manager_ ? manager_->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&out->punkVal)) : S_OK;
    default: return E_FAIL;
    }
}
HRESULT TempoStrip::SelectAll() {
    if (!manager_) return E_UNEXPECTED;
    manager_->track_.select_all(); manager_->invalidate(); return S_OK;
}
HRESULT TempoStrip::Draw(HDC dc, DWORD, LONG offset) {
    if (!manager_ || !manager_->timeline_.get()) return S_OK;
    if (!dc) return E_INVALIDARG;
    try {
        auto timeline = manager_->timeline_.get();
        producer::timeline::draw_music_lines(timeline, dc, manager_->groupBits_, offset);
        RECT clip{}; GetClipBox(dc, &clip);
        const bool range = gutterSelected_ && selectionStart_ != selectionEnd_;
        struct Group {
            LONG measure, beat, position;
            const producer::tempo::Event* first;
            const producer::tempo::Event* last;
            const producer::tempo::Event* selected;
            size_t count;
        };
        std::vector<Group> groups;
        for (const auto& event : manager_->track_.events()) {
            if (groups.empty() || groups.back().measure != event.position.measure || groups.back().beat != event.position.beat) {
                LONG position = 0;
                const HRESULT hr = producer::timeline::measure_beat_to_position(timeline, manager_->groupBits_,
                    event.position.measure, event.position.beat, &position);
                if (FAILED(hr)) return hr;
                groups.push_back({event.position.measure,event.position.beat,position,&event,&event,event.selected?&event:nullptr,event.bpm!=0?1u:0u});
            } else {
                auto& group = groups.back();
                if(event.bpm!=0) { if(group.first->bpm==0)group.first=&event;group.last=&event;++group.count; }
                if (event.selected && (!group.selected || (group.selected->bpm==0 && event.bpm!=0))) group.selected = &event;
            }
        }
        // The original derives an italic font from the caller's selected font.
        struct ItalicFont {
            HFONT handle = nullptr;
            ~ItalicFont() { if (handle) DeleteObject(handle); }
        } italic;
        LOGFONTA font{};
        const auto baseFont = GetCurrentObject(dc,OBJ_FONT);
        if (baseFont && GetObjectA(baseFont,sizeof(font),&font) > 0) {
            font.lfItalic = TRUE; italic.handle = CreateFontIndirectA(&font);
        }
        const auto label = [](double tempo) {
            if (tempo == 0) return std::string{};
            char text[384]{}; std::snprintf(text,sizeof(text),"%.2f",tempo); return std::string(text);
        };
        const auto text_width = [&](const std::string& text) {
            SIZE size{}; GetTextExtentPoint32A(dc,text.c_str(),static_cast<int>(text.size()),&size); return size.cx;
        };
        const auto beat_width = [&](LONG measure) {
            LONG start = 0, end = 0;
            producer::timeline::measure_beat_to_position(timeline,manager_->groupBits_,measure,0,&start);
            producer::timeline::measure_beat_to_position(timeline,manager_->groupBits_,measure,1,&end);
            return end-start;
        };
        const auto draw_label = [&](const Group& group, const producer::tempo::Event& event, RECT bounds, bool invert, bool gray) {
            const auto text = label(event.bpm);
            const auto previous = group.count > 1 && italic.handle ? SelectObject(dc,italic.handle) : nullptr;
            const COLORREF oldColor = gray ? SetTextColor(dc,RGB(168,168,168)) : 0;
            DrawTextA(dc,text.c_str(),static_cast<int>(text.size()),&bounds,DT_NOPREFIX);
            if (invert) InvertRect(dc,&bounds);
            if (gray) SetTextColor(dc,oldColor);
            if (previous) SelectObject(dc,previous);
        };
        LONG visible = 0, floorMeasure = 0, floorBeat = 0, start = 0;
        producer::timeline::get_marker(timeline,3,&visible);
        producer::timeline::clocks_to_measure_beat(timeline,manager_->groupBits_,visible,&floorMeasure,&floorBeat);
        producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,floorMeasure,floorBeat,&start);
        LONG ceilMeasure = floorMeasure, ceilBeat = floorBeat;
        if (start < visible) {
            producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,floorMeasure,floorBeat+1,&start);
            producer::timeline::clocks_to_measure_beat(timeline,manager_->groupBits_,start,&ceilMeasure,&ceilBeat);
        }
        const Group* ghost = nullptr;
        for (const auto& group : groups) {
            if (group.measure < ceilMeasure || (group.measure == ceilMeasure && group.beat < ceilBeat)) {
                if (group.last->bpm != 0) ghost = &group;
            }
            else break;
        }
        RECT ghostBounds{0,0,0,20}; bool hideGhost = false;
        LONG visibleX = 0;
        producer::timeline::clocks_to_position(timeline,visible,&visibleX);
        if (ghost) {
            LONG ghostX = 0;
            producer::timeline::measure_beat_to_position(timeline,manager_->groupBits_,ceilMeasure,ceilBeat,&ghostX);
            ghostBounds.left = ghostX + 1 - offset;
            ghostBounds.right = ghostBounds.left + text_width(label(ghost->last->bpm));
        }
        // Normal events start one pixel after the beat line. Selection uses the
        // beat position itself in a separate text-and-inversion pass.
        for (size_t i = 0; i < groups.size(); ++i) {
            const auto& group = groups[i];
            if (group.position + 1 - offset > clip.right) break;
            const auto* event = group.selected ? group.selected : group.first;
            if (event->bpm == 0) continue;
            const auto text = label(event->bpm);
            const auto previous = group.count > 1 && italic.handle ? SelectObject(dc,italic.handle) : nullptr;
            LONG right = group.position + 1 + (std::max)(text_width(text),beat_width(group.measure));
            if (previous) SelectObject(dc,previous);
            if (right-offset < clip.left) continue;
            for (size_t j=i+1;j<groups.size();++j) if(groups[j].first->bpm != 0) { right=(std::min)(right,groups[j].position);break; }
            RECT bounds{group.position+1-offset,0,right-offset,20};
            if (ghost) {
                if ((group.measure == ceilMeasure && group.beat == ceilBeat) ||
                    (group.measure == floorMeasure && bounds.right > visibleX-1-offset)) hideGhost = true;
                else if (bounds.left > ghostBounds.left && bounds.left < ghostBounds.right) ghostBounds.right = bounds.left;
            }
            if (!event->selected || range) draw_label(group,*event,bounds,false,false);
        }
        if (ghost && !hideGhost) draw_label(*ghost,*ghost->last,ghostBounds,false,true);
        if (!range) for (size_t i = 0; i < groups.size(); ++i) {
            const auto& group = groups[i];
            if (group.position-offset > clip.right) break;
            if (!group.selected) continue;
            const auto text = label(group.selected->bpm);
            const auto previous = group.count > 1 && italic.handle ? SelectObject(dc,italic.handle) : nullptr;
            LONG right = group.position + (std::max)(text_width(text),beat_width(group.measure));
            if (previous) SelectObject(dc,previous);
            if (right-offset < clip.left) continue;
            for (size_t j = i+1; j < groups.size(); ++j) if (groups[j].selected) { right = (std::min)(right,groups[j].position); break; }
            draw_label(group,*group.selected,{group.position-offset,0,right-offset,20},true,false);
        }
        if (range) {
            LONG measure = 0, beat = 0, clock = 0, left = 0, right = 0;
            producer::timeline::clocks_to_measure_beat(timeline,manager_->groupBits_,(std::min)(selectionStart_,selectionEnd_),&measure,&beat);
            producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,measure,beat,&clock);
            producer::timeline::clocks_to_position(timeline,clock,&left);
            producer::timeline::clocks_to_measure_beat(timeline,manager_->groupBits_,(std::max)(selectionStart_,selectionEnd_),&measure,&beat);
            producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,measure,beat+1,&clock);
            producer::timeline::clocks_to_position(timeline,clock-1,&right);
            RECT bounds{left-offset,0,right-offset,20}; InvertRect(dc,&bounds);
        }
        return S_OK;
    } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
HRESULT TempoStrip::SetStripProperty(DWORD property, VARIANT value) {
    if (property != 2 && property != 3 && property != 4) return E_FAIL;
    if (property != 2 && value.vt != VT_I4) return E_FAIL;
    if (!manager_ || !manager_->timeline_.get()) return E_UNEXPECTED;
    if (property == 2) gutterSelected_ = value.boolVal != 0; // Original does not check vt here.
    else if (property == 3) selectionStart_ = value.lVal;
    else selectionEnd_ = value.lVal;
    // RVA 0x89d2: marker callbacks still store the boundary, but cannot clear
    // the mouse selection while ResetSelectionMarkers (0x85a3) is active.
    if (property != 2 && resettingMarkers_) return S_OK;
    manager_->track_.clear_selection();
    if (gutterSelected_ && selectionStart_ != selectionEnd_) {
        LONG startMeasure = 0, startBeat = 0, endMeasure = 0, endBeat = 0;
        auto timeline = manager_->timeline_.get();
        const LONG startTime = selectionStart_ == -1 ? 0 : selectionStart_;
        HRESULT hr = producer::timeline::clocks_to_measure_beat(timeline, manager_->groupBits_, startTime, &startMeasure, &startBeat);
        if (SUCCEEDED(hr)) hr = producer::timeline::clocks_to_measure_beat(timeline, manager_->groupBits_, selectionEnd_, &endMeasure, &endBeat);
        if (FAILED(hr)) return hr;
        manager_->track_.select_beats({startMeasure, startBeat, 0}, {endMeasure, endBeat, 0});
    }
    manager_->invalidate();
    if (manager_->pageManager_.get()) manager_->pageManager_->RefreshData();
    return S_OK;
}
void TempoStrip::reset_markers() {
    if (!manager_ || !manager_->timeline_.get()) return;
    struct Restore { bool& value; bool previous; ~Restore(){value=previous;} } restore{resettingMarkers_,resettingMarkers_};
    resettingMarkers_ = true;
    producer::timeline::set_marker(manager_->timeline_.get(), 1, 0);
    producer::timeline::set_marker(manager_->timeline_.get(), 2, 0);
}
bool TempoStrip::range_selected() {
    if (!manager_ || !manager_->timeline_.get()) return false;
    VARIANT gutter{}; LONG start = -1, end = -1;
    auto timeline = manager_->timeline_.get();
    return SUCCEEDED(producer::timeline::get_strip_property(timeline, static_cast<producer::Strip*>(this), 7, &gutter)) &&
        gutter.boolVal == 1 && SUCCEEDED(producer::timeline::get_marker(timeline, 1, &start)) && start >= 0 &&
        SUCCEEDED(producer::timeline::get_marker(timeline, 2, &end)) && end > start;
}
HRESULT TempoStrip::CanCopy() {
    if (!manager_) return E_UNEXPECTED;
    return range_selected() || manager_->track_.selected_count() ? S_OK : S_FALSE;
}
HRESULT TempoStrip::CanCut() {
    if (!manager_) return E_UNEXPECTED;
    return range_selected() || (CanCopy() == S_OK && CanDelete() == S_OK) ? S_OK : S_FALSE;
}
HRESULT TempoStrip::CanPaste(IUnknown* data) {
    if (!manager_) return E_UNEXPECTED;
    const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
    if (!format) return E_FAIL;
    ComOwner<IDataObject> clipboard;
    ComOwner<IUnknown> imported;
    if (!data) {
        if (!manager_->timeline_.get() || FAILED(OleGetClipboard(clipboard.put())) || !clipboard.get()) return S_FALSE;
        if (FAILED(producer::timeline::create_data_object(manager_->timeline_.get(), imported.put())) || !imported.get()) return S_FALSE;
        if (FAILED(producer::timeline_data::import_data(imported.get(), clipboard.get()))) return S_FALSE;
        data = imported.get();
    }
    return producer::timeline_data::format_available(data, format) == S_OK ? S_OK : S_FALSE;
}
HRESULT TempoStrip::Copy(IUnknown* data) {
    if (CanCopy() != S_OK || !manager_) return E_UNEXPECTED;
    if (!manager_->timeline_.get()) return E_UNEXPECTED;
    try {
        const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
        if (!format) return E_FAIL;
        LONG start = 0;
        LONG boundaryStart = -1, boundaryEnd = -1;
        if (data) {
            if (FAILED(producer::timeline_data::get_boundaries(data, &start, nullptr))) return E_UNEXPECTED;
        } else {
            // Original RVAs 0x64d2/0x6b2c: origin is the first selected item,
            // boundaries are raw first/last event times, not snapped clocks.
            bool found = false;
            for (const auto& event : manager_->track_.events()) if (event.selected) {
                if (!found) { start = event.time; boundaryStart = event.time; found = true; }
                boundaryEnd = (std::max)(boundaryEnd, static_cast<LONG>(event.time));
            }
            if (!found) return E_UNEXPECTED;
            if (boundaryEnd < 0) boundaryStart = boundaryEnd = -1;
        }
        LONG measure = 0, beat = 0;
        auto timeline = manager_->timeline_.get();
        HRESULT hr = producer::timeline::clocks_to_measure_beat(timeline, manager_->groupBits_, start, &measure, &beat);
        if (SUCCEEDED(hr)) hr = producer::timeline::measure_beat_to_clocks(timeline, manager_->groupBits_, measure, beat, &start);
        if (FAILED(hr)) return E_UNEXPECTED;
        const auto bytes = manager_->track_.copy_selected(start);
        ComOwner<IStream> stream;
        hr = CreateStreamOnHGlobal(nullptr, TRUE, stream.put());
        if (FAILED(hr)) return E_OUTOFMEMORY;
        ULONG written = 0;
        hr = stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written);
        if (FAILED(hr) || written != bytes.size()) return E_UNEXPECTED;
        // The original passes the stream at its final position; the data object
        // owns a reference and its GetInternalClipFormat clones/rewinds it.
        if (data) return producer::timeline_data::add_format(data, format, stream.get()) == S_OK ? S_OK : E_FAIL;
        ComOwner<IUnknown> exportedTimeline;
        hr = producer::timeline::create_data_object(timeline, exportedTimeline.put());
        if (hr != S_OK || !exportedTimeline.get()) return E_FAIL;
        producer::timeline_data::set_boundaries(exportedTimeline.get(), boundaryStart, boundaryEnd);
        if (producer::timeline_data::add_format(exportedTimeline.get(), format, stream.get()) != S_OK) return E_FAIL;
        ComOwner<IDataObject> exported;
        hr = producer::timeline_data::export_data(exportedTimeline.get(), exported.put());
        if (FAILED(hr) || !exported.get()) return E_UNEXPECTED;
        if (OleSetClipboard(exported.get()) != S_OK) return E_FAIL;
        exported->AddRef();
        *manager_->clipboard_.put() = exported.get();
        return S_OK;
    } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
HRESULT TempoStrip::Cut(IUnknown* data) {
    if (CanCut() != S_OK) return E_UNEXPECTED;
    const HRESULT hr = Copy(data);
    return SUCCEEDED(hr) ? Delete() : hr;
}
HRESULT TempoStrip::Paste(IUnknown* data) {
    if (CanPaste(data) != S_OK || !manager_ || !manager_->timeline_.get()) return E_UNEXPECTED;
    ComOwner<IDataObject> clipboard;
    ComOwner<IUnknown> imported;
    if (!data) {
        if (FAILED(OleGetClipboard(clipboard.put())) || !clipboard.get()) return E_FAIL;
        if (FAILED(producer::timeline::create_data_object(manager_->timeline_.get(), imported.put())) || !imported.get()) return E_FAIL;
        if (FAILED(producer::timeline_data::import_data(imported.get(), clipboard.get()))) return E_FAIL;
        data = imported.get();
    }
    DWORD mode = 0; LONG cursor = 0;
    HRESULT hr = producer::timeline::get_paste_mode(manager_->timeline_.get(), &mode);
    if (SUCCEEDED(hr)) hr = producer::timeline::get_marker(manager_->timeline_.get(), 0, &cursor);
    return FAILED(hr) ? E_FAIL : paste_data(data, cursor, mode);
}
HRESULT TempoStrip::paste_data(IUnknown* data, LONG cursor, DWORD mode, bool clearSelection, bool synchronize, bool* modified) {
    if (modified) *modified = false;
    try {
        auto timeline = manager_->timeline_.get();
        HRESULT hr = S_OK;
        const auto snap = [&](LONG time, LONG& snapped) {
            LONG measure = 0, beat = 0;
            HRESULT converted = producer::timeline::clocks_to_measure_beat(timeline, manager_->groupBits_, time, &measure, &beat);
            if (SUCCEEDED(converted)) converted = producer::timeline::measure_beat_to_clocks(timeline, manager_->groupBits_, measure, beat, &snapped);
            return converted;
        };
        LONG origin = 0;
        if (SUCCEEDED(hr)) hr = snap(cursor, origin);
        if (FAILED(hr)) return E_FAIL;
        LONG selectionStart = 0, selectionEnd = 0;
        const bool overwrite = mode == 1 && SUCCEEDED(producer::timeline_data::get_boundaries(data, &selectionStart, &selectionEnd));
        ComOwner<IStream> stream;
        const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
        hr = producer::timeline_data::get_stream(data, format, stream.put());
        if (FAILED(hr) || !stream.get()) return E_FAIL;
        STATSTG stat{};
        hr = stream->Stat(&stat, STATFLAG_NONAME);
        if (FAILED(hr) || stat.cbSize.HighPart || stat.cbSize.LowPart > 64 * 1024 * 1024) return E_FAIL;
        std::vector<std::uint8_t> bytes(stat.cbSize.LowPart);
        ULONG read = 0;
        hr = stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read);
        if (FAILED(hr) || read != bytes.size()) return E_FAIL;
        std::vector<producer::tempo::Event> incoming;
        const auto decoded = producer::tempo::decode_copy(bytes, incoming);
        if (decoded != producer::tempo::LoadResult::ok)
            return decoded == producer::tempo::LoadResult::unsupported ? E_NOTIMPL : E_FAIL;
        if (!incoming.empty()) {
            const auto range = std::minmax_element(incoming.begin(), incoming.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
            VARIANT length{};
            if (SUCCEEDED(producer::timeline::get_property(timeline, 1, &length)) && length.vt == VT_I4 &&
                static_cast<std::int64_t>(origin) + range.second->time >= length.lVal) {
                const LONG adjusted = TempoManager::add_clocks(TempoManager::add_clocks(length.lVal, -1),
                    static_cast<LONG>(-static_cast<std::int64_t>(range.second->time)));
                origin = adjusted;
                snap(adjusted, origin);
            }
            if (static_cast<std::int64_t>(origin) + range.first->time < 0)
                origin = TempoManager::add_clocks(range.first->position.tick, static_cast<LONG>(-static_cast<std::int64_t>(range.first->time)));
            for (auto& event : incoming) {
                event.time = TempoManager::add_clocks(event.time, origin);
                TempoManager::PropertyData position{};
                hr = manager_->describe_position(event.time, position);
                if (FAILED(hr)) return hr;
                event.position = TempoManager::cached_position(position);
                event.selected = true;
            }
        }
        // Invalid input is rejected before changing the model; the original's
        // partial updates on malformed streams remain unverified.
        bool changed = false;
        if (overwrite) {
            const LONG duration = TempoManager::add_clocks(selectionEnd, static_cast<LONG>(-static_cast<std::int64_t>(selectionStart)));
            changed = manager_->track_.erase_range(cursor, TempoManager::add_clocks(cursor, duration));
        }
        // Paste's entry point deselects old items; Drop calls PasteAt directly.
        if (clearSelection) manager_->track_.clear_selection();
        for (const auto& event : incoming) { manager_->track_.replace_event(event); changed = true; }
        if (changed) {
            manager_->undoLabel_ = L"Paste Tempo(s)";
            if (synchronize) manager_->changed();
            manager_->invalidate();
        }
        if (modified) *modified = changed;
        return S_OK;
    } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
HRESULT TempoStrip::QueryContinueDrag(BOOL escape, DWORD keys) {
    // Original RVA 0x7e0d: release drops; pressing the other button cancels.
    if (escape) return DRAGDROP_S_CANCEL;
    if (sourceKeys_ & MK_LBUTTON) {
        if (keys & MK_RBUTTON) return DRAGDROP_S_CANCEL;
        if (!(keys & MK_LBUTTON)) return DRAGDROP_S_DROP;
    }
    if (sourceKeys_ & MK_RBUTTON) {
        if (keys & MK_LBUTTON) return DRAGDROP_S_CANCEL;
        if (!(keys & MK_RBUTTON)) return DRAGDROP_S_DROP;
    }
    return S_OK;
}
HRESULT TempoStrip::DragEnter(IDataObject* data, DWORD keys, POINTL point, DWORD* effect) {
    if (!data || !effect) return E_POINTER;
    if (dropData_) dropData_->Release();
    dropData_ = data; dropData_->AddRef();
    return DragOver(keys, point, effect);
}
HRESULT TempoStrip::DragOver(DWORD keys, POINTL point, DWORD* effect) {
    if (!effect) return E_POINTER;
    FORMATETC format{static_cast<CLIPFORMAT>(dragFormat_),nullptr,DVASPECT_CONTENT,-1,TYMED_ISTREAM};
    if (point.x < 0 || !dropData_ || dropData_->QueryGetData(&format) != S_OK) *effect = DROPEFFECT_NONE;
    else if (!(keys & MK_RBUTTON)) {
        if (keys & MK_CONTROL) *effect = DROPEFFECT_COPY;
        else if ((*effect & (DROPEFFECT_COPY|DROPEFFECT_MOVE)) == (DROPEFFECT_COPY|DROPEFFECT_MOVE)) *effect = DROPEFFECT_MOVE;
    }
    if (keys & (MK_LBUTTON|MK_RBUTTON)) { dropKeys_ = keys & (MK_LBUTTON|MK_RBUTTON); dropEffect_ = *effect; }
    return S_OK;
}
HRESULT TempoStrip::DragLeave() {
    if (dropData_) { auto old = dropData_; dropData_ = nullptr; old->Release(); }
    dropKeys_ = dropEffect_ = 0;
    return S_OK;
}
HRESULT TempoStrip::Drop(IDataObject* data, DWORD, POINTL point, DWORD* effect) {
    if (!effect) return E_POINTER;
    *effect = DROPEFFECT_NONE;
    if ((dropKeys_ & MK_RBUTTON) && manager_ && manager_->timeline_.get()) {
        HMODULE module=nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&producer::timeline::get_strip_property),&module);
        HMENU menu=LoadMenuA(module,MAKEINTRESOURCEA(243));
        if(menu){const HMENU popup=GetSubMenu(menu,0);
            if(!(dropEffect_&DROPEFFECT_MOVE))EnableMenuItem(popup,0x8026,MF_BYCOMMAND|MF_GRAYED);
            VARIANT window{};window.vt=VT_I4;
            const auto queried=producer::timeline::get_strip_property(manager_->timeline_.get(),static_cast<producer::Strip*>(this),1,&window);
            const HDC dc=SUCCEEDED(queried)?reinterpret_cast<HDC>(static_cast<LONG_PTR>(window.lVal)):nullptr;
            const HWND owner=dc?WindowFromDC(dc):nullptr;
            if(dc)ReleaseDC(owner,dc);
            if(owner){TrackPopupMenu(popup,TPM_RIGHTBUTTON,point.x,point.y,0,owner,nullptr);
                DestroyMenu(menu);menu=nullptr;
                MSG message{};while(PeekMessageA(&message,owner,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageA(&message);}
                dropEffect_=0; // Dismissal observed; menu-command integration remains unverified.
            }
            if(menu)DestroyMenu(menu);
        }
    }
    if (sourceDragging_ && point.x == dragOriginX_) { DragLeave(); return S_OK; }
    HRESULT hr = S_OK;
    if (dropEffect_ && manager_ && manager_->timeline_.get()) {
        ComOwner<IUnknown> imported;
        hr = producer::timeline::create_data_object(manager_->timeline_.get(), imported.put());
        if (SUCCEEDED(hr)) hr = producer::timeline_data::import_data(imported.get(), data);
        LONG measure = 0, beat = 0, clock = 0;
        if (SUCCEEDED(hr)) hr = producer::timeline::position_to_measure_beat(manager_->timeline_.get(), manager_->groupBits_, point.x, &measure, &beat);
        if (SUCCEEDED(hr)) hr = producer::timeline::measure_beat_to_clocks(manager_->timeline_.get(), manager_->groupBits_, measure, beat, &clock);
        bool modified=false;
        const bool synchronize=!sourceDragging_||(dropEffect_&DROPEFFECT_COPY);
        if (SUCCEEDED(hr)) hr = paste_data(imported.get(), clock, 0, false, synchronize, &modified);
        if (SUCCEEDED(hr)) *effect = dropEffect_;
        if (modified) sourceDragging_=false;
    }
    DragLeave(); return hr;
}
HRESULT TempoStrip::start_drag(DWORD keys, LONG x) {
    VARIANT capture{};capture.vt=VT_BOOL;
    producer::timeline::set_property(manager_->timeline_.get(),2,capture);
    LONG measure=0,beat=0,origin=0;
    HRESULT hr=producer::timeline::position_to_measure_beat(manager_->timeline_.get(),manager_->groupBits_,x,&measure,&beat);
    if(SUCCEEDED(hr))hr=producer::timeline::measure_beat_to_clocks(manager_->timeline_.get(),manager_->groupBits_,measure,beat,&origin);
    if(FAILED(hr))return E_FAIL;
    const auto bytes=manager_->track_.copy_selected(origin);
    ComOwner<IDataObject> data;
    *data.put()=new(std::nothrow) producer::tempo::TempoDataObject(static_cast<CLIPFORMAT>(dragFormat_),bytes,&objectCount);
    if(!data.get())return E_FAIL;
    auto events=manager_->track_.events();
    for(auto& event:events)event.dragged=event.selected&&event.bpm!=0;
    manager_->track_.replace_events(std::move(events));
    sourceKeys_=keys;sourceDragging_=true;dragOriginX_=x;
    const DWORD allowed=CanCut()==S_OK?DROPEFFECT_COPY|DROPEFFECT_MOVE:DROPEFFECT_COPY;
    DWORD effect=0;hr=DoDragDrop(data.get(),static_cast<IDropSource*>(this),allowed,&effect);
    sourceKeys_=0;
    if(hr==DRAGDROP_S_DROP&&(effect&DROPEFFECT_MOVE)){
        events=manager_->track_.events();
        events.erase(std::remove_if(events.begin(),events.end(),[](const auto& event){return event.dragged;}),events.end());
        manager_->track_.replace_events(std::move(events));
        manager_->undoLabel_=sourceDragging_?L"Delete Tempo(s)":L"Move Tempo(s)";manager_->changed();manager_->invalidate();
    }
    events=manager_->track_.events();for(auto& event:events)event.dragged=false;
    manager_->track_.replace_events(std::move(events));
    sourceDragging_=false;
    return hr==DRAGDROP_S_DROP&&effect?S_OK:E_FAIL;
}
HRESULT TempoStrip::insertion_time(LONG& time) {
    if (!manager_ || !manager_->timeline_.get()) return E_FAIL;
    LONG measure = 0, beat = 0;
    HRESULT hr = producer::timeline::position_to_measure_beat(manager_->timeline_.get(), manager_->groupBits_,
        insertionPosition_, &measure, &beat);
    if (SUCCEEDED(hr)) hr = producer::timeline::measure_beat_to_clocks(manager_->timeline_.get(), manager_->groupBits_, measure, beat, &time);
    return hr;
}
HRESULT TempoStrip::OnWMMessage(UINT message, WPARAM keys, LPARAM, LONG x, LONG) {
    if (!manager_ || !manager_->timeline_.get()) return E_FAIL;
    if (message == WM_CREATE) {
        dragFormat_ = RegisterClipboardFormatA("Jazz v.1 Tempolist");
        producer::timeline::get_marker(manager_->timeline_.get(), 1, &selectionStart_);
        producer::timeline::get_marker(manager_->timeline_.get(), 2, &selectionEnd_);
        return S_OK;
    }
    if (message == WM_COMMAND) {
        // Resource IDs and the LOWORD decoding are confirmed by the original
        // message entry point (RVA 0xb16d), independently of TimelineEdit order.
        switch(LOWORD(keys)) {
        case 0x8003: return Delete();
        case 0x8004: return Insert();
        case 0xe12a: return SelectAll();
        case 0x8000: return manager_->ShowProperties();
        case 0xe122: return Copy(nullptr);
        case 0xe123: return Cut(nullptr);
        case 0xe125: return Paste(nullptr);
        case 0x8001: return E_NOTIMPL; // Position-dependent dialog remains unverified.
        default: return S_OK; // Unknown commands do not modify the original model.
        }
    }
    if (message == WM_MOUSEMOVE) {
        if (dragPending_) { start_drag(sourceKeys_,x); dragPending_=false; }
        return S_OK; // Original does not propagate the drag helper's result.
    }
    if (message != WM_LBUTTONDOWN && message != WM_LBUTTONUP) return E_NOTIMPL;
    if (message == WM_LBUTTONDOWN) sourceKeys_ = static_cast<DWORD>(keys);
    insertionPosition_ = x;
    try {
        auto timeline = manager_->timeline_.get();
        LONG measure=0,beat=0;
        HRESULT hr = producer::timeline::position_to_measure_beat(timeline,manager_->groupBits_,x,&measure,&beat);
        producer::tempo::Position position{measure,beat,0};
        if (FAILED(hr)) return hr;
        auto events = manager_->track_.events();
        const auto same_beat = [](const auto& a,const auto& b){return a.measure==b.measure&&a.beat==b.beat;};
        const auto before = [](const auto& a,const auto& b){return a.measure<b.measure||(a.measure==b.measure&&a.beat<b.beat);};
        const auto clear = [&](){for(auto& event:events)event.selected=false;};
        size_t hit=events.size();
        for(size_t i=0;i<events.size();++i) if(same_beat(events[i].position,position)) {
            if(hit==events.size())hit=i;
            if(events[i].bpm!=0){hit=i;break;}
        }
        if(message==WM_LBUTTONUP) {
            if(collapseSelectionOnRelease_ && hit<events.size() && events[hit].bpm!=0) {
                clear();events[hit].selected=true;selectionAnchor_=events[hit].position;
                manager_->track_.replace_events(std::move(events));manager_->invalidate();
            }
            collapseSelectionOnRelease_=false;dragPending_=false;reset_markers();return S_OK;
        }
        collapseSelectionOnRelease_=false;dragPending_=false;
        reset_markers();
        VARIANT capture{};capture.vt=VT_BOOL;capture.boolVal=1;
        producer::timeline::set_property(timeline,2,capture);
        const auto append_placeholder = [&](producer::tempo::Position at)->HRESULT {
            LONG clock=0;at.beat=(std::max)(0L,(std::min)(255L,static_cast<LONG>(at.beat)));at.tick=0;
            const HRESULT converted=producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,at.measure,at.beat,&clock);
            if(FAILED(converted))return converted;
            events.push_back({clock,0,false,at});return S_OK;
        };
        if(hit==events.size()) { hr=append_placeholder(position);if(FAILED(hr))return hr; }
        if(keys&MK_CONTROL) {
            if(events[hit].selected && events[hit].bpm!=0){
                const HRESULT dragged=start_drag(static_cast<DWORD>(keys),x);
                if(FAILED(dragged)){
                    events=manager_->track_.events();events[hit].selected=!events[hit].selected;
                    manager_->track_.replace_events(std::move(events));
                }
                manager_->invalidate();manager_->ShowProperties();return dragged;
            }
            events[hit].selected=!events[hit].selected;selectionAnchor_=events[hit].position;
            dragPending_=events[hit].selected&&events[hit].bpm!=0;
        } else if(keys&MK_SHIFT) {
            auto start=selectionAnchor_,end=events[hit].position;
            if(before(end,start))std::swap(start,end);
            auto at=start;at.tick=0;
            while(!before(end,at)) {
                if(std::none_of(events.begin(),events.end(),[&](const auto& event){return same_beat(event.position,at);})) {
                    hr=append_placeholder(at);if(FAILED(hr))return hr;
                }
                LONG next=0;
                hr=producer::timeline::measure_beat_to_clocks(timeline,manager_->groupBits_,at.measure,at.beat+1,&next);
                LONG nextMeasure=0,nextBeat=0;
                if(SUCCEEDED(hr))hr=producer::timeline::clocks_to_measure_beat(timeline,manager_->groupBits_,next,&nextMeasure,&nextBeat);
                producer::tempo::Position following{nextMeasure,nextBeat,0};
                if(FAILED(hr))return hr;
                if(!before(at,following))return E_FAIL;
                at=following;
            }
            for(auto& event:events)event.selected=!before(event.position,start)&&!before(end,event.position);
        } else if(events[hit].selected && events[hit].bpm!=0) {
            collapseSelectionOnRelease_=true;dragPending_=true;
        } else {
            clear();events[hit].selected=true;selectionAnchor_=events[hit].position;
            dragPending_=events[hit].bpm!=0;
        }
        manager_->track_.replace_events(std::move(events));manager_->invalidate();
        manager_->ShowProperties();
        return S_OK;
    } catch(const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch(...) { return E_FAIL; }
}
HRESULT TempoStrip::CanInsert() {
    if (insertionPosition_ < 0) return S_FALSE;
    LONG time = 0;
    if (FAILED(insertion_time(time))) return S_OK; // Original hit lookup returns no item.
    producer::tempo_notifications::TimeSignature meter{};
    if (FAILED(producer::timeline::get_parameter(manager_->timeline_.get(), producer::tempo_notifications::timeSignature,
        manager_->groupBits_, time, &meter)) || !meter.denominator) return S_OK;
    const auto end = static_cast<std::int64_t>(time) + 3072 / meter.denominator;
    for (const auto& event : manager_->track_.events()) {
        if (event.time >= end) break;
        if (event.time >= time && event.bpm != 0) return S_FALSE;
    }
    return S_OK;
}
HRESULT TempoStrip::Insert() {
    if (insertionPosition_ < 0) return E_FAIL;
    if (!manager_) return E_UNEXPECTED;
    try {
        manager_->track_.clear_selection();
        reset_markers();
        LONG time = 0;
        HRESULT hr = insertion_time(time);
        if (FAILED(hr)) return hr;
        TempoManager::PropertyData position{};
        hr = manager_->describe_position(time, position);
        if (FAILED(hr)) return hr;
        // RVA 0x9082 inserts 120 BPM even if CanInsert reports an occupied beat.
        // Positive tempos create a new node, ordered after equal-time nodes.
        auto events=manager_->track_.events();
        const auto placeholder=std::find_if(events.begin(),events.end(),[&](const auto& event){
            return event.bpm==0 && event.position.measure==position.measure && event.position.beat==position.beat;
        });
        if(placeholder!=events.end()) {
            *placeholder={time,120.0,true,TempoManager::cached_position(position)};
            manager_->track_.replace_events(std::move(events));
        } else manager_->track_.insert_event({time, 120.0, true, TempoManager::cached_position(position)});
        manager_->undoLabel_ = L"Insert Tempo";
        manager_->invalidate();
        manager_->ShowProperties();
        manager_->changed();
        return S_OK;
    } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
HRESULT TempoStrip::Delete() {
    if (!manager_) return E_UNEXPECTED;
    manager_->undoLabel_ = L"Delete Tempo(s)";
    manager_->track_.delete_selected(); manager_->invalidate(); manager_->changed(); return S_OK;
}
HRESULT TempoStrip::CanDelete() { return manager_ && manager_->track_.selected_count() ? S_OK : S_FALSE; }
HRESULT TempoStrip::CanSelectAll() {
    return manager_ && std::any_of(manager_->track_.events().begin(),manager_->track_.events().end(),
        [](const auto& event){return event.bpm!=0;}) ? S_OK : S_FALSE;
}

class Factory final : public IClassFactory {
    LONG references_ = 1;
public:
    Factory() { InterlockedIncrement(&objectCount); }
    ~Factory() { InterlockedDecrement(&objectCount); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != IID_IUnknown && iid != IID_IClassFactory) return E_NOINTERFACE;
        *out = static_cast<IClassFactory*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = InterlockedDecrement(&references_);
        if (!count) delete this;
        return static_cast<ULONG>(count);
    }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION; // Aggregated original class remains to be recovered.
        try {
            auto manager = new TempoManager;
            const HRESULT hr = manager->QueryInterface(iid, out);
            manager->Release(); return hr;
        } catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
        if (lock) InterlockedIncrement(&lockCount); else InterlockedDecrement(&lockCount);
        return S_OK;
    }
};
}

extern "C" HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID clsid, REFIID iid, void** out) {
    if (!out) return E_POINTER;
    *out = nullptr;
    if (clsid != producer::CLSID_TempoMgr) return CLASS_E_CLASSNOTAVAILABLE;
    auto factory = new (std::nothrow) Factory;
    if (!factory) return E_OUTOFMEMORY;
    const HRESULT hr = factory->QueryInterface(iid, out);
    factory->Release(); return hr;
}
extern "C" HRESULT STDAPICALLTYPE DllCanUnloadNow() {
    return InterlockedCompareExchange(&objectCount, 0, 0) == 0 && InterlockedCompareExchange(&lockCount, 0, 0) == 0 ? S_OK : S_FALSE;
}
extern "C" HRESULT STDAPICALLTYPE DllRegisterServer() { return E_NOTIMPL; }
extern "C" HRESULT STDAPICALLTYPE DllUnregisterServer() { return E_NOTIMPL; }
