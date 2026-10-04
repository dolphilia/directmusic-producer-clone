#pragma once
#include "riff.h"
#include <array>
#include <optional>

namespace producer::app {
struct CollectionReference {std::wstring filename;std::optional<std::array<std::uint8_t,16>> objectId;};
struct CollectionEntry {std::wstring path;Bytes bytes;};
struct ResolvedCollection {CollectionReference reference;std::wstring path;Bytes bytes;};
struct CollectionPlaybackSnapshot {std::vector<Bytes> documents;std::vector<ResolvedCollection> collections;};
CollectionReference collection_reference(const Bytes& descriptor);
// Validate RIFF identity without claiming instrument/wave editing support.
std::optional<std::array<std::uint8_t,16>> collection_identity(const Bytes&);
std::vector<ResolvedCollection> resolve_collections(const std::vector<CollectionReference>&,const std::wstring& directory,const std::vector<CollectionEntry>& catalog={},bool runtimeReferences=false);
std::vector<CollectionReference> document_collection_references(const Bytes&);
// Inputs are owned RIFF documents: Segment plus Styles, or standalone Style.
// Rewrite memory copies only, preserving source documents and opaque chunks.
CollectionPlaybackSnapshot prepare_collection_playback(const std::vector<Bytes>&,const std::vector<ResolvedCollection>&);
// Rewrite only active filename fields in owned Band copies. Dependencies must
// match the original descriptors; GUIDs and opaque descriptor data stay intact.
Bytes relocate_collection_references(const Bytes&,const std::vector<ResolvedCollection>&,const std::wstring& directory);
struct BandInstrument {
    std::uint32_t patch,pchannel,flags;
    unsigned pan,volume;
    // Complete opaque collection descriptor; inspection never resolves it.
    Bytes collectionReference;
};
class BandDocument {
    Chunk root_;
    Bytes saved_;
    std::vector<Bytes> undo_,redo_;
public:
    BandDocument();
    void load(const Bytes&);
    Bytes save_bytes() const {return root_.encode();}
    void save(const std::wstring& path);
    bool dirty() const {return save_bytes()!=saved_;}
    std::vector<BandInstrument> instruments() const;
    // Patch: 7-bit program, LSB, MSB and observed percussion bit31.
    bool set_instrument(size_t index,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool add_gm_instrument(std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool set_collection_reference(size_t index,const CollectionReference&);
    bool relocate_collections(const std::vector<ResolvedCollection>&,const std::wstring& directory);
    // Commit the collection and its MIDI locale as one undoable assignment.
    bool set_dls_instrument(size_t index,const CollectionReference&,std::uint32_t bank,std::uint32_t program);
    std::vector<CollectionReference> collection_references() const;
    bool undo();bool redo();
};
struct BandEvent {std::int32_t logicalTime,physicalTime;Bytes band;};
bool is_band_track(const Chunk&);
std::vector<BandEvent> band_track_events(const Chunk&);
Chunk make_band_track();
// Replace at logical time or append; retain all other events/opaque bytes.
bool set_band_track_event(Chunk&,std::int32_t time,const Bytes& band);
bool move_band_track_event(Chunk&,size_t index,std::int32_t logical,std::int32_t physical);
bool delete_band_track_event(Chunk&,size_t index);
}
