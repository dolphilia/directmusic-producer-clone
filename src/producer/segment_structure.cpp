#include "segment_structure.h"
#include "document.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>

namespace producer::app {
namespace {
constexpr std::uint32_t boundaryMask=0x813900u,alignmentMask=0x80e0000u,markerFlag=0x1000000u;
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
    return add_tracks_impl(kinds,false);
}
bool SegmentDocument::add_editor_tracks(const std::vector<SegmentTrackKind>& kinds){
    return add_tracks_impl(kinds,true);
}
bool SegmentDocument::add_tracks_impl(const std::vector<SegmentTrackKind>& kinds,bool globalTempo){
    if(kinds.empty())return false;auto next=*this;auto list=next.root_.find("LIST","trkl");
    if(!list){next.root_.children.push_back(producer::app::list("LIST","trkl",{}));list=&next.root_.children.back();}
    std::uint32_t position=0;for(const auto& track:list->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto h=track.find("trkh");if(!h||h->data.size()<32||read32(h->data,16)==UINT32_MAX)throw std::runtime_error("Track position is out of range");position=std::max(position,read32(h->data,16)+1);
    }
    if(kinds.size()>std::uint64_t(UINT32_MAX)-position)throw std::runtime_error("Too many tracks");
    std::vector<SegmentTrackKind> seen;
    for(const auto kind:kinds){if(std::find(seen.begin(),seen.end(),kind)!=seen.end())throw std::runtime_error("Duplicate track type selection");seen.push_back(kind);auto track=make_segment_track(kind);auto& h=track.find("trkh")->data;put32(h,16,position++);put32(h,20,globalTempo&&kind==SegmentTrackKind::tempo?UINT32_MAX:selectedGroups_);list->children.push_back(std::move(track));}
    const auto before=save_bytes();next.import(next.root_.encode());next.record_edit(before);*this=std::move(next);return true;
}
namespace {
// Segment Properties defaults to TimeSig group1. This is independent of the
// selected editor track/group; configurable design-time groups remain separate.
SegmentDocument length_context(const SegmentDocument& source){auto d=source;d.select_track_group(1);return d;}
std::vector<tempo::Event> length_tempos(const SegmentDocument& source){
    auto events=source.primary_tempos();std::int32_t previous=-1;
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
std::optional<std::int32_t> segment_playback_clock(std::int32_t length,const SegmentLoopSettings& loop,std::int32_t startPoint,std::int64_t elapsed){
    if(!loop.music_time()||length<=0||startPoint<0||startPoint>=length||elapsed<0)return {};
    const auto linear=[&]{return elapsed>=std::int64_t(length)-startPoint?length-1:static_cast<std::int32_t>(startPoint+elapsed);};
    // Repeat0 ignores the saved loop interval, including imported stale bounds.
    if(!loop.repeats)return linear();
    const std::int64_t end=loop.end?loop.end:length;
    if(loop.start<0||end<=loop.start||end>length||startPoint>=end)return {};
    const auto first=end-startPoint,span=end-loop.start;
    if(elapsed<first)return static_cast<std::int32_t>(startPoint+elapsed);
    const auto after=elapsed-first;
    if(loop.repeats==UINT32_MAX)return static_cast<std::int32_t>(loop.start+after%span);
    // Both factors are bounded by signed32 and finite unsigned32, so the
    // product fits signed64; avoid adding elapsed to it or to startPoint.
    const auto repeated=span*loop.repeats;
    if(after<repeated)return static_cast<std::int32_t>(loop.start+after%span);
    const auto tail=after-repeated;
    return tail>=length-end?length-1:static_cast<std::int32_t>(end+tail);
}
SegmentLoopSettings SegmentDocument::loop_settings()const{
    const auto header=root_.find("segh");if(!header||header->data.size()<20)throw std::runtime_error("Segment loop header missing");
    const auto& h=header->data;return {read32(h,0),static_cast<std::int32_t>(read32(h,12)),static_cast<std::int32_t>(read32(h,16)),h.size()>=36?read32(h,32):0};
}
bool SegmentDocument::set_music_loop(std::uint32_t repeats,std::int32_t start,std::int32_t end){
    const auto current=loop_settings();if(!current.music_time())throw std::runtime_error("Music-time loop editing requires a music-time Segment length and loop");
    const auto effectiveEnd=end?end:length();const auto playStart=static_cast<std::int32_t>(read32(root_.find("segh")->data,8));
    if(start<0||end<0||start>=length()||effectiveEnd<=start||effectiveEnd>length()||playStart<0||playStart>=length()||effectiveEnd<=playStart)throw std::runtime_error("Loop start must precede its end, and the end must be after playback start and within the Segment");
    if(current.repeats==repeats&&current.start==start&&current.end==end)return false;
    const auto before=save_bytes();auto next=*this;auto& h=next.root_.find("segh")->data;
    put32(h,0,repeats);put32(h,12,static_cast<std::uint32_t>(start));put32(h,16,static_cast<std::uint32_t>(end));
    next.record_edit(before);*this=std::move(next);return true;
}
std::int32_t SegmentDocument::play_start()const{
    const auto header=root_.find("segh");if(!header||header->data.size()<12)throw std::runtime_error("Segment playback start header missing");
    return static_cast<std::int32_t>(read32(header->data,8));
}
bool SegmentDocument::set_music_start(std::int32_t start){
    const auto loop=loop_settings();if(!loop.music_time())throw std::runtime_error("Music-time playback start requires a music-time Segment length and loop");
    const auto end=loop.end?loop.end:length();
    if(start<0||start>=length()||loop.start<0||loop.start>=length()||end<=loop.start||end>length()||start>=end)throw std::runtime_error("Playback start must be within the Segment and before its loop end");
    if(play_start()==start)return false;
    const auto before=save_bytes();auto next=*this;put32(next.root_.find("segh")->data,8,static_cast<std::uint32_t>(start));
    next.record_edit(before);*this=std::move(next);return true;
}
std::uint32_t SegmentDocument::default_resolution()const{
    const auto header=root_.find("segh");if(!header||header->data.size()<24)throw std::runtime_error("Segment default resolution missing");
    return read32(header->data,20);
}
std::optional<SegmentPerformanceBoundary> SegmentDocument::performance_boundary()const{
    const auto value=default_resolution();
    if(value&0x4000u)return {}; // A Segment cannot use itself as its default.
    switch(value&boundaryMask){case 0:return SegmentPerformanceBoundary::immediate;case 0x800:return SegmentPerformanceBoundary::grid;case 0x1000:return SegmentPerformanceBoundary::beat;case 0x2000:return SegmentPerformanceBoundary::barline;case 0x11000:return SegmentPerformanceBoundary::align_beat;case 0x12000:return SegmentPerformanceBoundary::align_barline;case 0x810000:return SegmentPerformanceBoundary::align_segment;case 0x800000:return SegmentPerformanceBoundary::segment_end;case 0x100:return SegmentPerformanceBoundary::queue_end;default:return {};}
}
bool SegmentDocument::update_default_resolution(std::uint32_t mask,std::uint32_t value){
    if(value&~mask)throw std::runtime_error("Invalid Segment resolution field");
    const auto current=default_resolution(),updated=(current&~mask)|value;if(current==updated)return false;
    const auto before=save_bytes();auto next=*this;put32(next.root_.find("segh")->data,20,updated);next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::set_performance_boundary(SegmentPerformanceBoundary boundary){
    std::uint32_t bits=0;
    switch(boundary){case SegmentPerformanceBoundary::immediate:break;case SegmentPerformanceBoundary::grid:bits=0x800;break;case SegmentPerformanceBoundary::beat:bits=0x1000;break;case SegmentPerformanceBoundary::barline:bits=0x2000;break;case SegmentPerformanceBoundary::align_beat:bits=0x11000;break;case SegmentPerformanceBoundary::align_barline:bits=0x12000;break;case SegmentPerformanceBoundary::align_segment:bits=0x810000;break;case SegmentPerformanceBoundary::segment_end:bits=0x800000;break;case SegmentPerformanceBoundary::queue_end:bits=0x100;break;default:throw std::runtime_error("Unknown Segment performance boundary");}
    const auto current=performance_boundary();if(!current)throw std::runtime_error("Imported Segment boundary flags conflict or use DEFAULT");
    const auto raw=default_resolution();const bool markerBlocked=(bits&0x10100u)!=0;
    if(*current==boundary&&(!markerBlocked||!(raw&markerFlag)))return false;
    std::uint32_t mask=boundaryMask;if(markerBlocked)mask|=markerFlag;
    // Inactive original alignment radios are cleared. Entering Align therefore
    // stores no valid-start bit, which the original refresh displays as Time.
    if(!(bits&0x10000u)||!(raw&0x10000u))mask|=alignmentMask;
    return update_default_resolution(mask,bits);
}
std::optional<SegmentAlignment> SegmentDocument::alignment()const{
    if(!performance_boundary()||!(default_resolution()&0x10000u))return {};
    switch(default_resolution()&alignmentMask){case 0:case 0x80000:return SegmentAlignment::time;case 0x40000:return SegmentAlignment::grid;case 0x20000:return SegmentAlignment::beat;case 0x8000000:return SegmentAlignment::barline;default:return {};}
}
bool SegmentDocument::set_alignment(SegmentAlignment value){
    std::uint32_t bits=0;switch(value){case SegmentAlignment::time:bits=0x80000;break;case SegmentAlignment::grid:bits=0x40000;break;case SegmentAlignment::beat:bits=0x20000;break;case SegmentAlignment::barline:bits=0x8000000;break;default:throw std::runtime_error("Unknown Segment alignment choice");}
    const auto current=alignment();if(!current)throw std::runtime_error("Segment alignment is inactive or conflicting");
    if(*current==value&&!(default_resolution()&markerFlag))return false;
    return update_default_resolution(alignmentMask|markerFlag,bits);
}
bool SegmentDocument::set_boundary_marker(bool enabled){if(enabled&&(default_resolution()&0x10100u))throw std::runtime_error("Marker is unavailable for Align and Queue boundaries");return update_default_resolution(markerFlag,enabled?markerFlag:0);}
bool SegmentDocument::set_boundary_after_prepare(bool enabled){return update_default_resolution(0x400,enabled?0x400:0);}
std::optional<SegmentInvalidation> SegmentDocument::invalidation()const{
    switch(default_resolution()&0x10008000u){case 0x8000:return SegmentInvalidation::dont_cut_off;case 0x10000000:return SegmentInvalidation::cut_primary;case 0:return SegmentInvalidation::cut_all;default:return {};}
}
bool SegmentDocument::set_invalidation(SegmentInvalidation value){
    // Formal SegmentDesigner Boundary handler (radio IDs1216..1218) stores
    // NOINVALIDATE, INVALIDATE_PRI, or neither. Never normalize both-bit input.
    std::uint32_t bits=0;
    switch(value){case SegmentInvalidation::dont_cut_off:bits=0x8000;break;case SegmentInvalidation::cut_primary:bits=0x10000000;break;case SegmentInvalidation::cut_all:break;default:throw std::runtime_error("Unknown Segment invalidation choice");}
    if(!invalidation())throw std::runtime_error("Imported Segment invalidation flags conflict");
    return update_default_resolution(0x10008000,bits);
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
