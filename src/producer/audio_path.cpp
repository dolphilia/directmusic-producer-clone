#include "document.h"
#include "audio_path.h"
#include "band.h"
#include <functional>
#include "tool_graph.h"
#include "file_output_dmo.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
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
std::wstring AudioPathDocument::name() const {if(const auto info=unique(root_,"LIST","UNFO"))if(const auto n=unique(*info,"UNAM"))return decode_utf16(n->data);return L"";}
void AudioPathDocument::load(const Bytes& bytes){auto next=Chunk::parse(bytes);if(next.id!="RIFF"||next.type!="DMAP")throw std::runtime_error("Expected DMAP AudioPath");AudioPathDocument check;check.root_=next;if(const auto id=unique(next,"guid"))if(id->data.size()!=16)throw std::runtime_error("AudioPath GUID size invalid");(void)check.ports();(void)check.effects();(void)check.name();(void)check.tool_graph();root_=std::move(next);saved_=bytes;undo_.clear();redo_.clear();}
void AudioPathDocument::save(const std::wstring& path){const auto bytes=save_bytes();write_file_atomic(path,bytes);saved_=bytes;}
bool AudioPathDocument::commit(Chunk next){const auto before=save_bytes();if(next.encode()==before)return false;undo_.push_back(before);if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();root_=std::move(next);return true;}
bool AudioPathDocument::set_name(const std::wstring& name){if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)return false;auto next=root_;if(!next.find("LIST","UNFO"))next.children.push_back(list("UNFO",{}));auto info=next.find("LIST","UNFO");if(!info->find("UNAM"))info->children.push_back(leaf("UNAM",{}));info->find("UNAM")->data=utf16(name);return commit(std::move(next));}
bool AudioPathDocument::set_route_buffers(size_t port,size_t route,const std::vector<AudioBufferId>& buffers){
    const auto parsed=ports();if(port>=parsed.size()||route>=parsed[port].routes.size()||buffers.empty()||buffers.size()>1000)return false;const auto available=this->buffers();std::vector<AudioBufferId> seen;
    for(const auto& id:buffers){if(std::find(available.begin(),available.end(),id)==available.end()||std::find(seen.begin(),seen.end(),id)!=seen.end())return false;seen.push_back(id);}auto next=root_;auto ports=next.find("LIST","pcsl");size_t pi=0;
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
                result.push_back({buffer,index++,identity(h->data,4),read32(h->data,0)});
            }
        }++buffer;
    }return result;
}
bool AudioPathDocument::add_file_output(size_t buffer){
    const auto available=buffers();if(buffer>=available.size())return false;
    const auto classBytes=guid_bytes(fileOutputClass);const auto classId=identity(classBytes);
    for(const auto& effect:effects())if(effect.buffer==buffer&&effect.classId==classId)return false;
    auto next=root_;size_t current=0;
    for(auto& item:next.children)if(item.id=="LIST"&&item.type=="dbfl"&&current++==buffer){
        auto attributes=item.find("ddah");
        if(read32(attributes->data,16)&2){
            // A predefined descriptor is ignored by the runtime. Materialize an
            // owned stereo buffer and rewrite every reference to this identity.
            const GUID stereo={0x186cc545,0xdb29,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
            if(available[buffer]!=identity(guid_bytes(stereo))||item.find("RIFF","DSBC"))return false;
            GUID fresh{};if(FAILED(CoCreateGuid(&fresh)))throw std::runtime_error("Cannot create recording buffer identity");
            const auto replacement=guid_bytes(fresh);std::copy(replacement.begin(),replacement.end(),attributes->data.begin());put32(attributes->data,16,0);
            if(auto ports=next.find("LIST","pcsl"))for(auto& port:ports->children)if(auto routes=port.find("LIST","pchl"))for(auto& route:routes->children)if(route.id=="pchh")
                for(size_t i=0;i<read32(route.data,8);++i)if(identity(route.data,16+i*16)==available[buffer])std::copy(replacement.begin(),replacement.end(),route.data.begin()+16+i*16);
            Chunk descriptor;descriptor.id="RIFF";descriptor.type="DSBC";
            Bytes desc(20);put32(desc,0,0x000182c0);desc[4]=2;Bytes buses(8);put32(buses,4,1);
            descriptor.children={leaf("guid",replacement),leaf("dsbd",desc),leaf("bsid",buses)};item.children.push_back(std::move(descriptor));
        }
        auto descriptor=item.find("RIFF","DSBC");if(!descriptor)return false;
        const auto description=descriptor->find("dsbd");if(!description||description->data.size()<20)return false;
        // Preserve existing effects/order. The tap is appended after them.
        put32(description->data,0,read32(description->data,0)|0x200);
        if(!descriptor->find("LIST","fxls"))descriptor->children.push_back(list("fxls",{}));
        Bytes header(56);std::copy(classBytes.begin(),classBytes.end(),header.begin()+4);
        Chunk effect;effect.id="RIFF";effect.type="DSFX";effect.children={leaf("fxhr",header)};
        descriptor->find("LIST","fxls")->children.push_back(std::move(effect));break;
    }
    AudioPathDocument validated;validated.load(next.encode());return commit(std::move(next));
}
bool AudioPathDocument::undo(){if(undo_.empty())return false;redo_.push_back(save_bytes());root_=Chunk::parse(undo_.back());undo_.pop_back();return true;}
bool AudioPathDocument::redo(){if(redo_.empty())return false;undo_.push_back(save_bytes());root_=Chunk::parse(redo_.back());redo_.pop_back();return true;}
}
