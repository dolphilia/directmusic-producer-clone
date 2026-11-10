#pragma once
#include "conductor.h"
#include "midi_echo.h"
#include <optional>

namespace producer::app {
struct PerformanceOptionsSelection {PerformanceSettings audio;MidiInputSelection input;MidiEchoSettings echo;};
std::optional<PerformanceOptionsSelection> show_performance_options(HWND,const PerformanceSettings&,const std::vector<PerformanceSynthesizer>&,const MidiInputSelection&,const std::vector<MidiInputDevice>&,const MidiEchoSettings&);
}
