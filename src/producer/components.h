#pragma once
#include "document.h"
#include "audio_path.h"
#include <array>
#include <memory>

namespace producer::app {
struct EditorDescriptor { GUID originalClass; const char* responsibility; const char* form; bool implemented; };
// Source-only factories never fall back to COM registration or DLL loading.
class ComponentCatalog {
public:
    const std::array<EditorDescriptor,6>& editors() const;
    std::unique_ptr<SegmentDocument> create_document(const std::string& form) const;
    bool is_segment_path(const std::wstring& path) const;
    bool is_style_path(const std::wstring& path) const;
    std::unique_ptr<StyleDocument> create_style_document(const std::string& form) const;
    bool is_band_path(const std::wstring& path) const;
    bool is_audio_path(const std::wstring& path) const;
    std::unique_ptr<AudioPathDocument> create_audio_path_document(const std::string& form) const;
    bool is_collection_path(const std::wstring& path) const;
    std::unique_ptr<BandDocument> create_band_document(const std::string& form) const;
};
}
