#pragma once
#include "riff.h"
#include <array>
namespace producer::app {
struct ScriptEvent {
    std::int32_t logical=0,physical=0;std::uint32_t timing=1;
    bool hasId=false;std::array<std::uint8_t,16> objectId{};
    std::wstring filename,name,routine;
};
Chunk script_track();
std::vector<ScriptEvent> script_events(const Chunk&);
bool add_script_event(Chunk&,const ScriptEvent&);
bool change_script_event(Chunk&,size_t,const ScriptEvent&);
bool delete_script_event(Chunk&,size_t);
}
