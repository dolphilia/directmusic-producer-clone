void metronome_model_tests(const std::filesystem::path& dir){
    const auto file=dir/L"metronome.bin";MetronomeSettings s;Timeline timeline;
    require(metronome_clicks(timeline,0,6144,s).empty(),"Disabled Metronome produces no messages");
    s.enabled=true;auto clicks=metronome_clicks(timeline,0,6144,s);
    require(clicks.size()==8&&clicks[0].clocks==0&&clicks[7].clocks==5376,"4/4 Metronome follows quarter beat boundaries in a half-open window");
    require(clicks[0].firstBeat&&clicks[0].pitch==36&&clicks[0].velocity==117&&clicks[0].pchannel==9&&clicks[1].pitch==37&&clicks[1].velocity==100&&!clicks[1].firstBeat&&clicks[4].firstBeat,"First and other beats preserve independent pitch velocity and PChannel");
    const auto a=metronome_clicks(timeline,0,1000,s),b=metronome_clicks(timeline,1000,3072,s);
    require(a.size()==2&&b.size()==2&&a.back().clocks==768&&b.front().clocks==1536,"Adjacent lookahead windows neither duplicate nor skip a beat");
    require(metronome_clicks(timeline,200,768,s).empty(),"Starting between beats does not invent an immediate off-grid click");
    auto song=SegmentDocument::create_empty({false,8,16000},L"Meter click");require(song.set_meter(1,3,8,2),"Create 3/8 meter after a 4/4 measure");
    const auto changed=song.timeline();clicks=metronome_clicks(changed,3000,4500,s);
    require(clicks.size()==4&&clicks[0].clocks==3072&&clicks[1].clocks==3456&&clicks[2].clocks==3840&&clicks[3].clocks==4224&&clicks[0].firstBeat&&clicks[3].firstBeat,"Meter denominator and measure accent change at the exact TimeSig event");
    s.countInBars=2;require(metronome_count_in_clocks(timeline,1000,s)==0&&metronome_count_in_clocks(timeline,1000,s,true)==6144,"Record Only count-in excludes playback and includes recording");
    s.recordOnly=false;require(metronome_count_in_clocks(changed,3072,s)==2304,"All Playback count-in uses the starting measure meter");
    song.add_tracks({SegmentTrackKind::band,SegmentTrackKind::sequence,SegmentTrackKind::script,SegmentTrackKind::trigger});song.insert_default_band(0);song.insert_default_band(4000);song.add_note({3072,384,0,60,96});song.add_tempo(0,120);song.add_tempo(3072,90);song.add_tempo(4000,240);const auto native=song.save_bytes();
    const auto countIn=prepare_metronome_count_in(native,changed,3072,s);SegmentDocument carrier;carrier.load(countIn.segment);const auto carrierTracks=carrier.tracks();
    require(countIn.clocks==2304&&countIn.startPoint==3072&&carrier.length()==5376,"Count-in carrier retains source seek and exactly two3/8 bars");
    require(std::all_of(carrierTracks.begin(),carrierTracks.end(),[](const auto& t){return t.kind==SegmentTrackKind::band||t.kind==SegmentTrackKind::tempo||t.kind==SegmentTrackKind::meter;}),"Count-in carrier executes no authored Sequence Script Trigger CC or Wave track");
    require(carrier.band_events().size()==1&&carrier.band_events()[0].band==song.band_events()[0].band,"Count-in retains preceding Band history and excludes future Band events");
    require(carrier.tempos().size()==1&&carrier.tempos()[0].bpm==90&&countIn.timeline.beat_ticks(0,countIn.clocks).size()==6,"Count-in freezes the starting tempo and meter rather than executing future changes");
    require(song.save_bytes()==native,"Preparing a count-in leaves authored native bytes untouched");
    s.enabled=false;require(prepare_metronome_count_in(native,changed,3072,s).segment.empty(),"Disabled Metronome bypasses count-in without a carrier");s.enabled=true;
    save_metronome_settings(file,s);const auto before=read_file(file.wstring());require(load_metronome_settings(file)==s,"Metronome preferences roundtrip independently of Performance settings");
    s.pchannel=16;require(decode_metronome_settings(encode_metronome_settings(s)).pchannel==16,"Metronome retains PChannel17 without a16-channel internal cap");
    s.pchannel=0xfffffffbu;require(decode_metronome_settings(encode_metronome_settings(s)).pchannel==s.pchannel,"Metronome retains the largest permitted DWORD PChannel");
    for(std::uint32_t p=0xfffffffcu;;++p){auto bad=s;bad.pchannel=p;rejected([&]{save_metronome_settings(file,bad);},"Reserved Metronome PChannel rejects before preferences replacement");require(read_file(file.wstring())==before,"Rejected Metronome preferences preserve exact existing bytes");if(p==0xffffffffu)break;}
    auto bad=before;put32(bad,8,2);rejected([&]{decode_metronome_settings(bad);},"Nonboolean Metronome preferences reject");bad=before;put32(bad,24,128);rejected([&]{decode_metronome_settings(bad);},"Out of range Metronome MIDI pitch rejects");
    s.pchannel=9;s.firstVelocity=s.otherVelocity=0;require(metronome_clicks(timeline,0,3072,s).empty(),"Velocity zero silences clicks without emitting MIDI note-off as note-on");
    s.countInBars=0xffffffffu;rejected([&]{metronome_count_in_clocks(timeline,0,s);},"Count-in clock overflow rejects before playback scheduling");
    s.firstVelocity=117;rejected([&]{metronome_clicks(timeline,0,6144,s,1);},"Dense click windows respect explicit allocation bounds");
}
