#pragma once
#include "command_editor.h"
#include "properties_window.h"
namespace producer::app {
struct MarkerEditorContext {CommandEditorContext selection;Bytes clipboard;};
using MarkerPropertiesChanged=std::function<void(std::optional<MarkerPropertiesTarget>,bool)>;
void show_marker_editor(HWND,Framework&,size_t,MarkerEditorContext&,MarkerPropertiesChanged);
void refresh_marker_editor();
void select_marker_editor_event(size_t);
bool translate_marker_editor(MSG&);
void release_marker_editor();
}
