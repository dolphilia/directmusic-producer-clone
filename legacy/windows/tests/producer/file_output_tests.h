#pragma once
#include "producer/file_output_dmo.h"
#include <mediaerr.h>
using namespace producer::app;
class OutputTestBuffer final:public IMediaBuffer {
    ULONG refs_=1;DWORD length_=0;
public:
    Bytes bytes;explicit OutputTestBuffer(DWORD size):bytes(size){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override{if(!out)return E_POINTER;*out=nullptr;if(!IsEqualGUID(id,IID_IUnknown)&&!IsEqualGUID(id,__uuidof(IMediaBuffer)))return E_NOINTERFACE;*out=this;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef() override{return ++refs_;}ULONG STDMETHODCALLTYPE Release() override{auto n=--refs_;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE SetLength(DWORD n) override{if(n>bytes.size())return E_INVALIDARG;length_=n;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMaxLength(DWORD* n) override{if(!n)return E_POINTER;*n=static_cast<DWORD>(bytes.size());return S_OK;}
    HRESULT STDMETHODCALLTYPE GetBufferAndLength(BYTE** data,DWORD* n) override{if(data)*data=bytes.data();if(n)*n=length_;return S_OK;}
};
void file_output_tests(const std::filesystem::path& dir){
    WAVEFORMATEX pcm{WAVE_FORMAT_PCM,1,8000,8000,1,8,0};FileOutputWriter writer;
    const auto output=dir/L"owned-pcm.wav";Bytes signal={0,1,127,128,254};
    require(writer.start(output.wstring(),pcm)==S_OK,"FileOutput exclusive owned output");
    require(writer.append(signal.data(),static_cast<DWORD>(signal.size()))==S_OK,"FileOutput PCM append");
    const auto before=writer.status();require(writer.append(nullptr,1)==E_INVALIDARG&&writer.status().bytes==before.bytes,"invalid audio retains accumulated recording");
    require(writer.start((dir/L"other.wav").wstring(),pcm)==E_UNEXPECTED,"active writer cannot change output");
    require(writer.stop()==S_OK&&!writer.status().active&&writer.status().frames==5,"recording stop finalizes odd PCM");
    auto bytes=read_file(output.wstring());auto root=Chunk::parse(bytes);require(root.find("data")->data==signal&&read32(bytes,4)==bytes.size()-8&&bytes.back()==0,"RIFF actual lengths padding exact PCM");
    require(FAILED(writer.start(output.wstring(),pcm))&&read_file(output.wstring())==bytes,"existing audio output never overwritten");
    auto malformed=pcm;malformed.nBlockAlign=2;require(writer.start((dir/L"invalid.wav").wstring(),malformed)==E_INVALIDARG&&!std::filesystem::exists(dir/L"invalid.wav"),"invalid format creates no file");
    WAVEFORMATEX floating{WAVE_FORMAT_IEEE_FLOAT,2,48000,384000,8,32,0};
    float samples[]={0.1f,-0.1f,0.2f,-0.2f};require(writer.start((dir/L"float.wav").wstring(),floating)==S_OK&&writer.append(samples,sizeof(samples))==S_OK&&writer.stop()==S_OK,"float recording");
    root=Chunk::parse(read_file((dir/L"float.wav").wstring()));require(root.find("fact")&&read32(root.find("fact")->data,0)==2&&root.find("data")->data.size()==sizeof(samples),"float fact frames and PCM lengths");
    const auto com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);require(SUCCEEDED(com),"FileOutput COM thread");
    {
        FileOutputRegistration registration;IMediaObject* raw=nullptr;
        require(CoCreateInstance(fileOutputRuntimeClass,nullptr,CLSCTX_INPROC_SERVER,__uuidof(IMediaObject),reinterpret_cast<void**>(&raw))==S_OK&&raw,"private process factory resolves source DMO without original CLSID activation");
        std::unique_ptr<IMediaObject,void(*)(IMediaObject*)> obj(raw,[](auto p){p->Release();});
        FileOutputControl* control=nullptr;IMediaObjectInPlace* inplace=nullptr;
        require(raw->QueryInterface(fileOutputControlId,reinterpret_cast<void**>(&control))==S_OK&&raw->QueryInterface(__uuidof(IMediaObjectInPlace),reinterpret_cast<void**>(&inplace))==S_OK,"source control and inplace ABI");
        std::unique_ptr<FileOutputControl,void(*)(FileOutputControl*)> ctl(control,[](auto p){p->Release();});
        std::unique_ptr<IMediaObjectInPlace,void(*)(IMediaObjectInPlace*)> ip(inplace,[](auto p){p->Release();});
        require(control->Start()==DMO_E_TYPE_NOT_SET,"record start requires negotiated type");
        DMO_MEDIA_TYPE mt{};require(raw->GetInputType(0,0,&mt)==S_OK,"DMO default type enumeration");
        require(raw->SetInputType(0,&mt,DMO_SET_TYPEF_TEST_ONLY)==S_OK,"DMO test type accepted");
        DMO_MEDIA_TYPE current{};require(raw->GetInputCurrentType(0,&current)==DMO_E_TYPE_NOT_SET,"test-only negotiation does not mutate");
        require(raw->SetInputType(0,&mt,0)==S_OK&&raw->SetOutputType(0,&mt,0)==S_OK,"DMO identical passthrough negotiation");
        auto fmt=reinterpret_cast<WAVEFORMATEX*>(mt.pbFormat);fmt->nSamplesPerSec=48000;fmt->nAvgBytesPerSec=48000*4;
        require(raw->SetOutputType(0,&mt,0)==DMO_E_TYPE_NOT_ACCEPTED,"mismatched output rejected");CoTaskMemFree(mt.pbFormat);
        const auto path=dir/L"dmo.wav";require(control->SetFilename(path.c_str())==S_OK&&control->Start()==S_OK&&control->Start()==S_FALSE,"DMO record independent start idempotence");
        auto in=new OutputTestBuffer(16);for(size_t i=0;i<in->bytes.size();++i)in->bytes[i]=static_cast<BYTE>(i*7);in->SetLength(16);
        const auto exact=in->bytes;require(inplace->Process(16,in->bytes.data(),0,0)==S_OK&&in->bytes==exact,"inplace captures without changing audio");
        require(raw->ProcessInput(0,in,DMO_INPUT_DATA_BUFFERF_TIME,10000,0)==S_OK,"out-of-place captures accepted input");
        require(raw->ProcessInput(0,in,0,0,0)==DMO_E_NOTACCEPTING,"pending input prevents duplicate recording");
        auto out=new OutputTestBuffer(8);DMO_OUTPUT_DATA_BUFFER ob{};ob.pBuffer=out;DWORD status=99;
        require(raw->ProcessOutput(0,1,&ob,&status)==S_OK&&status==0&&(ob.dwStatus&DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)&&std::equal(out->bytes.begin(),out->bytes.end(),exact.begin()),"partial output preserves first half");
        require(raw->ProcessOutput(0,1,&ob,&status)==S_OK&&!(ob.dwStatus&DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)&&std::equal(out->bytes.begin(),out->bytes.end(),exact.begin()+8),"partial output preserves second half");
        require(raw->ProcessOutput(0,1,&ob,&status)==S_FALSE,"drained DMO no invented audio");out->Release();in->Release();
        require(control->SetFilename(L"other.wav")==E_UNEXPECTED&&control->SetOption(1)==E_UNEXPECTED,"active capture cannot change file policy");
        require(control->Stop()==S_OK&&control->Stop()==S_FALSE,"DMO independent stop finalized");
        root=Chunk::parse(read_file(path.wstring()));Bytes twice=exact;twice.insert(twice.end(),exact.begin(),exact.end());require(root.find("data")->data==twice,"DMO records each input exactly once across fragmented outputs");
        require(control->SetOption(1)==E_NOTIMPL,"unconfirmed overwrite option explicitly unsupported");
    }
    CoUninitialize();
    AudioPathDocument ap;const auto original=ap.save_bytes();require(ap.add_file_output(0),"predefined stereo materialized for source FileOutput");
    const auto recorded=ap.save_bytes();require(ap.effects().size()==1&&ap.ports()[0].routes[0].buffers[0]==ap.buffers()[0],"effect and route identity owned together");
    require(!ap.add_file_output(0)&&!ap.add_file_output(99)&&ap.save_bytes()==recorded,"duplicate or invalid buffer retains document");
    require(ap.undo()&&ap.save_bytes()==original&&ap.redo()&&ap.save_bytes()==recorded,"FileOutput insertion single undo redo");
    auto bad=Chunk::parse(recorded);auto effects=bad.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls");put32(effects->children[0].find("fxhr")->data,52,1);
    bool rejected=false;try{ap.load(bad.encode());}catch(...){rejected=true;}require(rejected&&ap.save_bytes()==recorded,"reserved effect input rejected atomically");
    ap.save((dir/L"recording.aup").wstring());AudioPathDocument reload;reload.load(read_file((dir/L"recording.aup").wstring()));require(reload.save_bytes()==recorded,"source capture AudioPath disk reload");
    Framework host;const auto owned=host.open_audio_path((dir/L"recording.aup").wstring());host.save_project((dir/L"recording.dmpj").wstring());
    host.save_runtime(RuntimeDocumentKind::AudioPath,owned,(dir/L"recording.aud").wstring());AudioPathDocument runtime;runtime.load(read_file((dir/L"recording.aud").wstring()));
    require(runtime.effects().empty()&&host.audio_path_document(owned).save_bytes()==recorded,"runtime export omits Producer-only FileOutput while source retains effect");
}
void file_output_buffer_tests(const std::filesystem::path& dir){
    AudioPathDocument path;require(path.add_file_output(0),"source custom FileOutput buffer prepared");
    // A new Segment has no instrument Band. Use the explicit GM piano
    // playback fixture, then retain one sustained note for the capture.
    auto segment=SegmentDocument::playback_test(69);
    while(segment.notes().size()>1)if(!segment.delete_note(segment.notes().size()-1))throw std::runtime_error("Cannot prepare recording note");
    require(segment.edit_note(0,{0,3072,0,69,100})&&segment.add_note({0,3072,16,60,100}),"recording mapped69 and unmapped60 notes with explicit instrument prepared");segment.set_audio_path(path.save_bytes());
    const auto source=segment.save_bytes();write_file_atomic((dir/L"capture-input.sgp").wstring(),source);path.save((dir/L"capture-input.aup").wstring());
    Conductor player;const auto file=dir/L"buffer-output.wav";
    player.start_file_output(path.save_bytes(),file.wstring(),GetDesktopWindow());require(player.file_output_active(),"actual DirectMusic FileOutput started before Play");
    Sleep(300);player.play(source,dir.wstring(),GetDesktopWindow());Sleep(1000);
    require(player.position().playing&&player.file_output_active(),"music plays on same recording buffer");
    player.stop();const auto afterStop=std::filesystem::file_size(file);Sleep(300);
    require(player.file_output_active()&&std::filesystem::file_size(file)>afterStop,"music Stop leaves independent buffer capture running");
    player.stop_file_output();require(!player.file_output_active(),"explicit buffer recording Stop");player.shutdown();
    const auto recorded=Chunk::parse(read_file(file.wstring()));const auto pcm=recorded.find("data");
    require(pcm&&std::any_of(pcm->data.begin(),pcm->data.end(),[](auto value){return value!=0;})&&segment.save_bytes()==source,"actual buffer audible PCM finalized and source document retained");
}
