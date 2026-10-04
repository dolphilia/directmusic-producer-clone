#include "conductor.h"
#include "document.h"
#include "audio_path.h"
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
        if(message->type==1&&message->size>=sizeof(runtime::NoteMessage)){
            const auto& n=*reinterpret_cast<const runtime::NoteMessage*>(message);
            const PlaybackNote copy{message->musicTime,n.duration,message->pchannel,message->group,n.musicValue,n.midiValue,n.velocity,n.noteFlags,n.playMode};
            AcquireSRWLockExclusive(&lock_);if(count_<notes_.size())notes_[count_++]=copy;else overflow_=true;ReleaseSRWLockExclusive(&lock_);
        }
        // Stamp BEFORE returning to avoid routing the same message back to us.
        if(message->graph&&SUCCEEDED(message->graph->StampPMsg(message)))return DMUS_S_REQUEUE;
        InterlockedExchange(&forwardingFailed_,1);return DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(runtime::Performance* performance,runtime::Message* message,LONGLONG) override {return ProcessPMsg(performance,message);}
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
    runtime::Loader* loader=nullptr;
    runtime::Segment* segment=nullptr;runtime::SegmentState* playing=nullptr;
    runtime::AudioPath* audioPath=nullptr;
    Bytes memory;bool downloaded=false,motif=false;
    std::vector<ResolvedStyle> styles;
    std::vector<runtime::Style*> runtimeStyles;
    std::vector<ResolvedCollection> collections;
    std::vector<runtime::Collection*> runtimeCollections;
    std::vector<DWORD> firstPatches;
    PlaybackRequest request;PlaybackId id=0;std::wstring name;
};
struct Conductor::State:PlaybackSession {
    runtime::Performance* performance=nullptr;bool com=false,audio=false;
    bool observeNotes=false;runtime::Graph* graph=nullptr;NoteObserver* observer=nullptr;
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
const std::vector<DWORD>& Conductor::playback_collection_first_patches() const{return state_->firstPatches;}
void Conductor::enable_note_observation(){if(state_->performance)throw std::runtime_error("Enable note observation before initialization");state_->observeNotes=true;}
std::vector<PlaybackNote> Conductor::observed_notes() const{return state_->observer?state_->observer->snapshot():std::vector<PlaybackNote>{};}
bool Conductor::note_observation_overflow() const{return state_->observer&&state_->observer->overflow();}
bool Conductor::note_observation_forwarding_failed() const{return state_->observer&&state_->observer->forwarding_failed();}
void Conductor::set_default_audio_path(const Bytes& bytes){
    Bytes validated=bytes;if(!validated.empty()){AudioPathDocument config;config.load(validated);}
    defaultAudioPath_.swap(validated);
}
void Conductor::play(const Bytes& bytes,const std::wstring& directory,HWND owner,const std::vector<ResolvedStyle>& styles,const std::vector<ResolvedCollection>& collections,const std::optional<MotifSelection>& motif) {
    // Validate the caller's serialized document before disturbing playback.
    auto snapshot=prepare_style_playback(bytes,styles); // Validate and allocate before Stop.
    snapshot.segment=prepare_command_playback(snapshot.segment);
    snapshot.segment=prepare_transport_audio_path(snapshot.segment,defaultAudioPath_);
    play_snapshot(std::move(snapshot),directory,owner,collections,motif);
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
void Conductor::play_snapshot(StylePlaybackSnapshot snapshot,const std::wstring& directory,HWND owner,const std::vector<ResolvedCollection>& collections,const std::optional<MotifSelection>& motif,const PlaybackOptions& options){
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
        if(!s.com){check("CoInitializeEx",CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED));s.com=true;}
        if(!s.performance){
            servers_={os_server(runtime::performanceClass,L"dmime.dll"),os_server(runtime::loaderClass,L"dmloader.dll")};
            check("CoCreate Performance8",CoCreateInstance(runtime::performanceClass,nullptr,CLSCTX_INPROC_SERVER,runtime::performance8Id,reinterpret_cast<void**>(&s.performance)));
            check("InitAudio",s.performance->InitAudio(nullptr,nullptr,owner,8,16,0,nullptr));s.audio=true;
            check("Add Command notifications",s.performance->AddNotificationType(runtime::commandNotification));
            check("Add Segment notifications",s.performance->AddNotificationType(runtime::segmentNotification));
            if(s.observeNotes){
                servers_.push_back(os_server(runtime::graphClass,L"dmime.dll"));
                check("CoCreate note graph",CoCreateInstance(runtime::graphClass,nullptr,CLSCTX_INPROC_SERVER,runtime::graphId,reinterpret_cast<void**>(&s.graph)));
                s.observer=new NoteObserver;
                check("Insert note observer",s.graph->InsertTool(s.observer,nullptr,0,0));
                check("Set note graph",s.performance->SetGraph(s.graph));
            }
        }
        check("CoCreate Loader8",CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&s.loader)));
        if(!snapshot.styles.empty())servers_.push_back(os_server(runtime::styleClass,L"dmstyle.dll"));
        check("Disable loader cache",s.loader->EnableCache(runtime::allTypes,FALSE));
        if(!directory.empty()){auto search=std::filesystem::absolute(directory).wstring();check("Set reference directory",s.loader->SetSearchDirectory(runtime::allTypes,search.data(),TRUE));}
        s.collections=std::move(collectionSnapshot.collections);s.runtimeCollections.reserve(s.collections.size());if(!s.collections.empty())servers_.push_back(os_server(runtime::collectionClass,L"dmusic.dll"));
        for(auto& dependency:s.collections){runtime::ObjectDesc collectionDescriptor{};collectionDescriptor.size=sizeof(collectionDescriptor);collectionDescriptor.valid=1|2|1024;collectionDescriptor.classId=runtime::collectionClass;std::memcpy(&collectionDescriptor.objectId,dependency.reference.objectId->data(),16);collectionDescriptor.memoryLength=static_cast<LONGLONG>(dependency.bytes.size());collectionDescriptor.memory=dependency.bytes.data();check("Register owned DLS snapshot",s.loader->SetObject(&collectionDescriptor));runtime::Collection* loaded=nullptr;const auto loadedResult=s.loader->GetObject(&collectionDescriptor,runtime::collectionId,reinterpret_cast<void**>(&loaded));if(loaded)s.runtimeCollections.push_back(loaded);check("Load owned DLS snapshot",loadedResult);if(loadedResult!=S_OK||!loaded)throw std::runtime_error("Owned DLS load was partial");DWORD patch=0;WCHAR name[256]{};const auto enumeration=loaded->EnumInstrument(0,&patch,name,256);check("Enum owned DLS first instrument",enumeration);if(enumeration!=S_OK)throw std::runtime_error("Referenced DLS has no instrument");s.firstPatches.push_back(patch);}
        std::function<void(const Chunk&)> verifyInstruments=[&](const Chunk& root){if(root.id=="RIFF"&&root.type=="DMBD"){BandDocument band;band.load(root.encode());for(const auto& instrument:band.instruments())if(!instrument.collectionReference.empty()){const auto reference=collection_reference(instrument.collectionReference);size_t index=0;while(index<s.collections.size()&&s.collections[index].reference.objectId!=reference.objectId)++index;if(index==s.collections.size())throw std::runtime_error("Mapped Band collection missing");runtime::Instrument* selected=nullptr;const auto found=s.runtimeCollections[index]->GetInstrument(instrument.patch,&selected);if(selected)reinterpret_cast<IUnknown*>(selected)->Release();check("Get assigned owned DLS instrument",found);if(found!=S_OK)throw std::runtime_error("Assigned DLS instrument unavailable");}return;}for(const auto& child:root.children)verifyInstruments(child);};if(hasSegment)verifyInstruments(Chunk::parse(snapshot.segment));for(const auto& style:snapshot.styles)verifyInstruments(Chunk::parse(style.bytes));
        s.styles=std::move(snapshot.styles);s.runtimeStyles.reserve(s.styles.size());
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
        s.memory=std::move(snapshot.segment);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);descriptor.valid=2|1024;descriptor.classId=runtime::segmentClass;descriptor.memoryLength=static_cast<LONGLONG>(s.memory.size());descriptor.memory=s.memory.data();
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
            const auto created=s.performance->CreateAudioPath(config,TRUE,&s.audioPath);release(config);check("Create embedded AudioPath",created);
            if(created!=S_OK||!s.audioPath)throw std::runtime_error("Embedded AudioPath creation was incomplete");
            // The performance graph observes remapped channels, not the local
            // PChannels serialized in the Segment. Record the public mapping
            // and reject an unmapped note before downloading any instruments.
            SegmentDocument routed;routed.load(s.memory);std::map<DWORD,DWORD> mapping;
            const auto mapChannel=[&](DWORD local){if(!mapping.count(local)){
                DWORD channel=0;check("Convert embedded AudioPath PChannel",s.audioPath->ConvertPChannel(local,&channel));mapping.emplace(local,channel);
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
void Conductor::stop(){for(const auto id:playback_ids())stop(id);}
void Conductor::stop_current() {
    auto& s=*state_;
    if(s.playing)register_notification_identity();
    if(s.performance&&s.segment){
        check("StopEx",s.performance->StopEx(s.playing?s.playing:static_cast<IUnknown*>(s.segment),0,0));
        const auto deadline=GetTickCount64()+2000;HRESULT playing;
        do {playing=s.performance->IsPlaying(s.segment,s.playing);if(playing!=S_OK)break;Sleep(10);}while(GetTickCount64()<deadline);
        check("IsPlaying after StopEx",playing);
        if(playing==S_OK)check("Stop timeout",HRESULT_FROM_WIN32(ERROR_TIMEOUT));
    }
    collect_notifications();
    release(s.playing);
    // Public notification consumers receive value copies even after release.
    if(s.downloaded){check("Unload segment",s.segment->Unload(s.audioPath?s.audioPath:static_cast<IUnknown*>(s.performance)));s.downloaded=false;}
    release(s.audioPath);release(s.segment);s.motif=false;for(auto& style:s.runtimeStyles)release(style);s.runtimeStyles.clear();for(auto& collection:s.runtimeCollections)release(collection);s.runtimeCollections.clear();release(s.loader);s.styles.clear();s.collections.clear();s.firstPatches.clear();s.memory.clear();s.id=0;s.request={};
}
PlaybackPosition Conductor::position() {
    auto& s=*state_;if(!s.performance||!s.segment||!s.playing)return {};
    const auto playing=s.performance->IsPlaying(s.segment,s.playing);check("IsPlaying",playing);
    PlaybackPosition p{};p.playing=playing==S_OK;check("GetTime",s.performance->GetTime(nullptr,&p.clocks));check("GetStartTime",s.playing->GetStartTime(&p.start));
    if(p.playing){producer::TempoParam tempo{};LONG next=0;const auto hr=s.performance->GetParam(producer::GUID_TempoParam,0xffffffff,0,p.clocks,&next,&tempo);if(s.motif&&(hr==DMUS_E_TRACK_NOT_FOUND||((s.request.flags&runtime::playSecondary)&&hr==DMUS_E_NOT_FOUND))){calls_.push_back({"Motif runtime tempo unavailable",hr});}else{check("Get runtime tempo",hr);p.tempo=tempo.tempo;p.tempoAvailable=true;}}return p;
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
StyleMeter Conductor::segment_meter(LONG time,DWORD groups,DWORD index) {
    if(!state_->segment)throw std::runtime_error("No loaded segment for meter query");
    producer::TimeSignatureParam meter{};LONG next=0;check("Get segment runtime meter",state_->segment->GetParam(producer::GUID_TimeSignatureParam,groups,index,time,&next,&meter));
    return {meter.beats,meter.denominator,meter.grids};
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
    auto cleanup=[&](PlaybackSession& session){
        if(s.performance&&session.segment)log("StopEx cleanup",s.performance->StopEx(session.playing?session.playing:static_cast<IUnknown*>(session.segment),0,0));
        release(session.playing);if(session.downloaded&&session.segment&&s.performance)log("Unload cleanup",session.segment->Unload(session.audioPath?session.audioPath:static_cast<IUnknown*>(s.performance)));session.downloaded=false;
        release(session.audioPath);release(session.segment);for(auto& style:session.runtimeStyles)release(style);session.runtimeStyles.clear();for(auto& collection:session.runtimeCollections)release(collection);session.runtimeCollections.clear();release(session.loader);session.styles.clear();session.collections.clear();session.firstPatches.clear();session.memory.clear();session.id=0;session.request={};
    };
    cleanup(s);for(auto& retained:s.retained)cleanup(*retained);s.retained.clear();
    if(s.performance)log("CloseDown",s.performance->CloseDown());s.audio=false;release(s.performance);
    s.notificationIds.clear();s.pendingNotifications.clear();
    // CloseDown completes the realtime callbacks before releasing the observer.
    release(s.graph);release(s.observer);
    if(s.com){CoUninitialize();s.com=false;}
}
}
