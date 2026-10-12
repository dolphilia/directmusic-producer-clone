#include "embedded_band_target.h"
#include <algorithm>
#include <stdexcept>
namespace producer::app { namespace {
Bytes identity(const Bytes& bytes){const auto root=Chunk::parse(bytes);const auto g=root.find("guid");return g&&g->data.size()==16?g->data:Bytes{};}
SegmentDocument selected(const Framework& host,size_t document,std::uint32_t groups,size_t track){
    auto d=*host.documents().at(document).document;d.select_track_group(groups,0,0,0,track);return d;
}
EmbeddedBandView view(SegmentDocument& d,EmbeddedBandTarget target,const std::vector<BandEvent>& events,size_t at){
    target.event=at;target.logical=events[at].logicalTime;target.identity=identity(events[at].band);target.lastBytes=events[at].band;
    BandDocument band;band.load(events[at].band);return {std::move(target),events[at],std::move(band),d.timeline()};
}
}
EmbeddedBandTarget make_embedded_band_target(const Framework& host,size_t document,std::uint32_t groups,size_t track,size_t event){
    auto d=selected(host,document,groups,track);const auto events=d.band_events();if(event>=events.size())throw std::runtime_error("Band event is no longer present");
    return view(d,{document,track,event,groups},events,event).target;
}
std::optional<EmbeddedBandView> read_embedded_band(const Framework& host,const EmbeddedBandTarget& target){
    try{auto d=selected(host,target.document,target.groups,target.track);const auto events=d.band_events();
        const auto matches=[&](const BandEvent& e){return target.identity.empty()?e.band==target.lastBytes:identity(e.band)==target.identity;};
        for(size_t i=0;i<events.size();++i)if(events[i].logicalTime==target.logical&&matches(events[i]))return view(d,target,events,i);
        // An independent native identity follows reordering/Undo. Ambiguous
        // copies are disabled until explicitly reselected, never guessed by index.
        size_t found=0,count=0;for(size_t i=0;i<events.size();++i)if(matches(events[i])){found=i;++count;}
        if(count==1)return view(d,target,events,found);
    }catch(const std::exception&){}return {};
}
bool update_embedded_band(Framework& host,EmbeddedBandTarget& target,std::int32_t logical,std::int32_t physical,const Bytes& bytes){
    const auto current=read_embedded_band(host,target);if(!current)return false;auto& owner=host.document(target.document);auto next=owner;
    next.select_track_group(target.groups,0,0,0,target.track);size_t at=current->target.event;if(!next.update_band(at,logical,physical,bytes,&at))return false;
    next.select_track_group(owner.selected_groups(),owner.selected_tempo_index(),owner.selected_meter_index(),owner.selected_sequence_index(),owner.selected_band_index());
    owner=std::move(next);target=make_embedded_band_target(host,target.document,target.groups,target.track,at);return true;
}
bool embedded_band_is_playing(const Bytes& snapshot,const EmbeddedBandTarget& target,std::int32_t clocks){
    try{
        if(clocks<0||!target.groups)return false;
        SegmentDocument song;song.load(snapshot);if(clocks>=song.length())return false;song.select_track_group(0xffffffffu);
        const auto all=song.tracks();if(std::count_if(all.begin(),all.end(),[](const auto& t){return t.kind==SegmentTrackKind::band;})!=1)return false;
        song.select_track_group(target.groups,0,0,0,target.track);
        const auto events=song.band_events();const BandEvent* active=nullptr;
        for(const auto& event:events)if(event.physicalTime<=clocks&&(!active||event.physicalTime>active->physicalTime))active=&event;
        if(!active||active->logicalTime!=target.logical)return false;
        if(std::count_if(events.begin(),events.end(),[&](const auto& e){return e.physicalTime==active->physicalTime;})!=1)return false;
        return target.identity.empty()?active->band==target.lastBytes:identity(active->band)==target.identity;
    }catch(const std::exception&){return false;}
}
Position band_start_position(const Timeline& axis,std::int32_t physical){
    if(physical<0)return {0,0,physical};
    if(physical<INT32_MAX){auto next=axis.position(physical+1);if(next.tick==0){next.tick=-1;return next;}}
    return axis.position(physical);
}
std::int32_t band_start_clocks(const Timeline& axis,Position p){
    const auto tick=p.tick;p.tick=0;const auto value=std::int64_t(axis.clocks(p))+tick;if(value<INT32_MIN||value>INT32_MAX)throw std::runtime_error("Band time overflow");return static_cast<std::int32_t>(value);
}
}
