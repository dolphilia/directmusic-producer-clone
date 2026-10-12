void responsive_style_start_tests(const std::filesystem::path& dir){
    const auto base=dir/L"ResponsiveStyle";std::filesystem::create_directories(base);
    StyleDocument style;require(style.new_pattern(L"Pulse",0)&&style.add_band_gm_instrument({},48,0,64,100),"Responsive Style owns Pattern and downloaded GM Band");
    style.set_tempo(120);
    for(int i=0;i<4;++i)require(style.insert_part_note(0,i,{i*4,384,0,UINT32_MAX,48,100}),"Responsive Style owns four beat notes");
    auto root=Chunk::parse(style.save_bytes());
    std::function<void(Chunk&)> fixedChord=[&](Chunk& c){
        if(c.id=="prth"){require(c.data.size()==160,"Responsive Style native Part stride");c.data[150]=2;}
        if(c.id=="note"){require(read32(c.data,0)==24&&c.data.size()==100,"Responsive Style native note stride");for(size_t i=4;i<c.data.size();i+=24)c.data[i+21]=2;}
        for(auto& child:c.children)fixedChord(child);
    };fixedChord(root);style.load(root.encode());style.save((base/L"Pulse.stp").wstring());
    Framework host;const auto si=host.new_segment();auto& segment=host.document(si);
    ChordEvent chord;chord.name=L"M";chord.subchords[0].chordRoot=13;chord.subchords[0].scaleRoot=13;
    require(segment.set_chord(chord)&&segment.set_band(0,style.bands().front().save_bytes()),"Responsive Segment owns transposed Chord and explicit Band Track");
    host.save_segment(si,(base/L"Pulse.sgp").wstring());const auto sti=host.open_style((base/L"Pulse.stp").wstring());
    require(host.assign_style_reference(si,sti,0),"Responsive Segment references owned Style");host.save_segment(si,(base/L"Pulse.sgp").wstring());
    const auto bytes=segment.save_bytes(),styleBytes=host.style_document(sti).save_bytes();
    Conductor player;player.enable_note_observation();player.prepare(nullptr);
    const auto prepare=[&]{for(auto it=player.calls().rbegin();it!=player.calls().rend();++it)if(it->operation.find("Playback prepare milliseconds ")==0)return std::stoul(it->operation.substr(30));throw std::runtime_error("Missing actual runtime prepare observation");};
    for(unsigned attempt=0;attempt<2;++attempt){
        const auto prior=player.observed_notes().size();player.play(bytes,base.wstring(),nullptr,segment.styles());const auto request=player.playback_request();
        require(prepare()==300&&(request.flags&producer::runtime::playAfterPrepareTime),"Independent owned Style uses 300ms and retains AFTERPREPARETIME");
        const auto deadline=GetTickCount64()+5000;while(player.observed_notes().size()<prior+4&&GetTickCount64()<deadline)Sleep(10);
        const auto notes=player.observed_notes();require(notes.size()>=prior+4,"Initial and replay Style generate four actual notes");
        std::cerr<<"responsive attempt="<<attempt<<" actualStart="<<request.actualStart<<" firstNote="<<notes[prior].clocks<<"\n";
        require(notes[prior].clocks==request.actualStart,"Responsive start preserves the first generated beat");
        for(size_t i=prior;i<prior+4;++i){require(notes[i].midiValue==61&&notes[i].musicValue==48&&notes[i].playMode==2,"Responsive generated pitch follows authored C sharp chord");if(i>prior)require(notes[i].clocks-notes[i-1].clocks==768,"Responsive generated notes preserve 120BPM beat clocks");}
        require(player.segment_tempo(0)==120,"Responsive runtime retains authored tempo");player.stop();
    }
    require(!player.note_observation_overflow()&&!player.note_observation_forwarding_failed(),"Responsive observer forwards actual notes without loss");
    // A previously scheduled instance still owns the shared performance clock.
    player.play(bytes,base.wstring(),nullptr,segment.styles());const auto primary=player.current_playback_id();
    player.play_extra(SegmentDocument::playback_test().save_bytes(),nullptr);const auto secondary=player.current_playback_id();
    require(primary!=secondary&&player.playback_ids().size()==2&&prepare()==300,"Secondary plain Segment retains current shared Style preparation");
    player.stop(secondary);require(player.playback_ids().size()==1&&player.current_playback_id()==primary,"Stopping responsive secondary retains the original Style session");player.stop();
    StyleDocument guarded;guarded.load(styleBytes);require(guarded.new_motif(L"Guard")&&guarded.assign_motif_band(1,0),"Lookahead guard owns a standalone Motif and Band");
    player.play_motif({L"",guarded.save_bytes()},L"Guard",nullptr);const auto motifId=player.current_playback_id();
    require(prepare()==1000,"Standalone Motif retains full runtime preparation");
    player.play(bytes,base.wstring(),nullptr,segment.styles());
    require(player.playback_ids().size()==2&&prepare()==1000,"Responsive replacement cannot shorten a retained Motif performance clock");
    player.stop(player.current_playback_id());require(player.current_playback_id()==motifId,"Replacement Stop retains original Motif owner");player.stop();
    require(segment.save_bytes()==bytes&&!segment.dirty()&&host.style_document(sti).save_bytes()==styleBytes&&!host.style_document(sti).dirty(),"Responsive playback leaves native bytes and history checkpoints untouched");
}
void style_player_tests(const std::filesystem::path& dir,const std::wstring& stylePath,const std::wstring& mapPath){
    const auto styleBytes=read_file(stylePath),mapBytes=read_file(mapPath);StyleDocument style;style.load(styleBytes);ChordMapDocument map;map.load(mapBytes);
    const StyleCatalogEntry source{stylePath,styleBytes};std::vector<Bytes> outputs;
    for(WORD shape=0;shape<=8;++shape){StylePlayerSettings settings;settings.shape=static_cast<StyleShape>(shape);settings.measures=8;settings.intro=true;settings.end=true;auto result=compose_style_player_segment(source,mapBytes,settings);SegmentDocument d;d.load(result.segment);
        require(d.length()==8*style.meter().beats*768*4/style.meter().denominator,"Shape uses Style meter and requested measure count");
        require(!d.commands().empty()&&!d.chords().empty(),"Shape produces actual Command and Chord tracks");
        const auto commands=d.commands();require(commands.front().type==2&&commands.back().type==4,"Requested intro and end persist as Command events");
        const auto root=Chunk::parse(result.segment);require(root.find("guid")&&root.find("guid")->data.size()==16,"Generated Segment owns native identity");
        require(styleBytes==read_file(stylePath)&&mapBytes==read_file(mapPath)&&!style.dirty()&&!map.dirty(),"Composition retains full immutable source documents");
        outputs.push_back(result.segment);write_file_atomic((dir/(L"shape-"+std::to_wstring(shape)+L".sgp")).wstring(),result.segment);
    }
    const auto grooves=[&](size_t i){SegmentDocument d;d.load(outputs.at(i));std::vector<unsigned> r;for(const auto& c:d.commands())if(c.type==0)r.push_back(c.groove);return r;};
    const auto quiet=grooves(4),loud=grooves(3),rising=grooves(7);require(!quiet.empty()&&!loud.empty()&&*std::max_element(quiet.begin(),quiet.end())<*std::min_element(loud.begin(),loud.end()),"Quiet and Loud Shape preserve low versus high groove contract");
    require(rising.size()>1&&rising.front()<rising.back(),"Rising Shape increases groove over the Segment");
    StylePlayerSettings settings;settings.shape=StyleShape::Quiet;auto noEnds=compose_style_player_segment(source,mapBytes,settings);SegmentDocument d;d.load(noEnds.segment);const auto noEndCommands=d.commands();require(std::none_of(noEndCommands.begin(),noEndCommands.end(),[](const CommandEvent& c){return c.type==2||c.type==4;}),"Unchecked intro/end do not inject those commands");
    for(unsigned invalid=0;invalid<3;++invalid){auto bad=settings;if(invalid==0)bad.activity=4;if(invalid==1)bad.measures=0;if(invalid==2)bad.shape=static_cast<StyleShape>(9);rejected([&]{compose_style_player_segment(source,mapBytes,bad);},"Invalid Shape settings rejected");}
    require(styleBytes==read_file(stylePath)&&mapBytes==read_file(mapPath),"Rejected composition preserves both full native inputs");
    Framework host;const auto si=host.open_style(stylePath);const auto index=host.adopt_composed_segment(noEnds.segment);require(host.document(index).styles().size()==1&&host.document(index).styles()[0].bytes==host.style_document(si).save_bytes(),"Main Framework adoption resolves owned Style by identity");
    const auto before=host.document(index).save_bytes();require(host.document(index).add_tempo(0,150)&&host.undo_segment(index)&&host.document(index).save_bytes()==before&&host.redo_segment(index),"Composed native Segment retains existing edit Undo/Redo semantics");
    host.save_segment(index,(dir/L"adopted.sgp").wstring());Framework restored;restored.open_style(stylePath);restored.open_segment((dir/L"adopted.sgp").wstring());require(restored.document(0).save_bytes()==host.document(index).save_bytes()&&restored.document(0).tempos()[0].bpm==150,"Saved Shape Segment restores actual tracks and tempo");
    Conductor player;StylePlayerSession session(player,GetDesktopWindow(),source,{},mapBytes,settings);const auto count=session.compositions();auto bad=settings;bad.activity=4;rejected([&]{session.set_settings(bad);},"Audition rejects invalid settings before runtime start");require(session.settings().activity==settings.activity&&session.compositions()==count&&!session.playing(),"Rejected audition change leaves state unchanged");
    settings.measures=16;session.set_settings(settings);require(session.primary_id()!=0&&session.compositions()==count+1&&session.restarts()==1,"Settings change starts a new performance from stopped state");
    session.stop();const auto bands=session.band_names();require(!bands.empty(),"Audition source owns selectable Bands");session.select_band(bands.front());require(session.primary_id()==0&&session.compositions()==count+1&&session.restarts()==1,"Band change while stopped retains the stop and composition");
    session.recompose();require(session.primary_id()!=0&&session.compositions()==count+2&&session.restarts()==2,"Re-Compose starts playback after Stop");session.stop();
    session.set_chordmap(mapBytes);require(session.primary_id()!=0&&session.compositions()==count+3,"ChordMap change starts playback after Stop");session.stop();
    session.set_style(source,{},mapBytes);require(session.primary_id()!=0&&session.compositions()==count+4,"Style change starts playback after Stop");session.stop();
    require(styleBytes==read_file(stylePath)&&mapBytes==read_file(mapPath),"Stopped parameter restarts retain immutable source documents");
    player.shutdown();responsive_style_start_tests(dir);
}
