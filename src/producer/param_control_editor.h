#pragma once
#include "command_editor.h"
namespace producer::app {
struct ParamControlEditorContext {CommandEditorContext selection;size_t object=0,parameter=0;Bytes clipboard;};
void show_param_control_editor(HWND,Framework&,size_t,ParamControlEditorContext&);
}
