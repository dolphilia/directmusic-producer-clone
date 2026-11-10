#include "new_files.h"
#include <algorithm>
#include <cstring>
#include <set>

namespace producer::app {namespace {
Bytes creation_name(Bytes bytes,const std::wstring& name,bool ansi){
    if(name.empty()||name.size()>255||name.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Enter a valid name");
    auto root=Chunk::parse(bytes);Bytes text;
    if(ansi){BOOL changed=FALSE;const int size=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0,nullptr,&changed);if(!size||changed)throw std::runtime_error("Name cannot be represented in the current code page");text.resize(static_cast<size_t>(size)+1);WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,name.data(),static_cast<int>(name.size()),reinterpret_cast<char*>(text.data()),size,nullptr,&changed);if(changed)throw std::runtime_error("Name cannot be represented in the current code page");}
    else text=utf16(name);
    const char* form=ansi?"INFO":"UNFO";const char* field=ansi?"INAM":"UNAM";auto info=root.find("LIST",form);
    if(!info){Chunk c;c.id="LIST";c.type=form;root.children.push_back(c);info=&root.children.back();}
    auto label=info->find(field);if(!label){Chunk c;c.id=field;info->children.push_back(c);label=&info->children.back();}label->data=std::move(text);return root.encode();
}
}
std::vector<OpenedDocument> Framework::create_files(const std::vector<NewFileRequest>& requests){
    if(requests.empty())return {};Framework prepared;std::set<DocumentKind> seen;std::vector<OpenedDocument> added;bool replace=false;
    for(const auto& r:requests){if(!seen.insert(r.kind).second)throw std::runtime_error("Duplicate file type selection");
        if(r.name.empty()||r.name.size()>255||r.name.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Enter a valid name");
        size_t index=0;
        switch(r.kind){
        case DocumentKind::Project:replace=true;prepared.name_=r.name;prepared.projectRoot_.find("name")->data=utf16(r.name);break;
        case DocumentKind::Segment:index=prepared.new_segment();if(r.initial.empty())prepared.document(index)=SegmentDocument::create_empty({},r.name);else prepared.document(index).load(creation_name(r.initial,r.name,false));break;
        case DocumentKind::Style:index=prepared.new_style();prepared.styles_.at(index).document->load(creation_name(prepared.style_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::Band:index=prepared.new_band();prepared.bands_.at(index).document->load(creation_name(prepared.band_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::Collection:index=prepared.new_collection();prepared.collection_document(index).load(creation_name(prepared.collection_document(index).save_bytes(),r.name,true));break;
        case DocumentKind::AudioPath:index=prepared.new_audio_path();prepared.audio_path_document(index).load(creation_name(prepared.audio_path_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::ChordMap:index=prepared.new_chordmap();prepared.chordmap_document(index).load(creation_name(prepared.chordmap_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::Script:index=prepared.new_script();prepared.script_document(index).load(creation_name(prepared.script_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::Container:index=prepared.new_container();prepared.container_document(index).load(creation_name(prepared.container_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::ToolGraph:index=prepared.new_tool_graph();prepared.tool_graph_document(index).load(creation_name(prepared.tool_graph_document(index).save_bytes(),r.name,false));break;
        case DocumentKind::Wave:{if(r.initial.empty())throw std::runtime_error("Select a wave file");auto d=components_.create_wave_document("WAVE");d->load(r.initial);auto root=Chunk::parse(creation_name(r.initial,r.name,true));GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Wave identity creation failed");auto guid=root.find("guid");if(!guid){Chunk c;c.id="guid";root.children.push_back(c);guid=&root.children.back();}guid->data.resize(16);std::memcpy(guid->data.data(),&id,16);d->load(root.encode());prepared.waves_.push_back({L"",std::move(d),std::nullopt});break;}
        default:throw std::runtime_error("Unknown file type");
        }added.push_back({r.kind,index});
    }
    if(replace){*this=std::move(prepared);projectDirty_=true;return added;}
    // All reserves happen before the first ownership change. Native parsing,
    // allocation and name validation cannot leave half a multi-file request.
    documents_.reserve(documents_.size()+prepared.documents_.size());styles_.reserve(styles_.size()+prepared.styles_.size());bands_.reserve(bands_.size()+prepared.bands_.size());collections_.reserve(collections_.size()+prepared.collections_.size());audioPaths_.reserve(audioPaths_.size()+prepared.audioPaths_.size());chordMaps_.reserve(chordMaps_.size()+prepared.chordMaps_.size());waves_.reserve(waves_.size()+prepared.waves_.size());scripts_.reserve(scripts_.size()+prepared.scripts_.size());containers_.reserve(containers_.size()+prepared.containers_.size());toolGraphs_.reserve(toolGraphs_.size()+prepared.toolGraphs_.size());
    for(auto& d:added)switch(d.kind){
    case DocumentKind::Segment:d.index+=documents_.size();break;case DocumentKind::Style:d.index+=styles_.size();break;case DocumentKind::Band:d.index+=bands_.size();break;case DocumentKind::Collection:d.index+=collections_.size();break;case DocumentKind::AudioPath:d.index+=audioPaths_.size();break;case DocumentKind::ChordMap:d.index+=chordMaps_.size();break;case DocumentKind::Wave:d.index+=waves_.size();break;case DocumentKind::Script:d.index+=scripts_.size();break;case DocumentKind::Container:d.index+=containers_.size();break;case DocumentKind::ToolGraph:d.index+=toolGraphs_.size();break;default:break;}
    const auto append=[](auto& to,auto& from){for(auto& item:from)to.push_back(std::move(item));};
    append(documents_,prepared.documents_);append(styles_,prepared.styles_);append(bands_,prepared.bands_);append(collections_,prepared.collections_);append(audioPaths_,prepared.audioPaths_);append(chordMaps_,prepared.chordMaps_);append(waves_,prepared.waves_);append(scripts_,prepared.scripts_);append(containers_,prepared.containers_);append(toolGraphs_,prepared.toolGraphs_);projectDirty_=true;return added;
}
}
