void band_mix_runtime_tests(const std::filesystem::path& dir){
    auto song=SegmentDocument::create_empty({},L"Live Band mix");song.add_tracks({SegmentTrackKind::band,SegmentTrackKind::sequence});song.insert_default_band(0);assign_sequence_gm_patch(song,0,0,16);song.add_note({0,30000,0,60,96});require(song.add_sequence_part(1),"Runtime fixture contains source-owned empty part metadata without sounding events");song.load(song.save_bytes());const auto bytes=song.save_bytes();
    Conductor player;player.enable_midi_observation();player.play(bytes,L"",GetDesktopWindow());const auto first=player.current_playback_id();
    const auto wait=[&](PlaybackId id){const auto end=GetTickCount64()+5000;while(!player.position(id).playing){if(GetTickCount64()>end)throw std::runtime_error("Live Band fixture start timed out");Sleep(10);}};wait(first);
    player.play_extra(bytes,GetDesktopWindow());const auto second=player.current_playback_id();wait(second);
    const auto firstMapped=player.convert_pchannel(first,0),secondMapped=player.convert_pchannel(second,0);
    require(first!=second&&firstMapped!=secondMapped,"Live mixer test owns distinct simultaneous routes for the same local PChannel");
    const auto beforeInvalid=player.calls().size();unsigned refused=0;
    for(const auto& values:std::vector<std::vector<BandMixValue>>{{},{{0,128,10}},{{0,10,128}},{{0,10,10},{0,20,20}},{{0,10,10},{0xfffffffcu,20,20}},{{0xfffffffdu,0,0}},{{0xfffffffeu,0,0}},{{0xffffffffu,0,0}}})try{player.set_band_mix(first,values);}catch(const std::exception&){++refused;}
    require(refused==8&&player.calls().size()==beforeInvalid,"Entire invalid mix batches fail before any runtime call, preserving ongoing playback");
    const auto sends=[&]{return std::count_if(player.calls().begin(),player.calls().end(),[](const auto& c){return c.operation=="Send live Band mixer MIDI";});};
    const auto received=[&](DWORD channel,BYTE cc,BYTE value){const auto messages=player.observed_midi();return std::any_of(messages.begin(),messages.end(),[&](const auto& m){return m.channel==channel&&(m.status&0xf0)==0xb0&&m.byte1==cc&&m.byte2==value;});};
    const auto waitCC=[&](DWORD channel,BYTE pan,BYTE volume){const auto end=GetTickCount64()+3000;while(!received(channel,7,volume)||!received(channel,10,pan)){if(GetTickCount64()>end)throw std::runtime_error("Live Band MIDI messages did not traverse the Performance graph");Sleep(10);}};
    player.set_band_mix(first,{{0,23,37}});waitCC(firstMapped,23,37);
    require(sends()==2&&player.current_playback_id()==second&&player.primary_playback_id()==first&&player.position(first).playing&&player.position(second).playing,"Committed Band edit reaches the retained primary without restart or changing current secondary ownership");
    require(!received(secondMapped,7,37)&&!received(secondMapped,10,23),"Primary mixer messages do not spill into the other local-PChannel0 route");
    player.set_band_mix(second,{{0,99,111}});waitCC(secondMapped,99,111);
    require(sends()==4&&player.position(first).playing&&player.position(second).playing,"Independent secondary mix uses its own converted Performance PChannel");
    const auto beforeUnknown=sends();rejected([&]{player.set_band_mix(0,{{0,0,0}});},"Zero playback ID is never current-session shorthand for mixer writes");rejected([&]{player.set_band_mix(second+10000,{{0,0,0}});},"Unknown mixer playback ID cannot write to the current session");
    require(sends()==beforeUnknown&&player.playback_bytes()==bytes&&song.save_bytes()==bytes&&!song.dirty()&&!song.can_undo(),"Live runtime mix leaves native bytes and source history untouched");
    player.stop(first);rejected([&]{player.set_band_mix(first,{{0,0,0}});},"Stopped mixer route cannot fall back to the remaining session");
    require(player.position(second).playing&&sends()==beforeUnknown,"Stopping the edited primary retains the independently mixed secondary");
    require(!player.position(second).tempoAvailable,"A surviving noncontrol secondary reports an unavailable Tempo map without failing playback observation");
    require(!player.note_observation_overflow()&&!player.note_observation_forwarding_failed(),"MIDI observer forwards actual messages without loss");
    const auto messages=player.observed_midi();player.stop();player.shutdown();require(player.playback_ids().empty(),"Live mix sessions normally release all runtime ownership");
    std::string observed;for(const auto& m:messages){if(!observed.empty())observed+=",";observed+="{\"channel\":"+std::to_string(m.channel)+",\"status\":"+std::to_string(m.status)+",\"cc\":"+std::to_string(m.byte1)+",\"value\":"+std::to_string(m.byte2)+"}";}
    const auto report="{\"passed\":true,\"primaryMapped\":"+std::to_string(firstMapped)+",\"secondaryMapped\":"+std::to_string(secondMapped)+",\"midi\":["+observed+"],\"scope\":\"Actual CC7/10 graph delivery and concurrent route ownership; PCM and main UI separate\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"band-mix-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void sequence_audition_runtime_tests(const std::filesystem::path& dir){
    band_mix_runtime_tests(dir);
    auto source=SegmentDocument::create_empty({false,20,40000},L"Preview ownership");
    source.add_tracks({SegmentTrackKind::band,SegmentTrackKind::sequence});source.insert_default_band(0);
    require(assign_sequence_gm_patch(source,0,0,16),"Create Organ preview fixture");
    require(source.add_note({0,192,0,60,96}),"Create preview note");
    source.load(source.save_bytes());const auto original=source.save_bytes();
    const auto organ=sequence_audition_snapshot(source,source.notes().front());
    auto transportSong=SegmentDocument::playback_test(60);
    Conductor transport,preview;transport.play(transportSong.save_bytes(),L"",GetDesktopWindow());
    const auto transportId=transport.current_playback_id();
    auto wait=[](Conductor& owner){const auto until=GetTickCount64()+5000;while(!owner.position().playing){if(GetTickCount64()>until)throw std::runtime_error("Preview runtime start timed out");Sleep(10);}};
    wait(transport);
    const auto primeBegin=GetTickCount64();preview.prepare_note_audition(organ,GetDesktopWindow());const auto primeMs=GetTickCount64()-primeBegin;
    require(preview.initialized()&&preview.playback_ids().empty()&&!preview.current_playback_id(),"Silent preparation publishes no playback identity");
    const auto count=[&](const char* op){return std::count_if(preview.calls().begin(),preview.calls().end(),[&](const auto& call){return call.operation==op;});};
    require(count("Download segment")==1&&count("PlaySegmentEx")==0,"Silent preparation downloads instruments without scheduling notes");
    const auto beforeRepeat=preview.calls().size();preview.prepare_note_audition(organ,GetDesktopWindow());
    require(preview.calls().size()==beforeRepeat,"Repeated preparation retains the existing private owner");
    require(transport.current_playback_id()==transportId&&transport.position().playing,"Preview preparation leaves Transport playing");
    const auto firstBegin=GetTickCount64();preview.play_note_audition(organ,GetDesktopWindow());const auto firstMs=GetTickCount64()-firstBegin;wait(preview);
    require(preview.current_playback_id()!=0&&preview.playback_bytes()==organ,"First preview plays its own current snapshot");
    require(count("Create standard AudioPath")==1&&count("PlaySegmentEx")==1,"First preview reuses the silently prepared AudioPath");
    preview.stop();require(preview.playback_ids().empty(),"Preview Stop removes public playback instances");
    require(transport.current_playback_id()==transportId&&transport.position().playing,"Preview Stop leaves Transport primary intact");
    require(assign_sequence_gm_patch(source,0,0,0),"Change current preview instrument to Piano");
    const auto piano=sequence_audition_snapshot(source,{0,192,0,62,43});
    const auto replayBegin=GetTickCount64();preview.play_note_audition(piano,GetDesktopWindow());const auto replayMs=GetTickCount64()-replayBegin;wait(preview);
    require(count("Create standard AudioPath")==1&&count("Download segment")==3&&preview.playback_bytes()==piano,"Replay downloads the new instrument while retaining its private path");
    preview.stop();preview.shutdown();
    require(!preview.initialized()&&preview.playback_ids().empty(),"Preview shutdown releases its prepared and active resources");
    require(transport.current_playback_id()==transportId&&transport.position().playing,"Preview shutdown leaves separate Transport running");
    transport.stop();transport.shutdown();
    require(source.undo()&&source.save_bytes()==original&&!source.dirty()&&!source.can_undo(),"Preview calls preserve source bytes and document history");
    preview.prepare_note_audition(organ,GetDesktopWindow());preview.play_note_audition(organ,GetDesktopWindow());wait(preview);preview.stop();preview.shutdown();
    require(!preview.initialized()&&count("Create standard AudioPath")==2,"A closed preview can silently prepare a new runtime");
    const std::string report="{\"passed\":true,\"primeApiMilliseconds\":"+std::to_string(primeMs)+",\"firstPlayApiMilliseconds\":"+std::to_string(firstMs)+",\"replayApiMilliseconds\":"+std::to_string(replayMs)+",\"scope\":\"No-Play preparation, resource ownership, current patch and independent Transport; PCM onset and timbre require separate main capture\",\"fullAcceptance\":false}\n";
    write_file_atomic((dir/L"sequence-audition-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
