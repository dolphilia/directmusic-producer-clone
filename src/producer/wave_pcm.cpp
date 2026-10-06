#include "wave_document.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <cstring>
namespace producer::app {namespace {
Chunk* single(Chunk& c,const char* id,const char* type=""){
    Chunk* found=nullptr;for(auto& x:c.children)if(x.id==id&&(!type[0]||x.type==type)){if(found)throw std::runtime_error("Ambiguous Wave position metadata");found=&x;}return found;
}
struct Edit {
    std::uint64_t begin,end,inserted,frames;
    std::uint64_t point(std::uint64_t x)const {
        if(x>frames)throw std::runtime_error("Wave position exceeds source frames");
        if(inserted)return x>=begin?x+inserted:x;
        return x<=begin?x:x<end?begin:x-(end-begin);
    }
    std::pair<std::uint64_t,std::uint64_t> range(std::uint64_t a,std::uint64_t b)const {
        if(a>b||b>frames)throw std::runtime_error("Wave region exceeds source frames");
        if(a==b)return {point(a),point(b)};
        // Insertion at the start precedes the region; insertion at its end
        // follows it. Insertion strictly inside expands the existing region.
        return inserted?std::make_pair(a>=begin?a+inserted:a,b>begin?b+inserted:b):std::make_pair(point(a),point(b));
    }
};
std::uint32_t checked(std::uint64_t n){if(n>UINT32_MAX)throw std::runtime_error("Wave sample position capacity exceeded");return static_cast<std::uint32_t>(n);}
void positions(Chunk& root,const Edit& edit){
    if(single(root,"plst"))throw std::runtime_error("PCM edits of playlist-order Wave need a playlist contract");
    if(auto fact=single(root,"fact")){if(fact->data.size()<4)throw std::runtime_error("Wave fact truncated");put32(fact->data,0,checked(edit.frames-(edit.end-edit.begin)+edit.inserted));}
    std::map<std::uint32_t,std::uint64_t> cues;
    if(auto cue=single(root,"cue ")){
        auto& b=cue->data;if(b.size()<4)throw std::runtime_error("Wave cue header truncated");const auto count=read32(b,0);if(count>(b.size()-4)/24)throw std::runtime_error("Wave cue records truncated");
        for(size_t i=0;i<count;++i){const size_t p=4+i*24;const auto id=read32(b,p),offset=read32(b,p+20);if(!cues.emplace(id,offset).second)throw std::runtime_error("Duplicate Wave cue identity");if(std::memcmp(b.data()+p+8,"data",4)||read32(b,p+12)||read32(b,p+16))throw std::runtime_error("Wave cue is not a contiguous PCM data position");put32(b,p+4,checked(edit.point(read32(b,p+4))));put32(b,p+20,checked(edit.point(offset)));}
    }
    if(auto labels=single(root,"LIST","adtl"))for(auto& item:labels->children)if(item.id=="ltxt"){
        auto& b=item.data;if(b.size()<20)throw std::runtime_error("Wave labelled region truncated");const auto cue=cues.find(read32(b,0));if(cue==cues.end())throw std::runtime_error("Wave labelled region has no cue");const auto a=cue->second,z=a+read32(b,4);const auto r=edit.range(a,z);put32(b,4,checked(r.second-r.first));
    }
    if(auto sample=single(root,"wsmp")){
        const auto old=sample->data;if(old.size()<20)throw std::runtime_error("WSMP header truncated");const auto header=read32(old,0),count=read32(old,16);if(header<20||header>old.size()||count>(old.size()-header)/16)throw std::runtime_error("WSMP loop layout invalid");
        Bytes b(old.begin(),old.begin()+header);size_t at=header;std::uint32_t kept=0;
        for(size_t i=0;i<count;++i){if(old.size()-at<16)throw std::runtime_error("WSMP loop truncated");const auto size=read32(old,at);if(size<16||size>old.size()-at)throw std::runtime_error("WSMP loop stride invalid");const auto a=read32(old,at+8),len=read32(old,at+12);if(!len)throw std::runtime_error("Empty WSMP loop");const auto r=edit.range(a,std::uint64_t(a)+len);if(r.second>r.first){const auto p=b.size();b.insert(b.end(),old.begin()+at,old.begin()+at+size);put32(b,p+8,checked(r.first));put32(b,p+12,checked(r.second-r.first));++kept;}at+=size;}
        b.insert(b.end(),old.begin()+at,old.end());put32(b,16,kept);sample->data=std::move(b);
    }
    if(auto sample=single(root,"smpl")){
        const auto old=sample->data;if(old.size()<36)throw std::runtime_error("SMPL header truncated");const auto count=read32(old,28);if(count>(old.size()-36)/24)throw std::runtime_error("SMPL loop records truncated");const size_t tail=36+size_t(count)*24;if(read32(old,32)>old.size()-tail)throw std::runtime_error("SMPL sampler data truncated");Bytes b(old.begin(),old.begin()+36);std::uint32_t kept=0;
        for(size_t i=0;i<count;++i){const size_t p=36+i*24;const auto r=edit.range(read32(old,p+8),std::uint64_t(read32(old,p+12))+1);if(r.second>r.first){const auto q=b.size();b.insert(b.end(),old.begin()+p,old.begin()+p+24);put32(b,q+8,checked(r.first));put32(b,q+12,checked(r.second-1));++kept;}}
        b.insert(b.end(),old.begin()+tail,old.end());put32(b,28,kept);sample->data=std::move(b);
    }
}
}
Bytes WaveDocument::copy_pcm(std::uint64_t begin,std::uint64_t end)const {
    const auto f=format();if(f.tag!=1)throw std::runtime_error("PCM frame editing requires uncompressed Wave");if(begin>end||end>f.frames)throw std::runtime_error("PCM selection outside Wave");
    Chunk clip;clip.id="RIFF";clip.type="WAVE";clip.children.push_back(*root_.find("fmt "));Chunk data;data.id="data";const auto& source=root_.find("data")->data;data.data.assign(source.begin()+static_cast<size_t>(begin*f.blockAlign),source.begin()+static_cast<size_t>(end*f.blockAlign));clip.children.push_back(std::move(data));return clip.encode();
}
bool WaveDocument::erase_pcm(std::uint64_t begin,std::uint64_t end){
    const auto f=format();if(f.tag!=1)throw std::runtime_error("PCM frame editing requires uncompressed Wave");if(begin>end||end>f.frames)throw std::runtime_error("PCM selection outside Wave");if(begin==end)return false;
    auto next=root_;positions(next,{begin,end,0,f.frames});auto& b=next.find("data")->data;b.erase(b.begin()+static_cast<size_t>(begin*f.blockAlign),b.begin()+static_cast<size_t>(end*f.blockAlign));return commit(std::move(next));
}
bool WaveDocument::paste_pcm(std::uint64_t insertion,const Bytes& wave){
    const auto f=format();WaveDocument source;source.load(wave);const auto sf=source.format();if(f.tag!=1||sf.tag!=1||f.channels!=sf.channels||f.sampleRate!=sf.sampleRate||f.blockAlign!=sf.blockAlign||f.bits!=sf.bits)throw std::runtime_error("PCM paste requires matching uncompressed channel rate and sample format");if(insertion>f.frames)throw std::runtime_error("PCM insertion outside Wave");if(!sf.frames)return false;
    const auto& payload=source.root_.find("data")->data;auto next=root_;auto& b=next.find("data")->data;if(payload.size()>b.max_size()-b.size()||f.frames>UINT32_MAX||sf.frames>UINT32_MAX-f.frames)throw std::runtime_error("PCM paste exceeds Wave capacity");positions(next,{insertion,insertion,sf.frames,f.frames});b.insert(b.begin()+static_cast<size_t>(insertion*f.blockAlign),payload.begin(),payload.end());return commit(std::move(next));
}
}
