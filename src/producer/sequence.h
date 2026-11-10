#pragma once
#include "riff.h"
namespace producer::app {
// PBend uses a signed UI domain; IO values add8192. Other controller values
// are native. RPN/NRPN parameter stores two separate7-bit bytes, not packed14.
struct SequenceController {
    std::uint8_t type=3, data=0;
    std::uint16_t parameter=0;
    bool operator==(const SequenceController& b) const {return type==b.type&&data==b.data&&parameter==b.parameter;}
    bool operator<(const SequenceController& b) const {return type!=b.type?type<b.type:data!=b.data?data<b.data:parameter<b.parameter;}
};
struct SequenceCurve {
    std::int32_t time=0,duration=1,resetDuration=0;
    std::uint32_t channel=0;
    std::int16_t start=0,end=0,reset=0;
    SequenceController controller;
    std::uint8_t shape=0,flags=0;
    std::uint16_t merge=0;
};
int sequence_curve_minimum(SequenceController);
int sequence_curve_maximum(SequenceController);
bool valid_sequence_curve(const SequenceCurve&);
double sequence_curve_value(const SequenceCurve&,double fraction);
std::vector<SequenceCurve> sequence_curves(const Bytes& payload);
Bytes sequence_insert_curve(const Bytes&,const SequenceCurve&,size_t* resultingIndex=nullptr);
Bytes sequence_change_curve(const Bytes&,size_t,const SequenceCurve&,size_t* resultingIndex=nullptr);
Bytes sequence_delete_curve(const Bytes&,size_t);
struct Note {std::int32_t time,duration;std::uint32_t channel;std::uint8_t pitch,velocity;};
// Note.time is the physical onset. The native raw time is logical ownership;
// its signed offset can place that onset before or after the owning beat.
struct SequenceNotePosition {Note note;std::int32_t logicalTime;std::int16_t offset;};
std::vector<SequenceNotePosition> sequence_note_positions(const Bytes& payload);
Bytes sequence_change_logical_time(const Bytes&,size_t index,std::int32_t logicalTime,size_t* resultingIndex=nullptr);
// Existing MIDI/controller/curve records are retained when inserting notes.
// This is the first Sequence connection, not the full Sequence strip editor.
std::vector<Note> sequence_notes(const Bytes& payload);
Bytes sequence_insert(const Bytes& payload,Note note,size_t* resultingIndex=nullptr);
// Index counts sounding note-on records only; controller/note-off/curve and
// unknown bytes remain owned by the payload. Moving retains the record's
// signed time offset and uses the same raw-clock ordering as insertion.
Bytes sequence_change(const Bytes& payload,size_t index,Note note,size_t* resultingIndex=nullptr);
struct SequenceTimingChange {size_t note;std::int32_t time,duration;};
// Apply original note indexes together before sorting, retaining all non-timing
// fields, signed offsets, extension bytes and unrelated subchunks.
Bytes sequence_change_timings(const Bytes&,const std::vector<SequenceTimingChange>&,std::vector<size_t>* resultingIndices=nullptr);
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
