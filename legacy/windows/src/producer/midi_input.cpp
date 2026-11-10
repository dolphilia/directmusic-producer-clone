#include "midi_input.h"
#include <mmsystem.h>
#include <array>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace producer::app {
namespace {
void check(MMRESULT code,const char* operation){
    if(code!=MMSYSERR_NOERROR)throw std::runtime_error(std::string(operation)+" failed (WinMM "+std::to_string(code)+")");
}
void validate(const MidiInputSelection& selection){
    if(selection&&(selection->manufacturer>0xffff||selection->product>0xffff||selection->name.empty()||selection->name.size()>=MAXPNAMELEN||selection->name.find(L'\0')!=std::wstring::npos))throw std::runtime_error("Invalid MIDI input identity");
}
MidiInputKey key(const MIDIINCAPSW& caps){return {caps.wMid,caps.wPid,caps.vDriverVersion,caps.szPname};}
}
bool operator==(const MidiInputKey& a,const MidiInputKey& b){return a.manufacturer==b.manufacturer&&a.product==b.product&&a.driverVersion==b.driverVersion&&a.name==b.name;}
std::vector<MidiInputDevice> midi_input_devices(){
    std::vector<MidiInputDevice> devices;const auto count=midiInGetNumDevs();
    for(UINT index=0;index<count;++index){MIDIINCAPSW caps{};check(midiInGetDevCapsW(index,&caps,sizeof(caps)),"Enumerate MIDI input");devices.push_back({index,key(caps)});}return devices;
}
Bytes encode_midi_input_selection(const MidiInputSelection& selected){
    validate(selected);Bytes bytes(28+(selected?selected->name.size()*2:0));std::memcpy(bytes.data(),"MINP",4);put32(bytes,4,1);put32(bytes,8,selected?1:0);
    if(selected){put32(bytes,12,selected->manufacturer);put32(bytes,16,selected->product);put32(bytes,20,selected->driverVersion);put32(bytes,24,static_cast<DWORD>(selected->name.size()));std::memcpy(bytes.data()+28,selected->name.data(),selected->name.size()*2);}return bytes;
}
MidiInputSelection decode_midi_input_selection(const Bytes& bytes){
    if(bytes.size()<28||std::memcmp(bytes.data(),"MINP",4)||read32(bytes,4)!=1||read32(bytes,8)>1)throw std::runtime_error("Invalid MIDI input preferences");
    const auto count=read32(bytes,24);if(count>=MAXPNAMELEN||bytes.size()!=28+size_t(count)*2)throw std::runtime_error("Invalid MIDI input name length");
    if(!read32(bytes,8)){for(size_t i=12;i<bytes.size();++i)if(bytes[i])throw std::runtime_error("Invalid empty MIDI input identity");return {};}
    MidiInputKey selected{read32(bytes,12),read32(bytes,16),read32(bytes,20),std::wstring(count,L'\0')};if(count)std::memcpy(selected.name.data(),bytes.data()+28,count*2);validate(selected);return selected;
}
MidiInputSelection load_midi_input_selection(const std::filesystem::path& file){if(file.empty()||!std::filesystem::exists(file))return {};return decode_midi_input_selection(read_file(file.wstring()));}
void save_midi_input_selection(const std::filesystem::path& file,const MidiInputSelection& selected){
    const auto bytes=encode_midi_input_selection(selected);if(file.empty())throw std::runtime_error("MIDI input preferences path is empty");if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path());write_file_atomic(file.wstring(),bytes);
}
struct MidiInput::State {
    HMIDIIN handle=nullptr;MidiInputKey key;bool started=false;
    std::mutex mutex;std::array<MidiInputEvent,1024> queue{};size_t count=0;bool overflow=false,driverOverload=false;
    static void CALLBACK receive(HMIDIIN,UINT message,DWORD_PTR instance,DWORD_PTR data,DWORD_PTR time){
        if(message!=MIM_DATA&&message!=MIM_MOREDATA&&message!=MIM_ERROR)return;
        auto& self=*reinterpret_cast<State*>(instance);std::lock_guard<std::mutex> lock(self.mutex);
        if(message==MIM_MOREDATA)self.driverOverload=true;
        if(self.count==self.queue.size()){self.overflow=true;return;}
        self.queue[self.count++]={static_cast<DWORD>(data),static_cast<DWORD>(time),message==MIM_ERROR};
        // No multimedia, COM, document, or window calls on the driver thread.
    }
    MMRESULT close()noexcept{
        if(!handle)return MMSYSERR_NOERROR;
        midiInStop(handle);started=false;midiInReset(handle);
        const auto result=midiInClose(handle);if(result==MMSYSERR_NOERROR)handle=nullptr;return result;
    }
};
void MidiInput::Deleter::operator()(State* state)const noexcept{
    if(!state)return;
    // A driver that refuses Close may still call back. Retain its callback
    // storage until process teardown rather than free an in-use instance.
    if(state->close()==MMSYSERR_NOERROR)delete state;
}
MidiInput::MidiInput()=default;
MidiInput::~MidiInput()=default;
MidiInputSelection MidiInput::selection()const{return state_?MidiInputSelection(state_->key):MidiInputSelection{};}
bool MidiInput::active()const{return state_&&state_->started;}
void MidiInput::select(const MidiInputSelection& selected){
    validate(selected);if(!selected){close();return;}
    if(state_&&state_->key==*selected&&state_->started)return;
    const auto devices=midi_input_devices();std::optional<UINT> index;
    for(const auto& device:devices)if(device.key==*selected){if(index)throw std::runtime_error("MIDI input identity is ambiguous; no device was changed");index=device.index;}
    if(!index)throw std::runtime_error("Selected MIDI input is unavailable");
    std::unique_ptr<State,Deleter> staged(new State);staged->key=*selected;
    check(midiInOpen(&staged->handle,*index,reinterpret_cast<DWORD_PTR>(&State::receive),reinterpret_cast<DWORD_PTR>(staged.get()),CALLBACK_FUNCTION|MIDI_IO_STATUS),"Open MIDI input");
    MIDIINCAPSW caps{};check(midiInGetDevCapsW(reinterpret_cast<UINT_PTR>(staged->handle),&caps,sizeof(caps)),"Verify opened MIDI input");
    if(!(key(caps)==*selected))throw std::runtime_error("MIDI input devices changed while opening");
    check(midiInStart(staged->handle),"Start MIDI input");staged->started=true;
    close();state_=std::move(staged);
}
MidiInputBatch MidiInput::drain(){
    MidiInputBatch batch;if(!state_)return batch;
    std::lock_guard<std::mutex> lock(state_->mutex);batch.events.assign(state_->queue.begin(),state_->queue.begin()+state_->count);state_->count=0;
    batch.overflow=state_->overflow;batch.driverOverload=state_->driverOverload;state_->overflow=state_->driverOverload=false;return batch;
}
void MidiInput::close(){if(!state_)return;check(state_->close(),"Close MIDI input");state_.reset();}
}
