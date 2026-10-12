#include "waves_reverb_parameters.h"
#include "compat/waves_reverb.h"
#include <dsound.h>
#include <objbase.h>
#include <objidl.h>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
namespace producer::app {namespace {
template<class T> struct Release{void operator()(T* p)const{if(p)p->Release();}};
template<class T> using Owner=std::unique_ptr<T,Release<T>>;
void check(HRESULT hr,const char* operation){if(hr!=S_OK)throw std::runtime_error(std::string(operation)+" (HRESULT "+std::to_string(static_cast<unsigned long>(hr))+")");}
struct Apartment {
    HRESULT result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    Apartment(){if(FAILED(result)&&result!=RPC_E_CHANGED_MODE)check(result,"Initialize Waves parameter apartment");}
    ~Apartment(){if(SUCCEEDED(result))CoUninitialize();}
};
Owner<IDirectSoundFXWavesReverb> create(){
    wchar_t id[40]{},path[32768]{},expanded[32768]{},system[MAX_PATH]{},wow[MAX_PATH]{};
    if(!StringFromGUID2(producer::compat::wavesReverbClass,id,40))throw std::runtime_error("Waves class formatting failed");
    const auto key=std::wstring(L"CLSID\\")+id+L"\\InprocServer32";DWORD size=sizeof(path);
    if(RegGetValueW(HKEY_CLASSES_ROOT,key.c_str(),nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,path,&size)!=ERROR_SUCCESS)throw std::runtime_error("Declared OS Waves registration unavailable");
    const auto length=ExpandEnvironmentStringsW(path,expanded,32768);
    if(!length||length>32768)throw std::runtime_error("Waves server path expansion failed");
    GetSystemDirectoryW(system,MAX_PATH);GetSystemWow64DirectoryW(wow,MAX_PATH);
    const auto server=std::filesystem::path(expanded).lexically_normal();
    if(CompareStringOrdinal(server.filename().c_str(),-1,L"dsdmo.dll",-1,TRUE)!=CSTR_EQUAL||
       (CompareStringOrdinal(server.parent_path().c_str(),-1,system,-1,TRUE)!=CSTR_EQUAL&&CompareStringOrdinal(server.parent_path().c_str(),-1,wow,-1,TRUE)!=CSTR_EQUAL))throw std::runtime_error("Waves server is outside the declared Windows runtime");
    IDirectSoundFXWavesReverb* raw=nullptr;
    const auto hr=CoCreateInstance(producer::compat::wavesReverbClass,nullptr,CLSCTX_INPROC_SERVER,producer::compat::wavesReverbInterface,reinterpret_cast<void**>(&raw));
    Owner<IDirectSoundFXWavesReverb> effect(raw);check(hr,"Create declared Waves parameters");
    if(!effect)throw std::runtime_error("Waves parameter interface missing");return effect;
}
Owner<IPersistStream> persistence(IDirectSoundFXWavesReverb* effect){
    IPersistStream* raw=nullptr;const auto hr=effect->QueryInterface(IID_IPersistStream,reinterpret_cast<void**>(&raw));
    Owner<IPersistStream> result(raw);check(hr,"Query Waves persistence");if(!result)throw std::runtime_error("Waves persistence missing");return result;
}
Owner<IStream> memory(){IStream* raw=nullptr;const auto hr=CreateStreamOnHGlobal(nullptr,TRUE,&raw);Owner<IStream> result(raw);check(hr,"Create Waves parameter stream");return result;}
Bytes save(IPersistStream* object){
    auto stream=memory();check(object->Save(stream.get(),FALSE),"Save opaque Waves parameters");STATSTG stat{};check(stream->Stat(&stat,STATFLAG_NONAME),"Read Waves stream size");
    if(!stat.cbSize.QuadPart||stat.cbSize.QuadPart>65536)throw std::runtime_error("Waves parameter stream size unsupported");
    LARGE_INTEGER zero{};check(stream->Seek(zero,STREAM_SEEK_SET,nullptr),"Rewind Waves parameter stream");Bytes bytes(static_cast<size_t>(stat.cbSize.QuadPart));ULONG read=0;
    check(stream->Read(bytes.data(),static_cast<ULONG>(bytes.size()),&read),"Read opaque Waves parameters");if(read!=bytes.size())throw std::runtime_error("Incomplete Waves parameter stream");return bytes;
}
WavesReverbParameters parameters(IDirectSoundFXWavesReverb* object){DSFXWavesReverb p{};check(object->GetAllParameters(&p),"Read Waves parameters");return {p.fInGain,p.fReverbMix,p.fReverbTime,p.fHighFreqRTRatio};}
} // namespace
bool valid_waves_reverb_parameters(const WavesReverbParameters& p){
    return std::isfinite(p.inputGain)&&p.inputGain>=DSFX_WAVESREVERB_INGAIN_MIN&&p.inputGain<=DSFX_WAVESREVERB_INGAIN_MAX&&
        std::isfinite(p.reverbMix)&&p.reverbMix>=DSFX_WAVESREVERB_REVERBMIX_MIN&&p.reverbMix<=DSFX_WAVESREVERB_REVERBMIX_MAX&&
        std::isfinite(p.reverbTime)&&p.reverbTime>=DSFX_WAVESREVERB_REVERBTIME_MIN&&p.reverbTime<=DSFX_WAVESREVERB_REVERBTIME_MAX&&
        std::isfinite(p.highFrequencyRatio)&&p.highFrequencyRatio>=DSFX_WAVESREVERB_HIGHFREQRTRATIO_MIN&&p.highFrequencyRatio<=DSFX_WAVESREVERB_HIGHFREQRTRATIO_MAX;
}
Bytes save_waves_reverb_parameters(const WavesReverbParameters& p){
    if(!valid_waves_reverb_parameters(p))throw std::runtime_error("Waves parameters outside public ranges");
    Apartment apartment;auto effect=create();const DSFXWavesReverb value{p.inputGain,p.reverbMix,p.reverbTime,p.highFrequencyRatio};
    check(effect->SetAllParameters(&value),"Set Waves parameters");if(parameters(effect.get())!=p)throw std::runtime_error("Waves parameter roundtrip differs");auto persistent=persistence(effect.get());return save(persistent.get());
}
std::optional<WavesReverbParameters> load_waves_reverb_parameters(const Bytes& bytes){
    if(bytes.empty()||bytes.size()>65536)return {};
    Apartment apartment;auto effect=create();auto persistent=persistence(effect.get());auto stream=memory();ULONG written=0;
    check(stream->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&written),"Write Waves parameter stream");if(written!=bytes.size())throw std::runtime_error("Incomplete Waves parameter input");
    LARGE_INTEGER zero{};check(stream->Seek(zero,STREAM_SEEK_SET,nullptr),"Rewind Waves parameter input");if(persistent->Load(stream.get())!=S_OK)return {};
    const auto p=parameters(effect.get());if(!valid_waves_reverb_parameters(p)||save(persistent.get())!=bytes)return {};return p;
}
}
