#include <windows.h>
#include <objidl.h>
#include <cstdio>
#include <vector>
#include <string>
#include <initializer_list>
#include "compat/time_signature.h"
#include "compat/strip_manager.h"
#include "compat/producer_ids.h"
#include "reference_timeline.h"

// GUID_IDirectMusicStyle and IDirectMusicStyle::GetTimeSignature slot11:
// saved SDK dmusici.h, lines1798 and1485..1512. No runtime Style implementation.
static constexpr GUID styleParam={0xd2ac28a1,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
static void result(const char* operation,HRESULT hr){std::printf("{\"operation\":\"%s\",\"hr\":\"0x%08lx\"}\n",operation,static_cast<unsigned long>(hr));std::fflush(stdout);}
struct Style {
    void** table;void* slots[13]{};LONG refs=1;producer::TimeSignatureParam signature;
    explicit Style(producer::TimeSignatureParam value):table(slots),signature(value){
        for(auto& slot:slots)slot=reinterpret_cast<void*>(&unexpected);
        slots[0]=reinterpret_cast<void*>(&query);slots[1]=reinterpret_cast<void*>(&add);slots[2]=reinterpret_cast<void*>(&release);slots[11]=reinterpret_cast<void*>(&get_signature);
    }
    IUnknown* unknown(){return reinterpret_cast<IUnknown*>(this);}
    static HRESULT STDMETHODCALLTYPE unexpected(Style*){std::puts("{\"operation\":\"unexpected_style_slot\"}");std::fflush(stdout);ExitProcess(3);return E_NOTIMPL;}
    static HRESULT STDMETHODCALLTYPE query(Style* self,REFIID iid,void** out){if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown)return E_NOINTERFACE;*out=self;add(self);return S_OK;}
    static ULONG STDMETHODCALLTYPE add(Style* self){return ++self->refs;}
    static ULONG STDMETHODCALLTYPE release(Style* self){return --self->refs;}
    static HRESULT STDMETHODCALLTYPE get_signature(Style* self,producer::TimeSignatureParam* out){
        if(!out)return E_POINTER;*out=self->signature;
        std::printf("{\"operation\":\"style_signature\",\"beats\":%u,\"denominator\":%u,\"grids\":%u}\n",out->beats,out->denominator,out->grids);std::fflush(stdout);return S_OK;
    }
};
class StyleManager final:public producer::StripManager {
    LONG refs_=1;IUnknown* timeline_=nullptr;
public:
    Style first{{0,4,4,4}},second{{0,3,4,2}},third{{0,5,8,3}};
    bool empty=false;
    ~StyleManager(){if(timeline_)timeline_->Release();}
    LONG refs()const{return refs_;}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=producer::IID_IDMUSProdStripMgr)return E_NOINTERFACE;*out=static_cast<producer::StripManager*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override{return --refs_;} // Stack lifetime owned by this probe.
    HRESULT STDMETHODCALLTYPE IsParamSupported(REFGUID guid) override{return guid==styleParam||guid==producer::GUID_TimeSignatureParam?S_OK:S_FALSE;}
    HRESULT STDMETHODCALLTYPE GetParam(REFGUID guid,LONG time,LONG* next,void* out) override{
        if(guid==producer::GUID_TimeSignatureParam){
            if(!out)return E_POINTER;
            if(empty)return E_FAIL;
            Style* value=time<3072?&first:time<7680?&second:&third;
            *static_cast<producer::TimeSignatureParam*>(out)=value->signature;
            if(next)*next=time<3072?3072-time:time<7680?7680-time:0;
            return S_OK;
        }
        if(guid!=styleParam||!out)return E_INVALIDARG;
        *static_cast<IUnknown**>(out)=nullptr;if(next)*next=0;
        Style* value=time<3072?&first:time<7680?&second:&third;
        const LONG following=time<3072?3072-time:time<7680?7680-time:0;
        if(empty)value=nullptr;
        if(value){value->unknown()->AddRef();*static_cast<IUnknown**>(out)=value->unknown();if(next)*next=following;}
        std::printf("{\"operation\":\"style_get_param\",\"at\":%ld,\"next\":%ld,\"present\":%s,\"hr\":\"0x%08lx\"}\n",time,value?following:0,value?"true":"false",static_cast<unsigned long>(value?S_OK:E_FAIL));std::fflush(stdout);
        return value?S_OK:E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE SetParam(REFGUID,LONG,void*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OnUpdate(REFGUID,DWORD,void*) override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetStripMgrProperty(DWORD property,VARIANT* value) override{
        if(!value)return E_POINTER;
        if(property==3){if(value->vt!=VT_BYREF||!value->byref)return E_INVALIDARG;*static_cast<producer::TimeSignatureTrackHeader*>(value->byref)={GUID_NULL,0,1,0,0};return S_OK;}
        if(property==4||property==5){if(value->vt!=VT_BYREF||!value->byref)return E_INVALIDARG;*static_cast<DWORD*>(value->byref)=0;return S_OK;}
        if(property==0){value->vt=VT_UNKNOWN;value->punkVal=timeline_;if(timeline_)timeline_->AddRef();return timeline_?S_OK:E_FAIL;}
        return E_INVALIDARG;
    }
    HRESULT STDMETHODCALLTYPE SetStripMgrProperty(DWORD property,VARIANT value) override{
        if(property!=0||value.vt!=VT_UNKNOWN)return E_INVALIDARG;
        if(timeline_)timeline_->Release();timeline_=value.punkVal;if(timeline_)timeline_->AddRef();return S_OK;
    }
};
static std::vector<BYTE> input(LONG at,BYTE beats,BYTE denominator){
    std::vector<BYTE> bytes{'t','i','m','s',12,0,0,0,8,0,0,0};
    for(int i=0;i<4;++i)bytes.push_back(BYTE(DWORD(at)>>(8*i)));bytes.insert(bytes.end(),{beats,denominator,4,0});return bytes;
}
static bool write(const std::wstring& file,const std::vector<BYTE>& bytes){
    HANDLE handle=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(handle==INVALID_HANDLE_VALUE)return false;
    DWORD count=0;const bool ok=WriteFile(handle,bytes.data(),DWORD(bytes.size()),&count,nullptr)&&count==bytes.size();CloseHandle(handle);return ok;
}
static HRESULT load(IPersistStream* persist,const std::vector<BYTE>& bytes){
    IStream* stream=nullptr;HRESULT hr=CreateStreamOnHGlobal(nullptr,TRUE,&stream);if(FAILED(hr))return hr;ULONG count=0;
    hr=stream->Write(bytes.data(),ULONG(bytes.size()),&count);LARGE_INTEGER zero{};if(SUCCEEDED(hr))hr=stream->Seek(zero,STREAM_SEEK_SET,nullptr);if(SUCCEEDED(hr))hr=persist->Load(stream);stream->Release();return hr;
}
static bool observe(producer::StripManager* manager,IPersistStream* persist,const wchar_t* directory,const char* phase){
    for(LONG at:{-1L,0L,1L,3071L,3072L,3073L,5375L,5376L,7680L,9599L,9600L,20000L}){
        producer::TimeSignatureParam value{};LONG next=-1;const HRESULT hr=manager->GetParam(producer::GUID_TimeSignatureParam,at,&next,&value);
        std::printf("{\"operation\":\"meter_query\",\"phase\":\"%s\",\"at\":%ld,\"hr\":\"0x%08lx\",\"next\":%ld,\"time\":%ld,\"beats\":%u,\"denominator\":%u,\"grids\":%u}\n",phase,at,static_cast<unsigned long>(hr),next,value.time,value.beats,value.denominator,value.grids);
    }
    IStream* stream=nullptr;HRESULT hr=CreateStreamOnHGlobal(nullptr,TRUE,&stream);if(FAILED(hr))return false;hr=persist->Save(stream,FALSE);result("save_meter",hr);
    STATSTG stat{};if(SUCCEEDED(hr))hr=stream->Stat(&stat,STATFLAG_NONAME);LARGE_INTEGER zero{};if(SUCCEEDED(hr))hr=stream->Seek(zero,STREAM_SEEK_SET,nullptr);
    std::vector<BYTE> bytes(SUCCEEDED(hr)?size_t(stat.cbSize.QuadPart):0);ULONG count=0;if(SUCCEEDED(hr))hr=stream->Read(bytes.data(),ULONG(bytes.size()),&count);stream->Release();
    std::wstring name;for(const char* p=phase;*p;++p)name+=wchar_t(*p);
    const bool ok=SUCCEEDED(hr)&&count==bytes.size()&&write(std::wstring(directory)+L"/"+name+L"-saved.bin",bytes);
    std::printf("{\"operation\":\"saved_meter\",\"phase\":\"%s\",\"bytes\":%zu,\"written\":%s}\n",phase,bytes.size(),ok?"true":"false");std::fflush(stdout);return ok;
}
int wmain(int argc,wchar_t** argv){
    if(argc!=4)return 2;HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);result("initialize",hr);if(FAILED(hr))return 1;
    HMODULE module=LoadLibraryExW(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!module){result("load_module",HRESULT_FROM_WIN32(GetLastError()));CoUninitialize();return 1;}
    using GetClass=HRESULT(STDAPICALLTYPE*)(REFCLSID,REFIID,void**);using CanUnload=HRESULT(STDAPICALLTYPE*)();
    const auto getClass=reinterpret_cast<GetClass>(GetProcAddress(module,"DllGetClassObject"));const auto canUnload=reinterpret_cast<CanUnload>(GetProcAddress(module,"DllCanUnloadNow"));bool ok=true;
    struct Case{const char* name;bool seeded;LONG at;BYTE beats,denominator;};
    for(const auto& test:std::initializer_list<Case>{{"style_only",false,0,0,0},{"explicit_first",true,0,7,4},{"explicit_late",true,6144,5,8}}){
        std::printf("{\"operation\":\"begin_style_case\",\"case\":\"%s\"}\n",test.name);std::fflush(stdout);
        ReferenceTimeline timeline;hr=timeline.load(argv[1],argv[3]);result("load_real_timeline",hr);if(FAILED(hr)){ok=false;break;}
        IClassFactory* factory=nullptr;producer::StripManager* manager=nullptr;IPersistStream* persist=nullptr;StyleManager styles;
        hr=getClass(producer::CLSID_TimeSignatureMgr,IID_IClassFactory,reinterpret_cast<void**>(&factory));if(SUCCEEDED(hr)){hr=factory->CreateInstance(nullptr,producer::IID_IDMUSProdStripMgr,reinterpret_cast<void**>(&manager));factory->Release();}result("create_manager",hr);if(FAILED(hr)){ok=false;break;}
        hr=manager->QueryInterface(IID_IPersistStream,reinterpret_cast<void**>(&persist));result("query_persist",hr);
        if(SUCCEEDED(hr)&&test.seeded){auto bytes=input(test.at,test.beats,test.denominator);std::wstring name;for(const char* p=test.name;*p;++p)name+=wchar_t(*p);ok=write(std::wstring(argv[2])+L"/"+name+L"-input.bin",bytes)&&ok;hr=load(persist,bytes);result("load_explicit_meter",hr);}
        bool styleInserted=false,meterInserted=false;
        if(SUCCEEDED(hr)){hr=timeline.insert(&styles);result("insert_style_manager",hr);styleInserted=SUCCEEDED(hr);}
        if(SUCCEEDED(hr)){hr=timeline.insert(manager);result("insert_meter_manager",hr);meterInserted=SUCCEEDED(hr);}
        if(SUCCEEDED(hr))ok=observe(manager,persist,argv[2],test.name)&&ok;else ok=false;
        IUnknown* strip=nullptr;if(meterInserted){hr=timeline.find_manager_strip(manager,&strip);result("find_meter_strip",hr);if(strip)strip->Release();ok=SUCCEEDED(hr)&&ok;hr=timeline.remove(manager);result("remove_meter_manager",hr);ok=SUCCEEDED(hr)&&ok;}
        if(styleInserted){hr=timeline.remove(&styles);result("remove_style_manager",hr);ok=SUCCEEDED(hr)&&ok;}
        if(persist)persist->Release();const ULONG remaining=manager->Release();std::printf("{\"operation\":\"release_manager\",\"remaining\":%lu}\n",remaining);
        hr=timeline.close();result("timeline_can_unload",hr);ok=hr==S_OK&&remaining==0&&ok;
        std::printf("{\"operation\":\"style_final_refs\",\"manager\":%ld,\"first\":%ld,\"second\":%ld,\"third\":%ld}\n",styles.refs(),styles.first.refs,styles.second.refs,styles.third.refs);
        ok=styles.refs()==1&&styles.first.refs==1&&styles.second.refs==1&&styles.third.refs==1&&ok;
        std::puts("{\"operation\":\"end_style_case\"}");std::fflush(stdout);
    }
    hr=canUnload?canUnload():E_NOINTERFACE;result("meter_can_unload",hr);ok=hr==S_OK&&ok;if(hr==S_OK)FreeLibrary(module);CoUninitialize();return ok?0:1;
}
