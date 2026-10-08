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
}
