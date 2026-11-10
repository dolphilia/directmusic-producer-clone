#pragma once
#include "document.h"
#include <array>

namespace producer::app {
struct MidiImport {
    SegmentDocument segment;
    unsigned format=0,division=0,sourceTracks=0,notes=0,curves=0;
    std::vector<unsigned> channels;
    std::vector<std::string> notices;
};
// Pure conversion: nothing is published to a Framework until the entire file
// and generated native document have been validated. Unsupported timed data
// is rejected rather than silently omitted.
MidiImport import_midi(const Bytes&);
}
