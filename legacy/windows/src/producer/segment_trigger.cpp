#include "segment_trigger.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
const Bytes trackId={0x65,0xd6,0xe4,0xba,0xa1,0x4e,0xd3,0x11,0x8b,0xda,0,0x60,8,0x93,0xb1,0xb6};
const Bytes segmentClass={0x82,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
const Bytes styleClass={0x8a,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
const Chunk* unique(const Chunk& root,const char* id,const char* type=""){
    const Chunk* found=nullptr;for(const auto& c:root.children)if(c.id==id&&(type[0]==0||c.type==type)){if(found)throw std::runtime_error("Ambiguous Segment Trigger field");found=&c;}return found;
}
const Chunk& list(const Chunk& track){
    const auto h=unique(track,"trkh"),p=unique(track,"LIST","segt");
    if(!h||h->data.size()<32||!std::equal(trackId.begin(),trackId.end(),h->data.begin())||!p||!read32(h->data,20))throw std::runtime_error("Invalid Segment Trigger track");
    const auto header=unique(*p,"sgth");if(header&&header->data.size()<4)throw std::runtime_error("Truncated Segment Trigger header");
    const auto entries=unique(*p,"LIST","lsgl");if(!entries)throw std::runtime_error("Missing Segment Trigger list");return *entries;
}
std::vector<size_t> slots(const Chunk& track){const auto& p=list(track);std::vector<size_t> out;for(size_t i=0;i<p.children.size();++i)if(p.children[i].id=="LIST"&&p.children[i].type=="lseg")out.push_back(i);return out;}
SegmentTrigger read(const Chunk& entry){
    const auto h=unique(entry,"sgih");if(!h||h->data.size()<16)throw std::runtime_error("Truncated Segment Trigger item");
    SegmentTrigger e;e.logical=static_cast<std::int32_t>(read32(h->data,0));e.physical=static_cast<std::int32_t>(read32(h->data,4));e.playFlags=read32(h->data,8);e.itemFlags=read32(h->data,12);
    // Signed pickup positions and unrecognized flags remain owned on load.
    if(const auto n=unique(entry,"snam"))e.motif=decode_utf16(n->data);
    if(const auto ref=unique(entry,"LIST","DMRF")){
        const auto rh=unique(*ref,"refh");if(!rh||rh->data.size()<20)throw std::runtime_error("Truncated Segment Trigger reference");
        const auto flags=read32(rh->data,16);
        if(!(flags&2)||!std::equal((e.itemFlags&1?styleClass:segmentClass).begin(),(e.itemFlags&1?styleClass:segmentClass).end(),rh->data.begin()))throw std::runtime_error("Segment Trigger reference class mismatch");
        if(flags&1){const auto id=unique(*ref,"guid");if(!id||id->data.size()!=16)throw std::runtime_error("Invalid Segment Trigger GUID");e.hasId=true;std::copy(id->data.begin(),id->data.end(),e.objectId.begin());}
        if(flags&16){const auto f=unique(*ref,"file");if(!f)throw std::runtime_error("Missing Segment Trigger filename");e.filename=decode_utf16(f->data);}
        if(flags&4){const auto n=unique(*ref,"name");if(!n)throw std::runtime_error("Missing Segment Trigger name");e.name=decode_utf16(n->data);}
    }return e;
}
bool text(const std::wstring& s){if(s.size()>255||s.find(L'\0')!=std::wstring::npos)return false;for(size_t i=0;i<s.size();++i){const auto c=static_cast<unsigned>(s[i]);if(c>=0xd800&&c<=0xdbff){if(++i>=s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}else if(c>=0xdc00&&c<=0xdfff)return false;}return true;}
bool valid(const SegmentTrigger& e){
    if(e.logical<0||e.physical<0||(e.itemFlags&~1u)||(e.playFlags&~0x0fffff80u)||!text(e.filename)||!text(e.name)||!text(e.motif))return false;
    if((e.playFlags&0x200)&&!(e.playFlags&0x80))return false; // CONTROL requires SECONDARY.
    if((e.playFlags&0x100)&&(e.playFlags&0x80))return false; // QUEUE is primary only.
    if((e.playFlags&0x080e0000)&&!(e.playFlags&0x10000))return false; // valid-start overrides require ALIGN.
    if(e.hasId&&!std::any_of(e.objectId.begin(),e.objectId.end(),[](auto b){return b!=0;}))return false;
    if((e.itemFlags&1)?e.motif.empty():!e.motif.empty())return false;
    return e.hasId||!e.filename.empty()||!e.name.empty(); // author only resolved selections
}
void field(Chunk& root,const char* id,Bytes b){if(auto c=root.find(id))c->data=std::move(b);else{Chunk fresh;fresh.id=id;fresh.data=std::move(b);root.children.push_back(std::move(fresh));}}
void write(Chunk& entry,const SegmentTrigger& e){
    if(!entry.find("sgih"))field(entry,"sgih",Bytes(16));auto& b=entry.find("sgih")->data;put32(b,0,e.logical);put32(b,4,e.physical);put32(b,8,e.playFlags);put32(b,12,e.itemFlags);
    if(!entry.find("LIST","DMRF")){Chunk c;c.id="LIST";c.type="DMRF";entry.children.push_back(c);}
    auto& ref=*entry.find("LIST","DMRF");if(!ref.find("refh"))field(ref,"refh",Bytes(20));auto& rh=ref.find("refh")->data;
    const auto& cls=e.itemFlags&1?styleClass:segmentClass;std::copy(cls.begin(),cls.end(),rh.begin());
    put32(rh,16,(read32(rh,16)&~std::uint32_t(1|4|16|32))|2u|(e.hasId?1u:0u)|(!e.name.empty()?4u:0u)|(!e.filename.empty()?16u:0u));
    if(e.hasId)field(ref,"guid",Bytes(e.objectId.begin(),e.objectId.end()));if(!e.filename.empty())field(ref,"file",utf16(e.filename));if(!e.name.empty())field(ref,"name",utf16(e.name));
    if(e.itemFlags&1)field(entry,"snam",utf16(e.motif));
}
Chunk& entries(Chunk& track){return *track.find("LIST","segt")->find("LIST","lsgl");}
}
Chunk segment_trigger_track(){
    Chunk track;track.id="RIFF";track.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);std::copy(trackId.begin(),trackId.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+28,"segt",4);
    Chunk p;p.id="LIST";p.type="segt";Chunk a;a.id="sgth";a.data=Bytes(4);Chunk l;l.id="LIST";l.type="lsgl";p.children={a,l};track.children={h,p};return track;
}
std::vector<SegmentTrigger> segment_triggers(const Chunk& track){std::vector<SegmentTrigger> out;const auto& p=list(track);for(const auto i:slots(track))out.push_back(read(p.children[i]));return out;}
bool add_segment_trigger(Chunk& track,const SegmentTrigger& e){if(!valid(e))return false;(void)segment_triggers(track);auto next=track;Chunk c;c.id="LIST";c.type="lseg";write(c,e);entries(next).children.push_back(std::move(c));track=std::move(next);return true;}
bool change_segment_trigger(Chunk& track,size_t index,const SegmentTrigger& e){if(!valid(e))return false;(void)segment_triggers(track);const auto s=slots(track);if(index>=s.size())return false;auto next=track;auto& item=entries(next).children[s[index]];const auto old=read(item);const auto ref=item.find("LIST","DMRF");if(ref&&ref->find("jzfr")&&((old.itemFlags&1)!=(e.itemFlags&1)||old.hasId!=e.hasId||old.objectId!=e.objectId||old.filename!=e.filename))return false;write(item,e);if(next.encode()==track.encode())return false;track=std::move(next);return true;}
bool delete_segment_trigger(Chunk& track,size_t index){(void)segment_triggers(track);const auto s=slots(track);if(index>=s.size())return false;auto next=track;auto& p=entries(next);p.children.erase(p.children.begin()+s[index]);track=std::move(next);return true;}
}
