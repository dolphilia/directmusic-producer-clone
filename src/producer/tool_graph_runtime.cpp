#include "tool_graph_runtime.h"
#include <algorithm>
#include <stdexcept>
namespace producer::app {namespace {
const ToolFactory& factory(const GraphTool& t,const std::vector<ToolFactory>& factories){
    const ToolFactory* found=nullptr;
    for(const auto& f:factories)if(f.classId==t.classId){if(found)throw std::runtime_error("Ambiguous Tool factory");found=&f;}
    if(!found||!found->create)throw std::runtime_error("Tool class needs an explicitly declared source/dependency factory");
    return *found;
}
void release_graph(runtime::Graph* g){if(g)g->Release();}
void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Runtime ToolGraph construction failed");}
}
void validate_tool_factories(const Bytes& bytes,const std::vector<ToolFactory>& factories){
    ToolGraphDocument graph;graph.load(bytes);for(const auto& t:graph.tools())(void)factory(t,factories);
}
OwnedRuntimeGraph create_tool_graph(const Bytes& bytes,const std::vector<ToolFactory>& factories){
    validate_tool_factories(bytes,factories);
    runtime::Graph* raw=nullptr;
    const auto hr=CoCreateInstance(runtime::graphClass,nullptr,CLSCTX_INPROC_SERVER,runtime::graphId,reinterpret_cast<void**>(&raw));
    OwnedRuntimeGraph graph(raw,release_graph);check(hr);if(!graph)throw std::runtime_error("No runtime graph");
    populate_tool_graph(*graph,bytes,factories);
    return graph;
}
void populate_tool_graph(runtime::Graph& graph,const Bytes& bytes,const std::vector<ToolFactory>& factories){
    validate_tool_factories(bytes,factories);
    ToolGraphDocument document;document.load(bytes);
    for(const auto& t:document.tools()){
        std::unique_ptr<runtime::Tool,void(*)(runtime::Tool*)> tool(factory(t,factories).create(t),[](runtime::Tool* p){if(p)p->Release();});
        if(!tool)throw std::runtime_error("Tool factory returned no instance");
        // InsertTool takes its own Tool reference and copies this DWORD mapping.
        std::vector<DWORD> channels(t.channels.begin(),t.channels.end());
        check(graph.InsertTool(tool.get(),channels.empty()?nullptr:channels.data(),static_cast<DWORD>(channels.size()),t.index));
    }
}
Bytes segment_tool_graph(const Bytes& bytes){
    if(bytes.empty())return {};const auto root=Chunk::parse(bytes);const Chunk* graph=nullptr;
    if(root.id!="RIFF"||root.type!="DMSG")throw std::runtime_error("Expected Segment graph owner");
    for(const auto& c:root.children)if(c.id=="RIFF"&&c.type=="DMTG"){if(graph)throw std::runtime_error("Duplicate embedded Segment graph");graph=&c;}
    if(!graph)return {};ToolGraphDocument validated;validated.load(graph->encode());return validated.save_bytes();
}
}
