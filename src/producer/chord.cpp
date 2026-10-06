#include "chord.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
struct Record {ChordEvent event;Chunk chunk;};
ChordEvent decode(const Bytes& b) {
    if(b.size()<4)throw std::runtime_error("Truncated Chord record");
    const size_t size=read32(b,0);
    if(size<40||size>b.size()-4||b.size()-4-size<8)throw std::runtime_error("Invalid Chord stride");
    ChordEvent e;e.name.clear();
    for(size_t i=0;i<16;++i){const auto c=static_cast<wchar_t>(b[4+2*i]|(b[5+2*i]<<8));if(!c)break;e.name.push_back(c);}
    e.time=static_cast<std::int32_t>(read32(b,36));e.measure=static_cast<std::uint16_t>(b[40]|(b[41]<<8));e.beat=b[42];e.flags=b[43];
    const size_t count=read32(b,4+size),stride=read32(b,8+size),start=12+size;
    if(stride<20||count>(b.size()-start)/stride||count*stride!=b.size()-start)throw std::runtime_error("Invalid SubChord array");
    e.subchords.clear();for(size_t i=0;i<count;++i){const size_t p=start+i*stride;e.subchords.push_back({read32(b,p),read32(b,p+4),read32(b,p+8),read32(b,p+12),b[p+16],b[p+17]});}
    if(e.time<0)throw std::runtime_error("Negative Chord time");return e;
}
std::vector<Record> records(const Chunk& cord) {
    if(cord.id!="LIST"||cord.type!="cord")throw std::runtime_error("Expected Chord list");
    const auto header=cord.find("crdh");
    if(!header||header->data.size()<4||std::count_if(cord.children.begin(),cord.children.end(),[](const Chunk& c){return c.id=="crdh";})!=1)throw std::runtime_error("Chord header missing or ambiguous");
    std::vector<Record> r;for(const auto& c:cord.children)if(c.id=="crdb")r.push_back({decode(c.data),c});
    std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.event.time<b.event.time;});return r;
}
Chunk encode(const ChordEvent& e,const Chunk* previous) {
    Chunk c;c.id="crdb";size_t size=40,stride=20;
    if(previous){c=*previous;size=read32(c.data,0);stride=read32(c.data,8+size);}
    Bytes b(12+size+stride*e.subchords.size());put32(b,0,static_cast<std::uint32_t>(size));
    if(previous){std::copy_n(c.data.begin()+4,size,b.begin()+4);const size_t oldCount=read32(c.data,4+size);for(size_t i=0;i<std::min(oldCount,e.subchords.size());++i)std::copy_n(c.data.begin()+12+size+i*stride,stride,b.begin()+12+size+i*stride);}
    if(!previous||decode(previous->data).name!=e.name){std::fill(b.begin()+4,b.begin()+36,0);auto name=utf16(e.name);if(name.size()>32)name.resize(32);std::copy(name.begin(),name.end(),b.begin()+4);}
    put32(b,36,static_cast<std::uint32_t>(e.time));b[40]=static_cast<std::uint8_t>(e.measure);b[41]=static_cast<std::uint8_t>(e.measure>>8);b[42]=e.beat;b[43]=e.flags;
    put32(b,4+size,static_cast<std::uint32_t>(e.subchords.size()));put32(b,8+size,static_cast<std::uint32_t>(stride));
    for(size_t i=0;i<e.subchords.size();++i){const auto& s=e.subchords[i];const size_t p=12+size+i*stride;put32(b,p,s.chordPattern);put32(b,p+4,s.scalePattern);put32(b,p+8,s.inversionPoints);put32(b,p+12,s.levels);b[p+16]=s.chordRoot;b[p+17]=s.scaleRoot;}
    c.data=std::move(b);return c;
}
void commit(Chunk& cord,const std::vector<Record>& r) {
    // Replace only body slots. Unknown children and header extensions retain
    // their order and padding; newly appended bodies follow existing children.
    size_t i=0;std::vector<Chunk> children;for(const auto& c:cord.children){if(c.id!="crdb")children.push_back(c);else if(i<r.size())children.push_back(r[i++].chunk);}while(i<r.size())children.push_back(r[i++].chunk);cord.children=std::move(children);
}
bool same_slot(const ChordEvent& a,const ChordEvent& b){return a.measure==b.measure&&a.beat==b.beat;}
}
bool valid_chord(const ChordEvent& e,const ChordEvent* previous) {
    if(e.time<0||e.name.size()>16||e.name.find(L'\0')!=std::wstring::npos||e.subchords.empty()||e.subchords.size()>8)return false;
    if((!previous||e.flags!=previous->flags)&&(e.flags&~1u))return false;
    for(size_t i=0;i<e.subchords.size();++i){const auto& s=e.subchords[i];const auto old=previous&&i<previous->subchords.size()?&previous->subchords[i]:nullptr;
        if(s.chordRoot>23&&(!old||s.chordRoot!=old->chordRoot))return false;
        if(s.scaleRoot>23&&(!old||s.scaleRoot!=old->scaleRoot))return false;
        if((s.chordPattern&0xff000000u)&&(!old||s.chordPattern!=old->chordPattern))return false;
        if((s.scalePattern&0xff000000u)&&(!old||s.scalePattern!=old->scalePattern))return false;
    }return true;
}
std::vector<ChordEvent> chord_events(const Chunk& cord){std::vector<ChordEvent> e;for(const auto& r:records(cord))e.push_back(r.event);return e;}
bool set_chord_event(Chunk& cord,ChordEvent e,size_t* result){
    auto r=records(cord);const auto found=std::find_if(r.begin(),r.end(),[&](const Record& old){return same_slot(old.event,e);});const auto old=found==r.end()?nullptr:&found->event;
    if(!valid_chord(e,old))return false;auto c=encode(e,found==r.end()?nullptr:&found->chunk);if(found!=r.end()){if(c.encode()==found->chunk.encode())return false;*found={e,std::move(c)};}else r.push_back({e,std::move(c)});
    std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.event.time<b.event.time;});const auto index=std::find_if(r.begin(),r.end(),[&](const Record& v){return same_slot(v.event,e);})-r.begin();commit(cord,r);if(result)*result=static_cast<size_t>(index);return true;
}
bool change_chord_event(Chunk& cord,size_t index,ChordEvent e,size_t* result){
    auto r=records(cord);if(index>=r.size()||!valid_chord(e,&r[index].event))return false;const auto c=encode(e,&r[index].chunk);if(c.encode()==r[index].chunk.encode())return false;
    r.erase(r.begin()+index);r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& v){return same_slot(v.event,e);}),r.end());r.push_back({e,c});std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& b){return a.event.time<b.event.time;});const auto pos=std::find_if(r.begin(),r.end(),[&](const Record& v){return same_slot(v.event,e);})-r.begin();commit(cord,r);if(result)*result=static_cast<size_t>(pos);return true;
}
bool delete_chord_event(Chunk& cord,size_t index){auto r=records(cord);if(index>=r.size())return false;r.erase(r.begin()+index);commit(cord,r);return true;}
Chunk chord_track(){Chunk t;t.id="RIFF";t.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);const Bytes id={0x8b,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd};std::copy(id.begin(),id.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+28,"cord",4);Chunk cord;cord.id="LIST";cord.type="cord";Chunk scale;scale.id="crdh";scale.data=Bytes(4);put32(scale.data,0,0xab5);cord.children.push_back(scale);t.children={h,cord};return t;}
}
