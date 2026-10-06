#include "script_track.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
const Bytes trackId={0x85,0xfa,8,0x41,0x86,0x35,0xd3,0x11,0x8b,0xd7,0,0x60,8,0x93,0xb1,0xb6};
const Bytes scriptClass={0x13,0x50,0x0b,0x81,0x8d,0xe8,0xd2,0x11,0x8b,0xc1,0,0x60,8,0x93,0xb1,0xb6};
const Chunk* unique(const Chunk& root,const char* id,const char* type=""){
    const Chunk* found=nullptr;for(const auto& c:root.children)if(c.id==id&&(type[0]==0||c.type==type)){if(found)throw std::runtime_error("Ambiguous Script Track field");found=&c;}return found;
}
const Chunk& list(const Chunk& track){
    const auto h=unique(track,"trkh"),p=unique(track,"LIST","scrt");
    if(!h||h->data.size()<32||!std::equal(trackId.begin(),trackId.end(),h->data.begin())||!p||!read32(h->data,20))throw std::runtime_error("Invalid Script Track track");
    const auto entries=unique(*p,"LIST","scrl");if(!entries)throw std::runtime_error("Missing Script Track list");return *entries;
}
std::vector<size_t> slots(const Chunk& track){const auto& p=list(track);std::vector<size_t> out;for(size_t i=0;i<p.children.size();++i)if(p.children[i].id=="LIST"&&p.children[i].type=="scre")out.push_back(i);return out;}
ScriptEvent read(const Chunk& entry){
    const auto h=unique(entry,"scrh");if(!h||h->data.size()<12)throw std::runtime_error("Truncated Script Track item");
    ScriptEvent e;e.timing=read32(h->data,0);e.logical=static_cast<std::int32_t>(read32(h->data,4));e.physical=static_cast<std::int32_t>(read32(h->data,8));
    // Signed pickup positions and unrecognized flags remain owned on load.
    if(const auto n=unique(entry,"scrn"))e.routine=decode_utf16(n->data);
    if(const auto ref=unique(entry,"LIST","DMRF")){
        const auto rh=unique(*ref,"refh");if(!rh||rh->data.size()<20)throw std::runtime_error("Truncated Script Track reference");
        const auto flags=read32(rh->data,16);
        if(!(flags&2)||!std::equal(scriptClass.begin(),scriptClass.end(),rh->data.begin()))throw std::runtime_error("Script Track reference class mismatch");
        if(flags&1){const auto id=unique(*ref,"guid");if(!id||id->data.size()!=16)throw std::runtime_error("Invalid Script Track GUID");e.hasId=true;std::copy(id->data.begin(),id->data.end(),e.objectId.begin());}
        if(flags&16){const auto f=unique(*ref,"file");if(!f)throw std::runtime_error("Missing Script Track filename");e.filename=decode_utf16(f->data);}
        if(flags&4){const auto n=unique(*ref,"name");if(!n)throw std::runtime_error("Missing Script Track name");e.name=decode_utf16(n->data);}
    }return e;
}
bool text(const std::wstring& s){if(s.size()>255||s.find(L'\0')!=std::wstring::npos)return false;for(size_t i=0;i<s.size();++i){const auto c=static_cast<unsigned>(s[i]);if(c>=0xd800&&c<=0xdbff){if(++i>=s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}else if(c>=0xdc00&&c<=0xdfff)return false;}return true;}
bool valid(const ScriptEvent& e){
    if(e.logical<0||e.physical<0||(e.timing!=1&&e.timing!=2&&e.timing!=4)||!text(e.filename)||!text(e.name)||!text(e.routine)||e.routine.empty())return false;
    if(e.hasId&&!std::any_of(e.objectId.begin(),e.objectId.end(),[](auto b){return b!=0;}))return false;
    return e.hasId||!e.filename.empty()||!e.name.empty();
}
void field(Chunk& root,const char* id,Bytes b){if(auto c=root.find(id))c->data=std::move(b);else{Chunk fresh;fresh.id=id;fresh.data=std::move(b);root.children.push_back(std::move(fresh));}}
void write(Chunk& entry,const ScriptEvent& e){
    if(!entry.find("scrh"))field(entry,"scrh",Bytes(12));auto& b=entry.find("scrh")->data;put32(b,0,e.timing);put32(b,4,e.logical);put32(b,8,e.physical);
    if(!entry.find("LIST","DMRF")){Chunk c;c.id="LIST";c.type="DMRF";entry.children.push_back(c);}
    auto& ref=*entry.find("LIST","DMRF");if(!ref.find("refh"))field(ref,"refh",Bytes(20));auto& rh=ref.find("refh")->data;
    const auto& cls=scriptClass;std::copy(cls.begin(),cls.end(),rh.begin());
    put32(rh,16,(read32(rh,16)&~std::uint32_t(1|4|16|32))|2u|(e.hasId?1u:0u)|(!e.name.empty()?4u:0u)|(!e.filename.empty()?16u:0u));
    if(e.hasId)field(ref,"guid",Bytes(e.objectId.begin(),e.objectId.end()));if(!e.filename.empty())field(ref,"file",utf16(e.filename));if(!e.name.empty())field(ref,"name",utf16(e.name));
    field(entry,"scrn",utf16(e.routine));
}
Chunk& entries(Chunk& track){return *track.find("LIST","scrt")->find("LIST","scrl");}
}
Chunk script_track(){
    Chunk track;track.id="RIFF";track.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);std::copy(trackId.begin(),trackId.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+28,"scrt",4);
    Chunk p;p.id="LIST";p.type="scrt";Chunk l;l.id="LIST";l.type="scrl";p.children={l};track.children={h,p};return track;
}
std::vector<ScriptEvent> script_events(const Chunk& track){std::vector<ScriptEvent> out;const auto& p=list(track);for(const auto i:slots(track))out.push_back(read(p.children[i]));return out;}
bool add_script_event(Chunk& track,const ScriptEvent& e){if(!valid(e))return false;(void)script_events(track);auto next=track;Chunk c;c.id="LIST";c.type="scre";write(c,e);entries(next).children.push_back(std::move(c));track=std::move(next);return true;}
bool change_script_event(Chunk& track,size_t index,const ScriptEvent& e){if(!valid(e))return false;(void)script_events(track);const auto s=slots(track);if(index>=s.size())return false;auto next=track;auto& item=entries(next).children[s[index]];const auto old=read(item);const auto ref=item.find("LIST","DMRF");if(ref&&ref->find("jzfr")&&(old.hasId!=e.hasId||old.objectId!=e.objectId||old.filename!=e.filename))return false;write(item,e);if(next.encode()==track.encode())return false;track=std::move(next);return true;}
bool delete_script_event(Chunk& track,size_t index){(void)script_events(track);const auto s=slots(track);if(index>=s.size())return false;auto next=track;auto& p=entries(next);p.children.erase(p.children.begin()+s[index]);track=std::move(next);return true;}
}
