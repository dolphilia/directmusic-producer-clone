#pragma once
#include "riff.h"
namespace producer::app {
struct CommandEvent {std::int32_t time;std::uint16_t measure;std::uint8_t beat,type,groove,range,repeat;};
// Known values for newly supplied fields; unchanged imported values survive.
bool valid_command(CommandEvent,const CommandEvent* previous=nullptr);
std::vector<CommandEvent> command_events(const Bytes&);
Bytes command_insert(const Bytes&,CommandEvent);
Bytes command_change(const Bytes&,size_t,CommandEvent,size_t* resultingIndex=nullptr);
Bytes command_delete(const Bytes&,size_t);
Chunk command_track();
// Windows Command loader requires reverse records for a strictly increasing
// positive-time list without a zero-time anchor. Document bytes stay untouched.
Bytes prepare_command_playback(const Bytes&);
}
