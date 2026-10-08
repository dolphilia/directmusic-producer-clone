#include "document.h"
#include "audio_path.h"
#include "band.h"
#include <functional>
#include "tool_graph.h"
#include "file_output_dmo.h"
#include "compat/waves_reverb.h"
#include "compat/directsound_send.h"
#include "environmental_reverb_dmo.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
bool is_declared_os_audio_effect(const AudioBufferId& classId){
    // The same nine public dsound.h classes used by Conductor audition.
    static const wchar_t* classes[]={L"{DAFD8210-5711-4B91-9FE3-F75B7AE279BF}",L"{EFE6629C-81F7-4281-BD91-C9D604A95AF6}",L"{EFCA3D92-DFD8-4672-A603-7420894BAD98}",L"{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}",L"{EF114C90-CD1D-484E-96E5-09CFAF912A21}",L"{EF011F79-4000-406D-87AF-BFFB3FC39D57}",L"{120CED89-3BF4-4173-A132-3CB406CF3231}",L"{EF985E71-D5C7-42D4-BA4D-2D073E2E96F4}",L"{87FC0268-9A55-4360-95AA-004A1D9DE26C}"};
    GUID id{};std::memcpy(&id,classId.data(),16);
    for(const auto text:classes){GUID expected{};if(SUCCEEDED(CLSIDFromString(text,&expected))&&IsEqualGUID(expected,id))return true;}
    return false;
}
Bytes prepare_audio_path_band_downloads(const Bytes& document,const Bytes& audioPath){
    if(document.empty()||audioPath.empty())return document;
    AudioPathDocument config;config.load(audioPath);const auto ports=config.ports();
    const auto connected=[&](std::uint32_t channel){
        for(const auto& port:ports)for(const auto& route:port.routes)
            if(channel>=port.base&&std::uint64_t(channel)<std::uint64_t(port.base)+port.count&&
               channel>=route.base&&std::uint64_t(channel)<std::uint64_t(route.base)+route.count&&!route.buffers.empty())return true;
        return false;
    };
    auto root=Chunk::parse(document);bool changed=false;std::size_t removedCount=0;
    std::function<bool(Chunk&)> prepare=[&](Chunk& node){
        const auto removedBefore=removedCount;
        if(node.id=="RIFF"&&node.type=="DMBD"){
            BandDocument validated;validated.load(node.encode());(void)validated.instruments();
            bool removed=false;
            if(auto list=node.find("LIST","lbil")){
                auto& items=list->children;
                items.erase(std::remove_if(items.begin(),items.end(),[&](const Chunk& item){
                    if(item.id!="LIST"||item.type!="lbin")return false;
                    const auto header=item.find("bins");
                    if(!header||header->data.size()<34)throw std::runtime_error("Invalid Band download instrument");
                    if(connected(read32(header->data,24)))return false;
                    changed=true;removed=true;++removedCount;return true;
                }),items.end());
            }
            // An empty Band event is not a loadable runtime Band on Windows.
            // Omit that no-op event in the private playback copy only.
            BandDocument prepared;prepared.load(node.encode());
            return removed&&prepared.instruments().empty();
        }
        bool omitted=false;
        auto& children=node.children;
        children.erase(std::remove_if(children.begin(),children.end(),[&](Chunk& child){
            if(!prepare(child))return false;omitted=true;return true;
        }),children.end());
        if(omitted&&node.id=="LIST"&&node.type=="lbnd")return true;
        if(removedCount>removedBefore&&node.id=="RIFF"&&node.type=="DMTK")if(const auto bands=node.find("RIFF","DMBT"))
            if(const auto events=bands->find("LIST","lbdl"))
                if(std::none_of(events->children.begin(),events->children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="lbnd";}))return true;
        return false;
    };
    prepare(root);return changed?root.encode():document;
}
Bytes prepare_transport_audio_path(const Bytes& segment,const Bytes& audioPath){
    if(audioPath.empty())return segment;
    AudioPathDocument config;config.load(audioPath);
    if(segment.empty()){SegmentDocument carrier;carrier.set_audio_path(audioPath);return carrier.save_bytes();}
    auto root=Chunk::parse(segment);if(root.id!="RIFF"||root.type!="DMSG")throw std::runtime_error("Transport requires a Segment playback snapshot");
    if(root.find("RIFF","DMAP"))return segment;
    root.children.push_back(Chunk::parse(config.save_bytes()));return root.encode();
}

namespace {
const Chunk* unique(const Chunk& c,const char* id,const char* type="") {const Chunk* result=nullptr;for(const auto& x:c.children)if(x.id==id&&(!*type||x.type==type)){if(result)throw std::runtime_error("Ambiguous AudioPath record");result=&x;}return result;}
AudioBufferId identity(const Bytes& b,size_t at=0){if(at>b.size()||b.size()-at<16)throw std::runtime_error("Truncated AudioPath buffer identity");AudioBufferId id{};std::copy_n(b.begin()+at,16,id.begin());return id;}
Chunk leaf(const char* id,Bytes data){Chunk c;c.id=id;c.data=std::move(data);return c;}
Chunk list(const char* type,std::vector<Chunk> children){Chunk c;c.id="LIST";c.type=type;c.children=std::move(children);return c;}
Bytes guid_bytes(const GUID& id){const auto b=reinterpret_cast<const std::uint8_t*>(&id);return Bytes(b,b+16);}
}
AudioPathDocument::AudioPathDocument(){
    root_.id="RIFF";root_.type="DMAP";GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create AudioPath identity");root_.children.push_back(leaf("guid",guid_bytes(id)));
    root_.children.push_back(list("UNFO",{leaf("UNAM",utf16(L"Audiopath"))}));
    // Frozen dmusici.h GUID_Synth_Default / GUID_Buffer_Stereo and
    // dmusicf.h predefined buffer flag2: DSBC is optional for this type.
    const GUID synth={0x26bb9432,0x45fe,0x48d3,{0xa3,0x75,0x24,0x72,0xc5,0xe3,0xe7,0x86}};
    const GUID stereo={0x186cc545,0xdb29,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
    Bytes port=guid_bytes(synth);port.resize(28);put32(port,20,16);put32(port,24,2);
    Bytes route(16);put32(route,4,16);put32(route,8,1);const auto buffer=guid_bytes(stereo);route.insert(route.end(),buffer.begin(),buffer.end());
    Bytes parameters(36);put32(parameters,0,36);put32(parameters,4,2);put32(parameters,12,1);
    root_.children.push_back(list("pcsl",{list("pcfl",{leaf("pcfh",port),leaf("pprh",parameters),list("pchl",{leaf("pchh",route)})})}));
    Bytes attributes=buffer;attributes.resize(20);put32(attributes,16,2);root_.children.push_back(list("dbfl",{leaf("ddah",attributes)}));saved_=save_bytes();
}
std::vector<AudioBufferId> AudioPathDocument::buffers() const {
    std::vector<AudioBufferId> result;for(const auto& c:root_.children)if(c.id=="LIST"&&c.type=="dbfl"){
        const auto attr=unique(c,"ddah");if(!attr||attr->data.size()<20)throw std::runtime_error("AudioPath buffer attributes missing");const auto id=identity(attr->data);
        if(std::find(result.begin(),result.end(),id)!=result.end())throw std::runtime_error("Duplicate AudioPath buffer identity");
        const auto descriptor=unique(c,"RIFF","DSBC");if(!(read32(attr->data,16)&2)&&!descriptor)throw std::runtime_error("AudioPath custom buffer descriptor missing");
        if(descriptor)if(const auto guid=unique(*descriptor,"guid"))if(guid->data.size()!=16||identity(guid->data)!=id)throw std::runtime_error("AudioPath buffer descriptor identity mismatch");result.push_back(id);
    }return result;
}
std::vector<AudioPathPort> AudioPathDocument::ports() const {
    const auto available=buffers();std::vector<AudioPathPort> result;const auto ports=unique(root_,"LIST","pcsl");if(!ports)return result;
    for(const auto& c:ports->children)if(c.id=="LIST"&&c.type=="pcfl"){
        const auto header=unique(c,"pcfh");if(!header||header->data.size()<28)throw std::runtime_error("AudioPath port header missing");
        const auto parameters=unique(c,"pprh");if(!parameters||parameters->data.size()<32||read32(parameters->data,0)<32||read32(parameters->data,0)>parameters->data.size())throw std::runtime_error("AudioPath port parameters missing or truncated");
        AudioPathPort port{read32(header->data,16),read32(header->data,20),read32(header->data,24),{}};
        if(!port.count||static_cast<std::uint64_t>(port.base)+port.count>0x100000000ull)throw std::runtime_error("AudioPath port range invalid");
        for(const auto& other:result)if(static_cast<std::uint64_t>(port.base)<static_cast<std::uint64_t>(other.base)+other.count&&static_cast<std::uint64_t>(other.base)<static_cast<std::uint64_t>(port.base)+port.count)throw std::runtime_error("Overlapping AudioPath port ranges");
        if(const auto channels=unique(c,"LIST","pchl"))for(const auto& r:channels->children)if(r.id=="pchh"){
            if(r.data.size()<16)throw std::runtime_error("Truncated AudioPath route");AudioPathRoute route{read32(r.data,0),read32(r.data,4),read32(r.data,12),{}};const auto count=read32(r.data,8);
            if(!route.count||route.base<port.base||static_cast<std::uint64_t>(route.base)+route.count>static_cast<std::uint64_t>(port.base)+port.count||count>(r.data.size()-16)/16)throw std::runtime_error("AudioPath route range or buffer count invalid");
            for(const auto& other:port.routes)if(static_cast<std::uint64_t>(route.base)<static_cast<std::uint64_t>(other.base)+other.count&&static_cast<std::uint64_t>(other.base)<static_cast<std::uint64_t>(route.base)+route.count)throw std::runtime_error("Overlapping AudioPath routes");
            for(size_t i=0;i<count;++i){const auto id=identity(r.data,16+i*16);if(std::find(available.begin(),available.end(),id)==available.end())throw std::runtime_error("AudioPath route buffer is missing");if(std::find(route.buffers.begin(),route.buffers.end(),id)!=route.buffers.end())throw std::runtime_error("Repeated AudioPath route buffer");route.buffers.push_back(id);}port.routes.push_back(std::move(route));
        }result.push_back(std::move(port));
    }return result;
}
std::vector<AudioPathBuffer> AudioPathDocument::buffer_details() const {
    const auto ids=buffers();const auto routing=ports();std::vector<AudioPathBuffer> result;size_t index=0;
    // SDK Stereo and the clone's owned stereo Environmental Reverb
    // realization; legacy EnvReverb DSP/format parity is unverified.
    const GUID stereo={0x186cc545,0xdb29,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
    for(const auto& item:root_.children)if(item.id=="LIST"&&item.type=="dbfl"){
        const auto attributes=unique(item,"ddah");const auto flags=read32(attributes->data,16);
        std::uint16_t channels=0;size_t buses=0;
        if(flags&2){if(ids[index]==identity(guid_bytes(stereo))||ids[index]==identity(guid_bytes(environmentalReverbBuffer)))channels=2;}
        else if(const auto descriptor=unique(item,"RIFF","DSBC")){
            const auto description=unique(*descriptor,"dsbd");
            if(description&&description->data.size()>=20)channels=static_cast<std::uint16_t>(description->data[4]|(std::uint16_t(description->data[5])<<8));
            if(const auto bus=unique(*descriptor,"bsid")){
                if(bus->data.size()%4)throw std::runtime_error("Truncated AudioPath synth bus list");
                buses=bus->data.size()/4;
            }
        }
        bool routed=false;for(const auto& port:routing)for(const auto& route:port.routes)
            if(std::find(route.buffers.begin(),route.buffers.end(),ids[index])!=route.buffers.end())routed=true;
        result.push_back({ids[index++],flags,channels,buses,routed});
    }return result;
}
std::wstring AudioPathDocument::name() const {if(const auto info=unique(root_,"LIST","UNFO"))if(const auto n=unique(*info,"UNAM"))return decode_utf16(n->data);return L"";}
void AudioPathDocument::load(const Bytes& bytes){auto next=Chunk::parse(bytes);if(next.id!="RIFF"||next.type!="DMAP")throw std::runtime_error("Expected DMAP AudioPath");AudioPathDocument check;check.root_=next;if(const auto id=unique(next,"guid"))if(id->data.size()!=16)throw std::runtime_error("AudioPath GUID size invalid");(void)check.ports();(void)check.effects();(void)check.name();(void)check.tool_graph();root_=std::move(next);saved_=bytes;undo_.clear();redo_.clear();}
void AudioPathDocument::save(const std::wstring& path){const auto bytes=save_bytes();write_file_atomic(path,bytes);saved_=bytes;}
bool AudioPathDocument::commit(Chunk next){const auto before=save_bytes();if(next.encode()==before)return false;undo_.push_back(before);if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;}
bool AudioPathDocument::set_name(const std::wstring& name){if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)return false;auto next=root_;if(!next.find("LIST","UNFO"))next.children.push_back(list("UNFO",{}));auto info=next.find("LIST","UNFO");if(!info->find("UNAM"))info->children.push_back(leaf("UNAM",{}));info->find("UNAM")->data=utf16(name);return commit(std::move(next));}
bool AudioPathDocument::set_route_buffers(size_t port,size_t route,const std::vector<AudioBufferId>& buffers){
    const auto parsed=ports();if(port>=parsed.size()||route>=parsed[port].routes.size()||buffers.empty()||buffers.size()>1000)return false;const auto available=this->buffers();std::vector<AudioBufferId> seen;
    const auto details=buffer_details();
    for(const auto& id:buffers){const auto found=std::find(available.begin(),available.end(),id);if(found==available.end()||std::find(seen.begin(),seen.end(),id)!=seen.end())return false;if(details[static_cast<size_t>(found-available.begin())].flags&8)return false;seen.push_back(id);}auto next=root_;auto ports=next.find("LIST","pcsl");size_t pi=0;
    for(auto& p:ports->children)if(p.id=="LIST"&&p.type=="pcfl"&&pi++==port){size_t ri=0;for(auto& r:p.find("LIST","pchl")->children)if(r.id=="pchh"&&ri++==route){const auto old=read32(r.data,8);Bytes data(r.data.begin(),r.data.begin()+16);put32(data,8,static_cast<std::uint32_t>(buffers.size()));for(const auto& id:buffers)data.insert(data.end(),id.begin(),id.end());data.insert(data.end(),r.data.begin()+16+old*16,r.data.end());r.data=std::move(data);break;}break;}return commit(std::move(next));
}
bool AudioPathDocument::set_port_range(size_t port,std::uint32_t base,std::uint32_t count){
    const auto parsed=ports();if(port>=parsed.size()||!count||static_cast<std::uint64_t>(base)+count>0x100000000ull)return false;
    const auto& old=parsed[port];for(const auto& r:old.routes)if(static_cast<std::uint64_t>(r.base-old.base)+r.count>count)return false;
    for(size_t i=0;i<parsed.size();++i)if(i!=port&&static_cast<std::uint64_t>(base)<static_cast<std::uint64_t>(parsed[i].base)+parsed[i].count&&static_cast<std::uint64_t>(parsed[i].base)<static_cast<std::uint64_t>(base)+count)return false;
    auto next=root_;size_t pi=0;for(auto& p:next.find("LIST","pcsl")->children)if(p.id=="LIST"&&p.type=="pcfl"&&pi++==port){auto& h=p.find("pcfh")->data;put32(h,16,base);put32(h,20,count);if(auto channels=p.find("LIST","pchl"))for(auto& r:channels->children)if(r.id=="pchh")put32(r.data,0,base+(read32(r.data,0)-old.base));break;}
    return commit(std::move(next));
}
bool AudioPathDocument::set_route_range(size_t port,size_t route,std::uint32_t base,std::uint32_t count){
    const auto parsed=ports();if(port>=parsed.size()||route>=parsed[port].routes.size()||!count)return false;const auto& p=parsed[port];
    if(base<p.base||static_cast<std::uint64_t>(base)+count>static_cast<std::uint64_t>(p.base)+p.count)return false;
    for(size_t i=0;i<p.routes.size();++i)if(i!=route&&static_cast<std::uint64_t>(base)<static_cast<std::uint64_t>(p.routes[i].base)+p.routes[i].count&&static_cast<std::uint64_t>(p.routes[i].base)<static_cast<std::uint64_t>(base)+count)return false;
    auto next=root_;size_t pi=0;for(auto& pc:next.find("LIST","pcsl")->children)if(pc.id=="LIST"&&pc.type=="pcfl"&&pi++==port){size_t ri=0;for(auto& r:pc.find("LIST","pchl")->children)if(r.id=="pchh"&&ri++==route){put32(r.data,0,base);put32(r.data,4,count);break;}break;}
    return commit(std::move(next));
}
Bytes AudioPathDocument::tool_graph() const {const auto graph=unique(root_,"RIFF","DMTG");if(!graph)return {};ToolGraphDocument check;check.load(graph->encode());return check.save_bytes();}
bool AudioPathDocument::set_tool_graph(const Bytes& bytes){ToolGraphDocument check;check.load(bytes);auto next=root_;if(auto graph=next.find("RIFF","DMTG"))*graph=Chunk::parse(check.save_bytes());else next.children.push_back(Chunk::parse(check.save_bytes()));return commit(std::move(next));}
bool AudioPathDocument::remove_tool_graph(){auto next=root_;next.children.erase(std::remove_if(next.children.begin(),next.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMTG";}),next.children.end());return commit(std::move(next));}
std::vector<AudioPathEffect> AudioPathDocument::effects() const {
    (void)buffers();std::vector<AudioPathEffect> result;size_t buffer=0;
    for(const auto& item:root_.children)if(item.id=="LIST"&&item.type=="dbfl"){
        const auto descriptor=unique(item,"RIFF","DSBC");
        if(descriptor)if(const auto effects=unique(*descriptor,"LIST","fxls")){size_t index=0;
            for(const auto& effect:effects->children)if(effect.id=="RIFF"&&effect.type=="DSFX"){
                const auto h=unique(effect,"fxhr");if(!h||h->data.size()<56)throw std::runtime_error("Truncated AudioPath effect header");
                if(read32(h->data,52)||std::any_of(h->data.begin()+20,h->data.begin()+36,[](auto b){return b!=0;}))throw std::runtime_error("AudioPath effect reserved fields must be zero");
                result.push_back({buffer,index++,identity(h->data,4),read32(h->data,0),identity(h->data,36)});
            }
        }++buffer;
    }return result;
}
bool AudioPathDocument::add_mixin_buffer(std::uint32_t channels){
    // nChannels is a WORD in DSOUND_IO_DSBUFFERDESC. Runtime format support
    // is a separate check; document editing must not impose a stereo limit.
    if(!channels||channels>0xffff)return false;
    GUID fresh{};if(FAILED(CoCreateGuid(&fresh)))throw std::runtime_error("Cannot create mix-in buffer identity");
    const auto id=guid_bytes(fresh);Bytes attributes=id;attributes.resize(20);put32(attributes,16,8);
    Bytes description(20);put32(description,0,0x000180c0);description[4]=static_cast<std::uint8_t>(channels);description[5]=static_cast<std::uint8_t>(channels>>8);
    Chunk descriptor;descriptor.id="RIFF";descriptor.type="DSBC";
    // No bsid: the destination is fed by Send, never by a synthesizer bus.
    descriptor.children={leaf("guid",id),leaf("dsbd",description)};
    auto next=root_;next.children.push_back(list("dbfl",{leaf("ddah",attributes),std::move(descriptor)}));
    AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
std::optional<size_t> AudioPathDocument::environmental_reverb_buffer() const {
    const auto details=buffer_details();const auto env=identity(guid_bytes(environmentalReverbBuffer));
    for(size_t i=0;i<details.size();++i)if(details[i].id==env&&(details[i].flags&2))return i;return {};
}
bool AudioPathDocument::add_environmental_reverb_buffer(){
    const auto ids=buffers();const auto env=identity(guid_bytes(environmentalReverbBuffer));if(std::find(ids.begin(),ids.end(),env)!=ids.end())return false;
    Bytes attr=guid_bytes(environmentalReverbBuffer);attr.resize(20);put32(attr,16,2|8);
    auto next=root_;next.children.push_back(list("dbfl",{leaf("ddah",attr)}));AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
std::optional<size_t> AudioPathDocument::default_send_destination(size_t buffer) const {
    const auto eligible=available_send_destinations(buffer);if(eligible.empty())return {};const auto env=environmental_reverb_buffer();
    if(env&&std::find(eligible.begin(),eligible.end(),*env)!=eligible.end())return env;return eligible.front();
}
std::vector<size_t> AudioPathDocument::send_destinations(size_t buffer,size_t effect) const {
    const auto fx=effects();const AudioBufferId empty{};
    const auto selected=std::find_if(fx.begin(),fx.end(),[&](const auto& f){return f.buffer==buffer&&f.index==effect;});
    if(selected==fx.end()||selected->sendBuffer==empty)return {};
    return available_send_destinations(buffer);
}
std::vector<size_t> AudioPathDocument::available_send_destinations(size_t buffer) const {
    const auto details=buffer_details();const auto fx=effects();const AudioBufferId empty{};
    if(buffer>=details.size()||!details[buffer].channels)return {};
    std::vector<size_t> eligible;
    for(size_t destination=0;destination<details.size();++destination){
        const auto& d=details[destination];const auto channels=details[buffer].channels;
        if(destination==buffer||!(d.flags&8)||d.routed||d.synthBuses||!d.channels)continue;
        if(channels==1?(d.channels!=1&&d.channels!=2):d.channels!=channels)continue;
        // Only local identities are traversed. Imported cross-AudioPath Send
        // references remain in the source; this editor does not invent their
        // external lifetime/format. Reject a newly introduced local cycle.
        std::vector<bool> visited(details.size());
        std::function<bool(size_t)> reachesSource=[&](size_t at){
            if(at==buffer)return true;if(visited[at])return false;visited[at]=true;
            for(const auto& edge:fx)if(edge.buffer==at&&edge.sendBuffer!=empty){
                const auto next=std::find_if(details.begin(),details.end(),[&](const auto& b){return b.id==edge.sendBuffer;});
                if(next!=details.end()&&reachesSource(static_cast<size_t>(next-details.begin())))return true;
            }return false;
        };
        if(!reachesSource(destination))eligible.push_back(destination);
    }return eligible;
}
bool AudioPathDocument::set_send_destination(size_t buffer,size_t effect,size_t destination){
    const auto eligible=send_destinations(buffer,effect);
    if(std::find(eligible.begin(),eligible.end(),destination)==eligible.end())return false;
    const auto target=buffers()[destination];auto next=root_;size_t b=0;
    for(auto& item:next.children)if(item.id=="LIST"&&item.type=="dbfl"&&b++==buffer){
        auto fx=item.find("RIFF","DSBC")->find("LIST","fxls");size_t index=0;
        for(auto& f:fx->children)if(f.id=="RIFF"&&f.type=="DSFX"&&index++==effect){
            auto& header=f.find("fxhr")->data;std::copy(target.begin(),target.end(),header.begin()+36);break;
        }break;
    }
    AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
bool AudioPathDocument::add_send(size_t buffer,size_t destination,size_t before,std::int32_t attenuation){
    if(attenuation<-10000||attenuation>0)return false;
    const auto eligible=available_send_destinations(buffer);
    if(std::find(eligible.begin(),eligible.end(),destination)==eligible.end())return false;
    return insert_effect(buffer,identity(guid_bytes(producer::compat::directSoundSendClass)),buffers()[destination],attenuation,before,true);
}
std::optional<std::int32_t> AudioPathDocument::send_attenuation(size_t buffer,size_t effect) const {
    size_t b=0;for(const auto& item:root_.children)if(item.id=="LIST"&&item.type=="dbfl"&&b++==buffer){
        const auto descriptor=item.find("RIFF","DSBC");if(!descriptor)return {};
        const auto fx=descriptor->find("LIST","fxls");if(!fx)return {};size_t i=0;
        for(const auto& f:fx->children)if(f.id=="RIFF"&&f.type=="DSFX"&&i++==effect){
            const auto h=f.find("fxhr");if(!h||identity(h->data,4)!=identity(guid_bytes(producer::compat::directSoundSendClass)))return {};
            const auto data=unique(f,"data");if(!data)return 0;if(data->data.size()!=4)return {};
            const auto value=static_cast<std::int32_t>(read32(data->data,0));return value>=-10000&&value<=0?std::optional<std::int32_t>(value):std::nullopt;
        }return {};
    }return {};
}
bool AudioPathDocument::set_send_attenuation(size_t buffer,size_t effect,std::int32_t attenuation){
    if(attenuation<-10000||attenuation>0)return false;const auto old=send_attenuation(buffer,effect);if(!old||*old==attenuation)return false;
    auto next=root_;size_t b=0;for(auto& item:next.children)if(item.id=="LIST"&&item.type=="dbfl"&&b++==buffer){
        auto fx=item.find("RIFF","DSBC")->find("LIST","fxls");size_t i=0;
        for(auto& f:fx->children)if(f.id=="RIFF"&&f.type=="DSFX"&&i++==effect){
            if(!f.find("data"))f.children.push_back(leaf("data",Bytes(4)));
            put32(f.find("data")->data,0,static_cast<std::uint32_t>(attenuation));break;
        }break;
    }AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
bool AudioPathDocument::add_file_output(size_t buffer){
    return add_default_effect(buffer,identity(guid_bytes(fileOutputClass)));
}
bool AudioPathDocument::add_waves_reverb(size_t buffer){
    return add_default_effect(buffer,identity(guid_bytes(producer::compat::wavesReverbClass)));
}
bool AudioPathDocument::add_default_effect(size_t buffer,const AudioBufferId& classId){
    return insert_effect(buffer,classId,{},std::nullopt,static_cast<size_t>(-1),false);
}
bool AudioPathDocument::insert_effect(size_t buffer,const AudioBufferId& classId,const AudioBufferId& sendBuffer,std::optional<std::int32_t> attenuation,size_t before,bool allowDuplicate){
    const auto available=buffers();if(buffer>=available.size())return false;
    const auto present=effects();size_t count=0;for(const auto& effect:present)if(effect.buffer==buffer){++count;if(!allowDuplicate&&effect.classId==classId)return false;}
    if(before==static_cast<size_t>(-1))before=count;if(before>count)return false;
    auto next=root_;size_t current=0;
    for(auto& item:next.children)if(item.id=="LIST"&&item.type=="dbfl"&&current++==buffer){
        auto attributes=item.find("ddah");
        if(read32(attributes->data,16)&2){
            // A predefined descriptor is ignored by the runtime. Materialize an
            // owned stereo buffer and rewrite every reference to this identity.
            const GUID stereo={0x186cc545,0xdb29,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
            if(available[buffer]!=identity(guid_bytes(stereo))||item.find("RIFF","DSBC"))return false;
            GUID fresh{};if(FAILED(CoCreateGuid(&fresh)))throw std::runtime_error("Cannot create effect buffer identity");
            const auto replacement=guid_bytes(fresh);std::copy(replacement.begin(),replacement.end(),attributes->data.begin());put32(attributes->data,16,read32(attributes->data,16)&~2u);
            if(auto ports=next.find("LIST","pcsl"))for(auto& port:ports->children)if(auto routes=port.find("LIST","pchl"))for(auto& route:routes->children)if(route.id=="pchh")
                for(size_t i=0;i<read32(route.data,8);++i)if(identity(route.data,16+i*16)==available[buffer])std::copy(replacement.begin(),replacement.end(),route.data.begin()+16+i*16);
            // Imported Send effects also own references to this identity.
            // Keep class, options, data, order and every other header byte.
            for(auto& owner:next.children)if(owner.id=="LIST"&&owner.type=="dbfl")if(auto ds=owner.find("RIFF","DSBC"))if(auto fx=ds->find("LIST","fxls"))
                for(auto& effect:fx->children)if(effect.id=="RIFF"&&effect.type=="DSFX")if(auto h=effect.find("fxhr"))if(identity(h->data,36)==available[buffer])std::copy(replacement.begin(),replacement.end(),h->data.begin()+36);
            Chunk descriptor;descriptor.id="RIFF";descriptor.type="DSBC";
            Bytes desc(20);put32(desc,0,0x000182c0);desc[4]=2;
            descriptor.children={leaf("guid",replacement),leaf("dsbd",desc)};
            if(!(read32(attributes->data,16)&8)){Bytes buses(8);put32(buses,4,1);descriptor.children.push_back(leaf("bsid",buses));}
            item.children.push_back(std::move(descriptor));
        }
        auto descriptor=item.find("RIFF","DSBC");if(!descriptor)return false;
        const auto description=descriptor->find("dsbd");if(!description||description->data.size()<20)return false;
        // Preserve existing effects/order. No data chunk means factory defaults;
        // custom serialized parameter editing is a separate contract.
        put32(description->data,0,read32(description->data,0)|0x200);
        if(!descriptor->find("LIST","fxls"))descriptor->children.push_back(list("fxls",{}));
        Bytes header(56);std::copy(classId.begin(),classId.end(),header.begin()+4);std::copy(sendBuffer.begin(),sendBuffer.end(),header.begin()+36);
        Chunk effect;effect.id="RIFF";effect.type="DSFX";effect.children={leaf("fxhr",header)};
        if(attenuation){Bytes data(4);put32(data,0,static_cast<std::uint32_t>(*attenuation));effect.children.push_back(leaf("data",data));}
        auto& children=descriptor->find("LIST","fxls")->children;auto position=children.end();size_t at=0;
        for(auto it=children.begin();it!=children.end();++it)if(it->id=="RIFF"&&it->type=="DSFX"&&at++==before){position=it;break;}
        children.insert(position,std::move(effect));break;
    }
    AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
static Bytes prepare_environmental_runtime(const Bytes& bytes){
    AudioPathDocument document;document.load(bytes);const auto details=document.buffer_details();
    auto root=Chunk::parse(bytes);size_t at=0;const auto env=identity(guid_bytes(environmentalReverbBuffer));
    for(auto& c:root.children)if(c.id=="LIST"&&c.type=="dbfl"){
        const auto& b=details[at++];if(b.id!=env||!(b.flags&2))continue;const auto attr=c.find("ddah");
        if(b.flags!=10||b.routed||b.synthBuses||attr->data.size()!=20||c.find("RIFF","DSBC"))
            throw std::runtime_error("Environmental Reverb requires an unshared predefined mix-in without routes or overrides");
        // Native bytes are immutable; this private copy owns a wet-only source
        // DMO bridge to declared Windows XAudio2 with SDK default conversion.
        put32(attr->data,16,8);Bytes desc(20);put32(desc,0,0x000182c0);desc[4]=2;
        Bytes fxHeader(56);const auto cls=guid_bytes(environmentalReverbRuntimeClass);std::copy(cls.begin(),cls.end(),fxHeader.begin()+4);
        Chunk effect;effect.id="RIFF";effect.type="DSFX";effect.children={leaf("fxhr",fxHeader)};
        Chunk ds;ds.id="RIFF";ds.type="DSBC";ds.children={leaf("guid",guid_bytes(environmentalReverbBuffer)),leaf("dsbd",desc),list("fxls",{effect})};c.children.push_back(std::move(ds));
    }return root.encode();
}
Bytes prepare_audio_path_send_runtime(const Bytes& bytes){
    if(bytes.empty())return bytes;const auto runtimeBytes=prepare_environmental_runtime(bytes);AudioPathDocument document;document.load(runtimeBytes);const auto ids=document.buffers();
    std::vector<std::vector<size_t>> dependencies(ids.size());bool hasSend=false;
    const auto sendClass=identity(guid_bytes(producer::compat::directSoundSendClass));
    for(const auto& effect:document.effects())if(effect.classId==sendClass){
        hasSend=true;if(effect.flags||!document.send_attenuation(effect.buffer,effect.index))throw std::runtime_error("Unsupported Send options or attenuation data");
        const auto found=std::find(ids.begin(),ids.end(),effect.sendBuffer);
        if(found==ids.end())throw std::runtime_error("External Send destination lifetime has not been declared");
        const auto target=static_cast<size_t>(found-ids.begin());const auto eligible=document.available_send_destinations(effect.buffer);
        if(std::find(eligible.begin(),eligible.end(),target)==eligible.end())throw std::runtime_error("Send destination has invalid channels, route, bus or cycle");
        dependencies[effect.buffer].push_back(target);
    }if(!hasSend)return runtimeBytes;
    std::vector<unsigned char> state(ids.size());std::vector<size_t> order;
    std::function<void(size_t)> visit=[&](size_t at){if(state[at]==2)return;if(state[at]==1)throw std::runtime_error("Local Send loop");state[at]=1;for(const auto next:dependencies[at])visit(next);state[at]=2;order.push_back(at);};
    for(size_t i=0;i<ids.size();++i)visit(i);
    auto root=Chunk::parse(runtimeBytes);std::vector<Chunk> buffers;for(const auto& c:root.children)if(c.id=="LIST"&&c.type=="dbfl")buffers.push_back(c);
    size_t i=0;for(auto& c:root.children)if(c.id=="LIST"&&c.type=="dbfl")c=std::move(buffers[order[i++]]);
    return root.encode();
}
static std::vector<AudioPathRecordingTarget> audio_path_effect_targets(const Bytes& bytes,const Bytes& runtimeBytes,const AudioBufferId& effectClass,bool unique){
    AudioPathDocument source,runtimeCopy;source.load(bytes);runtimeCopy.load(runtimeBytes);const auto ids=source.buffers(),runtimeIds=runtimeCopy.buffers();
    const auto details=source.buffer_details(),runtimeDetails=runtimeCopy.buffer_details();std::vector<size_t> selected;
    for(const auto& effect:source.effects())if(effect.classId==effectClass){
        if(std::find(selected.begin(),selected.end(),effect.buffer)!=selected.end()){
            if(unique)throw std::runtime_error("Multiple FileOutput effects in one buffer are ambiguous");
        }else selected.push_back(effect.buffer);
    }
    if(effectClass==identity(guid_bytes(environmentalReverbRuntimeClass)))if(const auto env=source.environmental_reverb_buffer())selected.push_back(*env);
    std::vector<AudioPathRecordingTarget> targets;const auto contains=[&](size_t buffer){return std::any_of(targets.begin(),targets.end(),[&](const auto& t){return t.buffer==buffer;});};
    for(const auto& port:source.ports())for(const auto& route:port.routes)for(size_t i=0;i<route.buffers.size();++i){
        const auto buffer=static_cast<size_t>(std::find(ids.begin(),ids.end(),route.buffers[i])-ids.begin());
        if(std::find(selected.begin(),selected.end(),buffer)!=selected.end()&&!contains(buffer))targets.push_back({buffer,route.base,0x6100,static_cast<std::uint32_t>(i)});
    }
    for(size_t buffer=0;buffer<details.size();++buffer)if(std::find(selected.begin(),selected.end(),buffer)!=selected.end()&&!contains(buffer)){
        if(!(details[buffer].flags&8)||details[buffer].routed||details[buffer].synthBuses)throw std::runtime_error("Disconnected effect buffer is not a valid mix-in");
        std::uint32_t index=0;bool found=false;for(size_t i=0;i<runtimeDetails.size();++i)if(runtimeDetails[i].flags&8){if(runtimeIds[i]==ids[buffer]){found=true;break;}++index;}
        if(!found)throw std::runtime_error("Recording mix-in is missing from runtime copy");targets.push_back({buffer,0,0x7100,index});
    }return targets;
}
std::vector<AudioPathRecordingTarget> audio_path_file_output_targets(const Bytes& bytes,const Bytes& runtimeBytes){
    return audio_path_effect_targets(bytes,runtimeBytes,identity(guid_bytes(fileOutputClass)),true);
}
std::vector<AudioPathRecordingTarget> audio_path_waves_reverb_targets(const Bytes& bytes,const Bytes& runtimeBytes){
    return audio_path_effect_targets(bytes,runtimeBytes,identity(guid_bytes(producer::compat::wavesReverbClass)),false);
}
std::vector<AudioPathRecordingTarget> audio_path_environmental_reverb_targets(const Bytes& bytes,const Bytes& runtimeBytes){
    return audio_path_effect_targets(bytes,runtimeBytes,identity(guid_bytes(environmentalReverbRuntimeClass)),true);
}
bool AudioPathDocument::undo(){if(undo_.empty())return false;redo_.push_back(save_bytes());root_=Chunk::parse(undo_.back());undo_.pop_back();return true;}
bool AudioPathDocument::redo(){if(redo_.empty())return false;undo_.push_back(save_bytes());root_=Chunk::parse(redo_.back());redo_.pop_back();return true;}
}
