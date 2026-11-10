#pragma once
#include "riff.h"
#include <array>
namespace producer::app {
struct SegmentTrigger {
    std::int32_t logical=0,physical=0;
    std::uint32_t playFlags=0,itemFlags=0;
    bool hasId=false;
    std::array<std::uint8_t,16> objectId{};
    std::wstring filename,name,motif;
};
Chunk segment_trigger_track();
std::vector<SegmentTrigger> segment_triggers(const Chunk&);
bool add_segment_trigger(Chunk&,const SegmentTrigger&);
bool change_segment_trigger(Chunk&,size_t,const SegmentTrigger&);
bool delete_segment_trigger(Chunk&,size_t);
}
