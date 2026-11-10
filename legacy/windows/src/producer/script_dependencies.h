#pragma once
#include "riff.h"
#include <array>
#include <windows.h>

namespace producer::app {
struct ScriptRuntimeDependency {
    std::array<std::uint8_t,16> classId{},objectId{};
    std::wstring path;
    Bytes sourceBytes,runtimeBytes;
};
struct ScriptRuntimeRequirement {GUID classId;const wchar_t* server;};
struct ScriptReferenceSelection {
    std::wstring filename,name,category,selectedPath;
    std::array<std::uint8_t,16> requestedId{},selectedId{};
    bool requestedGuid=false,selectedByGuid=false,requestedFile=false;
    bool requestedName=false,requestedCategory=false;
    bool selectedByFullPath=false,selectedByName=false,selectedByCategory=false;
    // Preserve an unused lower-priority IO failure as an observation. It is
    // never used as proof that the selected source or runtime succeeded.
    std::wstring fallbackPath;
    std::string fallbackReadError;
};
struct ScriptRuntimeSnapshot {
    Bytes script;
    std::vector<ScriptRuntimeDependency> dependencies;
    std::vector<ScriptRuntimeRequirement> requirements;
    std::vector<ScriptReferenceSelection> selections;
};
// No COM activation, search-directory fallback, or document writes. File
// references are resolved against the document that owns the reference.
// Container NOLOADS/KEEP survive; registering descriptors is separate from
// realizing an alias through the existing Script runtime.
ScriptRuntimeSnapshot prepare_script_runtime(const Bytes&,const std::wstring& directory);
const wchar_t* declared_script_runtime_server(REFGUID);
}
