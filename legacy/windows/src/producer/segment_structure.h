#pragma once
#include "riff.h"
#include <array>
#include <optional>

namespace producer::app {
enum class SegmentTrackKind {band,chord,chordmap,groove,lyric,marker,mute,parameter,pattern,script,trigger,sequence,signpost,style,tempo,meter,wave,unknown};
struct SegmentTrackDescriptor {SegmentTrackKind kind;const wchar_t* name;bool canCreate;};
const std::array<SegmentTrackDescriptor,17>& segment_track_descriptors();
Chunk make_segment_track(SegmentTrackKind);
SegmentTrackKind segment_track_kind(const Chunk&);
struct SegmentTrackInfo {SegmentTrackKind kind;size_t index;std::uint32_t groups,position;};
struct SegmentLengthPreview {std::int32_t clocks=0;std::uint32_t measures=0;std::uint64_t milliseconds=0;};
enum class SegmentPerformanceBoundary {immediate,grid,beat,barline,align_beat,align_barline,align_segment,segment_end,queue_end};
enum class SegmentAlignment {time,grid,beat,barline};
// Native invalidation choices: retain all, cut primary only, or cut all and
// regenerate secondary segments. These are independent of the start boundary.
enum class SegmentInvalidation {dont_cut_off,cut_primary,cut_all};
struct SegmentLoopSettings {
    std::uint32_t repeats=0;
    std::int32_t start=0,end=0; // native end0 means the Segment end
    std::uint32_t flags=0;
    bool music_time()const{return (flags&3u)==0;}
};
// Display/source time, independent of physical note offsets and prepare time.
// Unsupported clock-time or invalid loop/seek bounds have no music-time map.
std::optional<std::int32_t> segment_playback_clock(std::int32_t length,const SegmentLoopSettings&,std::int32_t startPoint,std::int64_t elapsed);
struct SegmentCreationLength {
    bool clockTime=false;
    std::uint32_t measures=20;
    std::uint64_t milliseconds=40000;
};
// A new empty Segment uses implicit120 BPM and explicit4/4. Existing Segment
// length edits must instead account for their complete tempo/meter maps.
std::int32_t initial_segment_clocks(const SegmentCreationLength&);
}
