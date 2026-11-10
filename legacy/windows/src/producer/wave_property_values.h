#pragma once
#include "wave_document.h"
#include <cwctype>
#include <limits>
#include <stdexcept>

namespace producer::app {
inline std::wstring wave_pitch_name(unsigned value){
    if(value>127)throw std::runtime_error("Root note requires MIDI 0-127");
    constexpr const wchar_t* names[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
    return std::wstring(names[value%12])+std::to_wstring(value/12);
}
inline unsigned wave_pitch_value(std::wstring value){
    if(value.empty())throw std::runtime_error("Root note requires a pitch name or MIDI 0-127");
    for(auto& c:value)c=static_cast<wchar_t>(std::towupper(c));
    if(value.find_first_not_of(L"0123456789")==std::wstring::npos){
        const auto n=std::stoull(value);if(n>127)throw std::runtime_error("Root note requires MIDI 0-127");return static_cast<unsigned>(n);
    }
    const std::wstring letters=L"CDEFGAB";const auto at=letters.find(value[0]);
    if(at==std::wstring::npos)throw std::runtime_error("Invalid pitch name");
    constexpr int notes[]={0,2,4,5,7,9,11};int note=notes[at];size_t digit=1;
    if(digit<value.size()&&(value[digit]==L'#'||value[digit]==L'B')){note+=value[digit]==L'#'?1:-1;++digit;}
    const auto octave=value.substr(digit);if(octave.empty()||octave.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Invalid pitch octave");
    const auto n=std::stoull(octave);if(n>10)throw std::runtime_error("Pitch outside MIDI range");
    const auto pitch=static_cast<int>(n)*12+note;if(pitch<0||pitch>127)throw std::runtime_error("Pitch outside MIDI range");return static_cast<unsigned>(pitch);
}
enum class WaveLoopField {start,end,length};
inline WaveSampleLoop wave_loop_value(WaveSampleLoop loop,WaveLoopField field,std::uint64_t value,std::uint64_t frames,bool locked){
    if(!loop.length||!frames)throw std::runtime_error("Wave has no nonempty loop range");
    std::uint64_t start=loop.start,length=loop.length;
    if(field==WaveLoopField::length){if(locked)throw std::runtime_error("Loop length is locked");length=value;}
    else if(field==WaveLoopField::start){if(!locked){const auto end=start+length-1;if(value>end)throw std::runtime_error("Loop start follows end");length=end-value+1;}start=value;}
    else if(locked){if(value+1<length||value==UINT64_MAX)throw std::runtime_error("Locked loop end precedes its length");start=value+1-length;}
    else {if(value<start||value==UINT64_MAX)throw std::runtime_error("Loop end precedes start");length=value-start+1;}
    if(start>UINT32_MAX||!length||length>UINT32_MAX||start>frames||length>frames-start)throw std::runtime_error("Loop range is outside Wave samples");
    loop.start=static_cast<std::uint32_t>(start);loop.length=static_cast<std::uint32_t>(length);return loop;
}
}
