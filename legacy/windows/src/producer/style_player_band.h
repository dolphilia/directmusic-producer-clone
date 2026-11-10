#pragma once
#include "band.h"
#include "compat/playback_runtime.h"
namespace producer::app {
// Serialize every public Band event, binding identified objects to full owned
// Style Band bytes. Only the anonymous event at zero may use the explicitly
// selected default Band. Unresolved later events fail without changing input.
Chunk read_style_player_bands(runtime::Track*, LONG length, const Bytes& ownedStyle, const Bytes& defaultBand);
std::wstring initial_style_player_band_name(const Bytes& segment);
}
