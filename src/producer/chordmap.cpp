#include "chordmap.h"
#include <windows.h>
#include <algorithm>
#include <set>
#include <stdexcept>
namespace producer::app {
bool ChordMapDocument::has_object_id() const{return root_.find("guid")!=nullptr;}
std::array<std::uint8_t,16> ChordMapDocument::object_id() const{const auto id=root_.find("guid");if(!id||id->data.size()!=16||std::count_if(root_.children.begin(),root_.children.end(),[](const Chunk& c){return c.id=="guid";})!=1)throw std::runtime_error("ChordMap identity missing, malformed or ambiguous");std::array<std::uint8_t,16> result{};std::copy(id->data.begin(),id->data.end(),result.begin());return result;}
namespace {
std::uint16_t word(const Bytes& b,size_t p){if(p>b.size()||b.size()-p<2)throw std::runtime_error("Truncated Chordmap WORD");return static_cast<std::uint16_t>(b[p]|b[p+1]<<8);}
void putword(Bytes& b,size_t p,std::uint16_t n){(void)word(b,p);b[p]=static_cast<std::uint8_t>(n);b[p+1]=static_cast<std::uint8_t>(n>>8);}
Chunk leaf(const char* id,Bytes b){Chunk c;c.id=id;c.data=std::move(b);return c;}Chunk list(const char* type,std::vector<Chunk> c={}){Chunk r;r.id="LIST";r.type=type;r.children=std::move(c);return r;}
const Chunk* unique(const Chunk& r,const char* id,const char* type=""){const Chunk* result=nullptr;for(const auto& c:r.children)if(c.id==id&&(!*type||c.type==type)){if(result)throw std::runtime_error("Ambiguous Chordmap chunk");result=&c;}return result;}
const Chunk& required(const Chunk& r,const char* id,const char* type=""){const auto c=unique(r,id,type);if(!c)throw std::runtime_error("Missing Chordmap chunk");return *c;}
Chunk& required(Chunk& r,const char* id,const char* type=""){return const_cast<Chunk&>(required(static_cast<const Chunk&>(r),id,type));}
size_t database_stride(const Chunk& r){const auto& b=required(r,"chdt").data;const auto s=word(b,0);if(s<20||(b.size()-2)%s)throw std::runtime_error("Invalid Chordmap subchord database");return s;}
ChordMapSubchord subchord(const Bytes& b,size_t p){if(p>b.size()||b.size()-p<20)throw std::runtime_error("Truncated Chordmap subchord");return {read32(b,p),read32(b,p+4),read32(b,p+8),read32(b,p+16),b[p+12],b[p+13],word(b,p+14)};}
bool valid_subchord(const ChordMapSubchord& s){return !(s.chord&0xff000000)&&!(s.scale&0xff000000)&&s.root<24&&s.scaleRoot<24;}
void patch_subchord(Bytes& b,size_t p,const ChordMapSubchord& s){put32(b,p,s.chord);put32(b,p+4,s.scale);put32(b,p+8,s.inversions);b[p+12]=s.root;b[p+13]=s.scaleRoot;putword(b,p+14,s.flags);put32(b,p+16,s.levels);}
std::vector<ChordMapSubchord> definition(const Chunk& r,const Chunk& c){const auto stride=database_stride(r);const auto& db=required(r,"chdt").data;const auto& ids=required(c,"sbcn").data;if(ids.size()%2)throw std::runtime_error("Malformed Chordmap subchord IDs");std::vector<ChordMapSubchord> result;for(size_t p=0;p<ids.size();p+=2){const auto index=word(ids,p);const auto at=2+static_cast<size_t>(index)*stride;if(at>db.size()||db.size()-at<stride)throw std::runtime_error("Chordmap subchord ID outside database");result.push_back(subchord(db,at));}return result;}
// DMPR strings are fixed/padded WCHAR buffers, not serialized path strings.
std::wstring padded_name(const Bytes& b){if(b.size()%2)throw std::runtime_error("Odd Chordmap WCHAR buffer");std::wstring n;for(size_t p=0;p<b.size();p+=2){const auto c=word(b,p);if(!c)return n;n.push_back(static_cast<wchar_t>(c));}throw std::runtime_error("Unterminated Chordmap name");}
std::wstring chord_name(const Chunk& c){return padded_name(required(c,"INAM").data);}
bool valid_name(const std::wstring& n){return !n.empty()&&n.find(L'\0')==std::wstring::npos;}
size_t connection_stride(const Bytes& b){const auto s=word(b,0);if(s<12||(b.size()-2)%s)throw std::runtime_error("Invalid Chordmap connection stride");return s;}
ChordMapConnection connection(const Bytes& b,size_t p){return {read32(b,p),word(b,p+4),word(b,p+6),word(b,p+8),word(b,p+10)};}
void patch_connection(Bytes& b,size_t p,const ChordMapConnection& c){put32(b,p,c.flags);putword(b,p+4,c.weight);putword(b,p+6,c.minBeats);putword(b,p+8,c.maxBeats);putword(b,p+10,c.destination);}
Chunk* node(Chunk& r,std::uint16_t id){for(auto& c:required(r,"LIST","cmap").children)if(c.id=="LIST"&&c.type=="choe"&&word(required(c,"cheh").data,4)==id)return &c;return nullptr;}
bool valid_connection(const Chunk& r,const ChordMapConnection& c){if(c.weight>100||c.minBeats>c.maxBeats)return false;if(!c.destination)return true;auto copy=r;return node(copy,c.destination)!=nullptr;}
bool valid_groups(std::uint32_t g){return g&&!(g&~0x7f3fu);}
std::uint16_t append_subchord(Chunk& r,const ChordMapSubchord& s,const Bytes& original={}){
    const auto stride=database_stride(r);auto& db=required(r,"chdt").data;
    const auto count=(db.size()-2)/stride;
    Bytes b=original.empty()?Bytes(stride):original;patch_subchord(b,0,s);
    // The database is shared by definitions. Reuse an identical complete
    // record, including imported extensions, without rewriting existing IDs.
    // Original Producer accepts the unique-record diagnostic but rejects its
    // duplicate-record counterpart. Sharing still permits copy-on-write edits.
    for(size_t i=0;i<std::min(count,static_cast<size_t>(UINT16_MAX)+1);++i)
        if(std::equal(b.begin(),b.end(),db.begin()+2+i*stride))
            return static_cast<std::uint16_t>(i);
    if(count>UINT16_MAX)throw std::runtime_error("Chordmap subchord IDs exhausted");
    db.insert(db.end(),b.begin(),b.end());return static_cast<std::uint16_t>(count);
}
void validate(const Chunk& r){if(r.id!="RIFF"||r.type!="DMPR")throw std::runtime_error("Expected DMPR Chordmap");if(required(r,"perh").data.size()<48)throw std::runtime_error("Truncated Chordmap header");(void)padded_name(Bytes(required(r,"perh").data.begin(),required(r,"perh").data.begin()+40));(void)database_stride(r);std::set<std::uint16_t> ids;std::vector<std::uint16_t> targets;const auto& map=required(r,"LIST","cmap");for(const auto& c:map.children)if(c.id=="LIST"&&c.type=="choe"){const auto& h=required(c,"cheh").data;if(h.size()<6||!ids.insert(word(h,4)).second)throw std::runtime_error("Duplicate or truncated Chordmap node identity");const auto& d=required(c,"LIST","chrd");(void)chord_name(d);(void)definition(r,d);const auto& b=required(c,"ncsq").data;const auto stride=connection_stride(b);for(size_t p=2;p<b.size();p+=stride)targets.push_back(connection(b,p).destination);}for(const auto id:targets)if(id&&!ids.count(id))throw std::runtime_error("Dangling Chordmap connection");
    if(const auto palette=unique(r,"LIST","chpl"))for(const auto& c:palette->children)if(c.id=="LIST"&&c.type=="chrd"){(void)chord_name(c);(void)definition(r,c);}
    const auto signs=unique(r,"LIST","spsq");if(signs)for(const auto& c:signs->children)if(c.id=="LIST"&&c.type=="spst"){if(required(c,"spsh").data.size()<8)throw std::runtime_error("Truncated Chordmap Signpost header");(void)definition(r,required(c,"LIST","chrd"));if(const auto cadence=unique(c,"LIST","cade"))for(const auto& d:cadence->children)if(d.id=="LIST"&&d.type=="chrd")(void)definition(r,d);}}
}
ChordMapDocument::ChordMapDocument(){root_.id="RIFF";root_.type="DMPR";Bytes header(48);const auto name=utf16(L"Chordmap");std::copy(name.begin(),name.end(),header.begin());put32(header,40,0x0cab5ab5);put32(header,44,1);GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create Chordmap identity");const auto p=reinterpret_cast<const std::uint8_t*>(&id);root_.children={leaf("perh",header),leaf("guid",Bytes(p,p+16)),leaf("chdt",{20,0}),list("chpl"),list("cmap"),list("spsq")};
    // Original empty CDP and Producer help specify 24 fixed roots, four layers.
    for(unsigned pitch=0;pitch<24;++pitch){Bytes ids(8),label=utf16(L"M");label.resize(24);
        for(unsigned layer=0;layer<4;++layer){ChordMapSubchord sub;sub.root=static_cast<std::uint8_t>(pitch);sub.scale=0xab5ab5;sub.levels=1u<<layer;putword(ids,layer*2,append_subchord(root_,sub));}
        required(root_,"LIST","chpl").children.push_back(list("chrd",{leaf("INAM",label),leaf("sbcn",ids),leaf("ched",Bytes(8))}));
    }
    saved_=save_bytes();}
void ChordMapDocument::load(const Bytes& bytes){auto r=Chunk::parse(bytes);validate(r);root_=std::move(r);saved_=bytes;undo_.clear();redo_.clear();}
bool ChordMapDocument::commit(Chunk next){validate(next);const auto before=save_bytes();if(next.encode()==before)return false;undo_.push_back(before);root_=std::move(next);redo_.clear();return true;}
void ChordMapDocument::save(const std::wstring& path){auto b=save_bytes();write_file_atomic(path,b);saved_=std::move(b);}
bool ChordMapDocument::undo(){if(undo_.empty())return false;auto next=Chunk::parse(undo_.back());redo_.push_back(save_bytes());root_=std::move(next);undo_.pop_back();return true;}bool ChordMapDocument::redo(){if(redo_.empty())return false;auto next=Chunk::parse(redo_.back());undo_.push_back(save_bytes());root_=std::move(next);redo_.pop_back();return true;}
std::wstring ChordMapDocument::name() const{const auto& b=required(root_,"perh").data;return padded_name(Bytes(b.begin(),b.begin()+40));}std::uint32_t ChordMapDocument::scale() const{return read32(required(root_,"perh").data,40)&0xffffff;}
bool ChordMapDocument::set_name(const std::wstring& n){if(!valid_name(n)||n.size()>19)return false;auto r=root_;auto& b=required(r,"perh").data;std::fill(b.begin(),b.begin()+40,0);const auto u=utf16(n);std::copy(u.begin(),u.end(),b.begin());return commit(std::move(r));}
bool ChordMapDocument::set_scale(std::uint32_t n){if(n&0xff000000)return false;auto r=root_;put32(required(r,"perh").data,40,(read32(required(r,"perh").data,40)&0xff000000)|n);return commit(std::move(r));}
std::vector<ChordMapNode> ChordMapDocument::nodes() const{std::vector<ChordMapNode> out;for(const auto& c:required(root_,"LIST","cmap").children)if(c.id=="LIST"&&c.type=="choe"){const auto& h=required(c,"cheh").data;const auto& d=required(c,"LIST","chrd");ChordMapNode n{word(h,4),read32(h,0),chord_name(d),definition(root_,d),{}};const auto& b=required(c,"ncsq").data;const auto stride=connection_stride(b);for(size_t p=2;p<b.size();p+=stride)n.connections.push_back(connection(b,p));out.push_back(std::move(n));}return out;}
std::vector<ChordMapSignpost> ChordMapDocument::signposts() const{std::vector<ChordMapSignpost> out;const auto signs=unique(root_,"LIST","spsq");if(signs)for(const auto& c:signs->children)if(c.id=="LIST"&&c.type=="spst"){const auto& b=required(c,"spsh").data;const auto& d=required(c,"LIST","chrd");size_t count=0;if(const auto cadence=unique(c,"LIST","cade"))for(const auto& x:cadence->children)if(x.id=="LIST"&&x.type=="chrd")++count;out.push_back({read32(b,0),read32(b,4),chord_name(d),definition(root_,d),count});}return out;}
std::vector<ChordMapPaletteChord> ChordMapDocument::palette() const {
    std::vector<ChordMapPaletteChord> out;const auto p=unique(root_,"LIST","chpl");
    if(p)for(const auto& c:p->children)if(c.id=="LIST"&&c.type=="chrd")out.push_back({chord_name(c),definition(root_,c)});
    return out;
}
namespace {
Chunk* palette_chord(Chunk& r,size_t index){auto p=const_cast<Chunk*>(unique(r,"LIST","chpl"));if(!p)return nullptr;size_t i=0;for(auto& c:p->children)if(c.id=="LIST"&&c.type=="chrd"&&i++==index)return &c;return nullptr;}
}
bool ChordMapDocument::edit_palette_subchord(size_t chord,size_t layer,const ChordMapSubchord& sub){
    if(!valid_subchord(sub))return false;auto r=root_;auto c=palette_chord(r,chord);if(!c)return false;
    auto& ids=required(*c,"sbcn").data;if(layer>=ids.size()/2)return false;
    const auto stride=database_stride(r),at=2+static_cast<size_t>(word(ids,layer*2))*stride;const auto& db=required(r,"chdt").data;
    const auto old=subchord(db,at);if(sub.root!=old.root)return false; // Palette root is permanently fixed.
    Bytes original(db.begin()+at,db.begin()+at+stride),patched=original;patch_subchord(patched,0,sub);if(patched==original)return false;
    putword(ids,layer*2,append_subchord(r,sub,original));return commit(std::move(r));
}
bool ChordMapDocument::insert_palette_node(size_t chord,std::uint16_t* output){
    if(unique(root_,"ceed"))throw std::runtime_error("Imported Chordmap design-node insertion requires ceed layout contract");
    auto r=root_;auto c=palette_chord(r,chord);if(!c)return false;auto copied=*c;
    std::uint32_t id=1;while(id<=UINT16_MAX&&node(r,static_cast<std::uint16_t>(id)))++id;if(id>UINT16_MAX)return false;
    Bytes h(8);putword(h,4,static_cast<std::uint16_t>(id));required(r,"LIST","cmap").children.push_back(list("choe",{leaf("cheh",h),std::move(copied),leaf("ncsq",{12,0})}));
    if(!commit(std::move(r)))return false;if(output)*output=static_cast<std::uint16_t>(id);return true;
}
bool ChordMapDocument::insert_node(const std::wstring& name,const ChordMapSubchord& sub,std::uint16_t* output){if(!valid_name(name)||!valid_subchord(sub))return false;if(unique(root_,"ceed"))throw std::runtime_error("Imported Chordmap design-node insertion requires ceed layout contract");auto r=root_;std::uint32_t id=1;while(id<=UINT16_MAX&&node(r,static_cast<std::uint16_t>(id)))++id;if(id>UINT16_MAX)return false;const auto subid=append_subchord(r,sub);Bytes h(8),ids(2);putword(h,4,static_cast<std::uint16_t>(id));putword(ids,0,subid);required(r,"LIST","cmap").children.push_back(list("choe",{leaf("cheh",h),list("chrd",{leaf("INAM",utf16(name)),leaf("sbcn",ids)}),leaf("ncsq",{12,0})}));if(!commit(std::move(r)))return false;if(output)*output=static_cast<std::uint16_t>(id);return true;}
bool ChordMapDocument::rename_node(std::uint16_t id,const std::wstring& name){if(!valid_name(name))return false;auto r=root_;auto n=node(r,id);if(!n)return false;auto& b=required(required(*n,"LIST","chrd"),"INAM").data;auto u=utf16(name);if(u.size()<b.size())u.resize(b.size());b=std::move(u);return commit(std::move(r));}
bool ChordMapDocument::edit_subchord(std::uint16_t id,size_t index,const ChordMapSubchord& sub){if(!valid_subchord(sub))return false;auto r=root_;auto n=node(r,id);if(!n)return false;auto& ids=required(required(*n,"LIST","chrd"),"sbcn").data;if(index>=ids.size()/2)return false;const auto stride=database_stride(r),at=2+static_cast<size_t>(word(ids,index*2))*stride;const auto& db=required(r,"chdt").data;Bytes original(db.begin()+at,db.begin()+at+stride),patched=original;patch_subchord(patched,0,sub);if(patched==original)return false;const auto replacement=append_subchord(r,sub,original);putword(ids,index*2,replacement);return commit(std::move(r));}
bool ChordMapDocument::delete_node(std::uint16_t id){if(unique(root_,"ceed"))throw std::runtime_error("Imported Chordmap design-node deletion requires ceed layout contract");auto r=root_;if(!node(r,id))return false;auto& entries=required(r,"LIST","cmap").children;entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const Chunk& c){return c.id=="LIST"&&c.type=="choe"&&word(required(c,"cheh").data,4)==id;}),entries.end());for(auto& c:entries)if(c.id=="LIST"&&c.type=="choe"){auto& b=required(c,"ncsq").data;const auto s=connection_stride(b);Bytes next(b.begin(),b.begin()+2);for(size_t p=2;p<b.size();p+=s)if(connection(b,p).destination!=id)next.insert(next.end(),b.begin()+p,b.begin()+p+s);b=std::move(next);}return commit(std::move(r));}
bool ChordMapDocument::set_connection(std::uint16_t id,size_t index,const ChordMapConnection& c){if(!valid_connection(root_,c))return false;auto r=root_;auto n=node(r,id);if(!n)return false;auto& b=required(*n,"ncsq").data;const auto s=connection_stride(b);if(index>=(b.size()-2)/s)return false;patch_connection(b,2+index*s,c);return commit(std::move(r));}
bool ChordMapDocument::insert_connection(std::uint16_t id,const ChordMapConnection& c){if(!valid_connection(root_,c))return false;auto r=root_;auto n=node(r,id);if(!n)return false;auto& b=required(*n,"ncsq").data;const auto s=connection_stride(b),p=b.size();b.resize(p+s);patch_connection(b,p,c);return commit(std::move(r));}
bool ChordMapDocument::delete_connection(std::uint16_t id,size_t index){auto r=root_;auto n=node(r,id);if(!n)return false;auto& b=required(*n,"ncsq").data;const auto s=connection_stride(b);if(index>=(b.size()-2)/s)return false;b.erase(b.begin()+2+index*s,b.begin()+2+(index+1)*s);return commit(std::move(r));}
bool ChordMapDocument::insert_signpost(std::uint16_t id,std::uint32_t groups){if(!valid_groups(groups))return false;auto r=root_;auto n=node(r,id);if(!n)return false;Bytes h(8);put32(h,0,groups);if(!unique(r,"LIST","spsq"))r.children.push_back(list("spsq"));required(r,"LIST","spsq").children.push_back(list("spst",{leaf("spsh",h),required(*n,"LIST","chrd")}));return commit(std::move(r));}
bool ChordMapDocument::set_signpost_groups(size_t index,std::uint32_t groups){if(!valid_groups(groups))return false;if(!unique(root_,"LIST","spsq"))return false;auto r=root_;size_t i=0;for(auto& c:required(r,"LIST","spsq").children)if(c.id=="LIST"&&c.type=="spst")if(i++==index){put32(required(c,"spsh").data,0,groups);return commit(std::move(r));}return false;}
bool ChordMapDocument::delete_signpost(size_t index){if(!unique(root_,"LIST","spsq"))return false;auto r=root_;auto& signs=required(r,"LIST","spsq").children;size_t i=0;for(auto p=signs.begin();p!=signs.end();++p)if(p->id=="LIST"&&p->type=="spst")if(i++==index){signs.erase(p);return commit(std::move(r));}return false;}
}
