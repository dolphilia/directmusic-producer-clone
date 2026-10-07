#include "document.h"
#include "audio_path.h"
#include "tool_graph.h"
#include "compat/producer_ids.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <objbase.h>
#include <stdexcept>

namespace producer::app {
namespace {
Chunk leaf(std::string id,Bytes b) { Chunk c; c.id=std::move(id); c.data=std::move(b); return c; }
Chunk list(std::string id,std::string type,std::vector<Chunk> children) { Chunk c; c.id=std::move(id);c.type=std::move(type);c.children=std::move(children);return c; }
Bytes tempo_header() {
    Bytes h(32); const GUID id={0xd2ac2885,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
    std::memcpy(h.data(),&id,16);put32(h,20,1); std::memcpy(h.data()+24,"tetr",4);return h;
}
Bytes meter_header() {
    Bytes h(32);const GUID id={0xd2ac2888,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
    std::memcpy(h.data(),&id,16);put32(h,20,1);std::memcpy(h.data()+28,"TIMS",4);return h;
}
template<class C> auto selected_track(C& root,const Bytes& expected,std::uint32_t groups,size_t index) -> decltype(root.find("LIST","trkl")) {
    const auto tracks=root.find("LIST","trkl");if(!tracks){if(index)throw std::runtime_error("Track index out of range for selected groups");return nullptr;}
    size_t count=0;for(auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){
        const auto h=track.find("trkh");if(!h||h->data.size()<32||std::count_if(track.children.begin(),track.children.end(),[](const Chunk& c){return c.id=="trkh";})!=1)throw std::runtime_error("Track header missing, ambiguous or truncated");
        if(!std::equal(expected.begin(),expected.begin()+16,h->data.begin()))continue;
        const auto bits=read32(h->data,20);if(!bits)throw std::runtime_error("Track has no groups");
        if((bits&groups)&&count++==index)return &track;
    }if(index)throw std::runtime_error("Track index out of range for selected groups");return nullptr;
}
bool valid_tempo(std::int32_t time,double bpm,std::int32_t length) { return time>=0 && time<length && std::isfinite(bpm) && bpm>=1 && bpm<=1000; }
// Assign only a newly appended track. Imported headers (including ties) are
// preserved; runtime enumeration is not a general editor-index mapping.
void append_position(Chunk& fresh,const Chunk& tracks) {
    std::uint32_t position=0;
    for(const auto& track:tracks.children)if(track.id=="RIFF"&&track.type=="DMTK") {
        const auto h=track.find("trkh");
        if(!h||h->data.size()<32)throw std::runtime_error("Cannot assign position with incomplete track header");
        const auto old=read32(h->data,16);
        if(old==UINT32_MAX)throw std::runtime_error("Track position exhausted");
        position=std::max(position,old+1);
    }
    put32(fresh.find("trkh")->data,16,position);
}
}
SegmentDocument::SegmentDocument() {
    Bytes h(40); put32(h,4,30720); put32(h,20,0x2000); // DMUS_SEGF_MEASURE, not ticks per beat.
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Segment GUID creation failed");
    Bytes identity(16);std::memcpy(identity.data(),&id,16);
    const auto track=Chunk::parse(list("RIFF","DMTK",{leaf("trkh",tempo_header()),leaf("tetr",{})}).encode());
    root_=list("RIFF","DMSG",{leaf("segh",h),leaf("guid",identity),leaf("vers",Bytes(8)),list("LIST","trkl",{track})});
    hasTempo_=true; *tempo_chunk()=leaf("tetr",Bytes{});
    auto stream=tempo_.save(); tempo_chunk()->data.assign(stream.begin()+8,stream.end()); saved_=save_bytes();
}
Chunk* SegmentDocument::tempo_chunk() {
    auto track=selected_track(root_,tempo_header(),selectedGroups_,tempoIndex_);if(!track)return nullptr;
    const auto data=track->find("tetr");if(!data||std::count_if(track->children.begin(),track->children.end(),[](const Chunk& c){return c.id=="tetr";})!=1)throw std::runtime_error("Tempo data missing or ambiguous");return data;
}
void SegmentDocument::import(const Bytes& bytes) {
    auto root=Chunk::parse(bytes); if(root.type!="DMSG")throw std::runtime_error("Expected DMSG segment");
    const auto header=root.find("segh");if(!header || header->data.size()<24 || read32(header->data,4)>0x7fffffff)throw std::runtime_error("Unsupported segment header");
    if(std::count_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id=="segh";})!=1 ||
       std::count_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="trkl";})>1)throw std::runtime_error("Ambiguous segment containers");
    if(const auto tracks=root.find("LIST","trkl")){const auto paramHeader=param_control_track().find("trkh")->data;for(const auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){const auto h=track.find("trkh");const bool paramClass=h&&h->data.size()>=16&&std::equal(paramHeader.begin(),paramHeader.begin()+16,h->data.begin());if(paramClass||track.find("LIST","prmt"))(void)param_control_objects(track);}}
    if(const auto tracks=root.find("LIST","trkl")){const auto expected=segment_trigger_track().find("trkh")->data;for(const auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){const auto h=track.find("trkh");if((h&&h->data.size()>=16&&std::equal(expected.begin(),expected.begin()+16,h->data.begin()))||track.find("LIST","segt"))(void)segment_triggers(track);}}
    if(const auto tracks=root.find("LIST","trkl")){const auto expected=script_track().find("trkh")->data;for(const auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){const auto h=track.find("trkh");if((h&&h->data.size()>=16&&std::equal(expected.begin(),expected.begin()+16,h->data.begin()))||track.find("LIST","scrt"))(void)script_events(track);}}
    root_=std::move(root);(void)tool_graph(); auto chunk=tempo_chunk(); hasTempo_=chunk!=nullptr;
    if (chunk) { if(tempo_.load(chunk->encode())!=tempo::LoadResult::ok)throw std::runtime_error("Unsupported tempo data");
        for(const auto& e:tempo_.events())if(e.time<0)throw std::runtime_error("Negative tempo positions are not yet displayed"); }
    else tempo_.replace_events({});
    bool explicitMeter=false;(void)explicit_timeline(explicitMeter);
    (void)app::style_references(root_); // Validate references without inventing a meter before resolution.
    (void)app::chordmap_references(root_);
}
void SegmentDocument::load(const Bytes& bytes) {
    SegmentDocument next; next.import(bytes); next.saved_=bytes; next.dirty_=false; *this=std::move(next);
}
void SegmentDocument::select_track_group(std::uint32_t groups,size_t tempoIndex,size_t meterIndex,size_t sequenceIndex,size_t bandIndex){
    if(!groups)throw std::runtime_error("Track group mask must be nonzero");auto next=*this;next.selectedGroups_=groups;next.tempoIndex_=tempoIndex;next.meterIndex_=meterIndex;next.sequenceIndex_=sequenceIndex;next.bandIndex_=bandIndex;next.import(save_bytes());(void)next.notes();(void)next.band_events();if(next.range_.strips&TimelineLyric){try{(void)next.lyrics(next.range_.lyricTrack);}catch(const std::exception&){next.range_.lyricTrack=0;}}if(!next.styles_.empty())(void)next.timeline();*this=std::move(next);
}
std::int32_t SegmentDocument::length() const { return static_cast<std::int32_t>(read32(root_.find("segh")->data,4)); }
void SegmentDocument::save(const std::wstring& path) { const auto b=save_bytes(); write_file_atomic(path,b);saved_=b;dirty_=false; }
void SegmentDocument::relocate_context(const std::wstring& oldDirectory,const std::wstring& newDirectory,const std::vector<StyleCatalogEntry>& styles,const std::vector<CollectionEntry>& collections){
    auto rebase=[&](const Bytes& bytes){
        if(bytes.empty())return bytes;
        const auto dependencies=resolve_collections(document_collection_references(bytes),oldDirectory,collections);
        auto result=relocate_collection_references(bytes,dependencies,newDirectory);
        return relocate_style_references(result,resolve_styles(producer::app::style_references(Chunk::parse(bytes)),oldDirectory,styles),newDirectory);
    };
    auto next=*this;const auto bytes=rebase(save_bytes());
    for(auto& snapshot:next.undo_)snapshot=rebase(snapshot);
    for(auto& snapshot:next.redo_)snapshot=rebase(snapshot);
    // The disk checkpoint can deliberately still name a dependency's old file.
    // It is replaced by save() after the write, not resolved as an edit snapshot.
    next.import(bytes);next.resolve_style_context(newDirectory,styles);next.dirty_=next.save_bytes()!=next.saved_;*this=std::move(next);
}
void SegmentDocument::retarget_style(const std::wstring& directory,const std::wstring& oldPath,const std::wstring& newPath,const Bytes& style){
    auto rebase=[&](const Bytes& bytes){return retarget_style_references(bytes,directory,oldPath,newPath,style);};auto next=*this;const auto bytes=rebase(save_bytes());for(auto& snapshot:next.undo_)snapshot=rebase(snapshot);for(auto& snapshot:next.redo_)snapshot=rebase(snapshot);
    // This dependency move has not saved the Segment: keep the disk checkpoint.
    next.import(bytes);next.dirty_=bytes!=next.saved_;*this=std::move(next);
}
bool SegmentDocument::relocate_collections(const std::vector<ResolvedCollection>& dependencies,const std::wstring& directory){
    const auto before=save_bytes(),after=relocate_collection_references(before,dependencies,directory);if(before==after)return false;
    auto next=*this;next.import(after);next.record_edit(before);*this=std::move(next);return true;
}
void SegmentDocument::select(size_t index) { auto events=tempo_.events(); if(index>=events.size())throw std::out_of_range("Tempo selection");for(size_t i=0;i<events.size();++i)events[i].selected=i==index;tempo_.replace_events(std::move(events)); }
void SegmentDocument::select_all() { tempo_.select_all(); }
void SegmentDocument::commit_tempo(const Bytes& before) {
    auto stream=tempo_.save();
    if(!hasTempo_) {
        auto tracks=root_.find("LIST","trkl"); if(!tracks) {root_.children.push_back(list("LIST","trkl",{}));tracks=&root_.children.back();}
        auto header=tempo_header();put32(header,20,selectedGroups_);tracks->children.push_back(list("RIFF","DMTK",{leaf("trkh",header),leaf("tetr",{})}));hasTempo_=true;
    }
    tempo_chunk()->data.assign(stream.begin()+8,stream.end());
    record_edit(before);
}
void SegmentDocument::record_edit(const Bytes& before) {
    const auto after=save_bytes(); if(before==after)return;
    undo_.push_back(before); if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();dirty_=after!=saved_;
}
bool SegmentDocument::add_tempo(std::int32_t time,double bpm) {
    if(!valid_tempo(time,bpm,length()))return false;
    const auto before=save_bytes();auto next=*this;next.tempo_.clear_selection();next.tempo_.replace_event({time,bpm,true});next.commit_tempo(before);const bool changed=before!=next.save_bytes();*this=std::move(next);return changed;
}
bool SegmentDocument::change_selected(double bpm) {
    if(!std::isfinite(bpm)||bpm<1||bpm>1000)return false;
    const auto before=save_bytes();auto next=*this;if(!next.tempo_.change_selected_tempo(bpm))return false;next.commit_tempo(before);const bool changed=before!=next.save_bytes();*this=std::move(next);return changed;
}
bool SegmentDocument::delete_selected() {
    const auto before=save_bytes();auto next=*this;next.tempo_.delete_selected();next.commit_tempo(before);const bool changed=before!=next.save_bytes();*this=std::move(next);return changed;
}
Bytes SegmentDocument::copy() const {
    const auto e=tempo_.first_selected();return e?tempo_.copy_selected(e->time):Bytes{};
}
bool SegmentDocument::paste(const Bytes& bytes,std::int32_t at) {
    std::vector<tempo::Event> events;
    if(tempo::decode_copy(bytes,events)!=tempo::LoadResult::ok || events.empty())return false;
    for(auto& e:events) {const auto time=std::int64_t(e.time)+at;if(time>INT32_MAX||time<0||!valid_tempo(static_cast<std::int32_t>(time),e.bpm,length()))return false;e.time=static_cast<std::int32_t>(time);e.selected=true;}
    const auto before=save_bytes();auto next=*this;next.tempo_.clear_selection();for(const auto& e:events)next.tempo_.replace_event(e);next.commit_tempo(before);const bool changed=before!=next.save_bytes();*this=std::move(next);return changed;
}
bool SegmentDocument::undo() {
    if(undo_.empty())return false;
    auto next=*this; next.import(undo_.back());next.resolve_style_history(*this);next.redo_.push_back(save_bytes());next.undo_.pop_back();next.dirty_=next.save_bytes()!=saved_;*this=std::move(next);return true;
}
bool SegmentDocument::select_range(TimelineSelection value){
    if(value.begin<0||value.end<=value.begin||value.end>length()||!value.strips||(value.strips&~TimelineAll))return false;try{if(value.strips&TimelineLyric)(void)lyrics(value.lyricTrack);if(value.strips&TimelineMarker)(void)markers(value.markerTrack);if(value.strips&TimelineMute)(void)mutes(value.muteTrack);}catch(const std::exception&){return false;}range_=value;return true;
}
Bytes SegmentDocument::copy_range() const {
    if(range_.begin<0||range_.end<=range_.begin||range_.end>length()||!range_.strips||(range_.strips&~TimelineAll))throw std::runtime_error("Select a Timeline range and strips first");
    TimelineClipboard value;value.span=range_.end-range_.begin;value.strips=range_.strips;
    if(value.strips&TimelineTempo){auto track=tempo_;auto events=track.events();for(auto& e:events)e.selected=e.time>=range_.begin&&e.time<range_.end&&e.bpm>0;track.replace_events(std::move(events));value.tempo=track.copy_selected(range_.begin);if(value.tempo.empty()){tempo::Track empty;empty.clear_selection();value.tempo=empty.copy_selected(0);}}
    if(value.strips&TimelineSequence){const auto fresh=sequence_track();const auto target=selected_track(root_,fresh.find("trkh")->data,selectedGroups_,sequenceIndex_);const auto data=target?target->find("seqt"):fresh.find("seqt");if(!data|| (target&&std::count_if(target->children.begin(),target->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1))throw std::runtime_error("Sequence payload missing or ambiguous");value.sequence=sequence_copy_range(data->data,range_.begin,range_.end);}
    if(value.strips&TimelineLyric){const auto fresh=lyric_track();const auto target=selected_track(root_,fresh.find("trkh")->data,selectedGroups_,range_.lyricTrack);value.lyric=copy_lyric_range(target?*target:fresh,range_.begin,range_.end);}
    if(value.strips&TimelineMarker){const auto fresh=marker_track();const auto target=selected_track(root_,fresh.find("trkh")->data,selectedGroups_,range_.markerTrack);value.marker=copy_marker_range(target?*target:fresh,range_.begin,range_.end);}
    if(value.strips&TimelineMute){const auto fresh=mute_track();const auto target=selected_track(root_,fresh.find("trkh")->data,selectedGroups_,range_.muteTrack);value.mute=copy_mute_range(target?*target:fresh,range_.begin,range_.end);}
    return encode_timeline_clipboard(value);
}
bool SegmentDocument::delete_range(){
    try{(void)copy_range();const auto before=save_bytes();auto next=*this;
        if(range_.strips&TimelineTempo){auto events=next.tempo_.events();events.erase(std::remove_if(events.begin(),events.end(),[&](const tempo::Event& e){return e.time>=range_.begin&&e.time<range_.end;}),events.end());next.tempo_.replace_events(std::move(events));if(next.hasTempo_)next.commit_tempo(before);}
        if(range_.strips&TimelineSequence){const auto fresh=sequence_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,sequenceIndex_);if(target)target->find("seqt")->data=sequence_delete_range(target->find("seqt")->data,range_.begin,range_.end);}
        if(range_.strips&TimelineLyric){const auto fresh=lyric_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,range_.lyricTrack);if(target)delete_lyric_range(*target,range_.begin,range_.end);}
        if(range_.strips&TimelineMarker){const auto fresh=marker_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,range_.markerTrack);if(target)delete_marker_range(*target,range_.begin,range_.end);}
        if(range_.strips&TimelineMute){const auto fresh=mute_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,range_.muteTrack);if(target)delete_mute_range(*target,range_.begin,range_.end);}
        if(next.save_bytes()==before)return false;next.undo_=undo_;next.redo_=redo_;next.record_edit(before);*this=std::move(next);return true;
    }catch(const std::exception&){return false;}
}
bool SegmentDocument::paste_range(const Bytes& bytes,std::int32_t at,bool overwrite){
    try{const auto value=decode_timeline_clipboard(bytes);if(value.strips!=range_.strips||at<0||std::int64_t(at)+value.span>length())return false;std::vector<tempo::Event> events;
        if(value.strips&TimelineTempo){if(tempo::decode_copy(value.tempo,events)!=tempo::LoadResult::ok)return false;for(auto& e:events){if(e.time<0||e.time>=value.span||!valid_tempo(e.time+at,e.bpm,length()))return false;e.time+=at;e.selected=true;}}
        if(value.strips&TimelineSequence)for(const auto& note:sequence_notes(value.sequence))if(note.time<0||note.time>=value.span||note.duration<=0||std::int64_t(at)+note.time+note.duration>length()||note.channel>=0xfffffffcu||note.pitch>127||!note.velocity||note.velocity>127)return false;
        const auto before=save_bytes();auto next=*this;
        if(value.strips&TimelineTempo){if(overwrite)next.tempo_.erase_range(at,at+value.span-1);next.tempo_.clear_selection();for(const auto& e:events)next.tempo_.replace_event(e);if(next.hasTempo_||!events.empty())next.commit_tempo(before);}
        if((value.strips&TimelineSequence)&&(!sequence_range_empty(value.sequence)||(overwrite&&selected_track(next.root_,sequence_track().find("trkh")->data,selectedGroups_,sequenceIndex_)))){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=sequence_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,sequenceIndex_);if(!target){put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));target=&tracks->children.back();}auto data=target->find("seqt");if(!data||std::count_if(target->children.begin(),target->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1)return false;data->data=sequence_paste_range(data->data,value.sequence,at,value.span,overwrite,length());}
        if(value.strips&TimelineLyric){auto fresh=lyric_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,range_.lyricTrack);if(target)paste_lyric_range(*target,value.lyric,at,value.span,overwrite,length());else if(!lyric_range_empty(value.lyric,at,value.span,length())){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}put32(fresh.find("trkh")->data,20,selectedGroups_);paste_lyric_range(fresh,value.lyric,at,value.span,overwrite,length());append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));}}
        const auto pasteTyped=[&](Chunk fresh,size_t index,const Bytes& payload,const auto& empty,const auto& paste){
            auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,index);
            if(target)paste(*target,payload,at,value.span,overwrite,length());
            else if(!empty(payload,at,value.span,length())){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}put32(fresh.find("trkh")->data,20,selectedGroups_);paste(fresh,payload,at,value.span,overwrite,length());append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));}
        };
        if(value.strips&TimelineMarker)pasteTyped(marker_track(),range_.markerTrack,value.marker,marker_range_empty,paste_marker_range);
        if(value.strips&TimelineMute)pasteTyped(mute_track(),range_.muteTrack,value.mute,mute_range_empty,paste_mute_range);
        if(next.save_bytes()==before)return false;next.undo_=undo_;next.redo_=redo_;next.range_={at,at+value.span,value.strips,range_.lyricTrack,range_.markerTrack,range_.muteTrack};next.record_edit(before);*this=std::move(next);return true;
    }catch(const std::exception&){return false;}
}
bool SegmentDocument::move_range(std::int32_t at) {
    try {
        const auto bytes=copy_range();
        const auto span=range_.end-range_.begin;
        if(at<0||at==range_.begin||std::int64_t(at)+span>length())return false;
        const auto before=save_bytes();auto next=*this;
        // Work on a private document: either strip may reject after deletion.
        // Collapsing its intermediate history makes relocation one edit.
        if(!next.delete_range()||!next.paste_range(bytes,at)||next.save_bytes()==before)return false;
        next.undo_=undo_;next.redo_=redo_;next.record_edit(before);
        *this=std::move(next);return true;
    }catch(const std::exception&){return false;}
}
bool SegmentDocument::redo() {
    if(redo_.empty())return false;
    auto next=*this;next.import(redo_.back());next.resolve_style_history(*this);next.undo_.push_back(save_bytes());next.redo_.pop_back();next.dirty_=next.save_bytes()!=saved_;*this=std::move(next);return true;
}
void SegmentDocument::resolve_style_history(const SegmentDocument& previous){
    const auto a=previous.style_references(),b=style_references();bool same=a.size()==b.size();for(size_t i=0;same&&i<a.size();++i)same=a[i].time==b[i].time&&a[i].groups==b[i].groups&&a[i].hasId==b[i].hasId&&a[i].objectId==b[i].objectId&&a[i].filename==b[i].filename&&a[i].name==b[i].name;
    if(!same)resolve_style_context(styleDirectory_,styleCatalog_,styleRuntimeReferences_);
}
Timeline SegmentDocument::explicit_timeline(bool& found) const {
    found=false;
    Timeline view;
    // Direct, explicit time-signature tracks only. Style-derived meters are
    // not inferred from opaque Style references.
    const auto track=selected_track(root_,meter_header(),selectedGroups_,meterIndex_);if(!track)return view;
    const auto wrapped=track->find("LIST","TIMS"),direct=track->find("tims");if(wrapped&&direct)throw std::runtime_error("Ambiguous meter payload");
    if(wrapped)view.load_meter(wrapped->encode());else if(direct)view.load_meter(direct->encode());else throw std::runtime_error("Meter track data missing");found=true;return view;
}
namespace {
template<class C> auto trigger_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","segt")){return selected_track(root,segment_trigger_track().find("trkh")->data,groups,index);}
bool trigger_range(const SegmentDocument& doc,const SegmentTrigger& e){return e.logical>=0&&e.physical>=0&&e.logical<doc.length()&&e.physical<doc.length();}
}
std::vector<SegmentTrigger> SegmentDocument::triggers(size_t index) const{const auto t=trigger_selected(root_,selectedGroups_,index);return t?segment_triggers(*t):std::vector<SegmentTrigger>{};}
bool SegmentDocument::add_trigger(const SegmentTrigger& e,size_t index){
    if(!trigger_range(*this,e))return false;const auto before=save_bytes();auto next=*this;auto t=trigger_selected(next.root_,selectedGroups_,index);
    if(!t){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=segment_trigger_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));t=&tracks->children.back();}
    if(!add_segment_trigger(*t,e))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::edit_trigger(size_t event,const SegmentTrigger& e,size_t index){if(!trigger_range(*this,e))return false;const auto before=save_bytes();auto next=*this;auto t=trigger_selected(next.root_,selectedGroups_,index);if(!t||!change_segment_trigger(*t,event,e))return false;next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::delete_trigger(size_t event,size_t index){const auto before=save_bytes();auto next=*this;auto t=trigger_selected(next.root_,selectedGroups_,index);if(!t||!delete_segment_trigger(*t,event))return false;next.record_edit(before);*this=std::move(next);return true;}
namespace {
template<class C> auto script_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","scrt")){
    const auto tracks=root.find("LIST","trkl");if(!tracks)return nullptr;const auto expected=script_track().find("trkh")->data;size_t count=0;
    for(auto& track:tracks->children)if(track.id=="RIFF"&&track.type=="DMTK"){const auto h=track.find("trkh");if(!h||h->data.size()<32)throw std::runtime_error("Script selector track header truncated");if(std::equal(expected.begin(),expected.begin()+16,h->data.begin())&&(read32(h->data,20)&groups)&&count++==index)return &track;}return nullptr;
}
bool script_range(const SegmentDocument& doc,const ScriptEvent& e){return e.logical>=0&&e.physical>=0&&e.logical<doc.length()&&e.physical<doc.length();}
}
std::vector<ScriptEvent> SegmentDocument::script_calls(size_t index) const{const auto t=script_selected(root_,selectedGroups_,index);return t?script_events(*t):std::vector<ScriptEvent>{};}
bool SegmentDocument::add_script_call(const ScriptEvent& e,size_t index){
    if(!script_range(*this,e))return false;const auto before=save_bytes();auto next=*this;auto t=script_selected(next.root_,selectedGroups_,index);
    if(!t){size_t count=0;while(script_selected(next.root_,selectedGroups_,count))++count;if(index!=count)return false;auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=script_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));t=&tracks->children.back();}
    if(!add_script_event(*t,e))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::edit_script_call(size_t event,const ScriptEvent& e,size_t index){if(!script_range(*this,e))return false;const auto before=save_bytes();auto next=*this;auto t=script_selected(next.root_,selectedGroups_,index);if(!t||!change_script_event(*t,event,e))return false;next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::delete_script_call(size_t event,size_t index){const auto before=save_bytes();auto next=*this;auto t=script_selected(next.root_,selectedGroups_,index);if(!t||!delete_script_event(*t,event))return false;next.record_edit(before);*this=std::move(next);return true;}
std::vector<StyleReference> SegmentDocument::style_references() const {return app::style_references(root_);}
std::vector<ChordMapReference> SegmentDocument::chordmap_references() const{return app::chordmap_references(root_);}
std::vector<ChordMapReference> SegmentDocument::chordmap_references(size_t index) const{const auto h=chordmap_reference_track(selectedGroups_).find("trkh")->data;const auto t=selected_track(root_,h,selectedGroups_,index);return t?chordmap_track_references(*t):std::vector<ChordMapReference>{};}
bool SegmentDocument::set_chordmap_reference(const ChordMapReference& value,size_t index){
    if(value.time<0||value.time>=length())return false;const auto before=save_bytes();auto next=*this;const auto h=chordmap_reference_track(selectedGroups_).find("trkh")->data;auto t=selected_track(next.root_,h,selectedGroups_,index);
    if(!t){if(index)return false;auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=chordmap_reference_track(selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));t=&tracks->children.back();}
    auto ref=value;ref.groups=read32(t->find("trkh")->data,20);if(!app::set_chordmap_reference(*t,ref))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::clear_chordmap_reference(size_t index){const auto before=save_bytes();auto next=*this;const auto h=chordmap_reference_track(selectedGroups_).find("trkh")->data;auto t=selected_track(next.root_,h,selectedGroups_,index);if(!t||!app::clear_chordmap_reference(*t))return false;next.record_edit(before);*this=std::move(next);return true;}
std::vector<StyleReference> SegmentDocument::style_references(size_t index) const {const auto header=style_reference_track(selectedGroups_).find("trkh")->data;const auto track=selected_track(root_,header,selectedGroups_,index);return track?style_track_references(*track):std::vector<StyleReference>{};}
bool SegmentDocument::update_style_reference(size_t index,const std::function<bool(Chunk&)>& edit,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog){
    const auto before=save_bytes();auto next=*this;const auto header=style_reference_track(selectedGroups_).find("trkh")->data;auto track=selected_track(next.root_,header,selectedGroups_,index);
    if(!track){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=style_reference_track(selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));track=&tracks->children.back();}
    if(!edit(*track))return false;next.resolve_style_context(directory,catalog);next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::add_style_reference(const StyleReference& value,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog,size_t index){if(value.time<0||value.time>=length())return false;return update_style_reference(index,[&](Chunk& track){return insert_style_reference(track,value);},directory,catalog);}
bool SegmentDocument::edit_style_reference(size_t event,const StyleReference& value,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog,size_t index){if(value.time<0||value.time>=length())return false;return update_style_reference(index,[&](Chunk& track){return change_style_reference(track,event,value);},directory,catalog);}
bool SegmentDocument::delete_style_reference(size_t event,const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog,size_t index){return update_style_reference(index,[&](Chunk& track){return app::delete_style_reference(track,event);},directory,catalog);}
Timeline SegmentDocument::timeline() const {
    bool explicitMeter=false;auto view=explicit_timeline(explicitMeter);if(explicitMeter)return view;
    const auto refs=style_references();if(refs.empty())return view;
    if(styles_.size()!=refs.size())throw std::runtime_error("Style meter unresolved; open the document with its Style dependency");
    Bytes payload(4);put32(payload,0,8);std::int32_t previousTime=0;StyleMeter previous{4,4,4};bool first=true;
    for(size_t i=0;i<refs.size();++i){
        const auto& ref=refs[i];const auto& style=styles_[i];
        if(style.reference.time!=ref.time||style.reference.groups!=ref.groups||style.reference.filename!=ref.filename||style.reference.hasId!=ref.hasId||style.reference.objectId!=ref.objectId)throw std::runtime_error("Style context does not match the current document");
        if(!(ref.groups&selectedGroups_))continue;
        if(first&&ref.time!=0)throw std::runtime_error("Style meter for selected groups must start at zero");
        if(!first&&(ref.time<=previousTime||(ref.time-previousTime)%((3072/previous.denominator)*previous.beats)))throw std::runtime_error("Style meter transition must be unique and measure aligned");
        const auto at=payload.size();payload.resize(at+8);put32(payload,at,ref.time);payload[at+4]=style.meter.beats;payload[at+5]=style.meter.denominator;payload[at+6]=static_cast<BYTE>(style.meter.grids);payload[at+7]=static_cast<BYTE>(style.meter.grids>>8);
        previous=style.meter;previousTime=ref.time;first=false;
    }
    if(!first){Chunk meter;meter.id="tims";meter.data=std::move(payload);view.load_meter(meter.encode());}return view;
}
void SegmentDocument::resolve_style_context(const std::wstring& directory,const std::vector<StyleCatalogEntry>& catalog,bool runtimeReferences) {
    auto next=*this;next.styles_=resolve_styles(style_references(),directory,catalog,runtimeReferences);
    (void)next.timeline(); // Validate transitions before publishing dependency snapshots.
    styles_=std::move(next.styles_);styleDirectory_=directory;styleCatalog_=catalog;styleRuntimeReferences_=runtimeReferences;
}
Chunk* SegmentDocument::meter_chunk() {
    auto track=selected_track(root_,meter_header(),selectedGroups_,meterIndex_);if(!track)return nullptr;
    if(auto c=track->find("LIST","TIMS"))return c;return track->find("tims");
}
bool SegmentDocument::update_meters(std::vector<meter::Event> events) {
    if(events.empty()||events.front().measure!=0)return false;
    // Style meter lookup is still unfinished. Do not reanchor a Style-backed
    // document against an invented 4/4 default.
    if(const auto tracks=root_.find("LIST","trkl"))for(const auto& track:tracks->children)if(track.find("LIST","sttr")&&(read32(track.find("trkh")->data,20)&selectedGroups_))return false;
    const auto selectedMeter=selected_track(root_,meter_header(),selectedGroups_,meterIndex_);const auto affected=selectedMeter?read32(selectedMeter->find("trkh")->data,20):selectedGroups_;
    if(const auto tracks=root_.find("LIST","trkl"))for(const auto& track:tracks->children)if(track.find("LIST","sttr")&&(read32(track.find("trkh")->data,20)&affected))return false;
    const auto selectedTempo=selected_track(root_,tempo_header(),selectedGroups_,tempoIndex_);
    const auto before=save_bytes();auto next=*this;
    Bytes payload(4);put32(payload,0,8);std::int64_t at=0;meter::Event previous=events.front();
    for(const auto& event:events) {
        at+=std::int64_t(event.measure-previous.measure)*(3072/previous.denominator)*previous.beats;
        if(at<0||at>=length())return false;
        const auto offset=payload.size();payload.resize(offset+8);put32(payload,offset,static_cast<std::uint32_t>(at));
        payload[offset+4]=event.beats;payload[offset+5]=event.denominator;payload[offset+6]=static_cast<std::uint8_t>(event.grids);payload[offset+7]=static_cast<std::uint8_t>(event.grids>>8);previous=event;
    }
    auto replacement=list("LIST","TIMS",{leaf("tims",payload)});
    if(auto existing=next.meter_chunk()) {
        if(existing->id=="tims")existing->data=payload;else{auto data=existing->find("tims");if(!data||std::count_if(existing->children.begin(),existing->children.end(),[](const Chunk& c){return c.id=="tims";})!=1)throw std::runtime_error("Meter data missing or ambiguous");data->data=payload;}
    } else {
        auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}
        auto header=meter_header();put32(header,20,selectedGroups_);tracks->children.push_back(list("RIFF","DMTK",{leaf("trkh",header),replacement}));
    }
    const auto newMeter=selected_track(next.root_,meter_header(),next.selectedGroups_,next.meterIndex_);
    const auto meterView=[](const Chunk* track){Timeline view;if(!track)return view;const auto wrapped=track->find("LIST","TIMS"),direct=track->find("tims");if(wrapped&&direct)throw std::runtime_error("Ambiguous meter payload");if(wrapped)view.load_meter(wrapped->encode());else if(direct)view.load_meter(direct->encode());else throw std::runtime_error("Meter data missing");return view;};
    const auto oldAffectedView=meterView(selectedMeter),newAffectedView=meterView(newMeter);
    const auto mapTime=[&](std::int32_t time,std::uint32_t groups){
        if(time<0)throw std::runtime_error("Negative Tempo positions are not supported by meter reanchor");
        std::int32_t result=0;bool first=true;
        for(unsigned i=0;i<32;++i){const auto bit=std::uint32_t(1)<<i;if(!(groups&bit))continue;Timeline oldView,newView;
            if(bit&affected){oldView=oldAffectedView;newView=newAffectedView;}
            else{auto oldContext=*this,newContext=next;oldContext.selectedGroups_=bit;oldContext.meterIndex_=0;newContext.selectedGroups_=bit;newContext.meterIndex_=0;oldView=oldContext.timeline();newView=newContext.timeline();}
            const auto position=oldView.position(time);const auto mapped=newView.clocks(position);if(mapped<0||mapped>=length())throw std::runtime_error("Meter reanchor exceeds Segment length");
            if(!first&&result!=mapped)throw std::runtime_error("Shared Tempo track has conflicting group coordinates");result=mapped;first=false;
        }if(first)throw std::runtime_error("Tempo track has no groups");return result;
    };
    const auto originalTracks=root_.find("LIST","trkl");auto nextTracks=next.root_.find("LIST","trkl");const auto expected=tempo_header();
    if(originalTracks)for(size_t i=0;i<originalTracks->children.size();++i){const auto& track=originalTracks->children[i];if(track.id!="RIFF"||track.type!="DMTK")continue;const auto h=track.find("trkh");if(!h||h->data.size()<32)throw std::runtime_error("Invalid track header");if(!std::equal(expected.begin(),expected.begin()+16,h->data.begin()))continue;const auto groups=read32(h->data,20);if(!(groups&affected))continue;
        const auto data=track.find("tetr");if(!data||std::count_if(track.children.begin(),track.children.end(),[](const Chunk& c){return c.id=="tetr";})!=1)throw std::runtime_error("Tempo data missing or ambiguous");tempo::Track checked;if(checked.load(data->encode())!=tempo::LoadResult::ok)throw std::runtime_error("Unsupported Tempo reanchor layout");
        auto& mapped=nextTracks->children[i].find("tetr")->data;
        // Patch clock fields in place: retain order, reserved DWORDs, BPMs,
        // enclosing track/header extensions and unrelated chunks.
        for(size_t offset=4;offset<mapped.size();offset+=16)put32(mapped,offset,static_cast<std::uint32_t>(mapTime(static_cast<std::int32_t>(read32(data->data,offset)),groups)));
        if(&track==selectedTempo){auto cachedEvents=tempo_.events();for(auto& event:cachedEvents){const auto position=oldAffectedView.position(event.time);event.time=mapTime(event.time,groups);event.position={position.measure,position.beat,position.tick};}next.tempo_.replace_events(std::move(cachedEvents));}
    }
    (void)next.timeline();
    next.record_edit(before);const bool changed=before!=next.save_bytes();*this=std::move(next);return changed;
}
bool SegmentDocument::set_meter(std::int32_t measure,unsigned beats,unsigned denominator,unsigned grids) {
    if(measure<0||beats<1||beats>255||denominator<1||denominator>128||(denominator&(denominator-1))||grids<1||grids>65535)return false;
    auto events=timeline().meters();const meter::Event event{measure,static_cast<BYTE>(beats),static_cast<BYTE>(denominator),static_cast<WORD>(grids)};
    const auto found=std::lower_bound(events.begin(),events.end(),measure,[](const meter::Event& e,LONG m){return e.measure<m;});
    if(found!=events.end()&&found->measure==measure)*found=event;else events.insert(found,event);
    return update_meters(std::move(events));
}
bool SegmentDocument::delete_meter(std::int32_t measure) {
    if(measure<=0)return false;auto events=timeline().meters();const auto found=std::find_if(events.begin(),events.end(),[=](const meter::Event& e){return e.measure==measure;});
    if(found==events.end())return false;events.erase(found);return update_meters(std::move(events));
}
std::vector<Note> SegmentDocument::notes() const {
    const auto expected=sequence_track();const auto track=selected_track(root_,expected.find("trkh")->data,selectedGroups_,sequenceIndex_);if(!track)return {};
    const auto data=track->find("seqt");if(!data||std::count_if(track->children.begin(),track->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1)throw std::runtime_error("Sequence payload missing or ambiguous");return sequence_notes(data->data);
}
bool SegmentDocument::add_note(Note note) {
    if(note.time<0||note.duration<=0||std::int64_t(note.time)+note.duration>length()||note.pitch>127||note.velocity<1||note.velocity>127||note.channel>=0xfffffffcu)return false;
    const auto before=save_bytes();auto next=*this;auto tracks=next.root_.find("LIST","trkl");
    if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}
    auto fresh=sequence_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,sequenceIndex_);
    if(!target){put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));target=&tracks->children.back();}
    auto data=target->find("seqt");if(!data||std::count_if(target->children.begin(),target->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1)throw std::runtime_error("Sequence payload missing or ambiguous");
    data->data=sequence_insert(data->data,note);next.record_edit(before);*this=std::move(next);return true;
}
SegmentDocument SegmentDocument::playback_test(unsigned firstPitch) {
    if(firstPitch>115)throw std::runtime_error("Playback test pitch range exceeds MIDI notes");
    SegmentDocument test;put32(test.root_.find("segh")->data,4,6144);
    auto band=gm_piano_band_track();auto tracks=test.root_.find("LIST","trkl");append_position(band,*tracks);tracks->children.push_back(std::move(band));
    test.add_tempo(3072,180);
    const BYTE intervals[]={0,2,4,5,7,9,11,12};for(int i=0;i<8;++i)test.add_note({i*768,384,0,static_cast<BYTE>(firstPitch+intervals[i]),96});
    return test;
}
bool SegmentDocument::edit_note(size_t index,Note note,size_t* resultingIndex){
    if(note.time<0||note.duration<=0||std::int64_t(note.time)+note.duration>length()||note.pitch>127||!note.velocity||note.velocity>127||note.channel>=0xfffffffcu)return false;
    const auto expected=sequence_track();auto next=*this;auto target=selected_track(next.root_,expected.find("trkh")->data,selectedGroups_,sequenceIndex_);
    if(!target)return false;auto data=target->find("seqt");if(!data||std::count_if(target->children.begin(),target->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1)throw std::runtime_error("Sequence payload missing or ambiguous");
    if(index>=sequence_notes(data->data).size())return false;const auto before=save_bytes();size_t afterIndex=0;data->data=sequence_change(data->data,index,note,&afterIndex);if(before==next.save_bytes())return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=afterIndex;return true;
}
bool SegmentDocument::delete_note(size_t index){
    const auto expected=sequence_track();auto next=*this;auto target=selected_track(next.root_,expected.find("trkh")->data,selectedGroups_,sequenceIndex_);
    if(!target)return false;auto data=target->find("seqt");if(!data||std::count_if(target->children.begin(),target->children.end(),[](const Chunk& c){return c.id=="seqt";})!=1)throw std::runtime_error("Sequence payload missing or ambiguous");
    if(index>=sequence_notes(data->data).size())return false;const auto before=save_bytes();data->data=sequence_delete(data->data,index);next.record_edit(before);*this=std::move(next);return true;
}
namespace {
template<class C> auto command_data(C& root,std::uint32_t groups,size_t index)->decltype(root.find("cmnd")) {
    const auto fresh=command_track();const auto t=selected_track(root,fresh.find("trkh")->data,groups,index);if(!t)return nullptr;
    const auto data=t->find("cmnd");if(!data||std::count_if(t->children.begin(),t->children.end(),[](const Chunk& c){return c.id=="cmnd";})!=1)throw std::runtime_error("Command payload missing or ambiguous");return data;
}
bool prepare_command(const SegmentDocument& doc,CommandEvent& event,const CommandEvent* previous=nullptr){
    if(event.time>=doc.length()||!valid_command(event,previous))return false;
    if(previous&&event.time==previous->time){event.measure=previous->measure;event.beat=previous->beat;return true;}
    const auto p=doc.timeline().position(event.time);if(p.measure>UINT16_MAX||p.beat>UINT8_MAX)return false;
    event.measure=static_cast<std::uint16_t>(p.measure);event.beat=static_cast<std::uint8_t>(p.beat);return true;
}
}
std::vector<CommandEvent> SegmentDocument::commands(size_t trackIndex) const {const auto data=command_data(root_,selectedGroups_,trackIndex);return data?command_events(data->data):std::vector<CommandEvent>{};}
bool SegmentDocument::add_command(CommandEvent event,size_t trackIndex){
    if(!prepare_command(*this,event))return false;const auto before=save_bytes();auto next=*this;
    auto data=command_data(next.root_,selectedGroups_,trackIndex);
    if(!data){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=command_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));data=tracks->children.back().find("cmnd");}
    data->data=command_insert(data->data,event);next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::edit_command(size_t index,CommandEvent event,size_t trackIndex,size_t* resultingIndex){
    const auto before=save_bytes();auto next=*this;auto data=command_data(next.root_,selectedGroups_,trackIndex);if(!data)return false;const auto events=command_events(data->data);if(index>=events.size()||!prepare_command(*this,event,&events[index]))return false;
    size_t after=0;data->data=command_change(data->data,index,event,&after);if(before==next.save_bytes())return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
bool SegmentDocument::delete_command(size_t index,size_t trackIndex){
    const auto before=save_bytes();auto next=*this;auto data=command_data(next.root_,selectedGroups_,trackIndex);if(!data||index>=command_events(data->data).size())return false;data->data=command_delete(data->data,index);next.record_edit(before);*this=std::move(next);return true;
}
namespace {
template<class C> auto chord_data(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","cord")) {
    const auto fresh=chord_track();const auto track=selected_track(root,fresh.find("trkh")->data,groups,index);if(!track)return nullptr;
    const auto data=track->find("LIST","cord");if(!data||std::count_if(track->children.begin(),track->children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="cord";})!=1)throw std::runtime_error("Chord payload missing or ambiguous");return data;
}
bool prepare_chord(const SegmentDocument& doc,ChordEvent& event,const ChordEvent* previous=nullptr){
    if(event.time>=doc.length()||!valid_chord(event,previous))return false;
    const auto view=doc.timeline();const auto p=view.position(event.time);if(p.measure>UINT16_MAX||p.beat>UINT8_MAX)return false;
    event.time=view.clocks({p.measure,p.beat,0});event.measure=static_cast<std::uint16_t>(p.measure);event.beat=static_cast<std::uint8_t>(p.beat);return true;
}
}
std::vector<ChordEvent> SegmentDocument::chords(size_t trackIndex) const {const auto data=chord_data(root_,selectedGroups_,trackIndex);return data?chord_events(*data):std::vector<ChordEvent>{};}
bool SegmentDocument::replace_composed_chords(const Chunk& cord,size_t trackIndex){
    const auto events=chord_events(cord);if(events.empty())throw std::runtime_error("Composed Chord list is empty");
    for(const auto& e:events)if(!valid_chord(e)||e.time>=length())throw std::runtime_error("Composed Chord invalid or outside Segment");
    const auto before=save_bytes();auto next=*this;auto data=chord_data(next.root_,selectedGroups_,trackIndex);
    if(!data){auto tracks=next.root_.find("LIST","trkl");if(!tracks)throw std::runtime_error("Segment track list missing");auto fresh=chord_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));data=tracks->children.back().find("LIST","cord");}
    // Replace the generated known bodies, retaining unrelated imported chunks,
    // track headers/configuration and an extended scale header in their slots.
    std::vector<Chunk> bodies;for(const auto& c:cord.children)if(c.id=="crdb")bodies.push_back(c);
    auto scale=data->find("crdh");if(!scale||scale->data.size()<4)throw std::runtime_error("Chord scale header invalid");std::copy_n(cord.find("crdh")->data.begin(),4,scale->data.begin());
    size_t i=0;std::vector<Chunk> children;for(const auto& c:data->children){if(c.id!="crdb")children.push_back(c);else if(i<bodies.size())children.push_back(bodies[i++]);}while(i<bodies.size())children.push_back(bodies[i++]);data->children=std::move(children);
    if(next.save_bytes()==before)return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::set_chord(ChordEvent event,size_t trackIndex){
    if(!prepare_chord(*this,event))return false;const auto before=save_bytes();auto next=*this;auto data=chord_data(next.root_,selectedGroups_,trackIndex);
    if(!data){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=chord_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));data=tracks->children.back().find("LIST","cord");}
    if(!set_chord_event(*data,std::move(event)))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::edit_chord(size_t index,ChordEvent event,size_t trackIndex,size_t* resultingIndex){
    const auto before=save_bytes();auto next=*this;auto data=chord_data(next.root_,selectedGroups_,trackIndex);if(!data)return false;const auto events=chord_events(*data);if(index>=events.size()||!prepare_chord(*this,event,&events[index]))return false;
    size_t after=0;if(!change_chord_event(*data,index,std::move(event),&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
bool SegmentDocument::delete_chord(size_t index,size_t trackIndex){const auto before=save_bytes();auto next=*this;auto data=chord_data(next.root_,selectedGroups_,trackIndex);if(!data||!delete_chord_event(*data,index))return false;next.record_edit(before);*this=std::move(next);return true;}
namespace {
template<class C> auto signpost_data(C& root,std::uint32_t groups,size_t index)->decltype(root.find("sgnp")){const auto fresh=signpost_track();const auto track=selected_track(root,fresh.find("trkh")->data,groups,index);if(!track)return nullptr;const auto data=track->find("sgnp");if(!data||std::count_if(track->children.begin(),track->children.end(),[](const Chunk& c){return c.id=="sgnp";})!=1)throw std::runtime_error("SignPost payload missing or ambiguous");return data;}
bool prepare_signpost(const SegmentDocument& doc,SignpostEvent& e,const SignpostEvent* old=nullptr){if(e.time>=doc.length()||!valid_signpost(e,old))return false;const auto view=doc.timeline();const auto p=view.position(e.time);if(p.measure>UINT16_MAX)return false;e.measure=static_cast<std::uint16_t>(p.measure);e.time=view.clocks({p.measure,0,0});return true;}
}
std::vector<SignpostEvent> SegmentDocument::signposts(size_t trackIndex) const{const auto data=signpost_data(root_,selectedGroups_,trackIndex);return data?signpost_events(data->data):std::vector<SignpostEvent>{};}
bool SegmentDocument::set_signpost(SignpostEvent e,size_t trackIndex){if(!prepare_signpost(*this,e))return false;const auto before=save_bytes();auto next=*this;auto data=signpost_data(next.root_,selectedGroups_,trackIndex);if(!data){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=signpost_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));data=tracks->children.back().find("sgnp");}if(!set_signpost_event(data->data,e))return false;next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::edit_signpost(size_t index,SignpostEvent e,size_t trackIndex,size_t* resultingIndex){const auto before=save_bytes();auto next=*this;auto data=signpost_data(next.root_,selectedGroups_,trackIndex);if(!data)return false;const auto events=signpost_events(data->data);if(index>=events.size()||!prepare_signpost(*this,e,&events[index]))return false;size_t after=0;if(!change_signpost_event(data->data,index,e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::delete_signpost(size_t index,size_t trackIndex){const auto before=save_bytes();auto next=*this;auto data=signpost_data(next.root_,selectedGroups_,trackIndex);if(!data||!delete_signpost_event(data->data,index))return false;next.record_edit(before);*this=std::move(next);return true;}
namespace {
template<class C> auto marker_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","MARK")){const auto fresh=marker_track();return selected_track(root,fresh.find("trkh")->data,groups,index);}
bool prepare_marker(const SegmentDocument& doc,MarkerEvent e){if(e.time<0||e.time>=doc.length()||(e.kind!=MarkerKind::play&&e.kind!=MarkerKind::enter))return false;(void)doc.timeline().position(e.time);return true;}
}
std::vector<MarkerEvent> SegmentDocument::markers(size_t trackIndex) const{const auto track=marker_selected(root_,selectedGroups_,trackIndex);return track?marker_events(*track):std::vector<MarkerEvent>{};}
bool SegmentDocument::add_marker(MarkerEvent e,size_t trackIndex,size_t* resultingIndex){
    if(!prepare_marker(*this,e))return false;const auto before=save_bytes();auto next=*this;auto track=marker_selected(next.root_,selectedGroups_,trackIndex);
    if(!track){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=marker_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));track=&tracks->children.back();}
    size_t after=0;if(!add_marker_event(*track,e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
bool SegmentDocument::edit_marker(size_t index,MarkerEvent e,size_t trackIndex,size_t* resultingIndex){if(!prepare_marker(*this,e))return false;const auto before=save_bytes();auto next=*this;auto track=marker_selected(next.root_,selectedGroups_,trackIndex);size_t after=0;if(!track||!change_marker_event(*track,index,e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::delete_marker(size_t index,size_t trackIndex){const auto before=save_bytes();auto next=*this;auto track=marker_selected(next.root_,selectedGroups_,trackIndex);if(!track||!delete_marker_event(*track,index))return false;next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::mark_boundaries(MarkerKind kind,unsigned division,std::int32_t begin,std::int32_t end,bool marking,size_t trackIndex){
    if((kind!=MarkerKind::play&&kind!=MarkerKind::enter)||division>2||begin<0||end<=begin||end>length())return false;
    const auto view=timeline();const auto& meters=view.meters();std::vector<std::int32_t> boundaries;
    for(size_t i=0;i<meters.size();++i){const auto& m=meters[i];const auto start=view.clocks({m.measure,0,0});const auto finish=i+1<meters.size()?view.clocks({meters[i+1].measure,0,0}):length();
        if(finish<=begin||start>=end)continue;const auto beat=3072/m.denominator;
        if(division==2&&beat%m.grids)throw std::runtime_error("Fractional-clock Marker grids require original observation");
        const auto step=division==0?beat*m.beats:division==1?beat:beat/m.grids;
        const auto lower=std::max(start,begin),upper=std::min(finish,end);
        const auto first=std::int64_t(start)+((std::int64_t(lower)-start+step-1)/step)*step;
        const auto count=first<upper?(upper-first-1)/step+1:0;if(count>1000000-static_cast<std::int64_t>(boundaries.size()))throw std::runtime_error("Marker boundary operation exceeds one million events");
        for(auto time=first;time<upper;time+=step)boundaries.push_back(static_cast<std::int32_t>(time));
    }
    if(boundaries.empty())return false;const auto before=save_bytes();auto next=*this;auto track=marker_selected(next.root_,selectedGroups_,trackIndex);
    if(!track){if(!marking)return false;auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=marker_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));track=&tracks->children.back();}
    if(!mark_marker_boundaries(*track,kind,boundaries,marking))return false;next.record_edit(before);*this=std::move(next);return true;
}
Bytes SegmentDocument::copy_marker(size_t index,size_t trackIndex) const{const auto track=marker_selected(root_,selectedGroups_,trackIndex);if(!track)throw std::runtime_error("Marker track missing");return copy_marker_event(*track,index);}
bool SegmentDocument::paste_marker(const Bytes& bytes,std::int32_t at,size_t trackIndex,size_t* resultingIndex){
    if(at<0||at>=length())return false;(void)timeline().position(at);const auto before=save_bytes();auto next=*this;auto track=marker_selected(next.root_,selectedGroups_,trackIndex);
    if(!track){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=marker_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));track=&tracks->children.back();}
    size_t after=0;if(!paste_marker_event(*track,bytes,at,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
namespace {
template<class C> auto lyric_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","lyrt")){const auto fresh=lyric_track();return selected_track(root,fresh.find("trkh")->data,groups,index);}
bool prepare_lyric(const SegmentDocument& doc,const LyricEvent& e){if(!valid_lyric(e)||e.physical>=doc.length()||e.logical>=doc.length())return false;(void)doc.timeline().position(e.physical);(void)doc.timeline().position(e.logical);return true;}
Chunk* ensure_lyric(Chunk& root,std::uint32_t groups,size_t index){auto track=lyric_selected(root,groups,index);if(track)return track;auto tracks=root.find("LIST","trkl");if(!tracks){root.children.push_back(list("LIST","trkl",{}));tracks=&root.children.back();}auto fresh=lyric_track();put32(fresh.find("trkh")->data,20,groups);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));return &tracks->children.back();}
}
std::vector<LyricEvent> SegmentDocument::lyrics(size_t trackIndex) const{const auto track=lyric_selected(root_,selectedGroups_,trackIndex);return track?lyric_events(*track):std::vector<LyricEvent>{};}
bool SegmentDocument::add_lyric(const LyricEvent& e,size_t trackIndex,size_t* resultingIndex){if(!prepare_lyric(*this,e))return false;const auto before=save_bytes();auto next=*this;size_t after=0;if(!add_lyric_event(*ensure_lyric(next.root_,selectedGroups_,trackIndex),e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::edit_lyric(size_t index,const LyricEvent& e,size_t trackIndex,size_t* resultingIndex){if(!prepare_lyric(*this,e))return false;const auto before=save_bytes();auto next=*this;auto track=lyric_selected(next.root_,selectedGroups_,trackIndex);size_t after=0;if(!track||!change_lyric_event(*track,index,e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::delete_lyric(size_t index,size_t trackIndex){const auto before=save_bytes();auto next=*this;auto track=lyric_selected(next.root_,selectedGroups_,trackIndex);if(!track||!delete_lyric_event(*track,index))return false;next.record_edit(before);*this=std::move(next);return true;}
Bytes SegmentDocument::copy_lyric(size_t index,size_t trackIndex) const{const auto track=lyric_selected(root_,selectedGroups_,trackIndex);if(!track)throw std::runtime_error("Lyric track missing");return copy_lyric_event(*track,index);}
bool SegmentDocument::paste_lyric(const Bytes& bytes,std::int32_t physical,std::int32_t logical,size_t trackIndex,size_t* resultingIndex){if(physical<0||physical>=length()||logical<0||logical>=length())return false;(void)timeline().position(physical);(void)timeline().position(logical);const auto before=save_bytes();auto next=*this;size_t after=0;if(!paste_lyric_event(*ensure_lyric(next.root_,selectedGroups_,trackIndex),bytes,physical,logical,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
namespace {
template<class C> auto mute_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("mute")){const auto fresh=mute_track();return selected_track(root,fresh.find("trkh")->data,groups,index);}
bool prepare_mute(const SegmentDocument& doc,MuteEvent e){return valid_mute(e)&&e.time<=doc.length();}
Chunk* ensure_mute(Chunk& root,std::uint32_t groups,size_t index){auto track=mute_selected(root,groups,index);if(track)return track;auto tracks=root.find("LIST","trkl");if(!tracks){root.children.push_back(list("LIST","trkl",{}));tracks=&root.children.back();}auto fresh=mute_track();put32(fresh.find("trkh")->data,20,groups);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));return &tracks->children.back();}
}
std::vector<MuteEvent> SegmentDocument::mutes(size_t trackIndex) const{const auto t=mute_selected(root_,selectedGroups_,trackIndex);return t?mute_events(*t):std::vector<MuteEvent>{};}
bool SegmentDocument::add_mute(MuteEvent e,size_t trackIndex,size_t* resultingIndex){if(!prepare_mute(*this,e))return false;const auto before=save_bytes();auto next=*this;size_t after=0;if(!add_mute_event(*ensure_mute(next.root_,selectedGroups_,trackIndex),e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::edit_mute(size_t i,MuteEvent e,size_t trackIndex,size_t* resultingIndex){if(!prepare_mute(*this,e))return false;const auto before=save_bytes();auto next=*this;auto t=mute_selected(next.root_,selectedGroups_,trackIndex);size_t after=0;if(!t||!change_mute_event(*t,i,e,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
bool SegmentDocument::delete_mute(size_t i,size_t trackIndex){const auto before=save_bytes();auto next=*this;auto t=mute_selected(next.root_,selectedGroups_,trackIndex);if(!t||!delete_mute_event(*t,i))return false;next.record_edit(before);*this=std::move(next);return true;}
Bytes SegmentDocument::copy_mute(size_t i,size_t trackIndex) const{const auto t=mute_selected(root_,selectedGroups_,trackIndex);if(!t)throw std::runtime_error("Mute track missing");return copy_mute_event(*t,i);}
bool SegmentDocument::paste_mute(const Bytes& bytes,std::int32_t at,size_t trackIndex,size_t* resultingIndex){if(at<0||at>length())return false;const auto before=save_bytes();auto next=*this;size_t after=0;if(!paste_mute_event(*ensure_mute(next.root_,selectedGroups_,trackIndex),bytes,at,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;}
std::vector<WaveItem> SegmentDocument::waves(size_t trackIndex) const {
    auto header=wave_track_identity();header.resize(32);
    const auto track=selected_track(root_,header,selectedGroups_,trackIndex);
    return track?wave_items(*track):std::vector<WaveItem>{};
}
bool SegmentDocument::insert_wave(size_t part,const WaveReference& ref,const WavePlacement& value,std::uint32_t variations,size_t trackIndex,size_t* resultingIndex){
    const auto before=save_bytes();auto next=*this;auto header=wave_track_identity();header.resize(32);auto track=selected_track(next.root_,header,selectedGroups_,trackIndex);size_t after=0;
    if(!track||!insert_wave_reference(*track,part,ref,value,variations,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
bool SegmentDocument::edit_wave(size_t part,size_t item,const WavePlacement& e,size_t trackIndex){
    const auto before=save_bytes();auto next=*this;auto header=wave_track_identity();header.resize(32);
    auto track=selected_track(next.root_,header,selectedGroups_,trackIndex);
    if(!track||!edit_wave_placement(*track,part,item,e))return false;
    next.record_edit(before);*this=std::move(next);return true;
}
Bytes SegmentDocument::copy_wave(size_t part,size_t item,size_t trackIndex) const{
    auto header=wave_track_identity();header.resize(32);auto track=selected_track(root_,header,selectedGroups_,trackIndex);
    if(!track)throw std::runtime_error("Wave track missing");return copy_wave_event(*track,part,item);
}
bool SegmentDocument::paste_wave(const Bytes& bytes,size_t part,std::int64_t time,size_t trackIndex,size_t* resultingIndex){
    const auto before=save_bytes();auto next=*this;auto header=wave_track_identity();header.resize(32);auto track=selected_track(next.root_,header,selectedGroups_,trackIndex);size_t after=0;
    if(!track||!paste_wave_event(*track,bytes,part,time,&after))return false;next.record_edit(before);*this=std::move(next);if(resultingIndex)*resultingIndex=after;return true;
}
bool SegmentDocument::delete_wave(size_t part,size_t item,size_t trackIndex){
    const auto before=save_bytes();auto next=*this;auto header=wave_track_identity();header.resize(32);auto track=selected_track(next.root_,header,selectedGroups_,trackIndex);
    if(!track||!delete_wave_event(*track,part,item))return false;next.record_edit(before);*this=std::move(next);return true;
}
std::vector<BandEvent> SegmentDocument::band_events() const {
    const auto expected=make_band_track();const auto track=selected_track(root_,expected.find("trkh")->data,selectedGroups_,bandIndex_);return track?band_track_events(*track):std::vector<BandEvent>{};
}
Bytes SegmentDocument::tool_graph()const{const Chunk* found=nullptr;for(const auto& c:root_.children)if(c.id=="RIFF"&&c.type=="DMTG"){if(found)throw std::runtime_error("Ambiguous embedded ToolGraph");found=&c;}if(!found)return {};ToolGraphDocument graph;graph.load(found->encode());return graph.save_bytes();}
bool SegmentDocument::set_tool_graph(const Bytes& bytes){ToolGraphDocument graph;graph.load(bytes);(void)tool_graph();const auto before=save_bytes();auto next=*this;auto chunk=Chunk::parse(bytes);if(auto current=next.root_.find("RIFF","DMTG"))*current=std::move(chunk);else next.root_.children.push_back(std::move(chunk));if(next.save_bytes()==before)return false;next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::remove_tool_graph(){if(tool_graph().empty())return false;const auto before=save_bytes();auto next=*this;next.root_.children.erase(std::remove_if(next.root_.children.begin(),next.root_.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMTG";}),next.root_.children.end());next.record_edit(before);*this=std::move(next);return true;}
Bytes SegmentDocument::audio_path() const {
    const Chunk* path=nullptr;for(const auto& c:root_.children)if(c.id=="RIFF"&&c.type=="DMAP"){
        if(path)throw std::runtime_error("Ambiguous embedded AudioPath");path=&c;
    }return path?path->encode():Bytes{};
}
bool SegmentDocument::set_audio_path(const Bytes& bytes){
    AudioPathDocument validated;validated.load(bytes);auto config=Chunk::parse(bytes);const auto count=std::count_if(root_.children.begin(),root_.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMAP";});if(count>1)throw std::runtime_error("Ambiguous embedded AudioPath");
    const auto before=save_bytes();auto next=*this;if(auto current=next.root_.find("RIFF","DMAP"))*current=std::move(config);else next.root_.children.push_back(std::move(config));if(next.save_bytes()==before)return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::remove_audio_path(){const auto count=std::count_if(root_.children.begin(),root_.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMAP";});if(count>1)throw std::runtime_error("Ambiguous embedded AudioPath");if(!count)return false;const auto before=save_bytes();auto next=*this;next.root_.children.erase(std::remove_if(next.root_.children.begin(),next.root_.children.end(),[](const Chunk& c){return c.id=="RIFF"&&c.type=="DMAP";}),next.root_.children.end());next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::set_band(std::int32_t time,const Bytes& band){
    if(time<0||time>=length())return false;const auto before=save_bytes();auto next=*this;auto tracks=next.root_.find("LIST","trkl");
    if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}
    auto fresh=make_band_track();auto target=selected_track(next.root_,fresh.find("trkh")->data,selectedGroups_,bandIndex_);
    if(!target){put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));target=&tracks->children.back();}
    if(!set_band_track_event(*target,time,band))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::move_band(size_t index,std::int32_t logical,std::int32_t physical){
    if(logical<0||logical>=length()||physical>=length())return false;(void)band_events();const auto before=save_bytes();auto next=*this;
    const auto expected=make_band_track();auto target=selected_track(next.root_,expected.find("trkh")->data,selectedGroups_,bandIndex_);if(!target||!move_band_track_event(*target,index,logical,physical))return false;next.record_edit(before);*this=std::move(next);return true;
}
bool SegmentDocument::delete_band(size_t index){
    (void)band_events();const auto before=save_bytes();auto next=*this;
    const auto expected=make_band_track();auto target=selected_track(next.root_,expected.find("trkh")->data,selectedGroups_,bandIndex_);if(!target||!delete_band_track_event(*target,index))return false;next.record_edit(before);*this=std::move(next);return true;
}
}

namespace producer::app {
namespace {
template<class C> auto param_selected(C& root,std::uint32_t groups,size_t index)->decltype(root.find("LIST","trkl")){const auto fresh=param_control_track();return selected_track(root,fresh.find("trkh")->data,groups,index);}
}
std::vector<ParamControlObject> SegmentDocument::parameter_controls(size_t trackIndex) const{const auto t=param_selected(root_,selectedGroups_,trackIndex);return t?param_control_objects(*t):std::vector<ParamControlObject>{};}
bool SegmentDocument::update_parameter_control(size_t index,const std::function<bool(Chunk&)>& edit){const auto before=save_bytes();auto next=*this;auto track=param_selected(next.root_,selectedGroups_,index);if(!track){auto tracks=next.root_.find("LIST","trkl");if(!tracks){next.root_.children.push_back(list("LIST","trkl",{}));tracks=&next.root_.children.back();}auto fresh=param_control_track();put32(fresh.find("trkh")->data,20,selectedGroups_);append_position(fresh,*tracks);tracks->children.push_back(std::move(fresh));track=&tracks->children.back();}if(!edit(*track))return false;(void)param_control_objects(*track);next.record_edit(before);*this=std::move(next);return true;}
bool SegmentDocument::add_parameter_object(const ParamObject& value,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return add_param_object(track,value);});}
bool SegmentDocument::edit_parameter_object(size_t object,const ParamObject& value,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return edit_param_object(track,object,value);});}
bool SegmentDocument::delete_parameter_object(size_t object,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return delete_param_object(track,object);});}
bool SegmentDocument::add_parameter(size_t object,std::uint32_t index,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return add_param_parameter(track,object,index);});}
bool SegmentDocument::delete_parameter(size_t object,size_t parameter,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return delete_param_parameter(track,object,parameter);});}
bool SegmentDocument::add_parameter_curve(size_t object,size_t parameter,const ParamCurve& value,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return add_param_curve(track,object,parameter,value);});}
bool SegmentDocument::edit_parameter_curve(size_t object,size_t parameter,size_t curve,const ParamCurve& value,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return edit_param_curve(track,object,parameter,curve,value);});}
bool SegmentDocument::delete_parameter_curve(size_t object,size_t parameter,size_t curve,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return delete_param_curve(track,object,parameter,curve);});}
bool SegmentDocument::paste_parameter_curve(size_t object,size_t parameter,const Bytes& bytes,std::int32_t at,size_t trackIndex){return update_parameter_control(trackIndex,[&](Chunk& track){return paste_param_curve(track,object,parameter,bytes,at);});}
Bytes SegmentDocument::copy_parameter_curve(size_t object,size_t parameter,size_t curve,size_t trackIndex) const{auto t=param_selected(root_,selectedGroups_,trackIndex);if(!t)throw std::runtime_error("Parameter Control track missing");return copy_param_curve(*t,object,parameter,curve);}
}
