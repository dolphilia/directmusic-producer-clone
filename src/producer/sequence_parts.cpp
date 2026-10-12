#include "sequence_parts.h"
#include <algorithm>
#include <array>
namespace producer::app { namespace {
constexpr std::array<std::uint8_t,16> designId{0x1d,0xb4,0x1a,0x75,0xe4,0x5f,0x4d,0xc3,0xa4,0xe1,0x53,0x51,0x50,0x43,0x48,0x31};
Chunk channel_design(std::uint32_t channel){Chunk c;c.id="qPCh";c.data.assign(designId.begin(),designId.end());c.data.resize(24);put32(c.data,16,1);put32(c.data,20,channel);return c;}
constexpr std::array<std::uint8_t,16> laneId{0x45,0xaa,0x3b,0xe1,0xe2,0x37,0x47,0x86,0xa5,0x17,0x43,0x43,0x4c,0x41,0x4e,0x31};
std::vector<SequenceLane> owned_lanes(const Chunk& track){
    std::vector<SequenceLane> result;const Chunk* found=nullptr;
    for(const auto& c:track.children)if(c.id=="qCCL"){
        if(found||c.data.size()<24||!std::equal(laneId.begin(),laneId.end(),c.data.begin())||read32(c.data,16)!=1||read32(c.data,20)!=(c.data.size()-24)/8||(c.data.size()-24)%8)throw std::runtime_error("Unsupported or ambiguous Sequence CC lane design data");found=&c;
        for(size_t at=24;at<c.data.size();at+=8){SequenceLane lane{read32(c.data,at),{c.data[at+4],c.data[at+5],static_cast<std::uint16_t>(c.data[at+6]|(c.data[at+7]<<8))}};SequenceCurve probe;probe.channel=lane.channel;probe.controller=lane.controller;
            if(!valid_sequence_curve(probe)||std::any_of(result.begin(),result.end(),[&](const auto& old){return old.channel==lane.channel&&old.controller==lane.controller;}))throw std::runtime_error("Invalid or duplicate Sequence CC lane design record");result.push_back(lane);}
    }return result;
}
}
std::optional<std::uint32_t> sequence_declared_channel(const Chunk& track){
    const Chunk* found=nullptr;for(const auto& c:track.children)if(c.id=="qPCh"){
        if(found||c.data.size()!=24||!std::equal(designId.begin(),designId.end(),c.data.begin())||read32(c.data,16)!=1||read32(c.data,20)>=0xfffffffcu)throw std::runtime_error("Unsupported or ambiguous Sequence part design data");found=&c;}
    return found?std::optional<std::uint32_t>(read32(found->data,20)):std::nullopt;
}
std::vector<SequenceLane> sequence_design_lanes(const Chunk& track){
    auto result=owned_lanes(track);
    // Bounded original observation: one channel0 part, one56-byte psqc, and
    // eight-byte cvsu entries. Do not guess psqc channel offsets or other IDs.
    const auto payload=track.find("seqt");if(!payload)return result;
    const auto declared=sequence_declared_channel(track);if(declared&&*declared!=0)return result;
    const auto notes=sequence_notes(payload->data);const auto curves=sequence_curves(payload->data);
    if(notes.empty()||std::any_of(notes.begin(),notes.end(),[](const auto& n){return n.channel!=0;})||std::any_of(curves.begin(),curves.end(),[](const auto& c){return c.channel!=0;}))return result;
    const Chunk* ui=nullptr;for(const auto& c:track.children)if(c.id=="LIST"&&c.type=="psql"){if(ui)return result;ui=&c;}
    if(!ui||std::count_if(ui->children.begin(),ui->children.end(),[](const auto& c){return c.id=="psqc";})!=1)return result;
    const auto part=ui->find("psqc");if(!part||part->data.size()!=56)return result;
    for(const auto& c:ui->children)if(c.id=="cvsu"&&c.data.size()==8){const auto code=read32(c.data,4);if(code!=128&&code!=11)continue;
        const SequenceLane lane{0,code==128?SequenceController{3,0,0}:SequenceController{4,11,0}};
        if(std::none_of(result.begin(),result.end(),[&](const auto& old){return old.channel==lane.channel&&old.controller==lane.controller;}))result.push_back(lane);}
    return result;
}
bool sequence_add_design_lane(Chunk& track,std::uint32_t channel,SequenceController controller){
    SequenceCurve probe;probe.channel=channel;probe.controller=controller;if(!valid_sequence_curve(probe))return false;
    auto lanes=owned_lanes(track);if(std::any_of(lanes.begin(),lanes.end(),[&](const auto& old){return old.channel==channel&&old.controller==controller;}))return false;
    auto target=track.find("qCCL");if(!target){Chunk c;c.id="qCCL";c.data.assign(laneId.begin(),laneId.end());c.data.resize(24);put32(c.data,16,1);track.children.push_back(c);target=&track.children.back();}
    const auto at=target->data.size();target->data.resize(at+8);put32(target->data,20,static_cast<std::uint32_t>(lanes.size()+1));put32(target->data,at,channel);target->data[at+4]=controller.type;target->data[at+5]=controller.data;target->data[at+6]=static_cast<std::uint8_t>(controller.parameter);target->data[at+7]=static_cast<std::uint8_t>(controller.parameter>>8);return true;
}
std::vector<std::uint32_t> sequence_part_channels(const Chunk& track){
    const auto payload=track.find("seqt");if(!payload||std::count_if(track.children.begin(),track.children.end(),[](const auto& c){return c.id=="seqt";})!=1)throw std::runtime_error("Sequence payload missing or ambiguous");
    std::vector<std::uint32_t> result;if(const auto channel=sequence_declared_channel(track))result.push_back(*channel);
    for(const auto& note:sequence_notes(payload->data))result.push_back(note.channel);
    for(const auto& curve:sequence_curves(payload->data))if(curve.channel<0xfffffffcu)result.push_back(curve.channel);
    for(const auto& lane:owned_lanes(track))result.push_back(lane.channel);
    std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());if(result.empty())result.push_back(0);return result;
}
std::vector<std::uint32_t> used_sequence_channels(const SegmentDocument& source){
    const auto root=Chunk::parse(source.save_bytes());std::vector<std::uint32_t> result;
    if(const auto tracks=root.find("LIST","trkl"))for(const auto& track:tracks->children)if(is_sequence_track(track)&&(read32(track.find("trkh")->data,20)&source.selected_groups())){
        const auto channels=sequence_part_channels(track);result.insert(result.end(),channels.begin(),channels.end());}
    std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
}
std::uint32_t next_sequence_channel(const SegmentDocument& source){const auto used=used_sequence_channels(source);std::uint32_t next=0;for(const auto channel:used){if(channel!=next)break;++next;}if(next>=0xfffffffcu)throw std::runtime_error("No unused PChannel");return next;}
bool SegmentDocument::add_sequence_part(std::uint32_t channel,size_t* resultingTrack){
    if(channel>=0xfffffffcu)return false;const auto used=used_sequence_channels(*this);if(std::binary_search(used.begin(),used.end(),channel))return false;
    const auto before=save_bytes();auto next=*this;auto tracks=next.root_.find("LIST","trkl");if(!tracks){Chunk c;c.id="LIST";c.type="trkl";next.root_.children.push_back(c);tracks=&next.root_.children.back();}
    std::uint32_t position=0;size_t index=0;for(const auto& t:tracks->children)if(t.id=="RIFF"&&t.type=="DMTK"){
        const auto h=t.find("trkh");if(!h||h->data.size()<32||read32(h->data,16)==UINT32_MAX)throw std::runtime_error("Track position is out of range");position=std::max(position,read32(h->data,16)+1);
        if(is_sequence_track(t)&&(read32(h->data,20)&selectedGroups_))++index;}
    auto track=sequence_track();put32(track.find("trkh")->data,16,position);put32(track.find("trkh")->data,20,selectedGroups_);track.children.push_back(channel_design(channel));tracks->children.push_back(std::move(track));
    next.import(next.root_.encode());next.record_edit(before);*this=std::move(next);if(resultingTrack)*resultingTrack=index;return true;
}
}
