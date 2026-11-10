#pragma once
#include "document.h"
#include <optional>
namespace producer::app {
// Source-owned, versioned design metadata for a newly authored empty part.
// This is not the undocumented Producer psql/psqc layout. Runtime seqt stays
// empty until a note or curve is entered; existing opaque UI data is untouched.
std::optional<std::uint32_t> sequence_declared_channel(const Chunk&);
struct SequenceLane {std::uint32_t channel;SequenceController controller;};
// Empty lane metadata is source-owned. Only the observed single-channel0
// PBend/Expression legacy layout is read; all legacy bytes stay untouched.
std::vector<SequenceLane> sequence_design_lanes(const Chunk&);
bool sequence_add_design_lane(Chunk&,std::uint32_t,SequenceController);
std::vector<std::uint32_t> sequence_part_channels(const Chunk&);
std::vector<std::uint32_t> used_sequence_channels(const SegmentDocument&);
std::uint32_t next_sequence_channel(const SegmentDocument&);
}
