#pragma once
#include "tool_graph.h"
#include "compat/playback_runtime.h"
#include <functional>
#include <memory>
namespace producer::app {
// Explicit source/dependency factories only. Never consult Producer COM registration.
struct ToolFactory {
    std::array<std::uint8_t,16> classId;
    std::function<runtime::Tool*(const GraphTool&)> create; // transfers one reference
    std::function<void(const GraphTool&)> validate; // optional preflight before replacing playback
};
using OwnedRuntimeGraph=std::unique_ptr<runtime::Graph,void(*)(runtime::Graph*)>;
void validate_tool_factories(const Bytes&,const std::vector<ToolFactory>&);
void populate_tool_graph(runtime::Graph&,const Bytes&,const std::vector<ToolFactory>&);
OwnedRuntimeGraph create_tool_graph(const Bytes&,const std::vector<ToolFactory>&);
// Only the Segment graph; AudioPath graphs have a different routing stage.
Bytes segment_tool_graph(const Bytes&);
}
