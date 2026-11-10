#pragma once
#include "sequence_strip_model.h"
#include <windows.h>
namespace producer::app {
enum class SequenceQuantizeTarget {selected,part,allParts};
struct SequenceQuantizeCommand {SequenceQuantizeTarget target=SequenceQuantizeTarget::part;SequenceQuantizeOptions options;};
std::optional<SequenceQuantizeCommand> show_sequence_quantize_editor(HWND,bool hasSelection);
}
