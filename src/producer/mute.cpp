#include "mute.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <set>
namespace producer::app {
namespace {
template<class C> auto payload(C& t)->decltype(t.find("mute")) {decltype(t.find("mute")) found=nullptr;for(auto& c:t.children)if(c.id=="mute"){if(found)throw std::runtime_error("Ambiguous Mute payload");found=&c;}return found;}
struct Record {Bytes bytes;MuteEvent event;bool selected=false;};
MuteEvent decode(const Bytes& b){if(b.size()<12)throw std::runtime_error("Truncated Mute record");MuteEvent e{static_cast<std::int32_t>(read32(b,0)),read32(b,4),read32(b,8)};if(!valid_mute(e))throw std::runtime_error("Invalid Mute clock or PChannel");return e;}
void encode(Bytes& b,MuteEvent e){put32(b,0,static_cast<std::uint32_t>(e.time));put32(b,4,e.channel);put32(b,8,e.map);}
std::vector<Record> records(const Chunk& t){const auto p=payload(t);if(!p)return {};const auto& b=p->data;if(b.size()<4)throw std::runtime_error("Missing Mute stride");const auto stride=read32(b,0);if(stride<12||(b.size()-4)%stride)throw std::runtime_error("Invalid Mute stride or tail");std::vector<Record> r;for(size_t i=4;i<b.size();i+=stride){Bytes v(b.begin()+i,b.begin()+i+stride);r.push_back({v,decode(v)});}return r;}
bool same(MuteEvent a,MuteEvent b){return a.time==b.time&&a.channel==b.channel&&a.map==b.map;}
bool collision(const std::vector<Record>& r,MuteEvent e,size_t except=static_cast<size_t>(-1)){for(size_t i=0;i<r.size();++i)if(i!=except&&r[i].event.channel==e.channel&&r[i].event.time==e.time)return true;return false;}
size_t stride(const Chunk& t){auto p=payload(t);return p?read32(p->data,0):12;}
bool commit(Chunk& t,std::vector<Record> r,size_t* selected){std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.event.channel!=b.event.channel?a.event.channel<b.event.channel:a.event.time<b.event.time;});auto next=t;auto p=payload(next);if(!p){Chunk c;c.id="mute";c.data=Bytes(4);put32(c.data,0,12);next.children.push_back(c);p=&next.children.back();}const auto n=read32(p->data,0);Bytes b(4);put32(b,0,n);size_t picked=0;for(size_t i=0;i<r.size();++i){if(r[i].selected)picked=i;auto v=r[i].bytes;v.resize(n);encode(v,r[i].event);b.insert(b.end(),v.begin(),v.end());}p->data=std::move(b);if(next.encode()==t.encode())return false;t=std::move(next);if(selected)*selected=picked;return true;}
}
bool valid_mute(const MuteEvent& e){return e.time>=0&&e.channel<0xfffffffcu&&(e.map<0xfffffffcu||e.map==mute_channel_silent);}
namespace {
std::vector<Record> range_records(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){
    if(at<0||span<=0||std::int64_t(at)+span>length)throw std::runtime_error("Mute range outside Segment");
    const auto clip=Chunk::parse_list(bytes);if(clip.type!="MUTC"||clip.encode()!=bytes||clip.children.size()>1||(!clip.children.empty()&&clip.children[0].id!="mute"))throw std::runtime_error("Invalid Mute range clipboard");
    auto incoming=records(clip);std::set<std::pair<std::uint32_t,std::int32_t>> keys;for(const auto& r:incoming)if(r.event.time>=span||!keys.emplace(r.event.channel,r.event.time).second)throw std::runtime_error("Mute range clock or duplicate key");
    for(auto& r:incoming){r.event.time+=at;encode(r.bytes,r.event);}return incoming;
}
}
Bytes copy_mute_range(const Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)throw std::runtime_error("Invalid Mute range");const auto all=records(track);Chunk clip;clip.id="LIST";clip.type="MUTC";
    if(const auto source=payload(track)){Chunk c;c.id="mute";c.data=Bytes(4);put32(c.data,0,stride(track));for(const auto& r:all)if(r.event.time>=begin&&r.event.time<end){auto b=r.bytes;put32(b,0,r.event.time-begin);c.data.insert(c.data.end(),b.begin(),b.end());}clip.children.push_back(std::move(c));}
    return clip.encode();
}
bool delete_mute_range(Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)throw std::runtime_error("Invalid Mute range");auto r=records(track);const auto count=r.size();r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& v){return v.event.time>=begin&&v.event.time<end;}),r.end());return r.size()!=count&&commit(track,std::move(r),nullptr);
}
bool mute_range_empty(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){return range_records(bytes,at,span,length).empty();}
bool paste_mute_range(Chunk& track,const Bytes& bytes,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length){
    auto incoming=range_records(bytes,at,span,length);auto existing=records(track);auto next=track;
    if(!incoming.empty()){
        const auto n=incoming[0].bytes.size();if(!existing.empty()&&stride(track)!=n)throw std::runtime_error("Incompatible Mute range stride");
        if(existing.empty()){auto p=payload(next);if(!p){Chunk c;c.id="mute";next.children.push_back(std::move(c));p=&next.children.back();}p->data=Bytes(4);put32(p->data,0,n);}
    }
    const auto count=existing.size();if(overwrite)existing.erase(std::remove_if(existing.begin(),existing.end(),[&](const Record& r){return r.event.time>=at&&r.event.time<std::int64_t(at)+span;}),existing.end());
    if(incoming.empty()&&existing.size()==count)return false;
    std::set<std::pair<std::uint32_t,std::int32_t>> keys;for(const auto& r:existing)keys.emplace(r.event.channel,r.event.time);
    for(const auto& r:incoming){if(!keys.emplace(r.event.channel,r.event.time).second)throw std::runtime_error("Mute range collides with destination PChannel/time");existing.push_back(r);}
    if(!commit(next,std::move(existing),nullptr)||next.encode()==track.encode())return false;track=std::move(next);return true;
}
std::vector<MuteEvent> mute_events(const Chunk& t){std::vector<MuteEvent> out;for(const auto& r:records(t))out.push_back(r.event);return out;}
bool add_mute_event(Chunk& t,MuteEvent e,size_t* selected){if(!valid_mute(e))return false;auto r=records(t);if(collision(r,e))return false;Bytes b(stride(t));encode(b,e);r.push_back({b,e,true});return commit(t,std::move(r),selected);}
bool change_mute_event(Chunk& t,size_t i,MuteEvent e,size_t* selected){if(!valid_mute(e))return false;auto r=records(t);if(i>=r.size()||same(r[i].event,e)||collision(r,e,i))return false;r[i].event=e;r[i].selected=true;return commit(t,std::move(r),selected);}
bool delete_mute_event(Chunk& t,size_t i){auto r=records(t);if(i>=r.size())return false;r.erase(r.begin()+i);return commit(t,std::move(r),nullptr);}
Bytes copy_mute_event(const Chunk& t,size_t i){auto r=records(t);if(i>=r.size())throw std::runtime_error("Mute selection out of range");Chunk c;c.id="mute";c.data=Bytes(4);put32(c.data,0,static_cast<std::uint32_t>(r[i].bytes.size()));c.data.insert(c.data.end(),r[i].bytes.begin(),r[i].bytes.end());Chunk clipboard;clipboard.id="LIST";clipboard.type="MUTC";clipboard.children={c};return clipboard.encode();}
bool paste_mute_event(Chunk& t,const Bytes& bytes,std::int32_t at,size_t* selected){auto clipboard=Chunk::parse_list(bytes);if(clipboard.type!="MUTC")throw std::runtime_error("Expected Mute clipboard");auto incoming=records(clipboard);if(incoming.size()!=1)throw std::runtime_error("Expected one Mute clipboard record");auto v=incoming.front();v.event.time=at;if(!valid_mute(v.event))return false;auto r=records(t);if(collision(r,v.event))return false;const auto n=stride(t);if(v.bytes.size()>n)throw std::runtime_error("Mute clipboard extension cannot fit destination stride");v.bytes.resize(n);v.selected=true;r.push_back(v);return commit(t,std::move(r),selected);}
Chunk mute_track(){Chunk t;t.id="RIFF";t.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);const Bytes id={0x98,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd};std::copy(id.begin(),id.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+24,"mute",4);Chunk x;x.id="trkx";x.data=Bytes(8);put32(x.data,0,8);t.children={h,x};return t;}
}
