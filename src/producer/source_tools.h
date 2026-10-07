#pragma once
#include "tool_graph_runtime.h"
#include "param_control.h"
#include <medparam.h>
namespace producer::app {
// Product-owned identity; never alias an original or test Tool class.
extern const ParamGuid source_velocity_tool_class;
std::vector<ToolFactory> source_tool_factories();
float source_velocity_gain(const GraphTool&);
Chunk source_velocity_payload(float);
struct ToolParameterCapability {std::uint32_t index;MP_PARAMINFO info;};
struct ToolParameterObject {
    ParamObject address;
    std::wstring name;
    std::vector<ToolParameterCapability> parameters;
};
// Inspect explicit source instances only; no registry activation or playback.
std::vector<ToolParameterObject> discover_audio_path_tool_parameters(const Bytes&);
const ToolParameterCapability* tool_parameter_capability(const std::vector<ToolParameterObject>&,const ParamObject&,std::uint32_t);
bool valid_tool_parameter_curve(const ToolParameterCapability&,const ParamCurve&);
void validate_source_parameter_controls(const std::vector<ParamControlObject>&,const std::vector<ToolParameterObject>&);
}
