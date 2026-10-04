#pragma once
#include "time_signature/time_signature_map.h"
#include "riff.h"

namespace producer::app {
struct Position { std::int32_t measure, beat, tick; };
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
