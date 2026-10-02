#pragma once
#include <oleidl.h>
#include <vector>
#include <cstring>

namespace drag_probe {
// Read-only, in-process OLE transfer. No system clipboard or physical input.
class Formats final : public IEnumFORMATETC {
    ULONG refs_=1,index_=0;
    FORMATETC format_;
public:
    explicit Formats(FORMATETC format):format_(format){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(iid!=IID_IUnknown&&iid!=IID_IEnumFORMATETC)return E_NOINTERFACE;
        *out=this;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override{auto n=--refs_;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Next(ULONG count,FORMATETC* out,ULONG* fetched) override {
        if(!out||(!fetched&&count!=1))return E_POINTER;
        ULONG n=0;if(count&&index_==0){*out=format_;++index_;n=1;}if(fetched)*fetched=n;
        return n==count?S_OK:S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Skip(ULONG count) override{const ULONG remaining=1-index_;index_=1;return count<=remaining?S_OK:S_FALSE;}
    HRESULT STDMETHODCALLTYPE Reset() override{index_=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** out) override{
        if(!out)return E_POINTER;auto copy=new Formats(format_);copy->index_=index_;*out=copy;return S_OK;
    }
};
class Data final : public IDataObject {
public:
    ULONG refs=1;
    bool supported=true;
    DWORD medium=TYMED_ISTREAM;
    HRESULT queryResult=S_OK;
    std::vector<unsigned char> bytes;
    explicit Data(std::vector<unsigned char> value,bool valid=true,DWORD storage=TYMED_ISTREAM):supported(valid),medium(storage),bytes(std::move(value)){}
    FORMATETC format() const {return {static_cast<CLIPFORMAT>(RegisterClipboardFormatA(supported?"Jazz v.1 Tempolist":"Producer probe unsupported")),nullptr,DVASPECT_CONTENT,-1,medium};}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=IID_IDataObject)return E_NOINTERFACE;
        *out=this;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override{return --refs;}
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* f) override {
        if(!f)return E_POINTER;
        return f->cfFormat==format().cfFormat&&(f->tymed&medium)?queryResult:DV_E_FORMATETC;
    }
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* f,STGMEDIUM* out) override {
        if(!out)return E_POINTER;*out={};const auto hr=QueryGetData(f);if(FAILED(hr))return hr;
        if(medium==TYMED_HGLOBAL){auto memory=GlobalAlloc(GMEM_MOVEABLE,bytes.size());if(!memory)return E_OUTOFMEMORY;
            auto dest=GlobalLock(memory);if(!dest){GlobalFree(memory);return E_OUTOFMEMORY;}
            std::memcpy(dest,bytes.data(),bytes.size());GlobalUnlock(memory);out->tymed=TYMED_HGLOBAL;out->hGlobal=memory;return S_OK;}
        IStream* stream=nullptr;auto result=CreateStreamOnHGlobal(nullptr,TRUE,&stream);if(FAILED(result))return result;
        ULONG written=0;result=stream->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&written);
        if(SUCCEEDED(result)&&written!=bytes.size())result=E_FAIL;
        if(SUCCEEDED(result)){LARGE_INTEGER zero{};result=stream->Seek(zero,STREAM_SEEK_SET,nullptr);}
        if(FAILED(result)){stream->Release();return result;}
        out->tymed=TYMED_ISTREAM;out->pstm=stream;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*,STGMEDIUM*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*,FORMATETC* out) override{if(out)out->ptd=nullptr;return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC*,STGMEDIUM*,BOOL) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction,IEnumFORMATETC** out) override{
        if(!out)return E_POINTER;*out=nullptr;if(direction!=DATADIR_GET)return E_NOTIMPL;
        *out=new Formats(format());return S_OK;
    }
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*,DWORD,IAdviseSink*,DWORD*) override{return OLE_E_ADVISENOTSUPPORTED;}
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override{return OLE_E_ADVISENOTSUPPORTED;}
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override{return OLE_E_ADVISENOTSUPPORTED;}
};
inline bool callbacks(IUnknown* strip) {
    std::puts("{\"operation\":\"begin_drag_callback_probe\"}");
    using Message=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
    const auto created=reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6])(strip,WM_CREATE,0,0,0,0);
    std::printf("{\"operation\":\"drag_initialize_strip\",\"hresult\":\"0x%08lx\"}\n",created);
    IDropSource* source=nullptr;IDropTarget* target=nullptr;
    auto hr=strip->QueryInterface(IID_IDropSource,reinterpret_cast<void**>(&source));
    std::printf("{\"operation\":\"drag_query_source\",\"hresult\":\"0x%08lx\"}\n",hr);
    hr=strip->QueryInterface(IID_IDropTarget,reinterpret_cast<void**>(&target));
    std::printf("{\"operation\":\"drag_query_target\",\"hresult\":\"0x%08lx\"}\n",hr);
    if(!source||!target){if(source)source->Release();if(target)target->Release();return false;}
    bool ok=true;
    for(auto object:{static_cast<IUnknown*>(source),static_cast<IUnknown*>(target)}) {
        IUnknown* identity=nullptr;hr=object->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&identity));
        ok=hr==S_OK&&identity==strip&&ok;
        std::printf("{\"operation\":\"drag_identity\",\"same\":%s}\n",identity==strip?"true":"false");
        if(identity)identity->Release();
    }
    for(BOOL escape:{FALSE,TRUE})for(DWORD keys:{0UL,1UL,2UL,3UL,4UL,8UL,9UL,10UL,16UL,256UL}) {
        hr=source->QueryContinueDrag(escape,keys);
        std::printf("{\"operation\":\"drag_continue_initial\",\"escape\":%ld,\"keys\":%lu,\"hresult\":\"0x%08lx\"}\n",escape,keys,hr);
    }
    for(DWORD effect:{0UL,1UL,2UL,3UL,4UL}) {
        hr=source->GiveFeedback(effect);
        std::printf("{\"operation\":\"drag_feedback\",\"effect\":%lu,\"hresult\":\"0x%08lx\"}\n",effect,hr);
        ok=hr==DRAGDROP_S_USEDEFAULTCURSORS&&ok;
    }
    std::vector<unsigned char> bytes(36,0);std::memcpy(bytes.data(),"tetr",4);
    const DWORD payload=28,record=24;const LONG time=132,tick=132;const double tempo=137;
    std::memcpy(bytes.data()+4,&payload,4);std::memcpy(bytes.data()+8,&record,4);
    std::memcpy(bytes.data()+12,&time,4);std::memcpy(bytes.data()+20,&tempo,8);std::memcpy(bytes.data()+28,&tick,4);
    for(DWORD medium:{static_cast<DWORD>(TYMED_ISTREAM),static_cast<DWORD>(TYMED_HGLOBAL)})for(bool valid:{true,false})for(LONG x:{192L,-1L})for(DWORD keys:{1UL,9UL,2UL})for(DWORD allowed:{0UL,1UL,2UL,3UL,4UL}) {
        Data data(bytes,valid,medium);DWORD effect=allowed;
        hr=target->DragEnter(&data,keys,{x,0},&effect);
        std::printf("{\"operation\":\"drag_enter\",\"medium\":%lu,\"valid\":%s,\"x\":%ld,\"keys\":%lu,\"allowed\":%lu,\"effect\":%lu,\"hresult\":\"0x%08lx\",\"refs\":%lu}\n",medium,valid?"true":"false",x,keys,allowed,effect,hr,data.refs);
        ok=hr==S_OK&&data.refs==2&&ok;
        effect=allowed;hr=target->DragOver(keys,{x,0},&effect);
        std::printf("{\"operation\":\"drag_over\",\"effect\":%lu,\"hresult\":\"0x%08lx\",\"refs\":%lu}\n",effect,hr,data.refs);
        ok=hr==S_OK&&ok;
        hr=target->DragLeave();
        std::printf("{\"operation\":\"drag_leave\",\"hresult\":\"0x%08lx\",\"refs\":%lu}\n",hr,data.refs);
        ok=hr==S_OK&&data.refs==1&&ok;
    }
    source->Release();target->Release();
    std::printf("{\"operation\":\"end_drag_callback_probe\",\"passed\":%s}\n",ok?"true":"false");std::fflush(stdout);
    return ok;
}
}
