#pragma once
#include "framework.h"
namespace producer::app {
struct EmbeddedBandTarget {
    size_t document=0,track=0,event=0;std::uint32_t groups=1;
    std::int32_t logical=0;Bytes identity,lastBytes;
};
struct EmbeddedBandView {EmbeddedBandTarget target;BandEvent event;BandDocument band;Timeline timeline;};
EmbeddedBandTarget make_embedded_band_target(const Framework&,size_t document,std::uint32_t groups,size_t track,size_t event);
std::optional<EmbeddedBandView> read_embedded_band(const Framework&,const EmbeddedBandTarget&);
bool update_embedded_band(Framework&,EmbeddedBandTarget&,std::int32_t logical,std::int32_t physical,const Bytes&);
// Bounded live-edit guard: one Band track, the currently effective event,
// and the same native identity in the immutable playback snapshot.
bool embedded_band_is_playing(const Bytes& snapshot,const EmbeddedBandTarget&,std::int32_t clocks);
// Signed tick representation is local to Band; global Timeline remains nonnegative.
Position band_start_position(const Timeline&,std::int32_t physical);
std::int32_t band_start_clocks(const Timeline&,Position);
}
