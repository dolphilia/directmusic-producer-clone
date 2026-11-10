#pragma once
#include "container_graph.h"
namespace producer::app {
// Standalone DMCN ownership shares the embedded Script graph validator.
// Saving design links does not imply runtime embedding or loader ownership.
class ContainerDocument {
    Bytes bytes_,saved_;
    std::vector<Bytes> undo_,redo_;
    bool commit(Bytes);
public:
    ContainerDocument();
    void load(const Bytes&);
    Bytes save_bytes()const{return bytes_;}
    void save(const std::wstring&);
    bool dirty()const{return bytes_!=saved_;}
    std::wstring name()const;
    ContainerGraph graph()const{return ContainerGraph(bytes_);}
    bool set_name(const std::wstring&);
    bool set_no_loads(bool);
    bool set_object_properties(size_t,const std::optional<std::wstring>&,bool);
    bool set_reference_runtime(size_t,bool);
    bool add_segment_reference(const std::array<std::uint8_t,16>&,const std::array<std::uint8_t,16>&,const std::wstring&,const std::wstring&,bool keep=false);
    bool remove_object(size_t);
    bool undo();bool redo();
};
}
