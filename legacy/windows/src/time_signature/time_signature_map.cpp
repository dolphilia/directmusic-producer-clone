#include "time_signature_map.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace producer::meter {
namespace {
LONG wrapped(std::int64_t value){return static_cast<LONG>(static_cast<std::uint32_t>(value));}
LONG span(const Event& e){return (3072/e.denominator)*e.beats;}
DWORD u32(const std::vector<std::uint8_t>& bytes,size_t at){
    return DWORD(bytes[at])|(DWORD(bytes[at+1])<<8)|(DWORD(bytes[at+2])<<16)|(DWORD(bytes[at+3])<<24);
}
void append(std::vector<std::uint8_t>& bytes,DWORD value){for(int i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));}
}
LONG Map::measure_at(LONG time) const {
    // Recovered clocks-to-measure helper 0x62d6 clamps nonpositive input
    // when a map already exists. First-record Load uses its own signed divide.
    if(time<=0)return 0;
    if(events_.empty())return time/3072;
    LONG at=0,measure=0;Event meter=events_.front();
    for(const auto& event:events_){
        const LONG following=wrapped(std::int64_t(at)+std::int64_t(span(meter))*wrapped(std::int64_t(event.measure)-measure));
        if(following>time)break;
        at=following;measure=event.measure;meter=event;
    }
    return wrapped(std::int64_t(measure)+wrapped(std::int64_t(time)-at)/span(meter));
}
void Map::insert(TimeSignatureParam value){
    if(!value.beats)value.beats=4;
    if(!value.denominator)value.denominator=2;
    if(!value.grids)value.grids=2;
    const Event provisional{0,value.beats,value.denominator,value.grids};
    const LONG measure=events_.empty()?value.time/span(provisional):measure_at(value.time);
    const Event event{measure,value.beats,value.denominator,value.grids};
    const auto at=std::lower_bound(events_.begin(),events_.end(),static_cast<DWORD>(measure),
        [](const Event& a,DWORD b){return static_cast<DWORD>(a.measure)<b;});
    if(at!=events_.end()&&at->measure==measure)*at=event;else events_.insert(at,event);
}
LoadResult Map::chunks(const std::vector<std::uint8_t>& bytes,size_t begin,size_t end){
    while(begin<end){
        if(end-begin<8)return LoadResult::malformed;
        const DWORD id=u32(bytes,begin),count=u32(bytes,begin+4);const size_t data=begin+8;
        if(count>end-data)return LoadResult::malformed;
        if(id==0x736d6974){ // tims
            if(count<4)return LoadResult::malformed;
            const DWORD recordSize=u32(bytes,data);
            if(recordSize<8)return LoadResult::unsupported;
            if((count-4)%recordSize)return LoadResult::malformed;
            for(size_t at=data+4;at<data+count;at+=recordSize){
                TimeSignatureParam value{};value.time=static_cast<LONG>(u32(bytes,at));
                value.beats=bytes[at+4];value.denominator=bytes[at+5];value.grids=WORD(bytes[at+6])|(WORD(bytes[at+7])<<8);
                insert(value);
            }
        }else if(id==0x5453494c){ // LIST:TIMS
            if(count<4)return LoadResult::malformed;
            if(u32(bytes,data)!=0x534d4954)return LoadResult::unsupported;
            const auto result=chunks(bytes,data+4,data+count);if(result!=LoadResult::ok)return result;
        }else return LoadResult::unsupported;
        const size_t next=data+count+(count&1);
        if(next>end)return LoadResult::malformed;
        begin=next;
    }
    return LoadResult::ok;
}
LoadResult Map::load(const std::vector<std::uint8_t>& bytes){
    events_.clear();return chunks(bytes,0,bytes.size());
}
Query Map::query(LONG time) const {
    Query result;if(events_.empty())return result;
    result.found=true;Event meter=events_.front();LONG at=0,measure=0;
    for(const auto& event:events_){
        const LONG following=wrapped(std::int64_t(at)+std::int64_t(span(meter))*wrapped(std::int64_t(event.measure)-measure));
        if(following>time){result.next=wrapped(std::int64_t(following)-time);break;}
        at=following;measure=event.measure;meter=event;
    }
    result.value={0,meter.beats,meter.denominator,meter.grids};return result;
}
std::vector<std::uint8_t> Map::save() const {
    std::vector<std::uint8_t> bytes{'L','I','S','T'};
    if(events_.size()>((std::numeric_limits<DWORD>::max)()-16)/8)throw std::bad_alloc();
    append(bytes,events_.empty()?4:16+static_cast<DWORD>(events_.size())*8);append(bytes,0x534d4954);
    if(events_.empty())return bytes;
    append(bytes,0x736d6974);append(bytes,4+static_cast<DWORD>(events_.size())*8);append(bytes,8);
    LONG at=0,measure=0;Event meter=events_.front();
    for(const auto& event:events_){
        at=wrapped(std::int64_t(at)+std::int64_t(span(meter))*wrapped(std::int64_t(event.measure)-measure));
        append(bytes,static_cast<DWORD>(at));bytes.push_back(event.beats);bytes.push_back(event.denominator);
        bytes.push_back(static_cast<BYTE>(event.grids));bytes.push_back(static_cast<BYTE>(event.grids>>8));
        measure=event.measure;meter=event;
    }
    return bytes;
}
}
