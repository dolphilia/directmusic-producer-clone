#include "sequence_strip_model.h"
#include "sequence_parts.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace producer::app { namespace {
std::int32_t next_grid(const Timeline& axis,std::int32_t clock){
    auto p=axis.grid_position(clock);p.tick=0;const auto start=axis.grid_clocks(p);
    // One beat may have a changing number of grids. Query its meter instead of
    // assuming a fixed192-clock quantum or carrying into an invalid grid.
    const auto beat=axis.position(start);unsigned denominator=4,grids=4;
    for(const auto& m:axis.meters())if(m.measure<=beat.measure){denominator=m.denominator;grids=m.grids;}
    const auto beatStart=axis.clocks({beat.measure,beat.beat,0});
    const auto beatLength=3072/denominator;
    const auto end=std::int64_t(beatStart)+beatLength;
    const auto next=std::min(end,std::int64_t(start)+beatLength/grids);
    if(next<=clock||next>INT32_MAX)throw std::runtime_error("Sequence grid boundary overflow");return static_cast<std::int32_t>(next);
}
std::vector<SequenceBandContext> sequence_bands(const SegmentDocument& source,std::int32_t clock){
    std::vector<SequenceBandContext> out;
    for(const auto& t:source.tracks())if(t.kind==SegmentTrackKind::band){auto context=source;context.select_track_group(source.selected_groups(),0,0,0,t.index);const auto events=context.band_events();
        for(size_t i=0;i<events.size();++i)if(events[i].physicalTime<=clock)out.push_back({t.index,i,t.groups,events[i]});}
    std::stable_sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.value.physicalTime<b.value.physicalTime;});
    for(size_t i=1;i<out.size();++i)if(out[i].value.physicalTime==out[i-1].value.physicalTime&&out[i].track!=out[i-1].track)throw std::runtime_error("Ambiguous current Band across tracks");
    return out;
}
}
std::int32_t sequence_snap_clock(const Timeline& axis,std::int32_t raw,std::int32_t length,SequenceSnap snap){
    if(length<=0)throw std::runtime_error("Invalid Sequence timeline length");raw=std::clamp(raw,0,length-1);
    if(snap==SequenceSnap::none)return raw;
    if(snap==SequenceSnap::grid){auto p=axis.grid_position(raw);p.tick=0;return axis.grid_clocks(p);}
    auto p=axis.position(raw);p.tick=0;if(snap==SequenceSnap::bar)p.beat=0;return axis.clocks(p);
}
std::int32_t sequence_grid_span(const Timeline& axis,std::int32_t clock){
    auto p=axis.grid_position(clock);p.tick=0;const auto start=axis.grid_clocks(p);return next_grid(axis,start)-start;
}
std::int32_t sequence_default_duration(const Timeline& axis,std::int32_t clock,SequenceSnap snap){
    if(snap==SequenceSnap::none||snap==SequenceSnap::grid)return sequence_grid_span(axis,clock);
    const auto p=axis.position(clock);unsigned beats=4,denominator=4;
    for(const auto& m:axis.meters())if(m.measure<=p.measure){beats=m.beats;denominator=m.denominator;}
    return static_cast<std::int32_t>((3072/denominator)*(snap==SequenceSnap::bar?beats:1));
}
bool move_sequence_cursor(const Timeline& axis,std::int32_t length,SequenceCursor& cursor,int horizontal,int vertical){
    const auto oldClock=cursor.clock;const auto oldPitch=cursor.pitch;
    cursor.pitch=static_cast<unsigned>(std::clamp<std::int64_t>(std::int64_t(cursor.pitch)+vertical,0,127));
    cursor.clock=std::clamp(cursor.clock,0,std::max(0,length-1));
    if(horizontal&&cursor.snap!=SequenceSnap::none){
        for(int i=0;i<std::abs(horizontal);++i){
            if(cursor.snap==SequenceSnap::grid){
                // Switching from None retains the marker's tick offset. Arrow
                // navigation advances one grid without quantizing that offset.
                const auto span=sequence_grid_span(axis,horizontal<0&&cursor.clock?cursor.clock-1:cursor.clock);
                const auto next=std::max<std::int64_t>(0,std::int64_t(cursor.clock)+(horizontal<0?-span:span));
                if(next>=length)break;cursor.clock=static_cast<std::int32_t>(next);continue;
            }
            if(horizontal<0){if(!cursor.clock)break;cursor.clock=sequence_snap_clock(axis,cursor.clock-1,length,cursor.snap);}
            else{std::int64_t next=cursor.clock;
                next=std::int64_t(sequence_snap_clock(axis,cursor.clock,length,cursor.snap))+sequence_default_duration(axis,cursor.clock,cursor.snap);
                if(next>=length)break;cursor.clock=static_cast<std::int32_t>(next);}}
    }return cursor.clock!=oldClock||cursor.pitch!=oldPitch;
}
Note sequence_drag_note(const Timeline& axis,std::int32_t length,Note note,SequenceNoteHandle handle,std::int64_t clockDelta,int pitchDelta,int velocityDelta,SequenceSnap snap){
    if(length<=0||note.time<0||note.duration<=0||std::int64_t(note.time)+note.duration>length||note.pitch>127||!note.velocity||note.velocity>127||note.channel>=0xfffffffcu)throw std::runtime_error("Invalid Sequence drag source");
    clockDelta=std::clamp<std::int64_t>(clockDelta,-INT32_MAX,INT32_MAX);const auto end=std::int64_t(note.time)+note.duration;
    if(handle==SequenceNoteHandle::velocity){note.velocity=static_cast<std::uint8_t>(std::clamp<std::int64_t>(std::int64_t(note.velocity)+velocityDelta,1,127));return note;}
    if(handle==SequenceNoteHandle::end){note.duration=static_cast<std::int32_t>(std::clamp<std::int64_t>(end+clockDelta,std::int64_t(note.time)+1,length)-note.time);return note;}
    const auto maximum=handle==SequenceNoteHandle::start?end-1:std::int64_t(length)-note.duration;
    auto at=sequence_snap_clock(axis,static_cast<std::int32_t>(std::clamp<std::int64_t>(std::int64_t(note.time)+clockDelta,0,maximum)),length,snap);
    note.time=at;if(handle==SequenceNoteHandle::start)note.duration=static_cast<std::int32_t>(end-at);
    else note.pitch=static_cast<std::uint8_t>(std::clamp<std::int64_t>(std::int64_t(note.pitch)+pitchDelta,0,127));return note;
}
unsigned sequence_velocity_value(Note note,std::int32_t first,std::int32_t last,const SequenceVelocityOptions& options){
    if(!note.velocity||note.velocity>127||first<0||last<first||note.time<first||note.time>last||options.minimum<1||options.maximum>127||options.minimum>options.maximum||options.start<-1000||options.start>1000||options.end<-1000||options.end>1000)throw std::runtime_error("Invalid velocity edit options");
    if(options.limit)return std::clamp<unsigned>(note.velocity,options.minimum,options.maximum);
    const double fraction=last==first?0.:double(std::int64_t(note.time)-first)/double(std::int64_t(last)-first);
    const double offset=options.start+(options.end-options.start)*fraction;
    const auto value=options.percent?note.velocity*(100.+offset)/100.:note.velocity+offset;
    return static_cast<unsigned>(std::clamp<long long>(std::llround(value),1,127));
}
Note sequence_quantize_note(const Timeline& axis,std::int32_t length,Note note,const SequenceQuantizeOptions& options){
    if(length<=0||note.time<0||note.duration<=0||std::int64_t(note.time)+note.duration>length||note.channel>=0xfffffffcu||note.pitch>127||!note.velocity||note.velocity>127||options.divisions<1||options.divisions>24||options.strength>100)throw std::runtime_error("Invalid Quantize options or source note");
    const auto position=axis.position(note.time);unsigned denominator=4;
    for(const auto& m:axis.meters())if(m.measure<=position.measure)denominator=m.denominator;
    const std::int64_t beat=3072/denominator,divisions=options.divisions;
    const auto nearest=[&](std::int64_t value){const auto division=(value*divisions*2+beat)/(beat*2);return (division*beat+divisions/2)/divisions;};
    // Original native observations: -96 at33% moves -31 clocks; +192 at1%
    // moves +1 and -96 at1% moves0. Truncate the signed delta towards zero.
    const auto interpolate=[&](std::int64_t value,std::int64_t target){return value+(target-value)*options.strength/100;};
    if(options.start){const auto begin=axis.clocks({position.measure,position.beat,0});const auto target=std::int64_t(begin)+nearest(position.tick);const auto time=interpolate(note.time,target);if(time<0||time>INT32_MAX)throw std::runtime_error("Quantized start out of range");note.time=static_cast<std::int32_t>(time);}
    if(options.duration){const auto duration=interpolate(note.duration,std::max<std::int64_t>(1,nearest(note.duration)));if(duration<1||duration>INT32_MAX)throw std::runtime_error("Quantized duration out of range");note.duration=static_cast<std::int32_t>(duration);}
    if(std::int64_t(note.time)+note.duration>length)throw std::runtime_error("Quantized note exceeds Segment length");return note;
}
SequencePartInfo sequence_part_info(const SegmentDocument& source,size_t index){
    auto context=source;context.select_track_group(source.selected_groups(),source.selected_tempo_index(),source.selected_meter_index(),index,source.selected_band_index());
    SequencePartInfo result;
    const auto tree=Chunk::parse(source.save_bytes());if(const auto tracks=tree.find("LIST","trkl")){size_t at=0;for(const auto& track:tracks->children)if(is_sequence_track(track)&&(read32(track.find("trkh")->data,20)&source.selected_groups())&&at++==index){
        result.channels=sequence_part_channels(track);if(const auto info=track.find("LIST","UNFO"))if(const auto name=info->find("UNAM")){const auto text=decode_utf16(name->data);if(!text.empty())result.name=text;}break;}}
    return result;
}
std::optional<SequenceBandContext> current_sequence_band(const SegmentDocument& source,std::int32_t clock){const auto bands=sequence_bands(source,clock);if(bands.empty())return {};return bands.back();}
std::optional<BandInstrument> sequence_patch(const SegmentDocument& source,std::int32_t clock,std::uint32_t channel){
    std::optional<BandInstrument> result;for(const auto& e:sequence_bands(source,clock)){BandDocument band;band.load(e.value.band);for(const auto& i:band.instruments())if(i.pchannel==channel&&(i.flags&1))result=i;}return result;
}
bool assign_sequence_gm_patch(SegmentDocument& source,std::int32_t clock,std::uint32_t channel,std::uint32_t patch){
    if(clock<0||clock>=source.length()||channel>=0xfffffffcu||(patch&~0x807f7f7fu))return false;
    const auto current=current_sequence_band(source,clock);BandDocument band;if(current)band.load(current->value.band);else band=BandDocument::inactive_default();
    if(!band.assign_gm_patch(channel,patch))return false;
    if(current){auto next=source;next.select_track_group(source.selected_groups(),source.selected_tempo_index(),source.selected_meter_index(),source.selected_sequence_index(),current->track);
        if(!next.update_band(current->event,current->value.logicalTime,current->value.physicalTime,band.save_bytes()))return false;source=std::move(next);return true;}
    return source.set_band(0,band.save_bytes());
}
bool assign_sequence_dls_patch(SegmentDocument& source,std::int32_t clock,std::uint32_t channel,const CollectionReference& reference,std::uint32_t bank,std::uint32_t program,bool emptyCollection){
    if(clock<0||clock>=source.length()||channel>=0xfffffffcu||(bank&~0x80007f7fu)||program>127)return false;
    const auto current=current_sequence_band(source,clock);BandDocument band;if(current)band.load(current->value.band);else band=BandDocument::inactive_default();
    const auto patch=(bank&0x80000000u)|((bank&0x7f7fu)<<8)|program;
    if(!band.assign_dls_patch(channel,patch,reference,emptyCollection))return false;
    if(current){auto next=source;next.select_track_group(source.selected_groups(),source.selected_tempo_index(),source.selected_meter_index(),source.selected_sequence_index(),current->track);if(!next.update_band(current->event,current->value.logicalTime,current->value.physicalTime,band.save_bytes()))return false;source=std::move(next);return true;}
    return source.set_band(0,band.save_bytes());
}
Bytes sequence_audition_snapshot(const SegmentDocument& source,Note note){
    if(note.channel>=0xfffffffcu||note.pitch>127||!note.velocity||note.velocity>127)throw std::runtime_error("Invalid Sequence audition note");
    std::uint32_t patch=note.channel==9?0x80000000u:0;unsigned pan=64,volume=100;Bytes collection;bool useDefaultGm=false;
    for(const auto& e:sequence_bands(source,note.time)){BandDocument band;band.load(e.value.band);for(const auto& i:band.instruments())if(i.pchannel==note.channel){if(i.flags&1){patch=i.patch;collection=i.collectionReference;useDefaultGm=(i.flags&0x1000u)!=0;}if(i.flags&32)pan=i.pan;if(i.flags&64)volume=i.volume;}}
    BandDocument band;if(!band.add_gm_instrument(patch,note.channel,pan,volume))throw std::runtime_error("Cannot create audition Band");
    if(!collection.empty()&&!band.set_collection_reference(0,collection_reference(collection),useDefaultGm))throw std::runtime_error("Cannot retain audition DLS assignment");
    auto preview=SegmentDocument::create_empty({false,1,2000},L"Note Audition");if(!preview.set_band(0,band.save_bytes()))throw std::runtime_error("Cannot create audition Band track");
    note.time=0;note.duration=384;if(!preview.add_note(note))throw std::runtime_error("Cannot create audition note");const auto path=source.audio_path();if(!path.empty())preview.set_audio_path(path);return preview.save_bytes();
}
}
