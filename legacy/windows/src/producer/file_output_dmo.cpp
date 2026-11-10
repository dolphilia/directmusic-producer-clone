#include "file_output_dmo.h"
#include <mediaerr.h>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cwchar>
namespace producer::app { namespace {
constexpr GUID audioType={0x73647561,0,0x10,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
constexpr GUID waveFormatType={0x05589f81,0xc356,0x11ce,{0xbf,1,0,0xaa,0,0x55,0x59,0x5a}};
GUID subtype(WORD tag){auto id=audioType;id.Data1=tag;return id;}
bool format(const DMO_MEDIA_TYPE* mt,WAVEFORMATEX& f){
    if(!mt||!IsEqualGUID(mt->majortype,audioType)||!IsEqualGUID(mt->formattype,waveFormatType)||
       !mt->pbFormat||mt->cbFormat<16||mt->pUnk)return false;
    f={};std::memcpy(&f,mt->pbFormat,std::min<std::size_t>(sizeof(f),mt->cbFormat));
    return FileOutputWriter::valid_format(f)&&IsEqualGUID(mt->subtype,subtype(f.wFormatTag));
}
bool same(const WAVEFORMATEX& a,const WAVEFORMATEX& b){return a.wFormatTag==b.wFormatTag&&a.nChannels==b.nChannels&&a.nSamplesPerSec==b.nSamplesPerSec&&a.wBitsPerSample==b.wBitsPerSample;}
HRESULT media_type(const WAVEFORMATEX& f,DMO_MEDIA_TYPE* mt){
    if(!mt)return E_POINTER;*mt={};mt->pbFormat=static_cast<BYTE*>(CoTaskMemAlloc(sizeof(f)));if(!mt->pbFormat)return E_OUTOFMEMORY;
    mt->majortype=audioType;mt->subtype=subtype(f.wFormatTag);mt->bFixedSizeSamples=TRUE;mt->lSampleSize=f.nBlockAlign;
    mt->formattype=waveFormatType;mt->cbFormat=sizeof(f);std::memcpy(mt->pbFormat,&f,sizeof(f));return S_OK;
}
class Output final:public IMediaObject,public IMediaObjectInPlace,public FileOutputControl {
    std::atomic<ULONG> refs_{1};std::recursive_mutex mutex_;FileOutputWriter writer_;
    WAVEFORMATEX input_{},output_{};bool hasInput_=false,hasOutput_=false;
    IMediaBuffer* pending_=nullptr;DWORD offset_=0,inputFlags_=0;REFERENCE_TIME timestamp_=0,length_=0;
    std::wstring filename_=L".\\dump.wav";DWORD option_=0;
    void flush(){if(pending_){pending_->Release();pending_=nullptr;}offset_=0;}
    HRESULT set_type(bool input,DWORD stream,const DMO_MEDIA_TYPE* mt,DWORD flags){
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;
        if(flags&~(DMO_SET_TYPEF_TEST_ONLY|DMO_SET_TYPEF_CLEAR))return E_INVALIDARG;
        if((flags&DMO_SET_TYPEF_CLEAR)&&(flags&DMO_SET_TYPEF_TEST_ONLY))return E_INVALIDARG;
        if(writer_.status().active||pending_)return DMO_E_NOTACCEPTING;
        if(flags&DMO_SET_TYPEF_CLEAR){(input?hasInput_:hasOutput_)=false;return S_OK;}
        WAVEFORMATEX f{};if(!format(mt,f))return DMO_E_TYPE_NOT_ACCEPTED;
        if((input?hasOutput_:hasInput_)&&!same(f,input?output_:input_))return DMO_E_TYPE_NOT_ACCEPTED;
        if(!(flags&DMO_SET_TYPEF_TEST_ONLY)){(input?input_:output_)=f;(input?hasInput_:hasOutput_)=true;}return S_OK;
    }
    HRESULT type(bool input,DWORD stream,DWORD index,DMO_MEDIA_TYPE* mt){
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;if(index)return DMO_E_NO_MORE_ITEMS;
        const bool exists=input?hasOutput_:hasInput_;const auto f=exists?(input?output_:input_):WAVEFORMATEX{WAVE_FORMAT_PCM,2,44100,176400,4,16,0};
        return mt?media_type(f,mt):S_OK;
    }
    HRESULT current(bool input,DWORD stream,DMO_MEDIA_TYPE* mt){std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;if(!(input?hasInput_:hasOutput_))return DMO_E_TYPE_NOT_SET;return media_type(input?input_:output_,mt);}
public:
    ~Output(){flush();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** p) override {if(!p)return E_POINTER;*p=nullptr;
        if(IsEqualGUID(id,IID_IUnknown)||IsEqualGUID(id,__uuidof(IMediaObject)))*p=static_cast<IMediaObject*>(this);
        else if(IsEqualGUID(id,__uuidof(IMediaObjectInPlace)))*p=static_cast<IMediaObjectInPlace*>(this);
        else if(IsEqualGUID(id,fileOutputControlId))*p=static_cast<FileOutputControl*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}ULONG STDMETHODCALLTYPE Release() override{const auto n=--refs_;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetStreamCount(DWORD* a,DWORD* b) override{if(!a||!b)return E_POINTER;*a=*b=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetInputStreamInfo(DWORD i,DWORD* f) override{if(i)return DMO_E_INVALIDSTREAMINDEX;if(!f)return E_POINTER;*f=DMO_INPUT_STREAMF_HOLDS_BUFFERS;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetOutputStreamInfo(DWORD i,DWORD* f) override{if(i)return DMO_E_INVALIDSTREAMINDEX;if(!f)return E_POINTER;*f=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetInputType(DWORD i,DWORD n,DMO_MEDIA_TYPE* mt) override{return type(true,i,n,mt);}
    HRESULT STDMETHODCALLTYPE GetOutputType(DWORD i,DWORD n,DMO_MEDIA_TYPE* mt) override{return type(false,i,n,mt);}
    HRESULT STDMETHODCALLTYPE SetInputType(DWORD i,const DMO_MEDIA_TYPE* mt,DWORD f) override{return set_type(true,i,mt,f);}
    HRESULT STDMETHODCALLTYPE SetOutputType(DWORD i,const DMO_MEDIA_TYPE* mt,DWORD f) override{return set_type(false,i,mt,f);}
    HRESULT STDMETHODCALLTYPE GetInputCurrentType(DWORD i,DMO_MEDIA_TYPE* mt) override{return current(true,i,mt);}
    HRESULT STDMETHODCALLTYPE GetOutputCurrentType(DWORD i,DMO_MEDIA_TYPE* mt) override{return current(false,i,mt);}
    HRESULT STDMETHODCALLTYPE GetInputSizeInfo(DWORD i,DWORD* size,DWORD* ahead,DWORD* align) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!size||!ahead||!align)return E_POINTER;if(!hasInput_)return DMO_E_TYPE_NOT_SET;*size=input_.nBlockAlign;*ahead=0;*align=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetOutputSizeInfo(DWORD i,DWORD* size,DWORD* align) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!size||!align)return E_POINTER;if(!hasOutput_)return DMO_E_TYPE_NOT_SET;*size=output_.nBlockAlign;*align=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetInputMaxLatency(DWORD i,REFERENCE_TIME* t) override{if(i)return DMO_E_INVALIDSTREAMINDEX;if(!t)return E_POINTER;*t=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE SetInputMaxLatency(DWORD i,REFERENCE_TIME t) override{return i?DMO_E_INVALIDSTREAMINDEX:t<0?E_INVALIDARG:S_OK;}
    HRESULT STDMETHODCALLTYPE Flush() override{std::lock_guard<std::recursive_mutex> lock(mutex_);flush();return S_OK;}
    HRESULT STDMETHODCALLTYPE Discontinuity(DWORD i) override{return i?DMO_E_INVALIDSTREAMINDEX:S_OK;}
    HRESULT STDMETHODCALLTYPE AllocateStreamingResources() override{return S_OK;}
    HRESULT STDMETHODCALLTYPE FreeStreamingResources() override{return Flush();}
    HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD i,DWORD* f) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!f)return E_POINTER;*f=hasInput_&&hasOutput_&&!pending_?DMO_INPUT_STATUSF_ACCEPT_DATA:0;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessInput(DWORD i,IMediaBuffer* b,DWORD flags,REFERENCE_TIME t,REFERENCE_TIME duration) override{
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!b)return E_POINTER;
        if(flags&~(DMO_INPUT_DATA_BUFFERF_SYNCPOINT|DMO_INPUT_DATA_BUFFERF_TIME|DMO_INPUT_DATA_BUFFERF_TIMELENGTH))return E_INVALIDARG;
        if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;if(pending_)return DMO_E_NOTACCEPTING;
        BYTE* bytes=nullptr;DWORD count=0;const auto hr=b->GetBufferAndLength(&bytes,&count);if(FAILED(hr))return hr;if((count&&!bytes)||count%input_.nBlockAlign)return E_INVALIDARG;
        // Record each accepted input once, independently of output fragmentation.
        const auto captured=writer_.append(bytes,count);if(FAILED(captured))return captured;
        b->AddRef();pending_=b;offset_=0;inputFlags_=flags;timestamp_=t;length_=duration;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD flags,DWORD count,DMO_OUTPUT_DATA_BUFFER* buffers,DWORD* status) override{
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(!status||!buffers)return E_POINTER;*status=0;if(flags||count!=1)return E_INVALIDARG;
        auto& out=buffers[0];out.dwStatus=0;if(!out.pBuffer)return E_POINTER;if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;
        if(!pending_){out.pBuffer->SetLength(0);return S_FALSE;}
        BYTE* src=nullptr;BYTE* dst=nullptr;DWORD size=0,unused=0,capacity=0;
        auto hr=pending_->GetBufferAndLength(&src,&size);if(FAILED(hr))return hr;hr=out.pBuffer->GetBufferAndLength(&dst,&unused);if(FAILED(hr))return hr;
        hr=out.pBuffer->GetMaxLength(&capacity);if(FAILED(hr))return hr;
        if(size<offset_||size%input_.nBlockAlign||(size&&!src))return E_INVALIDARG;
        const auto n=std::min(size-offset_,capacity/input_.nBlockAlign*input_.nBlockAlign);if(size>offset_&&!n)return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);if(n&&!dst)return E_POINTER;
        hr=out.pBuffer->SetLength(n);if(FAILED(hr))return hr;if(n)std::memcpy(dst,src+offset_,n);
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_TIME){out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_TIME;out.rtTimestamp=timestamp_+static_cast<REFERENCE_TIME>(std::uint64_t(offset_)*10000000/input_.nAvgBytesPerSec);}
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_TIMELENGTH){out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;out.rtTimelength=static_cast<REFERENCE_TIME>(std::uint64_t(n)*10000000/input_.nAvgBytesPerSec);}
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_SYNCPOINT)out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
        offset_+=n;if(offset_<size)out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;else flush();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Lock(LONG value) override{if(value)mutex_.lock();else mutex_.unlock();return S_OK;}
    HRESULT STDMETHODCALLTYPE Process(ULONG count,BYTE* data,REFERENCE_TIME,DWORD flags) override{
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(flags&~DMO_INPLACE_ZERO)return E_INVALIDARG;if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;
        if((count&&!data)||count%input_.nBlockAlign)return E_INVALIDARG;
        if(flags&DMO_INPLACE_ZERO){return S_FALSE;} // no tail; caller's scratch bytes are not input
        const auto hr=writer_.append(data,count);return FAILED(hr)?hr:S_OK;
    }
    HRESULT STDMETHODCALLTYPE Clone(IMediaObjectInPlace** result) override{if(!result)return E_POINTER;*result=nullptr;std::lock_guard<std::recursive_mutex> lock(mutex_);auto copy=new Output;copy->input_=input_;copy->output_=output_;copy->hasInput_=hasInput_;copy->hasOutput_=hasOutput_;copy->filename_=filename_;copy->option_=option_;*result=copy;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetLatency(REFERENCE_TIME* t) override{if(!t)return E_POINTER;*t=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE SetFilename(const WCHAR* name) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(!name||!*name)return E_INVALIDARG;if(writer_.status().active)return E_UNEXPECTED;filename_=name;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetFilename(WCHAR* name) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(!name)return E_POINTER;std::wcscpy(name,filename_.c_str());return S_OK;}
    HRESULT STDMETHODCALLTYPE GetOption(DWORD* option) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(!option)return E_POINTER;*option=option_;return S_OK;}
    HRESULT STDMETHODCALLTYPE SetOption(DWORD option) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(writer_.status().active)return E_UNEXPECTED;if(option)return E_NOTIMPL;option_=option;return S_OK;}
    HRESULT STDMETHODCALLTYPE Start() override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(writer_.status().active)return S_FALSE;if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;return writer_.start(filename_,input_);}
    HRESULT STDMETHODCALLTYPE Stop() override{std::lock_guard<std::recursive_mutex> lock(mutex_);return writer_.stop();}
};
class Factory final:public IClassFactory {
    std::atomic<ULONG> refs_{1};
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** p) override{if(!p)return E_POINTER;*p=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,IID_IClassFactory))return E_NOINTERFACE;*p=this;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}ULONG STDMETHODCALLTYPE Release() override{const auto n=--refs_;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID id,void** result) override{if(!result)return E_POINTER;*result=nullptr;if(outer)return CLASS_E_NOAGGREGATION;auto obj=new Output;const auto hr=obj->QueryInterface(id,result);obj->Release();return hr;}
    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override{return S_OK;}
};
}
IMediaObject* create_file_output_dmo(){return new Output;}
FileOutputRegistration::FileOutputRegistration(){auto factory=new Factory;const auto hr=CoRegisterClassObject(fileOutputRuntimeClass,factory,CLSCTX_INPROC_SERVER,REGCLS_MULTIPLEUSE,&cookie_);factory->Release();if(FAILED(hr))throw std::runtime_error("Source FileOutput process registration failed");}
FileOutputRegistration::~FileOutputRegistration(){if(cookie_)CoRevokeClassObject(cookie_);}
}
