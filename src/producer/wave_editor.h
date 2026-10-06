#pragma once
#include "framework.h"
#include <windows.h>
namespace producer::app {
struct WaveEditorContext {Bytes clipboard;size_t part=0;};
void show_wave_editor(HWND,Framework&,size_t,WaveEditorContext&);
}
