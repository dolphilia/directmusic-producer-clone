#pragma once
#include <vector>
#include <cstdint>
#include "compat/time_signature.h"

namespace producer::meter {
struct Event { LONG measure; BYTE beats; BYTE denominator; WORD grids; };
struct Query { bool found=false; TimeSignatureParam value{}; LONG next=0; };
enum class LoadResult { ok, malformed, unsupported };
// The original stores measure positions, so later meters can change the clocks
// of an earlier-loaded event. See time-signature-strip-manager.md.
class Map {
    std::vector<Event> events_;
    LONG measure_at(LONG time) const;
    LoadResult chunks(const std::vector<std::uint8_t>& bytes,size_t begin,size_t end);
    void insert(TimeSignatureParam value);
public:
    LoadResult load(const std::vector<std::uint8_t>& bytes);
    std::vector<std::uint8_t> save() const;
    Query query(LONG time) const;
    const std::vector<Event>& events() const {return events_;}
};
}
