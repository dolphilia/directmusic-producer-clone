#pragma once
#include <windows.h>
#include <objidl.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <new>

namespace producer::tempo {
class TransferFormats final : public IEnumFORMATETC {
    LONG refs_=1;
    LONG* live_;
    ULONG index_=0;
    FORMATETC format_;
public:
    TransferFormats(FORMATETC format,LONG* live):live_(live),format_(format){InterlockedIncrement(live_);}
    ~TransferFormats(){InterlockedDecrement(live_);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override{
        if(!out)return E_POINTER;*out=nullptr;
        if(iid!=IID_IUnknown&&iid!=IID_IEnumFORMATETC)return E_NOINTERFACE;
        *out=static_cast<IEnumFORMATETC*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return static_cast<ULONG>(InterlockedIncrement(&refs_));}
    ULONG STDMETHODCALLTYPE Release() override{const auto n=InterlockedDecrement(&refs_);if(!n)delete this;return static_cast<ULONG>(n);}
    HRESULT STDMETHODCALLTYPE Next(ULONG count,FORMATETC* out,ULONG* fetched) override{
        if(!out||(!fetched&&count!=1))return E_POINTER;
        ULONG n=0;if(count&&!index_){*out=format_;index_=1;n=1;}if(fetched)*fetched=n;
        return n==count?S_OK:S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Skip(ULONG count) override{const auto remaining=1-index_;index_=1;return count<=remaining?S_OK:S_FALSE;}
    HRESULT STDMETHODCALLTYPE Reset() override{index_=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** out) override{
        if(!out)return E_POINTER;*out=nullptr;
        auto copy=new(std::nothrow) TransferFormats(format_,live_);if(!copy)return E_OUTOFMEMORY;
        copy->index_=index_;*out=copy;return S_OK;
    }
};
class TempoDataObject final : public IDataObject {
    LONG refs_=1;
    LONG* live_;
    CLIPFORMAT format_;
    std::vector<std::uint8_t> bytes_;
public:
    TempoDataObject(CLIPFORMAT format,std::vector<std::uint8_t> bytes,LONG* live):live_(live),format_(format),bytes_(std::move(bytes)){InterlockedIncrement(live_);}
    ~TempoDataObject(){InterlockedDecrement(live_);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override{
        if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=IID_IDataObject)return E_NOINTERFACE;
        *out=static_cast<IDataObject*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override{return static_cast<ULONG>(InterlockedIncrement(&refs_));}
    ULONG STDMETHODCALLTYPE Release() override{const auto n=InterlockedDecrement(&refs_);if(!n)delete this;return static_cast<ULONG>(n);}
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* format) override{
        if(!format)return E_POINTER;
        return format->cfFormat==format_&&(format->tymed&TYMED_ISTREAM)?S_OK:DV_E_FORMATETC;
    }
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* format,STGMEDIUM* out) override{
        if(!out)return E_POINTER;*out={};const auto checked=QueryGetData(format);if(FAILED(checked))return checked;
        IStream* stream=nullptr;auto hr=CreateStreamOnHGlobal(nullptr,TRUE,&stream);if(FAILED(hr))return hr;
        ULONG written=0;hr=stream->Write(bytes_.data(),static_cast<ULONG>(bytes_.size()),&written);
        if(SUCCEEDED(hr)&&written!=bytes_.size())hr=E_FAIL;
        // Original COleDataSource returns independent streams at the saved
        // cursor (the end after exporting the tetr payload), not rewound.
        if(FAILED(hr)){stream->Release();return hr;}
        out->tymed=TYMED_ISTREAM;out->pstm=stream;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*,STGMEDIUM*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*,FORMATETC* out) override{if(out)out->ptd=nullptr;return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC*,STGMEDIUM*,BOOL) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction,IEnumFORMATETC** out) override{
        if(!out)return E_POINTER;*out=nullptr;if(direction!=DATADIR_GET)return E_NOTIMPL;
        FORMATETC format{format_,nullptr,DVASPECT_CONTENT,-1,TYMED_ISTREAM};
        *out=new(std::nothrow) TransferFormats(format,live_);return *out?S_OK:E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*,DWORD,IAdviseSink*,DWORD*) override{return OLE_E_ADVISENOTSUPPORTED;}
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override{return OLE_E_ADVISENOTSUPPORTED;}
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override{return OLE_E_ADVISENOTSUPPORTED;}
};
}
