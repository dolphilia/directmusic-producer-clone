#pragma once
#include "riff.h"
#include "container_graph.h"
#include <optional>
namespace producer::app {
class ScriptDocument {
    Chunk root_; Bytes saved_; std::vector<Bytes> undo_,redo_;
    bool commit(Chunk);
public:
    ScriptDocument();
    void load(const Bytes&);
    Bytes save_bytes()const{return root_.encode();}
    void save(const std::wstring&);
    bool dirty()const{return save_bytes()!=saved_;}
    std::wstring name()const;
    std::wstring language()const;
    std::optional<std::wstring> source()const;
    Bytes source_reference()const;
    Bytes container_bytes()const;
    ContainerGraph container()const{return ContainerGraph(container_bytes());}
    bool set_container_no_loads(bool);
    bool add_segment_reference(const std::array<std::uint8_t,16>& objectId,const std::array<std::uint8_t,16>& projectFileId,const std::wstring& filename,const std::wstring& alias,bool keep=false);
    bool remove_contained_object(size_t);
    bool set_contained_properties(size_t,const std::optional<std::wstring>& alias,bool keep);
    std::uint32_t flags()const;
    bool set_properties(const std::wstring& name,const std::wstring& language,bool loadAll,bool downloadAll);
    // Explicitly replaces an external source reference with owned text.
    bool set_source(const std::wstring&);
    bool undo(); bool redo();
};
}
