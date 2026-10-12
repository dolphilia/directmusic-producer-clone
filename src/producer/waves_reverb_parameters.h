#pragma once
#include "riff.h"
#include <optional>
namespace producer::app {
struct WavesReverbParameters {
    float inputGain=0, reverbMix=0, reverbTime=1000, highFrequencyRatio=0.001f;
};
inline bool operator==(const WavesReverbParameters& a,const WavesReverbParameters& b){
    return a.inputGain==b.inputGain&&a.reverbMix==b.reverbMix&&a.reverbTime==b.reverbTime&&a.highFrequencyRatio==b.highFrequencyRatio;
}
inline bool operator!=(const WavesReverbParameters& a,const WavesReverbParameters& b){return !(a==b);}
bool valid_waves_reverb_parameters(const WavesReverbParameters&);
// DSFX/data is an opaque DMO stream, not a guessed parameter struct. Activate
// only the registered system dsdmo, and let its public persistence own the wire.
Bytes save_waves_reverb_parameters(const WavesReverbParameters&);
// Imported unsupported/noncanonical streams remain intact and uneditable.
std::optional<WavesReverbParameters> load_waves_reverb_parameters(const Bytes&);
}
