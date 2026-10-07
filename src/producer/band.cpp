#include "band.h"
#include "dls.h"
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <objbase.h>
#include <filesystem>
#include <functional>

namespace producer::app {
namespace {
const GUID bandTrackId={0xd2ac2894,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
const GUID collectionClass={0x480ff4b0,0x28b2,0x11d1,{0xbe,0xf7,0,0xc0,0x4f,0xbf,0x8f,0xef}};
const Chunk* unique_band(const Chunk& root,const std::string& id,const std::string& type="") {
    const Chunk* found=nullptr;for(const auto& c:root.children)if(c.id==id&&(type.empty()||c.type==type)){if(found)throw std::runtime_error("Ambiguous Band chunk");found=&c;}return found;
}
std::vector<const Chunk*> instrument_lists(const Chunk& root){
    std::vector<const Chunk*> result;const auto list=unique_band(root,"LIST","lbil");if(!list)return result;
    for(const auto& child:list->children)if(child.id=="LIST"&&child.type=="lbin")result.push_back(&child);return result;
}
}
CollectionReference collection_reference(const Bytes& bytes){
    const auto descriptor=Chunk::parse_list(bytes);if(descriptor.type!="DMRF")throw std::runtime_error("Expected collection reference");
    const auto header=unique_band(descriptor,"refh");if(!header||header->data.size()<20||std::memcmp(header->data.data(),&collectionClass,16))throw std::runtime_error("Collection reference class missing or invalid");
    const auto valid=read32(header->data,16);if(!(valid&2)||valid&(32u|64u|1024u|2048u))throw std::runtime_error("Collection reference must use a project identity or relative filename");
    CollectionReference result;if(valid&1){const auto id=unique_band(descriptor,"guid");if(!id||id->data.size()!=16)throw std::runtime_error("Collection GUID missing");std::array<std::uint8_t,16> value{};std::copy(id->data.begin(),id->data.end(),value.begin());result.objectId=value;}
    if(valid&16){const auto file=unique_band(descriptor,"file");if(!file||file->data.size()<2||file->data.size()>520||file->data.size()%2||file->data[file->data.size()-1]||file->data[file->data.size()-2])throw std::runtime_error("Collection filename encoding");result.filename=decode_utf16(file->data);if(result.filename.empty()||result.filename.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Collection filename empty or contains NUL");}
    if(!result.objectId&&result.filename.empty())throw std::runtime_error("Collection reference identity missing");return result;
}
std::optional<std::array<std::uint8_t,16>> collection_identity(const Bytes& bytes){
    const auto root=Chunk::parse(bytes);if(root.id!="RIFF"||root.type!="DLS ")throw std::runtime_error("Expected DLS collection");
    const auto id=unique_band(root,"dlid");if(!id)return {};if(id->data.size()!=16)throw std::runtime_error("DLS identity size");std::array<std::uint8_t,16> result{};std::copy(id->data.begin(),id->data.end(),result.begin());return result;
}
std::vector<ResolvedCollection> resolve_collections(const std::vector<CollectionReference>& references,const std::wstring& directory,const std::vector<CollectionEntry>& catalog,bool runtimeReferences){
    std::vector<ResolvedCollection> result;for(const auto& ref:references){const CollectionEntry* owned=nullptr;std::filesystem::path path;Bytes bytes;
        if(ref.filename.empty()){if(!ref.objectId)throw std::runtime_error("Collection identity missing");for(const auto& entry:catalog){const auto id=collection_identity(entry.bytes);if(id&&*id==*ref.objectId){if(owned)throw std::runtime_error("Ambiguous owned collection GUID");owned=&entry;}}if(!owned)throw std::runtime_error("GUID-only collection requires an owned project entry");path=owned->path;bytes=owned->bytes;}
        else{if(directory.empty())throw std::runtime_error("Collection reference needs document directory");const auto base=std::filesystem::absolute(directory).lexically_normal(),relative=std::filesystem::path(ref.filename);if(relative.is_absolute()||relative.has_root_name())throw std::runtime_error("Collection filename must be relative");path=(base/relative).lexically_normal();const auto relation=path.lexically_relative(base);if(!runtimeReferences&&(relation.empty()||*relation.begin()==L".."))throw std::runtime_error("Collection reference escapes directory");
            for(const auto& entry:catalog)if(CompareStringOrdinal(entry.path.c_str(),-1,path.wstring().c_str(),-1,TRUE)==CSTR_EQUAL){if(owned)throw std::runtime_error("Ambiguous owned collection path");owned=&entry;}
            if(owned)bytes=owned->bytes;else{const auto canonical=std::filesystem::canonical(path),realBase=std::filesystem::canonical(base),contained=canonical.lexically_relative(realBase);if(!runtimeReferences&&(contained.empty()||contained.is_absolute()||*contained.begin()==L".."))throw std::runtime_error("Collection target escapes directory");bytes=read_file(canonical.wstring());}}
        const auto id=collection_identity(bytes);if(ref.objectId&&(!id||*id!=*ref.objectId))throw std::runtime_error("Collection identity mismatch");result.push_back({ref,path.wstring(),std::move(bytes)});
    }return result;
}
std::vector<CollectionReference> BandDocument::collection_references() const {std::vector<CollectionReference> result;for(const auto& i:instruments())if(!i.collectionReference.empty())result.push_back(collection_reference(i.collectionReference));return result;}
std::vector<CollectionReference> document_collection_references(const Bytes& bytes){const auto root=Chunk::parse(bytes);std::vector<CollectionReference> result;std::function<void(const Chunk&)> visit=[&](const Chunk& c){if(c.id=="RIFF"&&c.type=="DMBD"){BandDocument band;band.load(c.encode());const auto refs=band.collection_references();result.insert(result.end(),refs.begin(),refs.end());return;}for(const auto& child:c.children)visit(child);};visit(root);return result;}
CollectionPlaybackSnapshot prepare_collection_playback(const std::vector<Bytes>& documents,const std::vector<ResolvedCollection>& dependencies){
    CollectionPlaybackSnapshot result{documents,dependencies};std::vector<Chunk> roots;roots.reserve(documents.size());for(const auto& b:documents)roots.push_back(Chunk::parse(b));std::vector<Chunk*> descriptors;
    std::function<void(Chunk&)> visit=[&](Chunk& c){if(c.id=="RIFF"&&c.type=="DMBD"){BandDocument check;check.load(c.encode());(void)check.collection_references();if(auto instruments=c.find("LIST","lbil"))for(auto& item:instruments->children)if(item.id=="LIST"&&item.type=="lbin")if(auto ref=item.find("LIST","DMRF"))descriptors.push_back(ref);return;}for(auto& child:c.children)visit(child);};for(auto& root:roots)visit(root);
    if(descriptors.size()!=dependencies.size())throw std::runtime_error("Unresolved playback collection dependencies");
    // Complete preflight before runtime identity mapping or Conductor::stop.
    for(const auto& dependency:dependencies){DlsDocument check;check.load(dependency.bytes);check.validate_playback_samples();}
    const auto samePath=[](const std::wstring& a,const std::wstring& b){return !a.empty()&&!b.empty()&&CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;};
    for(size_t i=0;i<dependencies.size();++i){const auto ref=collection_reference(descriptors[i]->encode());const auto& supplied=dependencies[i];if(ref.filename!=supplied.reference.filename||ref.objectId!=supplied.reference.objectId)throw std::runtime_error("Playback collection context mismatch");const auto identity=collection_identity(supplied.bytes);if(ref.objectId&&(!identity||ref.objectId!=identity))throw std::runtime_error("Playback collection identity mismatch");auto& mapped=result.collections[i];bool reused=false;
        for(size_t j=0;j<i;++j)if(samePath(supplied.path,dependencies[j].path)){if(supplied.bytes!=dependencies[j].bytes)throw std::runtime_error("Conflicting collection snapshots share a path");mapped.bytes=result.collections[j].bytes;mapped.reference.objectId=result.collections[j].reference.objectId;reused=true;break;}
        if(!reused){DlsDocument runtimeSamples;runtimeSamples.load(mapped.bytes);mapped.bytes=runtimeSamples.playback_sample_bytes();mapped.reference.objectId=identity;if(!identity){GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Collection playback GUID creation failed");std::array<std::uint8_t,16> value{};std::memcpy(value.data(),&id,16);mapped.reference.objectId=value;auto root=Chunk::parse(mapped.bytes);Chunk chunk;chunk.id="dlid";chunk.data.assign(value.begin(),value.end());root.children.push_back(chunk);mapped.bytes=root.encode();}}
        for(size_t j=0;j<i;++j)if(mapped.reference.objectId==result.collections[j].reference.objectId&&(mapped.bytes!=result.collections[j].bytes||supplied.bytes!=dependencies[j].bytes))throw std::runtime_error("Conflicting collection snapshots share a runtime GUID");mapped.reference.filename.clear();
        auto& descriptor=*descriptors[i];auto header=descriptor.find("refh");put32(header->data,16,(read32(header->data,16)|3u)&~(16u|32u));auto guid=descriptor.find("guid");if(!guid){Chunk c;c.id="guid";descriptor.children.push_back(c);guid=&descriptor.children.back();}guid->data.assign(mapped.reference.objectId->begin(),mapped.reference.objectId->end());
    }
    for(size_t i=0;i<roots.size();++i)result.documents[i]=roots[i].encode();return result;
}
Bytes relocate_collection_references(const Bytes& bytes,const std::vector<ResolvedCollection>& dependencies,const std::wstring& directory){
    auto root=Chunk::parse(bytes);size_t at=0;
    std::function<void(Chunk&)> visit=[&](Chunk& c){
        if(c.id=="RIFF"&&c.type=="DMBD"){
            BandDocument check;check.load(c.encode());(void)check.collection_references();
            if(auto list=c.find("LIST","lbil"))for(auto& item:list->children)if(item.id=="LIST"&&item.type=="lbin")if(auto descriptor=item.find("LIST","DMRF")){
                if(at>=dependencies.size())throw std::runtime_error("Missing relocation dependency");
                const auto ref=collection_reference(descriptor->encode());const auto& target=dependencies[at++];
                if(ref.filename!=target.reference.filename||ref.objectId!=target.reference.objectId)throw std::runtime_error("Relocation dependency context mismatch");
                if(ref.objectId&&ref.objectId!=collection_identity(target.bytes))throw std::runtime_error("Relocation dependency identity mismatch");
                if(ref.filename.empty())continue;
                if(directory.empty()||target.path.empty())throw std::runtime_error("Relocation needs saved document and collection paths");
                const auto relative=std::filesystem::path(target.path).lexically_relative(std::filesystem::path(directory));
                const bool outside=relative.empty()||relative.is_absolute()||relative.has_root_name()||*relative.begin()==L"..";
                if(outside){
                    if(!ref.objectId)throw std::runtime_error("Filename-only collection must remain inside document directory");
                    auto header=descriptor->find("refh");put32(header->data,16,read32(header->data,16)&~16u);
                    // Inactive file payload remains opaque, just as in runtime mapping.
                }else{
                    const auto filename=relative.wstring();if(filename.size()>259)throw std::runtime_error("Relocated collection filename too long");
                    if(filename!=ref.filename)descriptor->find("file")->data=utf16(filename);
                }
            }
            return;
        }
        for(auto& child:c.children)visit(child);
    };visit(root);if(at!=dependencies.size())throw std::runtime_error("Extra relocation dependencies");return root.encode();
}
bool BandDocument::relocate_collections(const std::vector<ResolvedCollection>& dependencies,const std::wstring& directory){
    const auto before=save_bytes(),after=relocate_collection_references(before,dependencies,directory);if(after==before)return false;
    auto root=Chunk::parse(after);BandDocument check;check.load(after);(void)check.collection_references();
    undo_.push_back(before);if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(root);return true;
}
bool BandDocument::set_collection_reference(size_t index,const CollectionReference& value){
    if(!value.objectId&&value.filename.empty())return false;if(value.filename.size()>259||value.filename.find(L'\0')!=std::wstring::npos)return false;
    if(!value.filename.empty()){const auto path=std::filesystem::path(value.filename);if(path.is_absolute()||path.has_root_name()||path.lexically_normal().empty()||*path.lexically_normal().begin()==L"..")return false;}
    const auto parsed=instruments();if(index>=parsed.size())return false;auto next=root_;auto list=next.find("LIST","lbil");size_t at=0;
    for(auto& item:list->children)if(item.id=="LIST"&&item.type=="lbin"&&at++==index){auto ref=item.find("LIST","DMRF");if(!ref){Chunk c;c.id="LIST";c.type="DMRF";item.children.push_back(c);ref=&item.children.back();}else (void)collection_reference(ref->encode());
        auto header=ref->find("refh");if(!header){Chunk c;c.id="refh";c.data=Bytes(20);std::memcpy(c.data.data(),&collectionClass,16);ref->children.push_back(c);header=&ref->children.back();}const auto valid=(read32(header->data,16)&~(1u|16u|32u|64u|1024u|2048u))|2u|(value.objectId?1u:0u)|(!value.filename.empty()?16u:0u);put32(header->data,16,valid);
        for(const auto& name:{std::string("guid"),std::string("file")}){auto child=ref->find(name);const bool present=name=="guid"?value.objectId.has_value():!value.filename.empty();if(!present){ref->children.erase(std::remove_if(ref->children.begin(),ref->children.end(),[&](const Chunk& c){return c.id==name;}),ref->children.end());continue;}if(!child){Chunk c;c.id=name;ref->children.push_back(c);child=&ref->children.back();}child->data=name=="guid"?Bytes(value.objectId->begin(),value.objectId->end()):utf16(value.filename);}
        auto& flags=item.find("bins")->data;put32(flags,28,read32(flags,28)&~(0x100u|0x200u|0x400u|0x1000u));break;
    }
    if(next.encode()==save_bytes())return false;undo_.push_back(save_bytes());if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;
}
bool BandDocument::set_dls_instrument(size_t index,const CollectionReference& reference,std::uint32_t bank,std::uint32_t program){
    if((bank&~0x80007f7fu)||program>127)return false;
    const auto parsed=instruments();if(index>=parsed.size())return false;
    auto next=*this;const auto& old=parsed[index];const auto patch=(bank&0x80000000u)|((bank&0x7f7fu)<<8)|program;
    (void)next.set_instrument(index,patch,old.pchannel,old.pan,old.volume);
    const auto changed=next.set_collection_reference(index,reference);
    if(!changed){const auto refs=next.instruments();if(refs[index].collectionReference.empty())return false;const auto actual=collection_reference(refs[index].collectionReference);if(actual.filename!=reference.filename||actual.objectId!=reference.objectId)return false;}
    if(next.save_bytes()==save_bytes())return false;
    undo_.push_back(save_bytes());if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next.root_);return true;
}
BandDocument::BandDocument(){
    root_.id="RIFF";root_.type="DMBD";
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Cannot create Band identity");
    Chunk id;id.id="guid";const auto bytes=reinterpret_cast<const unsigned char*>(&identity);id.data.assign(bytes,bytes+sizeof(identity));root_.children.push_back(id);
    Chunk list;list.id="LIST";list.type="lbil";root_.children.push_back(list);saved_=save_bytes();
}
void BandDocument::load(const Bytes& bytes){auto next=Chunk::parse(bytes);if(next.id!="RIFF"||next.type!="DMBD")throw std::runtime_error("Expected DMBD Band");BandDocument check;check.root_=next;(void)check.instruments();root_=std::move(next);saved_=bytes;undo_.clear();redo_.clear();}
std::vector<BandInstrument> BandDocument::instruments() const {
    std::vector<BandInstrument> result;for(const auto item:instrument_lists(root_)){
        const auto header=unique_band(*item,"bins");if(!header||header->data.size()<34)throw std::runtime_error("Band instrument truncated");const auto& b=header->data;
        BandInstrument instrument{read32(b,0),read32(b,24),read32(b,28),b[32],b[33],{}};
        for(const auto& previous:result)if(previous.pchannel==instrument.pchannel)throw std::runtime_error("Ambiguous Band PChannel");
        if(const auto ref=unique_band(*item,"LIST","DMRF"))instrument.collectionReference=ref->encode();result.push_back(std::move(instrument));
    }return result;
}
bool BandDocument::set_instrument(size_t index,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){
    if((patch&~0x807f7f7fu)||pan>127||volume>127)return false;
    const auto parsed=instruments();if(index>=parsed.size())return false;for(size_t i=0;i<parsed.size();++i)if(i!=index&&parsed[i].pchannel==pchannel)return false;
    const auto& old=parsed[index];const auto flags=old.flags|0x63u; // PATCH, BANKSELECT, PAN, VOLUME.
    if(old.patch==patch&&old.pchannel==pchannel&&old.pan==pan&&old.volume==volume&&old.flags==flags)return false;
    auto next=root_;auto list=next.find("LIST","lbil");size_t at=0;for(auto& child:list->children)if(child.id=="LIST"&&child.type=="lbin"&&at++==index){auto& b=child.find("bins")->data;put32(b,0,patch);put32(b,24,pchannel);put32(b,28,flags);b[32]=static_cast<std::uint8_t>(pan);b[33]=static_cast<std::uint8_t>(volume);break;}
    undo_.push_back(save_bytes());if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;
}
void BandDocument::save(const std::wstring& path){const auto bytes=save_bytes();write_file_atomic(path,bytes);saved_=bytes;}
bool BandDocument::add_gm_instrument(std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){
    if((patch&~0x807f7f7fu)||pan>127||volume>127)return false;for(const auto& i:instruments())if(i.pchannel==pchannel)return false;
    auto next=root_;auto list=next.find("LIST","lbil");if(!list){Chunk c;c.id="LIST";c.type="lbil";next.children.push_back(c);list=&next.children.back();}
    Chunk item;item.id="LIST";item.type="lbin";Chunk header;header.id="bins";header.data=Bytes(44);put32(header.data,0,patch);put32(header.data,24,pchannel);put32(header.data,28,0x1163u);header.data[32]=static_cast<std::uint8_t>(pan);header.data[33]=static_cast<std::uint8_t>(volume);item.children.push_back(header);list->children.push_back(item);
    undo_.push_back(save_bytes());if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;
}
bool BandDocument::undo(){if(undo_.empty())return false;auto next=Chunk::parse(undo_.back());redo_.push_back(save_bytes());undo_.pop_back();root_=std::move(next);return true;}
bool BandDocument::redo(){if(redo_.empty())return false;auto next=Chunk::parse(redo_.back());undo_.push_back(save_bytes());redo_.pop_back();root_=std::move(next);return true;}
bool is_band_track(const Chunk& track){const auto h=track.find("trkh");return track.id=="RIFF"&&track.type=="DMTK"&&h&&h->data.size()>=32&&std::memcmp(h->data.data(),&bandTrackId,16)==0;}
std::vector<BandEvent> band_track_events(const Chunk& track){
    if(!is_band_track(track))throw std::runtime_error("Expected BandTrack");
    const auto h=unique_band(track,"trkh");if(!read32(h->data,20))throw std::runtime_error("BandTrack has no groups");
    const auto data=unique_band(track,"RIFF","DMBT");if(!data)throw std::runtime_error("BandTrack payload missing");
    const auto items=unique_band(*data,"LIST","lbdl");if(!items)throw std::runtime_error("BandTrack items missing");std::vector<BandEvent> events;
    for(const auto& item:items->children)if(item.id=="LIST"&&item.type=="lbnd"){
        const auto old=unique_band(item,"bdih"),header=unique_band(item,"bd2h"),band=unique_band(item,"RIFF","DMBD");
        if((old&&header)||(!old&&!header)||!band||(header&&header->data.size()<8)||(old&&old->data.size()<4))throw std::runtime_error("Ambiguous or truncated BandTrack item");
        const auto time=static_cast<std::int32_t>(read32((header?header:old)->data,0));const auto physical=header?static_cast<std::int32_t>(read32(header->data,4)):time;
        for(const auto& e:events)if(e.logicalTime==time)throw std::runtime_error("Duplicate Band logical time requires explicit selection");
        BandDocument validation;const auto bytes=band->encode();validation.load(bytes);events.push_back({time,physical,bytes});
    }return events;
}
Chunk make_band_track(){
    Chunk track;track.id="RIFF";track.type="DMTK";Chunk header;header.id="trkh";header.data=Bytes(32);std::memcpy(header.data.data(),&bandTrackId,16);put32(header.data,20,1);std::memcpy(header.data.data()+28,"DMBT",4);track.children.push_back(header);
    Chunk data;data.id="RIFF";data.type="DMBT";Chunk automatic;automatic.id="bdth";automatic.data=Bytes(4);put32(automatic.data,0,1);data.children.push_back(automatic);Chunk items;items.id="LIST";items.type="lbdl";data.children.push_back(items);track.children.push_back(data);return track;
}
bool set_band_track_event(Chunk& track,std::int32_t time,const Bytes& bytes){
    if(time<0)return false;BandDocument validation;validation.load(bytes);(void)band_track_events(track);auto next=track;
    auto items=next.find("RIFF","DMBT")->find("LIST","lbdl");
    for(auto& item:items->children)if(item.id=="LIST"&&item.type=="lbnd"){
        const auto header=item.find("bd2h")?item.find("bd2h"):item.find("bdih");
        if(static_cast<std::int32_t>(read32(header->data,0))==time){auto band=item.find("RIFF","DMBD");if(band->encode()==bytes)return false;*band=Chunk::parse(bytes);track=std::move(next);return true;}
    }
    Chunk item;item.id="LIST";item.type="lbnd";Chunk header;header.id="bd2h";header.data=Bytes(8);put32(header.data,0,time);put32(header.data,4,time);item.children={header,Chunk::parse(bytes)};
    auto pos=items->children.end();for(auto i=items->children.begin();i!=items->children.end();++i)if(i->id=="LIST"&&i->type=="lbnd"){const auto h=i->find("bd2h")?i->find("bd2h"):i->find("bdih");if(static_cast<std::int32_t>(read32(h->data,0))>time){pos=i;break;}}
    items->children.insert(pos,item);track=std::move(next);return true;
}
bool move_band_track_event(Chunk& track,size_t index,std::int32_t logical,std::int32_t physical){
    const auto events=band_track_events(track);if(index>=events.size()||logical<0)return false;
    if(events[index].logicalTime==logical&&events[index].physicalTime==physical)return false;
    for(size_t i=0;i<events.size();++i)if(i!=index&&events[i].logicalTime==logical)return false;
    auto next=track;auto items=next.find("RIFF","DMBT")->find("LIST","lbdl");size_t at=0;
    for(auto& item:items->children)if(item.id=="LIST"&&item.type=="lbnd"&&at++==index){
        auto header=item.find("bd2h");if(!header){header=item.find("bdih");if(logical!=physical){header->id="bd2h";header->data.insert(header->data.begin()+4,4,0);}}
        put32(header->data,0,logical);if(header->id=="bd2h")put32(header->data,4,physical);break;
    }
    // Sort Band items within their existing slots; opaque sibling positions stay.
    std::vector<Chunk> ordered;for(const auto& item:items->children)if(item.id=="LIST"&&item.type=="lbnd")ordered.push_back(item);
    std::stable_sort(ordered.begin(),ordered.end(),[](const Chunk& a,const Chunk& b){const auto ah=a.find("bd2h")?a.find("bd2h"):a.find("bdih"),bh=b.find("bd2h")?b.find("bd2h"):b.find("bdih");return static_cast<std::int32_t>(read32(ah->data,0))<static_cast<std::int32_t>(read32(bh->data,0));});
    at=0;for(auto& item:items->children)if(item.id=="LIST"&&item.type=="lbnd")item=std::move(ordered[at++]);track=std::move(next);return true;
}
bool delete_band_track_event(Chunk& track,size_t index){
    const auto events=band_track_events(track);if(index>=events.size())return false;auto next=track;auto& items=next.find("RIFF","DMBT")->find("LIST","lbdl")->children;size_t at=0;
    for(auto i=items.begin();i!=items.end();++i)if(i->id=="LIST"&&i->type=="lbnd"&&at++==index){items.erase(i);break;}track=std::move(next);return true;
}
}
