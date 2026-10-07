#pragma once
#include "time_signature/time_signature_map.h"
#include "riff.h"

namespace producer::app {
struct Position { std::int32_t measure, beat, tick; };
enum TimelineStrip : std::uint32_t { TimelineTempo=1, TimelineSequence=2, TimelineLyric=4, TimelineMarker=8, TimelineMute=16, TimelineAll=31 };
struct TimelineSelection { std::int32_t begin=0,end=0;std::uint32_t strips=0;size_t lyricTrack=0,markerTrack=0,muteTrack=0; };
// Internal format; original Timeline COM/OLE clipboard compatibility remains
// separate. The source range is half-open and retains leading/trailing space.
struct TimelineClipboard {std::int32_t span=0;std::uint32_t strips=0;Bytes tempo,sequence,lyric,marker,mute;};
Bytes encode_timeline_clipboard(const TimelineClipboard&);
TimelineClipboard decode_timeline_clipboard(const Bytes&);
// Typed source-only Timeline. This is not the original COM/OLE Timeline ABI.
class Timeline {
    meter::Map meters_;
public:
    Timeline();
    void load_meter(const Bytes& bytes);
    Position position(std::int32_t clocks) const;
    std::int32_t clocks(Position position) const;
    int pixel(std::int32_t clocks, double zoom, int scroll) const;
    const std::vector<meter::Event>& meters() const { return meters_.events(); }
};
}
