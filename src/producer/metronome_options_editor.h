#pragma once
#include "metronome.h"
#include <optional>
namespace producer::app {
std::optional<MetronomeSettings> show_metronome_options(HWND,const MetronomeSettings&);
}
