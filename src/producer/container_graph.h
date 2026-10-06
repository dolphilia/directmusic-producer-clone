#pragma once
#include "riff.h"
#include <array>
#include <optional>
#include <functional>
namespace producer::app {
struct ContainedObject {
    std::array<std::uint8_t,16> classId{};
    std::uint32_t flags=0;
    std::optional<std::wstring> alias;
    bool reference=false;
    // Design membership is independent of the linked/embedded payload.
    bool referenceRuntime=true;
    std::optional<Bytes> projectAssociation;
    Chunk payload;
};
// Validation and edits operate on owned RIFF copies. Loading an object or
// changing loader cache ownership is a separate runtime responsibility.
class ContainerGraph {
    Chunk root_;
public:
    explicit ContainerGraph(const Bytes&);
    Bytes save_bytes()const{return root_.encode();}
    std::uint32_t flags()const;
    std::vector<ContainedObject> objects()const;
    bool set_no_loads(bool);
    // Observed Producer design reference: cobu WORD1 and jzfr Project file/DocType GUIDs.
    bool add_segment_reference(const std::array<std::uint8_t,16>& objectId,const std::array<std::uint8_t,16>& projectFileId,const std::wstring& filename,const std::wstring& alias,bool keep=false);
    bool remove_object(size_t);
    bool set_object_properties(size_t,const std::optional<std::wstring>& alias,bool keep);
    bool set_reference_runtime(size_t,bool);
    Bytes runtime_bytes(const std::function<Chunk(const ContainedObject&)>& resolveEmbedded)const;
};
}
