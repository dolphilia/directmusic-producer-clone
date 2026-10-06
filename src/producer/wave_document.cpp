#include "wave_document.h"
#include <windows.h>
#include <algorithm>
#include <stdexcept>
namespace producer::app {
namespace {
const Chunk* unique(const Chunk& c,const char* id,const char* type=""){
    const Chunk* result=nullptr;for(const auto& x:c.children)if(x.id==id&&(type[0]==0||x.type==type)){if(result)throw std::runtime_error("Duplicate Wave chunk");result=&x;}return result;
}
unsigned word(const Bytes& b,size_t n){if(n+2>b.size())throw std::runtime_error("Wave field truncated");return b[n]|(unsigned(b[n+1])<<8);}
std::vector<size_t> loop_offsets(const Chunk& root){
    const auto sample=unique(root,"wsmp");if(!sample)return {};
    const auto& b=sample->data;if(b.size()<20)throw std::runtime_error("Wave sample header truncated");
    size_t at=read32(b,0);const auto count=read32(b,16);
    if(at<20||at>b.size()||count>(b.size()-at)/16)throw std::runtime_error("Wave sample loop layout invalid");
    std::vector<size_t> offsets;
    for(size_t i=0;i<count;++i){if(b.size()-at<16)throw std::runtime_error("Wave sample loop truncated");const auto size=read32(b,at);if(size<16||size>b.size()-at)throw std::runtime_error("Wave sample loop stride invalid");offsets.push_back(at);at+=size;}
    return offsets;
}
WaveFormat format(const Chunk& c){const auto f=unique(c,"fmt "),d=unique(c,"data");if(!f||f->data.size()<16||!d)throw std::runtime_error("Wave format or audio data missing");const auto& b=f->data;WaveFormat v{word(b,0),word(b,2),read32(b,4),word(b,12),word(b,14),0};if(!v.channels||!v.sampleRate||!v.blockAlign)throw std::runtime_error("Invalid Wave format");
    if(v.tag==1){if(!v.bits||v.bits%8||v.blockAlign!=v.channels*(v.bits/8)||std::uint64_t(v.sampleRate)*v.blockAlign!=read32(b,8)||d->data.size()%v.blockAlign)throw std::runtime_error("Invalid PCM Wave layout");v.frames=d->data.size()/v.blockAlign;}
    else if(const auto fact=unique(c,"fact")){if(fact->data.size()<4)throw std::runtime_error("Wave fact truncated");v.frames=read32(fact->data,0);}return v;
}
void validate(const Chunk& c){if(c.id!="RIFF"||c.type!="WAVE")throw std::runtime_error("Expected Wave document");(void)format(c);if(const auto g=unique(c,"guid"))if(g->data.size()!=16)throw std::runtime_error("Wave GUID size");if(const auto h=unique(c,"wavh"))if(h->data.size()<12)throw std::runtime_error("Wave Producer header truncated");if(const auto s=unique(c,"wsmp")){if(s->data.size()<20||read32(s->data,0)<20||read32(s->data,0)>s->data.size()||word(s->data,4)>127)throw std::runtime_error("Wave sample header invalid");}if(const auto s=unique(c,"smpl"))if(s->data.size()<36||read32(s->data,12)>127)throw std::runtime_error("Wave sampler header invalid");}
}
void WaveDocument::load(const Bytes& b){auto next=Chunk::parse(b);validate(next);root_=std::move(next);saved_=b;undo_.clear();redo_.clear();}
void WaveDocument::save(const std::wstring& p){const auto b=save_bytes();write_file_atomic(p,b);saved_=b;}
WaveFormat WaveDocument::format()const{return producer::app::format(root_);}
std::optional<std::array<std::uint8_t,16>> WaveDocument::identity()const{const auto g=unique(root_,"guid");if(!g)return {};std::array<std::uint8_t,16> id{};std::copy(g->data.begin(),g->data.end(),id.begin());return id;}
std::wstring WaveDocument::name()const{if(const auto u=unique(root_,"LIST","UNFO"))if(const auto n=unique(*u,"UNAM"))return decode_utf16(n->data);if(const auto i=unique(root_,"LIST","INFO"))if(const auto n=unique(*i,"INAM")){const auto end=std::find(n->data.begin(),n->data.end(),0);const int count=static_cast<int>(end-n->data.begin());if(!count)return {};const auto src=reinterpret_cast<const char*>(n->data.data());const int size=MultiByteToWideChar(CP_ACP,0,src,count,nullptr,0);if(!size)throw std::runtime_error("Wave name decode failed");std::wstring value(size,0);MultiByteToWideChar(CP_ACP,0,src,count,value.data(),size);return value;}return {};}
unsigned WaveDocument::root_note()const{if(const auto s=unique(root_,"wsmp"))return word(s->data,4);if(const auto s=unique(root_,"smpl"))return read32(s->data,12);return 60;}
bool WaveDocument::commit(Chunk n){validate(n);const auto before=save_bytes();if(n.encode()==before)return false;undo_.push_back(before);redo_.clear();root_=std::move(n);return true;}
bool WaveDocument::set_root_note(unsigned value){if(value>127)return false;auto n=root_;auto s=n.find("wsmp");if(!s){Chunk c;c.id="wsmp";c.data.resize(20);put32(c.data,0,20);put32(c.data,12,1);n.children.push_back(c);s=&n.children.back();}s->data[4]=static_cast<std::uint8_t>(value);s->data[5]=0;if(auto sampler=n.find("smpl"))put32(sampler->data,12,value);return commit(std::move(n));}
bool WaveDocument::set_name(const std::wstring& value){if(value.size()>255||value.find(wchar_t(0))!=std::wstring::npos)return false;BOOL substituted=FALSE;const int size=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,&substituted);if(substituted||(!value.empty()&&!size))return false;Bytes text(size+1,0);if(size){WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,value.data(),static_cast<int>(value.size()),reinterpret_cast<char*>(text.data()),size,nullptr,&substituted);if(substituted)return false;}auto n=root_;auto info=n.find("LIST","INFO");if(!info){Chunk c;c.id="LIST";c.type="INFO";n.children.push_back(c);info=&n.children.back();}auto name=info->find("INAM");if(!name){Chunk c;c.id="INAM";info->children.push_back(c);name=&info->children.back();}name->data=std::move(text);if(auto u=n.find("LIST","UNFO"))if(auto un=u->find("UNAM"))un->data=utf16(value);return commit(std::move(n));}
bool WaveDocument::set_properties(const std::wstring& value,unsigned note){if(note>127)return false;auto next=*this;if(value!=next.name()&&!next.set_name(value))return false;if(note!=next.root_note()&&!next.set_root_note(note))return false;return commit(std::move(next.root_));}
std::vector<WaveSampleLoop> WaveDocument::sample_loops()const{
    const auto offsets=loop_offsets(root_);std::vector<WaveSampleLoop> result;
    if(offsets.empty())return result;const auto& b=unique(root_,"wsmp")->data;
    for(const auto at:offsets)result.push_back({read32(b,at+4),read32(b,at+8),read32(b,at+12)});
    return result;
}
bool WaveDocument::set_sample_loops(const std::vector<WaveSampleLoop>& loops){
    const auto f=format();if(f.tag!=1)throw std::runtime_error("Wave sample loop editing requires owned PCM frames");
    if(loops.size()>UINT32_MAX)throw std::runtime_error("Wave sample loop count capacity");
    for(const auto& loop:loops)if(loop.type>1||!loop.length||static_cast<std::uint64_t>(loop.start)+loop.length>f.frames)throw std::runtime_error("Wave loop type must be 0 or 1 and start + length must fit PCM frames");
    const auto offsets=loop_offsets(root_);if(loops==sample_loops())return false;
    auto next=root_;auto sample=next.find("wsmp");
    if(!sample){Chunk c;c.id="wsmp";c.data.resize(20);put32(c.data,0,20);c.data[4]=static_cast<std::uint8_t>(root_note());put32(c.data,12,1);next.children.push_back(c);sample=&next.children.back();}
    const auto& old=sample->data;const auto header=read32(old,0);
    const auto end=offsets.empty()?header:offsets.back()+read32(old,offsets.back());
    Bytes b(old.begin(),old.begin()+header);put32(b,16,static_cast<std::uint32_t>(loops.size()));
    for(size_t i=0;i<loops.size();++i){const auto at=b.size();if(i<offsets.size()){const auto from=offsets[i];const auto size=read32(old,from);b.insert(b.end(),old.begin()+from,old.begin()+from+size);}else{b.resize(at+16,0);put32(b,at,16);}put32(b,at+4,loops[i].type);put32(b,at+8,loops[i].start);put32(b,at+12,loops[i].length);}
    b.insert(b.end(),old.begin()+end,old.end());sample->data=std::move(b);
    // SMPL is an independent sampler contract. Do not convert its inclusive
    // endpoints/types or replace cue/fraction/repeat metadata implicitly.
    return commit(std::move(next));
}
bool WaveDocument::remove_sample_loop(size_t index){
    const auto offsets=loop_offsets(root_);if(index>=offsets.size())return false;
    if(format().tag!=1)throw std::runtime_error("Wave sample loop editing requires owned PCM frames");
    auto next=root_;auto& b=next.find("wsmp")->data;const auto at=offsets[index],size=static_cast<size_t>(read32(b,at));
    b.erase(b.begin()+at,b.begin()+at+size);put32(b,16,static_cast<std::uint32_t>(offsets.size()-1));
    return commit(std::move(next));
}
bool WaveDocument::undo(){if(undo_.empty())return false;redo_.push_back(save_bytes());root_=Chunk::parse(undo_.back());undo_.pop_back();return true;}
bool WaveDocument::redo(){if(redo_.empty())return false;undo_.push_back(save_bytes());root_=Chunk::parse(redo_.back());redo_.pop_back();return true;}
}
