#include <windows.h>
#include <objidl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <initializer_list>
#include "compat/producer_ids.h"
#include "compat/strip_manager.h"
#include "compat/time_signature.h"

using producer::TimeSignatureParam;
static void result(const char* operation,HRESULT hr) {
    std::printf("{\"operation\":\"%s\",\"hr\":\"0x%08lx\"}\n",operation,static_cast<unsigned long>(hr));
    std::fflush(stdout);
}
static IStream* stream(const std::vector<unsigned char>& bytes) {
    IStream* out=nullptr;
    if(FAILED(CreateStreamOnHGlobal(nullptr,TRUE,&out)))return nullptr;
    ULONG count=0;
    if(!bytes.empty()&& (FAILED(out->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&count))||count!=bytes.size())){
        out->Release();return nullptr;
    }
    LARGE_INTEGER zero{};out->Seek(zero,STREAM_SEEK_SET,nullptr);return out;
}
static void u32(std::vector<unsigned char>& bytes,DWORD n) {
    for(int i=0;i<4;++i)bytes.push_back(static_cast<unsigned char>(n>>(8*i)));
}
static std::vector<unsigned char> input(std::initializer_list<TimeSignatureParam> events,DWORD recordSize=8,bool list=false) {
    std::vector<unsigned char> bytes{'t','i','m','s'};
    u32(bytes,4+static_cast<DWORD>(events.size())*recordSize);u32(bytes,recordSize);
    for(auto event:events){
        const auto* p=reinterpret_cast<const unsigned char*>(&event);bytes.insert(bytes.end(),p,p+8);
        for(DWORD i=8;i<recordSize;++i)bytes.push_back(0xa5);
    }
    if(list){std::vector<unsigned char> wrapper{'L','I','S','T'};u32(wrapper,static_cast<DWORD>(bytes.size())+4);
        wrapper.insert(wrapper.end(),{'T','I','M','S'});wrapper.insert(wrapper.end(),bytes.begin(),bytes.end());return wrapper;}
    return bytes;
}
static bool write_file(const std::wstring& name,const std::vector<unsigned char>& bytes) {
    FILE* file=nullptr;if(_wfopen_s(&file,name.c_str(),L"wb")||!file)return false;
    const auto count=fwrite(bytes.data(),1,bytes.size(),file);return fclose(file)==0&&count==bytes.size();
}
static bool save(IPersistStream* persist,const std::wstring& directory,const char* name) {
    IStream* out=stream({});if(!out)return false;
    const HRESULT hr=persist->Save(out,TRUE);
    STATSTG stat{};const HRESULT statHr=out->Stat(&stat,STATFLAG_NONAME);
    std::vector<unsigned char> bytes;
    bool ok=SUCCEEDED(hr)&&SUCCEEDED(statHr)&&stat.cbSize.HighPart==0&&stat.cbSize.LowPart<65536;
    if(ok){bytes.resize(stat.cbSize.LowPart);LARGE_INTEGER zero{};out->Seek(zero,STREAM_SEEK_SET,nullptr);ULONG count=0;
        ok=SUCCEEDED(out->Read(bytes.data(),static_cast<ULONG>(bytes.size()),&count))&&count==bytes.size();}
    out->Release();
    std::wstring filename;for(const char* c=name;*c;++c)filename+=static_cast<wchar_t>(*c);filename+=L"-saved.bin";
    if(ok)ok=write_file(directory+L"/"+filename,bytes);
    std::printf("{\"operation\":\"save\",\"case\":\"%s\",\"hr\":\"0x%08lx\",\"bytes\":%zu,\"fileWritten\":%s}\n",name,static_cast<unsigned long>(hr),bytes.size(),ok?"true":"false");
    std::fflush(stdout);return ok;
}
static void queries(producer::StripManager* manager,const char* name) {
    for(LONG time:{-1L,0L,1L,1535L,1536L,3071L,3072L,5375L,5376L,6143L,6144L,7679L,7680L,10000L}){
        TimeSignatureParam value{};std::memset(&value,0x5a,sizeof(value));LONG next=0x12345678;
        const HRESULT hr=manager->GetParam(producer::GUID_TimeSignatureParam,time,&next,&value);
        std::printf("{\"operation\":\"get_meter\",\"case\":\"%s\",\"time\":%ld,\"hr\":\"0x%08lx\",\"next\":%ld,\"returnedTime\":%ld,\"beats\":%u,\"denominator\":%u,\"grids\":%u}\n",
            name,time,static_cast<unsigned long>(hr),next,value.time,value.beats,value.denominator,value.grids);
    }
    std::fflush(stdout);
}
class BorrowedProbe final:public IUnknown {
public:
    LONG refs=1;unsigned adds=0,releases=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown)return E_NOINTERFACE;
        *out=static_cast<IUnknown*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {++adds;return static_cast<ULONG>(++refs);}
    ULONG STDMETHODCALLTYPE Release() override {++releases;return static_cast<ULONG>(--refs);}
};
// Canonical IUnknown and service pointers intentionally differ. Stack fixtures
// remain alive until after the manager releases its owned service references.
class ServiceProbe {
public:
    class View final:public IUnknown {
        ServiceProbe& owner_;
    public:
        explicit View(ServiceProbe& owner):owner_(owner){}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
            if(!out)return E_POINTER;*out=nullptr;++owner_.queries;
            if(iid==IID_IUnknown){++owner_.unknownQueries;*out=static_cast<IUnknown*>(&owner_.canonical);}
            else if(iid==owner_.serviceIid){++owner_.serviceQueries;*out=static_cast<IUnknown*>(&owner_.service);}
            else return E_NOINTERFACE;
            AddRef();return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override {++owner_.adds;return static_cast<ULONG>(++owner_.refs);}
        ULONG STDMETHODCALLTYPE Release() override {++owner_.releases;return static_cast<ULONG>(--owner_.refs);}
    };
    const GUID serviceIid;
    LONG refs=1;
    unsigned queries=0,unknownQueries=0,serviceQueries=0,adds=0,releases=0;
    View canonical,service;
    explicit ServiceProbe(REFGUID iid):serviceIid(iid),canonical(*this),service(*this){}
};
static void service_result(DWORD property,const char* phase,HRESULT hr,const ServiceProbe& source,const ServiceProbe& rejected,bool canonical=false,UINT vt=0,LONG duringGet=0){
    std::printf("{\"operation\":\"service_connection\",\"property\":%lu,\"phase\":\"%s\",\"hr\":\"0x%08lx\",\"refs\":%ld,\"adds\":%u,\"releases\":%u,\"queries\":%u,\"unknownQueries\":%u,\"serviceQueries\":%u,\"rejectedRefs\":%ld,\"rejectedQueries\":%u,\"canonicalUnknown\":%s,\"vt\":%u,\"duringGetRefs\":%ld}\n",
        property,phase,static_cast<unsigned long>(hr),source.refs,source.adds,source.releases,source.queries,source.unknownQueries,source.serviceQueries,rejected.refs,rejected.queries,canonical?"true":"false",vt,duringGet);
    std::fflush(stdout);
}
static bool services(producer::StripManager* manager,DWORD property,ServiceProbe& source,ServiceProbe& rejected){
    bool ok=true;
    auto set=[&](const char* phase,IUnknown* incoming,VARTYPE type,HRESULT expected,LONG expectedRefs){
        VARIANT value{};value.vt=type;value.punkVal=incoming;
        const HRESULT hr=manager->SetStripMgrProperty(property,value);
        service_result(property,phase,hr,source,rejected);ok=(hr==expected&&source.refs==expectedRefs&&rejected.refs==1)&&ok;
    };
    auto get=[&](const char* phase,bool connected){
        VARIANT value{};value.vt=VT_I4;value.lVal=0x5a5a5a5a;
        const HRESULT hr=manager->GetStripMgrProperty(property,&value);
        const bool identity=value.vt==VT_UNKNOWN&&value.punkVal==&source.canonical;
        const LONG during=source.refs;const UINT vt=value.vt;
        const bool nullResult=value.punkVal==nullptr;
        if(value.vt==VT_UNKNOWN&&value.punkVal)value.punkVal->Release();
        service_result(property,phase,hr,source,rejected,identity,vt,during);
        ok=(vt==VT_UNKNOWN&&(connected?(hr==S_OK&&identity&&during==3&&source.refs==2):(hr==E_FAIL&&nullResult&&source.refs==1)))&&ok;
    };
    set("wrong_type",&source.canonical,VT_I4,E_INVALIDARG,1);
    set("connect",&source.canonical,VT_UNKNOWN,S_OK,2);get("get_connected",true);
    set("repeat_same_input",&source.canonical,VT_UNKNOWN,S_OK,2);
    set("reject_replacement",&rejected.canonical,VT_UNKNOWN,property==1?S_OK:E_NOINTERFACE,1);get("get_after_rejection",false);
    set("reconnect",&source.canonical,VT_UNKNOWN,S_OK,2);
    set("disconnect",nullptr,VT_UNKNOWN,S_OK,1);get("get_disconnected",false);
    set("retain_until_destruction",&source.canonical,VT_UNKNOWN,S_OK,2);
    return ok;
}
static void track_metadata(producer::StripManager* manager,const char* phase){
    producer::TimeSignatureTrackHeader header{};std::memset(&header,0x5a,sizeof(header));
    VARIANT value{};value.vt=VT_BYREF;value.byref=&header;
    HRESULT hr=manager->GetStripMgrProperty(3,&value);
    std::printf("{\"operation\":\"track_header\",\"phase\":\"%s\",\"hr\":\"0x%08lx\",\"vt\":%u,\"expectedTrackClass\":%s,\"position\":%lu,\"groups\":%lu,\"chunk\":%lu,\"list\":%lu}\n",
        phase,static_cast<unsigned long>(hr),value.vt,header.classId==producer::CLSID_DirectMusicTimeSignatureTrack?"true":"false",header.position,header.groups,header.chunk,header.list);
    for(DWORD property:{4UL,5UL}){
        DWORD bits=0x5a5a5a5a;value.vt=VT_BYREF;value.byref=&bits;hr=manager->GetStripMgrProperty(property,&value);
        std::printf("{\"operation\":\"track_flags\",\"phase\":\"%s\",\"property\":%lu,\"hr\":\"0x%08lx\",\"flags\":%lu}\n",phase,property,static_cast<unsigned long>(hr),bits);
    }
}
static void metadata(producer::StripManager* manager,BorrowedProbe& first,BorrowedProbe& second){
    std::printf("{\"operation\":\"begin_metadata_probe\",\"version\":1}\n");
    result("supports_borrowed",manager->IsParamSupported(producer::GUID_TimeSignatureBorrowedObject));
    result("supports_undo_label",manager->IsParamSupported(producer::GUID_TimeSignatureUndoLabel));
    IUnknown* borrowed=nullptr;LONG next=0x12345678;
    HRESULT hr=manager->GetParam(producer::GUID_TimeSignatureBorrowedObject,0,&next,&borrowed);
    std::printf("{\"operation\":\"borrowed_initial\",\"hr\":\"0x%08lx\",\"isNull\":%s,\"next\":%ld}\n",static_cast<unsigned long>(hr),borrowed?"false":"true",next);
    if(borrowed)borrowed->Release();
    BSTR name=nullptr;hr=manager->GetParam(producer::GUID_TimeSignatureUndoLabel,0,nullptr,&name);
    std::printf("{\"operation\":\"undo_label\",\"hr\":\"0x%08lx\",\"utf16\":[",static_cast<unsigned long>(hr));
    for(UINT i=0;i<SysStringLen(name);++i)std::printf("%s%u",i?",":"",static_cast<unsigned>(name[i]));
    std::printf("]}\n");SysFreeString(name);
    for(DWORD property:{0UL,1UL,2UL,3UL,4UL,5UL,6UL,99UL}){
        VARIANT empty{};HRESULT nullHr=manager->GetStripMgrProperty(property,nullptr);
        HRESULT wrongHr=manager->GetStripMgrProperty(property,&empty);
        VARIANT byref{};byref.vt=VT_BYREF;HRESULT refHr=manager->GetStripMgrProperty(property,&byref);
        std::printf("{\"operation\":\"property_arguments\",\"property\":%lu,\"nullHr\":\"0x%08lx\",\"emptyHr\":\"0x%08lx\",\"nullByrefHr\":\"0x%08lx\"}\n",property,static_cast<unsigned long>(nullHr),static_cast<unsigned long>(wrongHr),static_cast<unsigned long>(refHr));
    }
    track_metadata(manager,"initial");
    for(DWORD property:{3UL,4UL,5UL,6UL,99UL}){
        VARIANT empty{},byref{};byref.vt=VT_BYREF;
        const HRESULT wrong=manager->SetStripMgrProperty(property,empty),null=manager->SetStripMgrProperty(property,byref);
        std::printf("{\"operation\":\"set_property_arguments\",\"property\":%lu,\"emptyHr\":\"0x%08lx\",\"nullByrefHr\":\"0x%08lx\"}\n",property,static_cast<unsigned long>(wrong),static_cast<unsigned long>(null));
    }
    producer::TimeSignatureTrackHeader header{};header.classId=GUID_NULL;header.position=99;header.groups=0x42;header.chunk=123;header.list=456;
    VARIANT value{};value.vt=VT_BYREF;value.byref=&header;result("set_track_header",manager->SetStripMgrProperty(3,value));
    DWORD flags=0xffffffff;value.byref=&flags;result("set_track_flags",manager->SetStripMgrProperty(4,value));
    flags=0x89abcdef;result("set_producer_flags",manager->SetStripMgrProperty(5,value));track_metadata(manager,"changed");
    result("set_borrowed_first",manager->SetParam(producer::GUID_TimeSignatureBorrowedObject,0,&first));
    borrowed=nullptr;hr=manager->GetParam(producer::GUID_TimeSignatureBorrowedObject,0,nullptr,&borrowed);
    std::printf("{\"operation\":\"borrowed_first\",\"hr\":\"0x%08lx\",\"identity\":%s,\"refs\":%ld,\"adds\":%u,\"releases\":%u}\n",static_cast<unsigned long>(hr),borrowed==&first?"true":"false",first.refs,first.adds,first.releases);
    if(borrowed)borrowed->Release();
    result("set_borrowed_second",manager->SetParam(producer::GUID_TimeSignatureBorrowedObject,0,&second));
    borrowed=nullptr;hr=manager->GetParam(producer::GUID_TimeSignatureBorrowedObject,0,nullptr,&borrowed);
    std::printf("{\"operation\":\"borrowed_second\",\"hr\":\"0x%08lx\",\"identity\":%s,\"firstRefs\":%ld,\"firstReleases\":%u,\"refs\":%ld,\"adds\":%u,\"releases\":%u}\n",static_cast<unsigned long>(hr),borrowed==&second?"true":"false",first.refs,first.releases,second.refs,second.adds,second.releases);
    if(borrowed)borrowed->Release();
    result("set_borrowed_null",manager->SetParam(producer::GUID_TimeSignatureBorrowedObject,0,nullptr));
    header.groups=1;value.byref=&header;result("reset_track_header",manager->SetStripMgrProperty(3,value));
    flags=56;value.byref=&flags;result("reset_track_flags",manager->SetStripMgrProperty(4,value));
    flags=0;result("reset_producer_flags",manager->SetStripMgrProperty(5,value));
    std::printf("{\"operation\":\"end_metadata_probe\"}\n");std::fflush(stdout);
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=3)return 2;
    const HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);result("initialize",init);if(FAILED(init))return 1;
    HMODULE module=LoadLibraryExW(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!module){result("load_module",HRESULT_FROM_WIN32(GetLastError()));CoUninitialize();return 1;}
    using GetClass=HRESULT(STDMETHODCALLTYPE*)(REFCLSID,REFIID,void**);
    const auto getClass=reinterpret_cast<GetClass>(GetProcAddress(module,"DllGetClassObject"));
    using CanUnload=HRESULT(STDMETHODCALLTYPE*)();const auto canUnload=reinterpret_cast<CanUnload>(GetProcAddress(module,"DllCanUnloadNow"));
    IClassFactory* factory=nullptr;producer::StripManager* manager=nullptr;IPersistStream* persist=nullptr;
    BorrowedProbe firstBorrowed,secondBorrowed;
    ServiceProbe runtimeService(producer::IID_TimeSignatureRuntimeTrack),frameworkService(producer::IID_IDMUSProdFramework),rejectedService(GUID_NULL);
    HRESULT hr=getClass?getClass(producer::CLSID_TimeSignatureMgr,IID_IClassFactory,reinterpret_cast<void**>(&factory)):E_NOINTERFACE;
    result("class_factory",hr);bool ok=SUCCEEDED(hr);
    if(ok){hr=factory->CreateInstance(nullptr,producer::IID_IDMUSProdStripMgr,reinterpret_cast<void**>(&manager));result("create_manager",hr);ok=SUCCEEDED(hr);factory->Release();}
    if(ok){hr=manager->QueryInterface(IID_IPersistStream,reinterpret_cast<void**>(&persist));result("query_persist",hr);ok=SUCCEEDED(hr);}
    if(ok){
        GUID classId{};hr=persist->GetClassID(&classId);result("class_id",hr);ok=SUCCEEDED(hr)&&classId==producer::CLSID_TimeSignatureMgr;
        result("class_id_null",persist->GetClassID(nullptr));result("dirty_initial",persist->IsDirty());
        ULARGE_INTEGER size{};result("size_max",persist->GetSizeMax(&size));
        GUID unknown{};result("supports_meter",manager->IsParamSupported(producer::GUID_TimeSignatureParam));
        result("supports_unknown",manager->IsParamSupported(unknown));
        TimeSignatureParam value{};result("get_unknown",manager->GetParam(unknown,0,nullptr,&value));
        result("get_meter_null",manager->GetParam(producer::GUID_TimeSignatureParam,0,nullptr,nullptr));
        result("set_meter",manager->SetParam(producer::GUID_TimeSignatureParam,0,&value));
        result("set_meter_null",manager->SetParam(producer::GUID_TimeSignatureParam,0,nullptr));
        result("load_null",persist->Load(nullptr));result("save_null",persist->Save(nullptr,TRUE));
        for(DWORD property:{0UL,1UL,2UL,4UL,5UL,6UL,99UL}){
            VARIANT v{};DWORD flags=0x12345678;if(property==4||property==5){v.vt=VT_BYREF;v.byref=&flags;}
            hr=manager->GetStripMgrProperty(property,&v);
            std::printf("{\"operation\":\"get_property\",\"property\":%lu,\"hr\":\"0x%08lx\",\"vt\":%u,\"flags\":%lu,\"value\":%ld}\n",property,static_cast<unsigned long>(hr),v.vt,flags,property==6?v.lVal:0);
            if(v.vt==VT_UNKNOWN&&v.punkVal)v.punkVal->Release();
        }
        metadata(manager,firstBorrowed,secondBorrowed);
        queries(manager,"empty");ok=save(persist,argv[2],"empty")&&ok;
        struct Case {const char* name;std::vector<unsigned char> bytes;};
        std::vector<Case> cases{
            {"four_four",input({{0,4,4,4}})}, {"three_eight",input({{0,3,8,4}})},
            {"five_four",input({{0,5,4,4}})}, {"zero_fields",input({{0,0,0,0}})},
            {"first_late",input({{3072,4,4,4}})},
            {"changes",input({{0,4,4,4},{3072,3,4,4},{7680,5,8,4}})},
            {"list_extended",input({{0,4,4,4},{3072,3,4,4}},12,true)},
            {"empty_chunk",input({})},
            {"first_unaligned",input({{1500,3,8,3}})},
            {"changes_unaligned",input({{0,4,4,4},{4100,3,4,4},{8000,5,8,4}})},
            {"duplicate_measure",input({{0,4,4,4},{0,3,4,2}})},
            {"unsorted",input({{6144,4,4,4},{0,3,4,4}})},
            {"negative_first",input({{-3072,4,4,4}})}
        };
        for(const auto& c:cases){
            std::wstring filename;for(const char* p=c.name;*p;++p)filename+=static_cast<wchar_t>(*p);filename+=L"-input.bin";
            ok=write_file(std::wstring(argv[2])+L"/"+filename,c.bytes)&&ok;
            IStream* in=stream(c.bytes);hr=in?persist->Load(in):E_OUTOFMEMORY;if(in)in->Release();
            std::printf("{\"operation\":\"load\",\"case\":\"%s\",\"hr\":\"0x%08lx\"}\n",c.name,static_cast<unsigned long>(hr));std::fflush(stdout);
            ok=SUCCEEDED(hr)&&ok;queries(manager,c.name);ok=save(persist,argv[2],c.name)&&ok;
            result("dirty_after_save",persist->IsDirty());
        }
        // Keep fake services disconnected during persistence: the original
        // may call additional Framework/runtime slots when loading data.
        std::printf("{\"operation\":\"begin_service_probe\",\"version\":1}\n");
        ok=services(manager,1,runtimeService,rejectedService)&&ok;
        ok=services(manager,2,frameworkService,rejectedService)&&ok;
        std::printf("{\"operation\":\"end_service_probe\"}\n");
    }
    if(persist)persist->Release();
    if(manager){const ULONG refs=manager->Release();std::printf("{\"operation\":\"release_manager\",\"remaining\":%lu}\n",refs);ok=(refs==0)&&ok;}
    std::printf("{\"operation\":\"borrowed_after_manager_release\",\"firstRefs\":%ld,\"firstAdds\":%u,\"firstReleases\":%u,\"secondRefs\":%ld,\"secondAdds\":%u,\"secondReleases\":%u}\n",firstBorrowed.refs,firstBorrowed.adds,firstBorrowed.releases,secondBorrowed.refs,secondBorrowed.adds,secondBorrowed.releases);
    service_result(1,"after_manager_release",S_OK,runtimeService,rejectedService);
    service_result(2,"after_manager_release",S_OK,frameworkService,rejectedService);
    ok=(runtimeService.refs==1&&frameworkService.refs==1&&rejectedService.refs==1)&&ok;
    hr=canUnload?canUnload():E_NOINTERFACE;result("can_unload",hr);ok=(hr==S_OK)&&ok;
    if(hr==S_OK)FreeLibrary(module);CoUninitialize();return ok?0:1;
}
