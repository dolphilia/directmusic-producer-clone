#pragma once
#include "riff.h"
#include <windows.h>
#include <filesystem>

namespace producer::app {
struct PerformanceSettings {
    DWORD voices=64,sampleRate=22050,latencyMs=55;
    bool persistLatency=false;
    GUID defaultSynth{};
};
bool operator==(const PerformanceSettings&,const PerformanceSettings&);
inline bool operator!=(const PerformanceSettings& a,const PerformanceSettings& b){return !(a==b);}
void validate_performance_settings(const PerformanceSettings&);
Bytes encode_performance_settings(const PerformanceSettings&);
PerformanceSettings decode_performance_settings(const Bytes&);
PerformanceSettings load_performance_settings(const std::filesystem::path&);
void save_performance_settings(const std::filesystem::path&,const PerformanceSettings&);
}
