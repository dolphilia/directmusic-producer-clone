#include "document.h"
#include "audio_path.h"
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
    root_=std::move(root); auto chunk=tempo_chunk(); hasTempo_=chunk!=nullptr;
    if (chunk) { if(tempo_.load(chunk->encode())!=tempo::LoadResult::ok)throw std::runtime_error("Unsupported tempo data");
        for(const auto& e:tempo_.events())if(e.time<0)throw std::runtime_error("Negative tempo positions are not yet displayed"); }
    else tempo_.replace_events({});
    bool explicitMeter=false;(void)explicit_timeline(explicitMeter);
    (void)app::style_references(root_); // Validate references without inventing a meter before resolution.
}
void SegmentDocument::load(const Bytes& bytes) {
    SegmentDocument next; next.import(bytes); next.saved_=bytes; next.dirty_=false; *this=std::move(next);
}
void SegmentDocument::select_track_group(std::uint32_t groups,size_t tempoIndex,size_t meterIndex,size_t sequenceIndex,size_t bandIndex){
    if(!groups)throw std::runtime_error("Track group mask must be nonzero");auto next=*this;next.selectedGroups_=groups;next.tempoIndex_=tempoIndex;next.meterIndex_=meterIndex;next.sequenceIndex_=sequenceIndex;next.bandIndex_=bandIndex;next.import(save_bytes());(void)next.notes();(void)next.band_events();if(!next.styles_.empty())(void)next.timeline();*this=std::move(next);
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
    auto next=*this; next.import(undo_.back());next.redo_.push_back(save_bytes());next.undo_.pop_back();next.dirty_=next.save_bytes()!=saved_;*this=std::move(next);return true;
}
bool SegmentDocument::redo() {
    if(redo_.empty())return false;
    auto next=*this;next.import(redo_.back());next.undo_.push_back(save_bytes());next.redo_.pop_back();next.dirty_=next.save_bytes()!=saved_;*this=std::move(next);return true;
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
std::vector<StyleReference> SegmentDocument::style_references() const {return app::style_references(root_);}
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
    styles_=std::move(next.styles_);
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
std::vector<BandEvent> SegmentDocument::band_events() const {
    const auto expected=make_band_track();const auto track=selected_track(root_,expected.find("trkh")->data,selectedGroups_,bandIndex_);return track?band_track_events(*track):std::vector<BandEvent>{};
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
