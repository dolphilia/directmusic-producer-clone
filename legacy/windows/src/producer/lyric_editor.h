#pragma once
#include "command_editor.h"
#include "properties_window.h"
namespace producer::app {
struct LyricEditorContext {CommandEditorContext selection;Bytes clipboard;};
using LyricPropertiesChanged=std::function<void(std::optional<LyricPropertiesTarget>,bool)>;
void show_lyric_editor(HWND,Framework&,size_t,LyricEditorContext&,LyricPropertiesChanged);
void refresh_lyric_editor();
void select_lyric_editor_event(size_t);
bool translate_lyric_editor(MSG&);
void release_lyric_editor();
}
