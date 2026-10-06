#include "container_document.h"
#include <windows.h>
#include <objbase.h>
#include <cstring>
#include <stdexcept>
namespace producer::app {
ContainerDocument::ContainerDocument(){
    Chunk root;root.id="RIFF";root.type="DMCN";
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Container identity creation failed");
    Chunk guid;guid.id="guid";guid.data.resize(16);std::memcpy(guid.data.data(),&id,16);root.children.push_back(guid);
    Chunk header;header.id="conh";header.data.resize(4);root.children.push_back(header);
    Chunk list;list.id="LIST";list.type="cosl";root.children.push_back(list);
    bytes_=root.encode();saved_=bytes_;
}
void ContainerDocument::load(const Bytes& b){(void)ContainerGraph(b);bytes_=b;saved_=b;undo_.clear();redo_.clear();}
void ContainerDocument::save(const std::wstring& p){write_file_atomic(p,bytes_);saved_=bytes_;}
bool ContainerDocument::commit(Bytes next){(void)ContainerGraph(next);if(next==bytes_)return false;undo_.push_back(bytes_);redo_.clear();bytes_=std::move(next);return true;}
std::wstring ContainerDocument::name()const{auto root=Chunk::parse(bytes_);if(auto info=root.find("LIST","UNFO"))if(auto name=info->find("UNAM"))return decode_utf16(name->data);return {};}
bool ContainerDocument::set_name(const std::wstring& s){
    if(s.size()>255)return false;
    for(size_t i=0;i<s.size();++i){auto c=static_cast<unsigned>(s[i]);if(!c)return false;if(c>=0xd800&&c<=0xdbff){if(++i==s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}else if(c>=0xdc00&&c<=0xdfff)return false;}
    auto root=Chunk::parse(bytes_);auto info=root.find("LIST","UNFO");if(!info){Chunk c;c.id="LIST";c.type="UNFO";root.children.push_back(c);info=&root.children.back();}auto n=info->find("UNAM");if(!n){Chunk c;c.id="UNAM";info->children.push_back(c);n=&info->children.back();}n->data=utf16(s);return commit(root.encode());
}
bool ContainerDocument::set_no_loads(bool v){auto g=graph();return g.set_no_loads(v)&&commit(g.save_bytes());}
bool ContainerDocument::set_object_properties(size_t i,const std::optional<std::wstring>& a,bool k){auto g=graph();return g.set_object_properties(i,a,k)&&commit(g.save_bytes());}
bool ContainerDocument::set_reference_runtime(size_t i,bool v){auto g=graph();return g.set_reference_runtime(i,v)&&commit(g.save_bytes());}
bool ContainerDocument::add_segment_reference(const std::array<std::uint8_t,16>& id,const std::array<std::uint8_t,16>& file,const std::wstring& name,const std::wstring& alias,bool keep){auto g=graph();return g.add_segment_reference(id,file,name,alias,keep)&&commit(g.save_bytes());}
bool ContainerDocument::remove_object(size_t i){auto g=graph();return g.remove_object(i)&&commit(g.save_bytes());}
bool ContainerDocument::undo(){if(undo_.empty())return false;redo_.push_back(bytes_);bytes_=std::move(undo_.back());undo_.pop_back();return true;}
bool ContainerDocument::redo(){if(redo_.empty())return false;undo_.push_back(bytes_);bytes_=std::move(redo_.back());redo_.pop_back();return true;}
}
