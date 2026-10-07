#pragma once
#include "riff.h"
namespace producer::app {
constexpr std::uint32_t mute_channel_silent=0xffffffffu;
struct MuteEvent {std::int32_t time=0;std::uint32_t channel=0,map=0;};
bool valid_mute(const MuteEvent&);
std::vector<MuteEvent> mute_events(const Chunk&);
bool add_mute_event(Chunk&,MuteEvent,size_t* =nullptr);
bool change_mute_event(Chunk&,size_t,MuteEvent,size_t* =nullptr);
bool delete_mute_event(Chunk&,size_t);
Bytes copy_mute_event(const Chunk&,size_t);
bool paste_mute_event(Chunk&,const Bytes&,std::int32_t,size_t* =nullptr);
Bytes copy_mute_range(const Chunk&,std::int32_t begin,std::int32_t end);
bool delete_mute_range(Chunk&,std::int32_t begin,std::int32_t end);
bool mute_range_empty(const Bytes&,std::int32_t at,std::int32_t span,std::int32_t length);
bool paste_mute_range(Chunk&,const Bytes&,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length);
Chunk mute_track();
}
