#pragma once
#include "sequence_strip_model.h"
#include <windows.h>
namespace producer::app {
enum class SequenceVelocityTarget {selected,part,allParts};
struct SequenceVelocityCommand {SequenceVelocityTarget target;SequenceVelocityOptions options;};
std::optional<SequenceVelocityCommand> show_sequence_velocity_editor(HWND,bool hasSelection);
}
