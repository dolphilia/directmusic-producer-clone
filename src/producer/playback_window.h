#pragma once
#include "conductor.h"
namespace producer::app {
inline constexpr UINT PlaybackStoppedMessage=WM_APP+21;
void show_playback_window(HWND parent,Conductor&);
}
