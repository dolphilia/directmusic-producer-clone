#pragma once
#include "riff.h"
#include <array>
#include <optional>
namespace producer::app {
// Time and duration retain the track's existing clock domain. Start offset is
// always in 100 ns units. Timing mode, loop flags and variation control are
// deliberately not changed by this placement/trim operation.
struct WavePlacement {
    std::int64_t time=0,startOffset=0,duration=1;
    std::int32_t volume=0,pitch=0;
};
struct WaveItem {
    size_t part=0,index=0;
    std::uint32_t pchannel=0,variations=0;
    WavePlacement placement;
    std::wstring filename;
    bool clockTime=false;
    std::optional<std::array<std::uint8_t,16>> objectId;
};
std::vector<WaveItem> wave_items(const Chunk&);
struct WaveReference { std::wstring filename; std::optional<std::array<std::uint8_t,16>> objectId; };
bool insert_wave_reference(Chunk&,size_t part,const WaveReference&,const WavePlacement&,std::uint32_t variations,size_t* resultingIndex=nullptr);
bool edit_wave_placement(Chunk&,size_t part,size_t index,const WavePlacement&);
Bytes copy_wave_event(const Chunk&,size_t part,size_t index);
bool paste_wave_event(Chunk&,const Bytes&,size_t part,std::int64_t time,size_t* resultingIndex=nullptr);
bool delete_wave_event(Chunk&,size_t part,size_t index);
Bytes wave_track_identity();
}
