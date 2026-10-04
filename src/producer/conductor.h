#pragma once
#include "riff.h"
#include "style.h"
#include "command.h"
#include <windows.h>
#include <memory>

namespace producer::app {
struct RuntimeCall { std::string operation; HRESULT result; };
struct PlaybackPosition { bool playing; LONG clocks,start; double tempo;bool tempoAvailable=false; };
struct CommandParameterSample { HRESULT result; LONG next; CommandEvent event; };
using PlaybackId=std::uint64_t;
struct PlaybackNotification { GUID type; DWORD option,field1,field2,group; LONG clocks; bool currentSegment; PlaybackId playbackId=0; };
struct PlaybackNote { LONG clocks,duration;DWORD channel,group;WORD musicValue;BYTE midiValue,velocity,flags,playMode; };
struct MotifSelection {size_t styleIndex;std::wstring name;};
enum class PlaybackBoundary {Immediate,Stored,Grid,Beat,Measure};
struct PlaybackOptions {
    PlaybackBoundary boundary=PlaybackBoundary::Immediate;
    bool afterPrepareTime=true,secondary=false;
    LONG delayClocks=0;
};
struct PlaybackRequest {DWORD flags=0;LONG submittedClocks=0,requestedClocks=0;DWORD runtimeDefaultResolution=0;LONG actualStart=0;};
struct PlaybackSessionView {PlaybackId id;std::wstring name;bool secondary;PlaybackPosition position;};
// Private playback copy: an embedded Segment AudioPath takes precedence.
// An empty Segment produces a Motif configuration carrier only.
Bytes prepare_transport_audio_path(const Bytes& segment,const Bytes& audioPath);
// All COM interfaces, the memory descriptor backing bytes and downloads have
// one owner and are released before CoUninitialize on the calling UI thread.
class Conductor {
    struct State;
    std::unique_ptr<State> state_;
    std::vector<RuntimeCall> calls_;
    std::vector<std::wstring> servers_;
    Bytes defaultAudioPath_;
    void check(const char*,HRESULT);
    void stop_current();
    void register_notification_identity();
    void collect_notifications();
    void play_snapshot(StylePlaybackSnapshot,const std::wstring&,HWND,const std::vector<ResolvedCollection>&,const std::optional<MotifSelection>&,const PlaybackOptions& = {});
public:
    Conductor();~Conductor();
    Conductor(const Conductor&)=delete;Conductor& operator=(const Conductor&)=delete;
    // Applies to subsequent requests, including Motifs. Active sessions retain
    // their owned path; live reconfiguration is a separate pending operation.
    void set_default_audio_path(const Bytes&);
    const Bytes& default_audio_path() const {return defaultAudioPath_;}
    void play(const Bytes&,const std::wstring& referenceDirectory,HWND owner,const std::vector<ResolvedStyle>& styles={},const std::vector<ResolvedCollection>& collections={},const std::optional<MotifSelection>& motif={});
    // Standalone owned Style. Optional owned AudioPath uses a private config
    // carrier only; the playable Segment is still obtained from GetMotif.
    void play_motif(const StyleCatalogEntry&,const std::wstring& name,HWND owner,const std::vector<ResolvedCollection>& collections={},const PlaybackOptions& options={},const Bytes& audioPath={});
    PlaybackRequest playback_request() const;
    PlaybackId current_playback_id() const;
    std::vector<PlaybackId> playback_ids() const;
    std::vector<PlaybackSessionView> playback_sessions();
    PlaybackPosition position(PlaybackId);
    void stop(PlaybackId);
    void stop();void shutdown() noexcept;
    PlaybackPosition position();
    // Value copies only; the runtime message and its COM references never
    // escape the UI-thread owner. Empty queue (S_FALSE) is not an error.
    std::vector<PlaybackNotification> notifications();
    size_t notification_identity_count()const;
    size_t pending_notification_count()const;
    // Optional bounded observer for the actual generated runtime notes. It
    // forwards messages unchanged; enable before the first play.
    void enable_note_observation();
    std::vector<PlaybackNote> observed_notes() const;
    bool note_observation_overflow() const;
    bool note_observation_forwarding_failed() const;
    StyleMeter segment_meter(LONG time,DWORD groups=1,DWORD index=0);
    // Queries do not change the loaded tracks or playback scope.
    DWORD segment_track_group(REFGUID type,DWORD groups,DWORD index);
    double segment_tempo(LONG time,DWORD groups=1,DWORD index=0);
    CommandEvent segment_command(LONG time,DWORD groups=1,DWORD index=0);
    // Preserve the runtime status and next boundary for observation. Param2
    // includes its own time; it is not silently replaced by the query time.
    CommandParameterSample command_parameter(LONG time,DWORD groups,DWORD index,bool withTime);
    bool initialized() const;
    const Bytes& playback_bytes() const;
    const std::vector<ResolvedStyle>& playback_styles() const;
    const std::vector<ResolvedCollection>& playback_collections() const;
    const std::vector<DWORD>& playback_collection_first_patches() const;
    const std::vector<RuntimeCall>& calls() const {return calls_;}
    const std::vector<std::wstring>& servers() const {return servers_;}
};
}
