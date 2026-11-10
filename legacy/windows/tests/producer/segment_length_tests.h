void segment_length_tests(const std::filesystem::path& dir){
    auto d=SegmentDocument::create_empty({},L"Length");const auto initial=d.save_bytes();const auto settings=d.length_settings();
    require(!settings.clockTime&&settings.measures==20&&settings.milliseconds==40000,"Existing Length displays20 bars and40 seconds");
    SegmentCreationLength bars;bars.measures=24;const auto preview=d.preview_length(bars);
    require(preview.clocks==73728&&preview.measures==24&&preview.milliseconds==48000&&d.save_bytes()==initial&&!d.dirty(),"Length preview is nonmutating");
    require(!d.set_length(settings)&&d.save_bytes()==initial&&!d.can_undo(),"Unchanged musical Length is a no-op");
    require(d.set_length(bars)&&d.length()==73728&&d.dirty(),"Length extension changes musical header");const auto extended=d.save_bytes();
    require(Chunk::parse(extended).find("LIST","trkl")->encode()==Chunk::parse(initial).find("LIST","trkl")->encode(),"Extension inserts no track events or commands");
    require(d.undo()&&d.save_bytes()==initial&&!d.dirty()&&d.can_redo(),"Length Undo restores clean saved bytes");
    for(const auto bad:{0u,UINT32_MAX}){bars.measures=bad;rejected([&]{d.set_length(bars);},"Invalid musical Length rejected");require(d.save_bytes()==initial&&!d.dirty()&&d.can_redo(),"Invalid Length retains bytes dirty and Redo");}
    require(d.redo()&&d.save_bytes()==extended,"Invalid edits do not destroy Length Redo");
    SegmentCreationLength clock;clock.clockTime=true;clock.milliseconds=1001;
    require(d.set_length(clock)&&d.length()==1538,"Empty Segment clocktime rounds music clocks upward");const auto timed=d.save_bytes();const auto h=Chunk::parse(timed).find("segh")->data;
    require(read32(h,24)==10010000&&read32(h,28)==0&&(read32(h,32)&1)&&d.length_settings().milliseconds==1001,"Clock Length retains exact reference time and mode");
    require(d.undo()&&d.save_bytes()==extended&&d.redo()&&d.save_bytes()==timed,"Clock Length is one exact history transaction");
    for(const auto bad:{std::uint64_t(0),UINT64_MAX}){clock.milliseconds=bad;rejected([&]{d.set_length(clock);},"Invalid clock Length rejected");require(d.save_bytes()==timed&&d.can_undo()&&!d.can_redo(),"Invalid clock edit preserves full history");}
    auto tempo=SegmentDocument::create_empty({},L"Tempo map");tempo.add_tracks({SegmentTrackKind::tempo});tempo.select(0);tempo.change_selected(60);tempo.add_tempo(3072,120);bars.measures=2;clock.milliseconds=4500;
    require(tempo.preview_length(bars).milliseconds==6000&&tempo.preview_length(clock).clocks==3840,"Length conversion integrates both60 and120 BPM spans");
    auto meter=SegmentDocument::create_empty({},L"Meter map");meter.set_meter(0,3,4,4);meter.set_meter(2,5,8,4);bars.measures=4;
    require(meter.preview_length(bars).clocks==8448&&meter.preview_length(bars).milliseconds==5500,"Length uses piecewise3/4 and5/8 measure map");
    meter.select_track_group(2);require(meter.preview_length(bars).clocks==8448&&meter.selected_groups()==2,"Length uses group1 without changing selected editor group");
    auto opaque=Chunk::parse(initial);auto& oh=opaque.find("segh")->data;oh.resize(73);oh[72]=0x91;put32(oh,32,0x20);Chunk privateData;privateData.id="priv";privateData.data={11,23,59};privateData.padding=0xa7;opaque.children.push_back(privateData);SegmentDocument unknown;unknown.load(opaque.encode());bars.measures=24;unknown.set_length(bars);const auto newRoot=Chunk::parse(unknown.save_bytes());
    require(newRoot.find("segh")->data.size()==73&&newRoot.find("segh")->data[72]==0x91&&read32(newRoot.find("segh")->data,32)==0x20&&newRoot.find("priv")->encode()==privateData.encode(),"Length preserves unknown flags header tail root bytes and padding");
    require(unknown.undo()&&unknown.save_bytes()==opaque.encode(),"Opaque header Length Undo is byte exact");
    auto legacy=Chunk::parse(initial);legacy.find("segh")->data.resize(24);SegmentDocument dx7;dx7.load(legacy.encode());dx7.set_length(bars);
    require(Chunk::parse(dx7.save_bytes()).find("segh")->data.size()==24,"Musical Length preserves legacy24-byte header");
    auto band=SegmentDocument::create_empty({},L"Tail protected");band.add_tracks({SegmentTrackKind::band});band.insert_default_band(18*3072);const auto beforeBand=band.save_bytes();bars.measures=10;
    rejected([&]{band.set_length(bars);},"Shortening across Band tail is refused");require(band.save_bytes()==beforeBand&&band.band_events().size()==1,"Refused shrink preserves the embedded Band and document");
    auto shortEmpty=SegmentDocument::create_empty({},L"Short empty");shortEmpty.add_tracks({SegmentTrackKind::sequence,SegmentTrackKind::tempo});bars.measures=4;
    require(shortEmpty.set_length(bars)&&shortEmpty.length()==12288,"Empty known tracks can shorten safely");
    auto unrecognized=Chunk::parse(initial);auto track=make_segment_track(SegmentTrackKind::tempo);track.find("trkh")->data[0]^=0x31;unrecognized.find("LIST","trkl")->children.push_back(track);SegmentDocument protectedUnknown;protectedUnknown.load(unrecognized.encode());const auto unknownBytes=protectedUnknown.save_bytes();
    rejected([&]{protectedUnknown.set_length(bars);},"Unknown timed track refuses shortening");require(protectedUnknown.save_bytes()==unknownBytes&&!protectedUnknown.dirty()&&!protectedUnknown.can_undo(),"Unknown track refusal is fully atomic");
    auto loop=Chunk::parse(initial);put32(loop.find("segh")->data,16,30720);SegmentDocument protectedLoop;protectedLoop.load(loop.encode());
    rejected([&]{protectedLoop.set_length(bars);},"Length cannot silently invalidate a loop endpoint");require(protectedLoop.save_bytes()==loop.encode(),"Refused loop shortening retains header");
    auto direct=Chunk::parse(initial);auto& mt=direct.find("LIST","trkl")->children[0];const auto directMeter=*mt.find("LIST","TIMS")->find("tims");mt.children.pop_back();mt.children.push_back(directMeter);SegmentDocument directDoc;directDoc.load(direct.encode());
    require(directDoc.set_length(bars)&&directDoc.length()==12288,"Direct tims payload also supports safe shortening");
    const auto path=dir/L"Length.sgp";d.save(path.wstring());SegmentDocument reloaded;reloaded.load(read_file(path.wstring()));
    require(reloaded.save_bytes()==timed&&reloaded.length_settings().clockTime&&reloaded.length_settings().milliseconds==1001&&!reloaded.dirty()&&!reloaded.can_undo(),"Native reload restores clock Length without process history");
}
