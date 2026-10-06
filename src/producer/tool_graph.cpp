#include "tool_graph.h"
#include <windows.h>
#include <objbase.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace producer::app {namespace {
const Chunk* one(const Chunk& c,const char* id,const char* type=""){const Chunk* found=nullptr;for(const auto& x:c.children)if(x.id==id&&(!*type||x.type==type)){if(found)throw std::runtime_error("Duplicate ToolGraph field");found=&x;}return found;}
bool valid_text(const std::wstring& s){for(size_t i=0;i<s.size();++i){const auto v=static_cast<unsigned>(s[i]);if(!v)return false;if(v>=0xd800&&v<=0xdbff){if(++i>=s.size()||s[i]<0xdc00||s[i]>0xdfff)return false;}else if(v>=0xdc00&&v<=0xdfff)return false;}return true;}
GraphTool tool(const Chunk& c){
    const auto h=one(c,"tolh");if(!h||h->data.size()<32)throw std::runtime_error("Missing/truncated Tool header");
    const auto count=read32(h->data,20);if(count>(h->data.size()-32)/4)throw std::runtime_error("Truncated Tool PChannel array");
    GraphTool out;std::copy_n(h->data.begin(),16,out.classId.begin());const auto raw=read32(h->data,16);std::memcpy(&out.index,&raw,4);
    for(size_t i=0;i<count;++i)out.channels.push_back(read32(h->data,32+4*i));
    const std::string id(h->data.begin()+24,h->data.begin()+28),type(h->data.begin()+28,h->data.begin()+32),zero(4,'\0');
    if(id==zero&&type==zero)throw std::runtime_error("Tool data identifier missing");
    for(const auto& x:c.children){if(&x==h)continue;const bool match=id==zero?(x.container()&&x.type==type):(x.id==id&&(!x.container()||x.type==type));if(match){if(out.payload)throw std::runtime_error("Ambiguous Tool payload");out.payload=x;}}
    return out; // SDK explicitly permits a Tool form without its optional data chunk.
}
void validate(const Chunk& c){if(c.id!="RIFF"||c.type!="DMTG")throw std::runtime_error("Expected ToolGraph DMTG");const auto list=one(c,"LIST","toll");if(!list)throw std::runtime_error("Missing ToolGraph tool list");if(auto g=one(c,"guid"))if(g->data.size()!=16)throw std::runtime_error("ToolGraph GUID size");if(auto v=one(c,"vers"))if(v->data.size()!=8)throw std::runtime_error("ToolGraph version size");if(auto u=one(c,"LIST","UNFO"))if(auto n=one(*u,"UNAM"))if(!valid_text(decode_utf16(n->data)))throw std::runtime_error("ToolGraph Unicode");for(const auto& x:list->children)if(x.id=="RIFF"&&x.type=="DMTL")(void)tool(x);}
std::vector<size_t> slots(const Chunk& list){std::vector<size_t> out;for(size_t i=0;i<list.children.size();++i)if(list.children[i].id=="RIFF"&&list.children[i].type=="DMTL")out.push_back(i);return out;}
void renumber(Chunk& list){std::uint32_t index=0;for(auto& x:list.children)if(x.id=="RIFF"&&x.type=="DMTL")put32(x.find("tolh")->data,16,index++);}
}
ToolGraphDocument::ToolGraphDocument(){root_.id="RIFF";root_.type="DMTG";GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("ToolGraph identity creation failed");Chunk g;g.id="guid";g.data.resize(16);std::memcpy(g.data.data(),&id,16);root_.children.push_back(g);Chunk list;list.id="LIST";list.type="toll";root_.children.push_back(list);saved_=save_bytes();}
void ToolGraphDocument::load(const Bytes& b){auto next=Chunk::parse(b);validate(next);root_=std::move(next);saved_=b;undo_.clear();redo_.clear();}
void ToolGraphDocument::save(const std::wstring& p){auto b=save_bytes();write_file_atomic(p,b);saved_=std::move(b);}
bool ToolGraphDocument::commit(Chunk n){validate(n);auto before=save_bytes();if(n.encode()==before)return false;undo_.push_back(std::move(before));redo_.clear();root_=std::move(n);return true;}
std::wstring ToolGraphDocument::name()const{if(auto u=one(root_,"LIST","UNFO"))if(auto n=one(*u,"UNAM"))return decode_utf16(n->data);return {};}
bool ToolGraphDocument::set_name(const std::wstring& name){if(!valid_text(name))return false;auto n=root_;auto u=n.find("LIST","UNFO");if(!u){Chunk info;info.id="LIST";info.type="UNFO";n.children.push_back(info);u=&n.children.back();}auto item=u->find("UNAM");if(!item){Chunk text;text.id="UNAM";u->children.push_back(text);item=&u->children.back();}item->data=utf16(name);return commit(std::move(n));}
std::vector<GraphTool> ToolGraphDocument::tools()const{std::vector<GraphTool> out;for(const auto& c:one(root_,"LIST","toll")->children)if(c.id=="RIFF"&&c.type=="DMTL")out.push_back(tool(c));return out;}
bool ToolGraphDocument::set_channels(size_t index,const std::vector<std::uint32_t>& values){if(values.size()>(std::numeric_limits<std::uint32_t>::max()-32u)/4u)return false;auto n=root_;auto list=n.find("LIST","toll");const auto positions=slots(*list);if(index>=positions.size())return false;auto h=list->children[positions[index]].find("tolh");const auto count=read32(h->data,20);Bytes data(h->data.begin(),h->data.begin()+32);put32(data,20,static_cast<std::uint32_t>(values.size()));for(auto v:values){const auto offset=data.size();data.resize(offset+4);put32(data,offset,v);}data.insert(data.end(),h->data.begin()+32+4*count,h->data.end());h->data=std::move(data);return commit(std::move(n));}
bool ToolGraphDocument::add_tool(const std::array<std::uint8_t,16>& cls,const std::vector<std::uint32_t>& values){if(values.size()>(std::numeric_limits<std::uint32_t>::max()-32u)/4u||std::all_of(cls.begin(),cls.end(),[](auto v){return !v;}))return false;auto n=root_;auto list=n.find("LIST","toll");Chunk item;item.id="RIFF";item.type="DMTL";Chunk h;h.id="tolh";h.data.resize(32+4*values.size());std::copy(cls.begin(),cls.end(),h.data.begin());put32(h.data,16,static_cast<std::uint32_t>(slots(*list).size()));put32(h.data,20,static_cast<std::uint32_t>(values.size()));std::copy_n("data",4,h.data.begin()+24);for(size_t i=0;i<values.size();++i)put32(h.data,32+4*i,values[i]);item.children.push_back(h);list->children.push_back(item);return commit(std::move(n));}
bool ToolGraphDocument::remove_tool(size_t index){auto n=root_;auto list=n.find("LIST","toll");const auto positions=slots(*list);if(index>=positions.size())return false;list->children.erase(list->children.begin()+positions[index]);renumber(*list);return commit(std::move(n));}
bool ToolGraphDocument::move_tool(size_t from,size_t to){auto n=root_;auto list=n.find("LIST","toll");const auto positions=slots(*list);if(from>=positions.size()||to>=positions.size()||from==to)return false;auto moved=list->children[positions[from]];if(from<to)for(size_t i=from;i<to;++i)list->children[positions[i]]=list->children[positions[i+1]];else for(size_t i=from;i>to;--i)list->children[positions[i]]=list->children[positions[i-1]];list->children[positions[to]]=std::move(moved);renumber(*list);return commit(std::move(n));}
bool ToolGraphDocument::undo(){if(undo_.empty())return false;redo_.push_back(save_bytes());root_=Chunk::parse(undo_.back());undo_.pop_back();return true;}
bool ToolGraphDocument::redo(){if(redo_.empty())return false;undo_.push_back(save_bytes());root_=Chunk::parse(redo_.back());redo_.pop_back();return true;}
}
