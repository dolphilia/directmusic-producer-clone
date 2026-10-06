#include "script_document.h"
#include <windows.h>
#include <objbase.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
const Chunk* single(const Chunk& c,const char* id,const char* type=""){
    const Chunk* r=nullptr;for(const auto& x:c.children)if(x.id==id&&(!*type||x.type==type)){if(r)throw std::runtime_error("Duplicate Script field");r=&x;}return r;
}
bool text_valid(const std::wstring& s){
    if(s.size()>1048576)return false;
    for(size_t i=0;i<s.size();++i){const auto v=static_cast<unsigned>(s[i]);if(!v)return false;if(v>=0xd800&&v<=0xdbff){if(++i>=s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}else if(v>=0xdc00&&v<=0xdfff)return false;}
    return true;
}
std::wstring text(const Chunk* c){if(!c)throw std::runtime_error("Missing Script text");auto s=decode_utf16(c->data);if(!text_valid(s))throw std::runtime_error("Invalid Script Unicode text");return s;}
void validate(const Chunk& c){
    if(c.id!="RIFF"||c.type!="DMSC")throw std::runtime_error("Expected Script DMSC");
    const auto h=single(c,"schd"),v=single(c,"scve"),lang=single(c,"scla"),src=single(c,"scsr"),ref=single(c,"LIST","DMRF"),container=single(c,"RIFF","DMCN");
    if(!h||h->data.size()<4||!v||v->data.size()!=8||!container)throw std::runtime_error("Script header/version/container missing or truncated");
    (void)ContainerGraph(container->encode());
    if(text(lang).empty())throw std::runtime_error("Empty Script language");
    if(bool(src)==bool(ref))throw std::runtime_error("Script requires one source representation");
    if(src)(void)text(src);
    if(ref){const auto rh=single(*ref,"refh");if(!rh||rh->data.size()<20||std::any_of(rh->data.begin(),rh->data.begin()+16,[](auto b){return b!=0;}))throw std::runtime_error("Script source reference class must be GUID_NULL");const auto file=single(*ref,"file");if(file)(void)text(file);}
    if(auto g=single(c,"guid"))if(g->data.size()!=16)throw std::runtime_error("Script GUID size");
    if(auto ver=single(c,"vers"))if(ver->data.size()!=8)throw std::runtime_error("Script version size");
    if(auto u=single(c,"LIST","UNFO"))if(auto n=single(*u,"UNAM"))(void)text(n);
}
void replace_text(Chunk& c,const char* id,const std::wstring& s){auto item=c.find(id);if(!item){Chunk n;n.id=id;c.children.push_back(n);item=&c.children.back();}item->data=utf16(s);}
}
ScriptDocument::ScriptDocument(){
    root_.id="RIFF";root_.type="DMSC";
    Chunk h;h.id="schd";h.data.resize(4);root_.children.push_back(h);
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Script identity creation failed");Chunk g;g.id="guid";g.data.resize(16);std::memcpy(g.data.data(),&id,16);root_.children.push_back(g);
    Chunk v;v.id="scve";v.data.resize(8);put32(v.data,0,0x80000);root_.children.push_back(v);
    Chunk container;container.id="RIFF";container.type="DMCN";Chunk ch;ch.id="conh";ch.data.resize(4);container.children.push_back(ch);Chunk list;list.id="LIST";list.type="cosl";container.children.push_back(list);root_.children.push_back(container);
    replace_text(root_,"scla",L"VBScript");replace_text(root_,"scsr",L"");saved_=save_bytes();
}
void ScriptDocument::load(const Bytes& b){auto next=Chunk::parse(b);validate(next);root_=std::move(next);saved_=b;undo_.clear();redo_.clear();}
void ScriptDocument::save(const std::wstring& path){auto b=save_bytes();write_file_atomic(path,b);saved_=std::move(b);}
std::wstring ScriptDocument::name()const{if(auto u=single(root_,"LIST","UNFO"))if(auto n=single(*u,"UNAM"))return text(n);return {};}
std::wstring ScriptDocument::language()const{return text(single(root_,"scla"));}
std::optional<std::wstring> ScriptDocument::source()const{if(auto s=single(root_,"scsr"))return text(s);return {};}
Bytes ScriptDocument::source_reference()const{if(auto r=single(root_,"LIST","DMRF"))return r->encode();return {};}
Bytes ScriptDocument::container_bytes()const{return single(root_,"RIFF","DMCN")->encode();}
std::uint32_t ScriptDocument::flags()const{return read32(single(root_,"schd")->data,0);}
bool ScriptDocument::commit(Chunk n){validate(n);auto before=save_bytes();if(n.encode()==before)return false;undo_.push_back(std::move(before));redo_.clear();root_=std::move(n);return true;}
bool ScriptDocument::set_properties(const std::wstring& name,const std::wstring& lang,bool loadAll,bool downloadAll){
    if(name.size()>255||lang.empty()||lang.size()>255||!text_valid(name)||!text_valid(lang))return false;
    auto next=root_;auto u=next.find("LIST","UNFO");if(!u){Chunk n;n.id="LIST";n.type="UNFO";next.children.push_back(n);u=&next.children.back();}replace_text(*u,"UNAM",name);replace_text(next,"scla",lang);auto h=next.find("schd");put32(h->data,0,(flags()&~3u)|(loadAll?1u:0u)|(downloadAll?2u:0u));return commit(std::move(next));
}
bool ScriptDocument::set_source(const std::wstring& s){if(!text_valid(s))return false;auto next=root_;next.children.erase(std::remove_if(next.children.begin(),next.children.end(),[](const auto& c){return c.id=="LIST"&&c.type=="DMRF";}),next.children.end());replace_text(next,"scsr",s);return commit(std::move(next));}
bool ScriptDocument::set_container_no_loads(bool value){auto graph=container();if(!graph.set_no_loads(value))return false;auto next=root_;*next.find("RIFF","DMCN")=Chunk::parse(graph.save_bytes());return commit(std::move(next));}
bool ScriptDocument::set_contained_properties(size_t index,const std::optional<std::wstring>& alias,bool keep){auto graph=container();if(!graph.set_object_properties(index,alias,keep))return false;auto next=root_;*next.find("RIFF","DMCN")=Chunk::parse(graph.save_bytes());return commit(std::move(next));}
bool ScriptDocument::add_segment_reference(const std::array<std::uint8_t,16>& objectId,const std::array<std::uint8_t,16>& projectFileId,const std::wstring& filename,const std::wstring& alias,bool keep){auto graph=container();if(!graph.add_segment_reference(objectId,projectFileId,filename,alias,keep))return false;auto next=root_;*next.find("RIFF","DMCN")=Chunk::parse(graph.save_bytes());return commit(std::move(next));}
bool ScriptDocument::remove_contained_object(size_t index){auto graph=container();if(!graph.remove_object(index))return false;auto next=root_;*next.find("RIFF","DMCN")=Chunk::parse(graph.save_bytes());return commit(std::move(next));}
bool ScriptDocument::undo(){if(undo_.empty())return false;redo_.push_back(save_bytes());root_=Chunk::parse(undo_.back());undo_.pop_back();return true;}
bool ScriptDocument::redo(){if(redo_.empty())return false;undo_.push_back(save_bytes());root_=Chunk::parse(redo_.back());redo_.pop_back();return true;}
}
