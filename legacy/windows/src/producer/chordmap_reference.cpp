#include "chordmap_reference.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <windows.h>
namespace producer::app {namespace {
const Bytes mapClass={0x8f,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
const Bytes trackClass={0x96,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
Chunk leaf(const char* id,Bytes bytes){Chunk c;c.id=id;c.data=std::move(bytes);return c;}
Chunk list(const char* type,std::vector<Chunk> children={}){Chunk c;c.id="LIST";c.type=type;c.children=std::move(children);return c;}
const Chunk* unique(const Chunk& n,const char* id,const char* type=""){
    const Chunk* result=nullptr;for(const auto& c:n.children)if(c.id==id&&(!*type||c.type==type)){if(result)throw std::runtime_error("Ambiguous ChordMap reference chunk");result=&c;}return result;
}
const Chunk& required(const Chunk& n,const char* id,const char* type=""){auto c=unique(n,id,type);if(!c)throw std::runtime_error("Missing ChordMap reference chunk");return *c;}
Chunk& required(Chunk& n,const char* id,const char* type=""){return const_cast<Chunk&>(required(static_cast<const Chunk&>(n),id,type));}
bool same_path(const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;}
bool valid_edit(const ChordMapReference& r){return r.time>=0&&r.groups&&(r.hasId||!r.filename.empty())&&r.name.find(L'\0')==std::wstring::npos&&r.filename.find(L'\0')==std::wstring::npos&&r.filename.size()<260;}
std::vector<size_t> slots(const Chunk& n){std::vector<size_t> result;const auto& refs=required(n,"LIST","pftr");for(size_t i=0;i<refs.children.size();++i)if(refs.children[i].id=="LIST"&&refs.children[i].type=="pfrf")result.push_back(i);return result;}
void write_reference(Chunk& ref,const ChordMapReference& r){
    auto stamp=ref.find("stmp");if(!stamp){ref.children.push_back(leaf("stmp",Bytes(4)));stamp=&ref.children.back();}put32(stamp->data,0,r.time);
    auto desc=ref.find("LIST","DMRF");if(!desc){ref.children.push_back(list("DMRF"));desc=&ref.children.back();}
    auto h=desc->find("refh");if(!h){Bytes header(20);std::copy(mapClass.begin(),mapClass.end(),header.begin());put32(header,16,2);desc->children.push_back(leaf("refh",header));h=&desc->children.back();}
    put32(h->data,16,(read32(h->data,16)&~(1u|4u|16u))|(r.hasId?1u:0u)|(r.name.empty()?0u:4u)|(r.filename.empty()?0u:16u)|2u);
    const auto field=[&](const char* id,const Bytes& bytes){auto c=desc->find(id);if(!c){desc->children.push_back(leaf(id,bytes));}else c->data=bytes;};
    if(r.hasId)field("guid",Bytes(r.objectId.begin(),r.objectId.end()));if(!r.name.empty())field("name",utf16(r.name));if(!r.filename.empty())field("file",utf16(r.filename));
}
std::vector<Chunk*> descriptors(Chunk& n){std::vector<Chunk*> result;auto tracks=n.find("LIST","trkl");if(!tracks)return result;for(auto& t:tracks->children)if(t.id=="RIFF"&&t.type=="DMTK"){auto h=t.find("trkh");if(!h||h->data.size()<32||!std::equal(trackClass.begin(),trackClass.end(),h->data.begin()))continue;for(auto& r:required(t,"LIST","pftr").children)if(r.id=="LIST"&&r.type=="pfrf")result.push_back(&required(r,"LIST","DMRF"));}return result;}
bool same_reference(const ChordMapReference& a,const ChordMapReference& b){return a.time==b.time&&a.groups==b.groups&&a.hasId==b.hasId&&a.objectId==b.objectId&&a.filename==b.filename&&a.name==b.name;}
}
Chunk chordmap_reference_track(std::uint32_t groups){
    if(!groups)throw std::runtime_error("ChordMap track needs groups");Bytes h(32);std::copy(trackClass.begin(),trackClass.end(),h.begin());put32(h,20,groups);put32(h,28,0x72746670);Bytes flags(8);put32(flags,0,0x38);Chunk track;track.id="RIFF";track.type="DMTK";track.children={leaf("trkh",h),leaf("trkx",flags),list("pftr")};return track;
}
std::vector<ChordMapReference> chordmap_track_references(const Chunk& track){
    const auto& header=required(track,"trkh");if(header.data.size()<32||!std::equal(trackClass.begin(),trackClass.end(),header.data.begin())||!read32(header.data,20)||read32(header.data,24)||read32(header.data,28)!=0x72746670)throw std::runtime_error("Invalid ChordMap track header");
    std::vector<ChordMapReference> result;for(const auto& ref:required(track,"LIST","pftr").children)if(ref.id=="LIST"&&ref.type=="pfrf"){
        const auto& stamp=required(ref,"stmp");if(stamp.data.size()!=4||read32(stamp.data,0)>INT32_MAX)throw std::runtime_error("Invalid ChordMap reference clock");const auto& desc=required(ref,"LIST","DMRF");const auto& h=required(desc,"refh");if(h.data.size()<20||!std::equal(mapClass.begin(),mapClass.end(),h.data.begin()))throw std::runtime_error("Invalid ChordMap reference class");
        ChordMapReference r;r.time=read32(stamp.data,0);r.groups=read32(header.data,20);const auto flags=read32(h.data,16);
        if(flags&1){const auto& id=required(desc,"guid");if(id.data.size()!=16)throw std::runtime_error("Invalid ChordMap reference GUID");r.hasId=true;std::copy(id.data.begin(),id.data.end(),r.objectId.begin());}
        if(flags&16)r.filename=decode_utf16(required(desc,"file").data);if(flags&4)r.name=decode_utf16(required(desc,"name").data);
        if(!r.hasId&&r.filename.empty())throw std::runtime_error("ChordMap reference lacks identity or filename");if(result.size()>=1000)throw std::runtime_error("ChordMap reference limit");result.push_back(std::move(r));
    }return result;
}
std::vector<ChordMapReference> chordmap_references(const Chunk& segment){
    std::vector<ChordMapReference> result;const auto tracks=unique(segment,"LIST","trkl");if(!tracks)return result;for(const auto& t:tracks->children)if(t.id=="RIFF"&&t.type=="DMTK"){
        const auto h=unique(t,"trkh");const bool cls=h&&h->data.size()>=16&&std::equal(trackClass.begin(),trackClass.end(),h->data.begin());if(cls||unique(t,"LIST","pftr")){auto refs=chordmap_track_references(t);if(refs.size()>1000-result.size())throw std::runtime_error("ChordMap reference limit");result.insert(result.end(),refs.begin(),refs.end());}
    }return result;
}
bool set_chordmap_reference(Chunk& track,const ChordMapReference& r){
    if(!valid_edit(r))return false;const auto refs=chordmap_track_references(track);if(refs.size()>1)throw std::runtime_error("Imported multiple ChordMap references require an explicit event editor");if(r.groups!=read32(required(track,"trkh").data,20))return false;
    auto next=track;auto& list=required(next,"LIST","pftr");if(refs.empty()){Chunk ref;ref.id="LIST";ref.type="pfrf";write_reference(ref,r);list.children.push_back(std::move(ref));}else write_reference(list.children.at(slots(next).at(0)),r);(void)chordmap_track_references(next);if(next.encode()==track.encode())return false;track=std::move(next);return true;
}
bool clear_chordmap_reference(Chunk& track){const auto refs=chordmap_track_references(track);if(refs.empty())return false;if(refs.size()>1)throw std::runtime_error("Imported multiple ChordMap references require an explicit event editor");auto next=track;auto& children=required(next,"LIST","pftr").children;children.erase(children.begin()+slots(next).at(0));track=std::move(next);return true;}
std::vector<ResolvedChordMap> resolve_chordmaps(const std::vector<ChordMapReference>& refs,const std::wstring& directory,const std::vector<ChordMapCatalogEntry>& catalog){
    std::vector<ResolvedChordMap> result;const auto base=std::filesystem::absolute(directory.empty()?L".":directory).lexically_normal();for(const auto& r:refs){const ChordMapCatalogEntry* owned=nullptr;Bytes bytes;std::filesystem::path path;
        if(r.hasId)for(const auto& c:catalog){ChordMapDocument d;d.load(c.bytes);if(d.has_object_id()&&d.object_id()==r.objectId){if(owned)throw std::runtime_error("Ambiguous owned ChordMap GUID");owned=&c;}}
        if(owned){bytes=owned->bytes;path=owned->path;}else if(!r.filename.empty()){
            const auto relative=std::filesystem::path(r.filename);if(relative.is_absolute()||relative.has_root_name())throw std::runtime_error("ChordMap filename must be relative");path=(base/relative).lexically_normal();const auto relation=path.lexically_relative(base);if(relation.empty()||*relation.begin()==L"..")throw std::runtime_error("ChordMap filename escapes Segment directory");
            for(const auto& c:catalog)if(!c.path.empty()&&same_path(c.path,path.wstring())){if(owned)throw std::runtime_error("Ambiguous owned ChordMap path");owned=&c;}if(owned)bytes=owned->bytes;else{const auto physical=std::filesystem::canonical(path).lexically_relative(std::filesystem::canonical(base));if(physical.empty()||*physical.begin()==L"..")throw std::runtime_error("ChordMap file resolves outside Segment directory");bytes=read_file(path.wstring());}
        }else throw std::runtime_error("ChordMap GUID is not in owned catalog");
        ChordMapDocument d;d.load(bytes);if(r.hasId&&(!d.has_object_id()||d.object_id()!=r.objectId))throw std::runtime_error("ChordMap dependency identity mismatch");result.push_back({r,path.wstring(),bytes});
    }return result;
}
ChordMapPlayback prepare_chordmap_playback(const Bytes& bytes,const std::vector<ResolvedChordMap>& maps){
    auto root=Chunk::parse(bytes);if(root.type!="DMSG")throw std::runtime_error("ChordMap playback requires Segment");const auto refs=chordmap_references(root);if(refs.size()!=maps.size())throw std::runtime_error("ChordMap playback needs resolved snapshots");auto descs=descriptors(root);ChordMapPlayback result{bytes,maps};
    for(size_t i=0;i<refs.size();++i){if(!same_reference(refs[i],maps[i].reference))throw std::runtime_error("ChordMap playback context mismatch");ChordMapDocument d;d.load(maps[i].bytes);if(!d.has_object_id())throw std::runtime_error("ChordMap runtime snapshot needs identity");auto id=d.object_id();if(refs[i].hasId&&refs[i].objectId!=id)throw std::runtime_error("ChordMap snapshot identity mismatch");for(size_t j=0;j<i;++j)if(id==result.maps[j].reference.objectId&&maps[i].bytes!=maps[j].bytes)throw std::runtime_error("Conflicting ChordMap snapshots share GUID");
        result.maps[i].reference.hasId=true;result.maps[i].reference.objectId=id;result.maps[i].reference.filename.clear();auto& desc=*descs.at(i);put32(required(desc,"refh").data,16,3);auto guid=desc.find("guid");if(!guid){desc.children.push_back(leaf("guid",Bytes(id.begin(),id.end())));}else guid->data.assign(id.begin(),id.end());
    }result.segment=root.encode();return result;
}
}
