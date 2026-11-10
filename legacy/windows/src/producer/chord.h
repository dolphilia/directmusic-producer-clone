#pragma once
#include "riff.h"
#include <optional>
namespace producer::app {
struct SubChord {
    std::uint32_t chordPattern=0x91,scalePattern=0xab5,inversionPoints=0,levels=15;
    std::uint8_t chordRoot=0,scaleRoot=0;
    // Transient provenance for an imported SubChord, including a split level.
    // It is not part of the native structure.
    size_t sourceIndex=SIZE_MAX;
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
std::optional<size_t> chord_level(const ChordEvent&,unsigned level);
bool toggle_chord_tone(ChordEvent&,unsigned level,unsigned note);
bool toggle_chord_scale(ChordEvent&,unsigned level,unsigned pitch);
bool transpose_chord(ChordEvent&,int semitones);
std::vector<ChordEvent> chord_events(const Chunk& cord);
bool set_chord_event(Chunk& cord,ChordEvent,size_t* resultingIndex=nullptr);
bool change_chord_event(Chunk& cord,size_t index,ChordEvent,size_t* resultingIndex=nullptr);
bool delete_chord_event(Chunk& cord,size_t index);
Chunk chord_track();
}
