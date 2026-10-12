#include "midi_echo.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace producer::app {
bool operator==(const MidiEchoSettings& a,const MidiEchoSettings& b){return a.device==b.device&&a.enabled==b.enabled&&a.firstPchannel==b.firstPchannel;}
void validate_midi_echo(const MidiEchoSettings& settings){
    (void)encode_midi_input_selection(settings.device);
    if(settings.firstPchannel%16||settings.firstPchannel>0xffffffe0u)throw std::runtime_error("MIDI Echo requires an aligned block of 16 non-reserved PChannels");
}
Bytes encode_midi_echo(const MidiEchoSettings& settings){
    validate_midi_echo(settings);const auto identity=encode_midi_input_selection(settings.device);Bytes bytes(20);std::memcpy(bytes.data(),"MECH",4);put32(bytes,4,1);put32(bytes,8,settings.enabled?1:0);put32(bytes,12,settings.firstPchannel);put32(bytes,16,static_cast<DWORD>(identity.size()));bytes.insert(bytes.end(),identity.begin(),identity.end());return bytes;
}
MidiEchoSettings decode_midi_echo(const Bytes& bytes){
    if(bytes.size()<48||std::memcmp(bytes.data(),"MECH",4)||read32(bytes,4)!=1||read32(bytes,8)>1||read32(bytes,16)!=bytes.size()-20)throw std::runtime_error("Invalid MIDI Echo preferences");
    MidiEchoSettings settings{decode_midi_input_selection(Bytes(bytes.begin()+20,bytes.end())),read32(bytes,8)!=0,read32(bytes,12)};validate_midi_echo(settings);return settings;
}
MidiEchoSettings load_midi_echo(const std::filesystem::path& file){if(file.empty()||!std::filesystem::exists(file))return {};return decode_midi_echo(read_file(file.wstring()));}
void save_midi_echo(const std::filesystem::path& file,const MidiEchoSettings& settings){
    const auto bytes=encode_midi_echo(settings);if(file.empty())throw std::runtime_error("MIDI Echo preferences path is empty");if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path());write_file_atomic(file.wstring(),bytes);
}
std::optional<LiveMidi> route_midi_echo(const MidiInputEvent& event,DWORD first){
    validate_midi_echo({{},false,first});if(event.malformed)throw std::runtime_error("Malformed MIDI input was not echoed");
    const BYTE status=static_cast<BYTE>(event.message),data1=static_cast<BYTE>(event.message>>8),data2=static_cast<BYTE>(event.message>>16);
    if(status>=0xf0)return {};
    if(status<0x80||data1>127||(status<0xc0||status>=0xe0?data2>127:data2!=0)||(event.message&0xff000000u))throw std::runtime_error("Invalid MIDI channel-voice event");
    return LiveMidi{first+(status&15),static_cast<BYTE>(status&0xf0),data1,data2};
}
void MidiInputs::configure(const MidiInputSelection& input,const MidiEchoSettings& echo){
    validate_midi_echo(echo);(void)encode_midi_input_selection(input);
    std::vector<std::shared_ptr<MidiInput>> previous;
    if(input_)previous.push_back(input_);if(echo_&&echo_!=input_)previous.push_back(echo_);
    const auto obtain=[&](const MidiInputSelection& selected){
        if(!selected)return std::shared_ptr<MidiInput>{};
        for(const auto& owner:previous)if(owner->active()&&owner->selection()==selected)return owner;
        auto owner=std::make_shared<MidiInput>();owner->select(selected);return owner;
    };
    auto nextInput=obtain(input);auto nextEcho=echo.enabled&&echo.device?(nextInput&&nextInput->selection()==echo.device?nextInput:obtain(echo.device)):std::shared_ptr<MidiInput>{};
    // Every new device is open and started before any previous owner closes.
    // A close failure is reported; reopen any already-closed previous device.
    std::vector<MidiInputSelection> oldSelections;for(const auto& owner:previous)oldSelections.push_back(owner->selection());
    try{for(const auto& owner:previous)if(owner!=nextInput&&owner!=nextEcho)owner->close();}
    catch(...){const auto failure=std::current_exception();bool restored=true;for(size_t i=0;i<previous.size();++i)try{previous[i]->select(oldSelections[i]);}catch(...){restored=false;}if(!restored)throw std::runtime_error("MIDI input close failed and previous ownership restoration was incomplete");std::rethrow_exception(failure);}
    input_=std::move(nextInput);echo_=std::move(nextEcho);echoSettings_=echo;
}
MidiEchoSettings MidiInputs::echo_configuration()const{auto settings=echoSettings_;settings.device=echo_?echo_->selection():MidiInputSelection{};settings.enabled=echo_&&echo_->active();return settings;}
MidiInputsBatch MidiInputs::drain(){MidiInputsBatch result;if(input_)result.input=input_->drain();if(echo_==input_&&echo_)result.echo=result.input;else if(echo_)result.echo=echo_->drain();return result;}
void MidiInputs::close(){if(input_)input_->close();if(echo_&&echo_!=input_)echo_->close();input_.reset();echo_.reset();}
}
