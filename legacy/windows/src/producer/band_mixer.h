#pragma once
#include "band.h"
namespace producer::app {
struct BandMixerPoint {int x,y;};
struct BandMixerArea {int left,top,right,bottom,width=24,height=22;};
bool band_mixer_active(const BandInstrument&);
BandMixerPoint band_mixer_position(const BandInstrument&,size_t ordinal,const BandMixerArea&);
BandMixValue band_mixer_value(std::uint32_t pchannel,BandMixerPoint,const BandMixerArea&);
std::vector<BandMixValue> band_mixer_drag(const std::vector<BandInstrument>&,
    const std::vector<std::uint32_t>& selected,const BandMixerArea&,int dx,int dy);
std::wstring band_mixer_pan_label(unsigned);
// Absent validity means inherited runtime state, not a reversible native value.
std::optional<std::vector<BandMixValue>> band_mixer_explicit_values(
    const std::vector<BandInstrument>&,const std::vector<std::uint32_t>& selected);
std::vector<BandMixValue> band_mixer_changed_values(
    const std::vector<BandInstrument>& before,const std::vector<BandInstrument>& after);
}
