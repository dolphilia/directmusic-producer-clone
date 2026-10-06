#include "components.h"
#include "compat/producer_ids.h"
#include "compat/time_signature.h"
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <stdexcept>

namespace producer::app {
const std::array<EditorDescriptor,11>& ComponentCatalog::editors() const {
    // Original class identities are traceability keys, not COM interfaces.
    static const std::array<EditorDescriptor,11> catalog{{
        {{0xdfce860b,0xa6fa,0x11d1,{0x88,0x81,0x00,0xc0,0x4f,0xbf,0x8d,0x15}},"Segment document","DMSG",true},
        {CLSID_TempoMgr,"Tempo track","tetr",true},
        {CLSID_TimeSignatureMgr,"Explicit meter track","TIMS",true},
        {{},"Style document (header editing; Pattern editor pending)","DMST",true},
        {{},"Band instrument document (DLS download pending)","DMBD",true},
        {{},"AudioPath document (routing and ownership; full designer pending)","DMAP",true},
        {{},"Chordmap graph document (composition and full designer pending)","DMPR",true},
        {{},"Standalone Wave document (PCM and sampler ownership)","WAVE",true},
        {{},"Script source document (execution and Container editor pending)","DMSC",true},
        {{},"ToolGraph owned order/PChannel document (custom tool runtime pending)","DMTG",true},
        {{},"Standalone Container design links (runtime embedding pending)","DMCN",true}
    }};
    return catalog;
}
std::unique_ptr<SegmentDocument> ComponentCatalog::create_document(const std::string& form) const {
    if(form=="DMSG")return std::make_unique<SegmentDocument>();
    throw std::runtime_error("No reconstructed document factory for this form");
}
bool ComponentCatalog::is_segment_path(const std::wstring& path) const {
    auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});
    return ext==L".sgp"||ext==L".sgt";
}
std::unique_ptr<StyleDocument> ComponentCatalog::create_style_document(const std::string& form) const {if(form=="DMST")return std::make_unique<StyleDocument>();throw std::runtime_error("No reconstructed Style factory for this form");}
bool ComponentCatalog::is_style_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".stp"||ext==L".sty";}
std::unique_ptr<BandDocument> ComponentCatalog::create_band_document(const std::string& form) const {if(form=="DMBD")return std::make_unique<BandDocument>();throw std::runtime_error("No reconstructed Band factory for this form");}
bool ComponentCatalog::is_band_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".bnp"||ext==L".bnd";}
std::unique_ptr<AudioPathDocument> ComponentCatalog::create_audio_path_document(const std::string& form) const {if(form=="DMAP")return std::make_unique<AudioPathDocument>();throw std::runtime_error("No reconstructed AudioPath factory for this form");}
bool ComponentCatalog::is_audio_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".aup"||ext==L".aud";}
std::unique_ptr<ChordMapDocument> ComponentCatalog::create_chordmap_document(const std::string& form) const {if(form=="DMPR")return std::make_unique<ChordMapDocument>();throw std::runtime_error("No reconstructed Chordmap factory for this form");}
bool ComponentCatalog::is_chordmap_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".cdp"||ext==L".cdm";}
bool ComponentCatalog::is_collection_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".dls"||ext==L".dlp";}
bool ComponentCatalog::is_wave_path(const std::wstring& path) const {auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".wvp"||ext==L".wav";}
std::unique_ptr<WaveDocument> ComponentCatalog::create_wave_document(const std::string& form) const{if(form=="WAVE")return std::make_unique<WaveDocument>();throw std::runtime_error("No Wave document factory for form");}
bool ComponentCatalog::is_script_path(const std::wstring& path) const{auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".spp"||ext==L".spt";}
std::unique_ptr<ScriptDocument> ComponentCatalog::create_script_document(const std::string& form) const{if(form=="DMSC")return std::make_unique<ScriptDocument>();throw std::runtime_error("No Script document factory for form");}
bool ComponentCatalog::is_tool_graph_path(const std::wstring& path) const{auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".tgp"||ext==L".tgr";}
std::unique_ptr<ToolGraphDocument> ComponentCatalog::create_tool_graph_document(const std::string& form) const{if(form=="DMTG")return std::make_unique<ToolGraphDocument>();throw std::runtime_error("No ToolGraph factory for form");}
bool ComponentCatalog::is_container_path(const std::wstring& path) const{auto ext=std::filesystem::path(path).extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return ext==L".cop"||ext==L".con";}
std::unique_ptr<ContainerDocument> ComponentCatalog::create_container_document(const std::string& form) const{if(form=="DMCN")return std::make_unique<ContainerDocument>();throw std::runtime_error("No Container factory for form");}

}
