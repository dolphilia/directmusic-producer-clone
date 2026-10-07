#include "dls.h"
#include <algorithm>
#include <stdexcept>
#include <objbase.h>
#include <cstring>

namespace producer::app {
namespace {
unsigned word(const Bytes& b,size_t at){if(at+2>b.size())throw std::runtime_error("Truncated DLS field");return b[at]|(static_cast<unsigned>(b[at+1])<<8);}
void putword(Bytes& b,size_t at,unsigned v){if(at+2>b.size())throw std::runtime_error("Truncated DLS field");b[at]=static_cast<std::uint8_t>(v);b[at+1]=static_cast<std::uint8_t>(v>>8);}
template<class C> auto& unique(C& c,const std::string& id,const std::string& type=""){auto found=c.find(id,type);if(!found||std::count_if(c.children.begin(),c.children.end(),[&](const Chunk& x){return x.id==id&&(type.empty()||x.type==type);})!=1)throw std::runtime_error("Missing or ambiguous DLS chunk: "+id+type);return *found;}
template<class C> auto& item(C& list,size_t index,const std::string& type){for(auto& c:list.children)if(c.id=="LIST"&&(c.type==type||(type=="rgn "&&c.type=="rgn2"))){if(index==0)return c;--index;}throw std::runtime_error("DLS item index out of range");}
size_t count(const Chunk& list,const std::string& type){return static_cast<size_t>(std::count_if(list.children.begin(),list.children.end(),[&](const Chunk& c){return c.id=="LIST"&&(c.type==type||(type=="rgn "&&c.type=="rgn2"));}));}
bool range(unsigned low,unsigned high){return low<=high&&high<=127;}
bool locale(std::uint32_t bank,std::uint32_t program){return (bank&~0x80007f7fu)==0&&program<=127;}
template<class C> auto& articulation_owner(C& root,size_t instrument,std::optional<size_t> region){
    auto& ins=item(unique(root,"LIST","lins"),instrument,"ins ");
    return region?item(unique(ins,"LIST","lrgn"),*region,"rgn "):ins;
}
template<class C> auto articulation_chunks(C& owner){
    using Pointer=decltype(&owner);std::vector<Pointer> result;
    for(auto& list:owner.children)if(list.id=="LIST"&&(list.type=="lart"||list.type=="lar2"))
        for(auto& c:list.children)if(c.id=="art1"||c.id=="art2")result.push_back(&c);
    return result;
}
std::vector<DlsConnection> connections(const Bytes& b){
    if(b.size()<8)throw std::runtime_error("DLS articulation header truncated");
    const size_t header=read32(b,0),n=read32(b,4);
    if(header<8||header>b.size()||n>(b.size()-header)/12)throw std::runtime_error("DLS articulation connection layout invalid");
    std::vector<DlsConnection> result;
    for(size_t i=0;i<n;++i){const auto at=header+i*12;const auto raw=read32(b,at+8);std::int32_t scale;std::memcpy(&scale,&raw,4);
        result.push_back({static_cast<std::uint16_t>(word(b,at)),static_cast<std::uint16_t>(word(b,at+2)),static_cast<std::uint16_t>(word(b,at+4)),static_cast<std::uint16_t>(word(b,at+6)),scale});}
    return result;
}
void replace_connections(Bytes& b,const std::vector<DlsConnection>& values){
    const auto old=connections(b);const size_t header=read32(b,0),end=header+old.size()*12;
    if(values.size()>UINT32_MAX||values.size()>(SIZE_MAX-header-(b.size()-end))/12)throw std::runtime_error("DLS articulation capacity");
    Bytes next(b.begin(),b.begin()+header);put32(next,4,static_cast<std::uint32_t>(values.size()));
    for(const auto& c:values){const auto at=next.size();next.resize(at+12);putword(next,at,c.source);putword(next,at+2,c.control);putword(next,at+4,c.destination);putword(next,at+6,c.transform);put32(next,at+8,static_cast<std::uint32_t>(c.scale));}
    next.insert(next.end(),b.begin()+end,b.end());b=std::move(next);
}
void pcm_format(const Bytes& b){
    if(b.size()<16||word(b,0)!=1||!word(b,2)||(word(b,14)!=8&&word(b,14)!=16)||!read32(b,4))throw std::runtime_error("WAV requires 8/16-bit integer PCM");
    const auto align=word(b,2)*(word(b,14)/8);const auto rate=static_cast<std::uint64_t>(read32(b,4))*align;
    if(align>65535||word(b,12)!=align||rate>UINT32_MAX||read32(b,8)!=rate)throw std::runtime_error("Invalid PCM block alignment or byte rate");
}
std::vector<size_t> cues(const Chunk& root){const auto& table=unique(root,"ptbl").data;const auto size=read32(table,0),n=read32(table,4);if(size<8||size>table.size()||n>(table.size()-size)/4)throw std::runtime_error("Invalid DLS pool table");const auto& pool=unique(root,"LIST","wvpl");std::vector<size_t> offsets;size_t at=0;for(const auto& c:pool.children){if(c.id=="LIST"&&c.type=="wave")offsets.push_back(at);at+=c.encode().size();}std::vector<size_t> result;for(size_t i=0;i<n;++i){const auto offset=read32(table,size+i*4);const auto found=std::find(offsets.begin(),offsets.end(),offset);if(found==offsets.end())throw std::runtime_error("DLS pool cue does not identify a wave");result.push_back(static_cast<size_t>(found-offsets.begin()));}return result;}
std::vector<size_t> loop_offsets(const Chunk& owner){
    if(!owner.find("wsmp"))return {};
    const auto& b=unique(owner,"wsmp").data;if(b.size()<20)throw std::runtime_error("DLS sample header truncated");
    size_t at=read32(b,0);const auto n=read32(b,16);if(at<20||at>b.size()||n>(b.size()-at)/16)throw std::runtime_error("DLS loop layout invalid");
    std::vector<size_t> result;for(size_t i=0;i<n;++i){if(b.size()-at<16)throw std::runtime_error("DLS loop truncated");const auto size=read32(b,at);if(size<16||size>b.size()-at)throw std::runtime_error("DLS loop stride invalid");result.push_back(at);at+=size;}return result;
}
std::vector<DlsLoop> sample_loops(const Chunk& owner){
    const auto offsets=loop_offsets(owner);std::vector<DlsLoop> result;for(const auto at:offsets){const auto& b=unique(owner,"wsmp").data;result.push_back({read32(b,at+4),read32(b,at+8),read32(b,at+12)});}return result;
}
void loop_bounds(const Chunk& owner,size_t frames){
    if(owner.find("wsmp")){
        const auto& b=unique(owner,"wsmp").data;if(b.size()<20)throw std::runtime_error("DLS sample header truncated");
        // WSMPL::usUnityNote is a MIDI playback note, including for one-shot
        // samples. Validate before adopting defaults or publishing playback;
        // lossless load/save of an invalid source remains possible.
        if(word(b,4)>127)throw std::runtime_error("DLS sample unity note outside MIDI range");
        size_t at=read32(b,0);const auto n=read32(b,16);if(at<20||at>b.size()||n>(b.size()-at)/16)throw std::runtime_error("DLS loop layout invalid");
        for(size_t i=0;i<n;++i){if(b.size()-at<16)throw std::runtime_error("DLS loop truncated");const auto size=read32(b,at),type=read32(b,at+4),start=read32(b,at+8),length=read32(b,at+12);
            if(size<16||size>b.size()-at||type>1||!length||static_cast<std::uint64_t>(start)+length>frames)throw std::runtime_error("PCM resize would invalidate DLS loop");at+=size;}
    }
    if(owner.find("smpl")){
        const auto& b=unique(owner,"smpl").data;if(b.size()<36)throw std::runtime_error("Wave sampler header truncated");const auto n=read32(b,28);
        if(n>(b.size()-36)/24||read32(b,32)>b.size()-36-static_cast<size_t>(n)*24)throw std::runtime_error("Wave sampler layout invalid");
        for(size_t i=0;i<n;++i){const auto at=36+i*24;const auto start=read32(b,at+8),end=read32(b,at+12);
            // SMPL's end sample is inclusive, unlike WSMP's start + length.
            if(read32(b,at+4)>2||start>end||end>=frames)throw std::runtime_error("PCM resize would invalidate Wave sampler loop");}
    }
}
bool edit_loop(Chunk& owner,size_t index,const DlsLoop& value,size_t frames){
    const auto offsets=loop_offsets(owner);if(index>=offsets.size())return false;
    if(value.type>1||!value.length||static_cast<std::uint64_t>(value.start)+value.length>frames)throw std::runtime_error("DLS loop exceeds sample frames or has unsupported type");
    auto& b=unique(owner,"wsmp").data;const auto at=offsets[index];put32(b,at+4,value.type);put32(b,at+8,value.start);put32(b,at+12,value.length);
    // Validate every remaining loop and independent SMPL metadata. Never
    // silently rewrite sampler or Producer extensions to match this WSMP.
    loop_bounds(owner,frames);return true;
}
void replace_loops(Chunk& owner,const std::vector<DlsLoop>& loops,size_t frames,const Chunk* defaults=nullptr){
    if(loops.size()>UINT32_MAX)throw std::runtime_error("DLS loop count capacity");
    for(const auto& l:loops)if(l.type>1||!l.length||static_cast<std::uint64_t>(l.start)+l.length>frames)throw std::runtime_error("DLS loop exceeds sample frames or has unsupported type");
    if(!owner.find("wsmp")){
        if(defaults&&defaults->find("wsmp"))owner.children.push_back(unique(*defaults,"wsmp"));
        else{Chunk sample;sample.id="wsmp";sample.data=Bytes(20,0);put32(sample.data,0,20);putword(sample.data,4,60);owner.children.push_back(std::move(sample));}
    }
    const auto offsets=loop_offsets(owner);auto& b=unique(owner,"wsmp").data;const auto header=read32(b,0);
    const auto end=offsets.empty()?header:offsets.back()+read32(b,offsets.back());
    Bytes next(b.begin(),b.begin()+header);put32(next,16,static_cast<std::uint32_t>(loops.size()));
    for(size_t i=0;i<loops.size();++i){const auto at=next.size();if(i<offsets.size()){const auto old=offsets[i];const auto size=read32(b,old);next.insert(next.end(),b.begin()+old,b.begin()+old+size);}else{next.resize(at+16,0);put32(next,at,16);}put32(next,at+4,loops[i].type);put32(next,at+8,loops[i].start);put32(next,at+12,loops[i].length);}
    next.insert(next.end(),b.begin()+end,b.end());b=std::move(next);loop_bounds(owner,frames);
}
void relocate_pool(Chunk& root,const std::vector<size_t>& mapping){
    const auto& pool=unique(root,"LIST","wvpl");std::vector<size_t> offsets;size_t at=0;for(const auto& c:pool.children){if(c.id=="LIST"&&c.type=="wave")offsets.push_back(at);at+=c.encode().size();}
    auto& table=unique(root,"ptbl").data;const auto header=read32(table,0);for(size_t i=0;i<mapping.size();++i){const auto offset=offsets.at(mapping[i]);if(offset>UINT32_MAX)throw std::runtime_error("DLS pool offset capacity");put32(table,header+i*4,static_cast<std::uint32_t>(offset));}
}
void resize_bounds(const Chunk& root,size_t index,size_t frames,const std::vector<size_t>& mapping){
    const auto& wave=item(unique(root,"LIST","wvpl"),index,"wave");loop_bounds(wave,frames);
    if(wave.find("wavh")&&unique(wave,"wavh").data.size()<12)throw std::runtime_error("Producer Wave header truncated");
    // These optional PCM metadata layouts carry sample counts/positions and
    // require their own editing contract before accepting a length change.
    for(const auto id:{"fact","cue ","plst"})if(wave.find(id))throw std::runtime_error("PCM resize metadata not supported: "+std::string(id));
    const auto& instruments=unique(root,"LIST","lins");
    for(size_t i=0;i<count(instruments,"ins ");++i){const auto& regions=unique(item(instruments,i,"ins "),"LIST","lrgn");
        for(size_t j=0;j<count(regions,"rgn ");++j){const auto& region=item(regions,j,"rgn ");const auto cue=read32(unique(region,"wlnk").data,8);if(mapping.at(cue)==index)loop_bounds(region,frames);}}
}
}
DlsDocument DlsDocument::create(){
    DlsDocument document;auto& root=document.root_;root.id="RIFF";root.type="DLS ";
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Cannot create DLS document identity");
    Chunk id;id.id="dlid";id.data.resize(16);std::memcpy(id.data.data(),&identity,16);root.children.push_back(id);
    Chunk count;count.id="colh";count.data.resize(4);root.children.push_back(count);
    Chunk version;version.id="vers";version.data.resize(8);put32(version.data,0,0x10000);put32(version.data,4,1);root.children.push_back(version);
    Chunk instruments;instruments.id="LIST";instruments.type="lins";root.children.push_back(instruments);
    Chunk table;table.id="ptbl";table.data.resize(8);put32(table.data,0,8);root.children.push_back(table);
    Chunk waves;waves.id="LIST";waves.type="wvpl";root.children.push_back(waves);
    Chunk info;info.id="LIST";info.type="INFO";
    for(const auto name:{"ICMT","ICOP","IENG","INAM","ISBJ"}){Chunk field;field.id=name;field.data={0};if(field.id=="INAM"){const std::string value="DLS Collection1";field.data.assign(value.begin(),value.end());field.data.push_back(0);}info.children.push_back(field);}
    root.children.push_back(info);return document;
}
void DlsDocument::load(const Bytes& b){auto next=Chunk::parse(b);if(next.type!="DLS ")throw std::runtime_error("Expected DLS collection");root_=std::move(next);saved_=b;undo_.clear();redo_.clear();}
void DlsDocument::save(const std::wstring& path){const auto b=save_bytes();write_file_atomic(path,b);saved_=b;}
std::vector<DlsArticulation> DlsDocument::articulations(size_t instrument,std::optional<size_t> region) const{
    const auto& owner=articulation_owner(root_,instrument,region);std::vector<DlsArticulation> result;
    for(const auto& list:owner.children)if(list.id=="LIST"&&(list.type=="lart"||list.type=="lar2"))
        for(const auto& c:list.children)if(c.id=="art1"||c.id=="art2")result.push_back({list.type,c.id,connections(c.data)});
    return result;
}
bool DlsDocument::set_articulation_connections(size_t instrument,std::optional<size_t> region,size_t block,const std::vector<DlsConnection>& values){
    auto next=root_;auto chunks=articulation_chunks(articulation_owner(next,instrument,region));if(block>=chunks.size())return false;
    replace_connections(chunks[block]->data,values);return adopt(std::move(next));
}
bool DlsDocument::add_articulation(size_t instrument,std::optional<size_t> region,const DlsArticulation& value){
    if((value.listType!="lart"&&value.listType!="lar2")||(value.chunkId!="art1"&&value.chunkId!="art2"))throw std::runtime_error("Unsupported DLS articulation list or chunk");
    auto next=root_;auto& owner=articulation_owner(next,instrument,region);
    Chunk list;list.id="LIST";list.type=value.listType;Chunk c;c.id=value.chunkId;c.data=Bytes(8,0);put32(c.data,0,8);replace_connections(c.data,value.connections);list.children.push_back(std::move(c));owner.children.push_back(std::move(list));
    return adopt(std::move(next));
}
void DlsDocument::validate_playback_samples() const{
    const auto waveViews=waves();const auto instrumentViews=instruments();const auto mapping=cues(root_);
    const auto& pool=unique(root_,"LIST","wvpl");
    for(size_t i=0;i<waveViews.size();++i)loop_bounds(item(pool,i,"wave"),waveViews[i].frames);
    const auto& list=unique(root_,"LIST","lins");
    for(size_t i=0;i<instrumentViews.size();++i){const auto& regions=unique(item(list,i,"ins "),"LIST","lrgn");
        for(size_t j=0;j<instrumentViews[i].regions.size();++j)loop_bounds(item(regions,j,"rgn "),waveViews.at(mapping.at(instrumentViews[i].regions[j].tableIndex)).frames);}
}
Bytes DlsDocument::playback_sample_bytes() const{
    validate_playback_samples();
    auto next=root_;const auto mapping=cues(root_);const auto values=instruments();
    const auto& pool=unique(root_,"LIST","wvpl");auto& list=unique(next,"LIST","lins");
    for(size_t i=0;i<values.size();++i){auto& regions=unique(item(list,i,"ins "),"LIST","lrgn");
        for(size_t j=0;j<values[i].regions.size();++j){auto& region=item(regions,j,"rgn ");
            if(region.find("wsmp"))continue; // Explicit zero loops is one shot.
            const auto& wave=item(pool,mapping.at(values[i].regions[j].tableIndex),"wave");
            if(wave.find("wsmp"))region.children.push_back(unique(wave,"wsmp"));
        }
    }
    return next.encode();
}
std::vector<DlsLoop> DlsDocument::wave_loops(size_t wave) const{const auto values=waves();if(wave>=values.size())throw std::runtime_error("Wave index out of range");return sample_loops(item(unique(root_,"LIST","wvpl"),wave,"wave"));}
std::vector<DlsLoop> DlsDocument::region_loops(size_t instrument,size_t region) const{const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())throw std::runtime_error("Region index out of range");return sample_loops(item(unique(item(unique(root_,"LIST","lins"),instrument,"ins "),"LIST","lrgn"),region,"rgn "));}
bool DlsDocument::set_wave_loop(size_t wave,size_t loop,const DlsLoop& value){const auto values=waves();if(wave>=values.size())return false;(void)instruments();auto next=root_;if(!edit_loop(item(unique(next,"LIST","wvpl"),wave,"wave"),loop,value,values[wave].frames))return false;return adopt(std::move(next));}
bool DlsDocument::set_region_loop(size_t instrument,size_t region,size_t loop,const DlsLoop& value){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())return false;const auto mapping=cues(root_);const auto waveViews=waves();auto next=root_;auto& r=item(unique(item(unique(next,"LIST","lins"),instrument,"ins "),"LIST","lrgn"),region,"rgn ");if(!edit_loop(r,loop,value,waveViews.at(mapping.at(values[instrument].regions[region].tableIndex)).frames))return false;return adopt(std::move(next));}
bool DlsDocument::set_wave_loops(size_t wave,const std::vector<DlsLoop>& loops){const auto values=waves();if(wave>=values.size())return false;(void)instruments();const auto mapping=cues(root_);const auto& from=item(unique(root_,"LIST","wvpl"),wave,"wave");pcm_format(unique(from,"fmt ").data);auto next=root_;replace_loops(item(unique(next,"LIST","wvpl"),wave,"wave"),loops,values[wave].frames);relocate_pool(next,mapping);return adopt(std::move(next));}
bool DlsDocument::region_inherits_wave_sample(size_t instrument,size_t region) const{(void)region_loops(instrument,region);return !item(unique(item(unique(root_,"LIST","lins"),instrument,"ins "),"LIST","lrgn"),region,"rgn ").find("wsmp");}
std::vector<DlsLoop> DlsDocument::effective_region_loops(size_t instrument,size_t region) const{if(!region_inherits_wave_sample(instrument,region))return region_loops(instrument,region);const auto values=instruments();return wave_loops(cues(root_).at(values.at(instrument).regions.at(region).tableIndex));}
bool DlsDocument::set_region_loops(size_t instrument,size_t region,const std::vector<DlsLoop>& loops){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())return false;const auto wave=cues(root_).at(values[instrument].regions[region].tableIndex);const auto& defaults=item(unique(root_,"LIST","wvpl"),wave,"wave");pcm_format(unique(defaults,"fmt ").data);const auto frames=waves().at(wave).frames;auto next=root_;auto& r=item(unique(item(unique(next,"LIST","lins"),instrument,"ins "),"LIST","lrgn"),region,"rgn ");replace_loops(r,loops,frames,&defaults);return adopt(std::move(next));}
bool DlsDocument::inherit_wave_sample(size_t instrument,size_t region){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())return false;const auto wave=cues(root_).at(values[instrument].regions[region].tableIndex);loop_bounds(item(unique(root_,"LIST","wvpl"),wave,"wave"),waves().at(wave).frames);auto next=root_;auto& r=item(unique(item(unique(next,"LIST","lins"),instrument,"ins "),"LIST","lrgn"),region,"rgn ");if(!r.find("wsmp"))return false;(void)unique(r,"wsmp");r.children.erase(std::remove_if(r.children.begin(),r.children.end(),[](const Chunk& c){return c.id=="wsmp";}),r.children.end());return adopt(std::move(next));}
std::vector<DlsWave> DlsDocument::waves() const{const auto& pool=unique(root_,"LIST","wvpl");std::vector<DlsWave> result;for(size_t i=0;i<count(pool,"wave");++i){const auto& wave=item(pool,i,"wave"),&fmt=unique(wave,"fmt ");const auto& b=fmt.data;if(b.size()<16)throw std::runtime_error("DLS wave format truncated");const auto channels=word(b,2),align=word(b,12),bits=word(b,14);const auto& data=unique(wave,"data").data;if(!channels||!align||data.size()%align)throw std::runtime_error("DLS wave frame alignment");result.push_back({word(b,0),channels,bits,align,read32(b,4),data.size()/align});}return result;}
std::vector<DlsInstrument> DlsDocument::instruments() const{const auto& list=unique(root_,"LIST","lins");if(read32(unique(root_,"colh").data,0)!=count(list,"ins "))throw std::runtime_error("DLS instrument count mismatch");const auto mapping=cues(root_);std::vector<DlsInstrument> result;for(size_t i=0;i<count(list,"ins ");++i){const auto& ins=item(list,i,"ins "),&header=unique(ins,"insh");if(header.data.size()<12)throw std::runtime_error("DLS instrument header truncated");DlsInstrument value{read32(header.data,4),read32(header.data,8),{}};const auto& regions=unique(ins,"LIST","lrgn");if(read32(header.data,0)!=count(regions,"rgn "))throw std::runtime_error("DLS region count mismatch");for(size_t j=0;j<count(regions,"rgn ");++j){const auto& region=item(regions,j,"rgn ");const auto& h=unique(region,"rgnh").data;const auto& link=unique(region,"wlnk").data;if(h.size()<12||link.size()<12)throw std::runtime_error("DLS region truncated");DlsRegion r{word(h,0),word(h,2),word(h,4),word(h,6),word(h,10),read32(link,8)};if(!range(r.keyLow,r.keyHigh)||!range(r.velocityLow,r.velocityHigh)||r.tableIndex>=mapping.size())throw std::runtime_error("DLS region range or wave cue invalid");value.regions.push_back(r);}result.push_back(value);}return result;}
bool DlsDocument::adopt(Chunk next){const auto before=save_bytes(),after=next.encode();if(before==after)return false;
    // All typed edits validate the complete relocated pool and Region links
    // before publishing anything or changing history.
    DlsDocument validated;validated.root_=next;(void)validated.instruments();(void)validated.waves();
    undo_.push_back(before);if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;}
bool DlsDocument::set_instrument(size_t index,std::uint32_t bank,std::uint32_t program){const auto values=instruments();if(index>=values.size()||!locale(bank,program))return false;for(size_t i=0;i<values.size();++i)if(i!=index&&values[i].bank==bank&&values[i].program==program)return false;auto next=root_;auto& header=unique(item(unique(next,"LIST","lins"),index,"ins "),"insh").data;put32(header,4,bank);put32(header,8,program);return adopt(std::move(next));}
bool DlsDocument::set_region(size_t instrument,size_t region,const DlsRegion& value){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size()||!range(value.keyLow,value.keyHigh)||!range(value.velocityLow,value.velocityHigh)||value.keyGroup>15||value.tableIndex>=cues(root_).size())return false;auto next=root_;auto& ins=item(unique(next,"LIST","lins"),instrument,"ins ");auto& r=item(unique(ins,"LIST","lrgn"),region,"rgn ");auto& h=unique(r,"rgnh").data;putword(h,0,value.keyLow);putword(h,2,value.keyHigh);putword(h,4,value.velocityLow);putword(h,6,value.velocityHigh);putword(h,10,value.keyGroup);put32(unique(r,"wlnk").data,8,value.tableIndex);return adopt(std::move(next));}
bool DlsDocument::duplicate_region(size_t instrument,size_t region){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())return false;auto next=root_;auto& ins=item(unique(next,"LIST","lins"),instrument,"ins ");auto& regions=unique(ins,"LIST","lrgn");const auto copy=item(regions,region,"rgn ");regions.children.push_back(copy);put32(unique(ins,"insh").data,0,static_cast<std::uint32_t>(count(regions,"rgn ")));return adopt(std::move(next));}
bool DlsDocument::remove_region(size_t instrument,size_t region){const auto values=instruments();if(instrument>=values.size()||region>=values[instrument].regions.size())return false;auto next=root_;auto& ins=item(unique(next,"LIST","lins"),instrument,"ins ");auto& regions=unique(ins,"LIST","lrgn");size_t index=0;for(auto at=regions.children.begin();at!=regions.children.end();++at)if(at->id=="LIST"&&(at->type=="rgn "||at->type=="rgn2")){if(index++==region){regions.children.erase(at);break;}}put32(unique(ins,"insh").data,0,static_cast<std::uint32_t>(count(regions,"rgn ")));return adopt(std::move(next));}
bool DlsDocument::duplicate_wave(size_t index){
    const auto values=waves();if(index>=values.size())return false;
    (void)instruments();const auto mapping=cues(root_);auto next=root_;auto& pool=unique(next,"LIST","wvpl");auto copy=item(pool,index,"wave");
    // Producer GUID and standard DLS ID, when present, identify the new Wave.
    // Preserve extensions after the known 16-byte identity.
    for(const auto id:{"guid","dlid"})if(copy.find(id)){auto& data=unique(copy,id).data;if(data.size()<16)throw std::runtime_error("Wave identity truncated");GUID guid{};if(FAILED(CoCreateGuid(&guid)))throw std::runtime_error("Wave GUID creation failed");std::memcpy(data.data(),&guid,16);}
    size_t waveIndex=0;for(auto at=pool.children.begin();at!=pool.children.end();++at)if(at->id=="LIST"&&at->type=="wave"){if(waveIndex++==index){pool.children.insert(at,std::move(copy));break;}}
    std::vector<size_t> offsets;size_t offset=0;for(const auto& c:pool.children){if(c.id=="LIST"&&c.type=="wave")offsets.push_back(offset);offset+=c.encode().size();}
    auto& table=unique(next,"ptbl").data;const size_t header=read32(table,0),entries=mapping.size();if(entries==UINT32_MAX)throw std::runtime_error("DLS pool table capacity");
    // Existing cue indices remain stable, including aliases and arbitrary cue
    // order. Account for every opaque child and RIFF padding in the pool.
    for(size_t i=0;i<entries;++i){const auto old=mapping[i];put32(table,header+i*4,static_cast<std::uint32_t>(offsets.at(old+(old>=index?1:0))));}
    const auto end=header+entries*4;table.insert(table.begin()+end,4,0);put32(table,end,static_cast<std::uint32_t>(offsets.at(index)));put32(table,4,static_cast<std::uint32_t>(entries+1));
    return adopt(std::move(next));
}
bool DlsDocument::remove_wave(size_t index,std::optional<std::uint32_t> replacementCue){
    const auto values=waves();if(index>=values.size())return false;(void)instruments();const auto mapping=cues(root_);
    if(replacementCue&&(*replacementCue>=mapping.size()||mapping[*replacementCue]==index))throw std::runtime_error("Replacement cue must identify a surviving Wave");
    auto next=root_;auto& instrumentsList=unique(next,"LIST","lins");
    std::vector<size_t> newCues(mapping.size(),SIZE_MAX);size_t n=0;for(size_t i=0;i<mapping.size();++i)if(mapping[i]!=index)newCues[i]=n++;
    for(size_t i=0;i<count(instrumentsList,"ins ");++i){auto& regions=unique(item(instrumentsList,i,"ins "),"LIST","lrgn");
        for(size_t j=0;j<count(regions,"rgn ");++j){auto& region=item(regions,j,"rgn ");auto& link=unique(region,"wlnk").data;const auto cue=read32(link,8);size_t target=cue;
            if(mapping.at(cue)==index){if(!replacementCue)throw std::runtime_error("Wave is referenced; select a replacement cue before deleting");target=*replacementCue;
                const auto& pool=unique(root_,"LIST","wvpl");const auto& from=unique(item(pool,index,"wave"),"fmt ").data;const auto& to=unique(item(pool,mapping[target],"wave"),"fmt ").data;pcm_format(from);pcm_format(to);
                if(!std::equal(from.begin(),from.begin()+16,to.begin()))throw std::runtime_error("Referenced Wave replacement must retain PCM channels rate and depth");
                loop_bounds(region,values.at(mapping[target]).frames);
            }put32(link,8,static_cast<std::uint32_t>(newCues.at(target)));
        }
    }
    auto& pool=unique(next,"LIST","wvpl");size_t waveIndex=0;for(auto at=pool.children.begin();at!=pool.children.end();++at)if(at->id=="LIST"&&at->type=="wave"&&waveIndex++==index){pool.children.erase(at);break;}
    std::vector<size_t> offsets;size_t offset=0;for(const auto& c:pool.children){if(c.id=="LIST"&&c.type=="wave")offsets.push_back(offset);offset+=c.encode().size();}
    auto& table=unique(next,"ptbl").data;const size_t header=read32(table,0);Bytes entries;entries.reserve(n*4);
    for(const auto old:mapping)if(old!=index){const auto at=entries.size();entries.resize(at+4);const auto relocated=offsets.at(old-(old>index?1:0));if(relocated>UINT32_MAX)throw std::runtime_error("DLS pool offset capacity");put32(entries,at,static_cast<std::uint32_t>(relocated));}
    table.erase(table.begin()+header,table.begin()+header+mapping.size()*4);table.insert(table.begin()+header,entries.begin(),entries.end());put32(table,4,static_cast<std::uint32_t>(n));
    return adopt(std::move(next));
}
bool DlsDocument::scale_wave(size_t index,unsigned percent){const auto values=waves();if(index>=values.size()||percent>400)return false;const auto& w=values[index];if(w.format!=1||(w.bits!=8&&w.bits!=16)||w.blockAlign!=w.channels*(w.bits/8))throw std::runtime_error("Wave editing requires 8/16-bit integer PCM");auto next=root_;auto& data=unique(item(unique(next,"LIST","wvpl"),index,"wave"),"data").data;for(size_t at=0;at<data.size();at+=w.bits/8){const auto sample=w.bits==8?static_cast<int>(data[at])-128:static_cast<int>(static_cast<std::int16_t>(word(data,at)));const auto scaled=sample*static_cast<int>(percent)/100;if(w.bits==8)data[at]=static_cast<std::uint8_t>(std::clamp(scaled,-128,127)+128);else putword(data,at,static_cast<unsigned>(std::clamp(scaled,-32768,32767)));}return adopt(std::move(next));}
Bytes DlsDocument::export_wave_pcm(size_t index) const{
    (void)waves();const auto& wave=item(unique(root_,"LIST","wvpl"),index,"wave");const auto& fmt=unique(wave,"fmt ");pcm_format(fmt.data);
    // A portable PCM WAVE contains the format and samples only. DLS/Producer
    // identity, articulation and loop metadata remain in the owned collection.
    Chunk wav;wav.id="RIFF";wav.type="WAVE";wav.children={fmt,unique(wave,"data")};return wav.encode();
}
bool DlsDocument::create_instrument(std::uint32_t bank,std::uint32_t program,const std::string& name,std::uint32_t cue){
    const auto values=instruments();const auto mapping=cues(root_);(void)waves();
    if(!locale(bank,program)||cue>=mapping.size())return false;
    if(std::any_of(values.begin(),values.end(),[&](const DlsInstrument& i){return i.bank==bank&&i.program==program;}))return false;
    if(std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c>126;}))throw std::runtime_error("New Instrument name requires printable ASCII");
    if(values.size()>=UINT32_MAX)throw std::runtime_error("DLS instrument capacity");
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Cannot create Instrument identity");
    Chunk ins;ins.id="LIST";ins.type="ins ";Chunk id;id.id="dlid";id.data.resize(16);std::memcpy(id.data.data(),&identity,16);
    Chunk header;header.id="insh";header.data=Bytes(12,0);put32(header.data,4,bank);put32(header.data,8,program);
    Chunk regions;regions.id="LIST";regions.type="lrgn";Chunk info;info.id="LIST";info.type="INFO";
    for(const auto field:{"ICMT","ICOP","IENG","INAM","ISBJ"}){if(std::string(field)=="INAM"&&name.empty())continue;Chunk value;value.id=field;if(value.id=="INAM")value.data.assign(name.begin(),name.end());value.data.push_back(0);info.children.push_back(std::move(value));}
    ins.children={std::move(id),std::move(header),std::move(regions),std::move(info)};
    auto next=root_;unique(next,"LIST","lins").children.push_back(std::move(ins));put32(unique(next,"colh").data,0,static_cast<std::uint32_t>(values.size()+1));
    // Reuse Region sample validation on an isolated document. Publishing the
    // instrument and its Region together creates exactly one history entry;
    // unsupported stereo/loops cannot leave an empty instrument behind.
    DlsDocument staged;staged.load(next.encode());if(!staged.create_region(values.size(),{0,127,0,127,0,cue}))return false;
    auto authored=Chunk::parse(staged.save_bytes());auto& added=item(unique(authored,"LIST","lins"),values.size(),"ins ");auto& region=item(unique(added,"LIST","lrgn"),0,"rgn ");
    // Saved original Insert Instrument fixture: retain these bytes without
    // assigning undocumented semantics to dmpr or the articulation record.
    auto& rgnh=unique(region,"rgnh").data;rgnh.resize(14,0);putword(rgnh,8,1);
    Chunk producer;producer.id="dmpr";producer.data={1,0,1,0};region.children.push_back(std::move(producer));
    Chunk articulation;articulation.id="LIST";articulation.type="lar2";Chunk connection;connection.id="art1";connection.data=Bytes(20,0);put32(connection.data,0,8);put32(connection.data,4,1);putword(connection.data,12,0x0500);put32(connection.data,16,0x7fffffff);articulation.children.push_back(std::move(connection));
    added.children.insert(added.children.end()-1,std::move(articulation));
    return adopt(std::move(authored));
}
bool DlsDocument::create_region(size_t instrument,const DlsRegion& value){
    const auto instrumentsView=instruments();const auto mapping=cues(root_);const auto wavesView=waves();
    if(instrument>=instrumentsView.size()||!range(value.keyLow,value.keyHigh)||!range(value.velocityLow,value.velocityHigh)||value.keyGroup>15||value.tableIndex>=mapping.size())return false;
    const auto waveIndex=mapping[value.tableIndex];const auto& waveView=wavesView.at(waveIndex);
    if(waveView.channels!=1)throw std::runtime_error("New Region stereo channel placement not implemented");
    const auto& wave=item(unique(root_,"LIST","wvpl"),waveIndex,"wave");
    pcm_format(unique(wave,"fmt ").data);
    Chunk region;region.id="LIST";region.type="rgn ";Chunk header;header.id="rgnh";header.data=Bytes(12,0);
    putword(header.data,0,value.keyLow);putword(header.data,2,value.keyHigh);putword(header.data,4,value.velocityLow);putword(header.data,6,value.velocityHigh);putword(header.data,10,value.keyGroup);
    Chunk sample;if(wave.find("wsmp")){sample=unique(wave,"wsmp");loop_bounds(wave,waveView.frames);if(word(sample.data,4)>127)throw std::runtime_error("Wave root note invalid");}
    else{sample.id="wsmp";sample.data=Bytes(20,0);put32(sample.data,0,20);putword(sample.data,4,60);}
    Chunk link;link.id="wlnk";link.data=Bytes(12,0);put32(link.data,4,1);put32(link.data,8,value.tableIndex);region.children={std::move(header),std::move(sample),std::move(link)};
    // A new Region owns Wave sample defaults; it does not copy another
    // Region's root override, articulation, identity or opaque extension.
    auto next=root_;auto& ins=item(unique(next,"LIST","lins"),instrument,"ins ");auto& regions=unique(ins,"LIST","lrgn");
    const auto n=count(regions,"rgn ");if(n==UINT32_MAX)throw std::runtime_error("DLS region capacity");regions.children.push_back(std::move(region));put32(unique(ins,"insh").data,0,static_cast<std::uint32_t>(n+1));return adopt(std::move(next));
}
bool DlsDocument::add_wave_pcm(const Bytes& bytes,const std::string& name){
    if(name.empty()||std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c>126;}))throw std::runtime_error("New Wave name requires printable ASCII");
    (void)instruments();(void)waves();const auto mapping=cues(root_);
    const auto wav=Chunk::parse(bytes);if(wav.id!="RIFF"||wav.type!="WAVE")throw std::runtime_error("Expected PCM WAVE file");
    const auto& format=unique(wav,"fmt ").data;pcm_format(format);
    if(word(format,2)>2||format.size()>18||format.size()==17||(format.size()==18&&word(format,16)!=0))throw std::runtime_error("New Wave requires mono/stereo PCM without format extensions");
    for(const auto id:{"wsmp","smpl","fact","cue ","plst"})if(wav.find(id))throw std::runtime_error("New Wave sampler metadata import not implemented: "+std::string(id));
    const auto& samples=unique(wav,"data").data;if(samples.empty()||samples.size()%word(format,12))throw std::runtime_error("New Wave requires complete nonempty PCM frames");
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Cannot create Wave identity");
    Chunk wave;wave.id="LIST";wave.type="wave";Chunk id;id.id="guid";id.data.resize(16);std::memcpy(id.data.data(),&identity,16);
    Chunk fmt;fmt.id="fmt ";fmt.data.assign(format.begin(),format.begin()+16);fmt.data.resize(18,0);
    Chunk sample;sample.id="wsmp";sample.data=Bytes(20,0);put32(sample.data,0,20);putword(sample.data,4,60);put32(sample.data,12,1);
    Chunk data;data.id="data";data.data=samples;Chunk info;info.id="LIST";info.type="INFO";
    for(const auto field:{"ICMT","ICOP","IENG","INAM","ISBJ"}){Chunk value;value.id=field;if(value.id=="INAM")value.data.assign(name.begin(),name.end());value.data.push_back(0);info.children.push_back(std::move(value));}
    wave.children={std::move(id),std::move(fmt),std::move(sample),std::move(data),std::move(info)};
    auto next=root_;auto& pool=unique(next,"LIST","wvpl");size_t offset=0;for(const auto& child:pool.children)offset+=child.encode().size();
    if(offset>UINT32_MAX||mapping.size()>=UINT32_MAX)throw std::runtime_error("DLS Wave pool capacity");
    pool.children.push_back(std::move(wave));auto& table=unique(next,"ptbl").data;const size_t header=read32(table,0),end=header+mapping.size()*4;
    table.insert(table.begin()+end,4,0);put32(table,end,static_cast<std::uint32_t>(offset));put32(table,4,static_cast<std::uint32_t>(mapping.size()+1));
    return adopt(std::move(next));
}
bool DlsDocument::import_wave_pcm(size_t index,const Bytes& bytes){
    const auto values=waves();if(index>=values.size())return false;const auto wav=Chunk::parse(bytes);if(wav.id!="RIFF"||wav.type!="WAVE")throw std::runtime_error("Expected PCM WAVE file");
    const auto& incoming=unique(wav,"fmt ").data;pcm_format(incoming);const auto& samples=unique(wav,"data").data;
    const auto& current=item(unique(root_,"LIST","wvpl"),index,"wave");const auto& format=unique(current,"fmt ").data;pcm_format(format);
    if(!std::equal(format.begin(),format.begin()+16,incoming.begin()))throw std::runtime_error("PCM import must retain channels, sample rate and bit depth");
    if(samples.empty()||samples.size()%word(format,12))throw std::runtime_error("PCM import requires complete nonempty frames");
    (void)instruments();const auto mapping=cues(root_);
    if(samples.size()!=unique(current,"data").data.size())resize_bounds(root_,index,samples.size()/word(format,12),mapping);
    // Imported ancillary WAVE chunks are not DLS metadata. Preserve all owned
    // metadata, padding, cue aliases and GUIDs exactly. Relocate pool offsets
    // after replacement, accounting for opaque children and odd RIFF padding.
    auto next=root_;auto& pool=unique(next,"LIST","wvpl");unique(item(pool,index,"wave"),"data").data=samples;
    std::vector<size_t> offsets;size_t offset=0;for(const auto& c:pool.children){if(c.id=="LIST"&&c.type=="wave")offsets.push_back(offset);offset+=c.encode().size();}
    auto& table=unique(next,"ptbl").data;const auto header=read32(table,0);for(size_t i=0;i<mapping.size();++i)put32(table,header+i*4,static_cast<std::uint32_t>(offsets.at(mapping[i])));
    return adopt(std::move(next));
}
bool DlsDocument::undo(){if(undo_.empty())return false;auto next=Chunk::parse(undo_.back());redo_.push_back(save_bytes());undo_.pop_back();root_=std::move(next);return true;}
bool DlsDocument::redo(){if(redo_.empty())return false;auto next=Chunk::parse(redo_.back());undo_.push_back(save_bytes());redo_.pop_back();root_=std::move(next);return true;}
}
