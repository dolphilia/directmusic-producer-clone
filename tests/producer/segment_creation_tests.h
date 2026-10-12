#include "segment_band_owner_tests.h"
#include "segment_length_tests.h"
void segment_creation_tests(const std::filesystem::path& dir){
    segment_band_owner_tests(dir);
    segment_length_tests(dir);
    using K=SegmentTrackKind;
    auto d=SegmentDocument::create_empty({},L"新規 Segment");const auto empty=d.save_bytes();
    require(d.length()==61440&&d.name()==L"新規 Segment","New Segment defaults to20 measures at4/4");
    require(d.tracks().size()==1&&d.tracks()[0].kind==K::meter&&d.tracks()[0].groups==1,"New Segment contains only explicit TimeSig");
    const auto meter=d.timeline().meters().front();
    require(meter.beats==4&&meter.denominator==4&&meter.grids==4,"New TimeSig is4/4 with four grids per beat");
    require(d.tempos().empty()&&d.notes().empty()&&d.band_events().empty(),"Creation invents no Tempo Band instrument or note");
    require(!d.dirty()&&!d.can_undo()&&!d.can_redo(),"New native payload starts at clean checkpoint with empty history");
    const auto second=SegmentDocument::create_empty({},L"Second");
    require(Chunk::parse(empty).find("guid")->data!=Chunk::parse(second.save_bytes()).find("guid")->data,"New Segments have separate identities");
    require(!d.add_tracks({})&&d.save_bytes()==empty&&!d.can_undo(),"Empty Add New Tracks selection is a no-op");
    require(d.add_tracks({K::band})&&d.tracks().size()==2&&d.band_events().empty(),"Band Track starts empty without implicit Band");
    const auto band=d.save_bytes();
    require(d.add_tracks({K::tempo,K::sequence})&&d.tempos().size()==1&&d.tempos()[0].time==0&&d.tempos()[0].bpm==120,"Adding Tempo initializes120 at zero");
    require(d.notes().empty()&&d.band_events().empty(),"Adding Sequence creates no notes or Band instruments");
    const auto added=d.save_bytes();const auto rows=d.tracks();
    require(rows.size()==4&&rows[0].kind==K::meter&&rows[1].kind==K::band&&rows[2].kind==K::tempo&&rows[3].kind==K::sequence,"Track display order follows actual native positions");
    require(rows[0].position==0&&rows[1].position==1&&rows[2].position==2&&rows[3].position==3,"New track positions append without rewriting existing headers");
    require(d.undo()&&d.save_bytes()==band&&d.redo()&&d.save_bytes()==added,"Multi-track addition is one whole-byte history transaction");
    require(d.undo(),"Prepare retained Redo before invalid selection");
    for(const auto& bad:std::vector<std::vector<K>>{{K::tempo,K::tempo},{K::tempo,K::pattern},{K::wave},{K::unknown}}){
        rejected([&]{d.add_tracks(bad);},"Unavailable or duplicate track choice rejected");
        require(d.save_bytes()==band&&d.can_redo(),"Rejected track batch preserves all bytes and Redo");
    }
    require(d.redo()&&d.add_tracks({K::tempo}),"Second Tempo can be created");d.select_track_group(1,1);
    require(d.selected_tempo_index()==1&&d.undo()&&d.selected_tempo_index()==0&&d.save_bytes()==added,"Undo of selected added track safely clamps editor context");
    rejected([&]{d.select_track_group(1,1);},"Public out-of-range track selection still rejects");
    require(d.save_bytes()==added&&d.can_redo()&&d.redo(),"Invalid selection preserves native bytes and history");
    auto grouped=SegmentDocument::create_empty({},L"Grouped");grouped.select_track_group(0x80000002u);grouped.add_tracks({K::sequence,K::band});
    require(grouped.tracks().size()==2&&grouped.tracks()[0].groups==0x80000002u&&grouped.tracks()[1].groups==0x80000002u,"Added tracks retain full DWORD group masks");
    auto opaque=Chunk::parse(empty);Chunk unknown;unknown.id="priv";unknown.data={17,23,91};unknown.padding=0x6a;opaque.children.push_back(unknown);
    SegmentDocument extended;extended.load(opaque.encode());extended.add_tracks({K::tempo});
    require(extended.undo()&&extended.save_bytes()==opaque.encode()&&extended.redo()&&Chunk::parse(extended.save_bytes()).find("priv")->encode()==unknown.encode(),"Track creation preserves opaque data and odd padding through history");
    SegmentCreationLength clock;clock.clockTime=true;clock.milliseconds=1501;auto timed=SegmentDocument::create_empty(clock,L"Clock");const auto header=Chunk::parse(timed.save_bytes()).find("segh")->data;
    require(timed.length()==2306&&read32(header,24)==15010000&&read32(header,28)==0&&read32(header,32)==1,"Clocktime retains exact reference length and rounds music clocks upward");
    SegmentCreationLength invalid;invalid.measures=0;rejected([&]{SegmentDocument::create_empty(invalid,L"Invalid");},"Zero measures rejected");invalid.measures=UINT32_MAX;rejected([&]{initial_segment_clocks(invalid);},"Measure length overflow rejected");invalid.clockTime=true;invalid.milliseconds=0;rejected([&]{initial_segment_clocks(invalid);},"Zero clocktime rejected");invalid.milliseconds=UINT64_MAX;rejected([&]{initial_segment_clocks(invalid);},"Clocktime overflow rejected");
    for(const auto& name:std::vector<std::wstring>{L"",std::wstring(256,L'x'),std::wstring(L"a\0b",3)})rejected([&]{SegmentDocument::create_empty({},name);},"Invalid creation name rejected");
    Framework host;const auto made=host.create_files({{DocumentKind::Segment,L"Seed",{}},{DocumentKind::Band,L"Owned Band",{}},{DocumentKind::Style,L"Owned Style",{}}});
    require(made.size()==3&&host.documents().size()==1&&host.band_documents().size()==1&&host.style_documents().size()==1,"One request creates multiple native document types");
    require(host.document(0).name()==L"Seed"&&host.band_document(0).name()==L"Owned Band"&&host.style_document(0).name()==L"Owned Style","Custom names reach native document payloads");
    const auto prior=host.document(0).save_bytes();
    rejected([&]{host.create_files({{DocumentKind::Segment,L"Should not append",{}},{DocumentKind::Wave,L"Missing Wave",{}}});},"Final invalid file request rejects whole batch");
    require(host.documents().size()==1&&host.document(0).save_bytes()==prior&&host.waves().empty(),"Failed creation batch publishes no partial files");
    rejected([&]{host.create_files({{DocumentKind::Project,L"Should not replace",{}},{DocumentKind::Segment,L"",{}}});},"Failed Project batch cannot discard current project");
    require(host.documents().size()==1&&host.document(0).save_bytes()==prior,"Project replacement remains atomic on invalid child");
    Framework types;types.create_files({{DocumentKind::AudioPath,L"Audio",{}},{DocumentKind::ChordMap,L"Map",{}},{DocumentKind::Container,L"Container",{}},{DocumentKind::Collection,L"Collection",{}},{DocumentKind::Script,L"Script",{}},{DocumentKind::ToolGraph,L"Tools",{}}});
    require(types.audio_paths().size()==1&&types.chordmaps().size()==1&&types.containers().size()==1&&types.collections().size()==1&&types.scripts().size()==1&&types.tool_graphs().size()==1,"Other six supported new component types use native factories");
    const auto owned=dir/L"Creation";std::filesystem::create_directories(owned);
    Framework save;save.create_files({{DocumentKind::Project,L"Creation Proof",{}},{DocumentKind::Segment,L"Seed",{}}});save.document(0).add_tracks({K::band,K::tempo,K::sequence});
    const auto payload=save.document(0).save_bytes();save.save_segment(0,(owned/L"Seed.sgp").wstring());save.save_project((owned/L"Creation.pro").wstring());Framework reopen;reopen.open_project((owned/L"Creation.pro").wstring());
    require(reopen.documents().size()==1&&reopen.document(0).save_bytes()==payload&&reopen.document(0).tracks().size()==4&&!reopen.dirty(),"Owned native Segment and Project restore exact track structure");
    require(!reopen.document(0).can_undo()&&!reopen.document(0).can_redo(),"Native reload does not carry process history");
    const auto gui=dir/L"GuiCreation";std::filesystem::create_directories(gui);Framework seed;seed.create_files({{DocumentKind::Project,L"GUI Creation",{}},{DocumentKind::Segment,L"Seed",{}}});seed.save_segment(0,(gui/L"GuiSeed.sgp").wstring());seed.save_project((gui/L"GuiCreation.pro").wstring());
}
