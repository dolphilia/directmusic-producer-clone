#pragma once
#include "command_editor.h"
namespace producer::app {
struct LyricEditorContext {CommandEditorContext selection;Bytes clipboard;};
void show_lyric_editor(HWND,Framework&,size_t,LyricEditorContext&);
}
