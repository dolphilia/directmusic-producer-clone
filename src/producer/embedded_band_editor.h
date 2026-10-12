#pragma once
#include "embedded_band_target.h"
#include "segment_timeline_window.h"
namespace producer::app {
void show_embedded_band_editor(HWND workspace,Framework&,EmbeddedBandTarget,TimelineSelectionChanged,TimelineHistoryRequested,TimelineBandMixStarted={});
void refresh_embedded_band_editors();
bool translate_embedded_band_editor(MSG&);
void release_embedded_band_editors();
size_t embedded_band_editor_count();
}
