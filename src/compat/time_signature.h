#pragma once
#include <windows.h>
#include <guiddef.h>
#include <cstddef>

namespace producer {
// TimeSigStripMgr 5.3.0.900: verified by Inspect-TimeSignatureAbi.mjs.
inline constexpr GUID CLSID_TimeSignatureMgr =
    {0x8c6005d2,0xabda,0x11d2,{0xb0,0xd9,0x00,0x10,0x5a,0x26,0x62,0x0b}};
inline constexpr GUID GUID_TimeSignatureParam =
    {0xd2ac28a4,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
inline constexpr GUID GUID_TimeSignatureBorrowedObject =
    {0xf9a03440,0x38f3,0x11d2,{0x89,0xb5,0x00,0xc0,0x4f,0xd9,0x12,0xc8}};
inline constexpr GUID GUID_TimeSignatureUndoLabel =
    {0x178633a6,0x4452,0x11d2,{0x89,0x0c,0x00,0xc0,0x4f,0xbf,0x8d,0x15}};
inline constexpr GUID CLSID_DirectMusicTimeSignatureTrack =
    {0xd2ac2888,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
// SetStripMgrProperty(1) QI at RVA 0x772a. Same OS track IID used by Tempo.
inline constexpr GUID IID_TimeSignatureRuntimeTrack =
    {0xf96029a1,0x4282,0x11d2,{0x87,0x17,0x00,0x60,0x08,0x93,0xb1,0xbd}};
// Timeline registration order at RVA 0x7829, 0x783f, 0x7853.
// SDK GUID_IDirectMusicStyle; Tempo is d2ac28a5, not d2ac28a1.
inline constexpr GUID GUID_TimeSignatureStyleParam =
    {0xd2ac28a1,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
inline constexpr GUID GUID_TimeSignatureRefreshPositions =
    {0x96a0a26c,0xf4e7,0x11d1,{0x88,0xcb,0x00,0xc0,0x4f,0xbf,0x8d,0x15}};
inline constexpr GUID TimeSignatureNotifications[] = {
    GUID_TimeSignatureRefreshPositions,GUID_TimeSignatureStyleParam,GUID_TimeSignatureParam
};
struct TimeSignatureParam { LONG time; BYTE beats; BYTE denominator; WORD grids; };
static_assert(sizeof(TimeSignatureParam)==8 && offsetof(TimeSignatureParam,grids)==6);
struct TimeSignatureTrackHeader { GUID classId; DWORD position; DWORD groups; DWORD chunk; DWORD list; };
static_assert(sizeof(TimeSignatureTrackHeader)==32 && offsetof(TimeSignatureTrackHeader,groups)==20);
}
