#include "script_dependencies.h"
#include "script_document.h"
#include "document.h"
#include "style.h"
#include "dls.h"
#include "wave_document.h"
#include "audio_path.h"
#include "compat/script_runtime.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>
#include <optional>
#include <exception>
#include <stdexcept>
#include <utility>

namespace producer::app {namespace {
constexpr GUID waveClass={0x8a667154,0xf9cb,0x11d2,{0xad,0x8a,0,0x60,0xb0,0x57,0x5a,0xbc}};
constexpr GUID waveTrackClass={0xeed36461,0x9ea5,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
constexpr GUID markerTrackClass={0x55a8fd00,0x4288,0x11d3,{0x9b,0xd1,0x8a,0x0d,0x61,0xc8,0x88,0x35}};
constexpr GUID audioPathConfigClass={0xee0b9ca0,0xa81e,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
constexpr GUID oldTrack(DWORD first){return {first,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};}
const Chunk* one(const Chunk& c,const char* id){
    const Chunk* result=nullptr;for(const auto& child:c.children)if(child.id==id){if(result)throw std::runtime_error("Ambiguous Script dependency field");result=&child;}return result;
}
std::array<std::uint8_t,16> identity(const Chunk& root){
    const auto guid=one(root,root.type=="DLS "?"dlid":"guid");if(!guid||guid->data.size()!=16||std::all_of(guid->data.begin(),guid->data.end(),[](auto b){return !b;}))throw std::runtime_error("Script nested dependency requires an explicit nonzero GUID");
    std::array<std::uint8_t,16> id{};std::copy_n(guid->data.begin(),16,id.begin());return id;
}
GUID class_id(const Bytes& bytes){if(bytes.size()<16)throw std::runtime_error("Script dependency class truncated");GUID result{};std::memcpy(&result,bytes.data(),16);return result;}
std::array<std::uint8_t,16> class_bytes(REFGUID cls){std::array<std::uint8_t,16> result{};std::memcpy(result.data(),&cls,16);return result;}
const char* form(REFGUID cls){
    if(IsEqualGUID(cls,runtime::segmentClass))return "DMSG";
    if(IsEqualGUID(cls,runtime::styleClass))return "DMST";
    if(IsEqualGUID(cls,runtime::collectionClass))return "DLS ";
    if(IsEqualGUID(cls,waveClass))return "WAVE";
    if(IsEqualGUID(cls,runtime::bandRuntimeClass))return "DMBD";
    throw std::runtime_error("Script object class needs a declared source dependency resolver");
}
void validate_object(const Bytes& bytes,REFGUID cls){
    const auto root=Chunk::parse(bytes);if(root.id!="RIFF"||root.type!=form(cls))throw std::runtime_error("Script dependency class/form mismatch");
    if(IsEqualGUID(cls,runtime::segmentClass)){SegmentDocument d;d.load(bytes);}
    else if(IsEqualGUID(cls,runtime::styleClass)){StyleDocument d;d.load(bytes);}
    else if(IsEqualGUID(cls,runtime::collectionClass)){DlsDocument d;d.load(bytes);d.validate_playback_samples();}
    else if(IsEqualGUID(cls,waveClass)){WaveDocument d;d.load(bytes);}
    else {BandDocument d;d.load(bytes);}
}
std::wstring descriptor_text(const Chunk* field,const char* label,bool required=true,bool allowEmpty=false){
    if(!field){if(required)throw std::runtime_error(std::string("Script descriptor ")+label+" missing");return L"";}
    const auto& b=field->data;if(b.size()<2||b.size()%2||b[b.size()-1]||b[b.size()-2])throw std::runtime_error(std::string("Script descriptor ")+label+" must be terminated UTF16");
    auto value=decode_utf16(b);if((required&&!allowEmpty&&value.empty())||utf16(value)!=b)throw std::runtime_error(std::string("Script descriptor ")+label+" is empty or contains embedded NUL");
    for(size_t i=0;i<value.size();++i){const auto c=static_cast<unsigned>(value[i]);if(c>=0xd800&&c<=0xdbff){if(++i==value.size()||value[i]<0xdc00||value[i]>0xdfff)throw std::runtime_error("Script descriptor has invalid Unicode");}else if(c>=0xdc00&&c<=0xdfff)throw std::runtime_error("Script descriptor has invalid Unicode");}
    return value;
}
const Chunk* metadata_list(const Chunk& root,const char* type){
    const Chunk* found=nullptr;for(const auto& c:root.children)if(c.id=="LIST"&&c.type==type){if(found)throw std::runtime_error("Ambiguous Script descriptor metadata");found=&c;}return found;
}
std::pair<std::wstring,std::wstring> object_labels(const Bytes& bytes){
    const auto root=Chunk::parse(bytes);const auto info=metadata_list(root,"UNFO");
    // Only explicit Unicode metadata is used. ANSI INFO conversion and
    // Loader case/collision policy remain separate compatibility work.
    const auto name=info?descriptor_text(one(*info,"UNAM"),"owned name",false):L"";
    const auto category=descriptor_text(one(root,"catg"),"owned category",false);
    return {name,category};
}
std::uint32_t reference_flags(const Chunk& reference){
    const auto header=one(reference,"refh");
    if(!header||header->data.size()<20)throw std::runtime_error("Script dependency reference header missing");
    const auto flags=read32(header->data,16);
    if(flags&(64|1024|2048))throw std::runtime_error("Script reference URL/serialized memory/stream source is not supported");
    if(!(flags&(1|4|16)))throw std::runtime_error("Script reference needs an owned GUID, name or explicit source filename");
    if(flags&1){const auto guid=one(reference,"guid");if(!guid||guid->data.size()!=16)throw std::runtime_error("Script reference GUID missing or truncated");}
    // DMUS_OBJECTDESC has 64 WCHAR slots for each string, including NUL.
    if(flags&4)if(descriptor_text(one(reference,"name"),"name").size()>=64)throw std::runtime_error("Script descriptor name exceeds SDK capacity");
    if(flags&8)if(descriptor_text(one(reference,"catg"),"category",true,true).size()>=64)throw std::runtime_error("Script descriptor category exceeds SDK capacity");
    if((flags&32)&&!(flags&16))throw std::runtime_error("Script full path requires filename");
    return flags;
}
std::wstring filename(const Chunk& reference){
    const auto flags=reference_flags(reference);const auto file=one(reference,"file");
    if(!(flags&16)||!file)throw std::runtime_error("Script reference needs an explicit source filename");
    const auto& b=file->data;if(b.size()<4||b.size()%2||b[b.size()-1]||b[b.size()-2])throw std::runtime_error("Script reference filename must be terminated UTF16");
    auto name=decode_utf16(b);if(name.empty()||utf16(name)!=b)throw std::runtime_error("Script reference filename is empty or contains embedded NUL");
    for(size_t i=0;i<name.size();++i){const auto c=static_cast<unsigned>(name[i]);if(c>=0xd800&&c<=0xdbff){if(++i==name.size()||name[i]<0xdc00||name[i]>0xdfff)throw std::runtime_error("Script reference filename has invalid Unicode");}else if(c>=0xdc00&&c<=0xdfff)throw std::runtime_error("Script reference filename has invalid Unicode");}return name;
}
std::filesystem::path resolve_path(const Chunk& reference,const std::filesystem::path& directory){
    auto path=std::filesystem::path(filename(reference));if((reference_flags(reference)&32)&&!path.is_absolute())throw std::runtime_error("Script full path must be absolute");if(path.is_relative()){if(directory.empty())throw std::runtime_error("Script reference directory unavailable");path=directory/path;}return std::filesystem::absolute(path).lexically_normal();
}
struct Resolver {
    struct Source {std::array<std::uint8_t,16> classId{};std::optional<std::array<std::uint8_t,16>> id;std::wstring path;std::filesystem::path directory;Bytes bytes;};
    ScriptRuntimeSnapshot result;
    std::vector<Source> catalog;
    struct FileFailure {std::array<std::uint8_t,16> classId;std::wstring path;std::exception_ptr exception;std::string message;};
    std::vector<FileFailure> fileFailures;
    void index_reference_file(const Chunk& reference,const std::filesystem::path& directory,REFGUID cls,size_t depth=0){
        const auto name=filename(reference);const auto candidate=std::filesystem::path(name);
        // A relative fallback has no meaning without its owner's directory.
        // An owned GUID/name can still satisfy it; select() errors if needed.
        if(candidate.is_relative()&&directory.empty())return;
        const auto p=resolve_path(reference,directory);const auto classId=class_bytes(cls);
        if(std::any_of(fileFailures.begin(),fileFailures.end(),[&](const auto& f){return f.classId==classId&&f.path==p.wstring();}))return;
        Bytes bytes;try{bytes=read_file(p.wstring());}
        catch(const InputFileReadError& error){
            if(catalog.size()+fileFailures.size()>=512)throw std::runtime_error("Script dependency graph exceeds implementation bounds");
            fileFailures.push_back({classId,p.wstring(),std::current_exception(),error.what()});return;
        }
        // Parse, format, size, conflicting identity and nested graph failures
        // are not swallowed as an unused-file open error.
        index_object(bytes,cls,p.parent_path(),p.wstring(),depth);
    }
    std::vector<std::pair<std::array<std::uint8_t,16>,std::array<std::uint8_t,16>>> active;
    size_t resolvedCount=0;
    void require(REFGUID cls){const auto server=declared_script_runtime_server(cls);if(std::none_of(result.requirements.begin(),result.requirements.end(),[&](const auto& r){return IsEqualGUID(r.classId,cls);}))result.requirements.push_back({cls,server});}
    void index_object(const Bytes& bytes,REFGUID cls,const std::filesystem::path& directory,const std::wstring& path,size_t depth=0){
        validate_object(bytes,cls);const auto classId=class_bytes(cls);const auto root=Chunk::parse(bytes);std::optional<std::array<std::uint8_t,16>> id;if(one(root,root.type=="DLS "?"dlid":"guid"))id=identity(root);
        for(const auto& prior:catalog){if(prior.classId!=classId)continue;if(!path.empty()&&prior.path==path){if(prior.bytes!=bytes)throw std::runtime_error("Script input changed while indexing source graph");return;}if(id&&prior.id==id&&prior.bytes!=bytes)throw std::runtime_error("Conflicting Script dependency GUID content");}
        if(depth>=64||catalog.size()+fileFailures.size()>=512)throw std::runtime_error("Script dependency graph exceeds implementation bounds");catalog.push_back({classId,id,path,directory,bytes});
        std::function<void(const Chunk&)> scan=[&](const Chunk& node){if(node.id=="LIST"&&node.type=="DMRF"){const auto flags=reference_flags(node);const auto childClass=class_id(one(node,"refh")->data);(void)form(childClass);
            // GUID/name-only edges do not request a file. Defer matching until all
            // explicit files and embedded top-level owners are indexed, so a
            // forward alias/order cannot change source ownership.
            if(flags&16)index_reference_file(node,directory,childClass,depth+1);return;
        }for(const auto& c:node.children)scan(c);};scan(root);
    }
    const Source& select(const Chunk& reference,const std::filesystem::path& directory,REFGUID cls){
        const auto flags=reference_flags(reference);const auto g=one(reference,"guid");ScriptReferenceSelection decision;decision.requestedGuid=(flags&1)!=0;decision.requestedFile=(flags&16)!=0;
        decision.requestedName=(flags&4)!=0;decision.requestedCategory=(flags&8)!=0;
        if(decision.requestedName)decision.name=descriptor_text(one(reference,"name"),"name");
        if(decision.requestedCategory)decision.category=descriptor_text(one(reference,"catg"),"category",true,true);
        std::optional<std::filesystem::path> path;if(decision.requestedFile)decision.filename=filename(reference);
        if(decision.requestedGuid){if(!g||g->data.size()!=16)throw std::runtime_error("Script reference GUID missing or truncated");std::copy_n(g->data.begin(),16,decision.requestedId.begin());}
        const auto classId=class_bytes(cls);const Source* selected=nullptr;
        if(decision.requestedGuid)for(const auto& source:catalog)if(source.classId==classId&&source.id&&*source.id==decision.requestedId){selected=&source;decision.selectedByGuid=true;break;}
        auto selectFile=[&]{if(path)for(const auto& source:catalog)if(source.classId==classId&&source.path==path->wstring()){selected=&source;decision.selectedByFullPath=(flags&32)!=0;break;}};
        if(decision.requestedFile&&(flags&32)){path=resolve_path(reference,directory);if(!selected)selectFile();}
        auto selectName=[&](bool withCategory){
            const Source* match=nullptr;
            for(const auto& source:catalog)if(source.classId==classId){
                const auto labels=object_labels(source.bytes);
                if(labels.first!=decision.name||(withCategory&&labels.second!=decision.category))continue;
                // Do not invent the native Loader's collision/order policy.
                // Repeated copies of the same owned object are harmless.
                if(match&&match->bytes!=source.bytes)throw std::runtime_error("Ambiguous owned Script descriptor name");match=&source;
            }
            if(match){selected=match;decision.selectedByName=true;decision.selectedByCategory=withCategory;}
        };
        if(!selected&&decision.requestedName&&decision.requestedCategory)selectName(true);
        if(!selected&&decision.requestedName)selectName(false);
        if(decision.requestedFile&&!path){
            const auto name=std::filesystem::path(decision.filename);
            if(!selected||!name.is_relative()||!directory.empty())path=resolve_path(reference,directory);
        }
        if(!selected)selectFile();
        if(path)for(const auto& failure:fileFailures)if(failure.classId==classId&&failure.path==path->wstring()){
            if(!selected)std::rethrow_exception(failure.exception);
            decision.fallbackPath=failure.path;decision.fallbackReadError=failure.message;break;
        }
        if(!selected)throw std::runtime_error("Script reference has no owned GUID, name/category or explicit file binding");decision.selectedPath=selected->path;if(selected->id)decision.selectedId=*selected->id;result.selections.push_back(decision);return *selected;
    }
    void transform(Chunk& node,const std::filesystem::path& directory){
        if(node.id=="LIST"&&node.type=="DMRF"){
            const auto header=one(node,"refh");if(!header||header->data.size()<20)throw std::runtime_error("Script dependency reference header missing");const auto cls=class_id(header->data);
            const auto id=resolve(node,directory,cls);
            auto h=node.find("refh");put32(h->data,16,3); // private GUID+CLASS only; retained file/opaque bytes are inactive
            auto guid=node.find("guid");if(!guid){Chunk c;c.id="guid";node.children.push_back(c);guid=&node.children.back();}guid->data.assign(id.begin(),id.end());return;
        }
        if(node.id=="RIFF"){
            if(node.type=="DMTG"||node.type=="DMCN"||node.type=="DMSC")throw std::runtime_error("Nested Script graph/configuration runtime resolver incomplete");
            if(node.type=="DMAP"){
                // Embedded configuration is owned by its Segment. Validate
                // every declared class before the Loader can instantiate it;
                // keep the complete native configuration and effect data.
                AudioPathDocument path;path.load(node.encode());
                if(!path.tool_graph().empty())throw std::runtime_error("Script contained AudioPath ToolGraph runtime integration remains pending");
                require(audioPathConfigClass);
                for(const auto& effect:path.effects()){
                    if(effect.sendBuffer!=AudioBufferId{})throw std::runtime_error("Script contained AudioPath Send runtime integration remains pending");
                    if(!is_declared_os_audio_effect(effect.classId))throw std::runtime_error("Script contained AudioPath effect needs a declared OS factory; original fallback refused");
                    GUID cls{};std::memcpy(&cls,effect.classId.data(),16);require(cls);
                }
            }
            if(node.type=="DMBD")require(runtime::bandRuntimeClass);
        }
        if(node.id=="trkh"){if(node.data.size()<32)throw std::runtime_error("Script Segment track header truncated");require(class_id(node.data));}
        for(auto& child:node.children)transform(child,directory);
    }
    std::array<std::uint8_t,16> resolve(const Chunk& reference,const std::filesystem::path& directory,REFGUID cls){
        (void)form(cls);require(cls);const auto& selected=select(reference,directory,cls);const auto& source=selected.bytes;const auto& spelling=selected.path;auto root=Chunk::parse(source);const auto id=identity(root),classId=class_bytes(cls);const auto key=std::make_pair(classId,id);
        for(const auto& prior:result.dependencies)if(prior.classId==classId&&prior.objectId==id){if(prior.sourceBytes!=source)throw std::runtime_error("Conflicting Script dependency GUID content");if(prior.path==spelling)return id;}
        if(std::find(active.begin(),active.end(),key)!=active.end())throw std::runtime_error("Cyclic Script file dependency graph is not supported");
        if(active.size()>=64||++resolvedCount>512)throw std::runtime_error("Script dependency graph exceeds implementation bounds");
        active.push_back(key);transform(root,selected.directory);active.pop_back();
        // Existing DLS policy materializes inherited Wave WSMP only in the
        // private playback copy; do not fork or weaken that tested contract.
        if(IsEqualGUID(cls,runtime::collectionClass)){DlsDocument d;d.load(root.encode());root=Chunk::parse(d.playback_sample_bytes());}
        const auto runtimeBytes=root.encode();for(const auto& prior:result.dependencies)if(prior.classId==classId&&prior.objectId==id){if(prior.runtimeBytes!=runtimeBytes)throw std::runtime_error("Conflicting Script dependency resolution for shared GUID");return id;}
        result.dependencies.push_back({classId,id,spelling,source,runtimeBytes});return id;
    }
};
}
const wchar_t* declared_script_runtime_server(REFGUID cls){
    if(IsEqualGUID(cls,audioPathConfigClass))return L"dmime.dll";
    if(is_declared_os_audio_effect(class_bytes(cls)))return L"dsdmo.dll";
    if(IsEqualGUID(cls,runtime::segmentClass)||IsEqualGUID(cls,oldTrack(0xd2ac2885))||IsEqualGUID(cls,oldTrack(0xd2ac2886))||IsEqualGUID(cls,oldTrack(0xd2ac2887))||IsEqualGUID(cls,oldTrack(0xd2ac2888))||IsEqualGUID(cls,markerTrackClass)||IsEqualGUID(cls,waveTrackClass))return L"dmime.dll";
    if(IsEqualGUID(cls,runtime::styleClass)||IsEqualGUID(cls,oldTrack(0xd2ac288b))||IsEqualGUID(cls,oldTrack(0xd2ac288c))||IsEqualGUID(cls,oldTrack(0xd2ac288d))||IsEqualGUID(cls,oldTrack(0xd2ac288e))||IsEqualGUID(cls,oldTrack(0xd2ac2897))||IsEqualGUID(cls,oldTrack(0xd2ac2898)))return L"dmstyle.dll";
    if(IsEqualGUID(cls,runtime::bandRuntimeClass)||IsEqualGUID(cls,runtime::bandTrackRuntimeClass))return L"dmband.dll";
    if(IsEqualGUID(cls,runtime::collectionClass))return L"dmusic.dll";
    if(IsEqualGUID(cls,waveClass))return L"dswave.dll";
    if(IsEqualGUID(cls,runtime::containerClass))return L"dmloader.dll";
    throw std::runtime_error("Script runtime class routing needs a declared module");
}
ScriptRuntimeSnapshot prepare_script_runtime(const Bytes& bytes,const std::wstring& directory){
    ScriptDocument document;document.load(bytes);auto root=Chunk::parse(bytes);auto container=root.find("RIFF","DMCN");
    Resolver resolver;resolver.require(runtime::containerClass);const auto objects=document.container().objects();size_t index=0;
    // Index explicit data first, so GUID precedence is independent of alias
    // traversal order. No registry discovery or filesystem search is added.
    for(const auto& object:objects){GUID cls{};std::memcpy(&cls,object.classId.data(),16);(void)form(cls);if(object.reference){if(reference_flags(object.payload)&16)resolver.index_reference_file(object.payload,std::filesystem::path(directory),cls);}else resolver.index_object(object.payload.encode(),cls,std::filesystem::path(directory),L"");}
    for(auto& entry:container->find("LIST","cosl")->children)if(entry.id=="LIST"&&entry.type=="cobl"){
        const auto& object=objects.at(index++);GUID cls{};std::memcpy(&cls,object.classId.data(),16);(void)form(cls);resolver.require(cls);
        auto payload=object.payload;auto base=std::filesystem::path(directory);
        if(object.reference){const auto& selected=resolver.select(payload,base,cls);payload=Chunk::parse(selected.bytes);base=selected.directory;}
        validate_object(payload.encode(),cls);resolver.transform(payload,base);
        auto h=entry.find("cobh");std::copy_n(payload.id.data(),4,h->data.begin()+20);std::copy_n(payload.type.data(),4,h->data.begin()+24);
        for(auto& child:entry.children)if(child.id==object.payload.id&&child.type==object.payload.type){child=std::move(payload);break;}
        // Public grammar places alias before header. Normalize the runtime
        // copy only, including late aliases retained by document readers.
        const auto alias=std::find_if(entry.children.begin(),entry.children.end(),[](const auto& c){return c.id=="coba";});if(alias!=entry.children.end())std::rotate(entry.children.begin(),alias,alias+1);
    }
    resolver.result.script=root.encode();return std::move(resolver.result);
}
}
