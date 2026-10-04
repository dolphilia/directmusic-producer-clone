#pragma once
#include "riff.h"
#include "tempo/tempo_track.h"
#include "timeline.h"
#include "sequence.h"
#include "command.h"
#include "style.h"
#include <functional>

namespace producer::app {
// One document owns both its RIFF tree and editable track. History stores the
// complete serialized tree, so unrelated chunks cannot disappear on Undo.
class SegmentDocument {
    Chunk root_;
    tempo::Track tempo_;
    bool hasTempo_=false, dirty_=false;
    Bytes saved_;
    std::vector<Bytes> undo_, redo_;
    std::vector<ResolvedStyle> styles_;
    std::uint32_t selectedGroups_=1;
    size_t tempoIndex_=0,meterIndex_=0,sequenceIndex_=0,bandIndex_=0;
    Timeline explicit_timeline(bool& found) const;
    Chunk* tempo_chunk();
    void import(const Bytes& bytes);
    void commit_tempo(const Bytes& before);
    void record_edit(const Bytes& before);
    Chunk* meter_chunk();
    bool update_meters(std::vector<meter::Event> events);
public:
    SegmentDocument();
    void load(const Bytes& bytes);
    // Select nth track of each type whose group bits intersect the mask.
    // Editor context only: selection never changes RIFF bytes or history.
    void select_track_group(std::uint32_t groups,size_t tempoIndex=0,size_t meterIndex=0,size_t sequenceIndex=0,size_t bandIndex=0);
    std::uint32_t selected_groups() const {return selectedGroups_;}
    size_t selected_tempo_index() const {return tempoIndex_;}
    size_t selected_meter_index() const {return meterIndex_;}
    size_t selected_sequence_index() const {return sequenceIndex_;}
    size_t selected_band_index() const {return bandIndex_;}
    Bytes save_bytes() const { return root_.encode(); }
    void save(const std::wstring& path);
    bool dirty() const { return dirty_; }
    std::int32_t length() const;
    const std::vector<tempo::Event>& tempos() const { return tempo_.events(); }
    void select(size_t index);
    void select_all();
    bool add_tempo(std::int32_t time,double bpm);
    bool change_selected(double bpm);
    bool delete_selected();
    Bytes copy() const;
    bool paste(const Bytes& bytes,std::int32_t at);
    bool undo(); bool redo();
    Timeline timeline() const;
    std::vector<StyleReference> style_references() const;
    void resolve_style_context(const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog={},bool runtimeReferences=false);
    const std::vector<ResolvedStyle>& styles() const {return styles_;}
    bool set_meter(std::int32_t measure,unsigned beats,unsigned denominator,unsigned grids);
    bool delete_meter(std::int32_t measure);
    std::vector<Note> notes() const;
    bool add_note(Note note);
    bool edit_note(size_t index,Note note,size_t* resultingIndex=nullptr);
    bool delete_note(size_t index);
    std::vector<CommandEvent> commands(size_t trackIndex=0) const;
    // Derive stored measure/beat from this document's selected Timeline.
    bool add_command(CommandEvent event,size_t trackIndex=0);
    bool edit_command(size_t index,CommandEvent event,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_command(size_t index,size_t trackIndex=0);
    std::vector<BandEvent> band_events() const;
    bool set_band(std::int32_t time,const Bytes& band);
    bool set_audio_path(const Bytes&);
    bool remove_audio_path();
    bool move_band(size_t index,std::int32_t logical,std::int32_t physical);
    bool delete_band(size_t index);
    bool relocate_collections(const std::vector<ResolvedCollection>&,const std::wstring& directory);
    // Rebase retained history as well as current bytes, without adding an edit.
    void relocate_context(const std::wstring& oldDirectory,const std::wstring& newDirectory,const std::vector<StyleCatalogEntry>&,const std::vector<CollectionEntry>&);
    void retarget_style(const std::wstring& directory,const std::wstring& oldPath,const std::wstring& newPath,const Bytes& style);
    static SegmentDocument playback_test(unsigned firstPitch=60);
};
}
