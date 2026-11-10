#include "container_graph.h"
#include <algorithm>
#include <stdexcept>
#include <windows.h>
#include <filesystem>
namespace producer::app { namespace {
const Chunk* one(const Chunk& c,const char* id,const char* type=""){
    const Chunk* found=nullptr;
    for(const auto& x:c.children)if(x.id==id&&(!*type||x.type==type)){
        if(found)throw std::runtime_error("Ambiguous Container field");found=&x;
    }
    return found;
}
bool valid_text(const std::wstring& s){
    for(size_t i=0;i<s.size();++i){auto x=static_cast<unsigned>(s[i]);
        if(!x)return false;
        if(x>=0xd800&&x<=0xdbff){if(++i==s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}
        else if(x>=0xdc00&&x<=0xdfff)return false;
    }return true;
}
std::wstring checked_text(const Chunk& c){auto s=decode_utf16(c.data);if(!valid_text(s))throw std::runtime_error("Invalid Container Unicode");return s;}
ContainedObject object(const Chunk& c){
    const auto h=one(c,"cobh"),a=one(c,"coba");
    if(!h||h->data.size()<28)throw std::runtime_error("Missing or truncated Container object header");
    ContainedObject out;std::copy_n(h->data.begin(),16,out.classId.begin());out.flags=read32(h->data,16);
    if(a)out.alias=checked_text(*a);
    const std::string id(h->data.begin()+20,h->data.begin()+24),type(h->data.begin()+24,h->data.begin()+28);
    const Chunk* payload=nullptr;
    for(const auto& child:c.children){
        // A zero ckid selects the list type, as allowed by the SDK header.
        const bool match=id==std::string(4,'\0')?(child.container()&&child.type==type):(child.id==id&&(!child.container()||child.type==type));
        if(match){if(payload)throw std::runtime_error("Ambiguous Container object payload");payload=&child;}
    }
    if(!payload)throw std::runtime_error("Container object payload does not match header");
    out.payload=*payload;out.reference=payload->id=="LIST"&&payload->type=="DMRF";
    out.referenceRuntime=out.reference;
    if(const auto membership=one(c,"cobu")){
        if(membership->data.empty())throw std::runtime_error("Truncated Container runtime membership");
        out.referenceRuntime=(membership->data[0]&1)!=0;
    }
    if(const auto association=one(c,"jzfr")){
        if(association->data.size()<32)throw std::runtime_error("Truncated Container Project association");
        out.projectAssociation=association->data;
    }
    if(out.reference){
        const auto r=one(*payload,"refh");if(!r||r->data.size()<20||!std::equal(out.classId.begin(),out.classId.end(),r->data.begin()))throw std::runtime_error("Container reference class mismatch");
        if(auto g=one(*payload,"guid"))if(g->data.size()!=16)throw std::runtime_error("Container reference GUID size");
        if(auto v=one(*payload,"vers"))if(v->data.size()!=8)throw std::runtime_error("Container reference version size");
        for(auto id2:{"file","name","catg"})if(auto t=one(*payload,id2))(void)checked_text(*t);
    }
    return out;
}
void validate(const Chunk& c){
    if(c.id!="RIFF"||c.type!="DMCN")throw std::runtime_error("Expected Container DMCN");
    const auto h=one(c,"conh"),list=one(c,"LIST","cosl");
    if(!h||h->data.size()<4||!list)throw std::runtime_error("Container header/object list missing");
    if(auto g=one(c,"guid"))if(g->data.size()!=16)throw std::runtime_error("Container GUID size");
    if(auto v=one(c,"vers"))if(v->data.size()!=8)throw std::runtime_error("Container version size");
    if(auto u=one(c,"LIST","UNFO"))if(auto n=one(*u,"UNAM"))(void)checked_text(*n);
    for(const auto& o:list->children)if(o.id=="LIST"&&o.type=="cobl")(void)object(o);
}
}
ContainerGraph::ContainerGraph(const Bytes& bytes):root_(Chunk::parse(bytes)){validate(root_);}
std::uint32_t ContainerGraph::flags()const{return read32(one(root_,"conh")->data,0);}
std::vector<ContainedObject> ContainerGraph::objects()const{std::vector<ContainedObject> out;for(const auto& c:one(root_,"LIST","cosl")->children)if(c.id=="LIST"&&c.type=="cobl")out.push_back(object(c));return out;}
bool ContainerGraph::set_no_loads(bool value){const auto before=save_bytes();put32(root_.find("conh")->data,0,(flags()&~2u)|(value?2u:0u));return save_bytes()!=before;}
bool ContainerGraph::set_object_properties(size_t index,const std::optional<std::wstring>& alias,bool keep){
    if(alias&&!valid_text(*alias))return false;
    if(alias&&!alias->empty()){const auto entries=objects();for(size_t i=0;i<entries.size();++i)if(i!=index&&entries[i].alias&&CompareStringOrdinal(alias->c_str(),-1,entries[i].alias->c_str(),-1,TRUE)==CSTR_EQUAL)return false;}
    auto next=root_;auto list=next.find("LIST","cosl");Chunk* selected=nullptr;
    for(auto& c:list->children)if(c.id=="LIST"&&c.type=="cobl"){if(!index){selected=&c;break;}--index;}
    if(!selected)return false;auto h=selected->find("cobh");put32(h->data,16,(read32(h->data,16)&~1u)|(keep?1u:0u));
    if(alias){auto a=selected->find("coba");if(!a){Chunk added;added.id="coba";selected->children.insert(selected->children.begin(),added);a=&selected->children.front();}a->data=utf16(*alias);}
    else selected->children.erase(std::remove_if(selected->children.begin(),selected->children.end(),[](const auto& c){return c.id=="coba";}),selected->children.end());
    validate(next);if(next.encode()==save_bytes())return false;root_=std::move(next);return true;
}

bool ContainerGraph::add_segment_reference(const std::array<std::uint8_t,16>& objectId,const std::array<std::uint8_t,16>& projectFileId,const std::wstring& filename,const std::wstring& alias,bool keep){
    const std::array<std::uint8_t,16> cls={0x82,0x28,0xac,0xd2,0x9b,0xb3,0xd1,0x11,0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd};
    const std::array<std::uint8_t,16> docType={0x09,0x86,0xce,0xdf,0xfa,0xa6,0xd1,0x11,0x88,0x81,0x00,0xc0,0x4f,0xbf,0x8d,0x15};
    const auto nonzero=[](const auto& id){return std::any_of(id.begin(),id.end(),[](auto b){return b!=0;});};
    if(!nonzero(objectId)||!nonzero(projectFileId)||alias.empty()||alias.size()>255||!valid_text(alias)||filename.empty()||filename.size()>255||!valid_text(filename)||filename.find_first_of(L"/\\:<>|?*\"")!=std::wstring::npos)return false;
    const auto path=std::filesystem::path(filename);if(path.filename()!=path||CompareStringOrdinal(path.extension().c_str(),-1,L".sgp",-1,TRUE)!=CSTR_EQUAL)return false;
    for(const auto& old:objects()){
        if(old.alias&&CompareStringOrdinal(alias.c_str(),-1,old.alias->c_str(),-1,TRUE)==CSTR_EQUAL)return false;
        if(old.classId==cls){auto guid=old.payload.find("guid");if(guid&&guid->data==Bytes(objectId.begin(),objectId.end()))return false;
            if(auto file=old.payload.find("file"))if(CompareStringOrdinal(filename.c_str(),-1,decode_utf16(file->data).c_str(),-1,TRUE)==CSTR_EQUAL)return false;}
    }
    Chunk entry;entry.id="LIST";entry.type="cobl";
    Chunk a;a.id="coba";a.data=utf16(alias);entry.children.push_back(a);
    Chunk h;h.id="cobh";h.data.resize(28);std::copy(cls.begin(),cls.end(),h.data.begin());put32(h.data,16,keep?1:0);std::copy_n("LIST",4,h.data.begin()+20);std::copy_n("DMRF",4,h.data.begin()+24);entry.children.push_back(h);
    Chunk membership;membership.id="cobu";membership.data={1,0};entry.children.push_back(membership);
    Chunk ref;ref.id="LIST";ref.type="DMRF";Chunk rh;rh.id="refh";rh.data.resize(20);std::copy(cls.begin(),cls.end(),rh.data.begin());put32(rh.data,16,0x13);ref.children.push_back(rh);
    Chunk guid;guid.id="guid";guid.data.assign(objectId.begin(),objectId.end());ref.children.push_back(guid);Chunk file;file.id="file";file.data=utf16(filename);ref.children.push_back(file);entry.children.push_back(ref);
    Chunk association;association.id="jzfr";association.data.assign(projectFileId.begin(),projectFileId.end());association.data.insert(association.data.end(),docType.begin(),docType.end());entry.children.push_back(association);
    auto next=root_;next.find("LIST","cosl")->children.push_back(entry);validate(next);root_=std::move(next);return true;
}
bool ContainerGraph::remove_object(size_t index){auto next=root_;auto& entries=next.find("LIST","cosl")->children;for(auto i=entries.begin();i!=entries.end();++i)if(i->id=="LIST"&&i->type=="cobl"){if(!index){entries.erase(i);validate(next);root_=std::move(next);return true;}--index;}return false;}
bool ContainerGraph::set_reference_runtime(size_t index,bool value){
    auto next=root_;for(auto& entry:next.find("LIST","cosl")->children)if(entry.id=="LIST"&&entry.type=="cobl"){
        if(index){--index;continue;}
        const auto selected=object(entry);
        // A runtime payload has no design filename/Project association to turn
        // into a new reference. Retain it until that ownership can be supplied.
        if(!selected.reference||selected.referenceRuntime==value)return false;
        auto membership=entry.find("cobu");if(!membership){Chunk c;c.id="cobu";c.data={0,0};entry.children.push_back(c);membership=&entry.children.back();}
        membership->data[0]=(membership->data[0]&~1u)|(value?1u:0u);
        validate(next);root_=std::move(next);return true;
    }return false;
}
Bytes ContainerGraph::runtime_bytes(const std::function<Chunk(const ContainedObject&)>& resolveEmbedded)const{
    auto next=root_;for(auto& entry:next.find("LIST","cosl")->children)if(entry.id=="LIST"&&entry.type=="cobl"){
        const auto selected=object(entry);
        if(selected.reference&&!selected.referenceRuntime){
            auto payload=resolveEmbedded(selected);
            if(payload.id!="RIFF")throw std::runtime_error("Embedded runtime object must be RIFF");
            auto h=entry.find("cobh");std::copy_n(payload.id.data(),4,h->data.begin()+20);std::copy_n(payload.type.data(),4,h->data.begin()+24);
            for(auto& child:entry.children)if(child.id==selected.payload.id&&child.type==selected.payload.type){child=std::move(payload);break;}
        }
        entry.children.erase(std::remove_if(entry.children.begin(),entry.children.end(),[](const auto& c){return c.id=="cobu"||c.id=="jzfr";}),entry.children.end());
    }validate(next);return next.encode();
}
}
