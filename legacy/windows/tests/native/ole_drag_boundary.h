#pragma once
#include <windows.h>
#include <oleidl.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include "runtime_tempo.h"

namespace ole_boundary {
struct Context {
    IUnknown* strip=nullptr;
    IUnknown* runtime=nullptr;
    IDataObject* retained=nullptr;
    unsigned calls=0;
    DWORD returnedEffect=0;
    HRESULT returnedResult=DRAGDROP_S_CANCEL;
    bool dropToSelf=false;
    bool realLoop=false;
    LONG dropX=384;
    bool passed=true;
    std::vector<unsigned char> bytes;
    ~Context(){if(retained)retained->Release();}
};
inline Context* active=nullptr;
struct Wake {DWORD thread;bool posted=false;};
inline DWORD WINAPI wake_cancel_loop(void* argument){
    Sleep(50);
    auto& state=*static_cast<Wake*>(argument);
    state.posted=PostThreadMessageW(state.thread,WM_KEYDOWN,VK_ESCAPE,0)!=FALSE;
    return state.posted?0:1;
}
// Let Windows run its OLE loop, but cancel at its first continuation callback.
// No physical input or foreign drop target is needed for this cancellation test.
class CancelSource final : public IDropSource {
    IDropSource* source_;
    ULONG references_=1;
public:
    unsigned continuations=0;
    explicit CancelSource(IDropSource* source):source_(source){source_->AddRef();}
    ~CancelSource(){source_->Release();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(iid!=IID_IUnknown&&iid!=IID_IDropSource)return E_NOINTERFACE;
        *out=static_cast<IDropSource*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return ++references_;}
    ULONG STDMETHODCALLTYPE Release() override{return --references_;}
    ULONG references() const{return references_;}
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL,DWORD keys) override {
        ++continuations;const auto hr=source_->QueryContinueDrag(TRUE,keys);
        std::printf("{\"operation\":\"drag_start_real_continue\",\"forced_escape\":true,\"hresult\":\"0x%08lx\"}\n",hr);return hr;
    }
    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD effect) override{return source_->GiveFeedback(effect);}
};
inline bool read_data(IDataObject* data,std::vector<unsigned char>& bytes,bool report=false) {
    FORMATETC format{static_cast<CLIPFORMAT>(RegisterClipboardFormatA("Jazz v.1 Tempolist")),nullptr,DVASPECT_CONTENT,-1,TYMED_ISTREAM};
    STGMEDIUM medium{};HRESULT hr=data->GetData(&format,&medium);
    bool ok=hr==S_OK&&medium.tymed==TYMED_ISTREAM&&medium.pstm;
    if(ok){LARGE_INTEGER zero{};ULARGE_INTEGER cursor{};hr=medium.pstm->Seek(zero,STREAM_SEEK_CUR,&cursor);
        if(report)std::printf("{\"operation\":\"drag_start_stream_cursor\",\"hresult\":\"0x%08lx\",\"position\":%llu}\n",hr,cursor.QuadPart);
        STATSTG stat{};hr=medium.pstm->Stat(&stat,STATFLAG_NONAME);ok=hr==S_OK&&stat.cbSize.QuadPart<=1024*1024;
        if(ok){bytes.resize(static_cast<size_t>(stat.cbSize.QuadPart));ULONG count=0;
            hr=medium.pstm->Seek(zero,STREAM_SEEK_SET,nullptr);
            if(SUCCEEDED(hr))hr=medium.pstm->Read(bytes.data(),static_cast<ULONG>(bytes.size()),&count);
            ok=SUCCEEDED(hr)&&count==bytes.size();}}
    if(medium.tymed)ReleaseStgMedium(&medium);
    return ok;
}
inline HRESULT WINAPI observe(IDataObject* data,IDropSource* source,DWORD allowed,DWORD* effect) {
    if(!active||!data||!source||!effect)return E_UNEXPECTED;
    auto& state=*active;++state.calls;
    std::printf("{\"operation\":\"drag_start_boundary\",\"allowed\":%lu}\n",allowed);
    IUnknown* identity=nullptr;const auto hr=source->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&identity));
    const bool same=hr==S_OK&&identity==state.strip;
    std::printf("{\"operation\":\"drag_start_identity\",\"same\":%s}\n",same?"true":"false");
    if(identity)identity->Release();state.passed=same&&state.passed;
    for(BOOL escape:{FALSE,TRUE})for(DWORD keys:{0UL,1UL,2UL,3UL,4UL,8UL,9UL,10UL,16UL,256UL}){
        const auto continued=source->QueryContinueDrag(escape,keys);
        std::printf("{\"operation\":\"drag_start_continue\",\"escape\":%ld,\"keys\":%lu,\"hresult\":\"0x%08lx\"}\n",escape,keys,continued);
    }
    const auto feedback=source->GiveFeedback(allowed);
    std::printf("{\"operation\":\"drag_start_feedback\",\"hresult\":\"0x%08lx\"}\n",feedback);
    state.passed=feedback==DRAGDROP_S_USEDEFAULTCURSORS&&state.passed;
    IEnumFORMATETC* formats=nullptr;auto enumeration=data->EnumFormatEtc(DATADIR_GET,&formats);
    std::printf("{\"operation\":\"drag_start_enum\",\"hresult\":\"0x%08lx\"}\n",enumeration);
    if(formats){FORMATETC format{};ULONG count=0;enumeration=formats->Next(1,&format,&count);
        const bool named=format.cfFormat==RegisterClipboardFormatA("Jazz v.1 Tempolist");
        std::printf("{\"operation\":\"drag_start_format\",\"hresult\":\"0x%08lx\",\"count\":%lu,\"tempo\":%s,\"medium\":%lu,\"aspect\":%lu,\"index\":%ld}\n",enumeration,count,named?"true":"false",format.tymed,format.dwAspect,format.lindex);
        if(format.ptd)CoTaskMemFree(format.ptd);
        enumeration=formats->Next(1,&format,&count);
        std::printf("{\"operation\":\"drag_start_enum_end\",\"hresult\":\"0x%08lx\",\"count\":%lu}\n",enumeration,count);
        formats->Release();}
    for(DWORD medium:{4UL,1UL}){FORMATETC format{static_cast<CLIPFORMAT>(RegisterClipboardFormatA("Jazz v.1 Tempolist")),nullptr,DVASPECT_CONTENT,-1,medium};
        const auto queried=data->QueryGetData(&format);
        std::printf("{\"operation\":\"drag_start_query_format\",\"medium\":%lu,\"hresult\":\"0x%08lx\"}\n",medium,queried);}
    FORMATETC streamFormat{static_cast<CLIPFORMAT>(RegisterClipboardFormatA("Jazz v.1 Tempolist")),nullptr,DVASPECT_CONTENT,-1,TYMED_ISTREAM};
    STGMEDIUM first{},second{};auto firstResult=data->GetData(&streamFormat,&first);
    if(firstResult==S_OK&&first.tymed==TYMED_ISTREAM&&first.pstm){
        LARGE_INTEGER one{};one.QuadPart=1;first.pstm->Seek(one,STREAM_SEEK_SET,nullptr);
        const auto secondResult=data->GetData(&streamFormat,&second);LARGE_INTEGER zero{};ULARGE_INTEGER cursor{};
        if(secondResult==S_OK&&second.tymed==TYMED_ISTREAM&&second.pstm){second.pstm->Seek(zero,STREAM_SEEK_CUR,&cursor);
            std::printf("{\"operation\":\"drag_start_shared_stream\",\"same\":%s,\"position\":%llu}\n",first.pstm==second.pstm?"true":"false",cursor.QuadPart);}
        STATSTG stat{};first.pstm->Stat(&stat,STATFLAG_NONAME);LARGE_INTEGER end{};end.QuadPart=static_cast<LONGLONG>(stat.cbSize.QuadPart);first.pstm->Seek(end,STREAM_SEEK_SET,nullptr);
    }
    if(first.tymed)ReleaseStgMedium(&first);if(second.tymed)ReleaseStgMedium(&second);
    const bool read=read_data(data,state.bytes,true);state.passed=read&&state.passed;
    std::printf("{\"operation\":\"drag_start_data\",\"read\":%s,\"size\":%zu}\n",read?"true":"false",state.bytes.size());
    if(!state.retained){data->AddRef();state.retained=data;}
    *effect=state.returnedEffect;
    if(state.dropToSelf){IDropTarget* target=nullptr;auto dropResult=state.strip->QueryInterface(IID_IDropTarget,reinterpret_cast<void**>(&target));
        const DWORD keys=MK_LBUTTON|(state.returnedEffect==DROPEFFECT_COPY?MK_CONTROL:0);
        DWORD chosen=allowed;
        if(SUCCEEDED(dropResult))dropResult=target->DragEnter(data,keys,{state.dropX,0},&chosen);
        if(SUCCEEDED(dropResult))dropResult=target->Drop(data,0,{state.dropX,0},&chosen);
        std::printf("{\"operation\":\"drag_start_self_drop\",\"x\":%ld,\"effect\":%lu,\"hresult\":\"0x%08lx\"}\n",state.dropX,chosen,dropResult);
        if(state.runtime){LONG next=0;runtime_tempo::TempoParam value{};
            const auto runtimeResult=runtime_tempo::get(state.runtime,3000,&next,&value);
            std::printf("{\"operation\":\"drag_start_drop_runtime\",\"hresult\":\"0x%08lx\",\"at\":3000,\"time\":%ld,\"tempo\":%.17g,\"next\":%ld}\n",runtimeResult,value.time,value.tempo,next);}
        state.passed=dropResult==S_OK&&state.passed;*effect=chosen;
        if(target)target->Release();}
    if(state.realLoop){CancelSource cancelling(source);*effect=0;
        Wake wakeState{GetCurrentThreadId()};
        const auto wake=CreateThread(nullptr,0,wake_cancel_loop,&wakeState,0,nullptr);
        if(!wake){state.passed=false;return E_FAIL;}
        std::fflush(stdout);
        const auto realResult=DoDragDrop(data,&cancelling,allowed,effect);
        WaitForSingleObject(wake,INFINITE);CloseHandle(wake);
        std::printf("{\"operation\":\"drag_start_real_wake\",\"owned_thread\":true,\"posted\":%s}\n",wakeState.posted?"true":"false");
        std::printf("{\"operation\":\"drag_start_real_loop\",\"hresult\":\"0x%08lx\",\"effect\":%lu,\"continuations\":%u,\"source_refs\":%lu}\n",realResult,*effect,cancelling.continuations,cancelling.references());
        state.passed=wakeState.posted&&realResult==DRAGDROP_S_CANCEL&&*effect==0&&cancelling.continuations>0&&cancelling.references()==1&&state.passed;
        return realResult;
    }
    return state.returnedResult;
}
// Only replaces the loaded module's named ole32!DoDragDrop import for the
// duration of this test. No instructions or on-disk DLL bytes are modified.
class Hook {
    ULONG_PTR* cell_=nullptr;
    ULONG_PTR previous_=0;
public:
    explicit Hook(HMODULE module,const char* library="ole32.dll",const char* function="DoDragDrop",
                  ULONG_PTR replacement=reinterpret_cast<ULONG_PTR>(observe)) {
        auto base=reinterpret_cast<unsigned char*>(module);
        const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if(!module||dos->e_magic!=IMAGE_DOS_SIGNATURE)return;
        const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)return;
        const auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if(!directory.VirtualAddress)return;
        auto imports=reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base+directory.VirtualAddress);
        for(;imports->Name;++imports){
            if(_stricmp(reinterpret_cast<const char*>(base+imports->Name),library)||!imports->OriginalFirstThunk)continue;
            auto names=reinterpret_cast<const IMAGE_THUNK_DATA32*>(base+imports->OriginalFirstThunk);
            auto values=reinterpret_cast<IMAGE_THUNK_DATA32*>(base+imports->FirstThunk);
            for(;names->u1.AddressOfData;++names,++values){
                if(IMAGE_SNAP_BY_ORDINAL32(names->u1.Ordinal))continue;
                const auto entry=reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base+names->u1.AddressOfData);
                if(std::strcmp(reinterpret_cast<const char*>(entry->Name),function))continue;
                auto cell=reinterpret_cast<ULONG_PTR*>(&values->u1.Function);DWORD prior=0;
                if(!VirtualProtect(cell,sizeof(*cell),PAGE_READWRITE,&prior))return;
                previous_=*cell;*cell=replacement;DWORD discarded=0;
                const bool protectedAgain=VirtualProtect(cell,sizeof(*cell),prior,&discarded)!=FALSE;
                cell_=cell;if(!protectedAgain)restore();return;
            }
        }
    }
    ~Hook(){restore();}
    bool installed() const{return cell_!=nullptr;}
    bool restore(){
        if(!cell_)return true;DWORD prior=0;
        if(!VirtualProtect(cell_,sizeof(*cell_),PAGE_READWRITE,&prior))return false;
        *cell_=previous_;DWORD discarded=0;
        const bool ok=VirtualProtect(cell_,sizeof(*cell_),prior,&discarded)!=FALSE;
        cell_=nullptr;return ok;
    }
};
}
