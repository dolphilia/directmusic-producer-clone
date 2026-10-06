#pragma once
#include "document.h"
#include "components.h"
#include "dls.h"
#include "segment_trigger_playback.h"
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
struct OpenWave {std::wstring path;std::unique_ptr<WaveDocument> document;std::optional<size_t> projectReference;};
struct OpenToolGraph {std::wstring path;std::unique_ptr<ToolGraphDocument> document;std::optional<size_t> projectReference;};
struct OpenContainer {std::wstring path;std::unique_ptr<ContainerDocument> document;std::optional<size_t> projectReference;};
struct OpenScript {std::wstring path;std::unique_ptr<ScriptDocument> document;std::optional<size_t> projectReference;};
struct OpenChordMap {std::wstring path;std::unique_ptr<ChordMapDocument> document;std::optional<size_t> projectReference;};
enum class DocumentKind {Project,Segment,Style,Band,Collection,AudioPath,ChordMap,Wave,Script,Container,ToolGraph};
struct OpenedDocument {DocumentKind kind;size_t index;};
enum class RuntimeDocumentKind {Segment,Style,Band,Collection,AudioPath,Container};
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
    std::vector<OpenChordMap> chordMaps_;
    std::vector<OpenWave> waves_;
    std::vector<OpenScript> scripts_;
    std::vector<OpenContainer> containers_;
    std::vector<OpenToolGraph> toolGraphs_;
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
    static DocumentKind document_kind(const std::wstring& path);
    OpenedDocument open_document(const std::wstring& path);
    // Ownership is acyclic: Framework -> documents -> RIFF and track models.
    // UI stores indexes, never raw pointers across document creation/deletion.
    void new_project();
    size_t new_segment();
    size_t import_midi_segment(const std::wstring& path);
    size_t open_segment(const std::wstring& path);
    size_t new_style();size_t open_style(const std::wstring& path);
    size_t new_band();size_t open_band(const std::wstring& path);
    void save_band(size_t index,const std::wstring& path);
    bool set_band_instrument(size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool undo_band(size_t index);bool redo_band(size_t index);
    bool add_band_gm_instrument(size_t index,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume);
    bool assign_band(size_t segmentIndex,size_t bandIndex,std::int32_t time);
    bool assign_style_band(size_t segmentIndex,size_t styleIndex,size_t bandIndex,std::int32_t time);
    bool assign_style_reference(size_t segmentIndex,size_t styleIndex,std::int32_t time,std::optional<size_t> event={},size_t trackIndex=0);
    bool delete_style_reference(size_t segmentIndex,size_t event,size_t trackIndex=0);
    bool assign_chordmap_reference(size_t segmentIndex,size_t mapIndex,std::int32_t time=0,size_t trackIndex=0);
    std::vector<ResolvedChordMap> playback_chordmaps(size_t segmentIndex) const;
    bool compose_chords(size_t segmentIndex,unsigned activity=1);
    bool undo_segment(size_t);bool redo_segment(size_t);
    bool assign_audio_path(size_t segmentIndex,size_t audioPathIndex);
    size_t new_audio_path();size_t open_audio_path(const std::wstring&);
    void save_audio_path(size_t,const std::wstring&);
    AudioPathDocument& audio_path_document(size_t i){return *audioPaths_.at(i).document;}
    const AudioPathDocument& audio_path_document(size_t i) const{return *audioPaths_.at(i).document;}
    const std::vector<OpenAudioPath>& audio_paths() const{return audioPaths_;}
    size_t new_chordmap();size_t open_chordmap(const std::wstring&);void save_chordmap(size_t,const std::wstring&);
    ChordMapDocument& chordmap_document(size_t i){return *chordMaps_.at(i).document;}
    const ChordMapDocument& chordmap_document(size_t i) const{return *chordMaps_.at(i).document;}
    const std::vector<OpenChordMap>& chordmaps() const{return chordMaps_;}
    bool assign_segment_trigger(size_t owner,size_t target,bool motif,const std::wstring& motifName,std::int32_t logical,std::int32_t physical,std::uint32_t flags,std::optional<size_t> event={},size_t track=0);
    bool assign_script_call(size_t owner,size_t script,const std::wstring& routine,std::int32_t logical,std::int32_t physical,std::uint32_t timing,std::optional<size_t> event={},size_t track=0);
    SegmentTriggerPlayback trigger_playback(size_t owner) const;
    bool insert_segment_wave(size_t segment,size_t wave,size_t part,std::int64_t time,std::uint32_t variations,size_t* resultingIndex=nullptr);
    std::vector<ResolvedWave> playback_waves(size_t segment)const;
    size_t open_wave(const std::wstring&);void save_wave(size_t,const std::wstring&);
    const std::vector<OpenWave>& waves()const{return waves_;}
    WaveDocument& wave_document(size_t i){return *waves_.at(i).document;}
    const WaveDocument& wave_document(size_t i)const{return *waves_.at(i).document;}
    size_t new_tool_graph();size_t open_tool_graph(const std::wstring&);void save_tool_graph(size_t,const std::wstring&);
    const std::vector<OpenToolGraph>& tool_graphs()const{return toolGraphs_;}
    ToolGraphDocument& tool_graph_document(size_t i){return *toolGraphs_.at(i).document;}
    const ToolGraphDocument& tool_graph_document(size_t i)const{return *toolGraphs_.at(i).document;}
    size_t new_container();size_t open_container(const std::wstring&);void save_container(size_t,const std::wstring&);
    bool add_container_segment_reference(size_t,size_t,const std::wstring&,bool keep=false);
    const std::vector<OpenContainer>& containers()const{return containers_;}
    ContainerDocument& container_document(size_t i){return *containers_.at(i).document;}
    const ContainerDocument& container_document(size_t i)const{return *containers_.at(i).document;}
    size_t new_script();size_t open_script(const std::wstring&);void save_script(size_t,const std::wstring&);
    bool add_script_segment_reference(size_t script,size_t segment,const std::wstring& alias,bool keep=false);
    const std::vector<OpenScript>& scripts()const{return scripts_;}
    ScriptDocument& script_document(size_t i){return *scripts_.at(i).document;}
    const ScriptDocument& script_document(size_t i)const{return *scripts_.at(i).document;}
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
    void save_runtime_as(RuntimeDocumentKind,size_t,const std::wstring& destination);
    std::wstring runtime_project_folder() const;
    std::wstring runtime_component_folder(RuntimeDocumentKind) const;
    std::wstring runtime_filename(RuntimeDocumentKind,size_t) const;
    std::wstring runtime_file_folder(RuntimeDocumentKind,size_t) const;
    bool set_runtime_file_folder(RuntimeDocumentKind,size_t,const std::wstring&);
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
