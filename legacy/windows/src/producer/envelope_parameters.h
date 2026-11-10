#pragma once
#include "dls.h"
#include <array>
#include <stdexcept>
namespace producer::app {
struct EnvelopeParameter { const wchar_t* name; std::uint16_t destination; bool level2; };
inline constexpr std::array<EnvelopeParameter,13> envelope_parameters{{
 {L"EG1 Volume — Attack",0x206,false},{L"EG1 Volume — Decay",0x207,false},
 {L"EG1 Volume — Sustain (raw)",0x20a,false},{L"EG1 Volume — Release",0x209,false},
 {L"EG1 Volume — Delay",0x20b,true},{L"EG1 Volume — Hold",0x20c,true},
 {L"EG1 Volume — Shutdown",0x20d,true},{L"EG2 Pitch — Attack",0x30a,false},
 {L"EG2 Pitch — Decay",0x30b,false},{L"EG2 Pitch — Sustain (raw)",0x30e,false},
 {L"EG2 Pitch — Release",0x30d,false},{L"EG2 Pitch — Delay",0x30f,true},
 {L"EG2 Pitch — Hold",0x310,true}
}};
inline std::optional<size_t> envelope_connection_index(const DlsArticulation& block,
 const EnvelopeParameter& parameter) {
 std::optional<size_t> found;
 for(size_t i=0;i<block.connections.size();++i){const auto& c=block.connections[i];
  if(c.destination==parameter.destination&&c.source==0&&c.control==0&&c.transform==0){
   if(found)throw std::runtime_error("Duplicate constant Envelope connections; resolve them in the Connection editor first");found=i;
  }
 }
 return found;
}
// Only the unmodulated constant is edited. Modulators, unknown records and order survive.
inline std::vector<DlsConnection> envelope_connections(const DlsArticulation& block,
 const EnvelopeParameter& parameter,std::int32_t scale) {
 if(parameter.level2&&block.chunkId!="art2")throw std::runtime_error("This parameter requires a Level 2 art2 block");
 auto values=block.connections;const auto found=envelope_connection_index(block,parameter);
 if(!found)values.push_back({0,0,parameter.destination,0,scale});
 else values[*found].scale=scale;
 return values;
}
}
