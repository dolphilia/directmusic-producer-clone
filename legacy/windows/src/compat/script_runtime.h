#pragma once
#include "playback_runtime.h"
#include <oaidl.h>
// Frozen public dmusici.h lines750-760 / 1655-1694, not Producer ABI.
namespace producer::runtime {
inline constexpr GUID scriptClass={0x810b5013,0xe88d,0x11d2,{0x8b,0xc1,0,0x60,8,0x93,0xb1,0xb6}};
inline constexpr GUID containerClass={0x9301e380,0x1f22,0x11d3,{0x82,0x26,0xd2,0xfa,0x76,0x25,0x5d,0x47}};
inline constexpr GUID bandRuntimeClass={0x79ba9e00,0xb6ee,0x11d1,{0x86,0xbe,0,0xc0,0x4f,0xbf,0x8f,0xef}};
inline constexpr GUID bandTrackRuntimeClass={0xd2ac2894,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID segmentBaseId={0xf96029a2,0x4282,0x11d2,{0x87,0x17,0,0x60,8,0x93,0xb1,0xbd}};
inline constexpr GUID scriptId={0x2252373a,0x5814,0x489b,{0x82,9,0x31,0xfe,0xde,0xba,0xf1,0x37}};
struct ScriptErrorInfo {DWORD size;HRESULT result;ULONG line;LONG character;WCHAR file[260],component[260],description[260],sourceLine[260];};
static_assert(sizeof(ScriptErrorInfo)==2096 && offsetof(ScriptErrorInfo,file)==16);
struct Script: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Init(Performance*,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE CallRoutine(WCHAR*,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetVariableVariant(WCHAR*,VARIANT,BOOL,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetVariableVariant(WCHAR*,VARIANT*,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetVariableNumber(WCHAR*,LONG,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetVariableNumber(WCHAR*,LONG*,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetVariableObject(WCHAR*,IUnknown*,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetVariableObject(WCHAR*,REFIID,void**,ScriptErrorInfo*)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumRoutine(DWORD,WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE EnumVariable(DWORD,WCHAR*)=0;
};
}
