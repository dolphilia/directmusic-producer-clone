#pragma once
#include <mediaobj.h>
namespace producer::app {
inline constexpr GUID environmentalReverbBuffer={0x186cc542,0xdb29,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
inline constexpr GUID environmentalReverbRuntimeClass={0x93f6b062,0xe5bf,0x4b4f,{0xa2,0x37,0x58,0xd9,0x09,0xd8,0x4e,0x19}};
inline constexpr GUID environmentalReverbControlId={0x2cc3f44d,0x1898,0x4fba,{0xb2,0xa9,0x77,0x89,0xe8,0x20,0x3a,0xcf}};
struct EnvironmentalReverbParameters {
    LONG room,roomHF;float roomRolloffFactor,decayTime,decayHFRatio;
    LONG reflections;float reflectionsDelay;LONG reverb;float reverbDelay,diffusion,density,hfReference;
};
struct EnvironmentalReverbControl: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDefaults(EnvironmentalReverbParameters*)=0;
};
// Wet-only DMO bridge to declared Windows XAudio2 APO, not the legacy dsound3d DSP.
IMediaObject* create_environmental_reverb_dmo();
class EnvironmentalReverbRegistration {
    DWORD cookie_=0;
public:
    EnvironmentalReverbRegistration();~EnvironmentalReverbRegistration();
    EnvironmentalReverbRegistration(const EnvironmentalReverbRegistration&)=delete;
    EnvironmentalReverbRegistration& operator=(const EnvironmentalReverbRegistration&)=delete;
};
}
