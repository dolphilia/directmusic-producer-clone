#pragma once
#include "document.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace producer::app {
inline bool sequence_property_same_note(Note a,Note b){return a.time==b.time&&a.duration==b.duration&&a.channel==b.channel&&a.pitch==b.pitch&&a.velocity==b.velocity;}
// Native history may reorder records. Keep a uniquely identifiable note, or
// the sole replacement in a one-note field edit. Ambiguous/bulk edits clear
// the UI selection rather than silently assigning Properties to another note.
inline std::optional<size_t> sequence_property_history_selection(const std::vector<Note>& before,const std::vector<Note>& after,size_t selected){
    if(selected>=before.size())return {};const auto note=before[selected];
    const auto count=[&](const auto& notes,Note n){return std::count_if(notes.begin(),notes.end(),[&](Note other){return sequence_property_same_note(n,other);});};
    if(count(before,note)!=1)return {};
    if(count(after,note)==1){for(size_t i=0;i<after.size();++i)if(sequence_property_same_note(note,after[i]))return i;}
    if(before.size()!=after.size()||count(after,note))return {};
    std::vector<bool> matched(after.size());std::vector<size_t> removed,added;
    for(size_t i=0;i<before.size();++i){size_t j=0;for(;j<after.size();++j)if(!matched[j]&&sequence_property_same_note(before[i],after[j])){matched[j]=true;break;}if(j==after.size())removed.push_back(i);}
    for(size_t i=0;i<after.size();++i)if(!matched[i])added.push_back(i);
    if(removed.size()!=1||removed[0]!=selected||added.size()!=1||after[added[0]].channel!=note.channel||count(after,after[added[0]])!=1)return {};return added[0];
}
// Sequence Properties uses the nearest grid and signed ticks (the original
// shows 2784 as 1:4:4:-96 at 4/4, rather than 1:4:3:96).
inline meter::Event sequence_property_meter(const Timeline& axis,std::int32_t clock){
    const auto measure=axis.position(clock).measure;auto value=axis.meters().front();
    for(const auto& m:axis.meters())if(m.measure<=measure)value=m;else break;
    if(!value.grids||(3072/value.denominator)%value.grids)throw std::runtime_error("Unsupported fractional-clock grid");return value;
}
inline GridPosition sequence_property_position(const Timeline& axis,std::int32_t clock){
    auto p=axis.grid_position(clock);const auto m=sequence_property_meter(axis,clock);const auto step=(3072/m.denominator)/m.grids;
    if(p.tick&&p.tick*2>=step&&std::int64_t(clock)+step-p.tick<=INT32_MAX){
        const auto remainder=p.tick;p=axis.grid_position(clock+step-remainder);p.tick=remainder-step;
    }return p;
}
inline std::int32_t sequence_property_clocks(const Timeline& axis,GridPosition p){
    const auto tick=p.tick;p.tick=0;const auto value=std::int64_t(axis.grid_clocks(p))+tick;
    if(value<0||value>INT32_MAX)throw std::runtime_error("Invalid Sequence position");return static_cast<std::int32_t>(value);
}
inline GridPosition sequence_property_duration(const Timeline& axis,Note note){
    const auto m=sequence_property_meter(axis,note.time);const auto beat=3072/m.denominator,grid=beat/m.grids;const auto bar=beat*m.beats;
    return {note.duration/bar,(note.duration%bar)/beat,(note.duration%beat)/grid,note.duration%grid};
}
// Fields: MIDI pitch class, octave, velocity; Start, End, Duration B:B:G:T.
// Durations use the time signature at the note's start. Variable-meter
// original parity remains a separate comparison obligation.
inline Note sequence_property_edit(const Timeline& axis,std::int32_t length,Note note,unsigned field,std::int64_t value){
    if(field>=15||value<INT32_MIN||value>INT32_MAX)throw std::runtime_error("Invalid Sequence field");
    if(field<3){
        const auto pitch=field==0?std::int64_t(note.pitch/12)*12+value:value*12+note.pitch%12;
        if(field==2){if(value<1||value>127)throw std::runtime_error("Invalid velocity");note.velocity=static_cast<std::uint8_t>(value);}
        else {if(value<0||(field==0&&value>11)||pitch>127)throw std::runtime_error("Invalid MIDI note");note.pitch=static_cast<std::uint8_t>(pitch);}
    }else{
        const auto row=(field-3)/4,column=(field-3)%4;
        auto p=row==2?sequence_property_duration(axis,note):sequence_property_position(axis,row?note.time+note.duration:note.time);
        const auto v=static_cast<std::int32_t>(value);const int base=row==2?0:1;
        if(column<3&&v<base)throw std::runtime_error("Invalid Sequence position");
        if(column==0)p.measure=v-base;if(column==1)p.beat=v-base;if(column==2)p.grid=v-base;if(column==3)p.tick=v;
        if(row==2){
            const auto m=sequence_property_meter(axis,note.time);const auto beat=3072/m.denominator,grid=beat/m.grids;
            if(p.beat>=m.beats||p.grid>=m.grids)throw std::runtime_error("Invalid duration division");
            const auto duration=std::int64_t(p.measure)*beat*m.beats+std::int64_t(p.beat)*beat+std::int64_t(p.grid)*grid+p.tick;
            if(duration<1||duration>INT32_MAX)throw std::runtime_error("Invalid duration");note.duration=static_cast<std::int32_t>(duration);
        }else{
            const auto clock=sequence_property_clocks(axis,p);
            if(!row)note.time=clock;
            else {if(clock<=note.time)throw std::runtime_error("End precedes start");note.duration=clock-note.time;}
        }
    }
    if(note.time<0||note.duration<=0||std::int64_t(note.time)+note.duration>length)throw std::runtime_error("Note exceeds Segment");return note;
}
}
