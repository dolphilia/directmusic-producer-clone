#include "timeline.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace producer::app {
Bytes encode_timeline_clipboard(const TimelineClipboard& value){
    if(value.span<=0||!value.strips||(value.strips&~TimelineAll)||(!(value.strips&TimelineTempo)&&!value.tempo.empty())||(!(value.strips&TimelineSequence)&&!value.sequence.empty())||(!(value.strips&TimelineLyric)&&!value.lyric.empty())||(!(value.strips&TimelineMarker)&&!value.marker.empty())||(!(value.strips&TimelineMute)&&!value.mute.empty()))throw std::runtime_error("Invalid Timeline clipboard");
    Chunk root;root.id="RIFF";root.type="TRNG";Chunk header;header.id="rhdr";header.data.resize(12);put32(header.data,0,1);put32(header.data,4,value.span);put32(header.data,8,value.strips);root.children.push_back(header);
    if(value.strips&TimelineTempo){Chunk c;c.id="temp";c.data=value.tempo;root.children.push_back(std::move(c));}
    if(value.strips&TimelineSequence){Chunk c;c.id="seqc";c.data=value.sequence;root.children.push_back(std::move(c));}
    if(value.strips&TimelineLyric){Chunk c;c.id="lyrc";c.data=value.lyric;root.children.push_back(std::move(c));}
    if(value.strips&TimelineMarker){Chunk c;c.id="mark";c.data=value.marker;root.children.push_back(std::move(c));}
    if(value.strips&TimelineMute){Chunk c;c.id="mutc";c.data=value.mute;root.children.push_back(std::move(c));}
    return root.encode();
}
TimelineClipboard decode_timeline_clipboard(const Bytes& bytes){
    if(bytes.size()>64*1024*1024)throw std::runtime_error("Timeline clipboard too large");const auto root=Chunk::parse(bytes);if(root.id!="RIFF"||root.type!="TRNG"||root.encode()!=bytes)throw std::runtime_error("Invalid Timeline clipboard envelope");
    TimelineClipboard result;bool header=false,tempo=false,sequence=false,lyric=false,marker=false,mute=false;
    for(const auto& c:root.children){if(c.id=="rhdr"&&!header&&c.data.size()==12&&read32(c.data,0)==1){header=true;result.span=static_cast<std::int32_t>(read32(c.data,4));result.strips=read32(c.data,8);}else if(c.id=="temp"&&!tempo){tempo=true;result.tempo=c.data;}else if(c.id=="seqc"&&!sequence){sequence=true;result.sequence=c.data;}else if(c.id=="lyrc"&&!lyric){lyric=true;result.lyric=c.data;}else if(c.id=="mark"&&!marker){marker=true;result.marker=c.data;}else if(c.id=="mutc"&&!mute){mute=true;result.mute=c.data;}else throw std::runtime_error("Ambiguous or unsupported Timeline clipboard chunk");}
    if(!header||result.span<=0||!result.strips||(result.strips&~TimelineAll)||tempo!=bool(result.strips&TimelineTempo)||sequence!=bool(result.strips&TimelineSequence)||lyric!=bool(result.strips&TimelineLyric)||marker!=bool(result.strips&TimelineMarker)||mute!=bool(result.strips&TimelineMute))throw std::runtime_error("Invalid Timeline clipboard selection");return result;
}
namespace {
struct MeterAt { std::int32_t time; unsigned beats, denominator; };
std::vector<MeterAt> records(const meter::Map& map) {
    const auto b=map.save();std::vector<MeterAt> out;
    // Map::save emits LIST:TIMS/tims with 8-byte records.
    for(size_t at=24;at+8<=b.size();at+=8)out.push_back({static_cast<std::int32_t>(read32(b,at)),b[at+4],b[at+5]});
    return out;
}
std::int32_t checked(std::int64_t value) { if(value<0||value>INT32_MAX)throw std::runtime_error("Timeline position out of range");return static_cast<std::int32_t>(value); }
void validate(const Chunk& c) {
    if(c.id=="tims") {
        if(c.data.size()<4||read32(c.data,0)!=8||(c.data.size()-4)%8)throw std::runtime_error("Unsupported meter records");
        for(size_t at=4;at<c.data.size();at+=8) {
            const unsigned beats=c.data[at+4],den=c.data[at+5];
            if(read32(c.data,at)>INT32_MAX||!beats||!den||den>128||(den&(den-1))||(!c.data[at+6]&&!c.data[at+7]))throw std::runtime_error("Invalid meter value");
        }
    } else if(c.id=="LIST"&&c.type=="TIMS") { for(const auto& child:c.children)validate(child); }
    else throw std::runtime_error("Unsupported meter chunk");
}
}
Timeline::Timeline() { load_meter(Bytes{'t','i','m','s',12,0,0,0,8,0,0,0,0,0,0,0,4,4,4,0}); }
void Timeline::load_meter(const Bytes& bytes) {
    // Parse within a synthetic RIFF envelope; all input validation precedes
    // calling the legacy-compatible model which assumes valid denominators.
    Bytes envelope{'R','I','F','F',0,0,0,0,'T','E','S','T'};envelope.insert(envelope.end(),bytes.begin(),bytes.end());put32(envelope,4,static_cast<std::uint32_t>(envelope.size()-8));
    const auto root=Chunk::parse(envelope);Bytes typed;
    const auto collect=[&](const auto& self,const Chunk& c)->void{
        if(c.id=="tims"){validate(c);const auto b=c.encode();typed.insert(typed.end(),b.begin(),b.end());}
        else if(c.id=="LIST"&&c.type=="TIMS"){
            // The document retains opaque siblings. Feed only typed meter
            // records to the legacy map, whose parser rejects unknown chunks.
            for(const auto& child:c.children)if(child.id=="tims"||(child.id=="LIST"&&child.type=="TIMS"))self(self,child);
        }else throw std::runtime_error("Unsupported meter chunk");
    };for(const auto& c:root.children)collect(collect,c);
    meter::Map next;if(next.load(typed)!=meter::LoadResult::ok||next.events().empty())throw std::runtime_error("Meter load failed");
    const auto r=records(next); if(r.empty()||r.front().time!=0)throw std::runtime_error("Meter must start at zero");
    for(size_t i=1;i<r.size();++i)if(r[i].time<=r[i-1].time)throw std::runtime_error("Meter overflow or order");
    meters_=std::move(next);
}
Position Timeline::position(std::int32_t clocks) const {
    if(clocks<0)throw std::runtime_error("Negative timeline position");
    const auto r=records(meters_);std::int64_t measure=0;auto active=r.front();
    for(size_t i=1;i<r.size()&&r[i].time<=clocks;++i) {measure+=(r[i].time-active.time)/((3072/active.denominator)*active.beats);active=r[i];}
    const auto beatLength=3072/active.denominator;const auto delta=clocks-active.time;
    return {checked(measure+delta/(beatLength*active.beats)),static_cast<std::int32_t>((delta/beatLength)%active.beats),static_cast<std::int32_t>(delta%beatLength)};
}
std::int32_t Timeline::clocks(Position p) const {
    if(p.measure<0||p.beat<0||p.tick<0)throw std::runtime_error("Negative musical position");
    const auto r=records(meters_);std::int64_t measure=0;auto active=r.front();
    for(size_t i=1;i<r.size();++i) { const auto next=measure+(r[i].time-active.time)/((3072/active.denominator)*active.beats);if(next>p.measure)break;measure=next;active=r[i]; }
    const auto beatLength=3072/active.denominator;if(static_cast<unsigned>(p.beat)>=active.beats||p.tick>=static_cast<int>(beatLength))throw std::runtime_error("Beat or tick out of range");
    return checked(active.time+(std::int64_t(p.measure)-measure)*beatLength*active.beats+std::int64_t(p.beat)*beatLength+p.tick);
}
int Timeline::pixel(std::int32_t time,double zoom,int scroll) const {
    if(!std::isfinite(zoom)||zoom<=0)throw std::runtime_error("Invalid zoom");const double result=std::round(time*zoom)-scroll;
    if(result<INT32_MIN||result>INT32_MAX)throw std::runtime_error("Pixel out of range");return static_cast<int>(result);
}
}
