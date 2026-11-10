#pragma once
#include "command_editor.h"
namespace producer::app {
struct MuteEditorContext {CommandEditorContext selection;Bytes clipboard;};
void show_mute_editor(HWND,Framework&,size_t,MuteEditorContext&);
}
