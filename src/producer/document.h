#pragma once
#include "riff.h"
#include "segment_structure.h"
#include "tempo/tempo_track.h"
#include "timeline.h"
#include "sequence.h"
#include "command.h"
#include "chord.h"
#include "signpost.h"
#include "marker.h"
#include "lyric.h"
#include "mute.h"
#include "param_control.h"
#include "wave_track.h"
#include "style.h"
#include "segment_trigger.h"
#include "script_track.h"
#include "chordmap_reference.h"
#include <functional>

namespace producer::app {
struct SequenceVelocityEdit {std::uint32_t groups;size_t track,note;unsigned velocity;};
struct SequenceTimingEdit {std::uint32_t groups;size_t track,note;std::int32_t time,duration;};
// One document owns both its RIFF tree and editable track. History stores the
// complete serialized tree, so unrelated chunks cannot disappear on Undo.
class SegmentDocument {
    Chunk root_;
    tempo::Track tempo_;
    bool hasTempo_=false, dirty_=false;
    Bytes saved_;
    std::vector<Bytes> undo_, redo_;
    std::vector<ResolvedStyle> styles_;
    std::wstring styleDirectory_;
    std::vector<StyleCatalogEntry> styleCatalog_;
    bool styleRuntimeReferences_=false;
    void resolve_style_history(const SegmentDocument& previous);
    std::uint32_t selectedGroups_=1;
    size_t tempoIndex_=0,meterIndex_=0,sequenceIndex_=0,bandIndex_=0;
    TimelineSelection range_;
    Timeline explicit_timeline(bool& found) const;
    Chunk* tempo_chunk();
    void import(const Bytes& bytes);
    void restore_history(const Bytes& bytes);
    void commit_tempo(const Bytes& before);
    void record_edit(const Bytes& before);
    bool update_parameter_control(size_t,const std::function<bool(Chunk&)>&);
    bool update_style_reference(size_t,const std::function<bool(Chunk&)>&,const std::wstring&,const std::vector<StyleCatalogEntry>&);
    Chunk* meter_chunk();
    bool update_meters(std::vector<meter::Event> events);
public:
    SegmentDocument();
    static SegmentDocument create_empty(const SegmentCreationLength&,const std::wstring& name);
    std::vector<SegmentTrackInfo> tracks()const;
    bool add_tracks(const std::vector<SegmentTrackKind>&);
    bool add_sequence_part(std::uint32_t channel,size_t* resultingTrack=nullptr);
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
    std::wstring name() const;
    bool set_name(const std::wstring&);
    std::int32_t length() const;
    SegmentCreationLength length_settings()const;
    SegmentLengthPreview preview_length(const SegmentCreationLength&)const;
    // Refuses shortening that would remove/clip track data until typed trimming
    // is available. Invalid edits never change bytes, dirty state or history.
    bool set_length(const SegmentCreationLength&);
    const std::vector<tempo::Event>& tempos() const { return tempo_.events(); }
    void select(size_t index);
    void select(const std::vector<size_t>& indexes);
    void select_all();
    bool add_tempo(std::int32_t time,double bpm);
    bool change_selected(double bpm);
    std::optional<double> tempo_at(std::int32_t time) const;
    bool change_current_tempo(std::int32_t time,double bpm);
    bool move_selected_tempo(std::int32_t time);
    bool delete_selected();
    Bytes copy() const;
    bool paste(const Bytes& bytes,std::int32_t at);
    bool select_range(TimelineSelection);
    TimelineSelection selected_range() const {return range_;}
    Bytes copy_range() const;
    bool delete_range();
    bool paste_range(const Bytes&,std::int32_t at,bool overwrite=false);
    bool move_range(std::int32_t at);
    bool can_undo() const {return !undo_.empty();}
    bool can_redo() const {return !redo_.empty();}
    bool undo(); bool redo();
    Timeline timeline() const;
    std::vector<ScriptEvent> script_calls(size_t trackIndex=0) const;
    bool add_script_call(const ScriptEvent&,size_t trackIndex=0);
    bool edit_script_call(size_t,const ScriptEvent&,size_t trackIndex=0);
    bool delete_script_call(size_t,size_t trackIndex=0);
    std::vector<SegmentTrigger> triggers(size_t trackIndex=0) const;
    bool add_trigger(const SegmentTrigger&,size_t trackIndex=0);
    bool edit_trigger(size_t,const SegmentTrigger&,size_t trackIndex=0);
    bool delete_trigger(size_t,size_t trackIndex=0);
    std::vector<StyleReference> style_references() const;
    std::vector<ChordMapReference> chordmap_references() const;
    std::vector<ChordMapReference> chordmap_references(size_t trackIndex) const;
    bool set_chordmap_reference(const ChordMapReference&,size_t trackIndex=0);
    bool clear_chordmap_reference(size_t trackIndex=0);
    std::vector<StyleReference> style_references(size_t trackIndex) const;
    bool add_style_reference(const StyleReference&,const std::wstring&,const std::vector<StyleCatalogEntry>&,size_t trackIndex=0);
    bool edit_style_reference(size_t,const StyleReference&,const std::wstring&,const std::vector<StyleCatalogEntry>&,size_t trackIndex=0);
    bool delete_style_reference(size_t,const std::wstring&,const std::vector<StyleCatalogEntry>&,size_t trackIndex=0);
    void resolve_style_context(const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog={},bool runtimeReferences=false);
    const std::vector<ResolvedStyle>& styles() const {return styles_;}
    bool set_meter(std::int32_t measure,unsigned beats,unsigned denominator,unsigned grids);
    bool can_edit_meter() const;
    bool edit_meter(size_t index,std::int32_t measure,unsigned beats,unsigned denominator,unsigned grids,size_t* selected=nullptr);
    bool delete_meter(std::int32_t measure);
    std::vector<Note> notes() const;
    std::vector<SequenceCurve> curves() const;
    std::vector<SequenceController> cc_lanes(std::uint32_t channel) const;
    bool add_cc_lane(std::uint32_t channel,SequenceController);
    bool add_curve(const SequenceCurve&,size_t* resultingIndex=nullptr);
    bool edit_curve(size_t,const SequenceCurve&,size_t* resultingIndex=nullptr);
    bool delete_curve(size_t);
    bool add_note(Note note,size_t* resultingIndex=nullptr);
    bool edit_note(size_t index,Note note,size_t* resultingIndex=nullptr);
    // One owner transaction, even when a velocity command spans several parts.
    bool edit_note_velocities(const std::vector<SequenceVelocityEdit>&);
    bool edit_note_timings(const std::vector<SequenceTimingEdit>&,std::vector<size_t>* resultingIndices=nullptr);
    bool delete_note(size_t index);
    std::vector<CommandEvent> commands(size_t trackIndex=0) const;
    // Derive stored measure/beat from this document's selected Timeline.
    bool add_command(CommandEvent event,size_t trackIndex=0);
    bool edit_command(size_t index,CommandEvent event,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_command(size_t index,size_t trackIndex=0);
    size_t chord_track_count() const;
    std::vector<ChordEvent> chords(size_t trackIndex=0) const;
    bool set_chord(ChordEvent event,size_t trackIndex=0);
    bool edit_chord(size_t index,ChordEvent event,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_chord(size_t index,size_t trackIndex=0);
    bool replace_composed_chords(const Chunk& cord,size_t trackIndex=0);
    std::vector<SignpostEvent> signposts(size_t trackIndex=0) const;
    bool set_signpost(SignpostEvent,size_t trackIndex=0);
    bool edit_signpost(size_t,SignpostEvent,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_signpost(size_t,size_t trackIndex=0);
    std::vector<MarkerEvent> markers(size_t trackIndex=0) const;
    bool add_marker(MarkerEvent,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool edit_marker(size_t,MarkerEvent,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_marker(size_t,size_t trackIndex=0);
    // Contiguous source range [begin,end); original division-selection ABI is separate.
    bool mark_boundaries(MarkerKind,unsigned division,std::int32_t begin,std::int32_t end,bool mark,size_t trackIndex=0);
    Bytes copy_marker(size_t,size_t trackIndex=0) const;
    bool paste_marker(const Bytes&,std::int32_t,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    std::vector<LyricEvent> lyrics(size_t trackIndex=0) const;
    bool add_lyric(const LyricEvent&,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool edit_lyric(size_t,const LyricEvent&,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_lyric(size_t,size_t trackIndex=0);
    Bytes copy_lyric(size_t,size_t trackIndex=0) const;
    bool paste_lyric(const Bytes&,std::int32_t physical,std::int32_t logical,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    std::vector<MuteEvent> mutes(size_t trackIndex=0) const;
    bool add_mute(MuteEvent,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool edit_mute(size_t,MuteEvent,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_mute(size_t,size_t trackIndex=0);
    bool toggle_mute_beat(std::uint32_t channel,std::int32_t at,size_t trackIndex=0);
    Bytes copy_mute(size_t,size_t trackIndex=0) const;
    bool paste_mute(const Bytes&,std::int32_t,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    std::vector<ParamControlObject> parameter_controls(size_t trackIndex=0) const;
    bool add_parameter_object(const ParamObject& value,size_t trackIndex=0);
    bool edit_parameter_object(size_t object,const ParamObject& value,size_t trackIndex=0);
    bool delete_parameter_object(size_t object,size_t trackIndex=0);
    bool add_parameter(size_t object,std::uint32_t index,size_t trackIndex=0);
    bool delete_parameter(size_t object,size_t parameter,size_t trackIndex=0);
    bool add_parameter_curve(size_t object,size_t parameter,const ParamCurve& value,size_t trackIndex=0);
    bool edit_parameter_curve(size_t object,size_t parameter,size_t curve,const ParamCurve& value,size_t trackIndex=0);
    bool delete_parameter_curve(size_t object,size_t parameter,size_t curve,size_t trackIndex=0);
    bool paste_parameter_curve(size_t object,size_t parameter,const Bytes& bytes,std::int32_t at,size_t trackIndex=0);
    Bytes copy_parameter_curve(size_t object,size_t parameter,size_t curve,size_t trackIndex=0) const;
    std::vector<BandEvent> band_events() const;
    std::vector<WaveItem> waves(size_t trackIndex=0) const;
    bool insert_wave(size_t part,const WaveReference&,const WavePlacement&,std::uint32_t variations,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool edit_wave(size_t part,size_t item,const WavePlacement&,size_t trackIndex=0);
    bool edit_wave_loop(size_t part,size_t item,const WaveItemLoop&,size_t trackIndex=0);
    Bytes copy_wave(size_t part,size_t item,size_t trackIndex=0) const;
    bool paste_wave(const Bytes&,size_t part,std::int64_t time,size_t trackIndex=0,size_t* resultingIndex=nullptr);
    bool delete_wave(size_t part,size_t item,size_t trackIndex=0);
    bool set_band(std::int32_t time,const Bytes& band);
    // Original Band-track Insert: existing selected track, an unoccupied beat,
    // independent inactive16-channel copy, logical beat / physical beat-minus1.
    bool insert_default_band(std::int32_t beat,size_t* resultingIndex=nullptr);
    // Embedded Band editor/Properties commit bytes and timing as one owner edit.
    // Index is explicit; unlike set_band this never creates or replaces another event.
    bool update_band(size_t index,std::int32_t logical,std::int32_t physical,const Bytes& band,size_t* resultingIndex=nullptr);
    bool set_audio_path(const Bytes&);
    Bytes audio_path() const;
    Bytes tool_graph() const;
    bool set_tool_graph(const Bytes&);
    bool remove_tool_graph();
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
