#include "segment_structure.h"
#include "document.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>

namespace producer::app {
namespace {
Chunk leaf(const char* id,Bytes bytes){Chunk c;c.id=id;c.data=std::move(bytes);return c;}
Chunk list(const char* id,const char* type,std::vector<Chunk> children){Chunk c;c.id=id;c.type=type;c.children=std::move(children);return c;}
Chunk control_track(std::uint32_t classId,const char* chunk,const char* type,Chunk payload){
    Bytes h(32);const GUID guid={classId,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
    std::memcpy(h.data(),&guid,16);put32(h,20,1);if(chunk)std::memcpy(h.data()+24,chunk,4);if(type)std::memcpy(h.data()+28,type,4);
    return list("RIFF","DMTK",{leaf("trkh",h),std::move(payload)});
}
}
const std::array<SegmentTrackDescriptor,17>& segment_track_descriptors(){
    static const std::array<SegmentTrackDescriptor,17> types{{
        {SegmentTrackKind::band,L"Band Track",true},{SegmentTrackKind::chord,L"Chord Track",true},
        {SegmentTrackKind::chordmap,L"Chordmap Track",true},{SegmentTrackKind::groove,L"Groove Track",true},
        {SegmentTrackKind::lyric,L"Lyric Track",true},{SegmentTrackKind::marker,L"Marker Track",true},
        {SegmentTrackKind::mute,L"Mute Track",true},{SegmentTrackKind::parameter,L"Parameter Control Track",true},
        {SegmentTrackKind::pattern,L"Pattern Track",false},{SegmentTrackKind::script,L"Script Track",true},
        {SegmentTrackKind::trigger,L"Segment Trigger Track",true},{SegmentTrackKind::sequence,L"Sequence Track",true},
        {SegmentTrackKind::signpost,L"Signpost Track",true},{SegmentTrackKind::style,L"Style Track",true},
        {SegmentTrackKind::tempo,L"Tempo Track",true},{SegmentTrackKind::meter,L"TimeSig Track",true},
        {SegmentTrackKind::wave,L"Wave Track",false}}};return types;
}
Chunk make_segment_track(SegmentTrackKind kind){
    switch(kind){
    case SegmentTrackKind::band:return make_band_track();
    case SegmentTrackKind::chord:return chord_track();
    case SegmentTrackKind::chordmap:return chordmap_reference_track();
    case SegmentTrackKind::groove:return command_track();
    case SegmentTrackKind::lyric:return lyric_track();
    case SegmentTrackKind::marker:return marker_track();
    case SegmentTrackKind::mute:return mute_track();
    case SegmentTrackKind::parameter:return param_control_track();
    case SegmentTrackKind::script:return script_track();
    case SegmentTrackKind::trigger:return segment_trigger_track();
    case SegmentTrackKind::sequence:return sequence_track();
    case SegmentTrackKind::signpost:return signpost_track();
    case SegmentTrackKind::style:return style_reference_track(1);
    case SegmentTrackKind::tempo:{tempo::Track track;track.replace_event({0,120.0,false});const auto bytes=track.save();return control_track(0xd2ac2885,"tetr",nullptr,leaf("tetr",Bytes(bytes.begin()+8,bytes.end())));}
    case SegmentTrackKind::meter:{Bytes data(12);put32(data,0,8);data[8]=4;data[9]=4;data[10]=4;return control_track(0xd2ac2888,nullptr,"TIMS",list("LIST","TIMS",{leaf("tims",data)}));}
    default:throw std::runtime_error("Track type is not available");
    }
}
SegmentTrackKind segment_track_kind(const Chunk& track){
    if(track.id!="RIFF"||track.type!="DMTK")return SegmentTrackKind::unknown;
    const auto h=track.find("trkh");if(!h||h->data.size()<32)throw std::runtime_error("Track header missing or truncated");
    for(const auto& type:segment_track_descriptors())if(type.canCreate){const auto expected=make_segment_track(type.kind);if(std::equal(h->data.begin(),h->data.begin()+16,expected.find("trkh")->data.begin()))return type.kind;}
    const auto wave=wave_track_identity();if(std::equal(wave.begin(),wave.end(),h->data.begin()))return SegmentTrackKind::wave;
    return SegmentTrackKind::unknown;
}
std::int32_t initial_segment_clocks(const SegmentCreationLength& value){
    std::uint64_t clocks;
    if(value.clockTime){
        if(!value.milliseconds||value.milliseconds>std::uint64_t(INT32_MAX)*500/768)throw std::runtime_error("Segment clock time is out of range");
        clocks=(value.milliseconds*768+499)/500;
    }else{
        if(!value.measures||value.measures>std::uint32_t(INT32_MAX)/3072)throw std::runtime_error("Segment measure count is out of range");
        clocks=std::uint64_t(value.measures)*3072;
    }return static_cast<std::int32_t>(clocks);
}
SegmentDocument SegmentDocument::create_empty(const SegmentCreationLength& length,const std::wstring& name){
    if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Invalid Segment name");
    const auto clocks=initial_segment_clocks(length);SegmentDocument result;auto root=Chunk::parse(result.save_bytes());auto& header=root.find("segh")->data;
    header.resize(std::max<size_t>(40,header.size()));put32(header,4,clocks);
    const auto reference=length.clockTime?length.milliseconds*10000:0;put32(header,24,static_cast<std::uint32_t>(reference));put32(header,28,static_cast<std::uint32_t>(reference>>32));put32(header,32,length.clockTime?1:0);
    root.find("LIST","trkl")->children={make_segment_track(SegmentTrackKind::meter)};
    root.children.push_back(list("LIST","UNFO",{leaf("UNAM",utf16(name))}));
    result.load(root.encode());return result;
}
std::vector<SegmentTrackInfo> SegmentDocument::tracks()const{
    std::vector<SegmentTrackInfo> result;std::array<size_t,18> counts{};
    const auto list=root_.find("LIST","trkl");if(!list)return result;
    for(const auto& track:list->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto kind=segment_track_kind(track);const auto slot=static_cast<size_t>(kind);const auto h=track.find("trkh");const auto groups=read32(h->data,20);
        if(!groups)throw std::runtime_error("Track has no groups");if(!(groups&selectedGroups_))continue;
        result.push_back({kind,counts[slot]++,groups,read32(h->data,16)});
    }
    std::stable_sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.position<b.position;});return result;
}
bool SegmentDocument::add_tracks(const std::vector<SegmentTrackKind>& kinds){
    if(kinds.empty())return false;auto next=*this;auto list=next.root_.find("LIST","trkl");
    if(!list){next.root_.children.push_back(producer::app::list("LIST","trkl",{}));list=&next.root_.children.back();}
    std::uint32_t position=0;for(const auto& track:list->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto h=track.find("trkh");if(!h||h->data.size()<32||read32(h->data,16)==UINT32_MAX)throw std::runtime_error("Track position is out of range");position=std::max(position,read32(h->data,16)+1);
    }
    if(kinds.size()>std::uint64_t(UINT32_MAX)-position)throw std::runtime_error("Too many tracks");
    std::vector<SegmentTrackKind> seen;
    for(const auto kind:kinds){if(std::find(seen.begin(),seen.end(),kind)!=seen.end())throw std::runtime_error("Duplicate track type selection");seen.push_back(kind);auto track=make_segment_track(kind);auto& h=track.find("trkh")->data;put32(h,16,position++);put32(h,20,selectedGroups_);list->children.push_back(std::move(track));}
    const auto before=save_bytes();next.import(next.root_.encode());next.record_edit(before);*this=std::move(next);return true;
}
namespace {
// Segment Properties defaults to TimeSig group1. This is independent of the
// selected editor track/group; configurable design-time groups remain separate.
SegmentDocument length_context(const SegmentDocument& source){auto d=source;d.select_track_group(1);return d;}
std::vector<tempo::Event> length_tempos(const SegmentDocument& source){
    const auto context=length_context(source);auto events=context.tempos();std::int32_t previous=-1;
    for(const auto& e:events){if(e.time<0||e.time<=previous||!std::isfinite(e.bpm)||e.bpm<1||e.bpm>1000)throw std::runtime_error("Segment tempo map cannot be converted to clock time");previous=e.time;}
    return events;
}
long double reference_at(const std::vector<tempo::Event>& events,std::int32_t end){
    long double result=0,bpm=120;std::int32_t at=0;
    for(const auto& e:events){if(e.time>=end)break;result+=(static_cast<long double>(e.time)-at)*600000000.0L/(768*bpm);at=e.time;bpm=e.bpm;}
    return result+(static_cast<long double>(end)-at)*600000000.0L/(768*bpm);
}
std::int32_t music_at(const std::vector<tempo::Event>& events,long double reference){
    long double bpm=120;std::int32_t at=0;
    for(const auto& e:events){const auto span=(static_cast<long double>(e.time)-at)*600000000.0L/(768*bpm);if(reference<=span)break;reference-=span;at=e.time;bpm=e.bpm;}
    const auto value=static_cast<long double>(at)+reference*768*bpm/600000000.0L;
    if(!std::isfinite(value)||value<=0||value>INT32_MAX)throw std::runtime_error("Segment clock time is out of range");
    return static_cast<std::int32_t>(std::ceil(value));
}
SegmentLengthPreview describe_length(const SegmentDocument& source,std::int32_t clocks){
    if(clocks<=0)throw std::runtime_error("Segment length must be positive");const auto axis=length_context(source).timeline();const auto p=axis.position(clocks);
    const auto measures=std::uint64_t(p.measure)+(p.beat||p.tick?1:0);if(!measures||measures>UINT32_MAX)throw std::runtime_error("Segment measure count is out of range");
    const auto reference=reference_at(length_tempos(source),clocks);if(reference>static_cast<long double>(INT64_MAX))throw std::runtime_error("Segment clock time is out of range");
    return {clocks,static_cast<std::uint32_t>(measures),static_cast<std::uint64_t>(std::ceil(reference/10000))};
}
std::uint64_t header64(const Bytes& header,size_t at){return std::uint64_t(read32(header,at))|(std::uint64_t(read32(header,at+4))<<32);}
void require_safe_shrink(const Chunk& root,std::int32_t clocks,long double reference){
    const auto& h=root.find("segh")->data;
    if(read32(h,8)>=static_cast<std::uint32_t>(clocks)||read32(h,12)>=static_cast<std::uint32_t>(clocks)||read32(h,16)>static_cast<std::uint32_t>(clocks))throw std::runtime_error("The new length would exclude the playback start or loop. Choose a longer Segment length.");
    for(const auto offset:{size_t(40),size_t(48),size_t(56)})if(h.size()>=offset+8&&header64(h,offset)>reference)throw std::runtime_error("The new length would exclude a clock-time start or loop. Choose a longer Segment length.");
    const auto tracks=root.find("LIST","trkl");if(!tracks)return;
    for(const auto& track:tracks->children){
        if(track.id!="RIFF"||track.type!="DMTK")throw std::runtime_error("This Segment contains track data whose length cannot be checked. Choose a longer Segment length.");
        const auto kind=segment_track_kind(track);bool safe=false;
        if(kind==SegmentTrackKind::tempo){const auto c=track.find("tetr");tempo::Track map;if(c&&map.load(c->encode())==tempo::LoadResult::ok){safe=true;for(const auto& e:map.events())if(e.time>=clocks)safe=false;}}
        else if(kind==SegmentTrackKind::meter){const auto wrapped=track.find("LIST","TIMS"),direct=track.find("tims");const auto c=wrapped?wrapped:direct;if(c&&!(wrapped&&direct)){Timeline axis;axis.load_meter(c->encode());safe=true;for(const auto& e:axis.meters())if(axis.clocks({e.measure,0,0})>=clocks)safe=false;}}
        else if(kind==SegmentTrackKind::band){safe=true;for(const auto& e:band_track_events(track))if(e.logicalTime>=clocks||e.physicalTime>=clocks)safe=false;}
        else if(kind!=SegmentTrackKind::unknown&&kind!=SegmentTrackKind::wave&&kind!=SegmentTrackKind::pattern){
            // Empty factory payloads can shrink without deleting any events.
            // Opaque or populated tracks remain intact on refusal.
            auto actual=track,empty=make_segment_track(kind);const auto removeHeader=[](Chunk& t){t.children.erase(std::remove_if(t.children.begin(),t.children.end(),[](const auto& c){return c.id=="trkh";}),t.children.end());};removeHeader(actual);removeHeader(empty);safe=actual.encode()==empty.encode();
        }
        if(!safe)throw std::runtime_error("The new length would remove or clip existing track data, or its end cannot be checked. Choose a length that keeps all events.");
    }
}
}
SegmentLengthPreview SegmentDocument::preview_length(const SegmentCreationLength& value)const{
    const auto context=length_context(*this);std::int32_t clocks;
    if(value.clockTime){if(!value.milliseconds||value.milliseconds>std::uint64_t(INT64_MAX)/10000)throw std::runtime_error("Segment clock time is out of range");clocks=music_at(length_tempos(*this),static_cast<long double>(value.milliseconds)*10000);}
    else{if(!value.measures||value.measures>INT32_MAX)throw std::runtime_error("Segment measure count is out of range");clocks=context.timeline().clocks({static_cast<std::int32_t>(value.measures),0,0});}
    auto result=describe_length(*this,clocks);if(value.clockTime)result.milliseconds=value.milliseconds;return result;
}
SegmentCreationLength SegmentDocument::length_settings()const{
    const auto display=describe_length(*this,length());const auto& h=root_.find("segh")->data;SegmentCreationLength value;value.measures=display.measures;value.milliseconds=display.milliseconds;
    value.clockTime=h.size()>=36&&(read32(h,32)&1);if(value.clockTime){const auto reference=header64(h,24);if(!reference||reference>INT64_MAX)throw std::runtime_error("Invalid Segment reference length");value.milliseconds=(reference+9999)/10000;}return value;
}
bool SegmentDocument::set_length(const SegmentCreationLength& value){
    const auto preview=preview_length(value);const auto before=save_bytes();auto next=*this;auto& h=next.root_.find("segh")->data;
    if(preview.clocks<length())require_safe_shrink(root_,preview.clocks,reference_at(length_tempos(*this),preview.clocks));
    put32(h,4,preview.clocks);if(value.clockTime||h.size()>=36){h.resize(std::max<size_t>(40,h.size()));const auto reference=value.clockTime?value.milliseconds*10000:0;put32(h,24,static_cast<std::uint32_t>(reference));put32(h,28,static_cast<std::uint32_t>(reference>>32));put32(h,32,(read32(h,32)&~1u)|(value.clockTime?1u:0u));}
    if(next.save_bytes()==before)return false;next.import(next.save_bytes());next.record_edit(before);*this=std::move(next);return true;
}

}
