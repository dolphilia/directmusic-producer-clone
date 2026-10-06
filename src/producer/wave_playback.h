#pragma once
#include "wave_document.h"
#include <windows.h>
namespace producer::app {
// Observed SfxCow DMRF refh class, resolved to Windows dswave.dll.
inline constexpr GUID wave_runtime_class={0x8a667154,0xf9cb,0x11d2,{0xad,0x8a,0,0x60,0xb0,0x57,0x5a,0xbc}};
struct WavePlaybackSnapshot {Bytes segment;std::vector<ResolvedWave> waves;};
// Private playback mapping only. Source, disk files and document history remain
// unchanged. Validated before Conductor changes a live playback session.
WavePlaybackSnapshot prepare_wave_playback(const Bytes&,const std::vector<ResolvedWave>&);
}
