#pragma once
#include "framework.h"
#include <functional>
namespace producer::app {
void show_sequence_part_editor(HWND,SegmentDocument&,const std::function<void(size_t,std::uint32_t)>& created);
struct SequenceDlsSelection {size_t collection;std::uint32_t bank,program;};
std::optional<SequenceDlsSelection> show_sequence_dls_editor(HWND,const Framework&,std::uint32_t channel,const std::optional<BandInstrument>&);
std::wstring sequence_instrument_label(const Framework&,const BandInstrument&);
}
