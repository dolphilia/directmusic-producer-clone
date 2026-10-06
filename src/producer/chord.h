#pragma once
#include "riff.h"
namespace producer::app {
struct SubChord {
    std::uint32_t chordPattern=0x91,scalePattern=0xab5,inversionPoints=0,levels=15;
    std::uint8_t chordRoot=0,scaleRoot=0;
};
struct ChordEvent {
    std::wstring name=L"M";
    std::int32_t time=0;
    std::uint16_t measure=0;
    std::uint8_t beat=0,flags=0;
    std::vector<SubChord> subchords={SubChord{}};
};
// DMUS_IO_CHORD / DMUS_IO_SUBCHORD, not the Chordmap format.
bool valid_chord(const ChordEvent&,const ChordEvent* previous=nullptr);
std::vector<ChordEvent> chord_events(const Chunk& cord);
bool set_chord_event(Chunk& cord,ChordEvent,size_t* resultingIndex=nullptr);
bool change_chord_event(Chunk& cord,size_t index,ChordEvent,size_t* resultingIndex=nullptr);
bool delete_chord_event(Chunk& cord,size_t index);
Chunk chord_track();
}
