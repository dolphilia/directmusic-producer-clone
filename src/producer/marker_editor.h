#pragma once
#include "command_editor.h"
namespace producer::app {
struct MarkerEditorContext {CommandEditorContext selection;Bytes clipboard;};
void show_marker_editor(HWND,Framework&,size_t,MarkerEditorContext&);
}
