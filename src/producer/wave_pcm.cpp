#include "wave_document.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <cstring>
#include <cmath>
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
std::uint64_t WaveDocument::nearest_zero_frame(std::uint64_t frame)const {
    const auto f=format();
    if(f.tag!=1||(f.bits!=8&&f.bits!=16&&f.bits!=24&&f.bits!=32))throw std::runtime_error("Snap to Zero requires 8, 16, 24 or 32-bit PCM");
    if(frame>f.frames)throw std::runtime_error("Snap position outside Wave");
    if(!f.frames)return frame;
    const auto& data=root_.find("data")->data;
    const auto sample=[&](std::uint64_t at,unsigned channel){
        const auto p=static_cast<size_t>(at*f.blockAlign+channel*(f.bits/8));
        if(f.bits==8)return std::int64_t(data[p])-128;
        std::int64_t value=0;for(unsigned j=0;j<f.bits/8;++j)value|=std::int64_t(data[p+j])<<(j*8);
        if(value&(std::int64_t(1)<<(f.bits-1)))value-=std::int64_t(1)<<f.bits;
        return value;
    };
    const auto zero=[&](std::uint64_t at){
        for(unsigned c=0;c<f.channels;++c){
            if(!at){if(sample(0,c))return false;}
            else if(at==f.frames){if(sample(at-1,c))return false;}
            else{const auto a=sample(at-1,c),b=sample(at,c);if(a&&b&&((a<0)==(b<0)))return false;}
        }
        return true;
    };
    // Search both sides at equal distance; the earlier frame wins a tie.
    for(std::uint64_t distance=0;distance<=std::max(frame,f.frames-frame);++distance){
        if(distance<=frame&&zero(frame-distance))return frame-distance;
        if(distance&&distance<=f.frames-frame&&zero(frame+distance))return frame+distance;
    }
    return frame;
}
bool WaveDocument::find_best_sample_loop_end(size_t index,std::uint64_t begin,std::uint64_t end){
    const auto f=format();auto loops=sample_loops();
    if(f.tag!=1||(f.bits!=8&&f.bits!=16&&f.bits!=24&&f.bits!=32))throw std::runtime_error("Find Best Loop End requires 8, 16, 24 or 32-bit PCM");
    if(index>=loops.size())throw std::runtime_error("Select an existing loop to refine");
    const auto loop=loops[index];
    if(loop.type>1||!loop.length||std::uint64_t(loop.start)+loop.length>f.frames)throw std::runtime_error("Selected loop is outside Wave");
    if(begin>=end||end>f.frames)throw std::runtime_error("Select a nonempty search region inside Wave");
    const auto first=std::max(std::uint64_t(loop.start)+1,begin+1);
    if(first>end)throw std::runtime_error("Search region must contain a sample after the loop start");
    const auto& data=root_.find("data")->data;
    const auto sample=[&](std::uint64_t at,unsigned channel){
        const auto p=static_cast<size_t>(at*f.blockAlign+channel*(f.bits/8));
        std::int64_t value=f.bits==8?std::int64_t(data[p])-128:0;
        if(f.bits!=8){for(unsigned j=0;j<f.bits/8;++j)value|=std::int64_t(data[p+j])<<(j*8);if(value&(std::int64_t(1)<<(f.bits-1)))value-=std::int64_t(1)<<f.bits;}
        return std::ldexp(static_cast<double>(value),1-static_cast<int>(f.bits));
    };
    // Compare the two samples surrounding the seam in every channel. This
    // deterministic heuristic is not a claim about the original search algorithm.
    std::vector<std::pair<double,double>> target;
    for(unsigned c=0;c<f.channels;++c){const auto head=sample(loop.start,c);target.push_back({loop.start?sample(loop.start-1,c):f.frames>1?2*head-sample(1,c):head,head});}
    const auto cost=[&](std::uint64_t at){
        double score=0;
        for(unsigned c=0;c<f.channels;++c){const auto tail=sample(at-1,c),head=at<f.frames?sample(at,c):at>1?2*tail-sample(at-2,c):tail;const auto a=tail-target[c].first,b=head-target[c].second;score+=a*a+b*b;}
        return score;
    };
    auto best=first;auto bestCost=cost(first);
    for(auto at=first+1;at<=end;++at){const auto score=cost(at);if(score<bestCost){best=at;bestCost=score;}}
    loops[index].length=checked(best-loop.start);
    return set_sample_loops(loops);
}
bool WaveDocument::clone_sample_loop(size_t index){
    const auto f=format();const auto loops=sample_loops();
    if(f.tag!=1)throw std::runtime_error("Clone Loop requires uncompressed PCM");
    if(index>=loops.size())throw std::runtime_error("Select an existing loop to clone");
    const auto loop=loops[index];
    if(loop.type>1||!loop.length||std::uint64_t(loop.start)+loop.length>f.frames)throw std::runtime_error("Selected loop is outside Wave");
    const auto origin=cloneOrigin_&&cloneOrigin_->index==index?*cloneOrigin_:CloneOrigin{index,loop};
    if(origin.loop.start!=loop.start||origin.loop.type!=loop.type||origin.loop.length>loop.length)throw std::runtime_error("Clone Loop origin changed");
    const auto insertion=std::uint64_t(loop.start)+loop.length,added=std::uint64_t(origin.loop.length);
    if(f.frames>UINT32_MAX||added>UINT32_MAX-f.frames||added>UINT32_MAX-loop.length)throw std::runtime_error("Clone Loop exceeds Wave capacity");
    const auto clip=Chunk::parse(copy_pcm(origin.loop.start,std::uint64_t(origin.loop.start)+added));
    const auto& payload=clip.find("data")->data;auto next=root_;
    positions(next,{insertion,insertion,added,f.frames});
    auto& data=next.find("data")->data;
    if(payload.size()>data.max_size()-data.size())throw std::runtime_error("Clone Loop exceeds PCM capacity");
    data.insert(data.begin()+static_cast<size_t>(insertion*f.blockAlign),payload.begin(),payload.end());
    // Ordinary insertion at a region's end follows that region. Clone Loop
    // explicitly includes the copy in this loop; other metadata keeps its mapping.
    auto& wsmp=next.find("wsmp")->data;size_t at=read32(wsmp,0);
    for(size_t i=0;i<index;++i)at+=read32(wsmp,at);
    put32(wsmp,at+12,checked(std::uint64_t(loop.length)+added));
    return commit(std::move(next),origin);
}
}
