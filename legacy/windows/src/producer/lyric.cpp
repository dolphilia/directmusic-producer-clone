#include "lyric.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
template<class C> auto unique(C& parent,const char* id,const char* type="",bool required=true)->decltype(parent.find(id,type)) {
    decltype(parent.find(id,type)) found=nullptr;
    for(auto& c:parent.children)if(c.id==id&&(!*type||c.type==type)) {if(found)throw std::runtime_error("Ambiguous Lyric container or field");found=&c;}
    if(!found&&required)throw std::runtime_error("Lyric container or field missing");return found;
}
bool unicode(const std::wstring& text){for(size_t i=0;i<text.size();++i){auto c=static_cast<unsigned>(text[i]);if(!c||c>0xffff)return false;if(c>=0xd800&&c<=0xdbff){if(++i==text.size())return false;auto d=static_cast<unsigned>(text[i]);if(d<0xdc00||d>0xdfff)return false;}else if(c>=0xdc00&&c<=0xdfff)return false;}return true;}
size_t terminator(const Bytes& b){if(b.size()%2)throw std::runtime_error("Odd Lyric UTF-16 payload");for(size_t i=0;i+1<b.size();i+=2)if(!b[i]&&!b[i+1])return i;throw std::runtime_error("Unterminated Lyric text");}
LyricEvent event(const Chunk& entry){const auto& h=unique(entry,"lyrh")->data;const auto& b=unique(entry,"lyrn")->data;if(h.size()<16||read32(h,0))throw std::runtime_error("Invalid Lyric header or reserved flags");LyricEvent e;e.timing=read32(h,4);e.logical=static_cast<std::int32_t>(read32(h,8));e.physical=static_cast<std::int32_t>(read32(h,12));auto end=terminator(b);for(size_t i=0;i<end;i+=2)e.text.push_back(static_cast<wchar_t>(b[i]|(b[i+1]<<8)));if(!valid_lyric(e))throw std::runtime_error("Invalid Lyric timing, time or Unicode");return e;}
struct Record {Chunk entry;LyricEvent value;bool selected=false;};
const Chunk& payload(const Chunk& track){const auto t=unique(track,"LIST","lyrt",false);if(t)return *unique(*t,"LIST","lyrl");static const Chunk empty=[] {Chunk c;c.id="LIST";c.type="lyrl";return c;}();return empty;}
std::vector<Record> records(const Chunk& list){std::vector<Record> out;for(const auto& c:list.children)if(c.id=="LIST"&&c.type=="lyre")out.push_back({c,event(c)});return out;}
void update(Chunk& entry,const LyricEvent& e){const auto previous=event(entry);auto& h=unique(entry,"lyrh")->data;put32(h,4,e.timing);put32(h,8,static_cast<std::uint32_t>(e.logical));put32(h,12,static_cast<std::uint32_t>(e.physical));if(previous.text!=e.text){auto& b=unique(entry,"lyrn")->data;const auto end=terminator(b);Bytes next=utf16(e.text);next.insert(next.end(),b.begin()+end+2,b.end());b=std::move(next);}}
Chunk fresh(const LyricEvent& e){Chunk entry;entry.id="LIST";entry.type="lyre";Chunk h;h.id="lyrh";h.data=Bytes(16);put32(h.data,4,e.timing);put32(h.data,8,static_cast<std::uint32_t>(e.logical));put32(h.data,12,static_cast<std::uint32_t>(e.physical));Chunk n;n.id="lyrn";n.data=utf16(e.text);entry.children={h,n};return entry;}
bool commit(Chunk& track,std::vector<Record> r,size_t* result){std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.value.physical<b.value.physical;});auto next=track;if(!unique(next,"LIST","lyrt",false)){Chunk t;t.id="LIST";t.type="lyrt";t.children={payload(track)};next.children.push_back(std::move(t));}auto list=unique(*unique(next,"LIST","lyrt"),"LIST","lyrl");size_t current=0,selected=0;std::vector<Chunk> children;for(const auto& c:list->children){if(c.id=="LIST"&&c.type=="lyre"){if(current<r.size()){if(r[current].selected)selected=current;children.push_back(r[current++].entry);}}else children.push_back(c);}while(current<r.size()){if(r[current].selected)selected=current;children.push_back(r[current++].entry);}list->children=std::move(children);if(next.encode()==track.encode())return false;track=std::move(next);if(result)*result=selected;return true;}
}
bool valid_lyric(const LyricEvent& e){return e.physical>=0&&e.logical>=0&&e.timing&&(e.timing&~28u)==0&&unicode(e.text);}
std::vector<LyricEvent> lyric_events(const Chunk& track){std::vector<LyricEvent> out;for(const auto& r:records(payload(track)))out.push_back(r.value);return out;}
bool add_lyric_event(Chunk& track,const LyricEvent& e,size_t* result){if(!valid_lyric(e))return false;auto r=records(payload(track));r.push_back({fresh(e),e,true});return commit(track,std::move(r),result);}
bool change_lyric_event(Chunk& track,size_t index,const LyricEvent& e,size_t* result){if(!valid_lyric(e))return false;auto r=records(payload(track));if(index>=r.size())return false;update(r[index].entry,e);r[index].value=e;r[index].selected=true;return commit(track,std::move(r),result);}
bool delete_lyric_event(Chunk& track,size_t index){auto r=records(payload(track));if(index>=r.size())return false;r.erase(r.begin()+index);return commit(track,std::move(r),nullptr);}
Bytes copy_lyric_event(const Chunk& track,size_t index){const auto r=records(payload(track));if(index>=r.size())throw std::runtime_error("Lyric selection out of range");return r[index].entry.encode();}
bool paste_lyric_event(Chunk& track,const Bytes& bytes,std::int32_t physical,std::int32_t logical,size_t* result){auto entry=Chunk::parse_list(bytes);if(entry.type!="lyre")throw std::runtime_error("Expected Lyric clipboard");auto e=event(entry);e.physical=physical;e.logical=logical;if(!valid_lyric(e))return false;update(entry,e);auto r=records(payload(track));r.push_back({entry,e,true});return commit(track,std::move(r),result);}

Bytes copy_lyric_range(const Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)throw std::runtime_error("Invalid Lyric source range");
    Chunk clip;clip.id="LIST";clip.type="LYRC";Chunk origin;origin.id="lorg";origin.data.resize(8);put32(origin.data,0,begin);put32(origin.data,4,end-begin);clip.children.push_back(origin);
    for(const auto& r:records(payload(track)))if(r.value.physical>=begin&&r.value.physical<end)clip.children.push_back(r.entry);
    return clip.encode();
}
bool delete_lyric_range(Chunk& track,std::int32_t begin,std::int32_t end){
    if(begin<0||end<=begin)return false;auto r=records(payload(track));const auto count=r.size();r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& r){return r.value.physical>=begin&&r.value.physical<end;}),r.end());
    return count!=r.size()&&commit(track,std::move(r),nullptr);
}
namespace {
std::vector<Record> lyric_range_entries(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){
    if(at<0||span<=0||std::int64_t(at)+span>length)throw std::runtime_error("Invalid Lyric destination range");
    const auto clip=Chunk::parse_list(bytes);if(clip.type!="LYRC"||clip.encode()!=bytes)throw std::runtime_error("Invalid Lyric range clipboard");const auto& origin=unique(clip,"lorg")->data;
    if(origin.size()!=8||read32(origin,0)>INT32_MAX||read32(origin,4)!=static_cast<std::uint32_t>(span)||std::int64_t(read32(origin,0))+span>INT32_MAX)throw std::runtime_error("Lyric range clocks outside bounds");
    const auto begin=static_cast<std::int32_t>(read32(origin,0));const auto delta=std::int64_t(at)-begin;std::vector<Record> incoming;
    for(const auto& c:clip.children){if(c.id=="lorg")continue;if(c.id!="LIST"||c.type!="lyre")throw std::runtime_error("Unexpected Lyric clipboard entry");auto e=event(c);if(e.physical<begin||std::int64_t(e.physical)>=std::int64_t(begin)+span)throw std::runtime_error("Lyric range clocks outside bounds");
        const auto physical=std::int64_t(e.physical)+delta,logical=std::int64_t(e.logical)+delta;if(physical<0||physical>=length||logical<0||logical>=length)throw std::runtime_error("Lyric range clocks outside bounds");
        auto entry=c;e.physical=static_cast<std::int32_t>(physical);e.logical=static_cast<std::int32_t>(logical);update(entry,e);incoming.push_back({std::move(entry),e,true});}
    return incoming;
}
}
bool lyric_range_empty(const Bytes& bytes,std::int32_t at,std::int32_t span,std::int32_t length){return lyric_range_entries(bytes,at,span,length).empty();}
bool paste_lyric_range(Chunk& track,const Bytes& bytes,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length){
    const auto incoming=lyric_range_entries(bytes,at,span,length);
    auto r=records(payload(track));if(overwrite)r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& r){return r.value.physical>=at&&std::int64_t(r.value.physical)<std::int64_t(at)+span;}),r.end());
    if(incoming.empty()&&!overwrite)return false;r.insert(r.end(),incoming.begin(),incoming.end());return commit(track,std::move(r),nullptr);
}

Chunk lyric_track(){Chunk track;track.id="RIFF";track.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);const Bytes id={0xf5,0x1c,0x5c,0x99,0xff,0x54,0xd3,0x11,0x8b,0xda,0x00,0x60,0x08,0x93,0xb1,0xb6};std::copy(id.begin(),id.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+28,"lyrt",4);Chunk x;x.id="trkx";x.data=Bytes(8);put32(x.data,0,16);Chunk t;t.id="LIST";t.type="lyrt";Chunk l;l.id="LIST";l.type="lyrl";t.children={l};track.children={h,x,t};return track;}
}
