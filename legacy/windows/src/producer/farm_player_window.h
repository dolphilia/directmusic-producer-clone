#pragma once
#include <windows.h>
namespace producer::app {
class Framework;
// Farm sample score controls use a private runtime; closing the window releases
// its script, segment states and Performance without stopping the main player.
void show_farm_player(HWND parent,Framework&);
}
