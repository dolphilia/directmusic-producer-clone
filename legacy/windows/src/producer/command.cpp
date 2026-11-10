#include "command.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <windows.h>
namespace producer::app {
namespace {
size_t stride(const Bytes& b){if(b.size()<4)throw std::runtime_error("Command header truncated");const auto s=read32(b,0);if(s<11||(b.size()-4)%s)throw std::runtime_error("Command stride or records invalid");return s;}
void patch(Bytes& b,size_t p,CommandEvent e,const CommandEvent* previous=nullptr){if(!valid_command(e,previous))throw std::runtime_error("Invalid Command event");put32(b,p,static_cast<std::uint32_t>(e.time));b[p+4]=static_cast<std::uint8_t>(e.measure);b[p+5]=static_cast<std::uint8_t>(e.measure>>8);b[p+6]=e.beat;b[p+7]=e.type;b[p+8]=e.groove;b[p+9]=e.range;b[p+10]=e.repeat;}
size_t insertion(const Bytes& b,size_t s,std::int32_t time){size_t p=4;for(;p<b.size();p+=s)if(static_cast<std::int32_t>(read32(b,p))>time)break;return p;}
}
bool valid_command(CommandEvent e,const CommandEvent* previous){return e.time>=0&&(e.type<=5||(previous&&e.type==previous->type))&&(e.groove<=100||(previous&&e.groove==previous->groove))&&(e.range<=100||(previous&&e.range==previous->range))&&(e.repeat<=5||(previous&&e.repeat==previous->repeat));}
Bytes prepare_command_playback(const Bytes& bytes){
    auto root=Chunk::parse(bytes);auto tracks=root.find("LIST","trkl");bool changed=false;
    if(tracks)for(auto& track:tracks->children){auto command=track.find("cmnd");if(!command)continue;
        const auto events=command_events(command->data);if(events.size()<2||events.front().time<=0)continue;
        bool increasing=true;for(size_t i=1;i<events.size();++i)increasing=increasing&&events[i].time>events[i-1].time;
        if(!increasing)continue;
        const auto s=stride(command->data);const auto source=command->data;
        for(size_t i=0;i<events.size();++i)std::copy_n(source.begin()+4+(events.size()-1-i)*s,s,command->data.begin()+4+i*s);
        changed=true;
    }
    return changed?root.encode():bytes;
}
std::vector<CommandEvent> command_events(const Bytes& b){const auto s=stride(b);std::vector<CommandEvent> out;for(size_t p=4;p<b.size();p+=s)out.push_back({static_cast<std::int32_t>(read32(b,p)),static_cast<std::uint16_t>(b[p+4]|(b[p+5]<<8)),b[p+6],b[p+7],b[p+8],b[p+9],b[p+10]});return out;}
Bytes command_insert(const Bytes& b,CommandEvent e){const auto s=stride(b);Bytes record(s);patch(record,0,e);auto out=b;const auto p=insertion(out,s,e.time);out.insert(out.begin()+p,record.begin(),record.end());return out;}
Bytes command_change(const Bytes& b,size_t index,CommandEvent e,size_t* resultingIndex){const auto s=stride(b);if(index>=(b.size()-4)/s)throw std::out_of_range("Command index");const auto previous=command_events(b)[index];const auto at=4+index*s;Bytes record(b.begin()+at,b.begin()+at+s);patch(record,0,e,&previous);auto out=b;size_t after=index;if(read32(b,at)==static_cast<std::uint32_t>(e.time))std::copy(record.begin(),record.end(),out.begin()+at);else{out.erase(out.begin()+at,out.begin()+at+s);const auto p=insertion(out,s,e.time);after=(p-4)/s;out.insert(out.begin()+p,record.begin(),record.end());}if(resultingIndex)*resultingIndex=after;return out;}
Bytes command_delete(const Bytes& b,size_t index){const auto s=stride(b);if(index>=(b.size()-4)/s)throw std::out_of_range("Command index");auto out=b;const auto at=4+index*s;out.erase(out.begin()+at,out.begin()+at+s);return out;}
Chunk command_track(){Chunk root;root.id="RIFF";root.type="DMTK";Chunk h;h.id="trkh";h.data.resize(32);const GUID id={0xd2ac288c,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};std::memcpy(h.data.data(),&id,16);put32(h.data,20,1);std::memcpy(h.data.data()+24,"cmnd",4);Chunk c;c.id="cmnd";c.data.resize(4);put32(c.data,0,12);root.children={h,c};return root;}
}
