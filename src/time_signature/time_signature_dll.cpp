#include <windows.h>
#include <objidl.h>
#include <oleauto.h>
#include <new>
#include <vector>
#include <string>
#include "time_signature_map.h"
#include "compat/strip_manager.h"
#include "compat/producer_ids.h"
#include "compat/strip.h"
#include "compat/prop_page_object.h"
#include "compat/timeline_services.h"

namespace {
LONG objectCount=0,lockCount=0;
class Manager;
class MeterStrip final:public producer::Strip,public producer::PropPageObject {
    LONG refs_=1;
    Manager* manager_; // Borrowed, original strip constructor RVA 0x8e33.
public:
    explicit MeterStrip(Manager* manager):manager_(manager){InterlockedIncrement(&objectCount);}
    ~MeterStrip(){InterlockedDecrement(&objectCount);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(iid==IID_IUnknown||iid==producer::IID_Strip)*out=static_cast<producer::Strip*>(this);
        else if(iid==producer::IID_IDMUSProdPropPageObject)*out=static_cast<producer::PropPageObject*>(this);
        else return E_NOINTERFACE;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return static_cast<ULONG>(InterlockedIncrement(&refs_));}
    ULONG STDMETHODCALLTYPE Release() override {const LONG n=InterlockedDecrement(&refs_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetStripProperty(DWORD,VARIANT*) override;
    HRESULT STDMETHODCALLTYPE Draw(HDC,DWORD,LONG) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetStripProperty(DWORD,VARIANT) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OnWMMessage(UINT,WPARAM,LPARAM,LONG,LONG) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetData(void**) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetData(void*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ShowProperties() override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() override {return E_NOTIMPL;}
};
class Manager final:public producer::StripManager,public IPersistStream,public producer::PropPageObject {
    friend class MeterStrip;
    LONG refs_=1;
    producer::meter::Map map_;
    DWORD groups_=1,flags_=56,producerFlags_=0;
    IUnknown* runtime_=nullptr;
    IUnknown* framework_=nullptr;
    IUnknown* timeline_=nullptr;
    MeterStrip* strip_;
    // SetParam RVA 0x66be stores without AddRef. GetParam 0x65ea adds a
    // reference for its caller; the manager destructor does not own this slot.
    IUnknown* borrowed_=nullptr;
    HRESULT connect_timeline(IUnknown* input){
        if(timeline_){
            producer::timeline::remove_page_object(timeline_,static_cast<producer::PropPageObject*>(this));
            for(int i=2;i>=0;--i)producer::timeline::remove_notification(timeline_,static_cast<producer::StripManager*>(this),producer::TimeSignatureNotifications[i],groups_);
            producer::timeline::remove_page_object(timeline_,static_cast<producer::PropPageObject*>(strip_));
            producer::timeline::remove_strip(timeline_,static_cast<producer::Strip*>(strip_));
            timeline_->Release();timeline_=nullptr;
        }
        if(!input)return S_OK;
        const HRESULT hr=input->QueryInterface(producer::IID_IDMUSProdTimeline,reinterpret_cast<void**>(&timeline_));
        if(FAILED(hr))return E_FAIL;
        // RVA 0x7815, 0x7832..0x785a ignore these service HRESULTs.
        producer::timeline::insert_strip(timeline_,static_cast<producer::Strip*>(strip_),producer::CLSID_DirectMusicTimeSignatureTrack,groups_);
        for(const GUID& type:producer::TimeSignatureNotifications)producer::timeline::add_notification(timeline_,static_cast<producer::StripManager*>(this),type,groups_);
        IUnknown* style=nullptr;
        producer::timeline::get_strip_manager(timeline_,producer::GUID_TimeSignatureStyleParam,groups_,0,&style);
        // Original RVA 0x7205 imports Style time signatures if another manager is
        // found. That path remains pending; this connection comparison uses
        // a Timeline with no Style manager. Never claim it as implemented.
        if(style)style->Release();
        return S_OK;
    }
public:
    Manager():strip_(new MeterStrip(this)){InterlockedIncrement(&objectCount);}
    ~Manager(){
        // Valid hosts disconnect before destruction; also release our owned
        // connection if an incomplete host tears the object down directly.
        connect_timeline(nullptr);strip_->Release();
        if(framework_)framework_->Release(); // Base offset 0x34, RVA 0x7503.
        if(runtime_)runtime_->Release();     // Base offset 0x50, RVA 0x7521.
        InterlockedDecrement(&objectCount);
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(iid==IID_IUnknown||iid==producer::IID_IDMUSProdStripMgr)*out=static_cast<producer::StripManager*>(this);
        else if(iid==IID_IPersist||iid==IID_IPersistStream)*out=static_cast<IPersistStream*>(this);
        else if(iid==producer::IID_IDMUSProdPropPageObject)*out=static_cast<producer::PropPageObject*>(this);
        else return E_NOINTERFACE;
        AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return static_cast<ULONG>(InterlockedIncrement(&refs_));}
    ULONG STDMETHODCALLTYPE Release() override {const auto n=InterlockedDecrement(&refs_);if(!n)delete this;return static_cast<ULONG>(n);}
    HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID type) override {
        return type==producer::GUID_TimeSignatureParam||type==producer::GUID_TimeSignatureBorrowedObject||type==producer::GUID_TimeSignatureUndoLabel?S_OK:S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE GetParam(REFGUID type,LONG time,LONG* next,void* value) override {
        if(!value)return E_POINTER;
        if(type==producer::GUID_TimeSignatureBorrowedObject){
            *static_cast<IUnknown**>(value)=borrowed_;if(borrowed_)borrowed_->AddRef();return S_OK;
        }
        if(type==producer::GUID_TimeSignatureUndoLabel){
            // Strip construction initializes undo resource ID to zero (RVA
            // 0x8e75). Editing will provide resource-specific labels later.
            *static_cast<BSTR*>(value)=SysAllocString(L"");return *static_cast<BSTR*>(value)?S_OK:E_OUTOFMEMORY;
        }
        if(type!=producer::GUID_TimeSignatureParam)return E_INVALIDARG;
        if(next)*next=0;
        const auto result=map_.query(time);
        if(!result.found)return static_cast<HRESULT>(0x88781161UL);
        *static_cast<producer::TimeSignatureParam*>(value)=result.value;
        if(next)*next=result.next;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetParam(REFGUID type,LONG,void* value) override {
        if(!value)return E_POINTER;
        if(type!=producer::GUID_TimeSignatureBorrowedObject)return E_INVALIDARG;
        borrowed_=static_cast<IUnknown*>(value);return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnUpdate(REFGUID,DWORD,void*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetStripMgrProperty(DWORD property,VARIANT* value) override {
        if(!value)return E_POINTER;
        if(property<=2){
            value->vt=VT_UNKNOWN;value->punkVal=nullptr;
            if(property==0){value->punkVal=timeline_;if(timeline_)timeline_->AddRef();return timeline_?S_OK:E_FAIL;}
            IUnknown* stored=property==1?runtime_:framework_;
            // Original Get properties 1/2 queries canonical IUnknown, rather
            // than returning the stored service-interface pointer directly.
            return stored?stored->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&value->punkVal)):E_FAIL;
        }
        if(property>=3&&property<=5){
            if(value->vt!=VT_BYREF)return E_INVALIDARG;
            if(!value->byref)return E_POINTER;
            if(property==3)*static_cast<producer::TimeSignatureTrackHeader*>(value->byref)={producer::CLSID_DirectMusicTimeSignatureTrack,0,groups_,0,0x534d4954};
            else *static_cast<DWORD*>(value->byref)=property==4?flags_:producerFlags_;return S_OK;
        }
        if(property==6){value->vt=VT_I4;value->lVal=0x30038;return S_OK;}
        return E_INVALIDARG;
    }
    HRESULT STDMETHODCALLTYPE SetStripMgrProperty(DWORD property,VARIANT value) override {
        if(property<=2){
            if(value.vt!=VT_UNKNOWN)return E_INVALIDARG;
            if(property==0)return connect_timeline(value.punkVal);
            IUnknown*& stored=property==1?runtime_:framework_;
            // Original releases the previous service before querying its
            // replacement. The caller must own its incoming pointer.
            if(stored)stored->Release();stored=nullptr;
            if(!value.punkVal)return S_OK;
            const GUID& iid=property==1?producer::IID_TimeSignatureRuntimeTrack:producer::IID_IDMUSProdFramework;
            const HRESULT hr=value.punkVal->QueryInterface(iid,reinterpret_cast<void**>(&stored));
            // RVA 0x7735 deliberately ignores runtime QI failure. Framework
            // QI at 0x76fa returns its HRESULT to the caller.
            return property==1?S_OK:hr;
        }
        if(property>5||value.vt!=VT_BYREF)return E_INVALIDARG;
        if(!value.byref)return E_POINTER;
        if(property==3)groups_=static_cast<producer::TimeSignatureTrackHeader*>(value.byref)->groups;
        else if(property==4)flags_=*static_cast<DWORD*>(value.byref)&0x30038;
        else producerFlags_=*static_cast<DWORD*>(value.byref);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetData(void**) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetData(void*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ShowProperties() override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OnRemoveFromPageManager() override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID* out) override {if(!out)return E_POINTER;*out=producer::CLSID_TimeSignatureMgr;return S_OK;}
    HRESULT STDMETHODCALLTYPE IsDirty() override {return S_FALSE;}
    HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE Load(IStream* stream) override {
        if(!stream)return E_INVALIDARG;
        try {
            STATSTG stat{};HRESULT hr=stream->Stat(&stat,STATFLAG_NONAME);if(FAILED(hr))return hr;
            LARGE_INTEGER zero{};ULARGE_INTEGER position{};hr=stream->Seek(zero,STREAM_SEEK_CUR,&position);if(FAILED(hr))return hr;
            if(stat.cbSize.QuadPart<position.QuadPart||stat.cbSize.QuadPart-position.QuadPart>16*1024*1024)return E_INVALIDARG;
            std::vector<std::uint8_t> bytes(static_cast<size_t>(stat.cbSize.QuadPart-position.QuadPart));ULONG count=0;
            hr=stream->Read(bytes.data(),static_cast<ULONG>(bytes.size()),&count);if(FAILED(hr))return hr;
            if(count!=bytes.size())return E_FAIL;
            const auto result=map_.load(bytes);
            return result==producer::meter::LoadResult::ok?S_OK:result==producer::meter::LoadResult::unsupported?E_NOTIMPL:E_FAIL;
        }catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_FAIL;}
    }
    HRESULT STDMETHODCALLTYPE Save(IStream* stream,BOOL) override {
        if(!stream)return E_INVALIDARG;
        try {const auto bytes=map_.save();ULONG count=0;const HRESULT hr=stream->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&count);return FAILED(hr)?hr:count==bytes.size()?S_OK:E_FAIL;}
        catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_FAIL;}
    }
};
HRESULT MeterStrip::GetStripProperty(DWORD property,VARIANT* value){
    if(!value)return E_POINTER;
    switch(property){
    case 0:{
        std::wstring name;
        for(unsigned bit=0;bit<32;){
            if(!(manager_->groups_&(DWORD(1)<<bit))){++bit;continue;}
            const unsigned first=bit;while(bit+1<32&&(manager_->groups_&(DWORD(1)<<(bit+1))))++bit;
            if(!name.empty())name+=L", ";name+=std::to_wstring(first+1);
            if(bit>first)name+=L"-"+std::to_wstring(bit+1);++bit;
        }
        name+=L": TimeSig";value->vt=VT_BSTR;value->bstrVal=SysAllocString(name.c_str());return value->bstrVal?S_OK:E_OUTOFMEMORY;
    }
    case 1:value->vt=VT_BOOL;value->boolVal=1;return S_OK;
    case 6:case 8:case 9:value->vt=VT_INT;value->intVal=20;return S_OK;
    case 7:case 10:value->vt=VT_BOOL;value->boolVal=0;return S_OK;
    case 12:value->vt=VT_UNKNOWN;value->punkVal=nullptr;return manager_->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&value->punkVal));
    default:return E_FAIL;
    }
}
class Factory final:public IClassFactory {
    LONG refs_=1;
public:
    Factory(){InterlockedIncrement(&objectCount);}
    ~Factory(){InterlockedDecrement(&objectCount);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=IID_IClassFactory)return E_NOINTERFACE;
        *out=static_cast<IClassFactory*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return static_cast<ULONG>(InterlockedIncrement(&refs_));}
    ULONG STDMETHODCALLTYPE Release() override {const auto n=InterlockedDecrement(&refs_);if(!n)delete this;return static_cast<ULONG>(n);}
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(outer)return CLASS_E_NOAGGREGATION;
        try{auto manager=new Manager;const HRESULT hr=manager->QueryInterface(iid,out);manager->Release();return hr;}
        catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(...){return E_FAIL;}
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {if(lock)InterlockedIncrement(&lockCount);else InterlockedDecrement(&lockCount);return S_OK;}
};
}
extern "C" HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID clsid,REFIID iid,void** out){
    if(!out)return E_POINTER;*out=nullptr;if(clsid!=producer::CLSID_TimeSignatureMgr)return CLASS_E_CLASSNOTAVAILABLE;
    auto factory=new(std::nothrow)Factory;if(!factory)return E_OUTOFMEMORY;
    const HRESULT hr=factory->QueryInterface(iid,out);factory->Release();return hr;
}
extern "C" HRESULT STDAPICALLTYPE DllCanUnloadNow(){return InterlockedCompareExchange(&objectCount,0,0)==0&&InterlockedCompareExchange(&lockCount,0,0)==0?S_OK:S_FALSE;}
extern "C" HRESULT STDAPICALLTYPE DllRegisterServer(){return E_NOTIMPL;}
extern "C" HRESULT STDAPICALLTYPE DllUnregisterServer(){return E_NOTIMPL;}
