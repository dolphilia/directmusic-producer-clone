#pragma once
#include "timeline.h"
#include <filesystem>

namespace producer::app {
// Transport preferences, never part of an authored Segment or its history.
// Pitch/velocity values match the observed installed-original session; they
// are not asserted to be immutable defaults in every original installation.
struct MetronomeSettings {
    bool enabled=false,recordOnly=true;
    std::uint32_t pchannel=9,firstPitch=36,otherPitch=37;
    std::uint32_t firstVelocity=117,otherVelocity=100,countInBars=0;
};
bool operator==(const MetronomeSettings&,const MetronomeSettings&);
inline bool operator!=(const MetronomeSettings& a,const MetronomeSettings& b){return !(a==b);}
void validate_metronome_settings(const MetronomeSettings&);
Bytes encode_metronome_settings(const MetronomeSettings&);
MetronomeSettings decode_metronome_settings(const Bytes&);
MetronomeSettings load_metronome_settings(const std::filesystem::path&);
void save_metronome_settings(const std::filesystem::path&,const MetronomeSettings&);
struct MetronomeClick {std::int32_t clocks;std::uint32_t pchannel;BYTE pitch,velocity;bool firstBeat;};
// Half-open scheduling window. The same Timeline map drives the editor and
// the clicks, including denominator changes and first-beat accents.
std::vector<MetronomeClick> metronome_clicks(const Timeline&,std::int32_t begin,std::int32_t end,const MetronomeSettings&,size_t maximum=4096);
std::int32_t metronome_count_in_clocks(const Timeline&,std::int32_t start,const MetronomeSettings&,bool recording=false);
struct MetronomeCountIn {Bytes segment;Timeline timeline;std::int32_t startPoint=0,clocks=0;};
// A private carrier retains Band history at the requested seek position, but
// never executes authored notes, controllers, scripts, triggers or waves.
MetronomeCountIn prepare_metronome_count_in(const Bytes&,const Timeline&,std::int32_t start,const MetronomeSettings&,const std::vector<Bytes>& styleBands={});
}
