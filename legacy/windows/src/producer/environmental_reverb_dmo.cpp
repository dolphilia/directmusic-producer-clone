#include "environmental_reverb_dmo.h"
#include "file_output_writer.h"
#include <dsound.h>
#include <xaudio2fx.h>
#include <xapo.h>
#include <mutex>
#include <vector>
#include <cmath>
#include <mediaerr.h>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <new>
#include <cwchar>
namespace producer::app { namespace {
constexpr GUID audioType={0x73647561,0,0x10,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
constexpr GUID waveFormatType={0x05589f81,0xc356,0x11ce,{0xbf,1,0,0xaa,0,0x55,0x59,0x5a}};
GUID subtype(WORD tag){auto id=audioType;id.Data1=tag;return id;}

constexpr EnvironmentalReverbParameters defaults{
    DSFX_I3DL2REVERB_ROOM_DEFAULT,DSFX_I3DL2REVERB_ROOMHF_DEFAULT,DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_DEFAULT,
    DSFX_I3DL2REVERB_DECAYTIME_DEFAULT,DSFX_I3DL2REVERB_DECAYHFRATIO_DEFAULT,DSFX_I3DL2REVERB_REFLECTIONS_DEFAULT,
    DSFX_I3DL2REVERB_REFLECTIONSDELAY_DEFAULT,DSFX_I3DL2REVERB_REVERB_DEFAULT,DSFX_I3DL2REVERB_REVERBDELAY_DEFAULT,
    DSFX_I3DL2REVERB_DIFFUSION_DEFAULT,DSFX_I3DL2REVERB_DENSITY_DEFAULT,DSFX_I3DL2REVERB_HFREFERENCE_DEFAULT};
class ReverbEngine {
    IXAPO* apo_=nullptr;IXAPOParameters* parameters_=nullptr;bool locked_=false,defaultsPending_=false,hasSignal_=false;WAVEFORMATEX floatFormat_{};XAUDIO2FX_REVERB_PARAMETERS native_{};
public:
    static constexpr DWORD frames=4096;
    std::vector<float> input,output;
    ~ReverbEngine(){clear();}
    void clear(){if(apo_&&locked_)apo_->UnlockForProcess();locked_=false;defaultsPending_=false;hasSignal_=false;if(parameters_)parameters_->Release();parameters_=nullptr;if(apo_)apo_->Release();apo_=nullptr;}
    bool ready()const{return locked_;}
    void reset(){if(apo_)apo_->Reset();hasSignal_=false;}
    HRESULT prepare(const WAVEFORMATEX& format){
        if(apo_&&locked_)return S_OK;clear();IUnknown* raw=nullptr;auto hr=XAudio2CreateReverb(&raw);if(FAILED(hr))return hr;
        hr=raw->QueryInterface(__uuidof(IXAPO),reinterpret_cast<void**>(&apo_));if(SUCCEEDED(hr))hr=raw->QueryInterface(__uuidof(IXAPOParameters),reinterpret_cast<void**>(&parameters_));raw->Release();if(FAILED(hr)){clear();return hr;}
        hr=apo_->Initialize(nullptr,0);if(FAILED(hr)){clear();return hr;}
        const XAUDIO2FX_REVERB_I3DL2_PARAMETERS i3dl2{100.0f,defaults.room,defaults.roomHF,defaults.roomRolloffFactor,defaults.decayTime,defaults.decayHFRatio,defaults.reflections,defaults.reflectionsDelay,defaults.reverb,defaults.reverbDelay,defaults.diffusion,defaults.density,defaults.hfReference};
        ReverbConvertI3DL2ToNative(&i3dl2,&native_);
        floatFormat_={WAVE_FORMAT_IEEE_FLOAT,format.nChannels,format.nSamplesPerSec,format.nSamplesPerSec*format.nChannels*4,static_cast<WORD>(format.nChannels*4),32,0};
        XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS in{&floatFormat_,frames},out{&floatFormat_,frames};hr=apo_->LockForProcess(1,&in,1,&out);if(FAILED(hr)){clear();return hr;}locked_=true;
        try{input.resize(frames*format.nChannels);output.resize(input.size());}catch(...){clear();return E_OUTOFMEMORY;}defaultsPending_=true;return S_OK;
    }
    HRESULT process(const WAVEFORMATEX& f,ULONG count,BYTE* data,bool zero){
        if(!zero&&f.wFormatTag==WAVE_FORMAT_IEEE_FLOAT)for(size_t i=0;i<count/4;++i){float v;std::memcpy(&v,data+i*4,4);if(!std::isfinite(v))return E_INVALIDARG;}
        if(!ready())return DMO_E_TYPE_NOT_SET;if(!count)return S_OK;
        bool signal=false;if(!zero)for(size_t i=0;i<count/(f.wBitsPerSample/8);++i){
            if(f.wFormatTag==WAVE_FORMAT_IEEE_FLOAT){float v;std::memcpy(&v,data+i*4,4);signal|=v!=0;}
            else{short v;std::memcpy(&v,data+i*2,2);signal|=v!=0;}if(signal)break;
        }
        // A reset stream with no real signal has no mathematical room history.
        // The stock float APO may generate tiny bias even after Reset; define
        // digital silence from stream history, never from an amplitude cutoff.
        if(!signal&&!hasSignal_){std::memset(data,0,count);return S_OK;}hasSignal_|=signal;
        // Parameters require Initialize/Lock first, and SetParameters belongs
        // on this processing thread. No allocation or format negotiation here.
        if(defaultsPending_){
            parameters_->SetParameters(&native_,sizeof(native_));
            // The APO crossfades from its initial thru state when first enabled.
            // Complete that transition on zero prehistory, never on user input.
            // The fixed block is the locked maximum; no external frame/time is
            // consumed and all scratch memory was allocated during preparation.
            std::fill(input.begin(),input.end(),0);std::fill(output.begin(),output.end(),0);
            XAPO_PROCESS_BUFFER_PARAMETERS in{input.data(),XAPO_BUFFER_VALID,frames},out{output.data(),XAPO_BUFFER_VALID,frames};
            apo_->Process(1,&in,1,&out,TRUE);
            if(out.ValidFrameCount!=frames||(out.BufferFlags!=XAPO_BUFFER_SILENT&&out.BufferFlags!=XAPO_BUFFER_VALID))return E_UNEXPECTED;
            if(out.BufferFlags==XAPO_BUFFER_VALID&&!std::all_of(output.begin(),output.end(),[](float v){return v==0;}))return E_UNEXPECTED;
            defaultsPending_=false;
        }bool audible=false;const auto total=count/f.nBlockAlign;
        for(DWORD at=0;at<total;){const auto n=std::min<DWORD>(frames,total-at);const auto samples=n*f.nChannels;auto bytes=data+at*f.nBlockAlign;
            for(DWORD i=0;i<samples;++i){if(zero)input[i]=0;else if(f.wFormatTag==WAVE_FORMAT_IEEE_FLOAT)std::memcpy(&input[i],bytes+i*4,4);else{short v;std::memcpy(&v,bytes+i*2,2);input[i]=v/32768.0f;}}
            XAPO_PROCESS_BUFFER_PARAMETERS in{input.data(),zero?XAPO_BUFFER_SILENT:XAPO_BUFFER_VALID,n},out{output.data(),XAPO_BUFFER_VALID,n};apo_->Process(1,&in,1,&out,TRUE);
            if(out.ValidFrameCount!=n)return E_UNEXPECTED;if(out.BufferFlags!=XAPO_BUFFER_SILENT&&out.BufferFlags!=XAPO_BUFFER_VALID)return E_UNEXPECTED;
            audible|=out.BufferFlags==XAPO_BUFFER_VALID;
            for(DWORD i=0;i<samples;++i){const float v=out.BufferFlags==XAPO_BUFFER_SILENT?0:output[i];if(f.wFormatTag==WAVE_FORMAT_IEEE_FLOAT)std::memcpy(bytes+i*4,&v,4);else{const auto scaled=std::round(std::clamp(v,-1.0f,32767.0f/32768.0f)*32768.0f);const auto pcm=static_cast<short>(scaled);std::memcpy(bytes+i*2,&pcm,2);}}
            at+=n;
        }return !zero||audible?S_FALSE:S_OK;
    }
};

bool format(const DMO_MEDIA_TYPE* mt,WAVEFORMATEX& f){
    if(!mt||!IsEqualGUID(mt->majortype,audioType)||!IsEqualGUID(mt->formattype,waveFormatType)||
       !mt->pbFormat||mt->cbFormat<16||mt->pUnk)return false;
    f={};std::memcpy(&f,mt->pbFormat,std::min<std::size_t>(sizeof(f),mt->cbFormat));
    return FileOutputWriter::valid_format(f)&&(f.nChannels==1||f.nChannels==2)&&f.nSamplesPerSec>=20000&&f.nSamplesPerSec<=48000&&((f.wFormatTag==WAVE_FORMAT_PCM&&f.wBitsPerSample==16)||(f.wFormatTag==WAVE_FORMAT_IEEE_FLOAT&&f.wBitsPerSample==32))&&IsEqualGUID(mt->subtype,subtype(f.wFormatTag));
}
bool same(const WAVEFORMATEX& a,const WAVEFORMATEX& b){return a.wFormatTag==b.wFormatTag&&a.nChannels==b.nChannels&&a.nSamplesPerSec==b.nSamplesPerSec&&a.wBitsPerSample==b.wBitsPerSample;}
HRESULT media_type(const WAVEFORMATEX& f,DMO_MEDIA_TYPE* mt){
    if(!mt)return E_POINTER;*mt={};mt->pbFormat=static_cast<BYTE*>(CoTaskMemAlloc(sizeof(f)));if(!mt->pbFormat)return E_OUTOFMEMORY;
    mt->majortype=audioType;mt->subtype=subtype(f.wFormatTag);mt->bFixedSizeSamples=TRUE;mt->lSampleSize=f.nBlockAlign;
    mt->formattype=waveFormatType;mt->cbFormat=sizeof(f);std::memcpy(mt->pbFormat,&f,sizeof(f));return S_OK;
}
class Output final:public IMediaObject,public IMediaObjectInPlace,public EnvironmentalReverbControl {
    std::atomic<ULONG> refs_{1};std::recursive_mutex mutex_;ReverbEngine engine_;
    WAVEFORMATEX input_{},output_{};bool hasInput_=false,hasOutput_=false;
    IMediaBuffer* pending_=nullptr;DWORD offset_=0,inputFlags_=0;REFERENCE_TIME timestamp_=0,length_=0;
    void clear_pending(){if(pending_){pending_->Release();pending_=nullptr;}offset_=0;}
    HRESULT set_type(bool input,DWORD stream,const DMO_MEDIA_TYPE* mt,DWORD flags){
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;
        if(flags&~(DMO_SET_TYPEF_TEST_ONLY|DMO_SET_TYPEF_CLEAR))return E_INVALIDARG;
        if((flags&DMO_SET_TYPEF_CLEAR)&&(flags&DMO_SET_TYPEF_TEST_ONLY))return E_INVALIDARG;
        if(pending_)return DMO_E_NOTACCEPTING;
        if(flags&DMO_SET_TYPEF_CLEAR){(input?hasInput_:hasOutput_)=false;engine_.clear();return S_OK;}
        WAVEFORMATEX f{};if(!format(mt,f))return DMO_E_TYPE_NOT_ACCEPTED;
        if((input?hasOutput_:hasInput_)&&!same(f,input?output_:input_))return DMO_E_TYPE_NOT_ACCEPTED;
        if(!(flags&DMO_SET_TYPEF_TEST_ONLY)){
            const bool had=input?hasInput_:hasOutput_;const auto old=input?input_:output_;
            if(had&&same(f,old))return S_OK;
            (input?input_:output_)=f;(input?hasInput_:hasOutput_)=true;engine_.clear();
            if(hasInput_&&hasOutput_){const auto hr=engine_.prepare(input_);if(FAILED(hr)){(input?input_:output_)=old;(input?hasInput_:hasOutput_)=had;return hr;}}
        }return S_OK;
    }
    HRESULT type(bool input,DWORD stream,DWORD index,DMO_MEDIA_TYPE* mt){
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;if(index)return DMO_E_NO_MORE_ITEMS;
        const bool exists=input?hasOutput_:hasInput_;const auto f=exists?(input?output_:input_):WAVEFORMATEX{WAVE_FORMAT_PCM,2,44100,176400,4,16,0};
        return mt?media_type(f,mt):S_OK;
    }
    HRESULT current(bool input,DWORD stream,DMO_MEDIA_TYPE* mt){std::lock_guard<std::recursive_mutex> lock(mutex_);if(stream)return DMO_E_INVALIDSTREAMINDEX;if(!(input?hasInput_:hasOutput_))return DMO_E_TYPE_NOT_SET;return media_type(input?input_:output_,mt);}
public:
    ~Output(){clear_pending();}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** p) override {if(!p)return E_POINTER;*p=nullptr;
        if(IsEqualGUID(id,IID_IUnknown)||IsEqualGUID(id,__uuidof(IMediaObject)))*p=static_cast<IMediaObject*>(this);
        else if(IsEqualGUID(id,__uuidof(IMediaObjectInPlace)))*p=static_cast<IMediaObjectInPlace*>(this);
        else if(IsEqualGUID(id,environmentalReverbControlId))*p=static_cast<EnvironmentalReverbControl*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
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
    HRESULT STDMETHODCALLTYPE Flush() override{std::lock_guard<std::recursive_mutex> lock(mutex_);clear_pending();engine_.reset();return S_OK;}
    HRESULT STDMETHODCALLTYPE Discontinuity(DWORD i) override{return i?DMO_E_INVALIDSTREAMINDEX:S_OK;}
    HRESULT STDMETHODCALLTYPE AllocateStreamingResources() override{std::lock_guard<std::recursive_mutex> lock(mutex_);return hasInput_&&hasOutput_?engine_.prepare(input_):DMO_E_TYPE_NOT_SET;}
    HRESULT STDMETHODCALLTYPE FreeStreamingResources() override{return Flush();}
    HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD i,DWORD* f) override{std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!f)return E_POINTER;*f=hasInput_&&hasOutput_&&!pending_?DMO_INPUT_STATUSF_ACCEPT_DATA:0;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessInput(DWORD i,IMediaBuffer* b,DWORD flags,REFERENCE_TIME t,REFERENCE_TIME duration) override{
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(i)return DMO_E_INVALIDSTREAMINDEX;if(!b)return E_POINTER;
        if(flags&~(DMO_INPUT_DATA_BUFFERF_SYNCPOINT|DMO_INPUT_DATA_BUFFERF_TIME|DMO_INPUT_DATA_BUFFERF_TIMELENGTH))return E_INVALIDARG;
        if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;if(pending_)return DMO_E_NOTACCEPTING;
        BYTE* bytes=nullptr;DWORD count=0;const auto hr=b->GetBufferAndLength(&bytes,&count);if(FAILED(hr))return hr;if((count&&!bytes)||count%input_.nBlockAlign)return E_INVALIDARG;
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
        hr=out.pBuffer->SetLength(n);if(FAILED(hr))return hr;if(n){std::memcpy(dst,src+offset_,n);hr=engine_.process(input_,n,dst,false);if(FAILED(hr))return hr;}
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_TIME){out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_TIME;out.rtTimestamp=timestamp_+static_cast<REFERENCE_TIME>(std::uint64_t(offset_)*10000000/input_.nAvgBytesPerSec);}
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_TIMELENGTH){out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;out.rtTimelength=static_cast<REFERENCE_TIME>(std::uint64_t(n)*10000000/input_.nAvgBytesPerSec);}
        if(inputFlags_&DMO_INPUT_DATA_BUFFERF_SYNCPOINT)out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
        offset_+=n;if(offset_<size)out.dwStatus|=DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;else clear_pending();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Lock(LONG value) override{if(value)mutex_.lock();else mutex_.unlock();return S_OK;}
    HRESULT STDMETHODCALLTYPE Process(ULONG count,BYTE* data,REFERENCE_TIME,DWORD flags) override{
        std::lock_guard<std::recursive_mutex> lock(mutex_);if(flags&~DMO_INPLACE_ZERO)return E_INVALIDARG;if(!hasInput_||!hasOutput_)return DMO_E_TYPE_NOT_SET;
        if((count&&!data)||count%input_.nBlockAlign)return E_INVALIDARG;
        return engine_.process(input_,count,data,(flags&DMO_INPLACE_ZERO)!=0);
    }
    HRESULT STDMETHODCALLTYPE Clone(IMediaObjectInPlace** result) override{if(!result)return E_POINTER;*result=nullptr;std::lock_guard<std::recursive_mutex> lock(mutex_);auto copy=new(std::nothrow) Output;if(!copy)return E_OUTOFMEMORY;copy->input_=input_;copy->output_=output_;copy->hasInput_=hasInput_;copy->hasOutput_=hasOutput_;if(copy->hasInput_&&copy->hasOutput_){const auto hr=copy->engine_.prepare(copy->input_);if(FAILED(hr)){copy->Release();return hr;}}*result=copy;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetLatency(REFERENCE_TIME* t) override{if(!t)return E_POINTER;*t=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetDefaults(EnvironmentalReverbParameters* p) override{if(!p)return E_POINTER;*p=defaults;return S_OK;}
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
IMediaObject* create_environmental_reverb_dmo(){return new Output;}
EnvironmentalReverbRegistration::EnvironmentalReverbRegistration(){auto factory=new Factory;const auto hr=CoRegisterClassObject(environmentalReverbRuntimeClass,factory,CLSCTX_INPROC_SERVER,REGCLS_MULTIPLEUSE,&cookie_);factory->Release();if(FAILED(hr))throw std::runtime_error("Source Environmental Reverb process registration failed");}
EnvironmentalReverbRegistration::~EnvironmentalReverbRegistration(){if(cookie_)CoRevokeClassObject(cookie_);}
}
