#include "framework.h"
#include <filesystem>
#include <stdexcept>
#include <exception>
#include <algorithm>
#include <cwctype>

namespace producer::app {
Chunk runtime_update_recovery_record(const std::vector<RuntimeUpdateRecoveryFile>& files){
    Chunk journal;journal.id="RIFF";journal.type="RTUP";
    Chunk version;version.id="vers";version.data.resize(4);put32(version.data,0,2);journal.children.push_back(version);
    for(size_t i=0;i<files.size();++i){const auto& f=files[i];const std::filesystem::path target(f.target);
        if(f.target.empty()||f.target.find(L'\0')!=std::wstring::npos||!target.is_absolute()||target.lexically_normal()!=target)throw std::runtime_error("Invalid recovery target path");
        if(f.target.rfind(L"\\\\?\\",0)==0||f.target.rfind(L"\\\\.\\",0)==0)throw std::runtime_error("Device namespace recovery target rejected");
        for(size_t k=0;k<i;++k)if(CompareStringOrdinal(f.target.c_str(),-1,files[k].target.c_str(),-1,TRUE)==CSTR_EQUAL)throw std::runtime_error("Duplicate recovery target");
        for(const auto& part:target.relative_path()){const auto name=part.wstring();if(name.empty()||name.back()==L'.'||name.back()==L' '||name.find_first_of(L":<>\"|?*")!=std::wstring::npos||std::any_of(name.begin(),name.end(),[](wchar_t c){return c<32;}))throw std::runtime_error("Ambiguous Win32 recovery target path");auto stem=name.substr(0,name.find(L'.'));std::transform(stem.begin(),stem.end(),stem.begin(),[](wchar_t c){return static_cast<wchar_t>(towupper(c));});if(stem==L"CON"||stem==L"PRN"||stem==L"AUX"||stem==L"NUL"||stem==L"CLOCK$"||((stem.rfind(L"COM",0)==0||stem.rfind(L"LPT",0)==0)&&stem.size()==4&&((stem[3]>=L'1'&&stem[3]<=L'9')||stem[3]==L'¹'||stem[3]==L'²'||stem[3]==L'³')))throw std::runtime_error("Reserved device recovery target rejected");}
        if(f.attributes.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE))throw std::runtime_error("Recovery target is not a regular file");
        if(!f.existed&&(!f.before.empty()||f.attributes.dwFileAttributes||f.attributes.ftCreationTime.dwLowDateTime||f.attributes.ftCreationTime.dwHighDateTime||f.attributes.ftLastAccessTime.dwLowDateTime||f.attributes.ftLastAccessTime.dwHighDateTime||f.attributes.ftLastWriteTime.dwLowDateTime||f.attributes.ftLastWriteTime.dwHighDateTime))throw std::runtime_error("Absent recovery target has prior data");
        Chunk item;item.id="LIST";item.type="file";Chunk name;name.id="path";name.data=utf16(f.target);item.children.push_back(name);
        Chunk metadata;metadata.id="info";metadata.data.resize(32);put32(metadata.data,0,f.existed?1:0);put32(metadata.data,4,f.attributes.dwFileAttributes);
        put32(metadata.data,8,f.attributes.ftCreationTime.dwLowDateTime);put32(metadata.data,12,f.attributes.ftCreationTime.dwHighDateTime);
        put32(metadata.data,16,f.attributes.ftLastAccessTime.dwLowDateTime);put32(metadata.data,20,f.attributes.ftLastAccessTime.dwHighDateTime);
        put32(metadata.data,24,f.attributes.ftLastWriteTime.dwLowDateTime);put32(metadata.data,28,f.attributes.ftLastWriteTime.dwHighDateTime);item.children.push_back(metadata);
        Chunk before;before.id="oldb";before.data=f.before;item.children.push_back(before);Chunk after;after.id="newb";after.data=f.after;item.children.push_back(after);journal.children.push_back(item);
    }
    return journal;
}
std::vector<RuntimeUpdateRecoveryFile> parse_runtime_update_recovery_record(const Bytes& bytes){
    auto root=Chunk::parse(bytes);
    if(root.type=="RTUP"&&!root.children.empty()&&root.children[0].id=="vers"&&root.children[0].data==Bytes({3,0,0,0})){
        (void)runtime_update_recovery_origin(bytes);root.children.erase(root.children.begin()+1);root.children[0].data[0]=2;return parse_runtime_update_recovery_record(root.encode());
    }
    if(root.type!="RTUP"||root.children.size()<2||root.children[0].id!="vers"||root.children[0].data!=Bytes({2,0,0,0}))throw std::runtime_error("Unsupported runtime recovery manifest");
    std::vector<RuntimeUpdateRecoveryFile> files;
    for(size_t i=1;i<root.children.size();++i){const auto& c=root.children[i];
        if(c.id!="LIST"||c.type!="file"||c.children.size()!=4||c.children[0].id!="path"||c.children[1].id!="info"||c.children[2].id!="oldb"||c.children[3].id!="newb")throw std::runtime_error("Invalid recovery file fields");
        const auto& info=c.children[1].data;if(info.size()!=32||read32(info,0)>1)throw std::runtime_error("Invalid recovery metadata");
        RuntimeUpdateRecoveryFile f;f.target=decode_utf16(c.children[0].data);f.existed=read32(info,0)!=0;f.before=c.children[2].data;f.after=c.children[3].data;
        f.attributes.dwFileAttributes=read32(info,4);f.attributes.ftCreationTime={read32(info,8),read32(info,12)};f.attributes.ftLastAccessTime={read32(info,16),read32(info,20)};f.attributes.ftLastWriteTime={read32(info,24),read32(info,28)};files.push_back(std::move(f));
    }
    // The internal format is canonical; do not silently ignore extra fields,
    // odd padding, alternate names, or duplicate paths in retained evidence.
    if(runtime_update_recovery_record(files).encode()!=bytes)throw std::runtime_error("Noncanonical recovery manifest");
    return files;
}
Chunk bind_runtime_update_recovery(const std::vector<RuntimeUpdateRecoveryFile>& files,const RuntimeRecoveryOrigin& origin){
    if(files.empty()||origin.sources.size()!=files.size()||origin.projectBytes.empty())throw std::runtime_error("Incomplete recovery origin");
    const auto valid=[](const std::wstring& p){RuntimeUpdateRecoveryFile f;f.target=p;(void)runtime_update_recovery_record({f});};valid(origin.projectPath);valid(origin.outputRoot);
    auto journal=runtime_update_recovery_record(files);journal.children[0].data[0]=3;Chunk context;context.id="LIST";context.type="orig";
    const auto field=[](const char* id,Bytes data){Chunk c;c.id=id;c.data=std::move(data);return c;};context.children.push_back(field("path",utf16(origin.projectPath)));context.children.push_back(field("data",origin.projectBytes));Bytes mode(4);put32(mode,0,origin.configured?1:0);context.children.push_back(field("mode",mode));context.children.push_back(field("root",utf16(origin.outputRoot)));
    for(size_t i=0;i<origin.sources.size();++i){const auto& s=origin.sources[i];valid(s.path);if(s.bytes.empty()||CompareStringOrdinal(s.target.c_str(),-1,files[i].target.c_str(),-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("Recovery source/output binding mismatch");
        if(CompareStringOrdinal(s.path.c_str(),-1,origin.projectPath.c_str(),-1,TRUE)==CSTR_EQUAL)throw std::runtime_error("Project cannot be a recovery source document");for(size_t k=0;k<i;++k)if(CompareStringOrdinal(s.path.c_str(),-1,origin.sources[k].path.c_str(),-1,TRUE)==CSTR_EQUAL)throw std::runtime_error("Duplicate recovery source");
        Chunk item;item.id="LIST";item.type="srce";item.children.push_back(field("path",utf16(s.path)));item.children.push_back(field("data",s.bytes));item.children.push_back(field("dest",utf16(s.target)));context.children.push_back(item);
    }
    journal.children.insert(journal.children.begin()+1,context);return journal;
}
RuntimeRecoveryOrigin runtime_update_recovery_origin(const Bytes& bytes){
    auto root=Chunk::parse(bytes);if(root.type!="RTUP"||root.children.size()<3||root.children[0].id!="vers"||root.children[0].data!=Bytes({3,0,0,0}))throw std::runtime_error("Recovery writes require a Project-bound version3 manifest");
    const auto& context=root.children[1];if(context.id!="LIST"||context.type!="orig"||context.children.size()<5||context.children[0].id!="path"||context.children[1].id!="data"||context.children[2].id!="mode"||context.children[3].id!="root"||context.children[2].data.size()!=4||read32(context.children[2].data,0)>1)throw std::runtime_error("Invalid recovery origin");
    RuntimeRecoveryOrigin origin;origin.projectPath=decode_utf16(context.children[0].data);origin.projectBytes=context.children[1].data;origin.configured=read32(context.children[2].data,0)!=0;origin.outputRoot=decode_utf16(context.children[3].data);
    for(size_t i=4;i<context.children.size();++i){const auto& c=context.children[i];if(c.id!="LIST"||c.type!="srce"||c.children.size()!=3||c.children[0].id!="path"||c.children[1].id!="data"||c.children[2].id!="dest")throw std::runtime_error("Invalid recovery source binding");origin.sources.push_back({decode_utf16(c.children[0].data),c.children[1].data,decode_utf16(c.children[2].data)});}
    root.children.erase(root.children.begin()+1);root.children[0].data[0]=2;const auto files=parse_runtime_update_recovery_record(root.encode());if(bind_runtime_update_recovery(files,origin).encode()!=bytes)throw std::runtime_error("Noncanonical recovery origin");return origin;
}
std::vector<RuntimeRecoveryTarget> inspect_runtime_update_recovery(const Bytes& bytes,const std::vector<std::wstring>& protectedPaths){
    const auto files=parse_runtime_update_recovery_record(bytes);std::vector<RuntimeRecoveryTarget> result;
    for(const auto& file:files){RuntimeRecoveryTarget item{file,RuntimeRecoveryState::Conflict,{}};
        try{const std::filesystem::path target(file.target);auto parent=target.root_path();
            const auto checkDirectory=[](const std::filesystem::path& p){const auto attr=GetFileAttributesW(p.c_str());if(attr==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND)return;throw std::runtime_error("Recovery parent unavailable");}if(!(attr&FILE_ATTRIBUTE_DIRECTORY)||(attr&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Recovery parent is not a plain directory");};
            checkDirectory(parent);for(const auto& part:target.relative_path().parent_path()){parent/=part;checkDirectory(parent);}
            const auto attr=GetFileAttributesW(target.c_str());bool exists=attr!=INVALID_FILE_ATTRIBUTES;
            if(!exists){const auto error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND)throw std::runtime_error("Recovery target unavailable");}
            else if(attr&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_DEVICE))throw std::runtime_error("Recovery target is not a writable regular file");
            for(const auto& protectedPath:protectedPaths){const auto source=std::filesystem::absolute(protectedPath).lexically_normal();if(CompareStringOrdinal(target.c_str(),-1,source.c_str(),-1,TRUE)==CSTR_EQUAL||(exists&&std::filesystem::exists(source)&&std::filesystem::equivalent(target,source)))throw std::runtime_error("Recovery target aliases a protected source");}
            if(!exists){if(!file.existed){item.state=RuntimeRecoveryState::Before;item.reason="new target absent";}else item.reason="prior target missing";}
            else{const auto current=read_file(file.target);if(file.existed&&current==file.before){WIN32_FILE_ATTRIBUTE_DATA actual{};if(!GetFileAttributesExW(target.c_str(),GetFileExInfoStandard,&actual))throw std::runtime_error("Recovery metadata unavailable");const auto sameTime=[](FILETIME a,FILETIME b){return a.dwLowDateTime==b.dwLowDateTime&&a.dwHighDateTime==b.dwHighDateTime;};if(actual.dwFileAttributes==file.attributes.dwFileAttributes&&sameTime(actual.ftCreationTime,file.attributes.ftCreationTime)&&sameTime(actual.ftLastWriteTime,file.attributes.ftLastWriteTime)){item.state=RuntimeRecoveryState::Before;item.reason="prior bytes and metadata retained";}else item.reason="prior bytes but metadata changed";}
                else if(current==file.after){item.state=RuntimeRecoveryState::After;item.reason="expected published bytes";}else item.reason="external bytes conflict";}
        }catch(const std::exception& e){item.state=RuntimeRecoveryState::Conflict;item.reason=e.what();}
        result.push_back(std::move(item));
    }
    return result;
}
std::vector<RuntimeRecoveryTarget> Framework::inspect_runtime_recovery(const std::wstring& journalPath) const {
    if(projectPath_.empty())throw std::runtime_error("Open a saved Project before inspecting recovery");
    std::vector<std::wstring> sources{projectPath_};const auto append=[&](const auto& list){for(const auto& entry:list)if(!entry.path.empty())sources.push_back(entry.path);};append(documents_);append(styles_);append(bands_);append(collections_);append(audioPaths_);
    return inspect_runtime_update_recovery(read_file(journalPath),sources);
}
bool Framework::assign_style_band(size_t segmentIndex,size_t styleIndex,size_t bandIndex,std::int32_t time){
    auto bands=style_document(styleIndex).bands();if(bandIndex>=bands.size())return false;auto band=std::move(bands[bandIndex]);
    std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});
    const auto dependencies=resolve_collections(band.collection_references(),std::filesystem::path(styles_.at(styleIndex).path).parent_path().wstring(),catalog);
    size_t dependency=0;const auto instruments=band.instruments();for(size_t i=0;i<instruments.size();++i)if(!instruments[i].collectionReference.empty()){
        const auto& owned=dependencies.at(dependency++);CollectionReference reference;reference.objectId=collection_identity(owned.bytes);
        if(!reference.objectId){const auto& path=documents_.at(segmentIndex).path;if(path.empty())throw std::runtime_error("Save Segment before copying a Style Band with a filename-only collection");const auto relative=std::filesystem::path(owned.path).lexically_relative(std::filesystem::path(path).parent_path());if(relative.empty()||relative.is_absolute()||*relative.begin()==L"..")throw std::runtime_error("Filename-only collection must be inside the Segment directory");reference.filename=relative.wstring();}
        (void)band.set_collection_reference(i,reference);
    }
    return document(segmentIndex).set_band(time,band.save_bytes());
}
bool Framework::assign_band(size_t segmentIndex,size_t bandIndex,std::int32_t time){
    auto band=band_document(bandIndex);const auto dependencies=band_collections(bandIndex);size_t dependency=0;const auto instruments=band.instruments();for(size_t i=0;i<instruments.size();++i)if(!instruments[i].collectionReference.empty()){
        const auto& owned=dependencies.at(dependency++);CollectionReference reference;reference.objectId=collection_identity(owned.bytes);
        if(!reference.objectId){const auto& path=documents_.at(segmentIndex).path;if(path.empty())throw std::runtime_error("Save Segment before copying a Band with a filename-only collection");const auto relative=std::filesystem::path(owned.path).lexically_relative(std::filesystem::path(path).parent_path());if(relative.empty()||relative.is_absolute()||*relative.begin()==L"..")throw std::runtime_error("Filename-only collection must be inside the Segment directory");reference.filename=relative.wstring();}
        (void)band.set_collection_reference(i,reference);
    }
    return document(segmentIndex).set_band(time,band.save_bytes());
}
namespace {
std::filesystem::path resolve(const std::filesystem::path& directory,const std::wstring& reference) {
    const auto relative=std::filesystem::path(reference);if(relative.is_absolute()||relative.has_root_name())throw std::runtime_error("Project reference must be relative");
    const auto full=(directory/relative).lexically_normal();
    const auto relation=full.lexically_relative(directory);if(relation.empty()||*relation.begin()==L"..")throw std::runtime_error("Project reference escapes directory");return full;
}
std::wstring relative_to(const std::filesystem::path& file,const std::filesystem::path& directory) {
    const auto relative=file.lexically_relative(directory);
    if(relative.empty()||relative.is_absolute()||*relative.begin()==L"..")throw std::runtime_error("Project documents and references must be inside the project directory");return relative.wstring();
}
std::filesystem::path runtime_name(std::filesystem::path p){auto ext=p.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});
            if(ext==L".sgp"||ext==L".sgt")p.replace_extension(L".sgt");else if(ext==L".stp"||ext==L".sty")p.replace_extension(L".sty");else if(ext==L".bnp"||ext==L".bnd")p.replace_extension(L".bnd");else if(ext==L".dlp"||ext==L".dls")p.replace_extension(L".dls");else if(ext==L".aup"||ext==L".aud")p.replace_extension(L".aud");else throw std::runtime_error("Runtime export component is not implemented");return p;}
        void convert_runtime(Chunk& node) {
            // Authoring-only records observed in the bundled design/runtime
            // sample pairs. Unknown chunks retain their bytes and padding.
            const auto design=[&](const Chunk& c){const auto& p=node.type;return (p=="DMSG"&&c.id=="LIST"&&c.type=="sgdl")||
                (p=="DMTK"&&(c.id=="ctdc"||c.id=="psrd"))||(p=="cord"&&c.id=="crdt")||
                ((p=="strf"||p=="lbin")&&c.id=="jzfr")||(p=="DMRF"&&c.id=="date")||
                (p=="DMST"&&c.id=="styu")||(p=="part"&&(c.id=="pptd"||c.id=="pogc"))||
                (p=="pttn"&&(c.id=="ptnu"||c.id=="ppnd"||c.id=="pcsu"||c.id=="pqtz"||c.id=="pvcz"||c.id=="ptlc"||c.id=="pogc"||c.id=="ppfd"||(c.id=="LIST"&&c.type=="pcsl")))||
                (p=="pref"&&((c.id=="LIST"&&c.type=="pprl")||c.id=="cvau"||c.id=="cvsu"||c.id=="pogc"||c.id=="ppfd"))||((p=="DLS "||p=="rgn "||p=="rgn2")&&c.id=="dmpr")||
                (p=="DMAP"&&c.id=="LIST"&&c.type=="papd")||((p=="pcfl"||p=="pchl"||p=="DSFX")&&c.id=="LIST"&&c.type=="UNFO")||(p=="DSFX"&&c.id=="pegd");};
            node.children.erase(std::remove_if(node.children.begin(),node.children.end(),design),node.children.end());
            if(node.type=="DMRF")if(auto file=node.find("file")){const auto filename=decode_utf16(file->data);const auto rewritten=runtime_name(std::filesystem::path(filename)).wstring();if(rewritten!=filename)file->data=utf16(rewritten);}
            for(auto& c:node.children)if(c.container())convert_runtime(c);
        }

Chunk file_reference(const std::wstring& name) {Chunk c;c.id="file";c.data=utf16(name);return c;}
bool runtime_references(const std::wstring& path){const auto extension=std::filesystem::path(path).extension().wstring();return CompareStringOrdinal(extension.c_str(),-1,L".sgt",-1,TRUE)==CSTR_EQUAL||CompareStringOrdinal(extension.c_str(),-1,L".sty",-1,TRUE)==CSTR_EQUAL||CompareStringOrdinal(extension.c_str(),-1,L".bnd",-1,TRUE)==CSTR_EQUAL;}
bool same_path(const std::wstring& a,const std::wstring& b){return !a.empty()&&!b.empty()&&CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;}
// Internal DMPJ journal: retain successful document saves across a bridge save
// and reload. Indices refer to DMPJ file chunks, never native LIST/file order.
std::vector<size_t> pending_metadata(const Chunk& root){
    std::vector<size_t> result;const auto pending=root.find("mupd");if(!pending)return result;
    if(pending->data.size()%4)throw std::runtime_error("Invalid project metadata journal");
    for(size_t offset=0;offset<pending->data.size();offset+=4){const auto index=read32(pending->data,offset);
        if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("Project metadata ownership mismatch");result.push_back(index);}
    return result;
}
Chunk after_document_save(const Chunk& root,const std::optional<size_t>& reference){
    auto next=root;if(!next.find("orig")||!reference)return next;
    const auto pending=pending_metadata(next);
    if(*reference>=next.children.size()||next.children[*reference].id!="file"||*reference>UINT32_MAX)throw std::runtime_error("Project metadata ownership mismatch");
    if(std::find(pending.begin(),pending.end(),*reference)==pending.end()){
        if(!next.find("mupd")){Chunk journal;journal.id="mupd";next.children.push_back(journal);}
        auto& bytes=next.find("mupd")->data;const auto offset=bytes.size();bytes.resize(offset+4);put32(bytes,offset,static_cast<std::uint32_t>(*reference));
    }return next;
}
Bytes style_description(const Chunk& document){
    const auto header=document.find("styh");
    if(!header||header->data.size()<4||!header->data[0]||!header->data[1])throw std::runtime_error("Native Style reference requires a time signature");
    return utf16(std::to_wstring(header->data[0])+L"/"+std::to_wstring(header->data[1]));
}
void refresh_native_metadata(Chunk& entry,const std::filesystem::path& path){
    auto header=entry.find("filh");if(!header||header->data.size()<44)throw std::runtime_error("Saved native document has no valid file metadata");
    const auto document=Chunk::parse(read_file(path.wstring()));const auto object=document.find(document.type=="DLS "?"dlid":"guid");
    if(!object||object->data.size()!=16||!std::equal(object->data.begin(),object->data.end(),header->data.begin()+28))throw std::runtime_error("Saved native document identity changed");
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&attributes)||attributes.nFileSizeHigh)throw std::runtime_error("Cannot read saved native document metadata");
    put32(header->data,16,attributes.ftLastWriteTime.dwLowDateTime);put32(header->data,20,attributes.ftLastWriteTime.dwHighDateTime);put32(header->data,24,attributes.nFileSizeLow);
    if(document.type=="DMST"){
        const auto description=style_description(document);
        if(!entry.find("LIST","UNFO")){Chunk info;info.id="LIST";info.type="UNFO";entry.children.push_back(info);}
        auto info=entry.find("LIST","UNFO");
        if(!info->find("ndsc")){Chunk node;node.id="ndsc";info->children.push_back(node);}
        info->find("ndsc")->data=description;
        StyleDocument saved;saved.load(document.encode());const auto display=saved.name().empty()?path.stem().wstring():saved.name();
        if(!info->find("nnam")){Chunk node;node.id="nnam";info->children.push_back(node);}
        info->find("nnam")->data=utf16(display);
    }else if(document.type=="DMAP"){
        AudioPathDocument saved;saved.load(document.encode());if(!entry.find("LIST","UNFO")){Chunk info;info.id="LIST";info.type="UNFO";entry.children.push_back(info);}auto info=entry.find("LIST","UNFO");if(!info->find("nnam")){Chunk name;name.id="nnam";info->children.push_back(name);}info->find("nnam")->data=utf16(saved.name().empty()?path.stem().wstring():saved.name());
    }
}
Chunk empty_native_project(){
    // Observed New Project: WORD GUID size, GUID bytes, then author UTF-16.
    // Bookmark/component state is optional editor state and is not fabricated.
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create project identity");
    Chunk root;root.id="RIFF";root.type="JAZP";
    Chunk project;project.id="LIST";project.type="proj";
    Chunk identity;identity.id="pjct";identity.data={16,0};
    const auto bytes=reinterpret_cast<const unsigned char*>(&id);identity.data.insert(identity.data.end(),bytes,bytes+sizeof(id));
    const auto author=utf16(L"Producer");identity.data.insert(identity.data.end(),author.begin(),author.end());project.children.push_back(identity);
    Chunk info;info.id="LIST";info.type="UNFO";Chunk directory;directory.id="rdir";directory.data=utf16(L"..\\RuntimeFiles\\");info.children.push_back(directory);project.children.push_back(info);
    Chunk names;names.id="pjpn";project.children.push_back(names);
    Chunk folders;folders.id="LIST";folders.type="rfld";project.children.push_back(folders);
    root.children.push_back(project);return root;
}
Chunk native_document_reference(const std::filesystem::path& path,const std::wstring& reference){
    const auto document=Chunk::parse(read_file(path.wstring()));
    const bool style=document.type=="DMST",band=document.type=="DMBD",collection=document.type=="DLS ",audio=document.type=="DMAP";
    if(document.type!="DMSG"&&!style&&!band&&!collection&&!audio)throw std::runtime_error("New native entries require Segment, Style, Band, DLS or AudioPath documents");
    const auto object=document.find(collection?"dlid":"guid");
    if(!object||object->data.size()!=16)throw std::runtime_error("Native reference requires a document GUID");
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&attributes)||attributes.nFileSizeHigh)
        throw std::runtime_error("Cannot read native document file time and size");
    GUID identity{};if(FAILED(CoCreateGuid(&identity)))throw std::runtime_error("Cannot create native file identity");
    Chunk file;file.id="LIST";file.type="file";
    Chunk name;name.id="name";name.data=utf16(reference);file.children.push_back(name);
    Chunk header;header.id="filh";header.data.resize(44);
    const auto id=reinterpret_cast<const unsigned char*>(&identity);std::copy(id,id+16,header.data.begin());
    put32(header.data,16,attributes.ftLastWriteTime.dwLowDateTime);put32(header.data,20,attributes.ftLastWriteTime.dwHighDateTime);
    put32(header.data,24,attributes.nFileSizeLow);std::copy(object->data.begin(),object->data.end(),header.data.begin()+28);file.children.push_back(header);
    auto display=path.stem().wstring();if(const auto info=document.find("LIST","UNFO"))if(const auto n=info->find("UNAM"))display=decode_utf16(n->data);
    if(collection)if(const auto info=document.find("LIST","INFO"))if(const auto n=info->find("INAM")){
        const auto end=std::find(n->data.begin(),n->data.end(),0);const int length=static_cast<int>(end-n->data.begin());
        if(length){const auto bytes=reinterpret_cast<const char*>(n->data.data());const int count=MultiByteToWideChar(CP_ACP,0,bytes,length,nullptr,0);if(!count)throw std::runtime_error("Cannot decode DLS display name");display.resize(count);if(!MultiByteToWideChar(CP_ACP,0,bytes,length,display.data(),count))throw std::runtime_error("Cannot decode DLS display name");}
    }
    Chunk info;info.id="LIST";info.type="UNFO";
    auto runtime=std::filesystem::path(reference);runtime.replace_extension(style?L".sty":band?L".bnd":collection?L".dls":audio?L".aud":L".sgt");
    Chunk rnam;rnam.id="rnam";rnam.data=utf16(runtime.wstring());info.children.push_back(rnam);
    Chunk nnam;nnam.id="nnam";nnam.data=utf16(display);info.children.push_back(nnam);
    if(style){
        Chunk description;description.id="ndsc";description.data=style_description(document);info.children.push_back(description);
    }
    file.children.push_back(info);
    // Window placement and bookmark state belong to an actual editor session.
    return file;
}
bool references_path(const Bytes& bytes,const std::wstring& owner,const std::wstring& target){
    for(const auto& ref:document_collection_references(bytes))if(!ref.filename.empty()&&!owner.empty()){
        const auto path=(std::filesystem::path(owner).parent_path()/ref.filename).lexically_normal().wstring();
        if(same_path(path,target))return true;
    }return false;
}
}
Framework::Framework() {new_project();}
std::vector<RuntimeRecoverySource> Framework::runtime_recovery_sources() const {
    if(projectPath_.empty()||dirty())throw std::runtime_error("Recovery requires a saved unchanged Project");
    const std::filesystem::path base(projectDirectory_);std::vector<RuntimeRecoverySource> files;
    const auto add=[&](const std::filesystem::path& path){const auto full=std::filesystem::absolute(path).lexically_normal();(void)relative_to(full,base);for(const auto& f:files)if(same_path(f.path,full.wstring()))return;const auto relative=std::filesystem::canonical(full).lexically_relative(std::filesystem::canonical(base));if(relative.empty()||relative.is_absolute()||*relative.begin()==L".."||!std::filesystem::is_regular_file(full))throw std::runtime_error("Recovery source escapes Project directory");files.push_back({full.wstring(),read_file(full.wstring()),{}});};
    for(const auto& entry:projectRoot_.children)if(entry.id=="file")add(resolve(base,decode_utf16(entry.data)));
    for(size_t i=0;i<files.size();++i){const auto bytes=files[i].bytes;const auto owner=std::filesystem::path(files[i].path).parent_path();const auto root=Chunk::parse(bytes);if(root.type=="DMSG")for(const auto& ref:style_references(root))if(!ref.filename.empty())add(resolve(owner,ref.filename));if(root.type=="DMSG"||root.type=="DMST"||root.type=="DMBD")for(const auto& ref:document_collection_references(bytes))if(!ref.filename.empty())add(resolve(owner,ref.filename));}
    return files;
}
std::filesystem::path Framework::runtime_recovery_target(const RuntimeRecoverySource& source,const std::wstring& outputRoot,bool configured) const {
    const auto relative=std::filesystem::path(relative_to(source.path,projectDirectory_));if(!configured)return (std::filesystem::path(outputRoot)/runtime_name(relative)).lexically_normal();
    const auto form=Chunk::parse(source.bytes).type;if(form!="DMSG"&&form!="DMST"&&form!="DMBD"&&form!="DLS "&&form!="DMAP")throw std::runtime_error("Recovery source form unsupported");const auto kind=form=="DMSG"?RuntimeDocumentKind::Segment:form=="DMST"?RuntimeDocumentKind::Style:form=="DMBD"?RuntimeDocumentKind::Band:form=="DLS "?RuntimeDocumentKind::Collection:RuntimeDocumentKind::AudioPath;
    auto name=runtime_name(relative).wstring();bool found=false;const auto owner=[&](const auto& list){for(size_t i=0;i<list.size();++i)if(same_path(list[i].path,source.path)){if(found)throw std::runtime_error("Ambiguous recovery source owner");name=runtime_filename(kind,i);found=true;}};
    switch(kind){case RuntimeDocumentKind::Segment:owner(documents_);break;case RuntimeDocumentKind::Style:owner(styles_);break;case RuntimeDocumentKind::Band:owner(bands_);break;case RuntimeDocumentKind::Collection:owner(collections_);break;case RuntimeDocumentKind::AudioPath:owner(audioPaths_);break;}
    return std::filesystem::absolute(std::filesystem::path(projectDirectory_)/runtime_component_folder(kind)/name).lexically_normal();
}
std::vector<RuntimeRecoveryTarget> Framework::validate_runtime_recovery(const std::wstring& journalPath,const std::wstring& expectedOutputRoot,bool configured) const {
    const auto bytes=read_file(journalPath);const auto origin=runtime_update_recovery_origin(bytes);const auto files=parse_runtime_update_recovery_record(bytes);const auto root=std::filesystem::absolute(expectedOutputRoot).lexically_normal().wstring();const auto sources=runtime_recovery_sources();
    if(!same_path(origin.projectPath,projectPath_)||origin.projectBytes!=read_file(projectPath_)||origin.configured!=configured||!same_path(origin.outputRoot,root)||sources.size()!=origin.sources.size())throw std::runtime_error("Recovery origin does not match this Project and output scope");
    std::vector<std::wstring> protectedPaths{projectPath_,std::filesystem::absolute(journalPath).lexically_normal().wstring()};
    for(size_t i=0;i<origin.sources.size();++i){const auto& source=origin.sources[i];const RuntimeRecoverySource* match=nullptr;for(const auto& s:sources)if(same_path(s.path,source.path))match=&s;if(!match||match->bytes!=source.bytes||!same_path(runtime_recovery_target(*match,root,configured).wstring(),files[i].target))throw std::runtime_error("Recovery source or allowed target mismatch");protectedPaths.push_back(source.path);}
    if(read_file(journalPath)!=bytes||read_file(projectPath_)!=origin.projectBytes)throw std::runtime_error("Recovery inputs changed");
    for(const auto& s:origin.sources)if(read_file(s.path)!=s.bytes)throw std::runtime_error("Recovery source changed");
    return inspect_runtime_update_recovery(bytes,protectedPaths);
}
void Framework::recover_runtime_update(const std::wstring& journalPath,const std::wstring& expectedOutputRoot,bool configured) const {
    const auto bytes=read_file(journalPath);const auto files=parse_runtime_update_recovery_record(bytes);
    const auto validate=[&]{if(read_file(journalPath)!=bytes)throw std::runtime_error("Recovery inputs changed");auto states=validate_runtime_recovery(journalPath,expectedOutputRoot,configured);if(read_file(journalPath)!=bytes)throw std::runtime_error("Recovery inputs changed");for(const auto& s:states)if(s.state==RuntimeRecoveryState::Conflict)throw std::runtime_error("Recovery conflict; no further outputs restored: "+s.reason);return states;};
    (void)validate();
    for(size_t i=0;i<files.size();++i){const auto states=validate();if(states[i].state==RuntimeRecoveryState::Before)continue;const auto& f=files[i];
        if(f.existed){write_file_atomic(f.target,f.before);const auto handle=CreateFileW(f.target.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(handle==INVALID_HANDLE_VALUE)throw std::runtime_error("Recovery metadata handle unavailable; journal retained");const auto ok=SetFileTime(handle,&f.attributes.ftCreationTime,&f.attributes.ftLastAccessTime,&f.attributes.ftLastWriteTime);CloseHandle(handle);if(!ok||!SetFileAttributesW(f.target.c_str(),f.attributes.dwFileAttributes))throw std::runtime_error("Recovery metadata restore failed; journal retained");}
        else if(!std::filesystem::remove(f.target))throw std::runtime_error("Recovery new output removal failed; journal retained");
    }
    const auto final=validate();for(const auto& s:final)if(s.state!=RuntimeRecoveryState::Before)throw std::runtime_error("Recovery verification failed; journal retained");
    // Retain the journal and staging tree as evidence. Directory ownership is
    // not inferred from absence before a crash; no recursive cleanup here.
}
void Framework::new_project() {
    documents_.clear();styles_.clear();bands_.clear();collections_.clear();audioPaths_.clear();warnings_.clear();name_=L"Untitled";projectPath_.clear();projectDirectory_.clear();projectDirty_=true;
    projectRoot_={};projectRoot_.id="RIFF";projectRoot_.type="DMPJ";
    Chunk v;v.id="vers";v.data={1,0,0,0};projectRoot_.children.push_back(v);
    Chunk n;n.id="name";n.data=utf16(name_);projectRoot_.children.push_back(n);
}
size_t Framework::new_segment() {documents_.push_back({L"",components_.create_document("DMSG"),std::nullopt});projectDirty_=true;return documents_.size()-1;}
size_t Framework::open_segment(const std::wstring& path) {
    const auto full=std::filesystem::absolute(path).lexically_normal().wstring();
    for(size_t i=0;i<documents_.size();++i)if(documents_[i].path==full)return i;
    auto d=components_.create_document("DMSG");d->load(read_file(full));
    try{d->resolve_style_context(std::filesystem::path(full).parent_path().wstring(),style_catalog(),runtime_references(full));}
    catch(const std::exception& e){const std::string message=e.what();warnings_.push_back(L"Style resolution failed; musical coordinates are unavailable: "+std::wstring(message.begin(),message.end()));}
    documents_.push_back({full,std::move(d),std::nullopt});projectDirty_=true;return documents_.size()-1;
}
void Framework::save_segment(size_t index,const std::wstring& path) {
    const auto full=std::filesystem::absolute(path).lexically_normal().wstring();
    for(size_t i=0;i<documents_.size();++i)if(i!=index&&same_path(documents_[i].path,full))throw std::runtime_error("Another document owns this destination");
    for(const auto& style:styles_)if(same_path(style.path,full))throw std::runtime_error("Style document owns this destination");
    for(const auto& band:bands_)if(CompareStringOrdinal(band.path.c_str(),-1,full.c_str(),-1,TRUE)==CSTR_EQUAL)throw std::runtime_error("Band document owns this destination");
    for(const auto& collection:collections_)if(CompareStringOrdinal(collection.path.c_str(),-1,full.c_str(),-1,TRUE)==CSTR_EQUAL)throw std::runtime_error("Collection owns this destination");
    for(const auto& audio:audioPaths_)if(same_path(audio.path,full))throw std::runtime_error("AudioPath owns destination");
    auto& d=documents_.at(index);auto next=*d.document;
    auto destination=full;
    if(!same_path(d.path,full)){
        const auto directory=std::filesystem::path(full).parent_path().wstring();
        if(d.path.empty())next.resolve_style_context(directory,style_catalog());
        else {std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});next.relocate_context(std::filesystem::path(d.path).parent_path().wstring(),directory,style_catalog(),catalog);}
    }
    auto nextProject=after_document_save(projectRoot_,d.projectReference);const bool metadataDirty=nextProject.find("mupd")&&!nextProject.find("mupd")->data.empty();
    next.save(full);*d.document=std::move(next);projectRoot_=std::move(nextProject);if(!same_path(d.path,full)||metadataDirty)projectDirty_=true;d.path=std::move(destination);
}
std::vector<StyleCatalogEntry> Framework::style_catalog() const {std::vector<StyleCatalogEntry> catalog;for(const auto& style:styles_)if(!style.path.empty())catalog.push_back({style.path,style.document->save_bytes()});return catalog;}
void Framework::refresh_styles() {
    const auto catalog=style_catalog();std::vector<SegmentDocument> next;next.reserve(documents_.size());
    for(const auto& owned:documents_){next.push_back(*owned.document);if(owned.path.empty())continue;try{next.back().resolve_style_context(std::filesystem::path(owned.path).parent_path().wstring(),catalog,runtime_references(owned.path));}catch(const std::exception& e){const std::string message=e.what();const auto warning=L"Style context unresolved: "+std::wstring(message.begin(),message.end());if(std::find(warnings_.begin(),warnings_.end(),warning)==warnings_.end())warnings_.push_back(warning);}}
    for(size_t i=0;i<documents_.size();++i)*documents_[i].document=std::move(next[i]);
}
size_t Framework::new_style(){styles_.push_back({L"",components_.create_style_document("DMST"),std::nullopt});projectDirty_=true;return styles_.size()-1;}
size_t Framework::new_collection(){collections_.push_back({L"",DlsDocument::create(),std::nullopt});projectDirty_=true;return collections_.size()-1;}
size_t Framework::open_collection(const std::wstring& path){const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_collection_path(full))throw std::runtime_error("Open collection as .dls or .dlp");for(size_t i=0;i<collections_.size();++i)if(CompareStringOrdinal(collections_[i].path.c_str(),-1,full.c_str(),-1,TRUE)==CSTR_EQUAL)return i;auto bytes=read_file(full);(void)collection_identity(bytes);DlsDocument document;document.load(bytes);collections_.push_back({full,std::move(document),std::nullopt});projectDirty_=true;return collections_.size()-1;}
void Framework::save_collection(size_t index,const std::wstring& path){
    const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_collection_path(full))throw std::runtime_error("Save collection as .dls or .dlp");
    const auto same=[&](const std::wstring& p){return same_path(p,full);};
    for(size_t i=0;i<collections_.size();++i)if(i!=index&&same(collections_[i].path))throw std::runtime_error("Another collection owns destination");
    for(const auto& d:documents_)if(same(d.path))throw std::runtime_error("Segment owns destination");for(const auto& d:styles_)if(same(d.path))throw std::runtime_error("Style owns destination");for(const auto& d:bands_)if(same(d.path))throw std::runtime_error("Band owns destination");
    for(const auto& audio:audioPaths_)if(same_path(audio.path,full))throw std::runtime_error("AudioPath owns destination");
    auto& owned=collections_.at(index);auto nextCollection=owned;nextCollection.path=full;
    std::vector<BandDocument> nextBands;std::vector<StyleDocument> nextStyles;std::vector<SegmentDocument> nextSegments;
    std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});
    const auto dependencies=[&](const Bytes& bytes,const std::wstring& owner){
        auto result=resolve_collections(document_collection_references(bytes),std::filesystem::path(owner).parent_path().wstring(),catalog);
        for(auto& ref:result)if(same_path(ref.path,owned.path))ref.path=full;return result;
    };
    for(const auto& band:bands_){nextBands.push_back(*band.document);if(!same_path(owned.path,full)&&references_path(band.document->save_bytes(),band.path,owned.path))nextBands.back().relocate_collections(dependencies(band.document->save_bytes(),band.path),std::filesystem::path(band.path).parent_path().wstring());}
    bool stylesChanged=false;
    for(const auto& style:styles_){nextStyles.push_back(*style.document);if(!same_path(owned.path,full)&&references_path(style.document->save_bytes(),style.path,owned.path))stylesChanged=nextStyles.back().relocate_collections(dependencies(style.document->save_bytes(),style.path),std::filesystem::path(style.path).parent_path().wstring())||stylesChanged;}
    auto nextStyleCatalog=style_catalog();for(size_t i=0;i<styles_.size();++i)for(auto& entry:nextStyleCatalog)if(same_path(entry.path,styles_[i].path))entry.bytes=nextStyles[i].save_bytes();
    for(const auto& segment:documents_){nextSegments.push_back(*segment.document);if(!same_path(owned.path,full)&&references_path(segment.document->save_bytes(),segment.path,owned.path))nextSegments.back().relocate_collections(dependencies(segment.document->save_bytes(),segment.path),std::filesystem::path(segment.path).parent_path().wstring());
        if(stylesChanged&&!segment.document->styles().empty())nextSegments.back().resolve_style_context(std::filesystem::path(segment.path).parent_path().wstring(),nextStyleCatalog);
    }
    // All reference validation/history preparation precedes the only file write.
    // Save dependent documents explicitly before Save Project; never silently
    // overwrite their files as a side effect of saving a collection.
    auto nextProject=after_document_save(projectRoot_,owned.projectReference);const bool metadataDirty=nextProject.find("mupd")&&!nextProject.find("mupd")->data.empty();
    nextCollection.document.save(full);projectRoot_=std::move(nextProject);
    for(size_t i=0;i<bands_.size();++i)*bands_[i].document=std::move(nextBands[i]);for(size_t i=0;i<styles_.size();++i)*styles_[i].document=std::move(nextStyles[i]);for(size_t i=0;i<documents_.size();++i)*documents_[i].document=std::move(nextSegments[i]);
    if(owned.path!=full||metadataDirty)projectDirty_=true;owned=std::move(nextCollection);
}
std::vector<ResolvedCollection> Framework::band_collections(size_t index) const{std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});const auto& band=bands_.at(index);return resolve_collections(band.document->collection_references(),std::filesystem::path(band.path).parent_path().wstring(),catalog,runtime_references(band.path));}
bool Framework::set_band_collection(size_t index,size_t instrument,size_t collection){const auto& owned=bands_.at(index);if(owned.path.empty())throw std::runtime_error("Save Band before assigning a project-relative collection");const auto& target=collections_.at(collection);CollectionReference reference{relative_to(target.path,std::filesystem::path(owned.path).parent_path()),collection_identity(target.document.save_bytes())};auto next=*owned.document;if(!next.set_collection_reference(instrument,reference))return false;std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});(void)resolve_collections(next.collection_references(),std::filesystem::path(owned.path).parent_path().wstring(),catalog);*bands_.at(index).document=std::move(next);return true;}
bool Framework::set_band_collection_instrument(size_t index,size_t instrument,size_t collection,size_t dlsIndex){
    const auto& owned=bands_.at(index);if(owned.path.empty())throw std::runtime_error("Save Band before assigning a project-relative collection");
    const auto& target=collections_.at(collection);const auto locale=target.document.instruments().at(dlsIndex);
    CollectionReference reference{relative_to(target.path,std::filesystem::path(owned.path).parent_path()),collection_identity(target.document.save_bytes())};
    auto next=*owned.document;if(!next.set_dls_instrument(instrument,reference,locale.bank,locale.program))return false;
    std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});
    (void)resolve_collections(next.collection_references(),std::filesystem::path(owned.path).parent_path().wstring(),catalog);*bands_.at(index).document=std::move(next);return true;
}
std::vector<ResolvedCollection> Framework::playback_collections(size_t index) const{std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});const auto& owned=documents_.at(index);auto result=resolve_collections(document_collection_references(owned.document->save_bytes()),std::filesystem::path(owned.path).parent_path().wstring(),catalog,runtime_references(owned.path));for(const auto& style:owned.document->styles()){auto dependencies=resolve_collections(document_collection_references(style.bytes),std::filesystem::path(style.path).parent_path().wstring(),catalog,runtime_references(style.path));result.insert(result.end(),dependencies.begin(),dependencies.end());}return result;}
StyleCatalogEntry Framework::style_playback_snapshot(size_t index) const{const auto& owned=styles_.at(index);return {owned.path,owned.document->save_bytes()};}
std::vector<ResolvedCollection> Framework::style_playback_collections(size_t index) const{std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});const auto owned=style_playback_snapshot(index);return resolve_collections(document_collection_references(owned.bytes),std::filesystem::path(owned.path).parent_path().wstring(),catalog,runtime_references(owned.path));}
bool Framework::assign_audio_path(size_t segmentIndex,size_t audioPathIndex){return document(segmentIndex).set_audio_path(audio_path_document(audioPathIndex).save_bytes());}
size_t Framework::new_audio_path(){audioPaths_.push_back({L"",components_.create_audio_path_document("DMAP"),std::nullopt});projectDirty_=true;return audioPaths_.size()-1;}
size_t Framework::open_audio_path(const std::wstring& path){const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_audio_path(full))throw std::runtime_error("Open AudioPath as .aup or .aud");for(size_t i=0;i<audioPaths_.size();++i)if(same_path(audioPaths_[i].path,full))return i;auto document=components_.create_audio_path_document("DMAP");document->load(read_file(full));audioPaths_.push_back({full,std::move(document),std::nullopt});projectDirty_=true;return audioPaths_.size()-1;}
void Framework::save_audio_path(size_t index,const std::wstring& path){const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_audio_path(full))throw std::runtime_error("Save AudioPath as .aup or .aud");for(size_t i=0;i<audioPaths_.size();++i)if(i!=index&&same_path(audioPaths_[i].path,full))throw std::runtime_error("Another AudioPath owns destination");for(const auto& d:documents_)if(same_path(d.path,full))throw std::runtime_error("Segment owns destination");for(const auto& d:styles_)if(same_path(d.path,full))throw std::runtime_error("Style owns destination");for(const auto& d:bands_)if(same_path(d.path,full))throw std::runtime_error("Band owns destination");for(const auto& d:collections_)if(same_path(d.path,full))throw std::runtime_error("Collection owns destination");auto& owned=audioPaths_.at(index);auto next=*owned.document;auto project=after_document_save(projectRoot_,owned.projectReference);const bool metadataDirty=project.find("mupd")&&!project.find("mupd")->data.empty();next.save(full);*owned.document=std::move(next);projectRoot_=std::move(project);if(owned.path!=full||metadataDirty)projectDirty_=true;owned.path=full;}
size_t Framework::new_band(){bands_.push_back({L"",components_.create_band_document("DMBD"),std::nullopt});projectDirty_=true;return bands_.size()-1;}
size_t Framework::open_band(const std::wstring& path){const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_band_path(full))throw std::runtime_error("Open Band as .bnp or .bnd");for(size_t i=0;i<bands_.size();++i)if(CompareStringOrdinal(bands_[i].path.c_str(),-1,full.c_str(),-1,TRUE)==CSTR_EQUAL)return i;auto band=components_.create_band_document("DMBD");band->load(read_file(full));bands_.push_back({full,std::move(band),std::nullopt});projectDirty_=true;return bands_.size()-1;}
void Framework::save_band(size_t index,const std::wstring& path){
    auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_band_path(full))throw std::runtime_error("Save Band as .bnp or .bnd");const auto same=[&](const std::wstring& p){return same_path(p,full);};
    for(size_t i=0;i<bands_.size();++i)if(i!=index&&same(bands_[i].path))throw std::runtime_error("Another Band owns this destination");for(const auto& d:documents_)if(same(d.path))throw std::runtime_error("Segment owns this destination");for(const auto& d:styles_)if(same(d.path))throw std::runtime_error("Style owns this destination");for(const auto& d:collections_)if(same(d.path))throw std::runtime_error("Collection owns this destination");
    for(const auto& audio:audioPaths_)if(same_path(audio.path,full))throw std::runtime_error("AudioPath owns destination");
    auto& owned=bands_.at(index);auto next=*owned.document;if(!same_path(owned.path,full)&&!next.collection_references().empty())next.relocate_collections(band_collections(index),std::filesystem::path(full).parent_path().wstring());
    auto nextProject=after_document_save(projectRoot_,owned.projectReference);const bool metadataDirty=nextProject.find("mupd")&&!nextProject.find("mupd")->data.empty();
    next.save(full);*owned.document=std::move(next);projectRoot_=std::move(nextProject);if(owned.path!=full||metadataDirty)projectDirty_=true;owned.path=std::move(full);
}
bool Framework::set_band_instrument(size_t index,size_t instrument,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){return bands_.at(index).document->set_instrument(instrument,patch,pchannel,pan,volume);}
bool Framework::undo_band(size_t index){return bands_.at(index).document->undo();}
bool Framework::redo_band(size_t index){return bands_.at(index).document->redo();}
bool Framework::add_band_gm_instrument(size_t index,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){return bands_.at(index).document->add_gm_instrument(patch,pchannel,pan,volume);}
size_t Framework::open_style(const std::wstring& path){const auto full=std::filesystem::absolute(path).lexically_normal().wstring();for(size_t i=0;i<styles_.size();++i)if(CompareStringOrdinal(styles_[i].path.c_str(),-1,full.c_str(),-1,TRUE)==CSTR_EQUAL)return i;auto style=components_.create_style_document("DMST");style->load(read_file(full));styles_.push_back({full,std::move(style),std::nullopt});projectDirty_=true;refresh_styles();return styles_.size()-1;}
void Framework::save_style(size_t index,const std::wstring& path){
    const auto full=std::filesystem::absolute(path).lexically_normal().wstring();if(!components_.is_style_path(full))throw std::runtime_error("Save Style as .stp or .sty");for(size_t i=0;i<styles_.size();++i)if(i!=index&&same_path(styles_[i].path,full))throw std::runtime_error("Another Style owns this destination");for(const auto& d:documents_)if(same_path(d.path,full))throw std::runtime_error("Segment owns this destination");for(const auto& band:bands_)if(same_path(band.path,full))throw std::runtime_error("Band owns this destination");for(const auto& c:collections_)if(same_path(c.path,full))throw std::runtime_error("Collection owns this destination");
    for(const auto& audio:audioPaths_)if(same_path(audio.path,full))throw std::runtime_error("AudioPath owns destination");
    auto& owned=styles_.at(index);auto next=*owned.document;auto destination=full;const bool moved=!same_path(owned.path,full);const auto oldBytes=owned.document->save_bytes();
    if(moved&&!owned.path.empty()){std::vector<CollectionEntry> collections;for(const auto& c:collections_)collections.push_back({c.path,c.document.save_bytes()});next.relocate_context(std::filesystem::path(owned.path).parent_path().wstring(),std::filesystem::path(full).parent_path().wstring(),collections);}
    auto catalog=style_catalog();bool catalogued=false;for(auto& entry:catalog)if(same_path(entry.path,owned.path)){entry={full,next.save_bytes()};catalogued=true;}if(!catalogued)catalog.push_back({full,next.save_bytes()});
    std::vector<SegmentDocument> segments;segments.reserve(documents_.size());auto warnings=warnings_;
    for(const auto& segment:documents_){segments.push_back(*segment.document);if(segment.path.empty())continue;const auto directory=std::filesystem::path(segment.path).parent_path().wstring();
        if(moved&&!owned.path.empty())segments.back().retarget_style(directory,owned.path,full,oldBytes);
        try{segments.back().resolve_style_context(directory,catalog);}catch(const std::exception& e){if(!segment.document->styles().empty())throw;const std::string message=e.what();const auto warning=L"Style context unresolved: "+std::wstring(message.begin(),message.end());if(std::find(warnings.begin(),warnings.end(),warning)==warnings.end())warnings.push_back(warning);}
    }
    // Prepare history, references and known cache contexts before the file write.
    auto nextProject=after_document_save(projectRoot_,owned.projectReference);const bool metadataDirty=nextProject.find("mupd")&&!nextProject.find("mupd")->data.empty();
    next.save(full);*owned.document=std::move(next);projectRoot_=std::move(nextProject);owned.path=std::move(destination);for(size_t i=0;i<documents_.size();++i)*documents_[i].document=std::move(segments[i]);warnings_=std::move(warnings);if(moved||metadataDirty)projectDirty_=true;
}
void Framework::apply_style_edit(size_t index,StyleDocument style){
    const auto& ownedStyle=styles_.at(index);auto catalog=style_catalog();for(auto& entry:catalog)if(entry.path==ownedStyle.path)entry.bytes=style.save_bytes();
    std::vector<SegmentDocument> next;next.reserve(documents_.size());auto warnings=warnings_;
    for(const auto& owned:documents_){next.push_back(*owned.document);if(owned.path.empty())continue;try{next.back().resolve_style_context(std::filesystem::path(owned.path).parent_path().wstring(),catalog,runtime_references(owned.path));}catch(const std::exception& e){
        // A known context must remain coherent with the edit. Previously
        // unresolved documents may stay unresolved, without blocking others.
        if(!owned.document->styles().empty())throw;
        const std::string message=e.what();const auto warning=L"Style context unresolved: "+std::wstring(message.begin(),message.end());if(std::find(warnings.begin(),warnings.end(),warning)==warnings.end())warnings.push_back(warning);
    }}
    *styles_[index].document=std::move(style);for(size_t i=0;i<documents_.size();++i)*documents_[i].document=std::move(next[i]);warnings_=std::move(warnings);
}
bool Framework::set_style_tempo(size_t index,double tempo){auto next=*styles_.at(index).document;if(!next.set_tempo(tempo))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_name(size_t index,const std::wstring& name){auto next=*styles_.at(index).document;if(!next.set_name(name))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_band_instrument(size_t index,size_t bandIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){auto next=*styles_.at(index).document;if(!next.set_band_instrument(bandIndex,instrumentIndex,patch,pchannel,pan,volume))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_band_collection_instrument(size_t index,size_t bandIndex,size_t instrumentIndex,size_t collectionIndex,size_t dlsIndex){
    const auto& owned=styles_.at(index);if(owned.path.empty())throw std::runtime_error("Save Style before assigning a project-relative collection");
    const auto& target=collections_.at(collectionIndex);const auto locale=target.document.instruments().at(dlsIndex);
    CollectionReference reference{relative_to(target.path,std::filesystem::path(owned.path).parent_path()),collection_identity(target.document.save_bytes())};
    auto next=*owned.document;if(!next.set_band_dls_instrument(bandIndex,instrumentIndex,reference,locale.bank,locale.program))return false;
    std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});
    (void)resolve_collections(document_collection_references(next.save_bytes()),std::filesystem::path(owned.path).parent_path().wstring(),catalog);
    apply_style_edit(index,std::move(next));return true;
}
bool Framework::set_style_meter(size_t index,unsigned beats,unsigned denominator,unsigned grids){auto next=*styles_.at(index).document;if(!next.set_meter(beats,denominator,grids))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_pattern_groove(size_t index,size_t patternIndex,unsigned bottom,unsigned top){auto next=*styles_.at(index).document;if(!next.set_pattern_groove(patternIndex,bottom,top))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_pattern_properties(size_t index,size_t patternIndex,const std::wstring& name,unsigned embellishment){auto next=*styles_.at(index).document;if(!next.set_pattern_properties(patternIndex,name,embellishment))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_pattern_layout(size_t index,size_t patternIndex,unsigned beats,unsigned denominator,unsigned grids,unsigned measures){auto next=*styles_.at(index).document;if(!next.set_pattern_layout(patternIndex,beats,denominator,grids,measures))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::duplicate_style_pattern(size_t index,size_t patternIndex,const std::wstring& name){auto next=*styles_.at(index).document;if(!next.duplicate_pattern(patternIndex,name))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::paste_style_pattern(size_t index,const Bytes& bytes,const std::wstring& name){auto next=*styles_.at(index).document;if(!next.paste_pattern(bytes,name))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::new_style_pattern(size_t index,const std::wstring& name,unsigned pchannel){auto next=*styles_.at(index).document;if(!next.new_pattern(name,pchannel))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::assign_style_motif_band(size_t index,size_t patternIndex,size_t bandIndex){auto next=*styles_.at(index).document;if(!next.assign_motif_band(patternIndex,bandIndex))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_motif_band_collection_instrument(size_t index,size_t patternIndex,size_t instrumentIndex,size_t collectionIndex,size_t dlsIndex){
    const auto& owned=styles_.at(index);if(owned.path.empty())throw std::runtime_error("Save Style before assigning a project-relative collection");
    const auto& target=collections_.at(collectionIndex);const auto locale=target.document.instruments().at(dlsIndex);
    CollectionReference reference{relative_to(target.path,std::filesystem::path(owned.path).parent_path()),collection_identity(target.document.save_bytes())};
    auto next=*owned.document;if(!next.set_motif_band_dls_instrument(patternIndex,instrumentIndex,reference,locale.bank,locale.program))return false;
    std::vector<CollectionEntry> catalog;for(const auto& c:collections_)catalog.push_back({c.path,c.document.save_bytes()});
    (void)resolve_collections(document_collection_references(next.save_bytes()),std::filesystem::path(owned.path).parent_path().wstring(),catalog);
    apply_style_edit(index,std::move(next));return true;
}
bool Framework::set_style_motif_band_instrument(size_t index,size_t patternIndex,size_t instrumentIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){auto next=*styles_.at(index).document;if(!next.set_motif_band_instrument(patternIndex,instrumentIndex,patch,pchannel,pan,volume))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::new_style_motif(size_t index,const std::wstring& name,unsigned pchannel){auto next=*styles_.at(index).document;if(!next.new_motif(name,pchannel))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_motif_settings(size_t index,size_t patternIndex,const StyleMotifSettings& value){auto next=*styles_.at(index).document;if(!next.set_motif_settings(patternIndex,value))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::delete_style_pattern(size_t index,size_t patternIndex){auto next=*styles_.at(index).document;if(!next.delete_pattern(patternIndex))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::unshare_style_pattern_part(size_t index,size_t patternIndex,size_t referenceIndex){auto next=*styles_.at(index).document;if(!next.unshare_pattern_part(patternIndex,referenceIndex))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_part_variation_choice(size_t index,size_t partIndex,size_t variationIndex,std::uint32_t choices){auto next=*styles_.at(index).document;if(!next.set_part_variation_choice(partIndex,variationIndex,choices))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::add_style_band_gm_instrument(size_t index,std::optional<size_t> bandIndex,std::uint32_t patch,std::uint32_t pchannel,unsigned pan,unsigned volume){auto next=*styles_.at(index).document;if(!next.add_band_gm_instrument(bandIndex,patch,pchannel,pan,volume))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::set_style_part_note(size_t index,size_t partIndex,size_t noteIndex,std::int32_t duration,unsigned velocity){auto next=*styles_.at(index).document;if(!next.set_part_note(partIndex,noteIndex,duration,velocity))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::edit_style_part_note(size_t index,size_t partIndex,size_t noteIndex,const StyleNoteEdit& value){auto next=*styles_.at(index).document;if(!next.edit_part_note(partIndex,noteIndex,value))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::insert_style_part_note(size_t index,size_t partIndex,size_t position,const StyleNoteEdit& value,std::optional<size_t> templateIndex){auto next=*styles_.at(index).document;if(!next.insert_part_note(partIndex,position,value,templateIndex))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::delete_style_part_note(size_t index,size_t partIndex,size_t noteIndex){auto next=*styles_.at(index).document;if(!next.delete_part_note(partIndex,noteIndex))return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::undo_style(size_t index){auto next=*styles_.at(index).document;if(!next.undo())return false;apply_style_edit(index,std::move(next));return true;}
bool Framework::redo_style(size_t index){auto next=*styles_.at(index).document;if(!next.redo())return false;apply_style_edit(index,std::move(next));return true;}
void Framework::open_project(const std::wstring& path) {
    const auto full=std::filesystem::absolute(path).lexically_normal();const auto input=read_file(full.wstring());const auto root=Chunk::parse(input);
    Framework next;next.projectPath_=full.wstring();next.projectDirectory_=full.parent_path().wstring();
    if(root.type=="DMPJ") {
        const auto version=root.find("vers");if(!version||version->data.size()!=4||read32(version->data,0)!=1)throw std::runtime_error("Unsupported product project version");
        if(const auto name=root.find("name"))next.name_=decode_utf16(name->data);
        next.projectRoot_=root; // Retain every unrecognized project chunk.
    } else if(root.type=="JAZP") {
        // Keep the native metadata and its path basis separate from the owned
        // document catalog. File names can be updated without inventing filh,
        // editor-window, runtime-export or source-control metadata.
        Chunk original;original.id="orig";original.data=input;next.projectRoot_.children.push_back(std::move(original));
        Chunk basis;basis.id="obas";basis.data=utf16(full.parent_path().wstring());next.projectRoot_.children.push_back(std::move(basis));
        for(const auto& c:root.children)if(c.id=="LIST"&&c.type=="file") {
            const auto name=c.find("name");if(!name)throw std::runtime_error("Original project file name missing");
            next.projectRoot_.children.push_back(file_reference(decode_utf16(name->data)));
        }
        next.warnings_.push_back(L"Native JAZP metadata is retained. .pro saves existing file entries in the original directory; new Segment/Style/Band/DLS/AudioPath entries are supported; typed runtime folder/name settings and configured multi-folder saves are supported; per-file folder memory and crash recovery remain incomplete.");
    } else throw std::runtime_error("Unsupported project form");
    if(std::count_if(next.projectRoot_.children.begin(),next.projectRoot_.children.end(),[](const Chunk& c){return c.id=="file";})>1000)throw std::runtime_error("Project document limit");
    // Load project-owned Styles first; GUID-only references must not depend on
    // the order of file entries in the serialized project.
    for(size_t i=0;i<next.projectRoot_.children.size();++i){const auto& c=next.projectRoot_.children[i];if(c.id!="file")continue;const auto reference=decode_utf16(c.data);const auto file=resolve(full.parent_path(),reference);if(!next.components_.is_style_path(file.wstring()))continue;
        if(!std::filesystem::exists(file)){next.warnings_.push_back(L"Style file missing; reference retained: "+reference);continue;}const auto index=next.open_style(file.wstring());if(!next.styles_[index].projectReference)next.styles_[index].projectReference=i;else next.warnings_.push_back(L"Repeated Style reference retained: "+reference);}
    unsigned count=0;
    for(size_t i=0;i<next.projectRoot_.children.size();++i) {
        const auto& c=next.projectRoot_.children[i];if(c.id!="file")continue;if(++count>1000)throw std::runtime_error("Project document limit");
        const auto reference=decode_utf16(c.data);const auto file=resolve(full.parent_path(),reference);
        if(next.components_.is_segment_path(file.wstring())) {
            const auto index=next.open_segment(file.wstring());
            if(!next.documents_[index].projectReference)next.documents_[index].projectReference=i;
            else next.warnings_.push_back(L"Repeated source reference is retained: "+reference);
        } else if(next.components_.is_band_path(file.wstring())){if(!std::filesystem::exists(file)){next.warnings_.push_back(L"Band file missing; reference retained: "+reference);continue;}const auto index=next.open_band(file.wstring());if(!next.bands_[index].projectReference)next.bands_[index].projectReference=i;else next.warnings_.push_back(L"Repeated Band reference retained: "+reference);}
        else if(next.components_.is_collection_path(file.wstring())){if(!std::filesystem::exists(file)){next.warnings_.push_back(L"Collection missing; reference retained: "+reference);continue;}const auto index=next.open_collection(file.wstring());if(!next.collections_[index].projectReference)next.collections_[index].projectReference=i;else next.warnings_.push_back(L"Repeated collection reference retained: "+reference);}
        else if(next.components_.is_audio_path(file.wstring())){if(!std::filesystem::exists(file)){next.warnings_.push_back(L"AudioPath missing; reference retained: "+reference);continue;}const auto index=next.open_audio_path(file.wstring());if(!next.audioPaths_[index].projectReference)next.audioPaths_[index].projectReference=i;else next.warnings_.push_back(L"Repeated AudioPath reference retained: "+reference);}
        else if(!next.components_.is_style_path(file.wstring()))next.warnings_.push_back(L"Reference retained; editor not implemented: "+reference);
    }
    next.projectDirty_=false;*this=std::move(next);
}
void Framework::save_project(const std::wstring& path) {
    const auto full=std::filesystem::absolute(path).lexically_normal();
    const bool native=CompareStringOrdinal(full.extension().c_str(),-1,L".pro",-1,TRUE)==CSTR_EQUAL;
    if(!native&&CompareStringOrdinal(full.extension().c_str(),-1,L".dmpj",-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("Project extension must be .dmpj or .pro");
    if(native&&CompareStringOrdinal(full.stem().c_str(),-1,full.parent_path().filename().c_str(),-1,TRUE)!=CSTR_EQUAL)
        throw std::runtime_error("Native Producer project name must match its containing folder. Save FolderName.pro inside FolderName; use .dmpj for other filenames.");
    auto root=projectRoot_;auto nextReferences=std::vector<std::optional<size_t>>(documents_.size());
    auto nextStyleReferences=std::vector<std::optional<size_t>>(styles_.size());
    auto nextAudioReferences=std::vector<std::optional<size_t>>(audioPaths_.size());
    auto nextBandReferences=std::vector<std::optional<size_t>>(bands_.size());
    auto nextCollectionReferences=std::vector<std::optional<size_t>>(collections_.size());
    // Preserve unsupported files and unknown metadata. Moving a project must
    // keep every reference resolvable inside the new project directory.
    for(size_t index=0;index<root.children.size();++index) {
        auto& c=root.children[index];if(c.id!="file")continue;
        // Owned documents may already have been saved into the new directory.
        // Their references are replaced below; only retained external files
        // still need to be resolved against the old project directory.
        bool owned=false;for(const auto& d:documents_)if(d.projectReference==index){owned=true;break;}
        for(const auto& style:styles_)if(style.projectReference==index){owned=true;break;}
        for(const auto& band:bands_)if(band.projectReference==index){owned=true;break;}
        for(const auto& collection:collections_)if(collection.projectReference==index){owned=true;break;}
        for(const auto& audio:audioPaths_)if(audio.projectReference==index){owned=true;break;}
        if(owned)continue;
        if(projectDirectory_.empty())throw std::runtime_error("Project reference has no base directory");
        c.data=utf16(relative_to(resolve(projectDirectory_,decode_utf16(c.data)),full.parent_path()));
    }
    for(size_t i=0;i<styles_.size();++i){const auto& style=styles_[i];if(style.path.empty()||style.document->dirty())throw std::runtime_error("Save every Style before saving the project");const auto reference=relative_to(style.path,full.parent_path());if(style.projectReference){const auto index=*style.projectReference;if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("Style project ownership mismatch");root.children[index].data=utf16(reference);nextStyleReferences[i]=index;}else{nextStyleReferences[i]=root.children.size();root.children.push_back(file_reference(reference));}}
    for(size_t i=0;i<documents_.size();++i) {
        const auto& d=documents_[i];if(d.path.empty()||d.document->dirty())throw std::runtime_error("Save every segment before saving the project");
        const auto reference=relative_to(d.path,full.parent_path());
        if(d.projectReference) {const auto index=*d.projectReference;if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("Project document ownership mismatch");root.children[index].data=utf16(reference);nextReferences[i]=index;}
        else {nextReferences[i]=root.children.size();root.children.push_back(file_reference(reference));}
    }
    for(size_t i=0;i<bands_.size();++i){const auto& band=bands_[i];if(band.path.empty()||band.document->dirty())throw std::runtime_error("Save every Band before saving the project");const auto reference=relative_to(band.path,full.parent_path());if(band.projectReference){const auto index=*band.projectReference;if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("Band project ownership mismatch");root.children[index].data=utf16(reference);nextBandReferences[i]=index;}else{nextBandReferences[i]=root.children.size();root.children.push_back(file_reference(reference));}}
    for(size_t i=0;i<audioPaths_.size();++i){const auto& audio=audioPaths_[i];if(audio.path.empty()||audio.document->dirty())throw std::runtime_error("Save every AudioPath before saving the project");const auto reference=relative_to(audio.path,full.parent_path());if(audio.projectReference){const auto index=*audio.projectReference;if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("AudioPath project ownership mismatch");root.children[index].data=utf16(reference);nextAudioReferences[i]=index;}else{nextAudioReferences[i]=root.children.size();root.children.push_back(file_reference(reference));}}
    for(size_t i=0;i<collections_.size();++i){const auto& collection=collections_[i];if(collection.path.empty()||collection.document.dirty())throw std::runtime_error("Save every collection before saving project");const auto reference=relative_to(collection.path,full.parent_path());if(collection.projectReference){const auto index=*collection.projectReference;if(index>=root.children.size()||root.children[index].id!="file")throw std::runtime_error("Collection project ownership mismatch");root.children[index].data=utf16(reference);nextCollectionReferences[i]=index;}else{nextCollectionReferences[i]=root.children.size();root.children.push_back(file_reference(reference));}}
    auto output=root.encode();
    if(native){
        if(!root.find("orig")){
            Chunk metadata;metadata.id="orig";metadata.data=empty_native_project().encode();root.children.push_back(metadata);
            Chunk base;base.id="obas";base.data=utf16(full.parent_path().wstring());root.children.push_back(base);
        }
        const auto original=root.find("orig"),basis=root.find("obas");
        if(!original||!basis)throw std::runtime_error("Native JAZP save requires imported metadata and its original directory");
        if(!same_path(decode_utf16(basis->data),full.parent_path().wstring()))throw std::runtime_error("Native JAZP runtime paths require the original project directory; relocation is not implemented");
        auto nativeRoot=Chunk::parse(original->data);if(nativeRoot.type!="JAZP")throw std::runtime_error("Native project metadata form mismatch");
        const auto pending=pending_metadata(root);
        std::vector<std::wstring> references;std::vector<bool> refresh;
        for(size_t i=0;i<root.children.size();++i)if(root.children[i].id=="file"){references.push_back(decode_utf16(root.children[i].data));refresh.push_back(std::find(pending.begin(),pending.end(),i)!=pending.end());}
        size_t index=0;for(auto& file:nativeRoot.children)if(file.id=="LIST"&&file.type=="file"){
            auto name=file.find("name");if(!name||index>=references.size())throw std::runtime_error("Native project file ownership mismatch");
            name->data=utf16(references[index]);
            if(refresh[index])refresh_native_metadata(file,resolve(full.parent_path(),references[index]));++index;
        }
        while(index<references.size()){
            const auto& reference=references[index++];
            nativeRoot.children.push_back(native_document_reference(resolve(full.parent_path(),reference),reference));
        }
        output=nativeRoot.encode();root.find("orig")->data=output;
        // Keep the empty journal chunk in place: erasing shifts owned indices.
        if(auto journal=root.find("mupd"))journal->data.clear();
    }
    write_file_atomic(full.wstring(),output);
    projectRoot_=std::move(root);projectDirectory_=full.parent_path().wstring();projectPath_=full.wstring();projectDirty_=false;
    for(size_t i=0;i<documents_.size();++i)documents_[i].projectReference=nextReferences[i];
    for(size_t i=0;i<styles_.size();++i)styles_[i].projectReference=nextStyleReferences[i];
    for(size_t i=0;i<audioPaths_.size();++i)audioPaths_[i].projectReference=nextAudioReferences[i];
    for(size_t i=0;i<bands_.size();++i)bands_[i].projectReference=nextBandReferences[i];
    for(size_t i=0;i<collections_.size();++i)collections_[i].projectReference=nextCollectionReferences[i];
}
void Framework::copy_project(const std::wstring& destination) const {
    if(projectPath_.empty()||dirty())throw std::runtime_error("Save the Project and every document before copying");
    const auto source=std::filesystem::path(projectPath_),base=source.parent_path(),target=std::filesystem::absolute(destination).lexically_normal(),folder=target.parent_path();
    const bool native=CompareStringOrdinal(target.extension().c_str(),-1,L".pro",-1,TRUE)==CSTR_EQUAL;
    if(!native&&CompareStringOrdinal(target.extension().c_str(),-1,L".dmpj",-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("Copy Project extension must be .pro or .dmpj");
    if(native!= (CompareStringOrdinal(source.extension().c_str(),-1,L".pro",-1,TRUE)==CSTR_EQUAL))throw std::runtime_error("Copy Project keeps the source format");
    if(native&&CompareStringOrdinal(target.stem().c_str(),-1,folder.filename().c_str(),-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("Native copy filename must match its new folder");
    if(std::filesystem::exists(folder)||!std::filesystem::is_directory(folder.parent_path()))throw std::runtime_error("Copy Project requires a new folder in an existing directory");
    const auto physicalBase=std::filesystem::canonical(base),physicalParent=std::filesystem::canonical(folder.parent_path());
    const auto relation=physicalParent.lexically_relative(physicalBase);if(relation.empty()||(*relation.begin()!=L".."&&!relation.is_absolute()))throw std::runtime_error("Project copy must be outside its source folder");
    struct CopyFile {std::filesystem::path source,relative;Bytes bytes;std::filesystem::file_time_type time;};std::vector<CopyFile> files;
    const auto add=[&](const std::filesystem::path& file){
        const auto full=std::filesystem::absolute(file).lexically_normal();const auto relative=relative_to(full,base);const auto physical=std::filesystem::canonical(full).lexically_relative(physicalBase);
        if(physical.empty()||physical.is_absolute()||*physical.begin()==L".."||!std::filesystem::is_regular_file(full))throw std::runtime_error("Project copy dependency must be a contained regular file");
        if(same_path(full.wstring(),source.wstring()))throw std::runtime_error("Project cannot reference itself");
        for(const auto& item:files)if(same_path(item.source.wstring(),full.wstring()))return;
        const auto time=std::filesystem::last_write_time(full);auto bytes=read_file(full.wstring());if(std::filesystem::last_write_time(full)!=time)throw std::runtime_error("Project copy source changed");files.push_back({full,relative,std::move(bytes),time});
    };
    for(const auto& entry:projectRoot_.children)if(entry.id=="file")add(resolve(base,decode_utf16(entry.data)));
    for(const auto& owned:styles_)if(read_file(owned.path)!=owned.document->save_bytes())throw std::runtime_error("Saved Style changed externally");
    for(const auto& owned:documents_)if(read_file(owned.path)!=owned.document->save_bytes())throw std::runtime_error("Saved Segment changed externally");
    for(const auto& owned:bands_)if(read_file(owned.path)!=owned.document->save_bytes())throw std::runtime_error("Saved Band changed externally");
    for(const auto& owned:audioPaths_)if(read_file(owned.path)!=owned.document->save_bytes())throw std::runtime_error("Saved AudioPath changed externally");
    for(const auto& owned:collections_)if(read_file(owned.path)!=owned.document.save_bytes())throw std::runtime_error("Saved collection changed externally");
    // Include filename-only dependencies as well as project-owned GUID entries.
    for(size_t i=0;i<files.size();++i){const auto bytes=files[i].bytes;const auto owner=files[i].source.parent_path();if(bytes.size()<12||std::string(bytes.begin(),bytes.begin()+4)!="RIFF")continue;
        const auto root=Chunk::parse(bytes);if(root.type=="DMSG")for(const auto& ref:style_references(root))if(!ref.filename.empty())add(resolve(owner,ref.filename));
        if(root.type=="DMSG"||root.type=="DMST"||root.type=="DMBD")for(const auto& ref:document_collection_references(bytes))if(!ref.filename.empty())add(resolve(owner,ref.filename));
    }
    for(const auto& item:files)if(same_path((folder/item.relative).wstring(),target.wstring()))throw std::runtime_error("Copied document collides with Project filename");
    auto projectBytes=read_file(projectPath_);const auto sourceProjectBytes=projectBytes;const auto original=projectRoot_.find("orig");if(native?(!original||original->data!=projectBytes):(projectRoot_.encode()!=projectBytes))throw std::runtime_error("Saved Project changed externally");
    if(!native){auto root=Chunk::parse(projectBytes);if(auto basis=root.find("obas")){if(!same_path(decode_utf16(basis->data),base.wstring()))throw std::runtime_error("Internal native basis mismatch");basis->data=utf16(folder.wstring());}projectBytes=root.encode();}
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create copy staging identity");wchar_t text[40]{};StringFromGUID2(id,text,40);const auto staging=folder.parent_path()/(L".producer-copy-"+std::wstring(text));
    if(!std::filesystem::create_directory(staging))throw std::runtime_error("Copy staging exists");
    try{for(const auto& item:files){const auto path=staging/item.relative;std::filesystem::create_directories(path.parent_path());write_file_atomic(path.wstring(),item.bytes);std::filesystem::last_write_time(path,item.time);if(read_file(item.source.wstring())!=item.bytes||std::filesystem::last_write_time(item.source)!=item.time)throw std::runtime_error("Project copy source changed");}
        write_file_atomic((staging/target.filename()).wstring(),projectBytes);Framework check;check.open_project((staging/target.filename()).wstring());
        // Opening the staging Project can read further dependencies. Recheck
        // the complete source set after that validation, before publication.
        if(read_file(projectPath_)!=sourceProjectBytes)throw std::runtime_error("Project copy source changed during validation");
        for(const auto& item:files)if(read_file(item.source.wstring())!=item.bytes||std::filesystem::last_write_time(item.source)!=item.time)throw std::runtime_error("Project copy dependency changed during validation");
        std::filesystem::rename(staging,folder);
    }catch(...){std::error_code ignored;std::filesystem::remove_all(staging,ignored);throw;}
}

namespace {
const wchar_t* runtime_filter(RuntimeDocumentKind kind){switch(kind){case RuntimeDocumentKind::Segment:return L".sgt;*.sgp";case RuntimeDocumentKind::Style:return L".sty;*.stp";case RuntimeDocumentKind::Band:return L".bnd;*.bnp";case RuntimeDocumentKind::Collection:return L".dls;*.dlp";case RuntimeDocumentKind::AudioPath:return L".aud;*.aup";default:throw std::runtime_error("Unknown runtime document kind");}}
const wchar_t* runtime_extension(RuntimeDocumentKind kind){switch(kind){case RuntimeDocumentKind::Segment:return L".sgt";case RuntimeDocumentKind::Style:return L".sty";case RuntimeDocumentKind::Band:return L".bnd";case RuntimeDocumentKind::Collection:return L".dls";case RuntimeDocumentKind::AudioPath:return L".aud";default:throw std::runtime_error("Unknown runtime document kind");}}
const Chunk* runtime_unique(const Chunk& c,const char* id,const char* type=""){const Chunk* result=nullptr;for(const auto& x:c.children)if(x.id==id&&(!*type||x.type==type)){if(result)throw std::runtime_error("Ambiguous native runtime setting");result=&x;}return result;}
Chunk* runtime_unique(Chunk& c,const char* id,const char* type=""){return const_cast<Chunk*>(runtime_unique(static_cast<const Chunk&>(c),id,type));}
Chunk& runtime_child(Chunk& c,const char* id,const char* type=""){if(auto result=runtime_unique(c,id,type))return *result;Chunk next;next.id=id;next.type=type;c.children.push_back(next);return c.children.back();}
bool runtime_text_valid(const std::wstring& text){return !text.empty()&&text.size()<32767&&text.find(L'\0')==std::wstring::npos&&std::none_of(text.begin(),text.end(),[](wchar_t c){return c<32;});}
bool runtime_folder_valid(const std::wstring& text){if(!runtime_text_valid(text)||text.find_first_of(L"<>\"|?*")!=std::wstring::npos)return false;const auto p=std::filesystem::path(text);return !p.has_root_name()||p.is_absolute();}
bool runtime_name_valid(const std::wstring& text,RuntimeDocumentKind kind){if(!runtime_text_valid(text)||text.find_first_of(L"<>:\"|?*")!=std::wstring::npos)return false;const auto p=std::filesystem::path(text);if(p.is_absolute()||p.has_root_name()||p.filename().empty())return false;for(const auto& x:p)if(x==L".."||x==L".")return false;return CompareStringOrdinal(p.extension().c_str(),-1,runtime_extension(kind),-1,TRUE)==CSTR_EQUAL;}
bool runtime_filter_matches(const std::wstring& filter,RuntimeDocumentKind kind){const auto expected=std::wstring(runtime_filter(kind));size_t start=0;while(start<=filter.size()){const auto end=filter.find(L';',start);auto token=filter.substr(start,end==std::wstring::npos?end:end-start);if(!token.empty()&&token[0]==L'*')token.erase(token.begin());std::transform(token.begin(),token.end(),token.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});const auto a=expected.find(L';');if(token==expected.substr(0,a)||token==expected.substr(a+2))return true;if(end==std::wstring::npos)break;start=end+1;}return false;}
const Chunk* runtime_component(const Chunk& native,RuntimeDocumentKind kind){const auto project=runtime_unique(native,"LIST","proj");const auto folders=project?runtime_unique(*project,"LIST","rfld"):nullptr;const Chunk* result=nullptr;if(folders)for(const auto& f:folders->children)if(f.id=="LIST"&&f.type=="fldr"){const auto filter=runtime_unique(f,"fltr");if(filter&&runtime_filter_matches(decode_utf16(filter->data),kind)){if(result)throw std::runtime_error("Ambiguous component runtime folders");if(!runtime_unique(f,"path"))throw std::runtime_error("Component runtime path missing");result=&f;}}return result;}
Chunk* runtime_file(Chunk& native,const Chunk& catalog,size_t reference){if(reference>=catalog.children.size()||catalog.children[reference].id!="file")throw std::runtime_error("Runtime setting document ownership mismatch");size_t ordinal=0;for(size_t i=0;i<reference;++i)if(catalog.children[i].id=="file")++ordinal;for(auto& c:native.children)if(c.id=="LIST"&&c.type=="file"){if(!ordinal--)return &c;}throw std::runtime_error("Native runtime file metadata missing");}
}
std::pair<std::wstring,std::optional<size_t>> Framework::runtime_owner(RuntimeDocumentKind kind,size_t index) const {
    switch(kind){case RuntimeDocumentKind::Segment:{const auto& o=documents_.at(index);return {o.path,o.projectReference};}case RuntimeDocumentKind::Style:{const auto& o=styles_.at(index);return {o.path,o.projectReference};}case RuntimeDocumentKind::Band:{const auto& o=bands_.at(index);return {o.path,o.projectReference};}case RuntimeDocumentKind::Collection:{const auto& o=collections_.at(index);return {o.path,o.projectReference};}case RuntimeDocumentKind::AudioPath:{const auto& o=audioPaths_.at(index);return {o.path,o.projectReference};}default:throw std::runtime_error("Unknown runtime document kind");}
}
Chunk Framework::runtime_metadata() const {
    if(const auto original=projectRoot_.find("orig")){auto native=Chunk::parse(original->data);if(native.type!="JAZP")throw std::runtime_error("Native runtime metadata form mismatch");return native;}
    if(projectDirectory_.empty()||projectPath_.empty())throw std::runtime_error("Save Project before editing runtime settings");auto native=empty_native_project();for(const auto& c:projectRoot_.children)if(c.id=="file"){const auto reference=decode_utf16(c.data);native.children.push_back(native_document_reference(resolve(projectDirectory_,reference),reference));}return native;
}
bool Framework::adopt_runtime_metadata(const Chunk& native){const auto bytes=native.encode();if(const auto old=projectRoot_.find("orig"))if(old->data==bytes)return false;auto next=projectRoot_;if(!next.find("orig")){Chunk original;original.id="orig";next.children.push_back(original);}next.find("orig")->data=bytes;if(!next.find("obas")){Chunk basis;basis.id="obas";basis.data=utf16(projectDirectory_);next.children.push_back(basis);}projectRoot_=std::move(next);projectDirty_=true;return true;}
std::wstring Framework::runtime_project_folder() const {if(!projectRoot_.find("orig"))return L"..\\RuntimeFiles\\";const auto native=runtime_metadata();const auto project=runtime_unique(native,"LIST","proj");const auto info=project?runtime_unique(*project,"LIST","UNFO"):nullptr;const auto folder=info?runtime_unique(*info,"rdir"):nullptr;return folder?decode_utf16(folder->data):L"..\\RuntimeFiles\\";}
std::wstring Framework::runtime_component_folder(RuntimeDocumentKind kind) const {if(!projectRoot_.find("orig")){(void)runtime_filter(kind);return runtime_project_folder();}const auto native=runtime_metadata();if(const auto component=runtime_component(native,kind))return decode_utf16(runtime_unique(*component,"path")->data);return runtime_project_folder();}
std::wstring Framework::runtime_filename(RuntimeDocumentKind kind,size_t index) const {const auto owner=runtime_owner(kind,index);if(owner.second&&projectRoot_.find("orig")){auto native=runtime_metadata();const auto entry=runtime_file(native,projectRoot_,*owner.second);if(const auto info=runtime_unique(*entry,"LIST","UNFO"))if(const auto name=runtime_unique(*info,"rnam"))return decode_utf16(name->data);}auto path=std::filesystem::path(owner.first).filename();if(path.empty())path=L"Untitled";path.replace_extension(runtime_extension(kind));return path.wstring();}
bool Framework::set_runtime_project_folder(const std::wstring& folder){if(!runtime_folder_valid(folder))return false;const auto previous=runtime_project_folder();auto native=runtime_metadata();auto& project=runtime_child(native,"LIST","proj");auto& info=runtime_child(project,"LIST","UNFO");runtime_child(info,"rdir").data=utf16(folder);if(auto folders=runtime_unique(project,"LIST","rfld"))for(auto& item:folders->children)if(item.id=="LIST"&&item.type=="fldr")if(auto path=runtime_unique(item,"path"))if(decode_utf16(path->data)==previous)path->data=utf16(folder);return adopt_runtime_metadata(native);}
bool Framework::set_runtime_component_folder(RuntimeDocumentKind kind,const std::wstring& folder){if(!runtime_folder_valid(folder))return false;auto native=runtime_metadata();auto component=const_cast<Chunk*>(runtime_component(native,kind));if(component)runtime_unique(*component,"path")->data=utf16(folder);else{auto& project=runtime_child(native,"LIST","proj");auto& folders=runtime_child(project,"LIST","rfld");Chunk item;item.id="LIST";item.type="fldr";runtime_child(item,"path").data=utf16(folder);runtime_child(item,"fltr").data=utf16(runtime_filter(kind));folders.children.push_back(std::move(item));}return adopt_runtime_metadata(native);}
bool Framework::set_runtime_filename(RuntimeDocumentKind kind,size_t index,const std::wstring& name){if(!runtime_name_valid(name,kind))return false;const auto owner=runtime_owner(kind,index);if(!owner.second)throw std::runtime_error("Save Project before editing a document runtime name");auto native=runtime_metadata();auto& info=runtime_child(*runtime_file(native,projectRoot_,*owner.second),"LIST","UNFO");runtime_child(info,"rnam").data=utf16(name);return adopt_runtime_metadata(native);}
void Framework::save_runtime_default(RuntimeDocumentKind kind,size_t index) const {if(projectDirectory_.empty())throw std::runtime_error("Save Project before default runtime save");const auto folder=runtime_component_folder(kind),name=runtime_filename(kind,index);if(!runtime_folder_valid(folder)||!runtime_name_valid(name,kind))throw std::runtime_error("Invalid or unsupported native runtime destination");save_runtime(kind,index,(std::filesystem::path(projectDirectory_)/std::filesystem::path(folder)/std::filesystem::path(name)).lexically_normal().wstring());}

void Framework::save_runtime(RuntimeDocumentKind kind,size_t index,const std::wstring& destination) const {
    Bytes bytes;const wchar_t* extension=nullptr;
    switch(kind){case RuntimeDocumentKind::Segment:bytes=documents_.at(index).document->save_bytes();extension=L".sgt";break;case RuntimeDocumentKind::Style:bytes=styles_.at(index).document->save_bytes();extension=L".sty";break;case RuntimeDocumentKind::Band:bytes=bands_.at(index).document->save_bytes();extension=L".bnd";break;case RuntimeDocumentKind::Collection:bytes=collections_.at(index).document.save_bytes();extension=L".dls";break;case RuntimeDocumentKind::AudioPath:bytes=audioPaths_.at(index).document->save_bytes();extension=L".aud";break;default:throw std::runtime_error("Unknown runtime document kind");}
    const auto target=std::filesystem::absolute(destination).lexically_normal();if(CompareStringOrdinal(target.extension().c_str(),-1,extension,-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("Choose the matching runtime file extension");
    const auto outputParent=std::filesystem::canonical(target.parent_path());
    const auto outputPath=outputParent/target.filename();
    const auto protected_path=[&](const std::wstring& source){if(source.empty())return;const auto path=std::filesystem::path(source);const auto physical=std::filesystem::canonical(path.parent_path())/path.filename();if(same_path(physical.wstring(),outputPath.wstring())||(std::filesystem::exists(source)&&std::filesystem::exists(target)&&std::filesystem::equivalent(source,target)))throw std::runtime_error("Runtime save cannot overwrite an owned source or Project");};
    protected_path(projectPath_);for(const auto& d:documents_)protected_path(d.path);for(const auto& d:styles_)protected_path(d.path);for(const auto& d:bands_)protected_path(d.path);for(const auto& d:collections_)protected_path(d.path);for(const auto& d:audioPaths_)protected_path(d.path);
    for(const auto& c:projectRoot_.children)if(c.id=="file"&&!projectDirectory_.empty())protected_path(resolve(projectDirectory_,decode_utf16(c.data)).wstring());
    auto root=Chunk::parse(bytes);convert_runtime(root);const auto output=root.encode();write_file_atomic(target.wstring(),output);
}
void Framework::export_runtime(const std::wstring& directory) const {export_runtime_impl(directory,false);}
void Framework::export_runtime_defaults() const {export_runtime_impl(projectDirectory_,true);}
void Framework::export_runtime_observed(const std::wstring& directory,const std::function<void(const std::wstring&)>& afterPublish) const {export_runtime_impl(directory,false,afterPublish);}
void Framework::export_runtime_defaults_observed(const std::function<void(const std::wstring&)>& afterPublish) const {export_runtime_impl(projectDirectory_,true,afterPublish);}
void Framework::export_runtime_impl(const std::wstring& directory,bool configured,const std::function<void(const std::wstring&)>& afterPublish) const {
    if(projectPath_.empty()||dirty())throw std::runtime_error("Save the Project and every document before runtime export");
    const auto target=std::filesystem::absolute(directory).lexically_normal();
    const bool updating=std::filesystem::exists(target);if((updating&&!std::filesystem::is_directory(target))||!std::filesystem::is_directory(target.parent_path()))throw std::runtime_error("Runtime export requires an output folder in an existing directory");
    const auto sourceProjectBytes=read_file(projectPath_);
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create runtime staging identity");wchar_t text[40]{};StringFromGUID2(id,text,40);
    const auto stage=std::filesystem::path(projectPath_).parent_path().parent_path()/(L".dmrt-"+std::wstring(text));
    const bool native=CompareStringOrdinal(std::filesystem::path(projectPath_).extension().c_str(),-1,L".pro",-1,TRUE)==CSTR_EQUAL;
    const auto project=stage/(native?stage.filename().wstring()+L".pro":L"project.dmpj");
    // Reuse validated source closure and its atomic copy; export never adopts
    // the temporary Project or modifies the editor's documents/history.
    copy_project(project.wstring());
    bool retainStage=false;
    try {
        struct Output {std::filesystem::path source,destination,input,published;Bytes bytes,inputBytes;};std::vector<Output> outputs;
        for(const auto& entry:std::filesystem::recursive_directory_iterator(stage))if(entry.is_regular_file()&&!same_path(entry.path().wstring(),project.wstring())){
            const auto destination=runtime_name(entry.path());for(const auto& o:outputs)if(same_path(o.destination.wstring(),destination.wstring()))throw std::runtime_error("Runtime filenames collide");
            auto bytes=read_file(entry.path().wstring());const auto root=Chunk::parse(bytes);if(root.type!="DMSG"&&root.type!="DMST"&&root.type!="DMBD"&&root.type!="DLS "&&root.type!="DMAP")throw std::runtime_error("Runtime export document form is not implemented");
            outputs.push_back({entry.path(),destination,std::filesystem::path(projectDirectory_)/entry.path().lexically_relative(stage),{},bytes,bytes});
        }
        for(auto& o:outputs){auto root=Chunk::parse(o.bytes);convert_runtime(root);o.bytes=root.encode();}
        // Remove only files created in this unique, unpublished staging tree.
        for(const auto& o:outputs)std::filesystem::remove(o.source);
        std::filesystem::remove(project);
        for(const auto& o:outputs)write_file_atomic(o.destination.wstring(),o.bytes);
        Framework validation;
        for(const auto& o:outputs)if(validation.components_.is_collection_path(o.destination.wstring()))validation.open_collection(o.destination.wstring());
        for(const auto& o:outputs)if(validation.components_.is_style_path(o.destination.wstring()))validation.open_style(o.destination.wstring());
        for(const auto& o:outputs)if(validation.components_.is_audio_path(o.destination.wstring()))validation.open_audio_path(o.destination.wstring());
        for(const auto& o:outputs)if(validation.components_.is_band_path(o.destination.wstring()))validation.open_band(o.destination.wstring());
        for(const auto& o:outputs)if(validation.components_.is_segment_path(o.destination.wstring())){const auto i=validation.open_segment(o.destination.wstring());validation.playback_collections(i);}
        const auto check_sources=[&]{if(read_file(projectPath_)!=sourceProjectBytes)throw std::runtime_error("Runtime export Project changed externally");for(const auto& o:outputs)if(read_file(o.input.wstring())!=o.inputBytes)throw std::runtime_error("Runtime export source changed externally");};
        check_sources();
        for(auto& o:outputs)o.published=target/o.destination.lexically_relative(stage);
        if(configured){
            for(auto& o:outputs){const auto form=Chunk::parse(o.inputBytes).type;const auto kind=form=="DMSG"?RuntimeDocumentKind::Segment:form=="DMST"?RuntimeDocumentKind::Style:form=="DMBD"?RuntimeDocumentKind::Band:form=="DLS "?RuntimeDocumentKind::Collection:RuntimeDocumentKind::AudioPath;
                auto name=runtime_name(o.input.lexically_relative(projectDirectory_)).wstring();bool found=false;
                const auto owner=[&](const auto& list){for(size_t i=0;i<list.size();++i)if(same_path(list[i].path,o.input.wstring())){if(found)throw std::runtime_error("Ambiguous runtime owner");name=runtime_filename(kind,i);found=true;}};
                switch(kind){case RuntimeDocumentKind::Segment:owner(documents_);break;case RuntimeDocumentKind::Style:owner(styles_);break;case RuntimeDocumentKind::Band:owner(bands_);break;case RuntimeDocumentKind::Collection:owner(collections_);break;case RuntimeDocumentKind::AudioPath:owner(audioPaths_);break;}
                const auto folder=runtime_component_folder(kind);if(!runtime_folder_valid(folder)||!runtime_name_valid(name,kind))throw std::runtime_error("Invalid configured runtime destination");o.published=std::filesystem::absolute(std::filesystem::path(projectDirectory_)/folder/name).lexically_normal();
            }
            for(size_t i=0;i<outputs.size();++i)for(size_t k=0;k<i;++k)if(same_path(outputs[i].published.wstring(),outputs[k].published.wstring()))throw std::runtime_error("Configured runtime destinations collide");
            for(auto& o:outputs){auto root=Chunk::parse(o.inputBytes);const auto rewrite=[&](auto&& self,Chunk& c)->void{if(c.type=="DMRF")if(auto file=c.find("file")){const auto reference=decode_utf16(file->data);if(!reference.empty()){const auto source=(o.input.parent_path()/reference).lexically_normal();const Output* match=nullptr;for(const auto& dependency:outputs)if(same_path(source.wstring(),dependency.input.wstring())){if(match)throw std::runtime_error("Ambiguous runtime dependency");match=&dependency;}if(!match)throw std::runtime_error("Runtime filename dependency missing from source closure");const auto relative=match->published.lexically_relative(o.published.parent_path());if(relative.empty()||relative.is_absolute())throw std::runtime_error("Runtime dependency cannot be made relative across output volumes");file->data=utf16(relative.wstring());}}for(auto& child:c.children)if(child.container())self(self,child);};rewrite(rewrite,root);convert_runtime(root);o.bytes=root.encode();}
        }
        if(configured)for(const auto& o:outputs){const auto form=Chunk::parse(o.bytes).type;
            if(form=="DMSG"){SegmentDocument document;document.load(o.bytes);}else if(form=="DMST"){StyleDocument document;document.load(o.bytes);}else if(form=="DMBD"){BandDocument document;document.load(o.bytes);(void)document.collection_references();}else if(form=="DLS "){DlsDocument document;document.load(o.bytes);}else{AudioPathDocument document;document.load(o.bytes);}
            (void)document_collection_references(o.bytes);if(form=="DMSG")(void)style_references(Chunk::parse(o.bytes));
        }
        if(!updating){std::filesystem::rename(stage,target);return;}
        const auto targetAttributes=GetFileAttributesW(target.c_str());if(targetAttributes==INVALID_FILE_ATTRIBUTES||(targetAttributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Runtime output folder unavailable or reparse point");
        struct Previous {std::filesystem::path target;bool exists;Bytes bytes;WIN32_FILE_ATTRIBUTE_DATA attributes{};};
        std::sort(outputs.begin(),outputs.end(),[](const Output& a,const Output& b){return a.published<b.published;});
        std::vector<Previous> previous;
        for(const auto& o:outputs){
            const auto destination=o.published;
            const auto relative=destination.lexically_relative(destination.root_path());auto parent=destination.root_path();
            for(const auto& part:relative.parent_path()){parent/=part;if(std::filesystem::exists(parent)){const auto attr=GetFileAttributesW(parent.c_str());if(attr==INVALID_FILE_ATTRIBUTES||!(attr&FILE_ATTRIBUTE_DIRECTORY)||(attr&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Runtime output parent unavailable or reparse point");}}
            auto ancestor=destination.parent_path();while(!std::filesystem::exists(ancestor)){const auto next=ancestor.parent_path();if(next==ancestor)throw std::runtime_error("Runtime output volume unavailable");ancestor=next;}
            const auto physical=std::filesystem::canonical(ancestor)/destination.lexically_relative(ancestor);
            const auto protect=[&](const std::filesystem::path& source){const auto path=std::filesystem::canonical(source.parent_path())/source.filename();if(same_path(path.wstring(),physical.wstring())||(std::filesystem::exists(destination)&&std::filesystem::equivalent(source,destination)))throw std::runtime_error("Runtime update cannot overwrite a source or Project alias");};
            protect(projectPath_);for(const auto& source:outputs)protect(source.input);
            Previous before;before.target=destination;before.exists=std::filesystem::exists(destination);
            if(before.exists){if(!GetFileAttributesExW(destination.c_str(),GetFileExInfoStandard,&before.attributes)||before.attributes.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_READONLY))throw std::runtime_error("Runtime update target is not a writable regular file");before.bytes=read_file(destination.wstring());}
            previous.push_back(std::move(before));
        }
        const auto recovery=stage/L"recovery";std::filesystem::create_directory(recovery);
        std::vector<RuntimeUpdateRecoveryFile> recoveryFiles;
        for(size_t i=0;i<previous.size();++i){const auto& old=previous[i];recoveryFiles.push_back({old.target.wstring(),old.exists,old.bytes,outputs[i].bytes,old.attributes});}
        // Persist the complete plan before publishing any target. Version 1
        // omitted creation/access times and the expected new bytes, making
        // later external-change checks impossible from retained evidence.
        RuntimeRecoveryOrigin origin;origin.projectPath=projectPath_;origin.projectBytes=sourceProjectBytes;origin.outputRoot=target.wstring();origin.configured=configured;for(const auto& o:outputs)origin.sources.push_back({o.input.wstring(),o.inputBytes,o.published.wstring()});
        write_file_atomic((recovery/L"update.riff").wstring(),bind_runtime_update_recovery(recoveryFiles,origin).encode());
        std::vector<std::filesystem::path> created;std::vector<size_t> written;
        try{
            for(size_t i=0;i<outputs.size();++i){const auto& old=previous[i];if(old.exists?(!std::filesystem::exists(old.target)||read_file(old.target.wstring())!=old.bytes):std::filesystem::exists(old.target))throw std::runtime_error("Runtime output changed concurrently");
                auto parent=old.target.root_path();const auto relative=old.target.lexically_relative(parent);for(const auto& part:relative.parent_path()){parent/=part;if(!std::filesystem::exists(parent)){if(!std::filesystem::create_directory(parent))throw std::runtime_error("Cannot create runtime output parent");created.push_back(parent);}const auto attributes=GetFileAttributesW(parent.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||!(attributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Runtime output parent changed before commit");}
                if(old.exists&&old.bytes==outputs[i].bytes)continue;write_file_atomic(old.target.wstring(),outputs[i].bytes);written.push_back(i);if(afterPublish)afterPublish(old.target.wstring());
            }
            check_sources();for(size_t i=0;i<outputs.size();++i)if(read_file(previous[i].target.wstring())!=outputs[i].bytes)throw std::runtime_error("Runtime output changed during commit");
        }catch(...){
            const auto failure=std::current_exception();std::string recoveryError;
            for(auto item=written.rbegin();item!=written.rend();++item){const auto index=*item;const auto& old=previous[index];try{if(read_file(old.target.wstring())!=outputs[index].bytes)throw std::runtime_error("Runtime output changed during rollback");
                    if(old.exists){write_file_atomic(old.target.wstring(),old.bytes);const auto h=CreateFileW(old.target.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot restore runtime timestamp");const auto ok=SetFileTime(h,&old.attributes.ftCreationTime,&old.attributes.ftLastAccessTime,&old.attributes.ftLastWriteTime);CloseHandle(h);if(!ok||!SetFileAttributesW(old.target.c_str(),old.attributes.dwFileAttributes))throw std::runtime_error("Cannot restore runtime metadata");}
                    else if(!std::filesystem::remove(old.target))throw std::runtime_error("Cannot remove new runtime output");}
                catch(const std::exception& e){if(!recoveryError.empty())recoveryError+="; ";recoveryError+=e.what();}}
            for(auto i=created.rbegin();i!=created.rend();++i){std::error_code error;std::filesystem::remove(*i,error);if(error){if(!recoveryError.empty())recoveryError+="; ";recoveryError+="Runtime output parent retained";}}
            if(!recoveryError.empty()){retainStage=true;throw std::runtime_error("Runtime update rollback incomplete; recovery retained at "+recovery.u8string()+"; "+recoveryError);}
            std::rethrow_exception(failure);
        }
        std::error_code cleanupError;std::filesystem::remove_all(stage,cleanupError);
    }catch(...){if(!retainStage){std::error_code ignored;std::filesystem::remove_all(stage,ignored);}throw;}
}
bool Framework::dirty() const {if(projectDirty_)return true;for(const auto& audio:audioPaths_)if(audio.path.empty()||audio.document->dirty())return true;for(const auto& d:documents_)if(d.path.empty()||d.document->dirty())return true;for(const auto& style:styles_)if(style.path.empty()||style.document->dirty())return true;for(const auto& band:bands_)if(band.path.empty()||band.document->dirty())return true;for(const auto& collection:collections_)if(collection.document.dirty())return true;return false;}
}
