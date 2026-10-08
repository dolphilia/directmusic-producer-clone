#pragma once
#include "framework.h"
#include "conductor.h"
namespace producer::app {
std::optional<size_t> show_style_player(HWND,Framework&,Conductor&,std::optional<size_t> initialStyle={});
}
