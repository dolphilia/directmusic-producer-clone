#pragma once
#include "riff.h"
namespace producer::app {
struct Note {std::int32_t time,duration;std::uint32_t channel;std::uint8_t pitch,velocity;};
// Existing MIDI/controller/curve records are retained when inserting notes.
// This is the first Sequence connection, not the full Sequence strip editor.
std::vector<Note> sequence_notes(const Bytes& payload);
Bytes sequence_insert(const Bytes& payload,Note note);
// Index counts sounding note-on records only; controller/note-off/curve and
// unknown bytes remain owned by the payload. Moving retains the record's
// signed time offset and uses the same raw-clock ordering as insertion.
Bytes sequence_change(const Bytes& payload,size_t index,Note note,size_t* resultingIndex=nullptr);
Bytes sequence_delete(const Bytes& payload,size_t index);
Chunk sequence_track();
Chunk gm_piano_band_track();
bool is_sequence_track(const Chunk&);
}
