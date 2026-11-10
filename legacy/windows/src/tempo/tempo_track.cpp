#include "tempo_track.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace producer::tempo {
namespace {
std::uint32_t read32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
void write32(std::uint8_t* p, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(value >> (8 * i));
}
double readDouble(const std::uint8_t* p) {
    std::uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) bits |= std::uint64_t(p[i]) << (8 * i);
    double value;
    static_assert(sizeof(value) == sizeof(bits) && std::numeric_limits<double>::is_iec559);
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
void writeDouble(std::uint8_t* p, double value) {
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 8; ++i) p[i] = static_cast<std::uint8_t>(bits >> (8 * i));
}
}
LoadResult Track::load(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 12) return LoadResult::malformed;
    if (std::memcmp(bytes.data(), "tetr", 4)) return LoadResult::unsupported;
    if (read32(bytes.data() + 4) != bytes.size() - 8) return LoadResult::malformed;
    if (read32(bytes.data() + 8) != 16) return LoadResult::unsupported;
    if ((bytes.size() - 12) % 16) return LoadResult::malformed;
    std::vector<Event> parsed;
    for (size_t offset = 12; offset < bytes.size(); offset += 16) {
        const auto rawTime = read32(bytes.data() + offset);
        std::int32_t time;
        std::memcpy(&time, &rawTime, sizeof(time));
        const double bpm = readDouble(bytes.data() + offset + 8);
        if (!(bpm > 0.0) || !std::isfinite(bpm)) return LoadResult::unsupported;
        parsed.push_back({time, bpm});
    }
    // Original Load -> InsertEvent(0x65c4, replace=false): stable time order.
    std::stable_sort(parsed.begin(), parsed.end(), [](const Event& a, const Event& b) { return a.time < b.time; });
    events_ = std::move(parsed);
    return LoadResult::ok;
}
LoadResult decode_copy(const std::vector<std::uint8_t>& bytes, std::vector<Event>& events) {
    if (bytes.size() < 12) return LoadResult::malformed;
    if (std::memcmp(bytes.data(), "tetr", 4) || read32(bytes.data() + 8) != 24) return LoadResult::unsupported;
    if (read32(bytes.data() + 4) != bytes.size() - 8 || (bytes.size() - 12) % 24) return LoadResult::malformed;
    std::vector<Event> parsed;
    for (size_t offset = 12; offset < bytes.size(); offset += 24) {
        Event event{};
        const auto time = read32(bytes.data() + offset), tick = read32(bytes.data() + offset + 16);
        std::memcpy(&event.time, &time, 4); std::memcpy(&event.position.tick, &tick, 4);
        event.bpm = readDouble(bytes.data() + offset + 8);
        if (!(event.bpm > 0) || !std::isfinite(event.bpm)) return LoadResult::unsupported;
        parsed.push_back(event);
    }
    events = std::move(parsed); return LoadResult::ok;
}
std::vector<std::uint8_t> Track::save() const {
    const size_t count = static_cast<size_t>(std::count_if(events_.begin(),events_.end(),[](const Event& event){return event.bpm != 0;}));
    if (count > (std::numeric_limits<std::uint32_t>::max() - 12u) / 16u)
        throw std::length_error("Tempo stream exceeds 32-bit size");
    std::vector<std::uint8_t> bytes(12 + count * 16, 0);
    std::memcpy(bytes.data(), "tetr", 4);
    write32(bytes.data() + 4, static_cast<std::uint32_t>(bytes.size() - 8));
    write32(bytes.data() + 8, 16);
    size_t offset = 12;
    for (const auto& event : events_) if (event.bpm != 0) {
        write32(bytes.data() + offset, static_cast<std::uint32_t>(event.time));
        writeDouble(bytes.data() + offset + 8, event.bpm);
        offset += 16;
    }
    return bytes;
}
Query Track::query(std::int32_t at) const {
    Query result;
    for (const auto& event : events_) {
        if (event.bpm == 0) continue; // Empty-beat selection placeholders are editor-only.
        if (event.time > at) {
            // MUSIC_TIME arithmetic is 32-bit in the reference ABI.
            const auto delta = static_cast<std::uint32_t>(event.time) - static_cast<std::uint32_t>(at);
            std::memcpy(&result.next, &delta, sizeof(delta));
            break;
        }
        result.found = true;
        result.eventTime = event.time;
        result.bpm = event.bpm;
    }
    return result;
}
std::vector<std::uint8_t> Track::copy_selected(std::int32_t origin) const {
    if (events_.empty()) return {};
    const size_t count = selected_count();
    if (count > (std::numeric_limits<std::uint32_t>::max() - 12u) / 24u)
        throw std::length_error("Tempo copy stream exceeds 32-bit size");
    std::vector<std::uint8_t> bytes(12 + count * 24, 0);
    std::memcpy(bytes.data(), "tetr", 4);
    write32(bytes.data() + 4, static_cast<std::uint32_t>(bytes.size() - 8));
    write32(bytes.data() + 8, 24);
    size_t offset = 12;
    for (const auto& event : events_) if (event.selected && event.bpm != 0) {
        write32(bytes.data() + offset, static_cast<std::uint32_t>(event.time) - static_cast<std::uint32_t>(origin));
        writeDouble(bytes.data() + offset + 8, event.bpm);
        write32(bytes.data() + offset + 16, static_cast<std::uint32_t>(event.position.tick));
        // The original writes uninitialized padding at +4 and +20. Keep it zero.
        offset += 24;
    }
    return bytes;
}
void Track::select_all() { for (auto& event : events_) event.selected = event.bpm != 0; }
void Track::clear_selection() { for (auto& event : events_) event.selected = false; }
void Track::select_beats(Position start, Position end) {
    // RVA 0x64eb compares cached measure/beat fields, including both end beats.
    // Preserve the original's separate boundary-measure cases for reversed input.
    for (auto& event : events_) {
        const auto position = event.position;
        event.selected = position.measure > start.measure && position.measure < end.measure;
        if (position.measure == start.measure)
            event.selected = position.beat >= start.beat && (position.measure != end.measure || position.beat <= end.beat);
        else if (position.measure == end.measure) event.selected = position.beat <= end.beat;
    }
}
size_t Track::selected_count() const {
    return static_cast<size_t>(std::count_if(events_.begin(), events_.end(), [](const Event& event) { return event.selected && event.bpm != 0; }));
}
const Event* Track::first_selected(bool includePlaceholders) const {
    for (const auto& event : events_) if (event.selected && (includePlaceholders || event.bpm != 0)) return &event;
    return nullptr;
}
bool Track::change_selected_tempo(double bpm) {
    if (!(bpm > 0) || !std::isfinite(bpm)) return false;
    for (auto& event : events_) if (event.selected) { event.bpm = bpm; return true; }
    return false;
}
void Track::delete_selected() {
    events_.erase(std::remove_if(events_.begin(), events_.end(), [](const Event& event) { return event.selected; }), events_.end());
}
bool Track::change_tempo_at(std::int32_t at, double bpm) {
    if (!(bpm > 0) || !std::isfinite(bpm)) return false;
    auto target = std::upper_bound(events_.begin(), events_.end(), at,
        [](std::int32_t time, const Event& event) { return time < event.time; });
    if (target == events_.begin()) return false;
    do { --target; } while (target != events_.begin() && target->bpm == 0);
    if (target->bpm == 0) return false;
    if (target->bpm == bpm) return false;
    target->bpm = bpm;
    return true;
}
bool Track::move_first_selected(std::int32_t time, Position position) {
    const auto selected = std::find_if(events_.begin(), events_.end(), [](const Event& event) { return event.selected; });
    if (selected == events_.end()) return false;
    selected->position = position;
    if (selected->time == time) return true;
    Event moved = *selected;
    moved.time = time;
    events_.erase(selected);
    // Original SetData removes the node and inserts after equal-time events.
    const auto target = std::upper_bound(events_.begin(), events_.end(), time,
        [](std::int32_t at, const Event& event) { return at < event.time; });
    events_.insert(target, moved);
    return true;
}
void Track::cache_position(size_t index, Position position) { events_.at(index).position = position; }
void Track::insert_event(Event event) {
    const auto target = std::upper_bound(events_.begin(), events_.end(), event.time,
        [](std::int32_t at, const Event& existing) { return at < existing.time; });
    events_.insert(target, event);
}
void Track::replace_event(Event event) {
    const auto target = std::lower_bound(events_.begin(), events_.end(), event.time,
        [](const Event& existing, std::int32_t at) { return existing.time < at; });
    // The original replaces the first equal-time node, leaving later duplicates.
    if (target != events_.end() && target->time == event.time) *target = event;
    else events_.insert(target, event);
}
bool Track::erase_range(std::int32_t start, std::int32_t end) {
    const auto count = events_.size();
    events_.erase(std::remove_if(events_.begin(), events_.end(), [=](const Event& event) {
        return event.time >= start && event.time <= end;
    }), events_.end());
    return count != events_.size();
}
void Track::replace_events(std::vector<Event> events) {
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.time < b.time; });
    events_ = std::move(events);
}
}
