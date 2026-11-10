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
void finite_segment_loop_runtime_tests(const std::filesystem::path& dir){
    std::string cases;
    for(const auto repeats:{0u,2u,UINT32_MAX}){
        auto song=SegmentDocument::create_empty({false,1,2000},L"Finite loop runtime");song.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::band,SegmentTrackKind::sequence});song.insert_default_band(0);assign_sequence_gm_patch(song,0,0,16);
        require(song.add_note({0,192,0,72,96})&&song.add_note({576,384,0,72,96})&&song.edit_note_logical_time(1,768),"Finite runtime fixture owns pilot and an early sounding note logically at loop start");
        require(song.set_music_loop(repeats,768,1536),"Finite repeat count and exact native loop bounds commit through the document API");song.load(song.save_bytes());const auto source=song.save_bytes();
        Conductor player;player.enable_note_observation();player.play(source,L"",GetDesktopWindow());const auto request=player.playback_request();
        const bool infinite=repeats==UINT32_MAX;const auto deadline=GetTickCount64()+8000;bool started=false,ended=false,stableStart=true,monotonic=true,outro=false;std::int64_t lastElapsed=-1;std::optional<std::int32_t> previous;unsigned wraps=0,samples=0;
        while(GetTickCount64()<deadline){const auto position=player.position();if(position.playing){started=true;const auto elapsed=std::int64_t(position.clocks)-position.start;
            if(elapsed>=0){stableStart=stableStart&&position.start==request.actualStart;monotonic=monotonic&&elapsed>=lastElapsed;lastElapsed=elapsed;
                const auto mapped=segment_playback_clock(song.length(),song.loop_settings(),0,elapsed);if(!mapped)throw std::runtime_error("Native loop cursor mapping unavailable");
                if(previous&&*mapped<*previous)++wraps;previous=mapped;++samples;if(repeats==2&&elapsed>=3072&&*mapped>=1536)outro=true;
                if(infinite&&elapsed>=3840)break;
            }
        }else if(started){ended=true;break;}Sleep(10);}
        require(started&&(infinite?!ended&&lastElapsed>=3840:ended),"Finite Segment naturally ends and Infinite remains playing beyond the original source length");
        require(stableStart&&monotonic&&samples>20,"Native GetStartTime stays at the resolved initial start across repeats while performance elapsed time advances monotonically");
        require(infinite?wraps>=4:wraps==repeats,"Observed mapped cursor crosses exactly the finite repeat boundaries or at least four Infinite boundaries");
        require(repeats!=2||outro,"Finite Repeat2 reaches its source outro after both repeated loop passes");
        if(infinite)player.stop();
        const auto notes=player.observed_notes();require(infinite?notes.size()>=5:notes.size()==repeats+2,"Native playback retains the pilot and the required finite or Infinite loop target occurrences");
        require(notes[0].clocks==request.actualStart&&notes[0].midiValue==72&&notes[0].velocity==96,"Finite loop pilot uses the resolved primary start and source pitch/velocity");
        for(size_t i=1;i<notes.size();++i)require(notes[i].clocks-request.actualStart==576+static_cast<LONG>(i-1)*768&&notes[i].midiValue==72&&notes[i].velocity==96&&notes[i].duration==384,"Every finite target occurrence retains physical offset duration pitch velocity and exact native loop spacing");
        bool fullPrepare=false;for(const auto& call:player.calls())if(call.operation=="Playback prepare milliseconds 1000")fullPrepare=true;
        require(fullPrepare&&!player.note_observation_overflow()&&!player.note_observation_forwarding_failed(),"Negative-offset finite playback has full preparation and forwards every observed note");
        require(song.save_bytes()==source&&!song.dirty()&&!song.can_undo(),"Finite runtime loading and natural completion preserve source bytes and clean history");
        if(!cases.empty())cases+=",";cases+="{\"repeats\":"+std::to_string(repeats)+",\"actualStart\":"+std::to_string(request.actualStart)+",\"noteCount\":"+std::to_string(notes.size())+",\"naturalEnd\":"+(ended?"true":"false")+",\"cursorSamples\":"+std::to_string(samples)+",\"cursorWraps\":"+std::to_string(wraps)+",\"lastElapsed\":"+std::to_string(lastElapsed)+",\"stableNativeStart\":true,\"finiteOutro\":"+(outro?"true":"false")+",\"relativeClocks\":[";
        for(size_t i=0;i<notes.size();++i){if(i)cases+=",";cases+=std::to_string(notes[i].clocks-request.actualStart);}cases+="]}";
        player.stop();player.shutdown();require(!player.initialized()&&player.playback_ids().empty(),"Naturally completed finite loop releases owned runtime normally");
    }
    const auto report="{\"passed\":true,\"cases\":["+cases+"],\"scope\":\"Actual native Repeat0 and2 natural end plus Infinite explicit stop; note spacing and stable native start/cursor wraps/outro; main GUI PCM and original comparison separate\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"finite-loop-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void segment_start_point_runtime_tests(const std::filesystem::path& dir){
    std::string cases;
    for(unsigned variant=0;variant<4;++variant){
        const bool infinite=variant==3;const auto repeats=infinite?UINT32_MAX:2u;const LONG start=variant==1?0:384;
        const std::optional<LONG> explicitStart=variant==1?std::optional<LONG>(0):variant==2?std::optional<LONG>(384):std::nullopt;
        auto song=SegmentDocument::create_empty({false,1,2000},L"Configured start runtime");song.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::band,SegmentTrackKind::sequence});song.insert_default_band(0);assign_sequence_gm_patch(song,0,0,16);
        require(song.add_note({0,192,0,72,96})&&song.add_note({576,384,0,72,96})&&song.edit_note_logical_time(1,768)&&song.set_music_loop(repeats,768,1536)&&song.set_music_start(384),"Start fixture owns distinct header start Loop Start and negative-offset target");
        song.load(song.save_bytes());const auto source=song.save_bytes();Conductor player;player.enable_note_observation();player.play(source,L"",GetDesktopWindow(),{},{},{},{},{},{},explicitStart);const auto request=player.playback_request();
        const auto deadline=GetTickCount64()+8000;bool started=false,ended=false,stable=true,outro=false;unsigned samples=0,wraps=0;std::int64_t last=-1;std::optional<std::int32_t> previous;
        while(GetTickCount64()<deadline){const auto p=player.position();if(p.playing){started=true;const auto elapsed=std::int64_t(p.clocks)-p.start;if(elapsed>=0){stable=stable&&p.start==request.actualStart&&elapsed>=last;last=elapsed;const auto mapped=segment_playback_clock(song.length(),song.loop_settings(),start,elapsed);if(!mapped)throw std::runtime_error("Configured start cursor mapping unavailable");if(previous&&*mapped<*previous)++wraps;previous=mapped;++samples;if(!infinite&&elapsed>=1536-start+1536&&*mapped>=1536)outro=true;if(infinite&&elapsed>=3840)break;}}else if(started){ended=true;break;}Sleep(10);}
        const auto notes=player.observed_notes();const auto setCalls=std::count_if(player.calls().begin(),player.calls().end(),[](const auto& call){return call.operation=="Set editor playback start point";});bool fullPrepare=false;for(const auto& call:player.calls())if(call.operation=="Playback prepare milliseconds 1000")fullPrepare=true;
        const bool observationComplete=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.stop();player.shutdown();
        std::string observed="{\"variant\":"+std::to_string(variant)+",\"headerStart\":384,\"requestedExplicit\":"+(explicitStart?std::to_string(*explicitStart):"null")+",\"nativeStart\":"+std::to_string(request.startPoint)+",\"actualStart\":"+std::to_string(request.actualStart)+",\"naturalEnd\":"+(ended?"true":"false")+",\"stableNativeStart\":"+(stable?"true":"false")+",\"cursorSamples\":"+std::to_string(samples)+",\"cursorWraps\":"+std::to_string(wraps)+",\"finiteOutro\":"+(outro?"true":"false")+",\"lastElapsed\":"+std::to_string(last)+",\"setStartCalls\":"+std::to_string(setCalls)+",\"relativeClocks\":[";
        for(size_t i=0;i<notes.size();++i){if(i)observed+=",";observed+=std::to_string(notes[i].clocks-request.actualStart);}observed+="]}";
        const auto raw="{\"observed\":true,\"acceptanceEvaluated\":false,\"case\":"+observed+",\"fullAcceptance\":false}\n";write_file_atomic((dir/(L"start-point-observed-"+std::to_wstring(variant)+L".json")).wstring(),Bytes(raw.begin(),raw.end()));
        require(request.startPoint==start&&setCalls==(explicitStart?1:0),"Native header default and explicit0 or explicit seek are independently distinguished by GetStartPoint and SetStartPoint calls");
        require(started&&(infinite?!ended&&last>=3840:ended)&&stable&&samples>20,"Configured start has stable resolved native origin and naturally ends only for finite playback");
        require(infinite?wraps>=4:wraps==2&&outro,"Configured start changes only initial pass while finite and Infinite cursor use the retained loop bounds");
        require(infinite?notes.size()>=4:notes.size()==(start?3u:4u),"Nonzero configured start omits the earlier pilot and retains each finite or Infinite loop target");
        for(size_t i=0;i<notes.size();++i){const LONG expected=start?192+static_cast<LONG>(i)*768:i?576+static_cast<LONG>(i-1)*768:0;require(notes[i].clocks-request.actualStart==expected&&notes[i].midiValue==72&&notes[i].velocity==96&&notes[i].duration==(!start&&!i?192:384),"Actual native note timestamps preserve physical lead and duration after configured or explicit initial seek");}
        require(fullPrepare&&observationComplete&&song.save_bytes()==source&&!song.dirty()&&!song.can_undo()&&!player.initialized(),"Start runtime retains full negative-offset preparation clean source and normal release");
        if(!cases.empty())cases+=",";cases+=observed;
    }
    const auto report="{\"passed\":true,\"cases\":["+cases+"],\"scope\":\"Native configured header start384 vs explicit0/384 with finite2 and Infinite; actual MIDI timestamps native origin and loop cursor; PCM/main GUI separate\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"start-point-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void segment_extended_boundary_runtime_tests(const std::filesystem::path& dir){
    std::string cases;const SegmentPerformanceBoundary choices[]={SegmentPerformanceBoundary::immediate,SegmentPerformanceBoundary::grid,SegmentPerformanceBoundary::beat,SegmentPerformanceBoundary::barline,SegmentPerformanceBoundary::align_beat,SegmentPerformanceBoundary::align_barline,SegmentPerformanceBoundary::align_segment,SegmentPerformanceBoundary::segment_end,SegmentPerformanceBoundary::queue_end};
    const DWORD words[]={0x11000400,0x11000c00,0x11001400,0x11002400,0x10011400,0x10012400,0x10810400,0x11800400,0x10000500};
    const SegmentAlignment alignChoices[]={SegmentAlignment::time,SegmentAlignment::grid,SegmentAlignment::beat,SegmentAlignment::barline};const DWORD alignBits[]={0x80000,0x40000,0x20000,0x8000000};
    const auto songFor=[&](unsigned i){auto song=SegmentDocument::create_empty({false,1,2000},L"Extended stored boundary runtime");song.add_editor_tracks({SegmentTrackKind::tempo});song.set_performance_boundary(SegmentPerformanceBoundary::beat);song.set_boundary_marker(true);song.set_boundary_after_prepare(true);song.set_invalidation(SegmentInvalidation::cut_primary);song.set_performance_boundary(choices[i]);return song;};
    const auto verify=[&](SegmentDocument song,unsigned choice,int alignment,DWORD expected){song.load(song.save_bytes());const auto source=song.save_bytes();Conductor player;player.play(source,L"",GetDesktopWindow());const auto request=player.playback_request();
        require(request.runtimeDefaultResolution==expected,"Original nine boundaries and explicit valid-start choices reach native GetDefaultResolution with retained timing and Invalidation");
        require(player.playback_bytes()==source&&song.save_bytes()==source&&!song.dirty()&&!song.can_undo(),"Extended Boundary native load uses source-owned resolution without altering bytes or clean history");
        player.stop();player.shutdown();require(!player.initialized()&&player.playback_ids().empty(),"Extended Boundary readback releases native session normally");
        if(!cases.empty())cases+=",";cases+="{\"boundary\":"+std::to_string(choice)+",\"alignment\":"+std::to_string(alignment)+",\"expectedResolution\":"+std::to_string(expected)+",\"runtimeResolution\":"+std::to_string(request.runtimeDefaultResolution)+"}";
    };
    for(unsigned i=0;i<9;++i){verify(songFor(i),i,-1,words[i]);if(i>=4&&i<=6)for(unsigned j=0;j<4;++j){auto song=songFor(i);if(j==0)song.set_alignment(SegmentAlignment::grid);song.set_alignment(alignChoices[j]);verify(std::move(song),i,static_cast<int>(j),words[i]|alignBits[j]);}}
    const auto report="{\"passed\":true,\"cases\":["+cases+"],\"scope\":\"Native GetDefaultResolution readback21 of nine boundaries and four explicit valid-start bits for three Align choices; no transition phase queue timing sounded cutoff regeneration PCM or original dynamic acceptance\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"extended-boundary-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void segment_invalidation_runtime_tests(const std::filesystem::path& dir){
    std::string cases;const SegmentInvalidation choices[]={SegmentInvalidation::dont_cut_off,SegmentInvalidation::cut_primary,SegmentInvalidation::cut_all};const DWORD bits[]={0x8000,0x10000000,0};
    for(unsigned i=0;i<3;++i){auto song=SegmentDocument::create_empty({false,1,2000},L"Stored invalidation runtime");song.add_editor_tracks({SegmentTrackKind::tempo});song.set_performance_boundary(SegmentPerformanceBoundary::beat);song.set_boundary_marker(true);song.set_boundary_after_prepare(true);song.set_invalidation(choices[i]);song.load(song.save_bytes());const auto source=song.save_bytes();
        Conductor player;player.play(source,L"",GetDesktopWindow());const auto request=player.playback_request();const auto expected=0x1001400|bits[i];
        require(request.runtimeDefaultResolution==expected,"Each original invalidation radio value reaches native GetDefaultResolution with Beat Marker and AfterPrepare retained");
        require(player.playback_bytes()==source&&song.save_bytes()==source&&!song.dirty()&&!song.can_undo(),"Invalidation native load retains source-owned bytes and clean history without a private override");
        player.stop();player.shutdown();require(!player.initialized()&&player.playback_ids().empty(),"Invalidation readback session normally releases runtime ownership");
        if(!cases.empty())cases+=",";cases+="{\"variant\":"+std::to_string(i)+",\"expectedResolution\":"+std::to_string(expected)+",\"runtimeResolution\":"+std::to_string(request.runtimeDefaultResolution)+"}";
    }
    const auto report="{\"passed\":true,\"cases\":["+cases+"],\"scope\":\"Actual native GetDefaultResolution readback of three source-owned invalidation choices; no sounded cutoff secondary regeneration or transition acceptance\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"segment-invalidation-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void segment_boundary_runtime_tests(const std::filesystem::path& dir){
    std::string cases;const SegmentPerformanceBoundary boundaries[]={SegmentPerformanceBoundary::immediate,SegmentPerformanceBoundary::grid,SegmentPerformanceBoundary::beat,SegmentPerformanceBoundary::barline};const LONG units[]={1,192,768,3072};const DWORD flags[]={0,0x800,0x1000,0x2000};
    for(unsigned variant=0;variant<4;++variant){
        auto song=SegmentDocument::create_empty({false,4,8000},L"Stored boundary runtime");song.add_editor_tracks({SegmentTrackKind::tempo});song.set_performance_boundary(boundaries[variant]);song.load(song.save_bytes());const auto source=song.save_bytes();
        Conductor player;player.play(source,L"",GetDesktopWindow());const auto id=player.current_playback_id();const auto request=player.playback_request();const LONG unit=units[variant];std::vector<PlaybackNotification> messages;
        const auto drain=[&]{auto next=player.notifications();messages.insert(messages.end(),next.begin(),next.end());};
        const auto deadline=GetTickCount64()+5000;PlaybackPosition before{};
        do{before=player.position(id);if(before.playing&&before.clocks>=before.start)break;if(GetTickCount64()>deadline)throw std::runtime_error("Stored boundary start timed out");Sleep(2);}while(true);
        if(unit>1){const LONG elapsed=before.clocks-before.start,target=before.start+(elapsed/unit+(elapsed%unit>=unit/2?1:0))*unit+unit/2;
            do{before=player.position(id);if(!before.playing)throw std::runtime_error("Stored boundary fixture ended before Stop");if(before.clocks>=target)break;if(GetTickCount64()>deadline)throw std::runtime_error("Stored boundary midpoint timed out");Sleep(2);}while(true);}
        drain();const auto clockBefore=player.performance_clocks();before=player.position(id);const auto began=GetTickCount64();player.stop_at_boundary(id);const auto returnedMs=GetTickCount64()-began;const auto clockAfter=player.performance_clocks();const auto after=player.position(id);auto ended=after,lastPlaying=before;drain();
        const auto stopDeadline=GetTickCount64()+4000;
        while(ended.playing){lastPlaying=ended;if(GetTickCount64()>stopDeadline)throw std::runtime_error("Stored boundary Stop timed out");Sleep(2);ended=player.position(id);drain();}
        const auto notificationWaitStart=GetTickCount64();const auto terminalArrived=[&]{return std::any_of(messages.begin(),messages.end(),[&](const auto& message){return message.playbackId==id&&IsEqualGUID(message.type,runtime::segmentNotification)&&(message.option==1||message.option==4);});};
        drain();while(!terminalArrived()&&GetTickCount64()-notificationWaitStart<500){Sleep(2);drain();}const auto notificationWaitMs=GetTickCount64()-notificationWaitStart;std::vector<LONG> terminalTimes;
        size_t ownedTerminalCount=0;for(const auto& message:messages)if(message.playbackId==id&&IsEqualGUID(message.type,runtime::segmentNotification)&&(message.option==1||message.option==4)){++ownedTerminalCount;if(message.resolvedClocks)terminalTimes.push_back(*message.resolvedClocks);}
        const auto next=[&](LONG clock){return unit==1?clock:request.actualStart+((clock-request.actualStart)/unit+1)*unit;};const auto currentExpectedFirst=next(before.clocks),currentExpectedLast=next(after.clocks);
        const auto expectedFirst=variant?clockBefore.boundariesAtLatency[variant-1]:clockBefore.latency,expectedLast=variant?clockAfter.boundariesAtLatency[variant-1]:clockAfter.latency;
        const bool readback=request.runtimeDefaultResolution==flags[variant]&&request.startPoint==0;
        const bool statePollTiming=ended.clocks>=currentExpectedFirst-32&&ended.clocks<=currentExpectedLast+64&&lastPlaying.clocks<=currentExpectedLast+64;
        const bool currentTimeNotificationTiming=terminalTimes.size()==1&&terminalTimes[0]>=currentExpectedFirst-32&&terminalTimes[0]<=currentExpectedLast+64;
        const bool terminalTimesValid=ownedTerminalCount!=0&&terminalTimes.size()==ownedTerminalCount;
        const bool timing=clockBefore.latency>=clockBefore.now&&clockAfter.latency>=clockAfter.now&&clockAfter.latency>=clockBefore.latency&&terminalTimesValid&&std::all_of(terminalTimes.begin(),terminalTimes.end(),[&](LONG clock){return clock>=expectedFirst-32&&clock<=expectedLast+64;});
        std::string notificationJson;for(const auto& message:messages){if(!notificationJson.empty())notificationJson+=",";notificationJson+="{\"id\":"+std::to_string(message.playbackId)+",\"segment\":"+(IsEqualGUID(message.type,runtime::segmentNotification)?"true":"false")+",\"option\":"+std::to_string(message.option)+",\"timeFlags\":"+std::to_string(message.timeFlags)+",\"rawMusic\":"+std::to_string(message.clocks)+",\"referenceTime\":"+std::to_string(message.referenceTime)+",\"resolvedMusic\":"+(message.resolvedClocks?std::to_string(*message.resolvedClocks):"null")+"}";}
        const auto clockJson=[](const PerformanceClockSnapshot& c){return "{\"nowReference\":"+std::to_string(c.nowReference)+",\"latencyReference\":"+std::to_string(c.latencyReference)+",\"queueReference\":"+std::to_string(c.queueReference)+",\"now\":"+std::to_string(c.now)+",\"latency\":"+std::to_string(c.latency)+",\"queue\":"+std::to_string(c.queue)+",\"grid\":"+std::to_string(c.boundariesAtLatency[0])+",\"beat\":"+std::to_string(c.boundariesAtLatency[1])+",\"measure\":"+std::to_string(c.boundariesAtLatency[2])+"}";};
        const auto observed="{\"variant\":"+std::to_string(variant)+",\"defaultResolution\":"+std::to_string(request.runtimeDefaultResolution)+",\"expectedResolution\":"+std::to_string(flags[variant])+",\"unit\":"+std::to_string(unit)+",\"actualStart\":"+std::to_string(request.actualStart)+",\"before\":"+std::to_string(before.clocks)+",\"after\":"+std::to_string(after.clocks)+",\"lastPlaying\":"+std::to_string(lastPlaying.clocks)+",\"ended\":"+std::to_string(ended.clocks)+",\"expectedFirst\":"+std::to_string(expectedFirst)+",\"expectedLast\":"+std::to_string(expectedLast)+",\"currentExpectedFirst\":"+std::to_string(currentExpectedFirst)+",\"currentExpectedLast\":"+std::to_string(currentExpectedLast)+",\"returnedMs\":"+std::to_string(returnedMs)+",\"notificationWaitMs\":"+std::to_string(notificationWaitMs)+",\"notificationWaitLimitMs\":500,\"readback\":"+(readback?"true":"false")+",\"timing\":"+(timing?"true":"false")+",\"statePollTiming\":"+(statePollTiming?"true":"false")+",\"currentTimeNotificationTiming\":"+(currentTimeNotificationTiming?"true":"false")+",\"timingBasis\":\"native-latency-horizon\",\"clockBefore\":"+clockJson(clockBefore)+",\"clockAfter\":"+clockJson(clockAfter)+",\"notifications\":["+notificationJson+"]}";
        const auto raw="{\"observed\":true,\"acceptanceEvaluated\":false,\"case\":"+observed+",\"fullAcceptance\":false}\n";write_file_atomic((dir/(L"boundary-observed-"+std::to_wstring(variant)+L".json")).wstring(),Bytes(raw.begin(),raw.end()));
        require(readback,"Source-owned Boundary edits reach native GetDefaultResolution without private runtime overrides");
        require(returnedMs<=100&&timing,"StopEx returns promptly and every owned terminal has a valid time within the observed native latency boundary with unchanged observation allowance");
        require(song.save_bytes()==source&&!song.dirty()&&!song.can_undo()&&player.playback_bytes()==source,"Boundary playback and asynchronous Stop preserve loaded source and clean history");
        player.stop();player.shutdown();require(!player.initialized()&&player.playback_ids().empty(),"Stored boundary Stop releases runtime ownership normally");if(!cases.empty())cases+=",";cases+=observed;
    }
    const auto report="{\"passed\":true,\"cases\":["+cases+"],\"scope\":\"Native default-resolution and valid terminal Stop times against independently observed GetLatencyTime/GetResolvedTime horizons; old current-time/poll comparisons retained as diagnostics; no heard PCM or original GUI comparison\",\"fullAcceptance\":false}\n";write_file_atomic((dir/L"segment-boundary-runtime.json").wstring(),Bytes(report.begin(),report.end()));
}
void sequence_audition_runtime_tests(const std::filesystem::path& dir){
    segment_extended_boundary_runtime_tests(dir);segment_invalidation_runtime_tests(dir);segment_boundary_runtime_tests(dir);
    segment_start_point_runtime_tests(dir);
    finite_segment_loop_runtime_tests(dir);
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
