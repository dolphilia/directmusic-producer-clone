#pragma once
#include "compat/directsound_send.h"
void capture_send_runtime_modules(){
    const auto utf8=[](const std::wstring& text){
        const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
        if(n<=0)throw std::runtime_error("Module UTF8 size");
        std::string result(n,0);
        if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),n,nullptr,nullptr)!=n)
            throw std::runtime_error("Module UTF8 write");
        return result;
    };
    const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());
    if(snapshot==INVALID_HANDLE_VALUE)throw std::runtime_error("Send runtime module snapshot");
    MODULEENTRY32W module{};module.dwSize=sizeof(module);
    if(!Module32FirstW(snapshot,&module)){CloseHandle(snapshot);throw std::runtime_error("Empty Send runtime module snapshot");}
    do{
        const auto file=CreateFileW(module.szExePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE){CloseHandle(snapshot);throw std::runtime_error("Send module disk identity");}
        wchar_t physical[32768]{};
        const auto length=GetFinalPathNameByHandleW(file,physical,32768,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);CloseHandle(file);
        if(!length||length>=32768){CloseHandle(snapshot);throw std::runtime_error("Send module physical path");}
        std::cout<<"loadedModuleLogical="<<utf8(module.szExePath)<<"\nloadedModule="<<utf8(physical)<<"\n";
    }while(Module32NextW(snapshot,&module));
    CloseHandle(snapshot);
}
// Actual builtin Send identity; no synthetic class is instantiated here.
void audio_path_send_runtime_tests(const std::filesystem::path& dir,bool runOs=false){
    const auto buffer=[](Chunk& root,size_t at)->Chunk&{size_t n=0;for(auto& c:root.children)if(c.id=="LIST"&&c.type=="dbfl"&&n++==at)return c;throw std::runtime_error("Send test buffer missing");};
    const auto guid=[](const GUID& value){AudioBufferId bytes{};std::memcpy(bytes.data(),&value,16);return bytes;};
    const auto sendClass=guid(producer::compat::directSoundSendClass);
    const AudioBufferId expected={0x76,0x21,0x60,0xef,0xbb,0xbc,0xe0,0x49,0x8c,0xca,0xe0,0x9a,0x5a,0x15,0x2b,0x33};
    require(sendClass==expected&&!is_declared_os_audio_effect(sendClass),"Original-derived builtin Send class is distinct from registered DSDMO classes");
    AudioPathDocument document;require(document.add_file_output(0)&&document.add_mixin_buffer(2)&&document.add_mixin_buffer(2),"Owned source and two eligible no-PChannel mix-ins");
    require(document.available_send_destinations(0)==std::vector<size_t>({1,2}),"New Send offers eligible local destinations without requiring an existing effect");
    const auto source=document.save_bytes();require(document.add_send(0,1,0,-600),"Insert actual Send before existing effect in one document transaction");
    auto effects=document.effects();require(effects.size()==2&&effects[0].classId==sendClass&&effects[0].sendBuffer==document.buffers()[1]&&effects[1].classId==guid(fileOutputClass),"Native Send class destination and effect ordering preserved");
    const auto first=document.save_bytes();auto native=Chunk::parse(first);auto& fx=buffer(native,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0];
    require(fx.find("data")&&fx.find("data")->data.size()==4&&read32(fx.find("data")->data,0)==0xfffffda8&&document.send_attenuation(0,0)==-600,"Native attenuation is exact four-byte signed -600");
    require(document.undo()&&document.save_bytes()==source&&document.redo()&&document.save_bytes()==first,"Send creation UndoRedo restores exact bytes");
    require(document.add_send(0,2)&&document.effects().size()==3&&document.send_attenuation(0,2)==0,"Multiple Send effects in one buffer can target distinct mix-ins");
    const auto beforeLevel=document.save_bytes();require(document.set_send_attenuation(0,0,-10000)&&document.send_attenuation(0,0)==-10000,"Observed attenuation lower bound accepted");
    const auto level=document.save_bytes();require(document.undo()&&document.save_bytes()==beforeLevel&&document.redo()&&document.save_bytes()==level,"Attenuation history is one atomic transaction");
    const auto invalid=[&](const std::function<bool(AudioPathDocument&)>& change,const char* message){
        AudioPathDocument d;d.load(level);require(d.set_name(L"Redo marker")&&d.undo(),"Seed nonempty redo stack");
        require(!change(d)&&d.save_bytes()==level&&!d.dirty(),message);
        require(d.redo()&&d.name()==L"Redo marker"&&d.undo()&&d.save_bytes()==level,"Rejected Send edit retains redo and saved bytes");
    };
    invalid([](auto& d){return d.set_send_attenuation(0,0,-10001);},"Below observed attenuation range refused without mutation");
    invalid([](auto& d){return d.set_send_attenuation(0,0,1);},"Positive Send gain refused without mutation");
    invalid([](auto& d){return d.set_send_attenuation(0,1,-600);},"FileOutput cannot be edited as Send attenuation");
    invalid([](auto& d){return d.add_send(0,0);},"Self destination refused without mutation");
    invalid([](auto& d){return d.add_send(0,1,99);},"Missing insertion position refused without mutation");
    invalid([](auto& d){return d.set_send_attenuation(0,0,-10000);},"Unchanged attenuation is a no-op");
    auto opaque=Chunk::parse(level);auto& opaqueFx=buffer(opaque,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0];
    Chunk unknown;unknown.id="xSnd";unknown.data={1,3,5};unknown.padding=0x91;opaqueFx.children.push_back(unknown);
    AudioPathDocument opaqueDoc;opaqueDoc.load(opaque.encode());require(opaqueDoc.set_send_attenuation(0,0,-600),"Attenuation edit on owned Send preserves unknown metadata");
    auto expectedOpaque=opaque;put32(buffer(expectedOpaque,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0].find("data")->data,0,0xfffffda8);
    require(opaqueDoc.save_bytes()==expectedOpaque.encode(),"Only four parameter bytes change; opaque payload padding and effect order unchanged");
    auto absent=opaque;auto& absentFx=buffer(absent,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0];
    absentFx.children.erase(std::remove_if(absentFx.children.begin(),absentFx.children.end(),[](const auto& c){return c.id=="data";}),absentFx.children.end());
    AudioPathDocument absentDoc;absentDoc.load(absent.encode());require(absentDoc.send_attenuation(0,0)==0&&absentDoc.set_send_attenuation(0,0,-600),"Missing parameter data retains observed zero default and can be authored");
    auto malformed=opaque;buffer(malformed,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0].find("data")->data.push_back(0x55);
    AudioPathDocument malformedDoc;malformedDoc.load(malformed.encode());require(!malformedDoc.send_attenuation(0,0)&&!malformedDoc.set_send_attenuation(0,0,-600)&&malformedDoc.save_bytes()==malformed.encode(),"Unknown parameter shape retained and not reinterpreted");
    rejected([&]{prepare_audio_path_send_runtime(malformed.encode());},"Unknown Send parameter shape rejected before OS factory");
    const auto orderedBytes=prepare_audio_path_send_runtime(opaqueDoc.save_bytes());AudioPathDocument ordered;ordered.load(orderedBytes);
    require(ordered.buffers()==std::vector<AudioBufferId>({document.buffers()[1],document.buffers()[2],document.buffers()[0]}),"Private runtime copy places both destinations before source");
    require(prepare_audio_path_send_runtime(orderedBytes)==orderedBytes&&opaqueDoc.save_bytes()==expectedOpaque.encode(),"Runtime ordering is idempotent and leaves saved native source unchanged");
    auto orderedTree=Chunk::parse(orderedBytes),sourceTree=Chunk::parse(opaqueDoc.save_bytes());
    require(buffer(orderedTree,2).encode()==buffer(sourceTree,0).encode(),"Runtime ordering retains complete source buffer and effect chain bytes");
    auto external=opaque;AudioBufferId externalId{};externalId.fill(0x7e);std::copy(externalId.begin(),externalId.end(),buffer(external,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0].find("fxhr")->data.begin()+36);
    rejected([&]{prepare_audio_path_send_runtime(external.encode());},"External destination refuses undeclared lifetime before factory");
    auto cycle=opaque;auto target=buffer(cycle,1).find("RIFF","DSBC");Chunk cycleFx;cycleFx.id="LIST";cycleFx.type="fxls";cycleFx.children.push_back(buffer(cycle,0).find("RIFF","DSBC")->find("LIST","fxls")->children[0]);
    const auto cycleSource=document.buffers()[0];std::copy(cycleSource.begin(),cycleSource.end(),cycleFx.children[0].find("fxhr")->data.begin()+36);target->children.push_back(cycleFx);
    rejected([&]{prepare_audio_path_send_runtime(cycle.encode());},"Imported local Send cycle refused without rewriting its native bytes");
    AudioPathDocument recording;recording.load(opaqueDoc.save_bytes());require(recording.add_file_output(1)&&recording.add_file_output(2),"Source and both mix-in buffers own source FileOutput taps");
    const auto recordingSource=recording.save_bytes(),recordingRuntime=prepare_audio_path_send_runtime(recordingSource);const auto targets=audio_path_file_output_targets(recordingSource,recordingRuntime);
    require(targets.size()==3&&targets[0].buffer==0&&targets[0].stage==0x6100&&targets[0].pchannel==0&&targets[0].index==0,"PChannel recording is numbered before global mix-in targets");
    require(targets[1].buffer==1&&targets[1].pchannel==0&&targets[1].stage==0x7100&&targets[1].index==0&&targets[2].buffer==2&&targets[2].stage==0x7100&&targets[2].index==1,"Global controls use observed PChannel0 and runtime mix-in ordinals");
    auto disconnected=Chunk::parse(recordingSource);put32(buffer(disconnected,1).find("ddah")->data,16,0);
    rejected([&]{audio_path_file_output_targets(disconnected.encode(),disconnected.encode());},"Disconnected ordinary FileOutput is not implicitly a mix-in");
    const auto file=dir/L"RealSend.aup";recording.save(file.wstring());AudioPathDocument restored;restored.load(read_file(file.wstring()));
    require(restored.save_bytes()==recordingSource&&restored.send_attenuation(0,0)==-600&&audio_path_file_output_targets(restored.save_bytes(),recordingRuntime).size()==3,"Fresh native restore preserves actual Send values and recording routing");
    auto reverberant=restored;require(reverberant.add_waves_reverb(0)&&reverberant.add_waves_reverb(1)&&reverberant.add_waves_reverb(2),"Waves stop targets cover routed and both global mix-in buffers");
    const auto wavesSource=reverberant.save_bytes(),wavesRuntime=prepare_audio_path_send_runtime(wavesSource);const auto wavesTargets=audio_path_waves_reverb_targets(wavesSource,wavesRuntime);
    require(wavesTargets.size()==3&&wavesTargets[0].buffer==0&&wavesTargets[0].stage==0x6100&&wavesTargets[0].pchannel==0&&wavesTargets[0].index==0&&wavesTargets[1].buffer==1&&wavesTargets[1].stage==0x7100&&wavesTargets[1].index==0&&wavesTargets[2].buffer==2&&wavesTargets[2].stage==0x7100&&wavesTargets[2].index==1,"Waves resolves exact routed and reordered global addresses without changing native bytes");
    audio_path_environmental_reverb_tests(dir,runOs);
    if(!runOs)return;
    const auto firstFile=dir/L"Mixed.wav",secondFile=dir/L"Mixed1.wav",thirdFile=dir/L"Mixed2.wav";Conductor player;
    player.start_file_output(recordingSource,firstFile.wstring(),GetDesktopWindow());
    require(player.file_output_active()&&std::filesystem::exists(firstFile)&&std::filesystem::exists(secondFile)&&std::filesystem::exists(thirdFile),"Actual declared OS creates source Send and obtains direct plus two global FileOutput controls");
    capture_send_runtime_modules();
    Sleep(150);player.stop_file_output();require(!player.file_output_active(),"Independent Stop releases all source and global recording controls");
    for(const auto& p:{firstFile,secondFile,thirdFile}){const auto bytes=read_file(p.wstring());const auto wave=Chunk::parse(bytes);require(read32(bytes,4)+8==bytes.size()&&wave.find("fmt ")&&wave.find("data"),"Recorded silence container finalized; audible PCM semantics remain a separate test");}
    player.start_file_output(wavesSource,(dir/L"GlobalWaves.wav").wstring(),GetDesktopWindow());
    require(player.file_output_active()&&std::count_if(player.calls().begin(),player.calls().end(),[](const auto& c){return c.operation=="Get retained recording Waves DMO"&&c.result==S_OK;})==3,"Actual OS resolves all routed/global Waves DMOs on the recording AudioPath");
    player.stop_file_output();require(!player.file_output_active()&&reverberant.save_bytes()==wavesSource,"Independent recording Stop releases owned global Waves references and preserves source");
    player.shutdown();require(recording.save_bytes()==recordingSource,"OS runtime construction and Stop retain native authoring bytes");
}
