#pragma once
#include "document.h"
#include "components.h"
#include "dls.h"
#include <memory>
#include <optional>
#include <windows.h>
#include <filesystem>

namespace producer::app {
struct OpenDocument { std::wstring path; std::unique_ptr<SegmentDocument> document; std::optional<size_t> projectReference; };
struct OpenStyleDocument {std::wstring path;std::unique_ptr<StyleDocument> document;std::optional<size_t> projectReference;};
struct OpenBandDocument {std::wstring path;std::unique_ptr<BandDocument> document;std::optional<size_t> projectReference;};
struct OpenCollection {std::wstring path;DlsDocument document;std::optional<size_t> projectReference;};
struct OpenAudioPath {std::wstring path;std::unique_ptr<AudioPathDocument> document;std::optional<size_t> projectReference;};
enum class RuntimeDocumentKind {Segment,Style,Band,Collection,AudioPath};
// Durable before/after evidence for a later explicit recovery operation.
// This is a manifest, not permission to overwrite a changed external file.
struct RuntimeUpdateRecoveryFile {
    std::wstring target;
    bool existed=false;
    Bytes before,after;
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
};
Chunk runtime_update_recovery_record(const std::vector<RuntimeUpdateRecoveryFile>& files);
std::vector<RuntimeUpdateRecoveryFile> parse_runtime_update_recovery_record(const Bytes& bytes);
enum class RuntimeRecoveryState {Before,After,Conflict};
struct RuntimeRecoveryTarget {RuntimeUpdateRecoveryFile file;RuntimeRecoveryState state;std::string reason;};
std::vector<RuntimeRecoveryTarget> inspect_runtime_update_recovery(const Bytes& bytes,const std::vector<std::wstring>& protectedPaths={});
struct RuntimeRecoverySource {std::wstring path;Bytes bytes;std::wstring target;};
struct RuntimeRecoveryOrigin {std::wstring projectPath,outputRoot;Bytes projectBytes;bool configured=false;std::vector<RuntimeRecoverySource> sources;};
Chunk bind_runtime_update_recovery(const std::vector<RuntimeUpdateRecoveryFile>& files,const RuntimeRecoveryOrigin& origin);
RuntimeRecoveryOrigin runtime_update_recovery_origin(const Bytes& bytes);
class Framework {
    std::wstring name_=L"Untitled", projectPath_;
    std::vector<OpenDocument> documents_;
    std::vector<OpenStyleDocument> styles_;
    std::vector<OpenBandDocument> bands_;
    std::vector<OpenCollection> collections_;
    std::vector<OpenAudioPath> audioPaths_;
    std::vector<std::wstring> warnings_;
    ComponentCatalog components_;
    Chunk projectRoot_;
    std::wstring projectDirectory_;
    bool projectDirty_=true;
    std::vector<StyleCatalogEntry> style_catalog() const;
    void refresh_styles();
    void apply_style_edit(size_t index,StyleDocument next);
    Chunk runtime_metadata() const;
    bool adopt_runtime_metadata(const Chunk&);
    std::vector<RuntimeRecoverySource> runtime_recovery_sources() const;
    std::filesystem::path runtime_recovery_target(const RuntimeRecoverySource&,const std::wstring&,bool) const;
    std::pair<std::wstring,std::optional<size_t>> runtime_owner(RuntimeDocumentKind,size_t) const;
public:
    Framework();
    // Ownership is acyclic: Framework -> documents -> RIFF and track models.
    // UI stores indexes, never raw pointers across document creation/deletion.
    void new_project();
    size_t new_segment();
    size_t open_segment(const std::wstring& path);
    size_t new_style();size_t open_style(const std::wstring& path);
    size_t new_band();size_t open_band(const std::wstring& path);
    void save_band(size_t index,const std::wstring& path);
    bool set_band_instrument(size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool undo_band(size_t index);bool redo_band(size_t index);
    bool add_band_gm_instrument(size_t index,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool assign_band(size_t segmentIndex,size_t bandIndex,std::int32_t time);
    bool assign_style_band(size_t segmentIndex,size_t styleIndex,size_t bandIndex,std::int32_t time);
    bool assign_audio_path(size_t segmentIndex,size_t audioPathIndex);
    size_t new_audio_path();size_t open_audio_path(const std::wstring&);
    void save_audio_path(size_t,const std::wstring&);
    AudioPathDocument& audio_path_document(size_t i){return *audioPaths_.at(i).document;}
    const AudioPathDocument& audio_path_document(size_t i) const{return *audioPaths_.at(i).document;}
    const std::vector<OpenAudioPath>& audio_paths() const{return audioPaths_;}
    size_t new_collection();size_t open_collection(const std::wstring& path);
    void save_collection(size_t index,const std::wstring& path);
    const std::vector<OpenCollection>& collections() const {return collections_;}
    DlsDocument& collection_document(size_t index){return collections_.at(index).document;}
    const DlsDocument& collection_document(size_t index) const {return collections_.at(index).document;}
    std::vector<ResolvedCollection> band_collections(size_t index) const;
    std::vector<ResolvedCollection> playback_collections(size_t segmentIndex) const;
    StyleCatalogEntry style_playback_snapshot(size_t styleIndex) const;
    std::vector<ResolvedCollection> style_playback_collections(size_t styleIndex) const;
    bool set_band_collection(size_t bandIndex,size_t instrumentIndex,size_t collectionIndex);
    bool set_band_collection_instrument(size_t bandIndex,size_t instrumentIndex,size_t collectionIndex,size_t dlsInstrumentIndex);
    const std::vector<OpenBandDocument>& band_documents() const {return bands_;}
    const BandDocument& band_document(size_t index) const {return *bands_.at(index).document;}
    void save_style(size_t index,const std::wstring& path);
    bool set_style_tempo(size_t index,double tempo);
    bool set_style_name(size_t index,const std::wstring& name);
    bool set_style_band_instrument(size_t styleIndex,size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool set_style_band_collection_instrument(size_t styleIndex,size_t bandIndex,size_t instrumentIndex,size_t collectionIndex,size_t dlsInstrumentIndex);
    bool add_style_band_gm_instrument(size_t styleIndex,std::optional<size_t> bandIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool set_style_meter(size_t index,unsigned beats,unsigned denominator,unsigned grids);
    bool set_pattern_groove(size_t styleIndex,size_t patternIndex,unsigned bottom,unsigned top);
    bool set_pattern_properties(size_t styleIndex,size_t patternIndex,const std::wstring& name,unsigned embellishment);
    bool set_pattern_layout(size_t styleIndex,size_t patternIndex,unsigned beats,unsigned denominator,unsigned grids,unsigned measures);
    bool duplicate_style_pattern(size_t styleIndex,size_t patternIndex,const std::wstring& name);
    bool paste_style_pattern(size_t styleIndex,const Bytes&,const std::wstring& name);
    bool new_style_pattern(size_t styleIndex,const std::wstring& name,unsigned pchannel=0);
    bool new_style_motif(size_t styleIndex,const std::wstring& name,unsigned pchannel=0);
    bool assign_style_motif_band(size_t styleIndex,size_t patternIndex,size_t bandIndex);
    bool set_style_motif_band_collection_instrument(size_t styleIndex,size_t patternIndex,size_t instrumentIndex,size_t collectionIndex,size_t dlsInstrumentIndex);
    bool set_style_motif_band_instrument(size_t styleIndex,size_t patternIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool set_style_motif_settings(size_t styleIndex,size_t patternIndex,const StyleMotifSettings&);
    bool delete_style_pattern(size_t styleIndex,size_t patternIndex);
    bool unshare_style_pattern_part(size_t styleIndex,size_t patternIndex,size_t referenceIndex);
    bool set_style_part_variation_choice(size_t styleIndex,size_t partIndex,size_t variationIndex,std::uint32_t choices);
    bool set_style_part_note(size_t styleIndex,size_t partIndex,size_t noteIndex,std::int32_t duration,unsigned velocity);
    bool edit_style_part_note(size_t styleIndex,size_t partIndex,size_t noteIndex,const StyleNoteEdit&);
    bool insert_style_part_note(size_t styleIndex,size_t partIndex,size_t position,const StyleNoteEdit&,std::optional<size_t> templateIndex={});
    bool delete_style_part_note(size_t styleIndex,size_t partIndex,size_t noteIndex);
    bool undo_style(size_t index);bool redo_style(size_t index);
    const std::vector<OpenStyleDocument>& style_documents() const {return styles_;}
    const StyleDocument& style_document(size_t index) const {return *styles_.at(index).document;}
    void open_project(const std::wstring& path);
    std::vector<RuntimeRecoveryTarget> inspect_runtime_recovery(const std::wstring& journalPath) const;
    std::vector<RuntimeRecoveryTarget> validate_runtime_recovery(const std::wstring& journalPath,const std::wstring& expectedOutputRoot,bool configured=false) const;
    void recover_runtime_update(const std::wstring& journalPath,const std::wstring& expectedOutputRoot,bool configured=false) const;
    void save_project(const std::wstring& path);
    void copy_project(const std::wstring& destination) const;
    void export_runtime(const std::wstring& directory) const;
    void export_runtime_defaults() const;
    void export_runtime_observed(const std::wstring& directory,const std::function<void(const std::wstring&)>& afterPublish) const;
    void export_runtime_defaults_observed(const std::function<void(const std::wstring&)>& afterPublish) const;
private:
    void export_runtime_impl(const std::wstring& directory,bool configured,const std::function<void(const std::wstring&)>& afterPublish={}) const;
public:
    void save_runtime(RuntimeDocumentKind,size_t,const std::wstring& destination) const;
    std::wstring runtime_project_folder() const;
    std::wstring runtime_component_folder(RuntimeDocumentKind) const;
    std::wstring runtime_filename(RuntimeDocumentKind,size_t) const;
    bool set_runtime_project_folder(const std::wstring&);
    bool set_runtime_component_folder(RuntimeDocumentKind,const std::wstring&);
    bool set_runtime_filename(RuntimeDocumentKind,size_t,const std::wstring&);
    void save_runtime_default(RuntimeDocumentKind,size_t) const;
    void save_segment(size_t index,const std::wstring& path);
    const std::vector<OpenDocument>& documents() const { return documents_; }
    SegmentDocument& document(size_t index) { return *documents_.at(index).document; }
    const std::vector<std::wstring>& warnings() const { return warnings_; }
    const std::wstring& project_path() const { return projectPath_; }
    bool dirty() const;
};
}
