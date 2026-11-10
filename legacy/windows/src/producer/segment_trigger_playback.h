#pragma once
#include "style.h"
#include "wave_playback.h"
namespace producer::app {
struct TriggeredSegmentSnapshot {std::wstring path;Bytes bytes;std::array<std::uint8_t,16> objectId{};};
struct TriggeredScriptSnapshot {std::wstring path;Bytes bytes;std::array<std::uint8_t,16> objectId{};std::vector<std::wstring> routines;};
struct TriggeredMotif {std::array<std::uint8_t,16> styleId{};std::wstring name;};
struct SegmentTriggerPlayback {
    Bytes source,parent;
    std::vector<TriggeredScriptSnapshot> scripts;
    std::vector<TriggeredSegmentSnapshot> segments;
    std::vector<ResolvedStyle> styles;
    std::vector<ResolvedCollection> collections;
    std::vector<ResolvedWave> waves;
    std::vector<TriggeredMotif> motifs;
};
}
