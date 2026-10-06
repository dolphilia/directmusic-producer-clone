#pragma once
#include "chordmap_reference.h"
#include "style.h"
namespace producer::app {
struct ChordComposition {Chunk chords;std::vector<std::wstring> servers;};
Chunk read_composed_chords(const Bytes&);
// Creates a private template; the caller adopts only the returned Chord list.
// No Performance, audio device, file publication, or source mutation occurs.
ChordComposition compose_chord_track(const Bytes&,std::uint32_t groups,const std::vector<ResolvedStyle>&,const std::vector<ResolvedChordMap>&,unsigned activity=1);
}
