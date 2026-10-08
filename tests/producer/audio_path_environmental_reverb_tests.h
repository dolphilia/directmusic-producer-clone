#pragma once
#include "producer/environmental_reverb_dmo.h"
#include <mediaerr.h>
#include <cmath>
void capture_send_runtime_modules();
void audio_path_environmental_reverb_tests(const std::filesystem::path& dir,bool runOs){
    AudioPathDocument d;require(d.add_mixin_buffer(2),"Existing custom mix-in retained before Environmental Reverb insertion");const auto before=d.save_bytes();
    require(d.add_environmental_reverb_buffer()&&d.environmental_reverb_buffer()==2&&d.default_send_destination(0)==2,"Singleton Environmental Reverb is preferred over earlier custom mix-in");
    const auto withEnv=d.save_bytes();const auto tree=Chunk::parse(withEnv);size_t n=0;const Chunk* env=nullptr;for(const auto& c:tree.children)if(c.id=="LIST"&&c.type=="dbfl"&&n++==2)env=&c;
    const Bytes expected{0x42,0xc5,0x6c,0x18,0x29,0xdb,0xd3,0x11,0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74,10,0,0,0};
    require(env&&env->find("ddah")->data==expected&&!env->find("RIFF","DSBC"),"Native Environmental Reverb uses frozen SDK GUID and defined/mixin flags, no custom descriptor");
    require(!d.add_environmental_reverb_buffer()&&!d.add_file_output(2)&&!d.add_waves_reverb(2)&&d.save_bytes()==withEnv,"Duplicate or effect override rejects without document mutation");
    require(!d.set_route_buffers(0,0,{d.buffers()[2]})&&d.save_bytes()==withEnv,"Environmental Reverb has no direct PChannel route");
    require(d.undo()&&d.save_bytes()==before&&d.redo()&&d.save_bytes()==withEnv,"Rejected edits preserve Environmental Reverb undo/redo transaction");
    require(d.add_file_output(0)&&d.add_send(0,*d.default_send_destination(0),0,-600),"Actual Send routes source to predefined Environmental Reverb before dry tap");
    const auto native=d.save_bytes(),runtime=prepare_audio_path_send_runtime(native);AudioPathDocument loaded;loaded.load(runtime);
    require(loaded.buffers().front()==d.buffers()[2]&&loaded.buffer_details().front().flags==8&&loaded.effects().front().classId==AudioBufferId{0x62,0xb0,0xf6,0x93,0xbf,0xe5,0x4f,0x4b,0xa2,0x37,0x58,0xd9,9,0xd8,0x4e,0x19},"Private runtime creates source-owned wet-only DSP destination before Send source");
    require(d.save_bytes()==native&&prepare_audio_path_send_runtime(runtime)==runtime,"Private realization is idempotent and leaves native source unchanged");
    const auto envTargets=audio_path_environmental_reverb_targets(native,runtime);
    require(envTargets.size()==1&&envTargets[0].buffer==2&&envTargets[0].pchannel==0&&envTargets[0].stage==0x7100&&envTargets[0].index==0,"Native predefined identity maps to private global source DSP ordinal after Send ordering");
    auto shared=Chunk::parse(native);for(auto& c:shared.children)if(c.id=="LIST"&&c.type=="dbfl")if(c.find("ddah")->data==expected)put32(c.find("ddah")->data,16,11);
    rejected([&]{prepare_audio_path_send_runtime(shared.encode());},"Shared Environmental Reverb lifetime is not silently adopted");
    const auto file=dir/L"EnvironmentalReverb.aup";d.save(file.wstring());AudioPathDocument restored;restored.load(read_file(file.wstring()));
    require(restored.save_bytes()==native&&restored.environmental_reverb_buffer()==2&&restored.send_attenuation(0,0)==-600,"Fresh native restore retains predefined buffer, actual Send destination and attenuation");
    if(!runOs)return;
    const auto com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);require(SUCCEEDED(com),"Initialize declared current Windows reverb COM");
    struct Apartment{~Apartment(){CoUninitialize();}} apartment;
    auto raw=create_environmental_reverb_dmo();std::unique_ptr<IMediaObject,void(*)(IMediaObject*)> effect(raw,[](auto* p){p->Release();});
    EnvironmentalReverbControl* control=nullptr;require(raw->QueryInterface(environmentalReverbControlId,reinterpret_cast<void**>(&control))==S_OK,"Source default control retrieved without legacy DSP activation");
    EnvironmentalReverbParameters p{};const auto defaults=control->GetDefaults(&p);control->Release();
    require(defaults==S_OK&&p.room==-1000&&p.roomHF==-100&&p.decayTime==1.49f&&p.reflections==-2602&&p.reflectionsDelay==0.007f&&p.reverb==200&&p.reverbDelay==0.011f,"Source reverb declares SDK I3DL2 defaults; legacy DSP parity remains separate");
    DMO_MEDIA_TYPE mt{};require(raw->GetInputType(0,0,&mt)==S_OK,"Enumerate source DSP media type");auto& f=*reinterpret_cast<WAVEFORMATEX*>(mt.pbFormat);f.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;f.nSamplesPerSec=22050;f.nBlockAlign=8;f.nAvgBytesPerSec=176400;f.wBitsPerSample=32;mt.subtype.Data1=3;mt.lSampleSize=8;
    require(raw->SetInputType(0,&mt,0)==S_OK&&raw->SetOutputType(0,&mt,0)==S_OK&&raw->AllocateStreamingResources()==S_OK,"Declared Windows APO accepts independent22050Hz stereo float stream");
    f.nSamplesPerSec=16000;f.nAvgBytesPerSec=128000;require(raw->SetInputType(0,&mt,DMO_SET_TYPEF_TEST_ONLY)==DMO_E_TYPE_NOT_ACCEPTED,"Unsupported APO sample rate rejected before processing");CoTaskMemFree(mt.pbFormat);
    IMediaObjectInPlace* inplace=nullptr;require(raw->QueryInterface(__uuidof(IMediaObjectInPlace),reinterpret_cast<void**>(&inplace))==S_OK,"DirectSound in-place bridge retrieved");
    std::unique_ptr<IMediaObjectInPlace,void(*)(IMediaObjectInPlace*)> ownInplace(inplace,[](auto* q){q->Release();});
    IMediaObjectInPlace* cloned=nullptr;require(inplace->Clone(&cloned)==S_OK&&cloned,"Typed cloned DMO prepares its own independent processing resources");
    std::unique_ptr<IMediaObjectInPlace,void(*)(IMediaObjectInPlace*)> ownClone(cloned,[](auto* q){q->Release();});
    std::vector<float> impulse(8192);impulse[0]=0.5f;require(inplace->Process(static_cast<ULONG>(impulse.size()*4),reinterpret_cast<BYTE*>(impulse.data()),0,0)==S_FALSE,"Source DMO processes wet-only room response and reports effect tail");
    double energy=0;bool finite=true;for(auto v:impulse){finite=finite&&std::isfinite(v);energy+=v*v;}
    require(finite&&impulse[0]==0&&impulse[1]==0&&energy>0.00000001,"Room response delays direct impulse and yields nonzero finite reflections");
    Bytes impulseBytes(impulse.size()*sizeof(float));std::memcpy(impulseBytes.data(),impulse.data(),impulseBytes.size());write_file_atomic((dir/L"EnvironmentalSourceImpulse.f32").wstring(),impulseBytes);
    auto clonedImpulse=std::vector<float>(8192);clonedImpulse[0]=0.5f;
    require(cloned->Process(static_cast<ULONG>(clonedImpulse.size()*4),reinterpret_cast<BYTE*>(clonedImpulse.data()),0,0)==S_FALSE&&clonedImpulse==impulse,"Fresh typed clone processes the same settings without sharing original tail state");
    std::vector<float> tail(8192);require(inplace->Process(static_cast<ULONG>(tail.size()*4),reinterpret_cast<BYTE*>(tail.data()),0,DMO_INPLACE_ZERO)==S_FALSE&&std::any_of(tail.begin(),tail.end(),[](float v){return v!=0;}),"Zero input continues nonzero room tail and reports S_FALSE");
    require(raw->Flush()==S_OK,"Flush resets owned DSP state");std::fill(impulse.begin(),impulse.end(),0);require(SUCCEEDED(inplace->Process(static_cast<ULONG>(impulse.size()*4),reinterpret_cast<BYTE*>(impulse.data()),0,DMO_INPLACE_ZERO))&&std::all_of(impulse.begin(),impulse.end(),[](float v){return v==0;}),"Reset room state emits exact silent zero-input PCM");
    Conductor player;player.start_file_output(native,(dir/L"EnvironmentalDry.wav").wstring(),GetDesktopWindow());
    require(player.file_output_active(),"Real declared AudioPath creates registered source reverb and actual builtin Send");capture_send_runtime_modules();
    require(std::count_if(player.calls().begin(),player.calls().end(),[](const auto& c){return c.operation=="Get retained recording Environmental Reverb DMO"&&c.result==S_OK;})==1,"Actual recording path retains its unique source Environmental Reverb DMO");
    auto song=SegmentDocument::playback_test(69);auto songRoot=Chunk::parse(song.save_bytes());put32(songRoot.find("segh")->data,4,122880);song.load(songRoot.encode());
    require(song.edit_note(0,{0,122880,0,69,100})&&song.set_audio_path(native),"Actual source Send/Env fixture owns sustained native note and AudioPath");const auto songSource=song.save_bytes();
    const auto started=[&]{const auto id=player.current_playback_id(),deadline=GetTickCount64()+5000;while(!player.position(id).playing&&GetTickCount64()<deadline)Sleep(10);require(id&&player.position(id).playing,"Source Environmental Reverb playback actually started");return id;};
    const auto flushes=[&]{return std::count_if(player.calls().begin(),player.calls().end(),[](const auto& c){return c.operation=="Flush retained recording Environmental Reverb after last Stop"&&c.result==S_OK;});};
    player.play(songSource,dir.wstring(),GetDesktopWindow());const auto primary=started();Sleep(150);PlaybackOptions secondary;secondary.secondary=true;
    player.play_extra(songSource,GetDesktopWindow(),{},secondary,primary);const auto extra=started();const auto beforeFlush=flushes();player.stop(primary);
    require(player.position(extra).playing&&player.file_output_active()&&flushes()==beforeFlush,"Shared recording session retains room history when another session stops");
    player.stop(extra);require(player.playback_ids().empty()&&player.file_output_active()&&flushes()==beforeFlush+1,"Last shared Stop flushes Environmental Reverb exactly once without finalizing FileOutput");
    player.play(songSource,dir.wstring(),GetDesktopWindow());started();Sleep(150);player.stop();
    require(player.file_output_active()&&flushes()==beforeFlush+2&&song.save_bytes()==songSource,"Replay Stop flushes the retained source room and preserves native input");
    player.stop_file_output();player.shutdown();
    require(restored.save_bytes()==native,"Source DSP lifecycle leaves canonical native bytes unchanged");
}
