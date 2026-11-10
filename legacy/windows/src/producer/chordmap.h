#pragma once
#include "riff.h"
#include <array>
namespace producer::app {
struct ChordMapSubchord {std::uint32_t chord=0x91,scale=0xab5,inversions=0xffffffff,levels=1;std::uint8_t root=0,scaleRoot=0;std::uint16_t flags=0;};
struct ChordMapConnection {std::uint32_t flags=0;std::uint16_t weight=50,minBeats=1,maxBeats=4,destination=0;};
struct ChordMapNode {std::uint16_t id=0;std::uint32_t flags=0;std::wstring name;std::vector<ChordMapSubchord> subchords;std::vector<ChordMapConnection> connections;};
struct ChordMapSignpost {std::uint32_t groups=0,flags=0;std::wstring name;std::vector<ChordMapSubchord> subchords;size_t cadences=0;};
struct ChordMapPaletteChord {std::wstring name;std::vector<ChordMapSubchord> subchords;};
class ChordMapDocument {
    Chunk root_;Bytes saved_;std::vector<Bytes> undo_,redo_;
    bool commit(Chunk);
public:
    ChordMapDocument();void load(const Bytes&);Bytes save_bytes() const{return root_.encode();}void save(const std::wstring&);
    bool dirty() const{return save_bytes()!=saved_;}bool undo();bool redo();
    std::wstring name() const;std::uint32_t scale() const;
    bool has_object_id() const;
    std::array<std::uint8_t,16> object_id() const;
    bool set_name(const std::wstring&);bool set_scale(std::uint32_t);
    std::vector<ChordMapNode> nodes() const;std::vector<ChordMapSignpost> signposts() const;
    std::vector<ChordMapPaletteChord> palette() const;
    bool edit_palette_subchord(size_t chord,size_t layer,const ChordMapSubchord&);
    bool insert_palette_node(size_t chord,std::uint16_t* id=nullptr);
    bool insert_node(const std::wstring&,const ChordMapSubchord&,std::uint16_t* id=nullptr);
    bool rename_node(std::uint16_t,const std::wstring&);bool edit_subchord(std::uint16_t,size_t,const ChordMapSubchord&);
    bool delete_node(std::uint16_t);
    bool set_connection(std::uint16_t,size_t,const ChordMapConnection&);bool insert_connection(std::uint16_t,const ChordMapConnection&);bool delete_connection(std::uint16_t,size_t);
    bool insert_signpost(std::uint16_t,std::uint32_t);bool set_signpost_groups(size_t,std::uint32_t);bool delete_signpost(size_t);
};
}
