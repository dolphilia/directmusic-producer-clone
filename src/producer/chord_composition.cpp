#include "chord_composition.h"
#include "chord.h"
#include "signpost.h"
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
void check(const char* operation,HRESULT hr){if(hr!=S_OK){std::ostringstream s;s<<operation<<" failed: 0x"<<std::hex<<static_cast<unsigned long>(hr);throw std::runtime_error(s.str());}}
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
