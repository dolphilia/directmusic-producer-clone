#pragma once
#include <windows.h>
#include "framework.h"
namespace producer::app {
// Parent remains disabled during this editor session, keeping its document
// indexes stable across project changes. Closing retains unsaved owned edits.
void show_dls_editor(HWND parent,Framework& framework,size_t collection);
std::optional<size_t> choose_dls_instrument(HWND parent,const DlsDocument& document);
}
