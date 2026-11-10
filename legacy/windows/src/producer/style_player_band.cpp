#include "style_player_band.h"
#include "style.h"
#include "native_script_track.h"
#include <algorithm>
#include <memory>
#include <cstring>
#include <cwchar>
#include <stdexcept>
namespace producer::app { namespace {
template<class T> struct Release {void operator()(T* p)const{if(p)p->Release();}};
template<class T> using Owner=std::unique_ptr<T,Release<T>>;
Bytes source_band(IUnknown* band, const std::vector<BandDocument>& sources, const Bytes& defaultBand, LONG time) {
    runtime::MusicObject* raw=nullptr;
    const auto queried=band->QueryInterface(runtime::musicObjectId,reinterpret_cast<void**>(&raw));
    Owner<runtime::MusicObject> object(raw);
    if(queried!=S_OK||!object)throw std::runtime_error("Generated Band descriptor unavailable");
    runtime::ObjectDesc d{};d.size=sizeof(d);
    if(object->GetDescriptor(&d)!=S_OK)throw std::runtime_error("Generated Band descriptor read failed");
    const bool hasId=(d.valid&1u)!=0,hasName=(d.valid&4u)!=0;
    const GUID bandClass={0x79ba9e00,0xb6ee,0x11d1,{0x86,0xbe,0,0xc0,0x4f,0xbf,0x8f,0xef}};
    if((d.valid&2u)&&!IsEqualGUID(d.classId,bandClass))throw std::runtime_error("Generated Band descriptor class differs");
    if(hasName&&(!std::wmemchr(d.name,0,64)||!d.name[0]))throw std::runtime_error("Generated Band source name invalid");
    if(!hasId&&!hasName) {
        if(time!=0)throw std::runtime_error("Later anonymous Band source cannot be resolved");
        return defaultBand;
    }
    Bytes result;
    for(const auto& source:sources) {
        const auto bytes=source.save_bytes();const auto root=Chunk::parse(bytes);
        const auto id=root.find("guid"),info=root.find("LIST","UNFO");
        const auto name=info?info->find("UNAM"):nullptr;
        if(hasId&&(!id||id->data.size()!=16||std::memcmp(id->data.data(),&d.objectId,16)))continue;
        if(hasName&&(!name||decode_utf16(name->data)!=d.name))continue;
        if(!result.empty())throw std::runtime_error("Generated Band source is ambiguous");
        result=bytes;
    }
    if(result.empty())throw std::runtime_error("Generated Band source bytes missing");
    return result;
}
} // namespace
Chunk read_style_player_bands(runtime::Track* track,LONG length,const Bytes& ownedStyle,const Bytes& defaultBand) {
    if(!track||length<=0)throw std::runtime_error("Generated Band track/length invalid");
    StyleDocument style;style.load(ownedStyle);const auto sources=style.bands();
    BandDocument validation;validation.load(defaultBand);
    if(std::count_if(sources.begin(),sources.end(),[&](const BandDocument& b){return b.save_bytes()==defaultBand;})!=1)throw std::runtime_error("Generated default Band is not uniquely owned");
    auto output=make_band_track();LONG time=0;
    for(unsigned i=0;i<8192;++i) {
        LONG next=0;runtime::BandParam p{};
        const auto read=track->GetParam(runtime::bandParam,time,&next,&p);
        Owner<IUnknown> band(p.band);
        if(read!=S_OK||!band)throw std::runtime_error("Generated Band event unavailable");
        const auto bytes=source_band(band.get(),sources,defaultBand,time);
        if(!set_band_track_event(output,time,bytes))throw std::runtime_error("Generated Band event cannot be represented");
        if(p.physicalTime!=time&&!move_band_track_event(output,i,time,p.physicalTime))throw std::runtime_error("Generated Band physical time cannot be represented");
        if(next==0)return output;
        // The declared OS BandTrack's classic GetParam returns the next
        // logical position (0/3072/6144 probe), unlike GetParamEx's delta.
        if(next<=time||next>length)throw std::runtime_error("Generated Band next time invalid");
        time=next;
    }
    throw std::runtime_error("Generated Band events exceeded bound");
}
std::wstring initial_style_player_band_name(const Bytes& segment) {
    const auto root=Chunk::parse(segment);
    if(root.id!="RIFF"||root.type!="DMSG")throw std::runtime_error("StylePlayer needs a native Segment");
    const auto tracks=root.find("LIST","trkl");
    const Chunk* selected=nullptr;
    if(tracks)for(const auto& track:tracks->children)if(is_band_track(track)) {
        if(selected)throw std::runtime_error("StylePlayer Band tracks are ambiguous");
        selected=&track;
    }
    if(!selected)throw std::runtime_error("StylePlayer initial Band is absent");
    const auto events=band_track_events(*selected);
    const BandEvent* initial=nullptr;
    for(const auto& event:events)if(event.logicalTime==0)initial=&event;
    if(!initial)throw std::runtime_error("StylePlayer initial Band is absent");
    const auto band=Chunk::parse(initial->band);const auto info=band.find("LIST","UNFO");
    const auto name=info?info->find("UNAM"):nullptr;
    if(!name||decode_utf16(name->data).empty())throw std::runtime_error("StylePlayer initial Band has no source name");
    return decode_utf16(name->data);
}
} // namespace producer::app
