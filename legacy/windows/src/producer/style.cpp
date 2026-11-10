#include "style.h"
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <objbase.h>

namespace producer::app {
namespace {
const Bytes styleClass={0x8a,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
const Bytes styleTrack={0x8d,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,4,0,0x60,8,0x93,0xb1,0xbd};
const Chunk* unique(const Chunk& root,const std::string& id,const std::string& type="") {
    const Chunk* found=nullptr;for(const auto& c:root.children)if(c.id==id&&(type.empty()||c.type==type)){
        if(found)throw std::runtime_error("Ambiguous Style chunk");found=&c;
    }return found;
}
std::vector<Chunk*> style_descriptors(Chunk& root){
    (void)producer::app::style_references(root);std::vector<std::pair<std::int32_t,Chunk*>> found;
    if(auto tracks=root.find("LIST","trkl"))for(auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto header=unique(track,"trkh");if(!header||!std::equal(styleTrack.begin(),styleTrack.end(),header->data.begin()))continue;
        for(auto& ref:track.find("LIST","sttr")->children)if(ref.id=="LIST"&&ref.type=="strf")found.push_back({static_cast<std::int32_t>(read32(ref.find("stmp")->data,0)),ref.find("LIST","DMRF")});
    }
    std::stable_sort(found.begin(),found.end(),[](const auto& a,const auto& b){return a.first<b.first;});std::vector<Chunk*> result;for(const auto& item:found)result.push_back(item.second);return result;
}
StyleMeter read_meter(const Bytes& b) {
    if(b.size()<4)throw std::runtime_error("Style meter truncated");
    const auto grids=static_cast<std::uint16_t>(b[2]|(b[3]<<8));
    if(!b[0]||!b[1]||b[1]>128||(b[1]&(b[1]-1))||!grids)throw std::runtime_error("Unsupported Style meter");
    return {b[0],b[1],grids};
}
Chunk* selected_part(Chunk& root,size_t index){size_t at=0;for(auto& c:root.children)if(c.id=="LIST"&&c.type=="part"&&at++==index)return &c;throw std::out_of_range("Part selection");}
bool valid_note_edit(const StyleNoteEdit& n){return n.duration>0&&n.velocity>0&&n.velocity<=127;}
void write_note_edit(Bytes& b,size_t at,const StyleNoteEdit& n){
    put32(b,at,static_cast<std::uint32_t>(n.gridStart));put32(b,at+4,n.variation);put32(b,at+8,static_cast<std::uint32_t>(n.duration));
    const auto offset=static_cast<std::uint16_t>(n.timeOffset);b[at+12]=static_cast<std::uint8_t>(offset);b[at+13]=static_cast<std::uint8_t>(offset>>8);b[at+14]=static_cast<std::uint8_t>(n.musicValue);b[at+15]=static_cast<std::uint8_t>(n.musicValue>>8);b[at+16]=static_cast<std::uint8_t>(n.velocity);
}
std::filesystem::path contained(const std::filesystem::path& directory,const std::wstring& filename,bool runtimeReferences) {
    const auto relative=std::filesystem::path(filename);if(relative.is_absolute()||relative.has_root_name())throw std::runtime_error("Style reference must be relative");
    const auto full=(directory/relative).lexically_normal();const auto relation=full.lexically_relative(directory);
    if(!runtimeReferences&&(relation.empty()||*relation.begin()==L".."))throw std::runtime_error("Style reference escapes directory");
    // Also check junctions/symlinks before reading the dependency.
    const auto physical=std::filesystem::canonical(full),base=std::filesystem::canonical(directory);
    const auto physicalRelation=physical.lexically_relative(base);
    if(!runtimeReferences&&(physicalRelation.empty()||*physicalRelation.begin()==L".."))throw std::runtime_error("Style reference resolves outside directory");return full;
}
}
StyleDocument::StyleDocument() {
    root_.id="RIFF";root_.type="DMST";Chunk header;header.id="styh";header.data=Bytes(12);header.data[0]=4;header.data[1]=4;header.data[2]=4;const double tempo=120;std::memcpy(header.data.data()+4,&tempo,8);root_.children.push_back(header);
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Style GUID creation failed");Chunk identity;identity.id="guid";identity.data.resize(16);std::memcpy(identity.data.data(),&id,16);root_.children.push_back(identity);saved_=save_bytes();
}
void StyleDocument::load(const Bytes& bytes) {
    auto root=Chunk::parse(bytes);if(root.id!="RIFF"||root.type!="DMST")throw std::runtime_error("Expected DMST Style");
    const auto header=unique(root,"styh");if(!header||(header->data.size()!=12&&header->data.size()<16))throw std::runtime_error("Unsupported Style header size");
    (void)read_meter(header->data);double tempo;std::memcpy(&tempo,header->data.data()+(header->data.size()==12?4:8),8);
    if(!std::isfinite(tempo)||tempo<=0)throw std::runtime_error("Invalid Style tempo");
    if(const auto guid=unique(root,"guid"))if(guid->data.size()!=16)throw std::runtime_error("Style GUID size");
    root_=std::move(root);saved_=bytes;undo_.clear();redo_.clear();dirty_=false;
}
void StyleDocument::save(const std::wstring& path){const auto bytes=save_bytes();write_file_atomic(path,bytes);saved_=bytes;dirty_=false;}
std::wstring StyleDocument::name() const {
    const auto info=unique(root_,"LIST","UNFO");if(!info)return L"";
    const auto value=unique(*info,"UNAM");return value?decode_utf16(value->data):L"";
}
bool StyleDocument::set_name(const std::wstring& value){
    if(value.empty()||value.size()>255||value.find(L'\0')!=std::wstring::npos)return false;
    if(name()==value)return false;auto next=root_;
    if(!next.find("LIST","UNFO")){Chunk info;info.id="LIST";info.type="UNFO";next.children.push_back(info);}
    auto info=next.find("LIST","UNFO");if(!info->find("UNAM")){Chunk n;n.id="UNAM";info->children.push_back(n);}
    info->find("UNAM")->data=utf16(value);adopt_edit(std::move(next));return true;
}
void StyleDocument::relocate_context(const std::wstring& oldDirectory,const std::wstring& newDirectory,const std::vector<CollectionEntry>& catalog){
    auto rebase=[&](const Bytes& bytes){if(bytes.empty())return bytes;return relocate_collection_references(bytes,resolve_collections(document_collection_references(bytes),oldDirectory,catalog),newDirectory);};
    auto next=*this;const auto bytes=rebase(save_bytes());for(auto& snapshot:next.undo_)snapshot=rebase(snapshot);for(auto& snapshot:next.redo_)snapshot=rebase(snapshot);StyleDocument check;check.load(bytes);next.root_=Chunk::parse(bytes);next.dirty_=bytes!=next.saved_;*this=std::move(next);
}
bool StyleDocument::relocate_collections(const std::vector<ResolvedCollection>& dependencies,const std::wstring& directory){
    const auto before=save_bytes(),after=relocate_collection_references(before,dependencies,directory);if(before==after)return false;
    StyleDocument check;check.load(after);adopt_edit(Chunk::parse(after));return true;
}
void StyleDocument::adopt_edit(Chunk root){
    auto next=*this;next.root_=std::move(root);next.undo_.push_back(save_bytes());if(next.undo_.size()>100)next.undo_.erase(next.undo_.begin());next.redo_.clear();next.dirty_=next.save_bytes()!=next.saved_;*this=std::move(next);
}
bool StyleDocument::set_tempo(double tempo){
    if(!std::isfinite(tempo)||tempo<1||tempo>1000||tempo==this->tempo())return false;
    auto root=root_;auto header=root.find("styh");std::memcpy(header->data.data()+(header->data.size()==12?4:8),&tempo,8);adopt_edit(std::move(root));return true;
}
bool StyleDocument::set_meter(unsigned beats,unsigned denominator,unsigned grids){
    if(!beats||beats>255||!denominator||denominator>128||(denominator&(denominator-1))||!grids||grids>65535)return false;
    const auto old=meter();if(old.beats==beats&&old.denominator==denominator&&old.grids==grids)return false;
    auto root=root_;auto& data=root.find("styh")->data;data[0]=static_cast<std::uint8_t>(beats);data[1]=static_cast<std::uint8_t>(denominator);data[2]=static_cast<std::uint8_t>(grids);data[3]=static_cast<std::uint8_t>(grids>>8);adopt_edit(std::move(root));return true;
}
std::vector<StylePattern> StyleDocument::patterns() const{
    std::vector<StylePattern> result;
    for(const auto& child:root_.children)if(child.id=="LIST"&&child.type=="pttn"){
        const auto header=unique(child,"ptnh");if(!header||header->data.size()<10)throw std::runtime_error("Pattern header truncated");const auto& b=header->data;
        StylePattern pattern{};pattern.meter=read_meter(b);pattern.grooveBottom=b[4];pattern.grooveTop=b[5];pattern.embellishment=b[6]|(b[7]<<8);pattern.measures=b[8]|(b[9]<<8);
        if(pattern.grooveBottom>100||pattern.grooveTop>100||pattern.grooveBottom>pattern.grooveTop||!pattern.measures)throw std::runtime_error("Unsupported Pattern range or length");
        if(const auto info=unique(child,"LIST","UNFO"))if(const auto name=unique(*info,"UNAM"))pattern.name=decode_utf16(name->data);
        result.push_back(std::move(pattern));
    }return result;
}
bool StyleDocument::set_pattern_groove(size_t index,unsigned bottom,unsigned top){
    if(bottom>100||top>100||bottom>top)return false;const auto parsed=patterns();if(index>=parsed.size())return false;if(parsed[index].grooveBottom==bottom&&parsed[index].grooveTop==top)return false;
    auto root=root_;size_t current=0;for(auto& child:root.children)if(child.id=="LIST"&&child.type=="pttn"){if(current++==index){auto& data=child.find("ptnh")->data;data[4]=static_cast<std::uint8_t>(bottom);data[5]=static_cast<std::uint8_t>(top);adopt_edit(std::move(root));return true;}}return false;
}
bool StyleDocument::set_pattern_properties(size_t index,const std::wstring& name,unsigned embellishment){
    if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)return false;
    const auto parsed=patterns();if(index>=parsed.size())return false;const auto& old=parsed[index];
    if(embellishment!=old.embellishment&&(embellishment>15||old.embellishment>15))return false;
    if(name==old.name&&embellishment==old.embellishment)return false;
    auto next=root_;size_t at=0;for(auto& pattern:next.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==index){
        if(name!=old.name){auto info=pattern.find("LIST","UNFO");if(!info){Chunk c;c.id="LIST";c.type="UNFO";pattern.children.push_back(std::move(c));info=&pattern.children.back();}
            auto label=info->find("UNAM");if(!label){Chunk c;c.id="UNAM";info->children.push_back(std::move(c));label=&info->children.back();}label->data=utf16(name);}
        auto& b=pattern.find("ptnh")->data;b[6]=static_cast<std::uint8_t>(embellishment);b[7]=static_cast<std::uint8_t>(embellishment>>8);
        adopt_edit(std::move(next));return true;
    }return false;
}
bool StyleDocument::undo(){if(undo_.empty())return false;auto next=*this;next.root_=Chunk::parse(next.undo_.back());next.undo_.pop_back();next.redo_.push_back(save_bytes());next.dirty_=next.save_bytes()!=next.saved_;*this=std::move(next);return true;}
bool StyleDocument::duplicate_pattern(size_t index,const std::wstring& name){
    const auto parsed=patterns();if(index>=parsed.size()||parsed.size()>=1000||name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)return false;
    (void)part_references(index); // Resolve every source binding before changing ownership.
    auto next=root_;Chunk copy;size_t at=0;for(const auto& child:root_.children)if(child.id=="LIST"&&child.type=="pttn"&&at++==index){copy=child;break;}
    (void)unique(copy,"LIST","UNFO");auto info=copy.find("LIST","UNFO");if(!info){Chunk c;c.id="LIST";c.type="UNFO";copy.children.push_back(std::move(c));info=&copy.children.back();}
    (void)unique(*info,"UNAM");auto label=info->find("UNAM");if(!label){Chunk c;c.id="UNAM";info->children.push_back(std::move(c));label=&info->children.back();}label->data=utf16(name);
    next.children.push_back(std::move(copy));adopt_edit(std::move(next));return true;
}
Bytes StyleDocument::copy_pattern(size_t index) const{
    if(index>=patterns().size())return {};
    const auto refs=part_references(index);std::vector<size_t> included;Chunk result;result.id="RIFF";result.type="SPC1";
    for(const auto& ref:refs)if(std::find(included.begin(),included.end(),ref.partIndex)==included.end()){
        (void)part_notes(ref.partIndex);size_t at=0;for(const auto& part:root_.children)if(part.id=="LIST"&&part.type=="part"&&at++==ref.partIndex){result.children.push_back(part);break;}included.push_back(ref.partIndex);
    }
    size_t at=0;for(const auto& pattern:root_.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==index){result.children.push_back(pattern);break;}
    return result.encode();
}
bool StyleDocument::paste_pattern(const Bytes& bytes,const std::wstring& name){
    if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos||patterns().size()>=1000)return false;
    const auto payload=Chunk::parse(bytes);if(payload.type!="SPC1")throw std::runtime_error("Unsupported Pattern clipboard version");
    StyleDocument source;auto staging=source.root_;size_t patternCount=0;
    for(const auto& c:payload.children){if(c.id!="LIST"||(c.type!="part"&&c.type!="pttn"))throw std::runtime_error("Unexpected Pattern clipboard chunk");if(c.type=="pttn")++patternCount;staging.children.push_back(c);}
    if(patternCount!=1)throw std::runtime_error("Pattern clipboard requires one Pattern");
    source.load(staging.encode());const auto catalog=source.parts();const auto refs=source.part_references(0);(void)source.patterns();
    if(catalog.size()>1000||parts().size()>1000-catalog.size())return false;
    for(size_t i=0;i<catalog.size();++i){if(std::none_of(refs.begin(),refs.end(),[&](const StylePartReference& r){return r.partIndex==i;}))throw std::runtime_error("Unreferenced clipboard Part");(void)source.part_notes(i);}
    // Validate/rename only the copy; neither source nor destination history is changed yet.
    (void)source.set_pattern_properties(0,name,source.patterns()[0].embellishment);
    auto next=root_;const auto existing=parts();std::vector<std::array<std::uint8_t,16>> identities;
    for(size_t i=0;i<catalog.size();++i){GUID guid{};if(FAILED(CoCreateGuid(&guid)))throw std::runtime_error("Clipboard Part GUID creation failed");std::array<std::uint8_t,16> id{};std::memcpy(id.data(),&guid,16);
        if(std::any_of(catalog.begin(),catalog.end(),[&](const StylePart& p){return p.objectId==id;})||std::find(identities.begin(),identities.end(),id)!=identities.end()||std::any_of(existing.begin(),existing.end(),[&](const StylePart& p){return p.objectId==id;}))throw std::runtime_error("Clipboard Part GUID collision");identities.push_back(id);
    }
    for(size_t i=0;i<catalog.size();++i){auto part=*selected_part(source.root_,i);std::copy(identities[i].begin(),identities[i].end(),part.find("prth")->data.begin()+132);next.children.push_back(std::move(part));}
    auto pattern=*source.root_.find("LIST","pttn");size_t ri=0;
    for(auto& ref:pattern.children)if(ref.id=="LIST"&&ref.type=="pref"){const auto& id=identities.at(refs.at(ri++).partIndex);std::copy(id.begin(),id.end(),ref.find("prfc")->data.begin());}
    next.children.push_back(std::move(pattern));StyleDocument validate;validate.load(next.encode());(void)validate.part_references(patterns().size());
    adopt_edit(std::move(next));return true;
}
bool StyleDocument::unshare_pattern_part(size_t patternIndex,size_t referenceIndex){
    const auto parsed=patterns();if(patternIndex>=parsed.size())return false;
    const auto refs=part_references(patternIndex);if(referenceIndex>=refs.size())return false;
    const auto selected=refs[referenceIndex];const auto catalog=parts();size_t uses=0;
    for(size_t i=0;i<parsed.size();++i)for(const auto& ref:part_references(i))if(ref.objectId==selected.objectId)++uses;
    if(uses<2)return false;if(catalog.size()>=1000)throw std::runtime_error("Style Part limit");
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Part GUID creation failed");
    std::array<std::uint8_t,16> id{};std::memcpy(id.data(),&identity,16);for(const auto& part:catalog)if(part.objectId==id)throw std::runtime_error("Part GUID collision");
    auto next=root_;auto copy=*selected_part(next,selected.partIndex);std::copy(id.begin(),id.end(),copy.find("prth")->data.begin()+132);
    size_t at=0,position=0;for(size_t i=0;i<next.children.size();++i){auto& pattern=next.children[i];if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==patternIndex){position=i;size_t ri=0;for(auto& ref:pattern.children)if(ref.id=="LIST"&&ref.type=="pref"&&ri++==referenceIndex){std::copy(id.begin(),id.end(),ref.find("prfc")->data.begin());break;}break;}}
    // The runtime resolves Part bindings while reading Patterns. Keep the new
    // Part before its first reference rather than introducing a forward binding.
    next.children.insert(next.children.begin()+position,std::move(copy));adopt_edit(std::move(next));return true;
}
bool StyleDocument::new_pattern(const std::wstring& name,unsigned pchannel){
    if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos||pchannel>15)return false;
    const auto catalog=parts();const auto parsed=patterns();if(catalog.size()>=1000||parsed.size()>=1000)return false;
    GUID guid{};if(FAILED(CoCreateGuid(&guid)))throw std::runtime_error("Part GUID creation failed");std::array<std::uint8_t,16> id{};std::memcpy(id.data(),&guid,16);
    for(const auto& p:catalog)if(p.objectId==id)throw std::runtime_error("Part GUID collision");
    auto leaf=[](const char* key,Bytes data){Chunk c;c.id=key;c.data=std::move(data);return c;};
    auto list=[](const char* type,std::vector<Chunk> children){Chunk c;c.id="LIST";c.type=type;c.children=std::move(children);return c;};
    const auto meterValue=meter();Bytes partHeader(160),patternHeader(16),reference(28),notes(4);
    for(auto* b:{&partHeader,&patternHeader}){(*b)[0]=meterValue.beats;(*b)[1]=meterValue.denominator;(*b)[2]=static_cast<std::uint8_t>(meterValue.grids);(*b)[3]=static_cast<std::uint8_t>(meterValue.grids>>8);}
    for(size_t i=0;i<32;++i)put32(partHeader,4+i*4,0xffffffffu);
    std::copy(id.begin(),id.end(),partHeader.begin()+132);partHeader[148]=1;partHeader[151]=127;
    patternHeader[5]=100;patternHeader[8]=1;patternHeader[11]=100;
    std::copy(id.begin(),id.end(),reference.begin());reference[20]=128;put32(reference,24,pchannel);put32(notes,0,24);
    auto next=root_;next.children.push_back(list("part",{leaf("prth",partHeader),leaf("note",notes),list("UNFO",{leaf("UNAM",utf16(name+L" Part"))})}));
    next.children.push_back(list("pttn",{leaf("ptnh",patternHeader),leaf("rhtm",Bytes(4)),list("UNFO",{leaf("UNAM",utf16(name))}),list("pref",{leaf("prfc",reference)})}));
    StyleDocument validate;validate.load(next.encode());(void)validate.part_references(parsed.size());adopt_edit(std::move(next));return true;
}
bool StyleDocument::delete_pattern(size_t index){
    const auto parsed=patterns();if(index>=parsed.size())return false;
    const auto removed=part_references(index);std::vector<std::array<std::uint8_t,16>> retained;
    for(size_t i=0;i<parsed.size();++i)if(i!=index)for(const auto& r:part_references(i))retained.push_back(r.objectId);
    auto next=root_;size_t at=0;for(auto i=next.children.begin();i!=next.children.end();++i)if(i->id=="LIST"&&i->type=="pttn"&&at++==index){next.children.erase(i);break;}
    next.children.erase(std::remove_if(next.children.begin(),next.children.end(),[&](const Chunk& c){if(c.id!="LIST"||c.type!="part")return false;const auto h=unique(c,"prth");std::array<std::uint8_t,16> id{};std::copy_n(h->data.begin()+132,16,id.begin());return std::any_of(removed.begin(),removed.end(),[&](const StylePartReference& r){return r.objectId==id;})&&std::find(retained.begin(),retained.end(),id)==retained.end();}),next.children.end());
    adopt_edit(std::move(next));return true;
}
std::optional<StyleMotifSettings> StyleDocument::motif_settings(size_t index) const{
    const auto parsed=patterns();if(index>=parsed.size())throw std::out_of_range("Motif selection");
    if(!(parsed[index].embellishment&16))return {};
    size_t at=0;for(const auto& p:root_.children)if(p.id=="LIST"&&p.type=="pttn"&&at++==index){
        const auto c=unique(p,"mtfs");if(!c)return {};if(c->data.size()<20)throw std::runtime_error("Motif settings truncated");
        return StyleMotifSettings{read32(c->data,0),static_cast<std::int32_t>(read32(c->data,4)),static_cast<std::int32_t>(read32(c->data,8)),static_cast<std::int32_t>(read32(c->data,12)),read32(c->data,16)};
    }return {};
}
bool StyleDocument::set_motif_settings(size_t index,const StyleMotifSettings& value){
    const auto parsed=patterns();if(index>=parsed.size()||!(parsed[index].embellishment&16))return false;
    const auto& p=parsed[index];const auto length=static_cast<std::int64_t>(p.meter.beats)*3072*p.measures/p.meter.denominator;
    if(value.playStart<0||value.playStart>=length||value.loopStart<0||value.loopStart>=length||value.loopEnd<0||(value.loopEnd&&(value.loopStart>=value.loopEnd||value.loopEnd>length)))return false;
    const auto old=motif_settings(index);if(old&&old->repeats==value.repeats&&old->playStart==value.playStart&&old->loopStart==value.loopStart&&old->loopEnd==value.loopEnd&&old->resolution==value.resolution)return false;
    auto next=root_;size_t at=0;for(auto& motif:next.children)if(motif.id=="LIST"&&motif.type=="pttn"&&at++==index){
        auto c=motif.find("mtfs");if(!c){Chunk settings;settings.id="mtfs";settings.data=Bytes(20);motif.children.push_back(std::move(settings));c=&motif.children.back();}
        put32(c->data,0,value.repeats);put32(c->data,4,static_cast<std::uint32_t>(value.playStart));put32(c->data,8,static_cast<std::uint32_t>(value.loopStart));put32(c->data,12,static_cast<std::uint32_t>(value.loopEnd));put32(c->data,16,value.resolution);adopt_edit(std::move(next));return true;
    }return false;
}
bool StyleDocument::assign_motif_band(size_t patternIndex,size_t bandIndex){
    const auto parsed=patterns();const auto owned=bands();if(patternIndex>=parsed.size()||!(parsed[patternIndex].embellishment&16)||bandIndex>=owned.size())return false;
    auto next=root_;size_t at=0;for(auto& pattern:next.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==patternIndex){
        Chunk* existing=nullptr;for(auto& child:pattern.children)if(child.id=="RIFF"&&child.type=="DMBD"){if(existing)throw std::runtime_error("Ambiguous Motif Band");existing=&child;}
        const auto bytes=owned[bandIndex].save_bytes();if(existing&&existing->encode()==bytes)return false;if(existing)*existing=Chunk::parse(bytes);else pattern.children.push_back(Chunk::parse(bytes));break;
    }adopt_edit(std::move(next));return true;
}
std::optional<BandDocument> StyleDocument::motif_band(size_t patternIndex) const {
    const auto parsed=patterns();if(patternIndex>=parsed.size()||!(parsed[patternIndex].embellishment&16))return {};
    size_t at=0;for(const auto& pattern:root_.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==patternIndex){
        const Chunk* found=nullptr;for(const auto& child:pattern.children)if(child.id=="RIFF"&&child.type=="DMBD"){if(found)throw std::runtime_error("Ambiguous Motif Band");found=&child;}
        if(found){BandDocument band;band.load(found->encode());return band;}return {};
    }return {};
}
bool StyleDocument::set_motif_band_instrument(size_t patternIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){
    auto band=motif_band(patternIndex);if(!band||!band->set_instrument(instrumentIndex,patch,pchannel,pan,volume))return false;
    auto next=root_;size_t at=0;for(auto& pattern:next.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==patternIndex){
        for(auto& child:pattern.children)if(child.id=="RIFF"&&child.type=="DMBD"){child=Chunk::parse(band->save_bytes());adopt_edit(std::move(next));return true;}
    }return false;
}
bool StyleDocument::set_motif_band_dls_instrument(size_t patternIndex,size_t instrumentIndex,const CollectionReference& reference,std::uint32_t bank,std::uint32_t program){
    auto band=motif_band(patternIndex);if(!band||!band->set_dls_instrument(instrumentIndex,reference,bank,program))return false;
    auto next=root_;size_t at=0;for(auto& pattern:next.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==patternIndex){
        for(auto& child:pattern.children)if(child.id=="RIFF"&&child.type=="DMBD"){child=Chunk::parse(band->save_bytes());adopt_edit(std::move(next));return true;}
    }return false;
}
bool StyleDocument::new_motif(const std::wstring& name,unsigned pchannel){
    for(const auto& p:patterns())if((p.embellishment&16)&&p.name==name)return false;
    auto next=*this;if(!next.new_pattern(name,pchannel))return false;
    auto& pattern=next.root_.children.back();pattern.find("ptnh")->data[6]=16;
    Chunk settings;settings.id="mtfs";settings.data=Bytes(20);put32(settings.data,16,1);pattern.children.push_back(std::move(settings));
    adopt_edit(std::move(next.root_));return true;
}
std::vector<BandDocument> StyleDocument::bands() const {std::vector<BandDocument> result;for(const auto& child:root_.children)if(child.id=="RIFF"&&child.type=="DMBD"){BandDocument band;band.load(child.encode());result.push_back(std::move(band));}return result;}
bool StyleDocument::set_band_instrument(size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){
    auto parsed=bands();if(bandIndex>=parsed.size()||!parsed[bandIndex].set_instrument(instrumentIndex,patch,pchannel,pan,volume))return false;
    auto next=root_;size_t at=0;for(auto& child:next.children)if(child.id=="RIFF"&&child.type=="DMBD"&&at++==bandIndex){child=Chunk::parse(parsed[bandIndex].save_bytes());break;}adopt_edit(std::move(next));return true;
}
bool StyleDocument::add_band_gm_instrument(std::optional<size_t> bandIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){
    auto parsed=bands();if(bandIndex&&*bandIndex>=parsed.size())return false;
    BandDocument band=bandIndex?parsed[*bandIndex]:BandDocument{};if(!band.add_gm_instrument(patch,pchannel,pan,volume))return false;
    auto next=root_;if(bandIndex){size_t at=0;for(auto& child:next.children)if(child.id=="RIFF"&&child.type=="DMBD"&&at++==*bandIndex){child=Chunk::parse(band.save_bytes());break;}}
    else next.children.push_back(Chunk::parse(band.save_bytes()));adopt_edit(std::move(next));return true;
}
bool StyleDocument::set_band_dls_instrument(size_t bandIndex,size_t instrumentIndex,const CollectionReference& reference,std::uint32_t bank,std::uint32_t program){
    auto parsed=bands();if(bandIndex>=parsed.size()||!parsed[bandIndex].set_dls_instrument(instrumentIndex,reference,bank,program))return false;
    auto next=root_;size_t at=0;for(auto& child:next.children)if(child.id=="RIFF"&&child.type=="DMBD"&&at++==bandIndex){child=Chunk::parse(parsed[bandIndex].save_bytes());break;}adopt_edit(std::move(next));return true;
}
std::vector<StylePart> StyleDocument::parts() const{
    std::vector<StylePart> result;for(const auto& child:root_.children)if(child.id=="LIST"&&child.type=="part"){
        const auto h=unique(child,"prth");if(!h||h->data.size()<150)throw std::runtime_error("Part header truncated");StylePart p{};p.meter=read_meter(h->data);std::copy_n(h->data.begin()+132,16,p.objectId.begin());p.measures=h->data[148]|(h->data[149]<<8);if(!p.measures)throw std::runtime_error("Part has zero measures");
        for(size_t i=0;i<p.variationChoices.size();++i)p.variationChoices[i]=read32(h->data,4+i*4);
        for(const auto& previous:result)if(previous.objectId==p.objectId)throw std::runtime_error("Ambiguous Part GUID");if(const auto info=unique(child,"LIST","UNFO"))if(const auto name=unique(*info,"UNAM"))p.name=decode_utf16(name->data);result.push_back(std::move(p));
    }return result;
}
std::vector<StylePartReference> StyleDocument::part_references(size_t index) const{
    const auto catalog=parts();std::vector<StylePartReference> result;size_t at=0;
    for(const auto& pattern:root_.children)if(pattern.id=="LIST"&&pattern.type=="pttn"&&at++==index){
        for(const auto& ref:pattern.children)if(ref.id=="LIST"&&ref.type=="pref"){
            const auto h=unique(ref,"prfc");if(!h||h->data.size()<20)throw std::runtime_error("Part reference truncated");StylePartReference p{};std::copy_n(h->data.begin(),16,p.objectId.begin());const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const StylePart& part){return part.objectId==p.objectId;});if(found==catalog.end())throw std::runtime_error("Part reference unresolved");p.partIndex=static_cast<size_t>(found-catalog.begin());if(h->data.size()>=28)p.pchannel=read32(h->data,24);result.push_back(p);
        }return result;
    }throw std::out_of_range("Pattern selection");
}
std::vector<StyleNote> StyleDocument::part_notes(size_t index) const{
    const auto catalog=parts();if(index>=catalog.size())throw std::out_of_range("Part selection");size_t at=0;std::vector<StyleNote> result;
    for(const auto& part:root_.children)if(part.id=="LIST"&&part.type=="part"&&at++==index){
        const auto notes=unique(part,"note");if(!notes)return result;const auto& b=notes->data;if(b.size()<4)throw std::runtime_error("Style note record size missing");const auto stride=read32(b,0);if(stride<22||(b.size()-4)%stride)throw std::runtime_error("Unsupported Style note array");
        for(size_t n=4;n<b.size();n+=stride){StyleNote note{};note.gridStart=static_cast<std::int32_t>(read32(b,n));note.variation=read32(b,n+4);note.duration=static_cast<std::int32_t>(read32(b,n+8));note.timeOffset=static_cast<std::int16_t>(b[n+12]|(b[n+13]<<8));note.musicValue=static_cast<std::uint16_t>(b[n+14]|(b[n+15]<<8));note.velocity=b[n+16];note.playMode=b[n+21];if(stride>=23)note.flags=b[n+22];result.push_back(note);}return result;
    }throw std::out_of_range("Part selection");
}
bool StyleDocument::set_part_variation_choice(size_t partIndex,size_t variationIndex,std::uint32_t choices){
    if(variationIndex>=32)return false;const auto catalog=parts();if(partIndex>=catalog.size()||catalog[partIndex].variationChoices[variationIndex]==choices)return false;
    auto next=root_;put32(selected_part(next,partIndex)->find("prth")->data,4+variationIndex*4,choices);adopt_edit(std::move(next));return true;
}
bool StyleDocument::set_part_note(size_t partIndex,size_t noteIndex,std::int32_t duration,unsigned velocity){
    if(duration<=0||!velocity||velocity>127)return false;const auto catalog=parts();if(partIndex>=catalog.size())return false;const auto notes=part_notes(partIndex);if(noteIndex>=notes.size())return false;if(notes[noteIndex].duration==duration&&notes[noteIndex].velocity==velocity)return false;
    auto root=root_;size_t at=0;for(auto& part:root.children)if(part.id=="LIST"&&part.type=="part"&&at++==partIndex){auto& b=part.find("note")->data;const auto position=4+noteIndex*read32(b,0);put32(b,position+8,static_cast<std::uint32_t>(duration));b[position+16]=static_cast<std::uint8_t>(velocity);adopt_edit(std::move(root));return true;}return false;
}
bool StyleDocument::edit_part_note(size_t partIndex,size_t noteIndex,const StyleNoteEdit& value){
    if(!valid_note_edit(value)||partIndex>=parts().size())return false;const auto notes=part_notes(partIndex);if(noteIndex>=notes.size())return false;
    auto root=root_;auto& b=selected_part(root,partIndex)->find("note")->data;const auto before=b;write_note_edit(b,4+noteIndex*read32(b,0),value);if(b==before)return false;adopt_edit(std::move(root));return true;
}
bool StyleDocument::insert_part_note(size_t partIndex,size_t position,const StyleNoteEdit& value,std::optional<size_t> templateIndex){
    if(!valid_note_edit(value)||partIndex>=parts().size())return false;const auto parsed=part_notes(partIndex);if(position>parsed.size()||(templateIndex&&*templateIndex>=parsed.size()))return false;
    auto root=root_;auto* part=selected_part(root,partIndex);auto* notes=part->find("note");
    if(!notes){Chunk n;n.id="note";n.data=Bytes(4);put32(n.data,0,24);part->children.push_back(std::move(n));notes=&part->children.back();}
    auto& b=notes->data;const auto stride=read32(b,0);if(b.size()>UINT32_MAX-stride)throw std::runtime_error("Style note array size limit");Bytes record(stride,0);
    if(templateIndex){const auto at=4+*templateIndex*stride;std::copy_n(b.begin()+at,stride,record.begin());}write_note_edit(record,0,value);
    b.insert(b.begin()+4+position*stride,record.begin(),record.end());adopt_edit(std::move(root));return true;
}
bool StyleDocument::delete_part_note(size_t partIndex,size_t noteIndex){
    if(partIndex>=parts().size())return false;const auto notes=part_notes(partIndex);if(noteIndex>=notes.size())return false;auto root=root_;auto& b=selected_part(root,partIndex)->find("note")->data;const auto stride=read32(b,0);const auto at=4+noteIndex*stride;b.erase(b.begin()+at,b.begin()+at+stride);adopt_edit(std::move(root));return true;
}
bool StyleDocument::set_pattern_layout(size_t index,unsigned beats,unsigned denominator,unsigned grids,unsigned measures){
    if(!beats||beats>255||!denominator||denominator>128||(denominator&(denominator-1))||!grids||grids>65535||!measures||measures>65535)return false;
    const auto parsed=patterns();if(index>=parsed.size())return false;const auto& old=parsed[index];if(old.meter.beats==beats&&old.meter.denominator==denominator&&old.meter.grids==grids&&old.measures==measures)return false;
    auto root=root_;size_t current=0;for(auto& child:root.children)if(child.id=="LIST"&&child.type=="pttn"&&current++==index){
        const auto rhythm=unique(child,"rhtm");if(!rhythm||rhythm->data.size()!=static_cast<size_t>(old.measures)*4)throw std::runtime_error("Pattern rhythm must have one DWORD per measure");
        const auto length=static_cast<std::int64_t>(beats)*3072*measures/denominator;
        if(const auto motif=unique(child,"mtfs")){
            if(motif->data.size()<20)throw std::runtime_error("Motif settings truncated");const auto start=static_cast<std::int32_t>(read32(motif->data,4)),loopStart=static_cast<std::int32_t>(read32(motif->data,8)),loopEnd=static_cast<std::int32_t>(read32(motif->data,12));
            if(start<0||start>=length||loopStart<0||loopEnd<0||(loopEnd&&(loopStart>=loopEnd||loopEnd>length)))throw std::runtime_error("Pattern layout would invalidate motif playback bounds");
        }
        auto& header=child.find("ptnh")->data;header[0]=static_cast<std::uint8_t>(beats);header[1]=static_cast<std::uint8_t>(denominator);header[2]=static_cast<std::uint8_t>(grids);header[3]=static_cast<std::uint8_t>(grids>>8);header[8]=static_cast<std::uint8_t>(measures);header[9]=static_cast<std::uint8_t>(measures>>8);
        // Part lengths/meters and all event/variation bytes are independent.
        // Preserve existing rhythm measures; new measures have no chord accents.
        child.find("rhtm")->data.resize(static_cast<size_t>(measures)*4,0);adopt_edit(std::move(root));return true;
    }return false;
}
bool StyleDocument::redo(){if(redo_.empty())return false;auto next=*this;next.root_=Chunk::parse(next.redo_.back());next.redo_.pop_back();next.undo_.push_back(save_bytes());next.dirty_=next.save_bytes()!=next.saved_;*this=std::move(next);return true;}
StyleMeter StyleDocument::meter() const {const auto h=unique(root_,"styh");if(!h)throw std::runtime_error("Style not loaded");return read_meter(h->data);}
double StyleDocument::tempo() const {const auto h=unique(root_,"styh");if(!h)throw std::runtime_error("Style not loaded");double result;std::memcpy(&result,h->data.data()+(h->data.size()==12?4:8),8);return result;}
bool StyleDocument::has_object_id() const{return unique(root_,"guid")!=nullptr;}
std::array<std::uint8_t,16> StyleDocument::object_id() const {const auto id=unique(root_,"guid");if(!id)throw std::runtime_error("Style identity missing");std::array<std::uint8_t,16> result{};std::copy(id->data.begin(),id->data.end(),result.begin());return result;}
std::vector<StyleReference> style_references(const Chunk& segment) {
    std::vector<StyleReference> result;const auto tracks=segment.find("LIST","trkl");if(!tracks)return result;
    for(const auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK") {
        const auto h=unique(track,"trkh");if(!h||h->data.size()<32)throw std::runtime_error("Style lookup track header");
        if(!std::equal(styleTrack.begin(),styleTrack.end(),h->data.begin()))continue;
        const auto groups=read32(h->data,20);if(!groups)throw std::runtime_error("Style track groups missing");
        const auto list=unique(track,"LIST","sttr");if(!list)throw std::runtime_error("Style track data missing");
        for(const auto& ref:list->children)if(ref.id=="LIST"&&ref.type=="strf"){
            const auto time=unique(ref,"stmp"),descriptor=unique(ref,"LIST","DMRF");
            if(!time||time->data.size()!=4||read32(time->data,0)>INT32_MAX||!descriptor)throw std::runtime_error("Style reference time or descriptor");
            const auto header=unique(*descriptor,"refh");if(!header||header->data.size()<20||!std::equal(styleClass.begin(),styleClass.end(),header->data.begin()))throw std::runtime_error("Style reference class");
            const auto valid=read32(header->data,16);StyleReference item{};item.time=static_cast<std::int32_t>(read32(time->data,0));item.groups=groups;
            if(valid&1){const auto guid=unique(*descriptor,"guid");if(!guid||guid->data.size()!=16)throw std::runtime_error("Style reference GUID missing");item.hasId=true;std::copy(guid->data.begin(),guid->data.end(),item.objectId.begin());}
            if(valid&16){const auto file=unique(*descriptor,"file");if(!file)throw std::runtime_error("Style reference filename missing");item.filename=decode_utf16(file->data);}
            if(valid&4){const auto name=unique(*descriptor,"name");if(!name)throw std::runtime_error("Style reference name missing");item.name=decode_utf16(name->data);}
            if(!item.hasId&&item.filename.empty())throw std::runtime_error("Style reference needs identity or filename");
            if(result.size()>=1000)throw std::runtime_error("Style reference limit");result.push_back(std::move(item));
        }
    }
    std::stable_sort(result.begin(),result.end(),[](const StyleReference& a,const StyleReference& b){return a.time<b.time;});return result;
}
Chunk style_reference_track(std::uint32_t groups){
    if(!groups)throw std::runtime_error("Style track needs groups");
    Chunk track;track.id="RIFF";track.type="DMTK";
    Chunk header;header.id="trkh";header.data.resize(32);std::copy(styleTrack.begin(),styleTrack.end(),header.data.begin());put32(header.data,20,groups);put32(header.data,28,0x72747473);
    Chunk flags;flags.id="trkx";flags.data.resize(8);put32(flags.data,0,0x38);
    Chunk refs;refs.id="LIST";refs.type="sttr";track.children={header,flags,refs};return track;
}
std::vector<StyleReference> style_track_references(const Chunk& track){
    Chunk root;root.id="RIFF";root.type="DMSG";Chunk tracks;tracks.id="LIST";tracks.type="trkl";tracks.children.push_back(track);root.children.push_back(tracks);return style_references(root);
}
namespace {
std::vector<size_t> reference_slots(const Chunk& track){
    (void)style_track_references(track);const auto refs=track.find("LIST","sttr");if(!refs)throw std::runtime_error("Not a Style reference track");std::vector<size_t> slots;
    for(size_t i=0;i<refs->children.size();++i)if(refs->children[i].id=="LIST"&&refs->children[i].type=="strf")slots.push_back(i);
    std::stable_sort(slots.begin(),slots.end(),[&](size_t a,size_t b){return read32(refs->children[a].find("stmp")->data,0)<read32(refs->children[b].find("stmp")->data,0);});return slots;
}
bool valid_reference_edit(const StyleReference& r){
    return r.time>=0&&(r.hasId||!r.filename.empty())&&r.filename.find(L'\0')==std::wstring::npos&&r.name.find(L'\0')==std::wstring::npos;
}
void write_reference(Chunk& ref,const StyleReference& r){
    auto stamp=ref.find("stmp");if(!stamp){Chunk c;c.id="stmp";c.data.resize(4);ref.children.push_back(c);stamp=&ref.children.back();}put32(stamp->data,0,static_cast<std::uint32_t>(r.time));
    auto descriptor=ref.find("LIST","DMRF");if(!descriptor){Chunk c;c.id="LIST";c.type="DMRF";ref.children.push_back(c);descriptor=&ref.children.back();}
    auto h=descriptor->find("refh");if(!h){Chunk c;c.id="refh";c.data.resize(20);std::copy(styleClass.begin(),styleClass.end(),c.data.begin());descriptor->children.push_back(c);h=&descriptor->children.back();}
    // Only the owned identity/name/file validity bits change. Inactive bytes,
    // extensions, unrelated flags and original sibling order survive edits.
    const auto valid=(read32(h->data,16)&~std::uint32_t(1|4|16|32))|2u|(r.hasId?1u:0u)|(!r.name.empty()?4u:0u)|(!r.filename.empty()?16u:0u);put32(h->data,16,valid);
    const auto field=[&](const char* id,Bytes data){if(auto current=descriptor->find(id))current->data=std::move(data);else{Chunk fresh;fresh.id=id;fresh.data=std::move(data);descriptor->children.push_back(std::move(fresh));}};
    if(r.hasId)field("guid",Bytes(r.objectId.begin(),r.objectId.end()));if(!r.filename.empty())field("file",utf16(r.filename));if(!r.name.empty())field("name",utf16(r.name));
}
}
bool insert_style_reference(Chunk& track,const StyleReference& r){
    if(!valid_reference_edit(r))return false;auto next=track;const auto events=style_track_references(next);if(events.size()>=1000||std::any_of(events.begin(),events.end(),[&](const auto& e){return e.time==r.time;}))return false;
    Chunk ref;ref.id="LIST";ref.type="strf";write_reference(ref,r);auto refs=next.find("LIST","sttr");if(!refs)return false;refs->children.push_back(std::move(ref));(void)style_track_references(next);track=std::move(next);return true;
}
bool change_style_reference(Chunk& track,size_t index,const StyleReference& r){
    if(!valid_reference_edit(r))return false;auto next=track;const auto slots=reference_slots(next);if(index>=slots.size())return false;const auto events=style_track_references(next);
    for(size_t i=0;i<events.size();++i)if(i!=index&&events[i].time==r.time)return false;write_reference(next.find("LIST","sttr")->children[slots[index]],r);(void)style_track_references(next);if(next.encode()==track.encode())return false;track=std::move(next);return true;
}
bool delete_style_reference(Chunk& track,size_t index){auto next=track;const auto slots=reference_slots(next);if(index>=slots.size())return false;auto refs=next.find("LIST","sttr");refs->children.erase(refs->children.begin()+slots[index]);track=std::move(next);return true;}
std::vector<ResolvedStyle> resolve_styles(const std::vector<StyleReference>& references,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog,bool runtimeReferences) {
    if(references.empty())return {};if(directory.empty())throw std::runtime_error("Style references need a document directory");
    const auto base=std::filesystem::absolute(directory).lexically_normal();std::vector<ResolvedStyle> result;
    for(const auto& ref:references){
        const StyleCatalogEntry* entry=nullptr;std::filesystem::path path;Bytes bytes;
        if(ref.filename.empty()){
            for(const auto& candidate:catalog){StyleDocument style;style.load(candidate.bytes);if(style.has_object_id()&&style.object_id()==ref.objectId){if(entry)throw std::runtime_error("Ambiguous project Style GUID");entry=&candidate;}}
            if(!entry)throw std::runtime_error("GUID-only Style resolution requires a matching project catalog entry");path=entry->path;bytes=entry->bytes;
        }else{
            const auto relative=std::filesystem::path(ref.filename);if(relative.is_absolute()||relative.has_root_name())throw std::runtime_error("Style reference must be relative");path=(base/relative).lexically_normal();const auto relation=path.lexically_relative(base);if(!runtimeReferences&&(relation.empty()||*relation.begin()==L".."))throw std::runtime_error("Style reference escapes directory");
            for(const auto& candidate:catalog)if(CompareStringOrdinal(candidate.path.c_str(),-1,path.wstring().c_str(),-1,TRUE)==CSTR_EQUAL){if(entry)throw std::runtime_error("Ambiguous project Style path");entry=&candidate;}
            if(entry)bytes=entry->bytes;else{path=contained(base,ref.filename,runtimeReferences);bytes=read_file(path.wstring());}
        }
        StyleDocument style;style.load(bytes);
        if(ref.hasId&&(!style.has_object_id()||style.object_id()!=ref.objectId))throw std::runtime_error("Style dependency identity mismatch");
        result.push_back({ref,path.wstring(),bytes,style.meter()});
    }return result;
}
Bytes relocate_style_references(const Bytes& bytes,const std::vector<ResolvedStyle>& styles,const std::wstring& directory){
    auto root=Chunk::parse(bytes);const auto refs=style_references(root);
    if(refs.size()!=styles.size())throw std::runtime_error("Style relocation context count");
    const auto descriptors=style_descriptors(root);
    const auto base=std::filesystem::absolute(directory).lexically_normal();
    for(size_t i=0;i<refs.size();++i){const auto& ref=refs[i];const auto& dependency=styles[i];const auto& context=dependency.reference;
        if(ref.time!=context.time||ref.groups!=context.groups||ref.filename!=context.filename||ref.name!=context.name||ref.hasId!=context.hasId||ref.objectId!=context.objectId)throw std::runtime_error("Style relocation context mismatch");
        StyleDocument style;style.load(dependency.bytes);if(ref.hasId&&(!style.has_object_id()||style.object_id()!=ref.objectId))throw std::runtime_error("Style relocation identity mismatch");
        if(ref.filename.empty())continue;
        const auto relative=std::filesystem::absolute(dependency.path).lexically_normal().lexically_relative(base);auto& descriptor=*descriptors.at(i);
        if(relative.empty()||relative.is_absolute()||*relative.begin()==L".."){
            if(!ref.hasId)throw std::runtime_error("Filename-only Style must remain inside the Segment directory");
            auto& header=descriptor.find("refh")->data;put32(header,16,read32(header,16)&~16u);
        }else{const auto filename=relative.wstring();if(filename.size()>259)throw std::runtime_error("Style relocation filename too long");descriptor.find("file")->data=utf16(filename);}
    }
    return root.encode();
}
Bytes retarget_style_references(const Bytes& bytes,const std::wstring& directory,const std::wstring& oldPath,const std::wstring& newPath,const Bytes& targetBytes){
    auto root=Chunk::parse(bytes);const auto refs=style_references(root);const auto descriptors=style_descriptors(root);StyleDocument target;target.load(targetBytes);const auto base=std::filesystem::absolute(directory).lexically_normal();
    for(size_t i=0;i<refs.size();++i){const auto& ref=refs[i];if(ref.filename.empty())continue;const auto relative=std::filesystem::path(ref.filename);if(relative.is_absolute()||relative.has_root_name())continue;const auto source=(base/relative).lexically_normal().wstring();if(CompareStringOrdinal(source.c_str(),-1,oldPath.c_str(),-1,TRUE)!=CSTR_EQUAL)continue;
        if(ref.hasId&&(!target.has_object_id()||target.object_id()!=ref.objectId))throw std::runtime_error("Retarget Style identity mismatch");
        const auto filename=std::filesystem::absolute(newPath).lexically_normal().lexically_relative(base);auto& descriptor=*descriptors.at(i);
        if(filename.empty()||filename.is_absolute()||*filename.begin()==L".."){
            if(!ref.hasId)throw std::runtime_error("Filename-only Style destination must remain inside dependent Segment directory");auto& header=descriptor.find("refh")->data;put32(header,16,read32(header,16)&~16u);
        }else{const auto name=filename.wstring();if(name.size()>259)throw std::runtime_error("Retarget Style filename too long");descriptor.find("file")->data=utf16(name);}
    }
    return root.encode();
}
StylePlaybackSnapshot prepare_style_playback(const Bytes& bytes,const std::vector<ResolvedStyle>& styles){
    auto root=Chunk::parse(bytes);if(root.type!="DMSG")throw std::runtime_error("Playback requires a DMSG segment");const auto refs=style_references(root);if(refs.size()!=styles.size())throw std::runtime_error("Playback requires resolved Style snapshots");
    StylePlaybackSnapshot result{bytes,styles};
    auto samePath=[](const std::wstring& a,const std::wstring& b){return !a.empty()&&!b.empty()&&CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;};
    for(size_t i=0;i<refs.size();++i){const auto& ref=refs[i];const auto& context=styles[i].reference;
        if(ref.time!=context.time||ref.groups!=context.groups||ref.filename!=context.filename||ref.hasId!=context.hasId||ref.objectId!=context.objectId)throw std::runtime_error("Playback Style context does not match document");
        StyleDocument style;style.load(styles[i].bytes);if(ref.hasId&&(!style.has_object_id()||style.object_id()!=ref.objectId))throw std::runtime_error("Playback Style snapshot identity mismatch");const auto meter=style.meter();if(meter.beats!=styles[i].meter.beats||meter.denominator!=styles[i].meter.denominator||meter.grids!=styles[i].meter.grids)throw std::runtime_error("Playback Style snapshot meter mismatch");
        auto& mapped=result.styles[i];bool reused=false;
        for(size_t j=0;j<i;++j)if(samePath(styles[j].path,styles[i].path)||samePath(refs[j].filename,ref.filename)){
            if(styles[j].bytes!=styles[i].bytes)throw std::runtime_error("Conflicting Style snapshots share a source path");mapped.bytes=result.styles[j].bytes;mapped.reference.objectId=result.styles[j].reference.objectId;reused=true;break;
        }
        if(!reused){if(style.has_object_id())mapped.reference.objectId=style.object_id();else{
            GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Playback Style GUID creation failed");std::memcpy(mapped.reference.objectId.data(),&id,16);auto styleRoot=Chunk::parse(styles[i].bytes);Chunk identity;identity.id="guid";identity.data.assign(mapped.reference.objectId.begin(),mapped.reference.objectId.end());styleRoot.children.push_back(std::move(identity));mapped.bytes=styleRoot.encode();
        }}
        for(size_t j=0;j<i;++j)if(mapped.reference.objectId==result.styles[j].reference.objectId&&mapped.bytes!=result.styles[j].bytes)throw std::runtime_error("Conflicting Style snapshots share a loader GUID");
        mapped.reference.hasId=true;mapped.reference.filename.clear();
    }
    struct Descriptor {std::int32_t time;Chunk* chunk;};std::vector<Descriptor> descriptors;
    if(auto tracks=root.find("LIST","trkl"))for(auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto header=track.find("trkh");if(!header||header->data.size()<32||!std::equal(styleTrack.begin(),styleTrack.end(),header->data.begin()))continue;
        for(auto& ref:track.find("LIST","sttr")->children)if(ref.id=="LIST"&&ref.type=="strf")descriptors.push_back({static_cast<std::int32_t>(read32(ref.find("stmp")->data,0)),ref.find("LIST","DMRF")});
    }
    std::stable_sort(descriptors.begin(),descriptors.end(),[](const Descriptor& a,const Descriptor& b){return a.time<b.time;});if(descriptors.size()!=result.styles.size())throw std::runtime_error("Playback Style descriptor count");
    for(size_t i=0;i<descriptors.size();++i){auto& descriptor=*descriptors[i].chunk;auto& valid=descriptor.find("refh")->data;put32(valid,16,(read32(valid,16)|1)&~(16u|32u));auto guid=descriptor.find("guid");if(!guid){Chunk id;id.id="guid";descriptor.children.push_back(std::move(id));guid=&descriptor.children.back();}guid->data.assign(result.styles[i].reference.objectId.begin(),result.styles[i].reference.objectId.end());}
    result.segment=root.encode();return result;
}
}
