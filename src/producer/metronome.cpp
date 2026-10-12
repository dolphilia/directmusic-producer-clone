#include "metronome.h"
#include "document.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace producer::app {
bool operator==(const MetronomeSettings& a,const MetronomeSettings& b){
    return a.enabled==b.enabled&&a.recordOnly==b.recordOnly&&a.pchannel==b.pchannel&&a.firstPitch==b.firstPitch&&a.otherPitch==b.otherPitch&&a.firstVelocity==b.firstVelocity&&a.otherVelocity==b.otherVelocity&&a.countInBars==b.countInBars;
}
void validate_metronome_settings(const MetronomeSettings& s){
    if(s.pchannel>=0xfffffffcu)throw std::runtime_error("Metronome PChannel is reserved");
    if(s.firstPitch>127||s.otherPitch>127||s.firstVelocity>127||s.otherVelocity>127)throw std::runtime_error("Metronome pitch and velocity must be 0 to 127");
}
Bytes encode_metronome_settings(const MetronomeSettings& s){
    validate_metronome_settings(s);Bytes b(40);std::memcpy(b.data(),"MTRO",4);put32(b,4,1);
    put32(b,8,s.enabled?1:0);put32(b,12,s.recordOnly?1:0);put32(b,16,s.pchannel);
    put32(b,20,s.firstPitch);put32(b,24,s.otherPitch);put32(b,28,s.firstVelocity);put32(b,32,s.otherVelocity);put32(b,36,s.countInBars);return b;
}
MetronomeSettings decode_metronome_settings(const Bytes& b){
    if(b.size()!=40||std::memcmp(b.data(),"MTRO",4)||read32(b,4)!=1||read32(b,8)>1||read32(b,12)>1)throw std::runtime_error("Invalid Metronome preferences");
    MetronomeSettings s;s.enabled=read32(b,8)!=0;s.recordOnly=read32(b,12)!=0;s.pchannel=read32(b,16);s.firstPitch=read32(b,20);s.otherPitch=read32(b,24);s.firstVelocity=read32(b,28);s.otherVelocity=read32(b,32);s.countInBars=read32(b,36);validate_metronome_settings(s);return s;
}
MetronomeSettings load_metronome_settings(const std::filesystem::path& p){
    if(p.empty()||!std::filesystem::exists(p))return {};return decode_metronome_settings(read_file(p.wstring()));
}
void save_metronome_settings(const std::filesystem::path& p,const MetronomeSettings& s){
    const auto bytes=encode_metronome_settings(s);if(p.empty())throw std::runtime_error("Metronome preferences path is empty");
    if(!p.parent_path().empty())std::filesystem::create_directories(p.parent_path());write_file_atomic(p.wstring(),bytes);
}
std::vector<MetronomeClick> metronome_clicks(const Timeline& timeline,std::int32_t begin,std::int32_t end,const MetronomeSettings& s,size_t maximum){
    validate_metronome_settings(s);if(begin<0||end<begin)throw std::runtime_error("Invalid Metronome scheduling window");
    std::vector<MetronomeClick> result;if(!s.enabled)return result;
    for(const auto& beat:timeline.beat_ticks(begin,end,maximum)){
        const bool first=beat.beat==0;const auto velocity=first?s.firstVelocity:s.otherVelocity;
        // MIDI velocity zero is a note-off, not an audible click.
        if(velocity)result.push_back({beat.clocks,s.pchannel,static_cast<BYTE>(first?s.firstPitch:s.otherPitch),static_cast<BYTE>(velocity),first});
    }return result;
}
std::int32_t metronome_count_in_clocks(const Timeline& timeline,std::int32_t start,const MetronomeSettings& s,bool recording){
    validate_metronome_settings(s);if(start<0)throw std::runtime_error("Invalid Metronome start point");
    if(!s.enabled||!s.countInBars||(s.recordOnly&&!recording))return 0;
    const auto position=timeline.position(start);if(position.measure==INT32_MAX)throw std::runtime_error("Metronome measure overflow");
    const auto begin=timeline.clocks({position.measure,0,0}),end=timeline.clocks({position.measure+1,0,0});
    const auto clocks=static_cast<std::uint64_t>(end-begin)*s.countInBars;
    if(clocks>INT32_MAX)throw std::runtime_error("Metronome count-in exceeds runtime clock range");return static_cast<std::int32_t>(clocks);
}
MetronomeCountIn prepare_metronome_count_in(const Bytes& bytes,const Timeline& timeline,std::int32_t start,const MetronomeSettings& settings,const std::vector<Bytes>& styleBands){
    MetronomeCountIn result;result.clocks=metronome_count_in_clocks(timeline,start,settings);if(!result.clocks)return result;
    SegmentDocument source;source.load(bytes);if(start>=source.length()||start>INT32_MAX-result.clocks)throw std::runtime_error("Metronome count-in seek/length exceeds runtime range");
    result.startPoint=start;const auto measure=timeline.position(start).measure;auto meter=timeline.meters().front();for(const auto& event:timeline.meters())if(event.measure<=measure)meter=event;
    auto carrier=Chunk::parse(SegmentDocument::create_empty({false,1,2000},L"Metronome Count-In").save_bytes());auto& header=carrier.find("segh")->data;
    put32(header,4,start+result.clocks);put32(header,8,start);
    auto tracks=carrier.find("LIST","trkl");auto& meterData=tracks->children.front().find("LIST","TIMS")->find("tims")->data;
    meterData[8]=meter.beats;meterData[9]=meter.denominator;meterData[10]=static_cast<BYTE>(meter.grids);meterData[11]=static_cast<BYTE>(meter.grids>>8);
    result.timeline.load_meter(tracks->children.front().find("LIST","TIMS")->encode());
    auto tempoTrack=make_segment_track(SegmentTrackKind::tempo);tempo::Track tempos;double bpm=120;for(const auto& event:source.tempos())if(event.time<=start&&event.bpm>0)bpm=event.bpm;
    tempos.replace_events({{0,bpm}});const auto tempoBytes=tempos.save();tempoTrack.find("tetr")->data=Bytes(tempoBytes.begin()+8,tempoBytes.end());tracks->children.push_back(std::move(tempoTrack));
    // Style defaults precede explicit Band history. Original native Band init
    // at the retained seek position applies partial setting-validity flags.
    for(const auto& band:styleBands){auto track=make_band_track();if(!set_band_track_event(track,0,band))throw std::runtime_error("Cannot bind count-in Style Band");tracks->children.push_back(std::move(track));}
    const auto root=Chunk::parse(bytes);if(const auto original=root.find("LIST","trkl"))for(const auto& track:original->children)if(is_band_track(track)){
        auto copy=track;const auto events=band_track_events(copy);for(size_t i=events.size();i>0;--i)if(events[i-1].physicalTime>start)delete_band_track_event(copy,i-1);
        tracks->children.push_back(std::move(copy));
    }
    result.segment=carrier.encode();return result;
}
}
