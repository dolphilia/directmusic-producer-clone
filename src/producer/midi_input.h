#pragma once
#include "riff.h"
#include <windows.h>
#include <filesystem>
#include <memory>
#include <optional>

namespace producer::app {
// Device numbers are transient. Keep the advertised identity and resolve it
// again before opening; identical advertised identities are ambiguous.
struct MidiInputKey {
    DWORD manufacturer=0,product=0,driverVersion=0;
    std::wstring name;
};
bool operator==(const MidiInputKey&,const MidiInputKey&);
inline bool operator!=(const MidiInputKey& a,const MidiInputKey& b){return !(a==b);}
using MidiInputSelection=std::optional<MidiInputKey>;
struct MidiInputDevice { UINT index;MidiInputKey key; };
struct MidiInputEvent { DWORD message,timestampMs;bool malformed; };
struct MidiInputBatch { std::vector<MidiInputEvent> events;bool overflow=false,driverOverload=false; };
std::vector<MidiInputDevice> midi_input_devices();
Bytes encode_midi_input_selection(const MidiInputSelection&);
MidiInputSelection decode_midi_input_selection(const Bytes&);
MidiInputSelection load_midi_input_selection(const std::filesystem::path&);
void save_midi_input_selection(const std::filesystem::path&,const MidiInputSelection&);
class MidiInput {
    struct State;
    struct Deleter {void operator()(State*)const noexcept;};
    std::unique_ptr<State,Deleter> state_;
public:
    MidiInput();~MidiInput();
    MidiInput(const MidiInput&)=delete;
    MidiInput& operator=(const MidiInput&)=delete;
    // Stage and start a replacement before closing the previous input.
    // Enumeration/open/start failures preserve the previous input owner.
    void select(const MidiInputSelection&);
    MidiInputSelection selection()const;
    bool active()const;
    MidiInputBatch drain();
    void close();
};
}
