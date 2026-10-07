#pragma once
#include "riff.h"
#include <array>
#include <optional>
namespace producer::app {
struct GraphTool {std::array<std::uint8_t,16> classId{};std::int32_t index=0;std::vector<std::uint32_t> channels;std::optional<Chunk> payload;};
class ToolGraphDocument {
    Chunk root_;Bytes saved_;std::vector<Bytes> undo_,redo_;
    bool commit(Chunk);
public:
    ToolGraphDocument();
    void load(const Bytes&);Bytes save_bytes()const{return root_.encode();}void save(const std::wstring&);
    bool dirty()const{return save_bytes()!=saved_;}
    std::wstring name()const;bool set_name(const std::wstring&);
    std::vector<GraphTool> tools()const;
    bool set_channels(size_t,const std::vector<std::uint32_t>&);
    bool set_tool_payload(size_t,const Chunk&);
    bool add_tool(const std::array<std::uint8_t,16>&,const std::vector<std::uint32_t>&);
    bool remove_tool(size_t);bool move_tool(size_t,size_t);
    bool undo();bool redo();
};
}
