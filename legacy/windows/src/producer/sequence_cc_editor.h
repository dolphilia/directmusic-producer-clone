#pragma once
#include "sequence.h"
#include <windows.h>
#include <optional>
#include <string>
namespace producer::app {
std::wstring sequence_controller_name(SequenceController);
std::optional<SequenceController> show_sequence_cc_editor(HWND,const std::vector<SequenceController>& existing);
std::optional<SequenceCurve> show_sequence_curve_editor(HWND,const SequenceCurve&,std::int32_t length);
}
