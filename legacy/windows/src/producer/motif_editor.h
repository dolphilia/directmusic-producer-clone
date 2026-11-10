#pragma once
#include "framework.h"
#include "conductor.h"
#include <windows.h>
namespace producer::app {
void show_motif_editor(HWND parent,Framework&,size_t styleIndex,size_t patternIndex);
void show_motif_band_editor(HWND parent,Framework&,size_t styleIndex,size_t patternIndex);
struct MotifPlaybackChoice {PlaybackOptions options;std::optional<size_t> audioPath;};
std::optional<MotifPlaybackChoice> choose_motif_playback(HWND parent,const Framework&);
}
