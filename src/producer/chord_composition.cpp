#include "chord_composition.h"
#include <dmerror.h>
#include "chord.h"
#include "signpost.h"
#include "native_script_track.h"
#include "style_player_band.h"
#include "command.h"
#include "compat/tempo_runtime.h"
#include "compat/time_signature.h"
#include "tempo/tempo_track.h"
#include <cmath>
#include "compat/playback_runtime.h"
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
namespace producer::app {namespace {
template<class T> struct Releaser {void operator()(T* p)const{if(p)p->Release();}};
template<class T> using Owned=std::unique_ptr<T,Releaser<T>>;
struct ComFailure:std::runtime_error {HRESULT code;ComFailure(const std::string& text,HRESULT hr):std::runtime_error(text),code(hr){}};
void check(const char* operation,HRESULT hr){if(hr!=S_OK){std::ostringstream s;s<<operation<<" failed: 0x"<<std::hex<<static_cast<unsigned long>(hr);throw ComFailure(s.str(),hr);}}
std::wstring server(REFGUID cls,const wchar_t* expected){
    wchar_t id[40]{},value[32768]{},expanded[32768]{},system[MAX_PATH]{},wow[MAX_PATH]{};
    if(!StringFromGUID2(cls,id,40))throw std::runtime_error("Composer CLSID formatting failed");
    const auto key=std::wstring(L"CLSID\\")+id+L"\\InprocServer32";DWORD size=sizeof(value);
    if(RegGetValueW(HKEY_CLASSES_ROOT,key.c_str(),nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,value,&size)!=ERROR_SUCCESS)throw std::runtime_error("Composition runtime registration unavailable");
    const auto n=ExpandEnvironmentStringsW(value,expanded,32768);if(!n||n>32768)throw std::runtime_error("Composition runtime path expansion failed");
    GetSystemDirectoryW(system,MAX_PATH);GetSystemWow64DirectoryW(wow,MAX_PATH);const auto path=std::filesystem::path(expanded).lexically_normal();
    auto lower=[](std::wstring s){std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return s;};
    if(lower(path.filename().wstring())!=lower(expected)||(lower(path.parent_path().wstring())!=lower(system)&&lower(path.parent_path().wstring())!=lower(wow)))throw std::runtime_error("Composition runtime is outside declared Windows system dependencies");return path.wstring();
}
struct Apartment {Apartment(){const auto hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(hr))check("Initialize composition apartment",hr);}~Apartment(){CoUninitialize();}};
Owned<IStream> stream(){IStream* p=nullptr;check("Create composition stream",CreateStreamOnHGlobal(nullptr,TRUE,&p));return Owned<IStream>(p);}
Bytes persist(IUnknown* object){IPersistStream* p=nullptr;check("Query composed Chord persistence",object->QueryInterface(IID_IPersistStream,reinterpret_cast<void**>(&p)));Owned<IPersistStream> owner(p);auto out=stream();check("Save composed Chord track",p->Save(out.get(),FALSE));STATSTG stat{};check("Measure composed Chord stream",out->Stat(&stat,STATFLAG_NONAME));if(stat.cbSize.QuadPart==0||stat.cbSize.QuadPart>64*1024*1024)throw std::runtime_error("Composed Chord stream size invalid");Bytes result(static_cast<size_t>(stat.cbSize.QuadPart));LARGE_INTEGER zero{};check("Seek composed Chord stream",out->Seek(zero,STREAM_SEEK_SET,nullptr));ULONG read=0;check("Read composed Chord stream",out->Read(result.data(),static_cast<ULONG>(result.size()),&read));if(read!=result.size())throw std::runtime_error("Composed Chord stream incomplete");return result;}
GUID track_id(const Chunk& t){const auto h=t.find("trkh");if(!h||h->data.size()<32)throw std::runtime_error("Composition track header incomplete");GUID id{};std::memcpy(&id,h->data.data(),16);return id;}
}
Chunk read_composed_chords(const Bytes& bytes){
    if(bytes.size()<12)throw std::runtime_error("Composed Chord persistence truncated");
    auto result=std::equal(bytes.begin(),bytes.begin()+4,"LIST")?Chunk::parse_list(bytes):Chunk::parse(bytes);
    if(result.id=="RIFF"&&result.type=="DMTK"){const auto cord=result.find("LIST","cord");if(!cord||std::count_if(result.children.begin(),result.children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="cord";})!=1)throw std::runtime_error("Composed Chord payload missing or ambiguous");auto payload=*cord;result=std::move(payload);}
    (void)chord_events(result);return result;
}
ChordComposition compose_chord_track(const Bytes& bytes,std::uint32_t groups,const std::vector<ResolvedStyle>& styles,const std::vector<ResolvedChordMap>& maps,unsigned activity){
    if(!groups||activity>3)throw std::runtime_error("Composition group/activity invalid");auto root=Chunk::parse(bytes);if(root.type!="DMSG")throw std::runtime_error("Composition requires Segment");const auto header=root.find("segh");if(!header||header->data.size()<8||read32(header->data,4)>INT32_MAX||!read32(header->data,4))throw std::runtime_error("Composition Segment length invalid");const auto length=static_cast<LONG>(read32(header->data,4));
    auto tracks=root.find("LIST","trkl");if(!tracks)throw std::runtime_error("Composition template has no tracks");
    const GUID tempo={0xd2ac2885,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}},meter={0xd2ac2888,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}},command={0xd2ac288c,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}},style={0xd2ac288d,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}},sign={0xf17e8672,0xc3b4,0x11d1,{0x87,0xb,0,0x60,8,0x93,0xb1,0xbd}};
    std::vector<Chunk> templateTracks;std::set<Bytes> seen;std::vector<std::pair<GUID,std::wstring>> dependencies;
    for(const auto& t:tracks->children)if(t.id=="RIFF"&&t.type=="DMTK"){
        const auto id=track_id(t);const auto h=t.find("trkh");if(!(read32(h->data,20)&groups))continue;
        const wchar_t* dll=nullptr;if(IsEqualGUID(id,tempo)||IsEqualGUID(id,meter)||IsEqualGUID(id,command))dll=L"dmime.dll";else if(IsEqualGUID(id,style))dll=L"dmstyle.dll";else if(IsEqualGUID(id,sign)||IsEqualGUID(id,runtime::chordMapTrackClass))dll=L"dmcompos.dll";else continue;
        const Bytes identity(h->data.begin(),h->data.begin()+16);if(!seen.insert(identity).second)throw std::runtime_error("Composition needs one track per harmonic role in selected group");auto copy=t;put32(copy.find("trkh")->data,20,1);templateTracks.push_back(std::move(copy));dependencies.push_back({id,dll});
    }
    tracks->children=std::move(templateTracks);
    root.children.erase(std::remove_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id!="segh"&&c.id!="guid"&&c.id!="vers"&&!(c.id=="LIST"&&c.type=="trkl");}),root.children.end());
    tracks=root.find("LIST","trkl");
    const auto refs=style_references(root);const auto mapRefs=chordmap_references(root);
    if(refs.size()!=1||refs[0].time!=0||mapRefs.size()!=1||mapRefs[0].time!=0)throw std::runtime_error("Composition requires one Style and ChordMap at clock zero");
    const Chunk* signs=nullptr;for(const auto& t:tracks->children)if(IsEqualGUID(track_id(t),sign))signs=t.find("sgnp");if(!signs||signpost_events(signs->data).empty())throw std::runtime_error("Composition requires SignPost markers");
    for(const auto& e:signpost_events(signs->data))if(e.time<0||e.time>=length)throw std::runtime_error("Composition SignPost outside Segment");
    std::vector<ResolvedStyle> selectedStyles;for(auto s:styles)if(s.reference.groups&groups){s.reference.groups=1;selectedStyles.push_back(std::move(s));}
    std::vector<ResolvedChordMap> selectedMaps;for(auto m:maps)if(m.reference.groups&groups){m.reference.groups=1;selectedMaps.push_back(std::move(m));}
    auto styleSnapshot=prepare_style_playback(root.encode(),selectedStyles);auto mapSnapshot=prepare_chordmap_playback(styleSnapshot.segment,selectedMaps);auto templateBytes=mapSnapshot.segment;
    // A harmonic template carries only its declared harmonic dependencies.
    // Source-owned unrelated tracks and metadata are never adopted from this copy.
    ChordComposition result;result.servers={server(runtime::loaderClass,L"dmloader.dll"),server(runtime::segmentClass,L"dmime.dll"),server(runtime::composerClass,L"dmcompos.dll"),server(runtime::styleClass,L"dmstyle.dll"),server(runtime::chordMapClass,L"dmcompos.dll")};for(const auto& d:dependencies)result.servers.push_back(server(d.first,d.second.c_str()));
    Apartment apartment;runtime::Loader* lp=nullptr;check("Create composition Loader",CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&lp)));Owned<runtime::Loader> loader(lp);
    std::vector<Owned<IUnknown>> owned;owned.reserve(styleSnapshot.styles.size()+mapSnapshot.maps.size()+1);
    auto load=[&](Bytes& source,REFGUID cls,REFGUID iid,const std::array<std::uint8_t,16>* identity)->IUnknown*{runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=2|1024|(identity?1:0);d.classId=cls;if(identity)std::memcpy(&d.objectId,identity->data(),16);d.memoryLength=source.size();d.memory=source.data();const auto cache=loader->EnableCache(cls,TRUE);if(cache!=S_OK&&cache!=S_FALSE)check("Cache composition class",cache);check("Register composition snapshot",loader->SetObject(&d));IUnknown* raw=nullptr;const auto hr=loader->GetObject(&d,iid,reinterpret_cast<void**>(&raw));if(raw)owned.emplace_back(raw);check("Load composition snapshot",hr);if(!raw)throw std::runtime_error("Composition object absent");return raw;};
    for(auto& s:styleSnapshot.styles)load(s.bytes,runtime::styleClass,runtime::styleId,&s.reference.objectId);
    for(auto& m:mapSnapshot.maps)load(m.bytes,runtime::chordMapClass,runtime::chordMapId,&m.reference.objectId);
    auto input=reinterpret_cast<runtime::Segment*>(load(templateBytes,runtime::segmentClass,runtime::segment8Id,nullptr));runtime::Composer* cp=nullptr;check("Create OS Composer",CoCreateInstance(runtime::composerClass,nullptr,CLSCTX_INPROC_SERVER,runtime::composerId,reinterpret_cast<void**>(&cp)));Owned<runtime::Composer> composer(cp);runtime::Segment* generatedRaw=nullptr;const auto composed=composer->ComposeSegmentFromTemplate(nullptr,input,static_cast<WORD>(activity),nullptr,&generatedRaw);Owned<runtime::Segment> generated(generatedRaw);check("Compose Chords from owned template",composed);if(!generated)throw std::runtime_error("Composer returned no Segment");LONG generatedLength=0;check("Get composed Segment length",generated->GetLength(&generatedLength));if(generatedLength!=length)throw std::runtime_error("Composer changed Segment length");
    GUID chordId{};const auto fresh=chord_track();std::memcpy(&chordId,fresh.find("trkh")->data.data(),16);runtime::Track* rawTrack=nullptr;check("Get composed Chord track",generated->GetTrack(chordId,1,0,&rawTrack));Owned<IUnknown> chord(reinterpret_cast<IUnknown*>(rawTrack));if(!chord)throw std::runtime_error("Composed Chord track missing");result.chords=read_composed_chords(persist(chord.get()));
    const auto events=chord_events(result.chords);if(events.empty())throw std::runtime_error("Composer produced no chords");for(const auto& e:events)if(!valid_chord(e)||e.time>=length)throw std::runtime_error("Composed Chord invalid or outside Segment");return result;
}
}
#include "style_player.h"
#include "chordmap.h"
namespace producer::app {
namespace {
std::vector<ResolvedChordMap> source_style_chordmaps(const StyleCatalogEntry& owned,const std::vector<ChordMapCatalogEntry>& catalog){
    const auto root=Chunk::parse(owned.bytes);auto track=chordmap_reference_track(1);auto list=track.find("LIST","pftr");
    for(const auto& child:root.children)if(child.id=="LIST"&&child.type=="prrf"){
        for(const auto& descriptor:child.children){
            if(descriptor.id!="LIST"||descriptor.type!="DMRF")throw std::runtime_error("Style ChordMap reference array contains unsupported data");
            Chunk reference;reference.id="LIST";reference.type="pfrf";Chunk stamp;stamp.id="stmp";stamp.data=Bytes(4);reference.children={stamp,descriptor};list->children.push_back(std::move(reference));
        }
    }
    return resolve_chordmaps(chordmap_track_references(track),std::filesystem::path(owned.path).parent_path().wstring(),catalog);
}
Bytes prepare_style_default_names(const Bytes& bytes,const std::vector<ResolvedChordMap>& maps){
    auto root=Chunk::parse(bytes);size_t index=0;
    // The declared OS Style ignores prrf entries without a reference name.
    // Hydrate every resolved entry in the private snapshot, then let the
    // public GetDefaultChordMap select its identity. Native bytes stay owned.
    for(auto& child:root.children)if(child.id=="LIST"&&child.type=="prrf")for(auto& descriptor:child.children){
        if(index>=maps.size())throw std::runtime_error("Style default ChordMap snapshot context mismatch");
        const auto& resolved=maps[index++];ChordMapDocument source;source.load(resolved.bytes);
        if(!source.has_object_id()||(resolved.reference.hasId&&resolved.reference.objectId!=source.object_id()))throw std::runtime_error("Style default ChordMap snapshot identity mismatch");
        const auto name=source.name();if(name.empty())throw std::runtime_error("Style default ChordMap needs a public source name");
        if(!resolved.reference.name.empty()&&resolved.reference.name!=name)throw std::runtime_error("Style default ChordMap source name differs");
        auto header=descriptor.find("refh");if(!header||header->data.size()<20)throw std::runtime_error("Style default ChordMap reference header missing");
        put32(header->data,16,read32(header->data,16)|4u);
        auto field=descriptor.find("name");if(field)field->data=utf16(name);else{Chunk value;value.id="name";value.data=utf16(name);descriptor.children.push_back(std::move(value));}
    }
    if(index!=maps.size())throw std::runtime_error("Style default ChordMap snapshot context mismatch");return root.encode();
}
// Source audition explicitly binds its owned default Band. This is a native
// Band selection, not a byte-equivalence claim for Composer's anonymous Band.
Bytes source_default_band(runtime::Style* style,const Bytes& ownedStyle){
    runtime::Band* bandRaw=nullptr;const auto hr=style->GetDefaultBand(&bandRaw);Owned<IUnknown> band(reinterpret_cast<IUnknown*>(bandRaw));check("Get StylePlayer default Band",hr);if(!band)throw std::runtime_error("StylePlayer default Band missing");
    runtime::MusicObject* raw=nullptr;check("Query default Band descriptor",band->QueryInterface(runtime::musicObjectId,reinterpret_cast<void**>(&raw)));Owned<runtime::MusicObject> object(raw);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);check("Get default Band descriptor",object->GetDescriptor(&descriptor));
    if(!(descriptor.valid&1))throw std::runtime_error("Default Band has no source identity");
    StyleDocument source;source.load(ownedStyle);Bytes result;for(const auto& candidate:source.bands()){
        const auto bytes=candidate.save_bytes();const auto root=Chunk::parse(bytes);const auto info=root.find("LIST","UNFO");const auto name=info?info->find("UNAM"):nullptr;if(!name)continue;auto bandName=decode_utf16(name->data);if(bandName.empty())continue;
        const auto guid=root.find("guid");if(!guid||guid->data.size()!=16||std::memcmp(guid->data.data(),&descriptor.objectId,16))continue;
        if((descriptor.valid&4)&&(!std::wmemchr(descriptor.name,0,64)||bandName!=descriptor.name))throw std::runtime_error("Default Band source name differs");
        if(!result.empty())throw std::runtime_error("Default Band source identity ambiguous");result=bytes;
    }
    if(result.empty())throw std::runtime_error("Default Band source bytes missing");return result;
}
template<class T,class F> void generated_events(runtime::Track* track,REFGUID parameter,LONG length,F append){
    LONG time=0;for(unsigned i=0;i<8192;++i){LONG next=0;T value{};check("Read generated track parameter",track->GetParam(parameter,time,&next,&value));append(time,value);if(next==0)return;if(next<0||next>length-time)throw std::runtime_error("Generated parameter next time invalid");time+=next;if(time>=length)return;}throw std::runtime_error("Generated parameter events exceeded bound");
}
Bytes generated_parameter_payload(runtime::Track* track,REFGUID cls,LONG length,const Bytes& ownedStyle,const Bytes& ownedMap){
    const GUID tempo={0xd2ac2885,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}},command={0xd2ac288c,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
    if(IsEqualGUID(cls,tempo)){producer::tempo::Track output;std::vector<producer::tempo::Event> events;generated_events<producer::TempoParam>(track,producer::GUID_TempoParam,length,[&](LONG time,const producer::TempoParam& p){if(p.time!=0||!std::isfinite(p.tempo)||p.tempo<=0)throw std::runtime_error("Generated Tempo parameter invalid");events.push_back({time,p.tempo});});output.replace_events(std::move(events));return output.save();}
    if(IsEqualGUID(cls,producer::CLSID_DirectMusicTimeSignatureTrack)){Chunk payload;payload.id="tims";payload.data=Bytes(4);put32(payload.data,0,8);generated_events<producer::TimeSignatureParam>(track,producer::GUID_TimeSignatureParam,length,[&](LONG time,const producer::TimeSignatureParam& p){if(p.time!=0||!p.beats||!p.denominator||p.denominator>128||(p.denominator&(p.denominator-1))||!p.grids)throw std::runtime_error("Generated meter parameter invalid");const auto at=payload.data.size();payload.data.resize(at+8);put32(payload.data,at,time);payload.data[at+4]=p.beats;payload.data[at+5]=p.denominator;payload.data[at+6]=static_cast<BYTE>(p.grids);payload.data[at+7]=static_cast<BYTE>(p.grids>>8);});return payload.encode();}
    if(IsEqualGUID(cls,command)){StyleDocument style;style.load(ownedStyle);const auto meter=style.meter();const auto beat=3072/meter.denominator,measure=beat*meter.beats;auto output=command_track();auto data=output.find("cmnd");generated_events<runtime::CommandParam2>(track,runtime::commandParam2,length,[&](LONG time,const runtime::CommandParam2& p){if(p.time!=0||time/measure>65535)throw std::runtime_error("Generated Command parameter invalid");data->data=command_insert(data->data,{time,static_cast<WORD>(time/measure),static_cast<BYTE>((time%measure)/beat),p.type,p.groove,p.range,p.repeat});});return data->encode();}
    const GUID styleClass={0xd2ac288d,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
    if(IsEqualGUID(cls,styleClass)){StyleDocument source;source.load(ownedStyle);auto output=style_reference_track(1);generated_events<IUnknown*>(track,producer::GUID_TimeSignatureStyleParam,length,[&](LONG time,IUnknown* raw){Owned<IUnknown> object(raw);if(!object)throw std::runtime_error("Generated Style reference missing");runtime::MusicObject* descriptorRaw=nullptr;check("Query generated Style descriptor",object->QueryInterface(runtime::musicObjectId,reinterpret_cast<void**>(&descriptorRaw)));Owned<runtime::MusicObject> descriptorObject(descriptorRaw);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);check("Read generated Style descriptor",descriptorObject->GetDescriptor(&descriptor));if(!(descriptor.valid&1)||!source.has_object_id()||std::memcmp(source.object_id().data(),&descriptor.objectId,16))throw std::runtime_error("Generated Style source identity differs");StyleReference r{};r.time=time;r.groups=1;r.hasId=true;std::memcpy(r.objectId.data(),&descriptor.objectId,16);if(!insert_style_reference(output,r))throw std::runtime_error("Generated Style reference cannot be represented");});return output.find("LIST","sttr")->encode();}
    if(IsEqualGUID(cls,runtime::chordMapTrackClass)){if(ownedMap.empty())throw std::runtime_error("Generated default ChordMap native ownership remains unresolved");ChordMapDocument source;source.load(ownedMap);auto output=chordmap_reference_track(1);generated_events<IUnknown*>(track,runtime::chordMapParam,length,[&](LONG time,IUnknown* raw){Owned<IUnknown> object(raw);if(!object)throw std::runtime_error("Generated ChordMap reference missing");runtime::MusicObject* descriptorRaw=nullptr;check("Query generated ChordMap descriptor",object->QueryInterface(runtime::musicObjectId,reinterpret_cast<void**>(&descriptorRaw)));Owned<runtime::MusicObject> descriptorObject(descriptorRaw);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);check("Read generated ChordMap descriptor",descriptorObject->GetDescriptor(&descriptor));if(!(descriptor.valid&1)||!source.has_object_id()||std::memcmp(source.object_id().data(),&descriptor.objectId,16))throw std::runtime_error("Generated ChordMap source identity differs");ChordMapReference r{};r.time=time;r.groups=1;r.hasId=true;std::memcpy(r.objectId.data(),&descriptor.objectId,16);if(time!=0)throw std::runtime_error("Multiple generated ChordMap references remain unsupported");if(!set_chordmap_reference(output,r))throw std::runtime_error("Generated ChordMap reference cannot be represented");});return output.find("LIST","pftr")->encode();}
    throw std::runtime_error("Generated track public Save unsupported; no parameter serializer for class");
}
// IDirectMusicSegment::Save is E_NOTIMPL in dmime, so the composed Segment is
// reassembled from every public track. Every reported Band binds to complete
// owned bytes, with an explicit default for an anonymous initial event.
Bytes persist_segment(runtime::Segment* segment,runtime::Style* style,const Bytes& ownedStyle,const Bytes& ownedMap){
    LONG length=0;DWORD repeats=0,resolution=0;check("Get composed length",segment->GetLength(&length));check("Get composed repeats",segment->GetRepeats(&repeats));check("Get composed resolution",segment->GetDefaultResolution(&resolution));
    LONG start=0,loopStart=0,loopEnd=0;check("Get composed start",segment->GetStartPoint(&start));check("Get composed loop",segment->GetLoopPoints(&loopStart,&loopEnd));
    if(length<=0)throw std::runtime_error("Composed Segment length invalid");
    Chunk root;root.id="RIFF";root.type="DMSG";Chunk header;header.id="segh";header.data=Bytes(40);put32(header.data,0,repeats);put32(header.data,4,static_cast<std::uint32_t>(length));put32(header.data,8,start);put32(header.data,12,loopStart);put32(header.data,16,loopEnd);put32(header.data,20,resolution);root.children.push_back(std::move(header));
    Chunk tracks;tracks.id="LIST";tracks.type="trkl";
    GUID identity{};check("Create composed Segment identity",CoCreateGuid(&identity));Chunk guid;guid.id="guid";guid.data.resize(16);std::memcpy(guid.data.data(),&identity,16);root.children.push_back(std::move(guid));
    for(DWORD index=0;;++index){
        if(index==4096)throw std::runtime_error("Composed track enumeration exceeded bound");
        runtime::Track* raw=nullptr;const auto hr=segment->GetTrack(GUID_NULL,0xFFFFFFFFu,index,&raw);Owned<IUnknown> track(reinterpret_cast<IUnknown*>(raw));if(hr==DMUS_E_NOT_FOUND&&!track)break;check("Enumerate composed track",hr);if(!track)throw std::runtime_error("Composed track absent");
        IPersistStream* ps=nullptr;check("Query composed track class",track->QueryInterface(IID_IPersistStream,reinterpret_cast<void**>(&ps)));Owned<IPersistStream> classOwner(ps);GUID cls{};check("Get composed track class",ps->GetClassID(&cls));
        DWORD group=0;check("Get composed track group",segment->GetTrackGroup(raw,&group));Bytes payload;try{if(IsEqualGUID(cls,runtime::bandTrackClass)){auto bands=read_style_player_bands(raw,length,ownedStyle,source_default_band(style,ownedStyle));payload=bands.find("RIFF","DMBT")->encode();}else try{payload=persist(track.get());}catch(const ComFailure& e){if(e.code!=E_NOTIMPL)throw;payload=generated_parameter_payload(raw,cls,length,ownedStyle,ownedMap);}}catch(const std::exception& e){wchar_t id[40]{};StringFromGUID2(cls,id,40);std::wstring w=id;throw std::runtime_error("Composed track "+std::string(w.begin(),w.end())+": "+e.what());}
        Chunk dmtk;dmtk.id="RIFF";dmtk.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);std::memcpy(h.data.data(),&cls,16);put32(h.data,16,index);put32(h.data,20,group);
        if(payload.size()>=12&&(std::equal(payload.begin(),payload.begin()+4,"LIST")||std::equal(payload.begin(),payload.begin()+4,"RIFF")))std::memcpy(h.data.data()+28,payload.data()+8,4);else if(payload.size()>=8)std::memcpy(h.data.data()+24,payload.data(),4);else throw std::runtime_error("Composed track payload too short");
        // Track persistence can be a LIST, RIFF, leaf, or several leaf chunks.
        // Parse inside a RIFF envelope instead of imposing a RIFF root on it.
        Bytes envelope={'R','I','F','F',0,0,0,0,'D','M','T','K'};envelope.insert(envelope.end(),payload.begin(),payload.end());put32(envelope,4,static_cast<std::uint32_t>(envelope.size()-8));const auto parsed=Chunk::parse(envelope);
        dmtk.children.push_back(std::move(h));dmtk.children.insert(dmtk.children.end(),parsed.children.begin(),parsed.children.end());tracks.children.push_back(std::move(dmtk));
    }
    if(tracks.children.empty())throw std::runtime_error("Composed Segment has no persistable tracks");root.children.push_back(std::move(tracks));return root.encode();
}
}
StylePlayerComposition compose_style_player_segment(const StyleCatalogEntry& owned,const std::optional<Bytes>& chordMap,const StylePlayerSettings& settings,const std::vector<ResolvedCollection>& collections,const std::vector<ChordMapCatalogEntry>& catalog){
    if(settings.activity>3||settings.measures==0||static_cast<WORD>(settings.shape)>8)throw std::runtime_error("StylePlayer composition parameters invalid");
    StyleDocument source;source.load(owned.bytes);StyleReference reference{};reference.groups=1;reference.hasId=true;Bytes styleBytes=owned.bytes;
    if(source.has_object_id())reference.objectId=source.object_id();else{GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Playback Style GUID creation failed");std::memcpy(reference.objectId.data(),&id,16);auto root=Chunk::parse(styleBytes);Chunk identity;identity.id="guid";identity.data.assign(reference.objectId.begin(),reference.objectId.end());root.children.push_back(std::move(identity));styleBytes=root.encode();}
    const auto nativeStyleBytes=styleBytes;auto collectionSnapshot=prepare_collection_playback({styleBytes},collections);styleBytes=std::move(collectionSnapshot.documents[0]);
    Bytes mapBytes;std::array<std::uint8_t,16> mapId{};
    if(chordMap){ChordMapDocument d;d.load(*chordMap);mapBytes=*chordMap;if(d.has_object_id())mapId=d.object_id();else{GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Playback ChordMap GUID creation failed");std::memcpy(mapId.data(),&id,16);auto root=Chunk::parse(mapBytes);Chunk identity;identity.id="guid";identity.data.assign(mapId.begin(),mapId.end());root.children.push_back(std::move(identity));mapBytes=root.encode();}}
    auto defaults=chordMap?std::vector<ResolvedChordMap>{}:source_style_chordmaps(owned,catalog);std::wstring mapPath;
    if(!defaults.empty())styleBytes=prepare_style_default_names(styleBytes,defaults);
    StylePlayerComposition result;result.servers={server(runtime::loaderClass,L"dmloader.dll"),server(runtime::segmentClass,L"dmime.dll"),server(runtime::composerClass,L"dmcompos.dll"),server(runtime::styleClass,L"dmstyle.dll")};if(chordMap||!defaults.empty())result.servers.push_back(server(runtime::chordMapClass,L"dmcompos.dll"));
    Apartment apartment;runtime::Loader* lp=nullptr;check("Create StylePlayer Loader",CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&lp)));Owned<runtime::Loader> loader(lp);
    std::vector<Owned<IUnknown>> keep;
    auto load=[&](Bytes& bytes,REFGUID cls,REFGUID iid,const std::array<std::uint8_t,16>& id)->IUnknown*{runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=2|1024|1;d.classId=cls;std::memcpy(&d.objectId,id.data(),16);d.memoryLength=bytes.size();d.memory=bytes.data();const auto cache=loader->EnableCache(cls,TRUE);if(cache!=S_OK&&cache!=S_FALSE)check("Cache StylePlayer class",cache);check("Register StylePlayer snapshot",loader->SetObject(&d));IUnknown* raw=nullptr;const auto hr=loader->GetObject(&d,iid,reinterpret_cast<void**>(&raw));if(raw)keep.emplace_back(raw);check("Load StylePlayer snapshot",hr);if(!raw)throw std::runtime_error("StylePlayer object absent");return raw;};
    for(auto& c:collectionSnapshot.collections)load(c.bytes,runtime::collectionClass,runtime::collectionId,*c.reference.objectId);
    for(auto& d:defaults){ChordMapDocument validation;validation.load(d.bytes);if(!validation.has_object_id())throw std::runtime_error("Style default ChordMap needs source identity");load(d.bytes,runtime::chordMapClass,runtime::chordMapId,validation.object_id());}
    auto style=reinterpret_cast<runtime::Style*>(load(styleBytes,runtime::styleClass,runtime::styleId,reference.objectId));runtime::ChordMap* map=nullptr;
    if(chordMap)map=reinterpret_cast<runtime::ChordMap*>(load(mapBytes,runtime::chordMapClass,runtime::chordMapId,mapId));
    else {
        runtime::ChordMap* raw=nullptr;const auto hr=style->GetDefaultChordMap(&raw);if(raw)keep.emplace_back(reinterpret_cast<IUnknown*>(raw));
        if(hr==S_FALSE&&!raw)throw std::runtime_error("Style has no available default ChordMap; open and select a ChordMap");
        check("Get StylePlayer default ChordMap",hr);if(!raw)throw std::runtime_error("Style default ChordMap missing");map=raw;
        runtime::MusicObject* descriptorRaw=nullptr;check("Query default ChordMap descriptor",map->QueryInterface(runtime::musicObjectId,reinterpret_cast<void**>(&descriptorRaw)));Owned<runtime::MusicObject> descriptorObject(descriptorRaw);runtime::ObjectDesc descriptor{};descriptor.size=sizeof(descriptor);check("Read default ChordMap descriptor",descriptorObject->GetDescriptor(&descriptor));
        if(!(descriptor.valid&1))throw std::runtime_error("Style default ChordMap has no source identity");
        bool found=false;for(const auto& d:defaults){ChordMapDocument validation;validation.load(d.bytes);if(std::memcmp(validation.object_id().data(),&descriptor.objectId,16))continue;if(found&&(mapBytes!=d.bytes||mapPath!=d.path))throw std::runtime_error("Style default ChordMap source ambiguous");found=true;mapBytes=d.bytes;mapPath=d.path;mapId=validation.object_id();}
        if(!found)throw std::runtime_error("Style default ChordMap has no owned source bytes");
    }
    runtime::Composer* cp=nullptr;check("Create OS Composer",CoCreateInstance(runtime::composerClass,nullptr,CLSCTX_INPROC_SERVER,runtime::composerId,reinterpret_cast<void**>(&cp)));Owned<runtime::Composer> composer(cp);
    runtime::Segment* generatedRaw=nullptr;const auto composed=composer->ComposeSegmentFromShape(style,settings.measures,static_cast<WORD>(settings.shape),settings.activity,settings.intro?TRUE:FALSE,settings.end?TRUE:FALSE,map,&generatedRaw);Owned<runtime::Segment> generated(generatedRaw);check("Compose Segment from Shape",composed);if(!generated)throw std::runtime_error("Composer returned no Segment");
    result.segment=persist_segment(generated.get(),style,nativeStyleBytes,mapBytes);
    const auto root=Chunk::parse(result.segment);if(root.type!="DMSG")throw std::runtime_error("Composed StylePlayer persistence is not a Segment");
    const auto refs=style_references(root);if(refs.size()!=1||refs[0].time!=0||!refs[0].hasId||refs[0].objectId!=reference.objectId)throw std::runtime_error("Composed Segment Style reference differs from owned snapshot");
    result.style=ResolvedStyle{refs[0],owned.path,nativeStyleBytes,source.meter()};
    const auto mapRefs=chordmap_references(root);for(const auto& r:mapRefs){if(!r.hasId||r.objectId!=mapId)throw std::runtime_error("Composed Segment ChordMap reference differs from owned snapshot");ResolvedChordMap m;m.reference=r;m.path=mapPath;m.bytes=mapBytes;result.maps.push_back(std::move(m));}
    return result;
}
}
