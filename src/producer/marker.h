#pragma once
#include "riff.h"
namespace producer::app {
enum class MarkerKind { play, enter };
struct MarkerEvent {std::int32_t time=0;MarkerKind kind=MarkerKind::play;};
// Original Producer may save identical MARK containers twice. They describe
// one logical track; mutations update every copy and retain unrelated chunks.
std::vector<MarkerEvent> marker_events(const Chunk& track);
bool add_marker_event(Chunk&,MarkerEvent,size_t* resultingIndex=nullptr);
bool change_marker_event(Chunk&,size_t,MarkerEvent,size_t* resultingIndex=nullptr);
bool delete_marker_event(Chunk&,size_t);
bool mark_marker_boundaries(Chunk&,MarkerKind,const std::vector<std::int32_t>&,bool mark);
Bytes copy_marker_event(const Chunk&,size_t);
bool paste_marker_event(Chunk&,const Bytes&,std::int32_t,size_t* resultingIndex=nullptr);
Chunk marker_track();
}
