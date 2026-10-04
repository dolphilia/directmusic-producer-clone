#pragma once
#include <windows.h>
#include "framework.h"
#include <map>
namespace producer::app {
struct CommandEditorContext {
    std::map<std::uint32_t,size_t> tracks;
    std::map<std::pair<std::uint32_t,size_t>,size_t> events;
};
void show_command_editor(HWND parent,Framework&,size_t documentIndex,CommandEditorContext&);
}
