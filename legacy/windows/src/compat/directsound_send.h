#pragma once
#include <windows.h>
namespace producer::compat {
// Builtin DirectSound Send ABI: original AudioPathDesigner static identification
// and declared Microsoft x86 runtime observations. It is not a CoCreate class.
inline constexpr GUID directSoundSendClass={0xef602176,0xbcbb,0x49e0,{0x8c,0xca,0xe0,0x9a,0x5a,0x15,0x2b,0x33}};
inline constexpr GUID directSoundSendInterface={0xb30f3564,0x1698,0x45ba,{0x9f,0x75,0xfc,0x3c,0x6c,0x3b,0x28,0x10}};
struct DirectSoundSendParameters {LONG attenuation;};
struct DirectSoundSend: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const DirectSoundSendParameters*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(DirectSoundSendParameters*)=0;
};
static_assert(sizeof(DirectSoundSendParameters)==4);
}
