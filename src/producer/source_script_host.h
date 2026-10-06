#pragma once
#include "script_document.h"
#include "compat/script_runtime.h"
namespace producer::app {
bool uses_source_script_host(const ScriptDocument&);
Bytes source_script_loader_bytes(const ScriptDocument&);
// The helper is an OS Script containing only host-private bridge variables and
// the original Container. Authored source is parsed once, unchanged, by the
// declared OS Active Scripting language engine.
HRESULT create_source_script_host(const ScriptDocument&,runtime::Script* helper,runtime::Performance*,runtime::Script**,runtime::ScriptErrorInfo*);
}
