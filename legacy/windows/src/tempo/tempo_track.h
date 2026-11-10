#pragma once
#include <cstdint>
#include <vector>

namespace producer::tempo {
struct Position {
    std::int32_t measure = 0;
    std::int32_t beat = 0;
    std::int32_t tick = 0;
};
struct Event {
    std::int32_t time;
    double bpm; // Internal zero-tempo placeholders represent empty-beat selection.
    bool selected = false;
    Position position{};
    bool dragged = false; // Source items survive selection changes during OLE transfer.
};
struct Query {
    bool found = false;
    std::int32_t eventTime = 0;
    double bpm = 120.0;
    std::int32_t next = 0;
};

// First implementation of the observed normal tetr format, not a complete
// IPersistStream replacement. Unknown chunks/record sizes and non-positive or
// non-finite tempos remain unsupported pending reference observations.
enum class LoadResult { ok, malformed, unsupported };
LoadResult decode_copy(const std::vector<std::uint8_t>& bytes, std::vector<Event>& events);
class Track {
public:
    LoadResult load(const std::vector<std::uint8_t>& bytes);
    std::vector<std::uint8_t> save() const;
    std::vector<std::uint8_t> copy_selected(std::int32_t origin) const;
    Query query(std::int32_t at) const;
    const std::vector<Event>& events() const { return events_; }
    void select_all();
    void clear_selection();
    void select_beats(Position start, Position end);
    size_t selected_count() const;
    const Event* first_selected(bool includePlaceholders = false) const;
    bool change_selected_tempo(double bpm);
    bool change_tempo_at(std::int32_t at, double bpm);
    bool move_first_selected(std::int32_t time, Position position);
    void cache_position(size_t index, Position position);
    void replace_events(std::vector<Event> events);
    void insert_event(Event event);
    void replace_event(Event event);
    bool erase_range(std::int32_t start, std::int32_t end);
    void delete_selected();
private:
    std::vector<Event> events_{{0, 120.0}};
};
}
