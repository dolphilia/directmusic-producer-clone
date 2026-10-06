#include "producer/midi_import.h"
namespace {
void midi_be(Bytes& b,unsigned v,unsigned n){for(unsigned i=n;i;--i)b.push_back(static_cast<std::uint8_t>(v>>((i-1)*8)));}
void midi_vlq(Bytes& b,unsigned v){Bytes q{static_cast<std::uint8_t>(v&127)};while(v>>=7)q.insert(q.begin(),static_cast<std::uint8_t>((v&127)|128));b.insert(b.end(),q.begin(),q.end());}
void midi_event(Bytes& b,unsigned delta,std::initializer_list<unsigned> data){midi_vlq(b,delta);for(const auto v:data)b.push_back(static_cast<std::uint8_t>(v));}
Bytes midi_file(unsigned format,unsigned division,const std::vector<Bytes>& tracks){Bytes b{'M','T','h','d'};midi_be(b,6,4);midi_be(b,format,2);midi_be(b,static_cast<unsigned>(tracks.size()),2);midi_be(b,division,2);for(const auto& t:tracks){b.insert(b.end(),{'M','T','r','k'});midi_be(b,static_cast<unsigned>(t.size()),4);b.insert(b.end(),t.begin(),t.end());}return b;}
void midi_import_tests(const std::filesystem::path& dir){
    Bytes conductor; midi_event(conductor,0,{255,81,3,9,39,192}); // 100 BPM
    midi_event(conductor,0,{255,88,4,4,2,24,8});
    midi_event(conductor,1920,{255,81,3,5,22,21}); // ~180 BPM
    midi_event(conductor,0,{255,88,4,3,2,24,8});midi_event(conductor,1440,{255,47,0});
    Bytes piano;midi_event(piano,0,{176,0,0});midi_event(piano,0,{176,32,0});midi_event(piano,0,{192,0});midi_event(piano,0,{176,7,90});midi_event(piano,0,{176,10,50});
    midi_event(piano,0,{144,60,100});midi_event(piano,240,{60,80}); // running status, overlapping same pitch
    midi_event(piano,240,{128,60,0});midi_event(piano,240,{60,0});
    midi_event(piano,0,{224,0,64});midi_event(piano,240,{192,19});midi_event(piano,0,{144,64,100});midi_event(piano,480,{144,64,0});midi_event(piano,1920,{255,47,0});
    Bytes second;midi_event(second,0,{193,40});midi_event(second,0,{177,7,80});midi_event(second,0,{145,67,100});midi_event(second,480,{129,67,0});midi_event(second,2880,{255,47,0});
    const auto source=midi_file(1,480,{conductor,piano,second}),original=source;auto imported=import_midi(source);
    require(source==original,"MIDI source bytes immutable");require(imported.format==1&&imported.division==480&&imported.sourceTracks==3,"SMF1 header contract");
    require(imported.notes==4&&imported.curves==4&&imported.channels==std::vector<unsigned>({0,1}),"channel notes and CC/pitch counts");
    auto& doc=imported.segment;require(doc.length()==5377,"absolute PPQN conversion and final tick length");
    const auto ts=doc.tempos();require(ts.size()==2&&ts[0].time==0&&ts[0].bpm==100&&ts[1].time==3072&&std::abs(ts[1].bpm-180)<0.001,"tempo change exact clock and microsecond conversion");
    auto n=doc.notes();require(n.size()==3&&n[0].time==0&&n[0].duration==768&&n[1].time==384&&n[1].duration==768&&n[2].time==1536,"FIFO note-off and running status pairing");
    doc.select_track_group(1,0,0,1);n=doc.notes();require(n.size()==1&&n[0].channel==1&&n[0].pitch==67,"separate channel strip selection");
    const auto timeline=doc.timeline();require(timeline.clocks({1,0,0})==3072&&timeline.clocks({2,0,0})==5376,"4/4 to 3/4 timeline imported");
    const auto root=Chunk::parse(doc.save_bytes());const auto tracks=root.find("LIST","trkl");std::vector<BandEvent> bandEvents;
    for(const auto& track:tracks->children)if(is_band_track(track))bandEvents=band_track_events(track);
    require(bandEvents.size()==2&&bandEvents[1].logicalTime==1536,"timed program changes create Band events");
    BandDocument b;b.load(bandEvents[0].band);const auto initial=b.instruments();require(initial.size()==2&&initial[0].patch==0&&initial[1].patch==40,"initial Band instrument assignments");
    b.load(bandEvents[1].band);require(b.instruments().size()==1&&b.instruments()[0].pchannel==0&&b.instruments()[0].patch==19,"later Band leaves other channel controllers alone");
    for(const auto& track:tracks->children)if(is_sequence_track(track)){const auto& p=track.find("seqt")->data;const auto at=8+read32(p,4);require(std::string(p.begin()+at,p.begin()+at+4)=="curl"&&read32(p,at+8)==32,"native curve stride and framing");}
    const auto native=doc.save_bytes();SegmentDocument reload;reload.load(native);require(reload.save_bytes()==native,"generated native Segment lossless reload");
    auto projectDir=dir/L"ImportedProject";std::filesystem::create_directories(projectDir);const auto mid=projectDir/L"Source.mid";write_file_atomic(mid.wstring(),source);
    Framework owner;const auto keep=owner.new_segment();owner.document(keep).add_tempo(0,123);const auto kept=owner.document(keep).save_bytes();
    const auto index=owner.import_midi_segment(mid.wstring());require(index==1&&owner.documents()[index].path.empty()&&owner.document(0).save_bytes()==kept,"atomic unsaved new Segment ownership");
    const auto ownedNative=owner.document(index).save_bytes();
    owner.save_segment(0,(projectDir/L"Existing.sgp").wstring());owner.save_segment(index,(projectDir/L"Imported.sgp").wstring());owner.save_project((projectDir/L"ImportedProject.pro").wstring());
    Framework restored;restored.open_project((projectDir/L"ImportedProject.pro").wstring());require(restored.documents().size()==2&&restored.document(1).save_bytes()==ownedNative,"native Project restores imported Segment and its own GUID");
    restored.document(1).select_track_group(1);const auto before=restored.document(1).save_bytes();require(restored.document(1).add_note({2304,384,0,72,88}),"imported Sequence editable");const auto edited=restored.document(1).save_bytes();require(restored.undo_segment(1)&&restored.document(1).save_bytes()==before&&restored.redo_segment(1)&&restored.document(1).save_bytes()==edited,"imported whole document Undo Redo");
    Bytes minimal;midi_event(minimal,0,{144,60,100});midi_event(minimal,480,{128,60,0});midi_event(minimal,0,{255,47,0});require(import_midi(midi_file(0,480,{minimal})).notes==1,"SMF0 defaults");
    Bytes drums;midi_event(drums,0,{153,36,100});midi_event(drums,480,{137,36,0});midi_event(drums,0,{255,47,0});auto drum=import_midi(midi_file(0,480,{drums}));const auto drumRoot=Chunk::parse(drum.segment.save_bytes());for(const auto& t:drumRoot.find("LIST","trkl")->children)if(is_band_track(t)){BandDocument d;d.load(band_track_events(t)[0].band);require(d.instruments()[0].patch==0x80000000u&&d.instruments()[0].pchannel==9,"channel ten GM percussion Band");}
    std::vector<Bytes> invalid;auto bad=source;bad.pop_back();invalid.push_back(bad);bad=source;bad.push_back(0);invalid.push_back(bad);
    invalid.push_back(midi_file(2,480,{minimal}));invalid.push_back(midi_file(0,0,{minimal}));invalid.push_back(midi_file(0,0xe728,{minimal}));invalid.push_back(midi_file(0,480,{minimal,minimal}));
    const std::vector<Bytes> badTracks={{0,60,100,0,255,47,0},{0,144,128,100,0,255,47,0},{0,144,60,100,0,255,47,0},{0,128,60,0,0,255,47,0},{0,144,60,100},{0,240,1,0,0,255,47,0},{0,255,5,1,65,0,255,47,0},{0,255,81,3,0,0,0,0,255,47,0},{0,255,47,1,0},{0,255,47,0,0},{0x81,0x80,0x80,0x80,0,255,47,0},{1,255,88,4,3,2,24,8,0,255,47,0}};
    for(const auto& t:badTracks)invalid.push_back(midi_file(0,480,{t}));
    owner.document(0).add_tempo(768,140);const auto stable=owner.document(0).save_bytes();const auto count=owner.documents().size();
    for(size_t i=0;i<invalid.size();++i){const auto path=dir/(L"Invalid-"+std::to_wstring(i)+L".mid");write_file_atomic(path.wstring(),invalid[i]);rejected([&]{owner.import_midi_segment(path.wstring());},"malformed unsupported MIDI rejected");require(owner.documents().size()==count&&owner.document(0).save_bytes()==stable,"failed import preserves owner and existing history");}
    require(owner.undo_segment(0)&&owner.document(0).save_bytes()==kept&&owner.redo_segment(0)&&owner.document(0).save_bytes()==stable,"failed imports retain pre-existing Undo Redo");
    Bytes probeTempo;midi_event(probeTempo,0,{255,81,3,9,39,192});midi_event(probeTempo,9600,{255,81,3,6,26,128});midi_event(probeTempo,9600,{255,47,0});
    Bytes probe;midi_event(probe,0,{192,0});for(unsigned i=0;i<4;++i){midi_event(probe,i?4560:0,{144,60,96});midi_event(probe,240,{128,60,0});}midi_event(probe,4560,{255,47,0});
    const auto probeFile=midi_file(1,480,{probeTempo,probe});write_file_atomic((dir/L"TempoProbe.mid").wstring(),probeFile);const auto converted=import_midi(probeFile);write_file_atomic((dir/L"TempoProbe.sgp").wstring(),converted.segment.save_bytes());require(converted.notes==4&&converted.segment.tempos()[1].bpm==150,"reproducible GUI tempo import fixture");
}
void midi_import_reference_tests(const std::filesystem::path& dir,const std::wstring& path){
    const auto source=read_file(path);auto r=import_midi(source);require(r.notes==646&&r.curves==8&&r.channels==std::vector<unsigned>({0,1,2,3,4,9}),"original DemoMIDI 646 notes and six channel assignments");
    require(r.segment.tempos().size()==1&&std::abs(r.segment.tempos()[0].bpm-87)<0.001,"original DemoMIDI tempo microseconds");
    const unsigned counts[]={23,24,173,91,62,273};for(size_t i=0;i<6;++i){r.segment.select_track_group(1,0,0,i);const auto channelNotes=r.segment.notes();require(channelNotes.size()==counts[i]&&std::all_of(channelNotes.begin(),channelNotes.end(),[](const Note& n){return n.duration>0;}),"original DemoMIDI per-channel notes and durations");}
    const auto native=r.segment.save_bytes();write_file_atomic((dir/L"DemoMIDI-imported.sgp").wstring(),native);SegmentDocument reload;reload.load(read_file((dir/L"DemoMIDI-imported.sgp").wstring()));require(reload.save_bytes()==native&&read_file(path)==source,"original input unchanged and imported native reload");
}
}
