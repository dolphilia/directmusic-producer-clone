#pragma once
#include "framework.h"
namespace producer::app {
struct NewFileRequest {DocumentKind kind;std::wstring name;Bytes initial;};
struct NewFileSelection {std::vector<DocumentKind> kinds;bool defaultNames=true;std::wstring name;};
std::optional<NewFileSelection> show_new_files(HWND,DocumentKind initial=DocumentKind::Project);
std::optional<SegmentCreationLength> show_segment_creation_length(HWND);
std::optional<SegmentCreationLength> show_segment_length(HWND,const SegmentDocument&);
std::optional<std::vector<SegmentTrackKind>> show_add_tracks(HWND,const std::wstring& segmentName);
}
