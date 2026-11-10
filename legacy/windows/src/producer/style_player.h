#pragma once
#include "chordmap_reference.h"
#include "style.h"
#include "dls.h"
#include <optional>
namespace producer::app {
// DMUS_SHAPET_* values from the frozen public dmusici.h.
enum class StyleShape : WORD {Falling=0,Level=1,Loopable=2,Loud=3,Quiet=4,Peaking=5,Random=6,Rising=7,Song=8};
// Public IDirectMusicComposer::ComposeSegmentFromShape parameters. The
// activity values are those of ComposeSegmentFromTemplate (0..3).
struct StylePlayerSettings {StyleShape shape=StyleShape::Rising;WORD activity=1;WORD measures=8;bool intro=false,end=false;};
struct StylePlayerComposition {
    Bytes segment;                        // generated tracks plus explicitly bound owned default Band
    ResolvedStyle style;                  // identity-bound private Style snapshot matching the reference
    std::vector<ResolvedChordMap> maps;   // full owned bytes for explicit or Style-selected default map
    std::vector<std::wstring> servers;    // declared OS runtime modules actually used
};
// Generates a fresh Segment from owned Style/ChordMap snapshots through the OS
// Composer. Distinct from compose_chord_track, which fills the Chord track of a
// user's own Segment template. No Performance, device or source mutation.
StylePlayerComposition compose_style_player_segment(const StyleCatalogEntry& style,const std::optional<Bytes>& chordMap,const StylePlayerSettings&,const std::vector<ResolvedCollection>& collections={},const std::vector<ChordMapCatalogEntry>& maps={});
}
