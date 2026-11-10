#pragma once
#include "framework.h"
#include <functional>
#include <optional>
namespace producer::app {
inline constexpr UINT SegmentWorkspaceChangedMessage=WM_APP+23;
bool segment_timeline_workspace_has_editors();
// A visible workspace remains the editor surface after its last child closes.
bool segment_timeline_workspace_visible();

enum class TimelineEventKind {tempo,lyric,marker,meter,chord,band,sequence_note};
struct TimelineEventTarget {
    TimelineEventKind kind;
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
using TimelineSelectionChanged=std::function<void(std::optional<TimelineEventTarget>,bool)>;
using TimelineHistoryRequested=std::function<void(bool redo)>;
using TimelineNoteAudition=std::function<void(size_t document,Note)>;
struct EmbeddedBandTarget;
// One gesture owns one route. A later Play cannot adopt an old preview/rollback.
using TimelineBandMixUpdate=std::function<void(const std::vector<BandMixValue>&)>;
using TimelineBandMixStarted=std::function<TimelineBandMixUpdate(const EmbeddedBandTarget&)>;
void initialize_segment_timeline_workspace(HWND parent,int editorOffset,int toolbarHeight=0);
void layout_segment_timeline_workspace(HWND parent,int editorOffset,int toolbarHeight=0);
bool segment_timeline_workspace_command(HWND parent,UINT command);
LRESULT segment_timeline_frame_proc(HWND parent,UINT,WPARAM,LPARAM);
void hide_segment_timeline_workspace();
bool activate_segment_timeline_document(size_t document);
bool focus_active_segment_timeline();
// View-owned insertion time: no RIFF/history mutation; absent for a closed editor.
std::optional<std::int32_t> segment_timeline_cursor(size_t document);
// Playback updates only the view-owned cursor, without audition/history or
// selection publication. Active note/curve gestures retain their insertion time.
void update_segment_timeline_play_cursor(size_t document,std::int64_t clocks);
// True only when a new editor was created, not when an existing one activates.
bool show_segment_timeline(HWND parent,Framework&,size_t document,TimelineSelectionChanged,TimelineHistoryRequested,TimelineNoteAudition={},TimelineBandMixStarted={});
void refresh_segment_timeline();
void select_segment_timeline_event(TimelineEventKind,size_t document,size_t track,size_t event);
void clear_segment_timeline_note_selection(size_t document,std::uint32_t groups,size_t track);
bool translate_segment_timeline(MSG&);
void release_segment_timeline(bool hideWorkspace=true);
}
