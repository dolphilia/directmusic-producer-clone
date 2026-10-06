#pragma once
#include "riff.h"
#include "band.h"
#include <array>
#include <optional>

namespace producer::app {
struct StyleMeter {std::uint8_t beats,denominator;std::uint16_t grids;};
struct StyleReference {
    std::int32_t time;std::uint32_t groups;std::array<std::uint8_t,16> objectId{};
    bool hasId=false;std::wstring filename,name;
};
struct ResolvedStyle {StyleReference reference;std::wstring path;Bytes bytes;StyleMeter meter;};
struct StyleCatalogEntry {std::wstring path;Bytes bytes;};
struct StylePattern {std::wstring name;StyleMeter meter;unsigned grooveBottom,grooveTop,measures,embellishment;};
struct StyleMotifSettings {std::uint32_t repeats=0;std::int32_t playStart=0,loopStart=0,loopEnd=0;std::uint32_t resolution=1;};
struct StylePart {std::wstring name;StyleMeter meter;std::array<std::uint8_t,16> objectId{};unsigned measures;std::array<std::uint32_t,32> variationChoices{};};
struct StylePartReference {size_t partIndex;std::array<std::uint8_t,16> objectId{};std::optional<std::uint32_t> pchannel;};
struct StyleNote {std::int32_t gridStart,duration;std::int16_t timeOffset;std::uint32_t variation;std::uint16_t musicValue;unsigned velocity,playMode;std::optional<unsigned> flags;};
struct StyleNoteEdit {std::int32_t gridStart,duration;std::int16_t timeOffset;std::uint32_t variation;std::uint16_t musicValue;unsigned velocity;};
struct StylePlaybackSnapshot {Bytes segment;std::vector<ResolvedStyle> styles;};
// Owns the complete DMST file. Header reads never strip Pattern/Band/unknown data.
class StyleDocument {
    Chunk root_;
    Bytes saved_;
    std::vector<Bytes> undo_,redo_;
    bool dirty_=false;
    void adopt_edit(Chunk next);
public:
    StyleDocument();
    void load(const Bytes&);
    Bytes save_bytes() const {return root_.encode();}
    StyleMeter meter() const;
    std::wstring name() const;
    bool set_name(const std::wstring& name);
    double tempo() const;
    std::array<std::uint8_t,16> object_id() const;
    bool has_object_id() const;
    bool dirty() const {return dirty_;}
    void save(const std::wstring& path);
    bool set_tempo(double tempo);
    bool set_meter(unsigned beats,unsigned denominator,unsigned grids);
    std::vector<StylePattern> patterns() const;
    std::vector<BandDocument> bands() const;
    bool set_band_instrument(size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool set_band_dls_instrument(size_t bandIndex,size_t instrumentIndex,const CollectionReference&,std::uint32_t bank,std::uint32_t program);
    // nullopt creates a Band with its first instrument in one transaction.
    bool add_band_gm_instrument(std::optional<size_t> bandIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool relocate_collections(const std::vector<ResolvedCollection>&,const std::wstring& directory);
    void relocate_context(const std::wstring& oldDirectory,const std::wstring& newDirectory,const std::vector<CollectionEntry>&);
    bool set_pattern_groove(size_t index,unsigned bottom,unsigned top);
    // Name and normal embellishment flags are one history transaction.
    // Existing non-normal flags may be renamed, but not converted here.
    bool set_pattern_properties(size_t index,const std::wstring& name,unsigned embellishment);
    bool set_pattern_layout(size_t index,unsigned beats,unsigned denominator,unsigned grids,unsigned measures);
    // Appends a full Pattern copy with its Part references still shared.
    bool duplicate_pattern(size_t index,const std::wstring& name);
    // Source-only clipboard RIFF SPC1, with all referenced Parts owned once.
    // Paste creates fresh identities; sharing within the Pattern is retained.
    Bytes copy_pattern(size_t index) const;
    bool paste_pattern(const Bytes&,const std::wstring& name);
    bool new_pattern(const std::wstring& name,unsigned pchannel=0);
    bool new_motif(const std::wstring& name,unsigned pchannel=0);
    bool assign_motif_band(size_t patternIndex,size_t bandIndex);
    std::optional<BandDocument> motif_band(size_t patternIndex) const;
    bool set_motif_band_dls_instrument(size_t patternIndex,size_t instrumentIndex,const CollectionReference&,std::uint32_t bank,std::uint32_t program);
    bool set_motif_band_instrument(size_t patternIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    // Optional mtfs remains absent on read; edits materialize it explicitly.
    std::optional<StyleMotifSettings> motif_settings(size_t patternIndex) const;
    bool set_motif_settings(size_t patternIndex,const StyleMotifSettings&);
    // Deletes the Pattern and its Parts only when no other Pattern uses them.
    bool delete_pattern(size_t index);
    // Copies only the selected reference's shared Part to a fresh identity.
    // Already unique references are a no-op; unrelated references stay shared.
    bool unshare_pattern_part(size_t patternIndex,size_t referenceIndex);
    std::vector<StylePart> parts() const;
    std::vector<StylePartReference> part_references(size_t patternIndex) const;
    std::vector<StyleNote> part_notes(size_t partIndex) const;
    bool set_part_variation_choice(size_t partIndex,size_t variationIndex,std::uint32_t choices);
    bool set_part_note(size_t partIndex,size_t noteIndex,std::int32_t duration,unsigned velocity);
    bool edit_part_note(size_t partIndex,size_t noteIndex,const StyleNoteEdit&);
    // Optional template copies all opaque record bytes, including future tails.
    bool insert_part_note(size_t partIndex,size_t position,const StyleNoteEdit&,std::optional<size_t> templateIndex={});
    bool delete_part_note(size_t partIndex,size_t noteIndex);
    bool undo();bool redo();
};
std::vector<StyleReference> style_references(const Chunk& segment);
Chunk style_reference_track(std::uint32_t groups);
std::vector<StyleReference> style_track_references(const Chunk& track);
bool insert_style_reference(Chunk& track,const StyleReference&);
bool change_style_reference(Chunk& track,size_t index,const StyleReference&);
bool delete_style_reference(Chunk& track,size_t index);
Bytes relocate_style_references(const Bytes&,const std::vector<ResolvedStyle>&,const std::wstring& directory);
Bytes retarget_style_references(const Bytes&,const std::wstring& directory,const std::wstring& oldPath,const std::wstring& newPath,const Bytes& style);
// Missing, ambiguous or identity-mismatched references fail atomically. No
// filename guessing, registry changes, original COM or external fallback.
std::vector<ResolvedStyle> resolve_styles(const std::vector<StyleReference>&,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog={},bool runtimeReferences=false);
// Rewrites playback copies only: file references become owned GUID references.
StylePlaybackSnapshot prepare_style_playback(const Bytes&,const std::vector<ResolvedStyle>&);
}
