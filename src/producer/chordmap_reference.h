#pragma once
#include "chordmap.h"
#include <array>
namespace producer::app {
struct ChordMapReference {
    std::int32_t time=0;std::uint32_t groups=1;
    std::array<std::uint8_t,16> objectId{};bool hasId=false;
    std::wstring filename,name;
};
struct ChordMapCatalogEntry {std::wstring path;Bytes bytes;};
struct ResolvedChordMap {ChordMapReference reference;std::wstring path;Bytes bytes;};
struct ChordMapPlayback {Bytes segment;std::vector<ResolvedChordMap> maps;};
Chunk chordmap_reference_track(std::uint32_t groups=1);
std::vector<ChordMapReference> chordmap_track_references(const Chunk&);
std::vector<ChordMapReference> chordmap_references(const Chunk&);
// The editor authors one reference per track. Imported runtime arrays survive
// reads, but cannot be silently replaced by a single editor selection.
bool set_chordmap_reference(Chunk&,const ChordMapReference&);
bool clear_chordmap_reference(Chunk&);
std::vector<ResolvedChordMap> resolve_chordmaps(const std::vector<ChordMapReference>&,const std::wstring&,const std::vector<ChordMapCatalogEntry>&);
ChordMapPlayback prepare_chordmap_playback(const Bytes&,const std::vector<ResolvedChordMap>&);
}
