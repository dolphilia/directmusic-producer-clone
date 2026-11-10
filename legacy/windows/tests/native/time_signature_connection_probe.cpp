#include <windows.h>
#include <oleauto.h>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "compat/producer_ids.h"
#include "compat/strip_manager.h"
#include "compat/strip.h"
#include "compat/time_signature.h"

// A non-windowed Timeline fixture: only statically identified connection slots
// are supplied. Unknown slots fail fast instead of pretending to be supported.
static void result(const char* name,HRESULT hr) {
    std::printf("{\"operation\":\"%s\",\"hr\":\"0x%08lx\"}\n",name,static_cast<unsigned long>(hr));std::fflush(stdout);
}
static void guid(REFGUID value) {
    std::printf("%08lx-%04x-%04x-%02x%02x-",value.Data1,value.Data2,value.Data3,value.Data4[0],value.Data4[1]);
    for(int i=2;i<8;++i)std::printf("%02x",value.Data4[i]);
}
struct TimelineFixture {
    void** table;
    void* slots[45]{};
    const char* name;
    LONG refs=1;
    bool accepted=true;
    HRESULT insertResult=S_OK,notificationResult=S_OK;
    producer::Strip* retained=nullptr;
    producer::Strip* lastStrip=nullptr;
    producer::StripManager* manager;
    TimelineFixture(const char* label,producer::StripManager* owner):table(slots),name(label),manager(owner) {
        for(auto& slot:slots)slot=reinterpret_cast<void*>(&unexpected);
        slots[0]=reinterpret_cast<void*>(&query);slots[1]=reinterpret_cast<void*>(&add);slots[2]=reinterpret_cast<void*>(&release);
        slots[19]=reinterpret_cast<void*>(&remove_page);slots[32]=reinterpret_cast<void*>(&remove_strip);
        slots[35]=reinterpret_cast<void*>(&get_manager);slots[36]=reinterpret_cast<void*>(&insert_strip);
        slots[40]=reinterpret_cast<void*>(&add_notification);slots[41]=reinterpret_cast<void*>(&remove_notification);
    }
    IUnknown* unknown(){return reinterpret_cast<IUnknown*>(this);}
    static HRESULT STDMETHODCALLTYPE unexpected(TimelineFixture*) {std::printf("{\"operation\":\"unexpected_timeline_slot\"}\n");std::fflush(stdout);ExitProcess(3);return E_NOTIMPL;}
    static HRESULT STDMETHODCALLTYPE query(TimelineFixture* self,REFIID iid,void** out) {
        if(!out)return E_POINTER;*out=nullptr;
        HRESULT hr=(iid==IID_IUnknown||(self->accepted&&iid==producer::IID_IDMUSProdTimeline))?S_OK:E_NOINTERFACE;
        std::printf("{\"operation\":\"timeline_query\",\"timeline\":\"%s\",\"iid\":\"",self->name);guid(iid);
        std::printf("\",\"hr\":\"0x%08lx\"}\n",static_cast<unsigned long>(hr));
        if(SUCCEEDED(hr)){*out=self;add(self);}return hr;
    }
    static ULONG STDMETHODCALLTYPE add(TimelineFixture* self) {
        ++self->refs;std::printf("{\"operation\":\"timeline_addref\",\"timeline\":\"%s\",\"refs\":%ld}\n",self->name,self->refs);return self->refs;
    }
    static ULONG STDMETHODCALLTYPE release(TimelineFixture* self) {
        --self->refs;std::printf("{\"operation\":\"timeline_release\",\"timeline\":\"%s\",\"refs\":%ld}\n",self->name,self->refs);return self->refs;
    }
    static HRESULT STDMETHODCALLTYPE remove_page(TimelineFixture* self,IUnknown* object) {
        std::printf("{\"operation\":\"remove_page_object\",\"timeline\":\"%s\",\"nonNull\":%s}\n",self->name,object?"true":"false");return S_OK;
    }
    static HRESULT STDMETHODCALLTYPE insert_strip(TimelineFixture* self,IUnknown* object,REFCLSID clsid,DWORD groups,DWORD index) {
        self->lastStrip=reinterpret_cast<producer::Strip*>(object);
        std::printf("{\"operation\":\"insert_strip\",\"timeline\":\"%s\",\"nonNull\":%s,\"class\":\"",self->name,object?"true":"false");guid(clsid);
        std::printf("\",\"groups\":%lu,\"index\":%lu,\"hr\":\"0x%08lx\"}\n",groups,index,static_cast<unsigned long>(self->insertResult));
        if(SUCCEEDED(self->insertResult)&&object){object->AddRef();self->retained=self->lastStrip;}return self->insertResult;
    }
    static HRESULT STDMETHODCALLTYPE remove_strip(TimelineFixture* self,IUnknown* object) {
        std::printf("{\"operation\":\"remove_strip\",\"timeline\":\"%s\",\"sameStrip\":%s}\n",self->name,object==self->lastStrip?"true":"false");
        if(self->retained){self->retained->Release();self->retained=nullptr;}return S_OK;
    }
    static HRESULT notification(TimelineFixture* self,const char* name,IUnknown* object,REFGUID type,DWORD groups) {
        std::printf("{\"operation\":\"%s\",\"timeline\":\"%s\",\"sameManager\":%s,\"type\":\"",name,self->name,object==self->manager?"true":"false");guid(type);
        std::printf("\",\"groups\":%lu,\"hr\":\"0x%08lx\"}\n",groups,static_cast<unsigned long>(self->notificationResult));return self->notificationResult;
    }
    static HRESULT STDMETHODCALLTYPE add_notification(TimelineFixture* self,IUnknown* manager,REFGUID type,DWORD groups){return notification(self,"add_notification",manager,type,groups);}
    static HRESULT STDMETHODCALLTYPE remove_notification(TimelineFixture* self,IUnknown* manager,REFGUID type,DWORD groups){return notification(self,"remove_notification",manager,type,groups);}
    static HRESULT STDMETHODCALLTYPE get_manager(TimelineFixture* self,REFGUID type,DWORD groups,DWORD index,IUnknown** out) {
        if(out)*out=nullptr;
        std::printf("{\"operation\":\"get_track_manager\",\"timeline\":\"%s\",\"type\":\"",self->name);guid(type);
        std::printf("\",\"groups\":%lu,\"index\":%lu,\"hr\":\"0x80004005\"}\n",groups,index);return E_FAIL;
    }
};
static void inspect_strip(producer::Strip* strip,producer::StripManager* manager,const char* phase) {
    std::printf("{\"operation\":\"begin_strip_properties\",\"phase\":\"%s\",\"present\":%s}\n",phase,strip?"true":"false");
    if(!strip)return;
    IUnknown* canonicalManager=nullptr;manager->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&canonicalManager));
    result("strip_null_property",strip->GetStripProperty(0,nullptr));
    for(DWORD property:{0UL,1UL,2UL,6UL,7UL,8UL,9UL,10UL,12UL,99UL}) {
        VARIANT value{};HRESULT hr=strip->GetStripProperty(property,&value);
        std::printf("{\"operation\":\"strip_property\",\"property\":%lu,\"hr\":\"0x%08lx\",\"vt\":%u,\"value\":%ld,\"managerIdentity\":%s,\"utf16\":[",property,static_cast<unsigned long>(hr),value.vt,
            value.vt==VT_BOOL?static_cast<LONG>(value.boolVal):value.vt==VT_INT?value.intVal:0,value.vt==VT_UNKNOWN&&value.punkVal==canonicalManager?"true":"false");
        if(value.vt==VT_BSTR)for(UINT i=0;i<SysStringLen(value.bstrVal);++i)std::printf("%s%u",i?",":"",value.bstrVal[i]);
        std::printf("]}\n");VariantClear(&value);
    }
    canonicalManager->Release();std::printf("{\"operation\":\"end_strip_properties\"}\n");
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=3)return 2;const HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);result("initialize",init);if(FAILED(init))return 1;
    HMODULE module=LoadLibraryExW(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!module){result("load_module",HRESULT_FROM_WIN32(GetLastError()));CoUninitialize();return 1;}
    using GetClass=HRESULT(STDAPICALLTYPE*)(REFCLSID,REFIID,void**);using CanUnload=HRESULT(STDAPICALLTYPE*)();
    auto getClass=reinterpret_cast<GetClass>(GetProcAddress(module,"DllGetClassObject"));auto canUnload=reinterpret_cast<CanUnload>(GetProcAddress(module,"DllCanUnloadNow"));
    IClassFactory* factory=nullptr;producer::StripManager* manager=nullptr;
    HRESULT hr=getClass?getClass(producer::CLSID_TimeSignatureMgr,IID_IClassFactory,reinterpret_cast<void**>(&factory)):E_NOINTERFACE;result("class_factory",hr);
    if(FAILED(hr)){CoUninitialize();return 1;}
    hr=factory->CreateInstance(nullptr,producer::IID_IDMUSProdStripMgr,reinterpret_cast<void**>(&manager));factory->Release();result("create_manager",hr);
    if(FAILED(hr)){CoUninitialize();return 1;}
    TimelineFixture first("first",manager),second("second",manager),rejected("rejected",manager);rejected.accepted=false;
    bool ok=true;
    auto set=[&](const char* phase,TimelineFixture* value,UINT vt=VT_UNKNOWN){
        std::printf("{\"operation\":\"begin_connection\",\"phase\":\"%s\"}\n",phase);std::fflush(stdout);
        VARIANT variant{};variant.vt=static_cast<VARTYPE>(vt);variant.punkVal=value?value->unknown():nullptr;
        const HRESULT answer=manager->SetStripMgrProperty(0,variant);result("set_timeline",answer);
        VARIANT property{};HRESULT get=manager->GetStripMgrProperty(0,&property);
        std::printf("{\"operation\":\"get_timeline\",\"hr\":\"0x%08lx\",\"vt\":%u,\"sameInput\":%s,\"isNull\":%s}\n",static_cast<unsigned long>(get),property.vt,
            value&&property.punkVal==value->unknown()?"true":"false",property.punkVal?"false":"true");VariantClear(&property);
        std::printf("{\"operation\":\"end_connection\",\"firstRefs\":%ld,\"secondRefs\":%ld,\"rejectedRefs\":%ld}\n",first.refs,second.refs,rejected.refs);std::fflush(stdout);
        if(vt==VT_UNKNOWN)ok=(answer==(value==&rejected?E_FAIL:S_OK))&&ok;
        else ok=(answer==E_INVALIDARG)&&ok;
    };
    set("wrong_type_empty",&first,VT_I4);set("connect_first",&first);inspect_strip(first.retained,manager,"groups_one");
    set("wrong_type_connected",&second,VT_I4);set("repeat_first",&first);set("reject_replacement",&rejected);
    first.insertResult=E_FAIL;first.notificationResult=E_FAIL;set("failed_insert_notifications",&first);set("disconnect_failed_insert",nullptr);
    producer::TimeSignatureTrackHeader header{};header.groups=0x42;VARIANT groups{};groups.vt=VT_BYREF;groups.byref=&header;result("change_groups",manager->SetStripMgrProperty(3,groups));
    set("connect_second",&second);inspect_strip(second.retained,manager,"groups_two_seven");first.insertResult=S_OK;first.notificationResult=S_OK;
    set("replace_second_first",&first);set("disconnect",nullptr);set("repeat_disconnect",nullptr);
    const ULONG remaining=manager->Release();std::printf("{\"operation\":\"release_manager\",\"remaining\":%lu}\n",remaining);
    std::printf("{\"operation\":\"final_fixture_refs\",\"first\":%ld,\"second\":%ld,\"rejected\":%ld,\"retainedStrips\":%u}\n",first.refs,second.refs,rejected.refs,(first.retained?1u:0u)+(second.retained?1u:0u));
    hr=canUnload?canUnload():E_NOINTERFACE;result("can_unload",hr);ok=ok&&remaining==0&&first.refs==1&&second.refs==1&&rejected.refs==1&&!first.retained&&!second.retained&&hr==S_OK;
    if(hr==S_OK)FreeLibrary(module);CoUninitialize();return ok?0:1;
}
