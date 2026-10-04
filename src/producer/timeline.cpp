#include "timeline.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace producer::app {
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
