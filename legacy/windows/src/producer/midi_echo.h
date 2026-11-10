#pragma once
#include "midi_input.h"

namespace producer::app {
struct MidiEchoSettings {
    MidiInputSelection device;
    bool enabled=false;
    DWORD firstPchannel=0;
};
bool operator==(const MidiEchoSettings&,const MidiEchoSettings&);
inline bool operator!=(const MidiEchoSettings& a,const MidiEchoSettings& b){return !(a==b);}
void validate_midi_echo(const MidiEchoSettings&);
Bytes encode_midi_echo(const MidiEchoSettings&);
MidiEchoSettings decode_midi_echo(const Bytes&);
MidiEchoSettings load_midi_echo(const std::filesystem::path&);
void save_midi_echo(const std::filesystem::path&,const MidiEchoSettings&);
struct LiveMidi {DWORD pchannel;BYTE status,byte1,byte2;};
// System/common/real-time messages do not have a channel and are not echoed.
// A malformed channel-voice event is an error, never a masked MIDI value.
std::optional<LiveMidi> route_midi_echo(const MidiInputEvent&,DWORD firstPchannel);
struct MidiInputsBatch {MidiInputBatch input,echo;};
class MidiInputs {
    std::shared_ptr<MidiInput> input_,echo_;
    MidiEchoSettings echoSettings_;
public:
    void configure(const MidiInputSelection&,const MidiEchoSettings&);
    void select(const MidiInputSelection& input){configure(input,echoSettings_);}
    MidiInputSelection selection()const{return input_?input_->selection():MidiInputSelection{};}
    MidiEchoSettings echo_configuration()const;
    bool active()const{return input_&&input_->active();}
    bool echo_active()const{return echo_&&echo_->active();}
    bool shared()const{return input_&&input_==echo_;}
    MidiInputsBatch drain();
    void close();
};
}
