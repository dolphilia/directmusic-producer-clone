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
// Source-owned range clipboard: [begin,end), anchored at begin. Copies all
// timed MIDI and curve records, including their extensions and signed offsets.
// Unknown subchunks stay in the source/destination, never silently duplicated.
Bytes sequence_copy_range(const Bytes&,std::int32_t begin,std::int32_t end);
Bytes sequence_delete_range(const Bytes&,std::int32_t begin,std::int32_t end);
bool sequence_range_empty(const Bytes&);
Bytes sequence_paste_range(const Bytes& destination,const Bytes& clipboard,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length=2147483647);
Chunk sequence_track();
Chunk gm_piano_band_track();
bool is_sequence_track(const Chunk&);
}
