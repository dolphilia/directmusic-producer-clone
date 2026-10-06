#include "conductor.h"
#include "source_script_host.h"
#include "document.h"
#include "audio_path.h"
#include "file_output_dmo.h"
#include "command.h"
#include "compat/playback_runtime.h"
#include "compat/tempo_runtime.h"
#include "compat/producer_ids.h"
#include <objidl.h>
#include "compat/time_signature.h"
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cwctype>
#include <cstring>
#include <functional>
#include <array>
#include <limits>
#include <map>
#include <dmerror.h>

namespace producer::app {
namespace {
class NoteObserver final:public runtime::Tool {
    volatile LONG references_=1;
    mutable SRWLOCK lock_=SRWLOCK_INIT;
    std::array<PlaybackNote,4096> notes_{};
    size_t count_=0;bool overflow_=false;
    volatile LONG forwardingFailed_=0;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** result) override {
        if(!result)return E_POINTER;*result=nullptr;
        if(IsEqualGUID(id,IID_IUnknown)||IsEqualGUID(id,runtime::toolId)){*result=static_cast<runtime::Tool*>(this);AddRef();return S_OK;}return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return static_cast<ULONG>(InterlockedIncrement(&references_));}
    ULONG STDMETHODCALLTYPE Release() override {const auto n=InterlockedDecrement(&references_);if(!n)delete this;return static_cast<ULONG>(n);}
    HRESULT STDMETHODCALLTYPE Init(runtime::Graph*) override {return S_OK;} // no parent reference cycle
    HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* value) override {if(!value)return E_POINTER;*value=8;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* value) override {if(!value)return E_POINTER;*value=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** values,DWORD count) override {if(!values||!*values)return E_POINTER;if(count!=1)return E_INVALIDARG;(*values)[0]=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessPMsg(runtime::Performance*,runtime::Message* message) override {
        if(!message)return E_POINTER;
        if(message->flags&0x20)return DMUS_S_FREE; // flushed messages are not rendered notes
        if(message->type==1&&message->size>=sizeof(runtime::NoteMessage)){
            const auto& n=*reinterpret_cast<const runtime::NoteMessage*>(message);
            const PlaybackNote copy{message->musicTime,n.duration,message->pchannel,message->group,n.musicValue,n.midiValue,n.velocity,n.noteFlags,n.playMode};
            AcquireSRWLockExclusive(&lock_);if(count_<notes_.size())notes_[count_++]=copy;else overflow_=true;ReleaseSRWLockExclusive(&lock_);
        }
        // Stamp BEFORE returning to avoid routing the same message back to us.
        if(message->graph&&SUCCEEDED(message->graph->StampPMsg(message)))return DMUS_S_REQUEUE;
        InterlockedExchange(&forwardingFailed_,1);return DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(runtime::Performance*,runtime::Message* message,LONGLONG) override {return message?DMUS_S_FREE:S_OK;}
    std::vector<PlaybackNote> snapshot() const {std::vector<PlaybackNote> result;result.reserve(notes_.size());AcquireSRWLockShared(&lock_);result.assign(notes_.begin(),notes_.begin()+count_);ReleaseSRWLockShared(&lock_);return result;}
    bool overflow() const {AcquireSRWLockShared(&lock_);const bool result=overflow_;ReleaseSRWLockShared(&lock_);return result;}
    bool forwarding_failed() const {return InterlockedCompareExchange(const_cast<volatile LONG*>(&forwardingFailed_),0,0)!=0;}
};
template<class T> void release(T*& object) {if(object){object->Release();object=nullptr;}}
std::wstring os_server(REFGUID clsid,const wchar_t* expected) {
    wchar_t guid[40]{};if(!StringFromGUID2(clsid,guid,40))throw std::runtime_error("Runtime CLSID formatting failed");
    const auto key=std::wstring(L"CLSID\\")+guid+L"\\InprocServer32";
    wchar_t value[32768]{};DWORD bytes=sizeof(value),type=0;
    const auto error=RegGetValueW(HKEY_CLASSES_ROOT,key.c_str(),nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,&type,value,&bytes);
    if(error!=ERROR_SUCCESS)throw std::runtime_error("OS DirectMusic registration unavailable");
    wchar_t expanded[32768]{};const auto n=ExpandEnvironmentStringsW(value,expanded,32768);if(!n||n>32768)throw std::runtime_error("Runtime server path expansion failed");
    auto path=std::filesystem::path(expanded).lexically_normal();wchar_t system[MAX_PATH]{},wow[MAX_PATH]{};
    GetSystemDirectoryW(system,MAX_PATH);GetSystemWow64DirectoryW(wow,MAX_PATH);
    auto lower=[](std::wstring s){std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return s;};
    if(lower(path.filename().wstring())!=lower(expected)||(lower(path.parent_path().wstring())!=lower(system)&&lower(path.parent_path().wstring())!=lower(wow)))throw std::runtime_error("DirectMusic COM server is outside the declared Windows system runtime");
    return path.wstring();
}
}
struct PlaybackSession {
    OwnedRuntimeGraph audioPathGraph{nullptr,[](runtime::Graph* p){if(p)p->Release();}};
    OwnedRuntimeGraph toolGraph{nullptr,[](runtime::Graph* p){if(p)p->Release();}};
    runtime::Loader* loader=nullptr;
    runtime::Segment* segment=nullptr;runtime::SegmentState* playing=nullptr;
    runtime::AudioPath* audioPath=nullptr;
    Bytes memory,loaderMemory;bool downloaded=false,motif=false,messageWindowEligible=false;
    std::vector<ResolvedStyle> styles;
    std::vector<runtime::Style*> runtimeStyles;
    std::vector<ResolvedChordMap> maps;
    std::vector<runtime::ChordMap*> runtimeMaps;
    std::vector<TriggeredScriptSnapshot> scripts;
    std::vector<runtime::Script*> runtimeScripts;
    std::vector<runtime::Script*> sourceScripts;
    std::vector<Bytes> scriptLoaderMemory;
    std::unique_ptr<NativeScriptTrackRuntime> nativeScripts;
    std::vector<Bytes> triggeredLoaderMemory;
    std::vector<ResolvedCollection> collections;
    std::vector<runtime::Collection*> runtimeCollections;
    std::vector<ResolvedWave> waves;
    std::vector<IUnknown*> runtimeWaves;
    std::vector<TriggeredSegmentSnapshot> triggeredSnapshots;
    std::vector<runtime::Segment*> triggeredSegments;
    size_t triggeredDownloads=0;
    std::vector<DWORD> firstPatches;
    PlaybackRequest request;PlaybackId id=0;std::wstring name;
};
// Validate before the loader can instantiate serialized effects. A document
// may retain unknown effects, but audition never falls back to Producer COM.
static void validate_audio_effects(const AudioPathDocument& path){
    static const wchar_t* classes[]={L"{DAFD8210-5711-4B91-9FE3-F75B7AE279BF}",L"{EFE6629C-81F7-4281-BD91-C9D604A95AF6}",L"{EFCA3D92-DFD8-4672-A603-7420894BAD98}",L"{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}",L"{EF114C90-CD1D-484E-96E5-09CFAF912A21}",L"{EF011F79-4000-406D-87AF-BFFB3FC39D57}",L"{120CED89-3BF4-4173-A132-3CB406CF3231}",L"{EF985E71-D5C7-42D4-BA4D-2D073E2E96F4}"};
    for(const auto& effect:path.effects()){
        GUID id{};std::memcpy(&id,effect.classId.data(),16);
        if(IsEqualGUID(id,fileOutputClass)){
            const auto root=Chunk::parse(path.save_bytes());size_t b=0;
            for(const auto& item:root.children)if(item.id=="LIST"&&item.type=="dbfl"&&b++==effect.buffer){
                const auto ds=item.find("RIFF","DSBC"),attr=item.find("ddah");
                if(read32(attr->data,16)&2)throw std::runtime_error("FileOutput requires a custom buffer");
                const auto fx=ds->find("LIST","fxls");size_t n=0;
                for(const auto& c:fx->children)if(c.id=="RIFF"&&c.type=="DSFX"&&n++==effect.index){const auto h=c.find("fxhr");
                    if(effect.flags||c.find("data")||std::any_of(h->data.begin()+36,h->data.begin()+52,[](auto x){return x!=0;}))throw std::runtime_error("FileOutput effect options are not yet supported");
                }
            }continue;
        }
        bool allowed=false;for(const auto text:classes){GUID expected{};if(SUCCEEDED(CLSIDFromString(text,&expected))&&IsEqualGUID(expected,id)){allowed=true;break;}}
        if(!allowed)throw std::runtime_error("AudioPath effect needs an explicitly declared source or OS factory; original fallback refused");
        (void)os_server(id,L"dsdmo.dll");
    }
}
static void prepare_source_effects(Chunk& path){
    for(auto& buffer:path.children)if(buffer.id=="LIST"&&buffer.type=="dbfl")if(auto descriptor=buffer.find("RIFF","DSBC"))if(auto effects=descriptor->find("LIST","fxls"))
        for(auto& effect:effects->children)if(effect.id=="RIFF"&&effect.type=="DSFX")if(auto header=effect.find("fxhr")){
            if(header->data.size()<56)throw std::runtime_error("Truncated runtime effect header");GUID id{};std::memcpy(&id,header->data.data()+4,16);
            if(IsEqualGUID(id,fileOutputClass))std::memcpy(header->data.data()+4,&fileOutputRuntimeClass,16);
        }
}
struct Conductor::State:PlaybackSession {
    std::unique_ptr<ScriptSession> script;
    std::vector<ScriptDiagnostic> scriptDiagnosticHistory;bool scriptDiagnosticHistoryOverflow=false;
    std::vector<NativeScriptCall> scriptHistory;bool scriptHistoryOverflow=false;
    std::unique_ptr<FileOutputRegistration> fileOutputRegistration;
    runtime::AudioPath* recordingPath=nullptr;
    FileOutputControl* recordingControl=nullptr;
    Bytes recordingConfig;
    runtime::Performance* performance=nullptr;bool com=false,audio=false;
    bool observeNotes=false,observeLyrics=false,observeScriptMessages=false;runtime::Graph* graph=nullptr;NoteObserver* observer=nullptr;LyricObserver* lyricObserver=nullptr;LyricObserver* scriptMessageObserver=nullptr;
    std::vector<std::unique_ptr<PlaybackSession>> retained;
    // Keys are compared as addresses, never dereferenced or AddRef'd. A queued
    // notification owns its punkUser, so that identity cannot be reused while
    // an old message still exists. New instances overwrite reused addresses.
    std::map<IUnknown*,PlaybackId> notificationIds;
    std::vector<PlaybackNotification> pendingNotifications;
    PlaybackId nextId=1;
};
Conductor::Conductor():state_(std::make_unique<State>()){}
Conductor::~Conductor(){shutdown();}
void Conductor::check(const char* operation,HRESULT result) {
    calls_.push_back({operation,result});if(FAILED(result)){std::ostringstream message;message<<operation<<" failed: 0x"<<std::hex<<static_cast<unsigned long>(result);throw std::runtime_error(message.str());}
}
void Conductor::initialize_runtime(HWND owner){auto& s=*state_;
        if(!s.com){check("CoInitializeEx",CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED));s.com=true;}
        if(!s.fileOutputRegistration)s.fileOutputRegistration=std::make_unique<FileOutputRegistration>();
        if(!s.performance){
            servers_={os_server(runtime::performanceClass,L"dmime.dll"),os_server(runtime::loaderClass,L"dmloader.dll")};
            check("CoCreate Performance8",CoCreateInstance(runtime::performanceClass,nullptr,CLSCTX_INPROC_SERVER,runtime::performance8Id,reinterpret_cast<void**>(&s.performance)));
            check("InitAudio",s.performance->InitAudio(nullptr,nullptr,owner,8,16,0,nullptr));s.audio=true;
            check("Add Command notifications",s.performance->AddNotificationType(runtime::commandNotification));
            check("Add Segment notifications",s.performance->AddNotificationType(runtime::segmentNotification));
            if(s.observeNotes||s.observeLyrics||s.observeScriptMessages){
                servers_.push_back(os_server(runtime::graphClass,L"dmime.dll"));
                check("CoCreate note graph",CoCreateInstance(runtime::graphClass,nullptr,CLSCTX_INPROC_SERVER,runtime::graphId,reinterpret_cast<void**>(&s.graph)));
                if(s.observeNotes){s.observer=new NoteObserver;check("Insert note observer",s.graph->InsertTool(s.observer,nullptr,0,0));}
                if(s.observeLyrics){s.lyricObserver=new LyricObserver;check("Insert Lyric observer",s.graph->InsertTool(s.lyricObserver,nullptr,0,-1));}
                if(s.observeScriptMessages){s.scriptMessageObserver=new LyricObserver(14,4,true);check("Insert Script Trace observer",s.graph->InsertTool(s.scriptMessageObserver,nullptr,0,-1));}
                check("Set note graph",s.performance->SetGraph(s.graph));
            }
        }
}
std::vector<NativeScriptCall> Conductor::track_script_calls(PlaybackId id)const{
    auto calls=state_->scriptHistory;const auto append=[&](const PlaybackSession& session){if(session.nativeScripts)for(auto call:session.nativeScripts->calls()){call.playbackId=session.id;call.messageVisible=session.messageWindowEligible;calls.push_back(call);}};append(*state_);for(const auto& retained:state_->retained)append(*retained);if(id)calls.erase(std::remove_if(calls.begin(),calls.end(),[&](const auto& call){return call.playbackId!=id;}),calls.end());return calls;
}
bool Conductor::track_script_observation_overflow()const{if(state_->scriptHistoryOverflow)return true;if(state_->nativeScripts&&state_->nativeScripts->overflow())return true;for(const auto& session:state_->retained)if(session->nativeScripts&&session->nativeScripts->overflow())return true;return false;}
ScriptResult Conductor::track_script_number(PlaybackId id,const std::array<std::uint8_t,16>& objectId,const std::wstring& name,LONG& value){
    if(name.empty()||name.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Invalid Track Script variable name");
    PlaybackSession* session=nullptr;if(state_->id==id&&id)session=state_.get();else for(auto& retained:state_->retained)if(retained->id==id&&id)session=retained.get();
    if(!session)throw std::runtime_error("Track Script playback session unavailable");size_t index=0;while(index<session->scripts.size()&&session->scripts[index].objectId!=objectId)++index;if(index>=session->sourceScripts.size())throw std::runtime_error("Track Script identity unavailable");
    ScriptResult result;result.error.size=sizeof(result.error);auto variable=name;LONG pending=0;result.result=session->sourceScripts[index]->GetVariableNumber(variable.data(),&pending,&result.error);if(result.passed())value=pending;return result;
}
ScriptSession& Conductor::script_session(){if(!state_->script)throw std::runtime_error("No initialized Script session");return *state_->script;}
ScriptResult Conductor::load_script(const Bytes& bytes,const std::wstring& directory,HWND owner){
    ScriptDocument document;document.load(bytes);
    // Resolve references into a private source snapshot before any activation.
    // Serialized documents and their original reference chunks remain untouched.
    auto runtimeRoot=Chunk::parse(bytes);auto container=runtimeRoot.find("RIFF","DMCN");
    std::vector<std::wstring> containedServers{os_server(runtime::containerClass,L"dmloader.dll")};
    const auto objects=document.container().objects();size_t objectIndex=0;
    for(auto& entry:container->find("LIST","cosl")->children)if(entry.id=="LIST"&&entry.type=="cobl"){
        const auto& object=objects.at(objectIndex++);GUID classId{};std::memcpy(&classId,object.classId.data(),16);
        // The SDK cobl grammar places its optional alias before the header.
        // Windows binds the alias while reading cobh; tolerate source readers'
        // retained late aliases by normalizing only this private runtime copy.
        const auto alias=std::find_if(entry.children.begin(),entry.children.end(),[](const Chunk& c){return c.id=="coba";});
        if(alias!=entry.children.end())std::rotate(entry.children.begin(),alias,alias+1);
        if(!IsEqualGUID(classId,runtime::segmentClass))throw std::runtime_error("Container runtime class routing incomplete; document retained");
        containedServers.push_back(os_server(classId,L"dmime.dll"));auto payload=object.payload;
        if(object.reference){
            const auto header=payload.find("refh"),file=payload.find("file");
            if(!(read32(header->data,16)&16)||!file)throw std::runtime_error("Container reference needs explicit source file resolution");
            const auto name=decode_utf16(file->data);if(name.empty())throw std::runtime_error("Container reference file is empty");
            auto path=std::filesystem::path(name);if(path.is_relative()){if(directory.empty())throw std::runtime_error("Container reference directory unavailable");path=std::filesystem::path(directory)/path;}
            payload=Chunk::parse(read_file(path.lexically_normal().wstring()));
            if(const auto expected=object.payload.find("guid")){const auto actual=payload.find("guid");if(!actual||actual->data!=expected->data)throw std::runtime_error("Container reference GUID mismatch");}
        }
        if(payload.id!="RIFF"||payload.type!="DMSG")throw std::runtime_error("Container Segment payload type mismatch");
        SegmentDocument segment;segment.load(payload.encode());
        std::function<void(const Chunk&)> validate=[&](const Chunk& node){
            if((node.id=="LIST"&&node.type=="DMRF")||(node.id=="RIFF"&&(node.type=="DMTG"||node.type=="DMAP"||node.type=="DMCN")))throw std::runtime_error("Nested Container runtime dependency resolver incomplete");
            if(node.id=="RIFF"&&node.type=="DMBD")containedServers.push_back(os_server(runtime::bandRuntimeClass,L"dmband.dll"));
            if(node.id=="trkh"){if(node.data.size()<16)throw std::runtime_error("Container Segment track header truncated");GUID track{};std::memcpy(&track,node.data.data(),16);containedServers.push_back(os_server(track,IsEqualGUID(track,runtime::bandTrackRuntimeClass)?L"dmband.dll":L"dmime.dll"));}
            for(const auto& child:node.children)validate(child);
        };validate(payload);
        if(object.reference){
            auto h=entry.find("cobh");std::copy_n("RIFF",4,h->data.begin()+20);std::copy_n("DMSG",4,h->data.begin()+24);
            for(auto& child:entry.children)if(child.id=="LIST"&&child.type=="DMRF"){child=std::move(payload);break;}
        }
    }
    auto scriptServer=os_server(runtime::scriptClass,L"dmscript.dll");
    auto language=document.language();std::transform(language.begin(),language.end(),language.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});
    const wchar_t* engineName=language==L"vbscript"?L"vbscript.dll":language==L"jscript"?L"jscript.dll":nullptr;
    if(!engineName)throw std::runtime_error("Script language runtime declaration incomplete; document retained");
    wchar_t engineClass[80]{};DWORD engineBytes=sizeof(engineClass);const auto engineKey=document.language()+L"\\CLSID";
    if(RegGetValueW(HKEY_CLASSES_ROOT,engineKey.c_str(),nullptr,RRF_RT_REG_SZ,nullptr,engineClass,&engineBytes)!=ERROR_SUCCESS)throw std::runtime_error("Declared Script language registration unavailable");
    GUID engineId{};if(FAILED(CLSIDFromString(engineClass,&engineId)))throw std::runtime_error("Script language registration CLSID invalid");
    auto engineServer=os_server(engineId,engineName);
    try{initialize_runtime(owner);}catch(...){shutdown();throw;}
    servers_.push_back(scriptServer);servers_.push_back(engineServer);
    servers_.insert(servers_.end(),containedServers.begin(),containedServers.end());
    auto candidate=std::make_unique<ScriptSession>();const auto result=candidate->load(runtimeRoot.encode(),directory,state_->performance);
    auto archive=[&](const ScriptSession& old){for(const auto& d:old.diagnostics()){if(state_->scriptDiagnosticHistory.size()<4096)state_->scriptDiagnosticHistory.push_back(d);else state_->scriptDiagnosticHistoryOverflow=true;}state_->scriptDiagnosticHistoryOverflow|=old.diagnostic_overflow();};
    if(result.passed()){if(state_->script)archive(*state_->script);state_->script.swap(candidate);}else archive(*candidate);return result;
}
std::vector<ScriptDiagnostic> Conductor::script_diagnostics()const{auto result=state_->scriptDiagnosticHistory;if(state_->script)result.insert(result.end(),state_->script->diagnostics().begin(),state_->script->diagnostics().end());std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.sequence<b.sequence;});return result;}
bool Conductor::script_diagnostic_overflow()const{return state_->scriptDiagnosticHistoryOverflow||(state_->script&&state_->script->diagnostic_overflow());}
bool Conductor::initialized() const{return state_->audio;}
PlaybackRequest Conductor::playback_request() const{return state_->request;}
PlaybackId Conductor::current_playback_id() const{return state_->id;}
std::vector<PlaybackId> Conductor::playback_ids() const {
    std::vector<PlaybackId> ids;if(state_->id)ids.push_back(state_->id);
    for(const auto& session:state_->retained)if(session->id)ids.push_back(session->id);return ids;
}
std::vector<PlaybackSessionView> Conductor::playback_sessions(){
    auto& s=*state_;std::vector<PlaybackSessionView> result;
    for(const auto id:playback_ids()){
        const PlaybackSession* session=&s;
        if(id!=s.id)for(const auto& retained:s.retained)if(retained->id==id){session=retained.get();break;}
        result.push_back({id,session->name,(session->request.flags&runtime::playSecondary)!=0,position(id)});
    }
    return result;
}
void Conductor::stop(PlaybackId id){
    auto& s=*state_;if(!id)throw std::runtime_error("Invalid playback ID");
    if(s.id==id){stop_current();if(!s.retained.empty()){std::swap(static_cast<PlaybackSession&>(s),*s.retained.back());s.retained.pop_back();}return;}
    const auto found=std::find_if(s.retained.begin(),s.retained.end(),[&](const auto& p){return p->id==id;});
    if(found==s.retained.end())throw std::runtime_error("Unknown playback ID");
    std::swap(static_cast<PlaybackSession&>(s),**found);
    try{stop_current();}catch(...){std::swap(static_cast<PlaybackSession&>(s),**found);throw;}
    std::swap(static_cast<PlaybackSession&>(s),**found);s.retained.erase(found);
}
PlaybackPosition Conductor::position(PlaybackId id){
    auto& s=*state_;if(s.id==id&&id)return position();
    const auto found=std::find_if(s.retained.begin(),s.retained.end(),[&](const auto& p){return p->id==id;});
    if(found==s.retained.end())throw std::runtime_error("Unknown playback ID");
    std::swap(static_cast<PlaybackSession&>(s),**found);
    try{auto result=position();std::swap(static_cast<PlaybackSession&>(s),**found);return result;}
    catch(...){std::swap(static_cast<PlaybackSession&>(s),**found);throw;}
}
const Bytes& Conductor::playback_bytes() const{return state_->memory;}
const std::vector<ResolvedStyle>& Conductor::playback_styles() const{return state_->styles;}
const std::vector<ResolvedCollection>& Conductor::playback_collections() const{return state_->collections;}
const std::vector<ResolvedWave>& Conductor::playback_waves() const{return state_->waves;}
const std::vector<DWORD>& Conductor::playback_collection_first_patches() const{return state_->firstPatches;}
void Conductor::enable_lyric_observation(){if(state_->performance)throw std::runtime_error("Enable Lyric observation before initialization");state_->observeLyrics=true;}
std::vector<PlaybackLyric> Conductor::observed_lyrics()const{return state_->lyricObserver?state_->lyricObserver->snapshot():std::vector<PlaybackLyric>{};}
bool Conductor::lyric_observation_failed()const{return state_->lyricObserver&&state_->lyricObserver->failed();}
void Conductor::enable_script_message_observation(){if(state_->performance)throw std::runtime_error("Enable Script message observation before initialization");state_->observeScriptMessages=true;}
std::vector<PlaybackLyric> Conductor::observed_script_messages()const{return state_->scriptMessageObserver?state_->scriptMessageObserver->snapshot():std::vector<PlaybackLyric>{};}
bool Conductor::script_message_observation_failed()const{return state_->scriptMessageObserver&&state_->scriptMessageObserver->failed();}
void Conductor::enable_note_observation(){if(state_->performance)throw std::runtime_error("Enable note observation before initialization");state_->observeNotes=true;}
std::vector<PlaybackNote> Conductor::observed_notes() const{return state_->observer?state_->observer->snapshot():std::vector<PlaybackNote>{};}
bool Conductor::note_observation_overflow() const{return state_->observer&&state_->observer->overflow();}
bool Conductor::note_observation_forwarding_failed() const{return state_->observer&&state_->observer->forwarding_failed();}
void Conductor::set_default_audio_path(const Bytes& bytes){
    Bytes validated=bytes;if(!validated.empty()){AudioPathDocument config;config.load(validated);}
    defaultAudioPath_.swap(validated);
}
bool Conductor::file_output_active() const{return state_->recordingControl!=nullptr;}
void Conductor::start_file_output(const Bytes& bytes,const std::wstring& filename,HWND owner){
    auto& s=*state_;if(s.recordingPath)throw std::runtime_error("FileOutput recording already active");
    if(!playback_ids().empty())throw std::runtime_error("Stop playback before starting buffer recording");
    AudioPathDocument path;path.load(bytes);validate_audio_effects(path);
    if(!path.tool_graph().empty())throw std::runtime_error("Recording AudioPath ToolGraph construction is not yet connected; implicit activation refused");
    const auto effects=path.effects();
    const AudioPathEffect* selected=nullptr;
    for(const auto& effect:effects){GUID id{};std::memcpy(&id,effect.classId.data(),16);if(IsEqualGUID(id,fileOutputClass)){
        if(selected)throw std::runtime_error("Multi-buffer FileOutput recording is not yet connected");selected=&effect;
    }}
    if(!selected)throw std::runtime_error("Add FileOutput to an AudioPath buffer before recording");
    DWORD pchannel=0,bufferIndex=0;bool connected=false;const auto buffer=path.buffers().at(selected->buffer);
    for(const auto& port:path.ports())for(const auto& route:port.routes)for(size_t i=0;i<route.buffers.size();++i)if(!connected&&route.buffers[i]==buffer){pchannel=route.base;bufferIndex=static_cast<DWORD>(i);connected=true;}
    if(!connected)throw std::runtime_error("FileOutput buffer is not connected to a PChannel route");
    initialize_runtime(owner);
    runtime::Loader* loader=nullptr;runtime::Segment* carrier=nullptr;IUnknown* config=nullptr;
    runtime::AudioPath* runtimePath=nullptr;FileOutputControl* control=nullptr;
    auto privatePath=Chunk::parse(bytes);prepare_source_effects(privatePath);
    const auto carrierBytes=prepare_transport_audio_path({},privatePath.encode());
    try{
        check("Create FileOutput config loader",CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&loader)));
        runtime::ObjectDesc desc{};desc.size=sizeof(desc);desc.valid=2|1024;desc.classId=runtime::segmentClass;desc.memoryLength=static_cast<LONGLONG>(carrierBytes.size());desc.memory=const_cast<BYTE*>(carrierBytes.data());
        const auto loaded=loader->GetObject(&desc,runtime::segment8Id,reinterpret_cast<void**>(&carrier));check("Load FileOutput configuration carrier",loaded);
        if(loaded!=S_OK||!carrier)throw std::runtime_error("Incomplete FileOutput configuration carrier");
        check("Get FileOutput AudioPath config",carrier->GetAudioPathConfig(&config));if(!config)throw std::runtime_error("No FileOutput config");
        const auto created=s.performance->CreateAudioPath(config,TRUE,&runtimePath);check("Create FileOutput AudioPath",created);
        if(created!=S_OK||!runtimePath)throw std::runtime_error("Incomplete FileOutput buffer creation");
        check("Get source buffer FileOutput control",runtimePath->GetObjectInPath(pchannel,0x6100,bufferIndex,fileOutputRuntimeClass,0,fileOutputControlId,reinterpret_cast<void**>(&control)));
        if(!control)throw std::runtime_error("No source FileOutput control");
        check("Set FileOutput filename",control->SetFilename(filename.c_str()));check("Start buffer FileOutput",control->Start());
        // Copy before transferring COM ownership: allocation failure must stop
        // and finalize the newly created output through the same error path.
        s.recordingConfig=bytes;s.recordingPath=runtimePath;runtimePath=nullptr;s.recordingControl=control;control=nullptr;
        release(config);release(carrier);release(loader);
    }catch(...){if(control)control->Stop();release(control);release(runtimePath);release(config);release(carrier);release(loader);throw;}
}
void Conductor::stop_file_output(){auto& s=*state_;if(!s.recordingControl)return;
    const auto result=s.recordingControl->Stop();release(s.recordingControl);release(s.recordingPath);s.recordingConfig.clear();check("Stop and finalize buffer FileOutput",result);
}
void Conductor::set_tool_factories(std::vector<ToolFactory> factories){
    if(state_->performance)throw std::runtime_error("Declare Tool factories before initialization");
    for(size_t i=0;i<factories.size();++i){if(!factories[i].create)throw std::runtime_error("Missing Tool factory");for(size_t j=0;j<i;++j)if(factories[i].classId==factories[j].classId)throw std::runtime_error("Duplicate Tool factory");}
    toolFactories_=std::move(factories);
}
void Conductor::play(const Bytes& bytes,const std::wstring& directory,HWND owner,const std::vector<ResolvedStyle>& styles,const std::vector<ResolvedCollection>& collections,const std::optional<MotifSelection>& motif,const std::vector<ResolvedWave>& waves,const SegmentTriggerPlayback& triggers,const std::vector<ResolvedChordMap>& maps) {
    // Validate the caller's serialized document before disturbing playback.
    if(!triggers.source.empty()&&triggers.source!=bytes)throw std::runtime_error("Trigger playback graph does not match current source");
    if(triggers.source.empty()){const auto root=Chunk::parse(bytes);if(const auto tracks=root.find("LIST","trkl"))for(const auto& track:tracks->children)if(track.find("LIST","segt"))for(const auto& e:segment_triggers(track))if(e.hasId||!e.filename.empty()||!e.name.empty())throw std::runtime_error("Resolve owned Segment Trigger graph before playback");}
    if(triggers.source.empty()){const auto root=Chunk::parse(bytes);if(const auto tracks=root.find("LIST","trkl"))for(const auto& track:tracks->children)if(track.find("LIST","scrt"))for(const auto& e:script_events(track))if(e.hasId||!e.filename.empty()||!e.name.empty())throw std::runtime_error("Resolve owned Script Track graph before playback");}
    auto snapshot=prepare_style_playback(triggers.source.empty()?bytes:triggers.parent,styles); // Validate and allocate before Stop.
    auto mapSnapshot=prepare_chordmap_playback(snapshot.segment,maps);snapshot.segment=std::move(mapSnapshot.segment);
    snapshot.segment=prepare_command_playback(snapshot.segment);
    snapshot.segment=prepare_transport_audio_path(snapshot.segment,defaultAudioPath_);
    play_snapshot(std::move(snapshot),directory,owner,collections,motif,{},waves,triggers,mapSnapshot.maps);
}
void Conductor::play_motif(const StyleCatalogEntry& owned,const std::wstring& name,HWND owner,const std::vector<ResolvedCollection>& collections,const PlaybackOptions& options,const Bytes& audioPath){
    StyleDocument source;source.load(owned.bytes);StyleReference reference{};reference.groups=1;reference.hasId=true;
    Bytes bytes=owned.bytes;if(source.has_object_id())reference.objectId=source.object_id();else{
        GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Playback Style GUID creation failed");std::memcpy(reference.objectId.data(),&id,16);
        auto root=Chunk::parse(bytes);Chunk identity;identity.id="guid";identity.data.assign(reference.objectId.begin(),reference.objectId.end());root.children.push_back(std::move(identity));bytes=root.encode();
    }
    Bytes context=prepare_transport_audio_path({},audioPath.empty()?defaultAudioPath_:audioPath);
    StylePlaybackSnapshot snapshot{std::move(context),{{reference,owned.path,std::move(bytes),source.meter()}}};
    play_snapshot(std::move(snapshot),L"",owner,collections,MotifSelection{0,name},options);
}
void Conductor::play_snapshot(StylePlaybackSnapshot snapshot,const std::wstring& directory,HWND owner,const std::vector<ResolvedCollection>& collections,const std::optional<MotifSelection>& motif,const PlaybackOptions& options,const std::vector<ResolvedWave>& waves,const SegmentTriggerPlayback& triggers,const std::vector<ResolvedChordMap>& maps){
    if(options.delayClocks<0)throw std::runtime_error("Playback delay cannot be negative");
    DWORD flags=options.afterPrepareTime?runtime::playAfterPrepareTime:0;
    switch(options.boundary){
    case PlaybackBoundary::Immediate:break;
    case PlaybackBoundary::Stored:flags|=runtime::playDefault;break;
    case PlaybackBoundary::Grid:flags|=runtime::playGrid;break;
    case PlaybackBoundary::Beat:flags|=runtime::playBeat;break;
    case PlaybackBoundary::Measure:flags|=runtime::playMeasure;break;
    default:throw std::runtime_error("Invalid playback boundary");
    }
    if(options.secondary)flags|=runtime::playSecondary;
    if(motif){
        if(motif->name.empty()||motif->name.find(L'\0')!=std::wstring::npos||motif->styleIndex>=snapshot.styles.size())throw std::runtime_error("Invalid owned Motif selection");
        StyleDocument selected;selected.load(snapshot.styles[motif->styleIndex].bytes);size_t matches=0;
        const auto patterns=selected.patterns();for(size_t i=0;i<patterns.size();++i)if((patterns[i].embellishment&16)&&patterns[i].name==motif->name){++matches;selected.motif_settings(i);}
        if(matches!=1)throw std::runtime_error("Owned Motif name must select exactly one Pattern");
    }
    const bool hasSegment=!snapshot.segment.empty();std::vector<Bytes> documents;if(hasSegment)documents.push_back(snapshot.segment);for(const auto& style:snapshot.styles)documents.push_back(style.bytes);auto collectionSnapshot=prepare_collection_playback(documents,collections);if(hasSegment)snapshot.segment=std::move(collectionSnapshot.documents[0]);for(size_t i=0;i<snapshot.styles.size();++i)snapshot.styles[i].bytes=std::move(collectionSnapshot.documents[i+(hasSegment?1:0)]);
    auto waveSnapshot=prepare_wave_playback(snapshot.segment,waves);snapshot.segment=std::move(waveSnapshot.segment);
    const auto graphBytes=segment_tool_graph(snapshot.segment);
    if(!graphBytes.empty())validate_tool_factories(graphBytes,toolFactories_);
    Bytes pathGraphBytes,pathConfig;
    if(!snapshot.segment.empty()){auto root=Chunk::parse(snapshot.segment);
        if(auto path=root.find("RIFF","DMAP")){AudioPathDocument owned;owned.load(path->encode());validate_audio_effects(owned);pathConfig=owned.save_bytes();pathGraphBytes=owned.tool_graph();prepare_source_effects(*path);
            if(!pathGraphBytes.empty())validate_tool_factories(pathGraphBytes,toolFactories_);
            // Only the private loader snapshot omits tool classes. Populate the
            // AudioPath's own graph via explicit source factories after creation.
            path->children.erase(std::remove_if(path->children.begin(),path->children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMTG";}),path->children.end());}
        root.children.erase(std::remove_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMTG";}),root.children.end());snapshot.segment=root.encode();}
    std::vector<std::wstring> triggerServers;
    if(hasSegment){const auto root=Chunk::parse(snapshot.segment);if(const auto tracks=root.find("LIST","trkl"))for(const auto& track:tracks->children){const auto h=track.find("trkh");if(h&&h->data.size()>=16){GUID id{};std::memcpy(&id,h->data.data(),16);if(IsEqualGUID(id,runtime::chordMapTrackClass))triggerServers.push_back(os_server(id,L"dmcompos.dll"));}}}
    const GUID scriptTrackClass={0x4108fa85,0x3586,0x11d3,{0x8b,0xd7,0,0x60,8,0x93,0xb1,0xb6}};
    for(const auto& script:triggers.scripts){
        ScriptDocument document;document.load(script.bytes);if(!document.source())throw std::runtime_error("Script Track external text source needs owned resolver");if(!document.container().objects().empty())throw std::runtime_error("Script Track Container runtime routing incomplete");
        const auto root=Chunk::parse(script.bytes);const auto id=root.find("guid");if(!id||id->data!=Bytes(script.objectId.begin(),script.objectId.end()))throw std::runtime_error("Script Track snapshot identity mismatch");
        triggerServers.push_back(os_server(runtime::scriptClass,L"dmscript.dll"));triggerServers.push_back(os_server(runtime::graphClass,L"dmime.dll"));triggerServers.push_back(os_server(runtime::containerClass,L"dmloader.dll"));
        auto language=document.language();std::transform(language.begin(),language.end(),language.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});const wchar_t* engine=language==L"vbscript"?L"vbscript.dll":language==L"jscript"?L"jscript.dll":nullptr;if(!engine)throw std::runtime_error("Script Track language runtime declaration incomplete");
        wchar_t cls[80]{};DWORD bytes=sizeof(cls);const auto key=document.language()+L"\\CLSID";if(RegGetValueW(HKEY_CLASSES_ROOT,key.c_str(),nullptr,RRF_RT_REG_SZ,nullptr,cls,&bytes)!=ERROR_SUCCESS)throw std::runtime_error("Script Track language registration unavailable");GUID engineId{};if(FAILED(CLSIDFromString(cls,&engineId)))throw std::runtime_error("Script Track engine CLSID invalid");triggerServers.push_back(os_server(engineId,engine));
    }
    for(const auto& child:triggers.segments){
        SegmentDocument valid;valid.load(child.bytes);const auto root=Chunk::parse(child.bytes);
        if(root.find("RIFF","DMAP")||root.find("RIFF","DMTG"))throw std::runtime_error("Triggered child AudioPath/ToolGraph runtime integration remains pending");
        std::function<void(const Chunk&)> audit=[&](const Chunk& node){
            if(node.id=="trkh"){if(node.data.size()<16)throw std::runtime_error("Triggered track header truncated");GUID id{};std::memcpy(&id,node.data.data(),16);if(!IsEqualGUID(id,scriptTrackClass))triggerServers.push_back(os_server(id,IsEqualGUID(id,runtime::bandTrackRuntimeClass)?L"dmband.dll":L"dmime.dll"));}
            if(node.id=="RIFF"&&node.type=="DMBD")triggerServers.push_back(os_server(runtime::bandRuntimeClass,L"dmband.dll"));
            if(node.id=="LIST"&&node.type=="DMRF"){const auto h=node.find("refh"),id=node.find("guid");if(!h||h->data.size()<20||!id||id->data.size()!=16||(read32(h->data,16)&3)!=3||(read32(h->data,16)&(16|32|64|1024|2048)))throw std::runtime_error("Unresolved triggered runtime reference");GUID cls{};std::memcpy(&cls,h->data.data(),16);if(!IsEqualGUID(cls,runtime::segmentClass)&&!IsEqualGUID(cls,runtime::styleClass)&&!IsEqualGUID(cls,runtime::collectionClass)&&!IsEqualGUID(cls,wave_runtime_class)&&!IsEqualGUID(cls,runtime::scriptClass))throw std::runtime_error("Triggered reference needs declared source dependency resolver");}
            for(const auto& c:node.children)audit(c);
        };audit(root);
    }
    const auto verifyStyleCopies=[&](const std::vector<ResolvedStyle>& a,const std::vector<ResolvedStyle>& b){for(const auto& x:a)for(const auto& y:b)if(x.reference.hasId&&y.reference.hasId&&x.reference.objectId==y.reference.objectId&&x.bytes!=y.bytes)throw std::runtime_error("Conflicting owned trigger Style snapshots share a GUID");};verifyStyleCopies(snapshot.styles,triggers.styles);verifyStyleCopies(triggers.styles,triggers.styles);
    auto& s=*state_;
    collect_notifications();
    // A secondary instance shares the performance clock while owning its
    // loader, descriptors, downloads and SegmentState independently.
    // Let Performance replace primary playback at the resolved start time.
    // Keep its loader/downloads alive while a replacement is only scheduled.
    for(const auto id:playback_ids()){
        const auto p=position(id);
        if(!p.playing&&p.clocks>=p.start)stop(id);
    }
    if(s.id){auto retained=std::make_unique<PlaybackSession>();s.retained.reserve(s.retained.size()+1);std::swap(static_cast<PlaybackSession&>(s),*retained);s.retained.push_back(std::move(retained));}
    s.id=s.nextId++;
    s.name=motif?motif->name:L"Segment";
    if(motif&&!snapshot.styles[motif->styleIndex].path.empty())s.name=std::filesystem::path(snapshot.styles[motif->styleIndex].path).filename().wstring()+L" / "+s.name;
    try {
        initialize_runtime(owner);
        check("CoCreate Loader8",CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&s.loader)));
        servers_.insert(servers_.end(),triggerServers.begin(),triggerServers.end());
        s.waves=std::move(waveSnapshot.waves);s.waves.insert(s.waves.end(),triggers.waves.begin(),triggers.waves.end());s.runtimeWaves.reserve(s.waves.size());
        if(!s.waves.empty())servers_.push_back(os_server(wave_runtime_class,L"dswave.dll"));
        if(!snapshot.styles.empty())servers_.push_back(os_server(runtime::styleClass,L"dmstyle.dll"));
        check("Disable loader cache",s.loader->EnableCache(runtime::allTypes,FALSE));
        if(!triggers.segments.empty()||!triggers.motifs.empty()){check("Enable owned triggered Segment cache",s.loader->EnableCache(runtime::segmentClass,TRUE));check("Enable owned trigger Style cache",s.loader->EnableCache(runtime::styleClass,TRUE));}
        if(!directory.empty()){auto search=std::filesystem::absolute(directory).wstring();check("Set reference directory",s.loader->SetSearchDirectory(runtime::allTypes,search.data(),TRUE));}
        // Keep owned Wave objects bound to this session after global cache policy
        // and file search setup; Segment references must use the edited snapshot.
        if(!s.waves.empty())check("Enable owned Wave cache",s.loader->EnableCache(wave_runtime_class,TRUE));
        for(auto& dependency:s.waves){
            WaveDocument doc;doc.load(dependency.bytes);const auto id=doc.identity().value();
            runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);descriptor.valid=1|2|1024;descriptor.classId=wave_runtime_class;
            std::memcpy(&descriptor.objectId,id.data(),16);descriptor.memoryLength=static_cast<LONGLONG>(dependency.bytes.size());descriptor.memory=dependency.bytes.data();
            check("Register owned Wave snapshot",s.loader->SetObject(&descriptor));IUnknown* loaded=nullptr;
            const auto hr=s.loader->GetObject(&descriptor,IID_IUnknown,reinterpret_cast<void**>(&loaded));if(loaded)s.runtimeWaves.push_back(loaded);
            check("Load owned Wave snapshot",hr);if(hr!=S_OK||!loaded)throw std::runtime_error("Owned Wave load was partial");
        }
        s.collections=std::move(collectionSnapshot.collections);s.collections.insert(s.collections.end(),triggers.collections.begin(),triggers.collections.end());s.runtimeCollections.reserve(s.collections.size());if(!s.collections.empty())servers_.push_back(os_server(runtime::collectionClass,L"dmusic.dll"));
        for(auto& dependency:s.collections){runtime::ObjectDesc collectionDescriptor{};collectionDescriptor.size=sizeof(collectionDescriptor);collectionDescriptor.valid=1|2|1024;collectionDescriptor.classId=runtime::collectionClass;std::memcpy(&collectionDescriptor.objectId,dependency.reference.objectId->data(),16);collectionDescriptor.memoryLength=static_cast<LONGLONG>(dependency.bytes.size());collectionDescriptor.memory=dependency.bytes.data();check("Register owned DLS snapshot",s.loader->SetObject(&collectionDescriptor));runtime::Collection* loaded=nullptr;const auto loadedResult=s.loader->GetObject(&collectionDescriptor,runtime::collectionId,reinterpret_cast<void**>(&loaded));if(loaded)s.runtimeCollections.push_back(loaded);check("Load owned DLS snapshot",loadedResult);if(loadedResult!=S_OK||!loaded)throw std::runtime_error("Owned DLS load was partial");DWORD patch=0;WCHAR name[256]{};const auto enumeration=loaded->EnumInstrument(0,&patch,name,256);check("Enum owned DLS first instrument",enumeration);if(enumeration!=S_OK)throw std::runtime_error("Referenced DLS has no instrument");s.firstPatches.push_back(patch);}
        std::function<void(const Chunk&)> verifyInstruments=[&](const Chunk& root){if(root.id=="RIFF"&&root.type=="DMBD"){BandDocument band;band.load(root.encode());for(const auto& instrument:band.instruments())if(!instrument.collectionReference.empty()){const auto reference=collection_reference(instrument.collectionReference);size_t index=0;while(index<s.collections.size()&&s.collections[index].reference.objectId!=reference.objectId)++index;if(index==s.collections.size())throw std::runtime_error("Mapped Band collection missing");runtime::Instrument* selected=nullptr;const auto found=s.runtimeCollections[index]->GetInstrument(instrument.patch,&selected);if(selected)reinterpret_cast<IUnknown*>(selected)->Release();check("Get assigned owned DLS instrument",found);if(found!=S_OK)throw std::runtime_error("Assigned DLS instrument unavailable");}return;}for(const auto& child:root.children)verifyInstruments(child);};if(hasSegment)verifyInstruments(Chunk::parse(snapshot.segment));for(const auto& child:triggers.segments)verifyInstruments(Chunk::parse(child.bytes));for(const auto& style:snapshot.styles)verifyInstruments(Chunk::parse(style.bytes));
        s.styles=std::move(snapshot.styles);s.styles.insert(s.styles.end(),triggers.styles.begin(),triggers.styles.end());s.runtimeStyles.reserve(s.styles.size());
        if(hasSegment){
            const auto source=Chunk::parse(snapshot.segment);
            if(const auto path=source.find("RIFF","DMAP")){
                const auto config=path->encode();
                snapshot.segment=prepare_audio_path_band_downloads(snapshot.segment,config);
                for(auto& dependency:s.styles)dependency.bytes=prepare_audio_path_band_downloads(dependency.bytes,config);
            }
        }
        for(auto& dependency:s.styles){
            runtime::ObjectDesc styleDescriptor{};styleDescriptor.size=sizeof(styleDescriptor);styleDescriptor.valid=2|1024;styleDescriptor.classId=runtime::styleClass;styleDescriptor.memoryLength=static_cast<LONGLONG>(dependency.bytes.size());styleDescriptor.memory=dependency.bytes.data();
            if(dependency.reference.hasId){styleDescriptor.valid|=1;std::memcpy(&styleDescriptor.objectId,dependency.reference.objectId.data(),16);}
            // Register by object GUID and memory only. A filename-valid
            // descriptor makes this OS loader attempt to open that filename.
            check("Register owned Style snapshot",s.loader->SetObject(&styleDescriptor));
            runtime::Style* loaded=nullptr;check("Load owned Style snapshot",s.loader->GetObject(&styleDescriptor,runtime::styleId,reinterpret_cast<void**>(&loaded)));s.runtimeStyles.push_back(loaded);
            runtime::StyleTimeSignature meter{};check("Get Style runtime meter",loaded->GetTimeSignature(&meter));double tempo=0;check("Get Style runtime tempo",loaded->GetTempo(&tempo));
            StyleDocument source;source.load(dependency.bytes);const auto expected=source.meter();if(meter.beats!=expected.beats||meter.denominator!=expected.denominator||meter.grids!=expected.grids||tempo!=source.tempo())throw std::runtime_error("OS Style header interpretation differs from source reader");
        }
        // This loader is session-local. Segment/Style caches bind the trigger
        // track to the same owned objects whose downloads and Stop we retain.
        if(!triggers.segments.empty()||!triggers.motifs.empty()){
            check("Enable owned triggered Segment cache",s.loader->EnableCache(runtime::segmentClass,TRUE));
            check("Enable owned trigger Style cache",s.loader->EnableCache(runtime::styleClass,TRUE));
        }
        s.maps=maps;s.runtimeMaps.reserve(s.maps.size());
        if(!s.maps.empty()){
            servers_.push_back(os_server(runtime::chordMapClass,L"dmcompos.dll"));
            servers_.push_back(os_server(runtime::chordMapTrackClass,L"dmcompos.dll"));
            check("Enable owned ChordMap cache",s.loader->EnableCache(runtime::chordMapClass,TRUE));
        }
        for(auto& source:s.maps){
            runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=1|2|1024;d.classId=runtime::chordMapClass;std::memcpy(&d.objectId,source.reference.objectId.data(),16);d.memoryLength=source.bytes.size();d.memory=source.bytes.data();
            check("Register owned ChordMap snapshot",s.loader->SetObject(&d));runtime::ChordMap* loaded=nullptr;const auto hr=s.loader->GetObject(&d,runtime::chordMapId,reinterpret_cast<void**>(&loaded));if(loaded)s.runtimeMaps.push_back(loaded);check("Load owned ChordMap snapshot",hr);if(hr!=S_OK||!loaded)throw std::runtime_error("Owned ChordMap load incomplete");
            DWORD scale=0;check("Get owned ChordMap scale",loaded->GetScale(&scale));ChordMapDocument expected;expected.load(source.bytes);if((scale&0xffffff)!=expected.scale())throw std::runtime_error("OS ChordMap scale differs from source snapshot");
        }
        s.scripts=triggers.scripts;s.runtimeScripts.reserve(s.scripts.size());s.sourceScripts.reserve(s.scripts.size());s.scriptLoaderMemory.reserve(s.scripts.size());
        if(!s.scripts.empty())check("Enable owned Script Track cache",s.loader->EnableCache(runtime::scriptClass,TRUE));
        for(auto& source:s.scripts){
            ScriptDocument authored;authored.load(source.bytes);s.scriptLoaderMemory.push_back(source_script_loader_bytes(authored));auto& helperBytes=s.scriptLoaderMemory.back();
            runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=1|2|1024;d.classId=runtime::scriptClass;std::memcpy(&d.objectId,source.objectId.data(),16);d.memoryLength=helperBytes.size();d.memory=helperBytes.data();check("Register owned Track Script",s.loader->SetObject(&d));
            runtime::Script* loaded=nullptr;const auto hr=s.loader->GetObject(&d,runtime::scriptId,reinterpret_cast<void**>(&loaded));if(loaded)s.runtimeScripts.push_back(loaded);check("Load owned Track Script",hr);if(hr!=S_OK||!loaded)throw std::runtime_error("Track Script partial load");
            runtime::ScriptErrorInfo error{};error.size=sizeof(error);check("Init owned Track Script",loaded->Init(s.performance,&error));
            runtime::Script* execution=loaded;if(uses_source_script_host(authored)){check("Init source Track Script",create_source_script_host(authored,loaded,s.performance,&execution,&error));}else execution->AddRef();s.sourceScripts.push_back(execution);
            std::vector<std::wstring> routines;bool ended=false;for(DWORD i=0;i<4096;++i){WCHAR name[260]{};const auto result=execution->EnumRoutine(i,name);if(result==S_FALSE){ended=true;break;}check("Enum Track Script routine",result);size_t n=0;while(n<260&&name[n])++n;if(!n||n==260)throw std::runtime_error("Track Script routine enumeration invalid");routines.emplace_back(name,n);}if(!ended)throw std::runtime_error("Track Script routine enumeration exceeded bound");
            for(const auto& routine:source.routines)if(std::count(routines.begin(),routines.end(),routine)!=1)throw std::runtime_error("Track Script routine missing or ambiguous");
        }
        s.triggeredSnapshots=triggers.segments;if(!pathConfig.empty())for(auto& child:s.triggeredSnapshots)child.bytes=prepare_audio_path_band_downloads(child.bytes,pathConfig);
        for(const auto& child:s.triggeredSnapshots)s.triggeredLoaderMemory.push_back(NativeScriptTrackRuntime::loader_bytes(child.bytes));
        size_t childMemoryIndex=0;
        for(auto& child:s.triggeredSnapshots){runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=1|2|1024;d.classId=runtime::segmentClass;std::memcpy(&d.objectId,child.objectId.data(),16);auto& loaderBytes=s.triggeredLoaderMemory[childMemoryIndex++];d.memoryLength=loaderBytes.size();d.memory=loaderBytes.data();check("Register owned triggered Segment",s.loader->SetObject(&d));}
        s.triggeredSegments.reserve(s.triggeredSnapshots.size()+triggers.motifs.size());
        for(auto& child:s.triggeredSnapshots){runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=3;d.classId=runtime::segmentClass;std::memcpy(&d.objectId,child.objectId.data(),16);runtime::Segment* loaded=nullptr;const auto hr=s.loader->GetObject(&d,runtime::segment8Id,reinterpret_cast<void**>(&loaded));if(loaded)s.triggeredSegments.push_back(loaded);check("Load owned triggered Segment",hr);if(hr!=S_OK||!loaded)throw std::runtime_error("Triggered Segment partial load");}
        for(const auto& m:triggers.motifs){size_t i=0;while(i<s.styles.size()&&s.styles[i].reference.objectId!=m.styleId)++i;if(i==s.styles.size())throw std::runtime_error("Triggered Motif Style missing");
            runtime::Segment* motif=nullptr;auto name=m.name;check("Get owned triggered Motif",s.runtimeStyles[i]->GetMotif(name.data(),&motif));if(!motif)throw std::runtime_error("Triggered Motif unavailable");runtime::Segment* owned=nullptr;const auto hr=motif->QueryInterface(runtime::segment8Id,reinterpret_cast<void**>(&owned));release(motif);if(owned)s.triggeredSegments.push_back(owned);check("Query triggered Motif Segment8",hr);
        }
        s.memory=std::move(snapshot.segment);s.loaderMemory=NativeScriptTrackRuntime::loader_bytes(s.memory);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);descriptor.valid=2|1024;descriptor.classId=runtime::segmentClass;descriptor.memoryLength=static_cast<LONGLONG>(s.loaderMemory.size());descriptor.memory=s.loaderMemory.data();
        HRESULT segmentResult;
        if(motif){
            s.motif=true;
            runtime::Segment* generated=nullptr;auto name=motif->name;
            const auto obtained=s.runtimeStyles.at(motif->styleIndex)->GetMotif(name.data(),&generated);
            if(obtained!=S_OK||!generated){release(generated);check("Get owned Motif",obtained);throw std::runtime_error("Owned Motif runtime lookup returned no complete segment");}
            calls_.push_back({"Get owned Motif",obtained});
            segmentResult=generated->QueryInterface(runtime::segment8Id,reinterpret_cast<void**>(&s.segment));release(generated);
            check("Query Motif Segment8",segmentResult);
        }else{segmentResult=s.loader->GetObject(&descriptor,runtime::segment8Id,reinterpret_cast<void**>(&s.segment));check("Load current DMSG",segmentResult);}
        // A success-severity partial-load warning can drop an unsupported
        // track. Never download/play that subset as the current document.
        if(segmentResult!=S_OK||!s.segment)throw std::runtime_error("Current Segment load was partial; playback refused");
        for(size_t i=0;i<s.waves.size();++i){WaveDocument wave;wave.load(s.waves[i].bytes);const auto id=wave.identity().value();runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=3;d.classId=wave_runtime_class;std::memcpy(&d.objectId,id.data(),16);IUnknown* cached=nullptr;const auto hr=s.loader->GetObject(&d,IID_IUnknown,reinterpret_cast<void**>(&cached));const bool same=cached==s.runtimeWaves[i];release(cached);check("Verify owned Wave cache after Segment load",hr);if(hr!=S_OK||!same)throw std::runtime_error("Wave Track Loader cache identity changed");}
        for(size_t i=0;i<s.scripts.size();++i){runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=3;d.classId=runtime::scriptClass;std::memcpy(&d.objectId,s.scripts[i].objectId.data(),16);runtime::Script* cached=nullptr;const auto hr=s.loader->GetObject(&d,runtime::scriptId,reinterpret_cast<void**>(&cached));const bool same=cached==s.runtimeScripts[i];release(cached);check("Verify owned Script cache after Segment load",hr);if(hr!=S_OK||!same)throw std::runtime_error("Script Track Loader cache identity changed");}
        bool sourceScriptTracks=false;if(!s.memory.empty()){const auto root=Chunk::parse(s.memory);if(const auto tracks=root.find("LIST","trkl"))for(const auto& t:tracks->children)if(t.find("LIST","scrt"))sourceScriptTracks=true;}for(const auto& child:s.triggeredSnapshots){const auto root=Chunk::parse(child.bytes);if(const auto tracks=root.find("LIST","trkl"))for(const auto& t:tracks->children)if(t.find("LIST","scrt"))sourceScriptTracks=true;}
        if(sourceScriptTracks){servers_.push_back(os_server(runtime::graphClass,L"dmime.dll"));s.nativeScripts=std::make_unique<NativeScriptTrackRuntime>(s.scripts,s.sourceScripts);s.nativeScripts->attach(s.segment,s.memory);for(size_t i=0;i<s.triggeredSnapshots.size();++i)s.nativeScripts->attach(s.triggeredSegments[i],s.triggeredSnapshots[i].bytes);}
        if(!graphBytes.empty()){servers_.push_back(os_server(runtime::graphClass,L"dmime.dll"));s.toolGraph=create_tool_graph(graphBytes,toolFactories_);check("Attach owned Segment ToolGraph",s.segment->SetGraph(s.toolGraph.get()));}
        if(hasSegment&&Chunk::parse(s.memory).find("RIFF","DMAP")){
            // GetMotif creates a new Segment, not the context DMSG containing
            // the user's AudioPath. Load the context solely as a configuration
            // carrier; never Download or Play it in place of the selected Motif.
            runtime::Segment* carrier=nullptr;
            std::unique_ptr<runtime::Segment,void(*)(runtime::Segment*)> context(nullptr,[](runtime::Segment* p){if(p)p->Release();});
            if(motif){
                const auto loaded=s.loader->GetObject(&descriptor,runtime::segment8Id,reinterpret_cast<void**>(&carrier));context.reset(carrier);
                check("Load Motif AudioPath context DMSG",loaded);
                if(loaded!=S_OK||!context)throw std::runtime_error("Motif AudioPath context load was incomplete");
            }
            IUnknown* config=nullptr;const auto obtained=(context?context.get():s.segment)->GetAudioPathConfig(&config);
            if(obtained!=S_OK||!config){release(config);check("Get embedded AudioPath config",obtained);throw std::runtime_error("Embedded AudioPath configuration was incomplete");}
            calls_.push_back({"Get embedded AudioPath config",obtained});
            HRESULT created=S_OK;
            if(s.recordingPath){s.audioPath=s.recordingPath;s.audioPath->AddRef();}
            else created=s.performance->CreateAudioPath(config,TRUE,&s.audioPath);release(config);check("Create embedded AudioPath",created);
            if(created!=S_OK||!s.audioPath)throw std::runtime_error("Embedded AudioPath creation was incomplete");
            if(!pathGraphBytes.empty()){
                servers_.push_back(os_server(runtime::graphClass,L"dmime.dll"));runtime::Graph* graph=nullptr;
                const auto found=s.audioPath->GetObjectInPath(0,0x2200,0,GUID_NULL,0,runtime::graphId,reinterpret_cast<void**>(&graph));
                s.audioPathGraph.reset(graph);check("Get owned AudioPath ToolGraph",found);
                if(found!=S_OK||!graph)throw std::runtime_error("AudioPath graph lookup was incomplete");
                populate_tool_graph(*graph,pathGraphBytes,toolFactories_);
            }
            // The performance graph observes remapped channels, not the local
            // PChannels serialized in the Segment. Record the public mapping
            // Producer's AudioPath audition contract leaves PChannels outside
            // its mix groups silent. Keep their events in the runtime document;
            // the AudioPath, rather than a destructive note filter, owns routing.
            SegmentDocument routed;routed.load(s.memory);std::map<DWORD,std::optional<DWORD>> mapping;
            const auto mapChannel=[&](DWORD local){if(!mapping.count(local)){
                DWORD channel=0;const auto converted=s.audioPath->ConvertPChannel(local,&channel);
                if(converted==DMUS_E_NOT_FOUND){
                    calls_.push_back({"AudioPath PChannel "+std::to_string(local)+" disconnected; silent",converted});
                    // Remember this query without inventing a mapped channel.
                    // Repeated notes on the same local channel remain intact.
                    mapping.emplace(local,std::nullopt);return;
                }
                check("Convert embedded AudioPath PChannel",converted);mapping.emplace(local,channel);
                calls_.push_back({"Embedded AudioPath PChannel "+std::to_string(local)+" maps to "+std::to_string(channel),S_OK});
            }};
            for(const auto& note:routed.notes())mapChannel(note.channel);
            // Style-generated notes are absent from the Segment Sequence.
            // Validate each declared Part channel through the same public API.
            for(const auto& dependency:s.styles){StyleDocument style;style.load(dependency.bytes);
                const auto patterns=style.patterns();for(size_t i=0;i<patterns.size();++i)
                    for(const auto& reference:style.part_references(i))if(reference.pchannel)mapChannel(*reference.pchannel);
            }
        }
        if(!s.audioPath){
            // Own a standard path for this session, just as configured paths
            // are session-owned. Reusing InitAudio's default path after custom
            // path teardown can leave its synth silent despite successful API
            // calls. Download, Play and Unload all use this same owned path.
            const auto obtained=s.performance->CreateStandardAudioPath(8,16,TRUE,&s.audioPath);
            check("Create standard AudioPath",obtained);
            if(obtained!=S_OK||!s.audioPath)throw std::runtime_error("Standard AudioPath unavailable");
        }
        if(s.lyricObserver||s.scriptMessageObserver){DWORD channel=0;const auto converted=s.audioPath->ConvertPChannel(0,&channel);calls_.push_back({"Message Window AudioPath PChannel 1",converted});if(converted!=DMUS_E_NOT_FOUND){check("Convert Message Window PChannel",converted);if(converted!=S_OK||channel>=0xfffffffc)throw std::runtime_error("Message Window channel mapping invalid");if(s.lyricObserver)s.lyricObserver->include_channel(channel);}s.messageWindowEligible=converted==S_OK;if(s.scriptMessageObserver)s.scriptMessageObserver->set_visible(s.messageWindowEligible);}
        // GetMotif generates notes and Band data without necessarily carrying
        // its Style's tempo. A standalone primary owns the musical clock;
        // secondary instances inherit that clock and must not reset it.
        if(motif&&!hasSegment&&!options.secondary){
            StyleDocument selected;selected.load(s.styles.at(motif->styleIndex).bytes);const auto bpm=selected.tempo();
            producer::TempoParam existing{};LONG next=0;
            const auto found=s.segment->GetParam(producer::GUID_TempoParam,0xffffffff,0,0,&next,&existing);
            calls_.push_back({"Find standalone Motif tempo track",found});
            if(found==DMUS_E_TRACK_NOT_FOUND){
                servers_.push_back(os_server(producer::CLSID_DirectMusicTempoTrack,L"dmime.dll"));
                IPersistStream* rawPersist=nullptr;const auto created=CoCreateInstance(producer::CLSID_DirectMusicTempoTrack,nullptr,CLSCTX_INPROC_SERVER,IID_IPersistStream,reinterpret_cast<void**>(&rawPersist));
                std::unique_ptr<IPersistStream,void(*)(IPersistStream*)> persist(rawPersist,[](IPersistStream* p){if(p)p->Release();});check("Create standalone Motif tempo track",created);
                tempo::Track authored;authored.replace_events({{0,bpm}});const auto bytes=authored.save();IStream* rawStream=nullptr;
                const auto allocated=CreateStreamOnHGlobal(nullptr,TRUE,&rawStream);std::unique_ptr<IStream,void(*)(IStream*)> stream(rawStream,[](IStream* p){if(p)p->Release();});check("Create standalone tempo stream",allocated);
                ULONG written=0;check("Write standalone tempo stream",stream->Write(bytes.data(),static_cast<ULONG>(bytes.size()),&written));if(written!=bytes.size())throw std::runtime_error("Standalone tempo stream write incomplete");LARGE_INTEGER zero{};check("Seek standalone tempo stream",stream->Seek(zero,STREAM_SEEK_SET,nullptr));
                check("Load standalone Motif tempo track",persist->Load(stream.get()));IUnknown* rawTrack=nullptr;
                const auto queried=persist->QueryInterface(producer::IID_DirectMusicTrack,reinterpret_cast<void**>(&rawTrack));std::unique_ptr<IUnknown,void(*)(IUnknown*)> track(rawTrack,[](IUnknown* p){if(p)p->Release();});check("Query standalone tempo track",queried);
                check("Insert standalone Motif tempo track",s.segment->InsertTrack(reinterpret_cast<runtime::Track*>(track.get()),1));
            }else{
                check("Find standalone Motif tempo track",found);if(found!=S_OK)throw std::runtime_error("Standalone Motif tempo lookup incomplete");
                producer::TempoParam value{0,bpm};check("Set standalone Motif tempo",s.segment->SetParam(producer::GUID_TempoParam,0xffffffff,0,0,&value));
            }
            producer::TempoParam resolved{};check("Verify standalone Motif tempo",s.segment->GetParam(producer::GUID_TempoParam,0xffffffff,0,0,&next,&resolved));if(resolved.tempo!=bpm)throw std::runtime_error("Standalone Motif runtime tempo differs from Style");
        }
        DWORD defaultResolution=0;check("Get runtime default resolution",s.segment->GetDefaultResolution(&defaultResolution));
        for(auto* child:s.triggeredSegments){const auto hr=child->Download(s.audioPath?s.audioPath:static_cast<IUnknown*>(s.performance));++s.triggeredDownloads;check("Download owned triggered Segment",hr);if(hr!=S_OK)throw std::runtime_error("Triggered Segment download partial");}
        s.downloaded=true;const auto download=s.segment->Download(s.audioPath?s.audioPath:static_cast<IUnknown*>(s.performance));check("Download segment",download);if(!s.collections.empty()&&download!=S_OK)throw std::runtime_error("Owned DLS segment download was partial");
        // Give the performance its advertised preparation window. Immediate
        // starts can reach the synth before the initial Band/first note is ready.
        DWORD prepareMilliseconds=0;check("Get playback prepare time",s.performance->GetPrepareTime(&prepareMilliseconds));
        calls_.push_back({"Playback prepare milliseconds "+std::to_string(prepareMilliseconds),S_OK});
        LONG submitted=0;check("Get scheduling music time",s.performance->GetTime(nullptr,&submitted));
        const auto requested=options.delayClocks?static_cast<LONGLONG>(submitted)+options.delayClocks:0;
        if(requested>std::numeric_limits<LONG>::max())throw std::runtime_error("Scheduled music time exceeds runtime range");
        s.request={flags,submitted,static_cast<LONG>(requested)};
        s.request.runtimeDefaultResolution=defaultResolution;
        calls_.push_back({"Playback flags "+std::to_string(flags)+" requested clocks "+std::to_string(requested),S_OK});
        check("PlaySegmentEx",s.performance->PlaySegmentEx(s.segment,nullptr,nullptr,flags,requested,&s.playing,nullptr,s.audioPath));
        register_notification_identity();
        check("Get resolved playback start",s.playing->GetStartTime(&s.request.actualStart));
    } catch(...){
        // Failure of a new instance must not tear down unrelated playback.
        try{stop_current();}catch(...){shutdown();throw;}
        if(!s.retained.empty()){std::swap(static_cast<PlaybackSession&>(s),*s.retained.back());s.retained.pop_back();}
        throw;
    }
}
void Conductor::stop(){for(const auto id:playback_ids())stop(id);if(state_->scriptMessageObserver)state_->scriptMessageObserver->set_visible(true);}
void Conductor::stop_current() {
    auto& s=*state_;
    if(s.nativeScripts){s.nativeScripts->cancel();for(auto call:s.nativeScripts->calls()){call.playbackId=s.id;call.messageVisible=s.messageWindowEligible;if(state_->scriptHistory.size()<4096)state_->scriptHistory.push_back(call);else state_->scriptHistoryOverflow=true;}state_->scriptHistoryOverflow|=s.nativeScripts->overflow();}
    if(s.playing)register_notification_identity();
    if(s.performance&&s.segment){
        check("StopEx",s.performance->StopEx(s.playing?s.playing:static_cast<IUnknown*>(s.segment),0,0));
        const auto deadline=GetTickCount64()+2000;HRESULT playing;
        do {playing=s.performance->IsPlaying(s.segment,s.playing);if(playing!=S_OK)break;Sleep(10);}while(GetTickCount64()<deadline);
        check("IsPlaying after StopEx",playing);
        if(playing==S_OK)check("Stop timeout",HRESULT_FROM_WIN32(ERROR_TIMEOUT));
    }
    collect_notifications();
    for(auto* child:s.triggeredSegments)if(s.performance){check("Stop owned triggered Segment",s.performance->StopEx(child,0,0));const auto deadline=GetTickCount64()+2000;HRESULT playing;do{playing=s.performance->IsPlaying(child,nullptr);if(playing!=S_OK)break;Sleep(10);}while(GetTickCount64()<deadline);check("IsPlaying triggered Segment after Stop",playing);if(playing==S_OK)check("Triggered Segment stop timeout",HRESULT_FROM_WIN32(ERROR_TIMEOUT));}
    for(size_t i=0;i<s.triggeredDownloads;++i)check("Unload owned triggered Segment",s.triggeredSegments[i]->Unload(s.audioPath?s.audioPath:static_cast<IUnknown*>(s.performance)));s.triggeredDownloads=0;
    for(auto*& child:s.triggeredSegments)release(child);s.triggeredSegments.clear();s.triggeredSnapshots.clear();
    release(s.playing);
    // Public notification consumers receive value copies even after release.
    if(s.downloaded){check("Unload segment",s.segment->Unload(s.audioPath?s.audioPath:static_cast<IUnknown*>(s.performance)));s.downloaded=false;}
    s.audioPathGraph.reset();release(s.audioPath);release(s.segment);s.toolGraph.reset();s.motif=false;for(auto& style:s.runtimeStyles)release(style);s.runtimeStyles.clear();for(auto& collection:s.runtimeCollections)release(collection);s.runtimeCollections.clear();for(auto& wave:s.runtimeWaves)release(wave);s.runtimeWaves.clear();s.nativeScripts.reset();for(auto& script:s.sourceScripts)release(script);s.sourceScripts.clear();for(auto& script:s.runtimeScripts)release(script);s.runtimeScripts.clear();for(auto& map:s.runtimeMaps)release(map);s.runtimeMaps.clear();release(s.loader);s.scriptLoaderMemory.clear();s.maps.clear();s.scripts.clear();s.triggeredLoaderMemory.clear();s.loaderMemory.clear();s.waves.clear();s.styles.clear();s.collections.clear();s.firstPatches.clear();s.memory.clear();s.id=0;s.request={};
}
PlaybackPosition Conductor::position() {
    auto& s=*state_;if(!s.performance||!s.segment||!s.playing)return {};
    const auto playing=s.performance->IsPlaying(s.segment,s.playing);check("IsPlaying",playing);
    PlaybackPosition p{};p.playing=playing==S_OK;check("GetTime",s.performance->GetTime(nullptr,&p.clocks));check("GetStartTime",s.playing->GetStartTime(&p.start));
    for(auto* child:s.triggeredSegments){const auto childPlaying=s.performance->IsPlaying(child,nullptr);check("IsPlaying owned triggered Segment",childPlaying);if(childPlaying==S_OK)p.playing=true;}
    if(p.playing){producer::TempoParam tempo{};LONG next=0;const auto hr=s.performance->GetParam(producer::GUID_TempoParam,0xffffffff,0,p.clocks,&next,&tempo);
        // Wave-only Segments legitimately provide no Tempo parameter. Preserve
        // that absence rather than inventing a tempo or rejecting playback.
        if(hr==DMUS_E_TRACK_NOT_FOUND||(s.motif&&(s.request.flags&runtime::playSecondary)&&hr==DMUS_E_NOT_FOUND)){
            calls_.push_back({s.motif?"Motif runtime tempo unavailable":"Segment runtime tempo unavailable",hr});
        }else{check("Get runtime tempo",hr);p.tempo=tempo.tempo;p.tempoAvailable=true;}
    }return p;
}
void Conductor::register_notification_identity(){
    auto& s=*state_;if(!s.playing||!s.id)return;IUnknown* identity=nullptr;
    const auto hr=s.playing->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&identity));
    const std::unique_ptr<IUnknown,void(*)(IUnknown*)> owner(identity,[](IUnknown* value){if(value)value->Release();});
    check("Read playback notification identity",hr);if(hr!=S_OK||!identity)throw std::runtime_error("Playback canonical identity unavailable");s.notificationIds.insert_or_assign(identity,s.id);
}
void Conductor::collect_notifications(){
    auto& s=*state_;if(!s.performance)return;std::vector<PlaybackId> terminal;
    for(unsigned count=0;count<4096;++count){
        runtime::NotificationMessage* message=nullptr;const auto hr=s.performance->GetNotificationPMsg(&message);
        struct OwnedMessage {runtime::Performance* owner;runtime::NotificationMessage* value;~OwnedMessage(){if(value)owner->FreePMsg(&value->message);}} owned{s.performance,message};
        check("Get notification",hr);
        if(hr==S_FALSE){for(const auto id:terminal)for(auto i=s.notificationIds.begin();i!=s.notificationIds.end();){if(i->second==id)i=s.notificationIds.erase(i);else ++i;}return;}
        if(hr!=S_OK||!message||message->message.size<sizeof(*message))throw std::runtime_error("Incomplete runtime notification");
        IUnknown* identity=nullptr;const auto queried=message->message.user?message->message.user->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&identity)):E_NOINTERFACE;
        PlaybackId id=0;if(queried==S_OK&&identity){const auto found=s.notificationIds.find(identity);if(found!=s.notificationIds.end())id=found->second;}release(identity);
        s.pendingNotifications.push_back({message->notificationType,message->option,message->field1,message->field2,message->message.group,message->message.musicTime,id&&id==s.id,id});
        if(id&&IsEqualGUID(message->notificationType,runtime::segmentNotification)&&(message->option==runtime::segmentEnded||message->option==4))terminal.push_back(id);
        owned.value=nullptr;check("Free notification",s.performance->FreePMsg(&message->message));
    }
    throw std::runtime_error("Runtime notification queue exceeded bounded drain");
}
std::vector<PlaybackNotification> Conductor::notifications(){
    collect_notifications();auto& s=*state_;std::vector<PlaybackNotification> result;result.swap(s.pendingNotifications);for(auto& n:result)n.currentSegment=n.playbackId&&n.playbackId==s.id;return result;
}
size_t Conductor::notification_identity_count()const{return state_->notificationIds.size();}
size_t Conductor::pending_notification_count()const{return state_->pendingNotifications.size();}
std::optional<DWORD> Conductor::performance_channel(DWORD localChannel) {
    if(!state_->audioPath||localChannel>=0xfffffffc)throw std::runtime_error("AudioPath mapping requires a loaded path and nonreserved channel");
    DWORD mapped=0;const auto hr=state_->audioPath->ConvertPChannel(localChannel,&mapped);
    const auto operation="Observe AudioPath ConvertPChannel "+std::to_string(localChannel);
    if(hr==DMUS_E_NOT_FOUND){calls_.push_back({operation+" disconnected; silent",hr});return std::nullopt;}
    check(operation.c_str(),hr);
    if(hr!=S_OK||mapped>=0xfffffffc)throw std::runtime_error("AudioPath channel mapping incomplete");return mapped;
}
StyleMeter Conductor::segment_meter(LONG time,DWORD groups,DWORD index) {
    if(!state_->segment)throw std::runtime_error("No loaded segment for meter query");
    producer::TimeSignatureParam meter{};LONG next=0;check("Get segment runtime meter",state_->segment->GetParam(producer::GUID_TimeSignatureParam,groups,index,time,&next,&meter));
    return {meter.beats,meter.denominator,meter.grids};
}
DWORD Conductor::segment_chordmap_scale(LONG time,DWORD groups,DWORD index){
    if(!state_->segment||!groups||time<0)throw std::runtime_error("ChordMap query requires loaded Segment, clock and group");
    runtime::ChordMap* map=nullptr;LONG next=0;const auto hr=state_->segment->GetParam(runtime::chordMapParam,groups,index,time,&next,&map);
    const std::unique_ptr<runtime::ChordMap,void(*)(runtime::ChordMap*)> owned(map,[](runtime::ChordMap* p){if(p)p->Release();});
    check("Get Segment runtime ChordMap",hr);if(hr!=S_OK||!map)throw std::runtime_error("Segment ChordMap binding incomplete");DWORD scale=0;check("Get bound ChordMap scale",map->GetScale(&scale));return scale;
}
DWORD Conductor::segment_track_group(REFGUID type,DWORD groups,DWORD index) {
    if(!state_->segment)throw std::runtime_error("No loaded Segment");
    if(!groups)throw std::runtime_error("Track group mask is zero");
    runtime::Track* track=nullptr;
    const auto result=state_->segment->GetTrack(type,groups,index,&track);
    DWORD actual=0;const auto groupResult=result==S_OK&&track?state_->segment->GetTrackGroup(track,&actual):E_UNEXPECTED;
    // GetTrack owns a reference even when a failing provider returns a pointer.
    if(track)reinterpret_cast<IUnknown*>(track)->Release();
    check("Segment GetTrack",result);
    if(result!=S_OK)throw std::runtime_error("Segment GetTrack incomplete");
    check("Segment GetTrackGroup",groupResult);
    if(groupResult!=S_OK||!actual)throw std::runtime_error("Segment GetTrackGroup incomplete");
    return actual;
}
double Conductor::segment_tempo(LONG time,DWORD groups,DWORD index) {
    if(!state_->segment||!groups)throw std::runtime_error("Tempo query requires a loaded Segment and nonzero mask");
    producer::TempoParam tempo{};LONG next=0;
    const auto result=state_->segment->GetParam(producer::GUID_TempoParam,groups,index,time,&next,&tempo);
    check("Get segment runtime tempo",result);
    if(result!=S_OK)throw std::runtime_error("Segment tempo query incomplete");
    return tempo.tempo;
}
CommandEvent Conductor::segment_command(LONG time,DWORD groups,DWORD index){
    const auto sample=command_parameter(time,groups,index,false);if(sample.result!=S_OK)throw std::runtime_error("Segment command query incomplete");return sample.event;
}
CommandParameterSample Conductor::command_parameter(LONG time,DWORD groups,DWORD index,bool withTime){
    if(!state_->segment||!groups)throw std::runtime_error("Command query requires loaded Segment and nonzero mask");
    LONG next=0;runtime::CommandParam command{};runtime::CommandParam2 timed{};
    const auto result=state_->segment->GetParam(withTime?runtime::commandParam2:runtime::commandParam,groups,index,time,&next,withTime?static_cast<void*>(&timed):static_cast<void*>(&command));
    calls_.push_back({withTime?"Get segment runtime CommandParam2":"Get segment runtime command",result});
    return {result,next,withTime?CommandEvent{timed.time,0,0,timed.type,timed.groove,timed.range,timed.repeat}:CommandEvent{time,0,0,command.type,command.groove,command.range,command.repeat}};
}
void Conductor::shutdown() noexcept {
    auto& s=*state_;
    auto log=[&](const char* op,HRESULT result)noexcept{try{calls_.push_back({op,result});}catch(...){}};
    auto cleanup=[&](PlaybackSession& session){if(session.nativeScripts)session.nativeScripts->cancel();
        if(s.performance&&session.segment)log("StopEx cleanup",s.performance->StopEx(session.playing?session.playing:static_cast<IUnknown*>(session.segment),0,0));
        for(auto* child:session.triggeredSegments)if(s.performance)log("Stop trigger cleanup",s.performance->StopEx(child,0,0));
        for(size_t i=0;i<session.triggeredDownloads;++i)if(s.performance)log("Unload trigger cleanup",session.triggeredSegments[i]->Unload(session.audioPath?session.audioPath:static_cast<IUnknown*>(s.performance)));session.triggeredDownloads=0;
        for(auto*& child:session.triggeredSegments)release(child);session.triggeredSegments.clear();session.triggeredSnapshots.clear();
        release(session.playing);if(session.downloaded&&session.segment&&s.performance)log("Unload cleanup",session.segment->Unload(session.audioPath?session.audioPath:static_cast<IUnknown*>(s.performance)));session.downloaded=false;
        release(session.audioPath);release(session.segment);for(auto& style:session.runtimeStyles)release(style);session.runtimeStyles.clear();for(auto& collection:session.runtimeCollections)release(collection);session.runtimeCollections.clear();for(auto& wave:session.runtimeWaves)release(wave);session.runtimeWaves.clear();session.nativeScripts.reset();for(auto& script:session.sourceScripts)release(script);session.sourceScripts.clear();for(auto& script:session.runtimeScripts)release(script);session.runtimeScripts.clear();for(auto& map:session.runtimeMaps)release(map);session.runtimeMaps.clear();release(session.loader);session.scriptLoaderMemory.clear();session.maps.clear();session.scripts.clear();session.triggeredLoaderMemory.clear();session.loaderMemory.clear();session.waves.clear();session.styles.clear();session.collections.clear();session.firstPatches.clear();session.memory.clear();session.id=0;session.request={};
    };
    cleanup(s);for(auto& retained:s.retained)cleanup(*retained);s.retained.clear();
    s.script.reset();
    if(s.recordingControl)log("Finalize FileOutput cleanup",s.recordingControl->Stop());release(s.recordingControl);release(s.recordingPath);s.recordingConfig.clear();
    if(s.performance)log("CloseDown",s.performance->CloseDown());s.audio=false;release(s.performance);
    s.notificationIds.clear();s.pendingNotifications.clear();
    // CloseDown completes the realtime callbacks before releasing the observer.
    release(s.graph);release(s.observer);release(s.lyricObserver);release(s.scriptMessageObserver);
    s.fileOutputRegistration.reset();
    if(s.com){CoUninitialize();s.com=false;}
}
}
