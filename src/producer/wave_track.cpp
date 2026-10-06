#include "wave_track.h"
#include <cstring>
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace producer::app {
namespace {
template<class C> auto unique(C& parent,const char* id,const char* type="")->decltype(parent.find(id,type)) {
    decltype(parent.find(id,type)) found=nullptr;
    for(auto& c:parent.children)if(c.id==id&&(!*type||c.type==type)){
        if(found)throw std::runtime_error("Ambiguous Wave field");found=&c;
    }
    if(!found)throw std::runtime_error("Missing Wave field");return found;
}
template<class C> C& nth(C& parent,const char* type,size_t index){
    for(auto& c:parent.children)if(c.id=="LIST"&&c.type==type){if(!index--)return c;}
    throw std::runtime_error("Wave selection out of range");
}
std::int64_t signed64(const Bytes& b,size_t at){
    if(at+8>b.size())throw std::runtime_error("Truncated Wave time");
    const std::uint64_t bits=read32(b,at)|(static_cast<std::uint64_t>(read32(b,at+4))<<32);
    std::int64_t result;std::memcpy(&result,&bits,8);return result;
}
void put64(Bytes& b,size_t at,std::int64_t value){
    const auto bits=static_cast<std::uint64_t>(value);put32(b,at,static_cast<std::uint32_t>(bits));put32(b,at+4,static_cast<std::uint32_t>(bits>>32));
}
WavePlacement placement(const Bytes& b){
    // Win32 DirectMusic file layout includes 4 bytes of alignment after the
    // variation DWORD; original SfxCow waih is 64 bytes, not a packed 60.
    if(b.size()<64)throw std::runtime_error("Truncated Wave item header");
    return {signed64(b,16),signed64(b,24),signed64(b,40),static_cast<std::int32_t>(read32(b,0)),static_cast<std::int32_t>(read32(b,4))};
}
bool valid(const WavePlacement& e){
    const auto max=std::numeric_limits<std::int64_t>::max();
    return e.time>=0&&e.startOffset>=0&&e.duration>0&&e.volume<=0&&e.time<=max-e.duration;
}
std::wstring filename(const Chunk& item){
    const auto reference=unique(item,"LIST","DMRF");
    if(!reference->find("file"))return {};
    const auto& b=unique(*reference,"file")->data;
    if(b.size()%2)throw std::runtime_error("Odd Wave reference filename");
    std::wstring result;for(size_t i=0;i<b.size();i+=2){auto v=b[i]|(b[i+1]<<8);if(!v){if(result.empty())throw std::runtime_error("Empty Wave reference filename");return result;}result.push_back(static_cast<wchar_t>(v));}
    throw std::runtime_error("Unterminated Wave reference filename");
}
}
Bytes wave_track_identity(){return {0x61,0x64,0xd3,0xee,0xa5,0x9e,0xd3,0x11,0x9b,0xd1,0x00,0x80,0xc7,0x15,0x0a,0x74};}
std::vector<WaveItem> wave_items(const Chunk& track){
    bool clockTime=false;
    if(track.find("trkx")){const auto& flags=unique(track,"trkx")->data;if(flags.size()<8)throw std::runtime_error("Truncated Wave track configuration");clockTime=(read32(flags,0)&0x40)!=0;}
    const auto& payload=*unique(track,"LIST","wavt");
    if(unique(payload,"wath")->data.size()<8)throw std::runtime_error("Truncated Wave track header");
    std::vector<WaveItem> result;size_t partIndex=0;
    for(const auto& part:payload.children)if(part.id=="LIST"&&part.type=="wavp"){
        const auto& ph=unique(part,"waph")->data;if(ph.size()<24)throw std::runtime_error("Truncated Wave part header");
        size_t index=0;for(const auto& item:unique(part,"LIST","wavi")->children)if(item.id=="LIST"&&item.type=="wave"){
            const auto& h=unique(item,"waih")->data;auto p=placement(h);
            WaveItem value{partIndex,index++,read32(ph,8),read32(h,8),p,filename(item),clockTime,{}};
            const auto reference=unique(item,"LIST","DMRF");if(reference->find("guid")){const auto& bytes=unique(*reference,"guid")->data;if(bytes.size()!=16)throw std::runtime_error("Wave reference GUID truncated");std::array<std::uint8_t,16> identity{};std::copy(bytes.begin(),bytes.end(),identity.begin());value.objectId=identity;}
            if(value.filename.empty()&&!value.objectId)throw std::runtime_error("Wave reference has neither identity nor filename");result.push_back(std::move(value));
        }++partIndex;
    }return result;
}
bool insert_wave_reference(Chunk& track,size_t part,const WaveReference& reference,const WavePlacement& e,std::uint32_t variations,size_t* result){
    if(!valid(e)||!variations)return false;
    if(reference.filename.empty()||reference.filename.size()>255||reference.filename==L"."||reference.filename==L".."||reference.filename.find_first_of(L"/\\:<>\"|?*")!=std::wstring::npos||reference.filename.find(wchar_t(0))!=std::wstring::npos)throw std::runtime_error("Wave reference must be a simple owned filename");
    if(reference.objectId&&std::all_of(reference.objectId->begin(),reference.objectId->end(),[](auto b){return b==0;}))throw std::runtime_error("Null Wave object identity");
    (void)wave_items(track);const auto& config=unique(track,"trkx")->data;if(config.size()<8||!(read32(config,0)&0x40))throw std::runtime_error("Wave insertion requires a clock-time part");
    auto next=track;auto& target=nth(*unique(next,"LIST","wavt"),"wavp",part);const auto& ph=unique(target,"waph")->data;
    if(ph.size()<24||((variations&read32(ph,4))!=variations))throw std::runtime_error("Choose variations belonging to the Wave part");
    Chunk item;item.id="LIST";item.type="wave";Chunk header;header.id="waih";header.data.resize(64);put32(header.data,0,static_cast<std::uint32_t>(e.volume));put32(header.data,4,static_cast<std::uint32_t>(e.pitch));put32(header.data,8,variations);put64(header.data,16,e.time);put64(header.data,24,e.startOffset);put64(header.data,40,e.duration);item.children.push_back(header);
    Chunk ref;ref.id="LIST";ref.type="DMRF";Chunk refh;refh.id="refh";refh.data={0x54,0x71,0x66,0x8a,0xcb,0xf9,0xd2,0x11,0xad,0x8a,0x00,0x60,0xb0,0x57,0x5a,0xbc};refh.data.resize(20);put32(refh.data,16,0x12u|(reference.objectId?1u:0u));ref.children.push_back(refh);
    if(reference.objectId){Chunk guid;guid.id="guid";guid.data.assign(reference.objectId->begin(),reference.objectId->end());ref.children.push_back(guid);}Chunk file;file.id="file";file.data=utf16(reference.filename);ref.children.push_back(file);item.children.push_back(ref);
    auto& list=*unique(target,"LIST","wavi");size_t index=0;for(const auto& c:list.children)if(c.id=="LIST"&&c.type=="wave")++index;list.children.push_back(item);(void)wave_items(next);track=std::move(next);if(result)*result=index;return true;
}
bool edit_wave_placement(Chunk& track,size_t part,size_t index,const WavePlacement& e){
    if(!valid(e))return false;
    const auto items=wave_items(track);if(!items.empty()&&!items.front().clockTime)throw std::runtime_error("Music-time Wave placement requires logical time and tempo conversion contract");auto next=track;auto& list=nth(*unique(next,"LIST","wavt"),"wavp",part);
    auto& item=nth(*unique(list,"LIST","wavi"),"wave",index);auto& h=unique(item,"waih")->data;
    // Only observed SDK fields are changed. Keep padding, reserved time,
    // logical time, loops, flags, extensions, references and Producer data.
    put32(h,0,static_cast<std::uint32_t>(e.volume));put32(h,4,static_cast<std::uint32_t>(e.pitch));
    put64(h,16,e.time);put64(h,24,e.startOffset);put64(h,40,e.duration);
    if(next.encode()==track.encode())return false;track=std::move(next);return true;
}
Bytes copy_wave_event(const Chunk& track,size_t part,size_t index){
    const auto items=wave_items(track);auto selected=std::find_if(items.begin(),items.end(),[&](const auto& e){return e.part==part&&e.index==index;});
    if(selected==items.end())throw std::runtime_error("Wave selection out of range");
    const auto& source=nth(*unique(track,"LIST","wavt"),"wavp",part);
    Chunk clip;clip.id="RIFF";clip.type="WVCP";
    Chunk version;version.id="vers";version.data=Bytes(4);put32(version.data,0,1);clip.children.push_back(version);
    Chunk mode;mode.id="mode";mode.data=Bytes(4);put32(mode.data,0,selected->clockTime?1u:0u);clip.children.push_back(mode);
    clip.children.push_back(*unique(source,"waph"));clip.children.push_back(nth(*unique(source,"LIST","wavi"),"wave",index));return clip.encode();
}
bool paste_wave_event(Chunk& track,const Bytes& bytes,size_t part,std::int64_t time,size_t* result){
    if(time<0)return false;
    const auto clip=Chunk::parse(bytes);if(clip.type!="WVCP"||clip.children.size()!=4)throw std::runtime_error("Expected one Wave event clipboard");
    const auto& version=unique(clip,"vers")->data;const auto& mode=unique(clip,"mode")->data;
    if(version.size()!=4||read32(version,0)!=1||mode.size()!=4||read32(mode,0)!=1)throw std::runtime_error("Only clock-time Wave clipboard version1 is supported");
    (void)wave_items(track);const auto config=unique(track,"trkx");if(config->data.size()<8||!(read32(config->data,0)&0x40))throw std::runtime_error("Wave paste clock domain mismatch");
    auto next=track;auto& target=nth(*unique(next,"LIST","wavt"),"wavp",part);
    if(unique(target,"waph")->data!=unique(clip,"waph")->data)throw std::runtime_error("Wave clipboard part settings differ; PChannel/variations/gain must match");
    auto item=*unique(clip,"LIST","wave");auto& h=unique(item,"waih")->data;auto e=placement(h);e.time=time;if(!valid(e))return false;put64(h,16,time);
    auto& list=*unique(target,"LIST","wavi");size_t index=0;for(const auto& c:list.children)if(c.id=="LIST"&&c.type=="wave")++index;
    list.children.push_back(std::move(item));(void)wave_items(next);track=std::move(next);if(result)*result=index;return true;
}
bool delete_wave_event(Chunk& track,size_t part,size_t index){
    (void)wave_items(track);auto next=track;auto& target=nth(*unique(next,"LIST","wavt"),"wavp",part);auto& list=*unique(target,"LIST","wavi");
    size_t current=0;for(auto it=list.children.begin();it!=list.children.end();++it)if(it->id=="LIST"&&it->type=="wave"){
        if(current++==index){list.children.erase(it);track=std::move(next);return true;}
    }return false;
}

}
