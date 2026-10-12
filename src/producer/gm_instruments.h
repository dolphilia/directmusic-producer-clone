#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace producer::app {
struct GmInstrument {std::uint32_t patch;const wchar_t* name;unsigned family;bool percussion;};
const std::vector<GmInstrument>& gm_instruments();
const wchar_t* gm_instrument_family(unsigned);
std::wstring gm_instrument_name(std::uint32_t patch);
std::wstring gm_drum_name(unsigned pitch);
}
