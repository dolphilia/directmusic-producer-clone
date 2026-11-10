#pragma once
#include <windows.h>
#include <unknwn.h>
#include <cstddef>

// ABI subset verified against the SDK headers recorded in host-map.md.
// IDirectMusicTrack slots 7/8 are GetParam/SetParam. IID is Track, not Track8.
namespace runtime_tempo {
inline constexpr GUID TrackIid =
    {0xf96029a1, 0x4282, 0x11d2, {0x87, 0x17, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}};
inline constexpr GUID TempoParamGuid =
    {0xd2ac28a5, 0xb39b, 0x11d1, {0x87, 0x04, 0x00, 0x60, 0x08, 0x93, 0xb1, 0xbd}};
struct TempoParam { LONG time; double tempo; };
static_assert(sizeof(TempoParam) == 16 && offsetof(TempoParam, tempo) == 8);
inline HRESULT get(IUnknown* track, LONG time, LONG* next, TempoParam* value) {
    using Get = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, LONG, LONG*, void*);
    auto table = *reinterpret_cast<void***>(track);
    return reinterpret_cast<Get>(table[7])(track, TempoParamGuid, time, next, value);
}
inline HRESULT set(IUnknown* track, LONG time, TempoParam* value) {
    using Set = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, REFGUID, LONG, void*);
    auto table = *reinterpret_cast<void***>(track);
    return reinterpret_cast<Set>(table[8])(track, TempoParamGuid, time, value);
}
}
