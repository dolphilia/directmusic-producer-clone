#pragma once
#include <windows.h>
#include <unknwn.h>
#include <cstddef>

namespace producer {
// DirectX SDK declarations checked against the sources in sdk-reference-sources.json.
inline constexpr GUID IID_DirectMusicTrack =
    {0xf96029a1, 0x4282, 0x11d2, {0x87, 0x17, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}};
inline constexpr GUID GUID_TempoParam =
    {0xd2ac28a5, 0xb39b, 0x11d1, {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}};
struct TempoParam { LONG time; double tempo; };
static_assert(sizeof(TempoParam) == 16 && offsetof(TempoParam, tempo) == 8);
}
