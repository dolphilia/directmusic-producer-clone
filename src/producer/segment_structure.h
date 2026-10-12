#pragma once
#include "riff.h"
#include <array>

namespace producer::app {
enum class SegmentTrackKind {band,chord,chordmap,groove,lyric,marker,mute,parameter,pattern,script,trigger,sequence,signpost,style,tempo,meter,wave,unknown};
struct SegmentTrackDescriptor {SegmentTrackKind kind;const wchar_t* name;bool canCreate;};
const std::array<SegmentTrackDescriptor,17>& segment_track_descriptors();
Chunk make_segment_track(SegmentTrackKind);
SegmentTrackKind segment_track_kind(const Chunk&);
struct SegmentTrackInfo {SegmentTrackKind kind;size_t index;std::uint32_t groups,position;};
struct SegmentLengthPreview {std::int32_t clocks=0;std::uint32_t measures=0;std::uint64_t milliseconds=0;};
struct SegmentCreationLength {
    bool clockTime=false;
    std::uint32_t measures=20;
    std::uint64_t milliseconds=40000;
};
// A new empty Segment uses implicit120 BPM and explicit4/4. Existing Segment
// length edits must instead account for their complete tempo/meter maps.
std::int32_t initial_segment_clocks(const SegmentCreationLength&);
}
