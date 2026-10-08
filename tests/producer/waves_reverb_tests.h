#pragma once
void waves_reverb_document_tests(const std::filesystem::path& dir){
    const auto leaf=[](const char* name,Bytes data){Chunk c;c.id=name;c.data=std::move(data);return c;};
    AudioBufferId waves{};std::memcpy(waves.data(),&producer::compat::wavesReverbClass,16);
    AudioPathDocument path;auto root=Chunk::parse(path.save_bytes());
    root.children.push_back(leaf("xTop",{1,3,5}));root.children.back().padding=0x91;path.load(root.encode());
    const auto before=path.save_bytes();const auto oldId=path.buffers()[0];
    require(path.add_waves_reverb(0),"Waves default effect materializes owned stereo buffer");
    const auto added=path.save_bytes();const auto details=path.buffer_details();const auto effects=path.effects();
    require(effects.size()==1&&effects[0].classId==waves&&!effects[0].flags&&effects[0].sendBuffer==AudioBufferId{},"Waves uses exact public class without Send or effect options");
    require(details.size()==1&&details[0].id!=oldId&&details[0].channels==2&&details[0].synthBuses==2&&details[0].routed&&!(details[0].flags&2),"Materialized Waves buffer owns route and stereo buses");
    auto expected=root;auto& attributes=expected.find("LIST","dbfl")->find("ddah")->data;
    std::copy(details[0].id.begin(),details[0].id.end(),attributes.begin());put32(attributes,16,0);
    // Explicit public wire layout and the existing owned-stereo policy;
    // only the newly generated GUID is observed from the actual result.
    Bytes expectedDescription(20);put32(expectedDescription,0,0x000182c0);expectedDescription[4]=2;
    Bytes expectedBuses(8);put32(expectedBuses,4,1);Bytes expectedHeader(56);std::copy(waves.begin(),waves.end(),expectedHeader.begin()+4);
    Chunk expectedEffect;expectedEffect.id="RIFF";expectedEffect.type="DSFX";expectedEffect.children={leaf("fxhr",expectedHeader)};
    Chunk expectedEffects;expectedEffects.id="LIST";expectedEffects.type="fxls";expectedEffects.children={expectedEffect};
    Chunk descriptor;descriptor.id="RIFF";descriptor.type="DSBC";descriptor.children={leaf("guid",Bytes(details[0].id.begin(),details[0].id.end())),leaf("dsbd",expectedDescription),leaf("bsid",expectedBuses),expectedEffects};
    expected.find("LIST","dbfl")->children.push_back(descriptor);
    auto& route=expected.find("LIST","pcsl")->children[0].find("LIST","pchl")->children[0].data;
    std::copy(details[0].id.begin(),details[0].id.end(),route.begin()+16);
    require(added==expected.encode(),"Waves materialization changes only owned identity descriptor and route, preserving opaque bytes");
    const auto actual=Chunk::parse(added);const auto fx=actual.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls");
    require(fx&&fx->children.size()==1&&fx->children[0].children.size()==1&&fx->children[0].find("fxhr")->data.size()==56&&!fx->children[0].find("data"),"Factory defaults serialized as fxhr without invented custom parameter bytes");
    require(path.dirty()&&path.undo()&&path.save_bytes()==before&&!path.dirty(),"Waves addition has one exact Undo");
    require(!path.add_waves_reverb(99)&&path.save_bytes()==before&&!path.dirty()&&!path.undo()&&path.redo()&&path.save_bytes()==added,"Absent buffer rejection preserves bytes dirty Undo and Redo");
    require(!path.add_waves_reverb(0)&&path.save_bytes()==added&&path.undo()&&path.save_bytes()==before&&path.redo(),"Duplicate effect is a no-op and preserves history");

    AudioPathDocument custom;require(custom.add_file_output(0),"Custom source fixture owns FileOutput first");
    auto opaque=Chunk::parse(custom.save_bytes());auto ds=opaque.find("LIST","dbfl")->find("RIFF","DSBC");
    ds->children.push_back(leaf("xDsc",{7,9,11}));ds->children.back().padding=0xa3;
    auto& existing=ds->find("LIST","fxls")->children[0];existing.children.push_back(leaf("xFxo",{2,4,6}));existing.children.back().padding=0xb7;
    custom.load(opaque.encode());const auto prior=custom.save_bytes();require(custom.add_waves_reverb(0),"Waves appends after existing FileOutput");
    auto exact=opaque;Bytes header(56);std::copy(waves.begin(),waves.end(),header.begin()+4);Chunk addedEffect;addedEffect.id="RIFF";addedEffect.type="DSFX";addedEffect.children={leaf("fxhr",header)};
    exact.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls")->children.push_back(addedEffect);
    require(custom.save_bytes()==exact.encode(),"Custom buffer addition preserves identity effect order headers opaque chunks padding and flags exactly");
    require(custom.undo()&&custom.save_bytes()==prior&&!custom.dirty()&&custom.redo()&&custom.save_bytes()==exact.encode(),"Custom Waves effect one exact history transaction");
    AudioPathDocument mix;require(mix.add_mixin_buffer(1)&&mix.add_waves_reverb(1),"Waves can be authored on mono mix-in without inventing runtime Send support");
    require(mix.buffer_details()[1].channels==1&&mix.buffer_details()[1].flags==8&&!mix.buffer_details()[1].synthBuses&&!mix.buffer_details()[1].routed,"Mix-in role preserved by effect authoring");
    auto unsupported=root;unsupported.find("LIST","dbfl")->find("ddah")->data[0]^=0x55;auto& r=unsupported.find("LIST","pcsl")->children[0].find("LIST","pchl")->children[0].data;r[16]^=0x55;
    AudioPathDocument unknown;unknown.load(unsupported.encode());require(unknown.set_name(L"Redo")&&unknown.undo(),"Unknown predefined fixture has Redo");
    require(!unknown.add_waves_reverb(0)&&unknown.save_bytes()==unsupported.encode()&&!unknown.dirty()&&!unknown.undo()&&unknown.redo(),"Unconfirmed predefined formats reject atomically without erasing Redo");

    const auto base=dir/L"Waves";std::filesystem::create_directories(base);
    Framework host;const auto ai=host.new_audio_path(),si=host.new_segment();host.audio_path_document(ai).load(custom.save_bytes());host.document(si)=SegmentDocument::playback_test(69);
    require(host.assign_audio_path(si,ai),"Native Segment owns Waves AudioPath");
    host.save_audio_path(ai,(base/L"Effect.aup").wstring());host.save_segment(si,(base/L"Song.sgp").wstring());host.save_project((base/L"Waves.pro").wstring());
    Framework restored;restored.open_project((base/L"Waves.pro").wstring());
    require(restored.audio_path_document(0).save_bytes()==custom.save_bytes()&&restored.document(0).save_bytes()==host.document(si).save_bytes()&&!restored.dirty(),"Native Project fresh Framework reload keeps complete source bytes");
    host.save_runtime(RuntimeDocumentKind::AudioPath,ai,(base/L"Effect.aud").wstring());AudioPathDocument exported;exported.load(read_file((base/L"Effect.aud").wstring()));
    auto exportExpected=exact;auto& exportEffects=exportExpected.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls")->children;exportEffects.erase(exportEffects.begin());
    require(exported.save_bytes()==exportExpected.encode()&&exported.effects().size()==1&&exported.effects()[0].classId==waves&&host.audio_path_document(ai).save_bytes()==custom.save_bytes(),"Runtime export removes only Producer FileOutput and retains OS Waves plus source bytes");
}
void waves_reverb_runtime_tests(const std::filesystem::path& dir){
    const auto waitStarted=[](Conductor& player,const char* phase){
        // AFTERPREPARETIME schedules in the future. Require real IsPlaying
        // within a bound instead of assuming a fixed200ms is the start time.
        const auto request=player.playback_request();const auto id=player.current_playback_id();
        const auto began=GetTickCount64(),deadline=began+5000;auto first=player.position(),position=first;
        while(!position.playing&&GetTickCount64()<deadline){Sleep(10);position=player.position();}
        std::cout<<"{\"phase\":\""<<phase<<"\",\"playbackId\":"<<id<<",\"flags\":"<<request.flags<<",\"submittedClocks\":"<<request.submittedClocks<<",\"requestedClocks\":"<<request.requestedClocks<<",\"actualStart\":"<<request.actualStart<<",\"initialClocks\":"<<first.clocks<<",\"initialPlaying\":"<<(first.playing?"true":"false")<<",\"observedClocks\":"<<position.clocks<<",\"waitMilliseconds\":"<<(GetTickCount64()-began)<<",\"playing\":"<<(position.playing?"true":"false")<<"}\n";
        for(const auto& call:player.calls())if(call.operation.find("Playback prepare milliseconds ")==0)std::cout<<call.operation<<"\n";
        require(id&&position.playing,"Waves actual owned AudioPath playing");return id;
    };
    AudioPathDocument path;require(path.add_file_output(0)&&path.add_waves_reverb(0),"Runtime fixture has FileOutput then Waves to distinguish class index from chain index");
    auto song=SegmentDocument::playback_test(69);while(song.notes().size()>1)require(song.delete_note(song.notes().size()-1),"Reduce to one sustained note");
    require(song.edit_note(0,{0,3072,0,69,100})&&song.set_audio_path(path.save_bytes()),"Runtime note and owned Waves path prepared");
    const auto source=song.save_bytes();song.save((dir/L"WavesInput.sgp").wstring());path.save((dir/L"WavesInput.aup").wstring());
    AudioPathDocument diskPath;diskPath.load(read_file((dir/L"WavesInput.aup").wstring()));SegmentDocument diskSong;diskSong.load(read_file((dir/L"WavesInput.sgp").wstring()));
    require(diskPath.save_bytes()==path.save_bytes()&&diskSong.save_bytes()==source,"Fresh native disk reload preserves Waves effect and playable source");
    Conductor player;rejected([&]{player.waves_reverb_parameters(0,0);},"Uninitialized Waves query cannot create an alternate effect");
    for(int replay=0;replay<2;++replay){
        player.play(replay?diskSong.save_bytes():source,dir.wstring(),GetDesktopWindow());const auto id=waitStarted(player,replay?"disk-replay":"source-play");
        const auto snapshot=player.playback_bytes();
        const auto p=player.waves_reverb_parameters(0,0,0,id);
        require(p.inputGain==0.0f&&p.reverbMix==0.0f&&p.reverbTime==1000.0f&&p.highFrequencyRatio==0.001f,"Actual SDK Waves DMO GetAllParameters returns public defaults");
        rejected([&]{player.waves_reverb_parameters(0,0,1,id);},"Absent second Waves instance not mistaken for FileOutput");
        rejected([&]{player.waves_reverb_parameters(0,0,0,id+1000);},"Missing playback session rejects without fallback");
        require(player.playback_bytes()==snapshot&&song.save_bytes()==source,"Readonly retrieval retains complete private playback snapshot and native source");
        std::cout<<"{\"replay\":"<<replay<<",\"playbackId\":"<<id<<",\"inputGain\":"<<p.inputGain<<",\"reverbMix\":"<<p.reverbMix<<",\"reverbTime\":"<<p.reverbTime<<",\"highFrequencyRatio\":"<<p.highFrequencyRatio<<"}\n";
        player.stop();rejected([&]{player.waves_reverb_parameters(0,0);},"Stopped session query cannot use a stale DMO");
    }
    auto multiRoot=Chunk::parse(path.save_bytes());auto second=*multiRoot.find("LIST","dbfl");GUID fresh{};require(SUCCEEDED(CoCreateGuid(&fresh)),"Second Waves buffer has distinct owned identity");
    Bytes identity(16);std::memcpy(identity.data(),&fresh,16);std::copy(identity.begin(),identity.end(),second.find("ddah")->data.begin());second.find("RIFF","DSBC")->find("guid")->data=identity;
    multiRoot.children.push_back(second);auto& route=multiRoot.find("LIST","pcsl")->children[0].find("LIST","pchl")->children[0].data;put32(route,8,2);route.insert(route.end(),identity.begin(),identity.end());
    AudioPathDocument multi;multi.load(multiRoot.encode());auto multiSong=diskSong;require(multiSong.set_audio_path(multi.save_bytes()),"Runtime fixture has two buffer indices on the same route");
    player.play(multiSong.save_bytes(),dir.wstring(),GetDesktopWindow());const auto multiId=waitStarted(player,"two-buffers");
    const auto firstParameters=player.waves_reverb_parameters(0,0,0,multiId),secondParameters=player.waves_reverb_parameters(0,1,0,multiId);
    require(firstParameters.reverbTime==1000.0f&&secondParameters.inputGain==0.0f&&secondParameters.reverbMix==0.0f&&secondParameters.reverbTime==1000.0f&&secondParameters.highFrequencyRatio==0.001f,"GetObjectInPath resolves Waves from each route buffer index");
    rejected([&]{player.waves_reverb_parameters(0,2,0,multiId);},"Missing buffer index cannot reuse a different buffer effect");player.stop();
    auto sustained=diskSong;auto sustainedRoot=Chunk::parse(sustained.save_bytes());put32(sustainedRoot.find("segh")->data,4,122880);sustained.load(sustainedRoot.encode());
    require(sustained.edit_note(0,{0,122880,0,69,100}),"Recording Waves fixture sustains through explicit Stop");
    const auto sustainedSource=sustained.save_bytes(),recordingPath=path.save_bytes();
    player.start_file_output(recordingPath,(dir/L"WavesRecording.wav").wstring(),GetDesktopWindow());
    const auto flushes=[&]{return std::count_if(player.calls().begin(),player.calls().end(),[](const auto& c){return c.operation=="Flush retained recording Waves after last Stop"&&c.result==S_OK;});};
    player.play(sustainedSource,dir.wstring(),GetDesktopWindow());const auto primary=waitStarted(player,"recording-primary");
    PlaybackOptions secondary;secondary.secondary=true;
    player.play_extra(sustainedSource,GetDesktopWindow(),{},secondary,primary);const auto shared=waitStarted(player,"recording-shared");
    const auto beforeFlush=flushes();player.stop(primary);
    require(player.position(shared).playing&&player.file_output_active()&&flushes()==beforeFlush,"Stopping one shared session keeps the other audible without flushing its Waves history");
    const auto parameters=player.waves_reverb_parameters(0,0,0,shared);
    require(parameters.inputGain==0.0f&&parameters.reverbMix==0.0f&&parameters.reverbTime==1000.0f&&parameters.highFrequencyRatio==0.001f,"Recording obtains the same Waves DMO without changing public defaults");
    player.stop(shared);const auto stoppedSize=std::filesystem::file_size(dir/L"WavesRecording.wav");Sleep(300);
    require(player.playback_ids().empty()&&player.file_output_active()&&flushes()==beforeFlush+1&&std::filesystem::file_size(dir/L"WavesRecording.wav")>stoppedSize,"Last musical Stop flushes the retained Waves DMO once while FileOutput continues");
    player.play(sustainedSource,dir.wstring(),GetDesktopWindow());const auto recordedReplay=waitStarted(player,"recording-replay");
    const auto replayParameters=player.waves_reverb_parameters(0,0,0,recordedReplay);
    require(replayParameters.reverbTime==1000.0f&&replayParameters.reverbMix==0.0f&&replayParameters.inputGain==0.0f&&replayParameters.highFrequencyRatio==0.001f,"Replay after Flush retains the exact factory parameters");
    player.stop();require(flushes()==beforeFlush+2&&player.file_output_active(),"Replay Stop flushes retained Waves without finalizing recording");
    player.stop_file_output();const auto recorded=Chunk::parse(read_file((dir/L"WavesRecording.wav").wstring()));
    require(!player.file_output_active()&&recorded.find("data")&&recorded.find("data")->data.size()>0&&sustained.save_bytes()==sustainedSource&&path.save_bytes()==recordingPath,"Recording finalizes and retains native inputs after shared Stop and replay");
    player.shutdown();require(!player.initialized()&&song.save_bytes()==source,"Waves replay cleanup and native source unchanged");
}
