#include "marker.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
struct Record {MarkerEvent event;Bytes bytes;bool selected=false;};
const char* key(MarkerKind kind){return kind==MarkerKind::play?"play":"vals";}
bool valid(MarkerEvent e){return e.time>=0&&(e.kind==MarkerKind::play||e.kind==MarkerKind::enter);}
const Chunk& payload(const Chunk& track){
    const Chunk* first=nullptr;
    for(const auto& c:track.children)if(c.id=="LIST"&&c.type=="MARK"){
        if(!first)first=&c;
        else if(c.encode()!=first->encode())throw std::runtime_error("Inconsistent duplicate Marker containers");
    }
    if(!first)throw std::runtime_error("Marker container missing");
    return *first;
}
const Chunk* leaf(const Chunk& mark,MarkerKind kind){const Chunk* found=nullptr;for(const auto& c:mark.children)if(c.id==key(kind)){
    if(found)throw std::runtime_error("Ambiguous Marker records");found=&c;
}return found;}
size_t stride(const Chunk* c){if(!c)return 4;const auto& b=c->data;if(b.size()<4)throw std::runtime_error("Truncated Marker header");const auto s=read32(b,0);if(s<4||(b.size()-4)%s)throw std::runtime_error("Invalid Marker stride");return s;}
std::vector<Record> records(const Chunk& mark){std::vector<Record> out;for(auto kind:{MarkerKind::play,MarkerKind::enter}){
    const auto c=leaf(mark,kind);const auto s=stride(c);if(!c)continue;
    for(size_t p=4;p<c->data.size();p+=s){const auto t=read32(c->data,p);if(t>INT32_MAX)throw std::runtime_error("Negative Marker time");out.push_back({{static_cast<std::int32_t>(t),kind},Bytes(c->data.begin()+p,c->data.begin()+p+s)});}
}std::stable_sort(out.begin(),out.end(),[](const Record& a,const Record& b){return a.event.time<b.event.time;});return out;}
bool commit(Chunk& track,Chunk mark,std::vector<Record> r,size_t* result){
    // Separate RIFF arrays imply play-before-enter at an equal time on reload.
    // Return selection in that same order, including newly inserted overlaps.
    std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.event.time!=b.event.time?a.event.time<b.event.time:static_cast<int>(a.event.kind)<static_cast<int>(b.event.kind);});
    size_t selected=0;for(size_t i=0;i<r.size();++i)if(r[i].selected)selected=i;
    for(auto kind:{MarkerKind::play,MarkerKind::enter}){
        auto c=mark.find(key(kind));const auto s=stride(c);Bytes bytes(4);put32(bytes,0,static_cast<std::uint32_t>(s));
        bool has=false;for(const auto& v:r)if(v.event.kind==kind){has=true;bytes.insert(bytes.end(),v.bytes.begin(),v.bytes.end());}
        if(c)c->data=std::move(bytes);
        else if(has){Chunk fresh;fresh.id=key(kind);fresh.data=std::move(bytes);mark.children.push_back(std::move(fresh));}
    }
    auto next=track;for(auto& c:next.children)if(c.id=="LIST"&&c.type=="MARK")c=mark;
    if(next.encode()==track.encode())return false;
    track=std::move(next);if(result)*result=selected;return true;
}
}
std::vector<MarkerEvent> marker_events(const Chunk& track){std::vector<MarkerEvent> out;for(const auto& r:records(payload(track)))out.push_back(r.event);return out;}
bool add_marker_event(Chunk& track,MarkerEvent e,size_t* result){if(!valid(e))return false;auto mark=payload(track);auto r=records(mark);Bytes b(stride(leaf(mark,e.kind)));put32(b,0,static_cast<std::uint32_t>(e.time));r.push_back({e,std::move(b),true});return commit(track,std::move(mark),std::move(r),result);}
bool change_marker_event(Chunk& track,size_t index,MarkerEvent e,size_t* result){if(!valid(e))return false;auto mark=payload(track);auto r=records(mark);if(index>=r.size())return false;auto& v=r[index];if(v.event.time==e.time&&v.event.kind==e.kind)return false;
    // Changing record type cannot discard a source extension or truncate it.
    const auto s=stride(leaf(mark,e.kind));if(e.kind!=v.event.kind&&v.bytes.size()>s)return false;
    v.bytes.resize(s);put32(v.bytes,0,static_cast<std::uint32_t>(e.time));v.event=e;v.selected=true;return commit(track,std::move(mark),std::move(r),result);
}
bool delete_marker_event(Chunk& track,size_t index){auto mark=payload(track);auto r=records(mark);if(index>=r.size())return false;r.erase(r.begin()+index);return commit(track,std::move(mark),std::move(r),nullptr);}
bool mark_marker_boundaries(Chunk& track,MarkerKind kind,const std::vector<std::int32_t>& times,bool marking){
    if(kind!=MarkerKind::play&&kind!=MarkerKind::enter)throw std::runtime_error("Invalid Marker kind");
    if(!std::is_sorted(times.begin(),times.end())||std::adjacent_find(times.begin(),times.end())!=times.end()||(!times.empty()&&times.front()<0))throw std::runtime_error("Invalid Marker boundary list");
    auto mark=payload(track);auto r=records(mark);const auto previousCount=r.size();
    if(marking){
        std::vector<std::int32_t> existing;for(const auto& v:r)if(v.event.kind==kind)existing.push_back(v.event.time);
        for(auto time:times)if(!std::binary_search(existing.begin(),existing.end(),time)){Bytes b(stride(leaf(mark,kind)));put32(b,0,static_cast<std::uint32_t>(time));r.push_back({{time,kind},std::move(b)});}
    }else r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& v){return v.event.kind==kind&&std::binary_search(times.begin(),times.end(),v.event.time);}),r.end());
    if(r.size()==previousCount)return false;
    return commit(track,std::move(mark),std::move(r),nullptr);
}
Bytes copy_marker_event(const Chunk& track,size_t index){const auto& mark=payload(track);const auto r=records(mark);if(index>=r.size())throw std::runtime_error("Marker selection out of range");Chunk out;out.id="LIST";out.type="MARK";Chunk c;c.id=key(r[index].event.kind);c.data=Bytes(4);put32(c.data,0,static_cast<std::uint32_t>(r[index].bytes.size()));c.data.insert(c.data.end(),r[index].bytes.begin(),r[index].bytes.end());out.children.push_back(std::move(c));return out.encode();}
bool paste_marker_event(Chunk& track,const Bytes& bytes,std::int32_t at,size_t* result){if(at<0)return false;const auto clip=Chunk::parse_list(bytes);if(clip.type!="MARK")throw std::runtime_error("Expected Marker clipboard");const auto copied=records(clip);if(copied.size()!=1||clip.children.size()!=1)throw std::runtime_error("Expected one Marker record");auto mark=payload(track);auto r=records(mark);auto v=copied[0];auto c=mark.find(key(v.event.kind));
    if(c){const auto s=stride(c);if(v.bytes.size()>s)return false;v.bytes.resize(s);}
    else {Chunk fresh;fresh.id=key(v.event.kind);fresh.data=Bytes(4);put32(fresh.data,0,static_cast<std::uint32_t>(v.bytes.size()));mark.children.push_back(std::move(fresh));}
    v.event.time=at;v.selected=true;put32(v.bytes,0,static_cast<std::uint32_t>(at));r.push_back(std::move(v));return commit(track,std::move(mark),std::move(r),result);
}
namespace {
std::vector<Record> range_records(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){
    if(at<0||span<=0||std::int64_t(at)+span>length)throw std::runtime_error("Marker range outside Segment");
    const auto clip=Chunk::parse_list(bytes);if(clip.type!="MARK"||clip.encode()!=bytes)throw std::runtime_error("Invalid Marker range clipboard");
    for(const auto& c:clip.children)if(c.id!="play"&&c.id!="vals")throw std::runtime_error("Unsupported Marker range chunk");
    auto incoming=records(clip);for(auto& r:incoming){if(r.event.time>=span)throw std::runtime_error("Marker outside clipboard span");r.event.time+=at;put32(r.bytes,0,r.event.time);}
    return incoming;
}
}
Bytes copy_marker_range(const Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)throw std::runtime_error("Invalid Marker range");const auto& mark=payload(track);const auto all=records(mark);
    Chunk clip;clip.id="LIST";clip.type="MARK";
    for(auto kind:{MarkerKind::play,MarkerKind::enter})if(const auto source=leaf(mark,kind)){
        Chunk c;c.id=key(kind);c.data=Bytes(4);put32(c.data,0,stride(source));
        for(const auto& r:all)if(r.event.kind==kind&&r.event.time>=begin&&r.event.time<end){auto b=r.bytes;put32(b,0,r.event.time-begin);c.data.insert(c.data.end(),b.begin(),b.end());}
        clip.children.push_back(std::move(c));
    }
    return clip.encode();
}
bool delete_marker_range(Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)throw std::runtime_error("Invalid Marker range");auto mark=payload(track);auto r=records(mark);const auto count=r.size();
    r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& v){return v.event.time>=begin&&v.event.time<end;}),r.end());
    return r.size()!=count&&commit(track,std::move(mark),std::move(r),nullptr);
}
bool marker_range_empty(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){return range_records(bytes,at,span,length).empty();}
bool paste_marker_range(Chunk& track,const Bytes& bytes,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length){
    auto incoming=range_records(bytes,at,span,length);auto mark=payload(track);auto existing=records(mark);
    // Validate record compatibility before private deletion. Empty arrays can
    // adopt the complete source stride; populated arrays cannot lose tails.
    for(auto kind:{MarkerKind::play,MarkerKind::enter}){
        const auto first=std::find_if(incoming.begin(),incoming.end(),[&](const Record& r){return r.event.kind==kind;});if(first==incoming.end())continue;
        auto c=mark.find(key(kind));const bool populated=std::any_of(existing.begin(),existing.end(),[&](const Record& r){return r.event.kind==kind;});
        if(populated&&stride(c)!=first->bytes.size())throw std::runtime_error("Incompatible Marker range stride");
        if(!populated){if(!c){Chunk fresh;fresh.id=key(kind);mark.children.push_back(std::move(fresh));c=&mark.children.back();}c->data=Bytes(4);put32(c->data,0,first->bytes.size());}
    }
    const auto count=existing.size();if(overwrite)existing.erase(std::remove_if(existing.begin(),existing.end(),[&](const Record& r){return r.event.time>=at&&r.event.time<std::int64_t(at)+span;}),existing.end());
    if(incoming.empty()&&existing.size()==count)return false;
    existing.insert(existing.end(),incoming.begin(),incoming.end());return commit(track,std::move(mark),std::move(existing),nullptr);
}
Chunk marker_track(){Chunk track;track.id="RIFF";track.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);const Bytes id={0x00,0xfd,0xa8,0x55,0x88,0x42,0xd3,0x11,0x9b,0xd1,0x8a,0x0d,0x61,0xc8,0x88,0x35};std::copy(id.begin(),id.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+28,"MARK",4);Chunk x;x.id="trkx";x.data=Bytes(8);put32(x.data,0,8);Chunk mark;mark.id="LIST";mark.type="MARK";track.children={h,x,mark};return track;}
}
