#include "performance_settings.h"
#include <cstring>
#include <stdexcept>

namespace producer::app {
bool operator==(const PerformanceSettings& a,const PerformanceSettings& b){
    return a.voices==b.voices&&a.sampleRate==b.sampleRate&&a.latencyMs==b.latencyMs&&a.persistLatency==b.persistLatency&&IsEqualGUID(a.defaultSynth,b.defaultSynth);
}
void validate_performance_settings(const PerformanceSettings& s){
    if(!s.voices)throw std::runtime_error("Number of Voices must be greater than zero");
    if(!s.sampleRate)throw std::runtime_error("Sample Rate must be greater than zero");
    if(s.latencyMs>9999)throw std::runtime_error("Latency must be 0 to 9999 milliseconds");
}
Bytes encode_performance_settings(const PerformanceSettings& s){
    validate_performance_settings(s);Bytes b(40);std::memcpy(b.data(),"PREF",4);
    put32(b,4,1);put32(b,8,s.voices);put32(b,12,s.sampleRate);put32(b,16,s.latencyMs);put32(b,20,s.persistLatency?1:0);std::memcpy(b.data()+24,&s.defaultSynth,16);return b;
}
PerformanceSettings decode_performance_settings(const Bytes& b){
    if(b.size()!=40||std::memcmp(b.data(),"PREF",4)||read32(b,4)!=1||read32(b,20)>1)throw std::runtime_error("Invalid Performance preferences");
    PerformanceSettings s;s.voices=read32(b,8);s.sampleRate=read32(b,12);s.latencyMs=read32(b,16);s.persistLatency=read32(b,20)!=0;std::memcpy(&s.defaultSynth,b.data()+24,16);validate_performance_settings(s);return s;
}
PerformanceSettings load_performance_settings(const std::filesystem::path& p){
    if(p.empty()||!std::filesystem::exists(p))return {};return decode_performance_settings(read_file(p.wstring()));
}
void save_performance_settings(const std::filesystem::path& p,const PerformanceSettings& s){
    const auto bytes=encode_performance_settings(s);if(p.empty())throw std::runtime_error("Performance preferences path is empty");
    if(!p.parent_path().empty())std::filesystem::create_directories(p.parent_path());write_file_atomic(p.wstring(),bytes);
}
}
