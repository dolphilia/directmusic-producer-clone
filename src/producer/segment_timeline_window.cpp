#include "segment_timeline_window.h"
#include "new_files.h"
#include "embedded_band_editor.h"
#include "sequence_strip_model.h"
#include "sequence_velocity_editor.h"
#include "sequence_quantize_editor.h"
#include "sequence_cc_editor.h"
#include "sequence_part_editor.h"
#include "gm_instruments.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <limits>
#include <vector>
#include <map>
#include <tuple>
namespace producer::app { namespace {
constexpr UINT ZoomIn=4500,ZoomOut=4501,Fit=4502,Properties=4503,SelectionText=4504,AddTracks=4505,InsertBand=4506,DeleteBand=4507;
constexpr UINT InsertNote=4510,DeleteNote=4511,SnapNone=4512,SnapGrid=4513,SnapBeat=4514,SnapBar=4515,NewInstrument=4516,OpenSequenceBand=4517,LocateSequenceBand=4518,EditVelocity=4519,AddCC=4520,DrawCurve=4521,DeleteCurve=4522,CurveProperties=4523,FirstPatch=6000;
constexpr UINT AddSequencePart=4524,OtherDls=4525,Quantize=4526;
constexpr UINT WorkspaceControl=4700,Cascade=4701,TileHorizontal=4702,TileVertical=4703,CloseAll=4704,FirstChild=50000;
constexpr int labelWidth=130,rulerHeight=34,rowHeight=36,baseRows=6,footerHeight=72;
constexpr int pitchHeight=12,pitchRows=18,ccHeight=24,ccGraphHeight=96;
HWND workspace=nullptr;
std::vector<HWND> timelineWindows;
struct Session {
    Framework& host;size_t document;TimelineSelectionChanged changed;TimelineHistoryRequested history;
    TimelineNoteAudition audition;TimelineBandMixStarted mixStarted;
    using SequenceKey=std::tuple<std::uint32_t,size_t,std::uint32_t>;
    struct SequenceStrip {bool expanded=false;int topPitch=70;unsigned pitch=60;SequenceSnap snap=SequenceSnap::grid;std::optional<size_t> note;std::map<SequenceController,bool> cc;std::optional<SequenceController> activeCC;std::optional<size_t> curve;bool drawCurve=false,ccContext=false;};
    std::map<SequenceKey,SequenceStrip> sequences;
    std::optional<SequenceKey> activeSequence;
    struct NoteDrag {SequenceKey key;size_t index;Note original,preview;SequenceNoteHandle handle;POINT start,current;Bytes before;std::int64_t fineClock=0;int finePitch=0,fineVelocity=0;};
    std::optional<NoteDrag> noteDrag;
    struct CurveDrag {SequenceKey key;SequenceController controller;std::optional<size_t> index;SequenceCurve original,preview;int handle;POINT start,current;Bytes before;};
    std::optional<CurveDrag> curveDrag;
    std::optional<TimelineEventTarget> selected;
    struct MuteCell {std::uint32_t channel,groups;std::int32_t clock;size_t track;};
    std::optional<MuteCell> muteCell;
    struct BandCell {size_t track;std::uint32_t groups;std::int32_t beat;};
    std::optional<BandCell> bandCell;
    std::int32_t cursor=0;
    double zoom=1;std::int32_t scroll=0;int verticalScroll=0,wheelRemainder=0;bool zoomInitialized=false,updating=false,owned=false;
    SegmentDocument& doc(){return host.document(document);}
    void publish(bool show=false){if(changed)changed(selected,show);}
};
int cc_section_height(const Session::SequenceStrip& state){if(state.cc.empty())return ccHeight;int height=0;for(const auto& lane:state.cc)height+=ccHeight+(lane.second?ccGraphHeight:0);return height;}
struct Row {SegmentTrackKind kind;size_t track;std::optional<std::uint32_t> channel;std::wstring label;int height=rowHeight;};
Session::SequenceKey sequence_key(Session& s,const Row& row){return {s.doc().selected_groups(),row.track,*row.channel};}
SegmentDocument track_document(Session& s,SegmentTrackKind kind,size_t track){
    auto d=s.doc();d.select_track_group(d.selected_groups(),kind==SegmentTrackKind::tempo?track:0,kind==SegmentTrackKind::meter?track:0,kind==SegmentTrackKind::sequence?track:0,kind==SegmentTrackKind::band?track:0);return d;
}
std::vector<Row> view_rows(Session& s){
    std::vector<Row> rows;for(const auto& t:s.doc().tracks()){
        std::wstring name=L"Track";for(const auto& descriptor:segment_track_descriptors())if(descriptor.kind==t.kind){name=descriptor.name;const auto suffix=name.rfind(L" Track");if(suffix!=std::wstring::npos)name.resize(suffix);break;}
        std::wstring groups;for(unsigned bit=0;bit<32;++bit)if(t.groups&(std::uint32_t(1)<<bit)){if(!groups.empty())groups+=L",";groups+=std::to_wstring(bit+1);}
        if(t.kind==SegmentTrackKind::sequence){const auto part=sequence_part_info(s.doc(),t.index);for(const auto channel:part.channels){auto [partState,created]=s.sequences.try_emplace({s.doc().selected_groups(),t.index,channel});auto& state=partState->second;if(created&&channel==9){state.topPitch=43;state.pitch=36;}auto curveDoc=track_document(s,t.kind,t.index);
            const auto lanes=curveDoc.cc_lanes(channel);std::map<SequenceController,bool> current;
            for(const auto& controller:lanes){const auto old=state.cc.find(controller);current.emplace(controller,old!=state.cc.end()&&old->second);}state.cc=std::move(current);
            if(state.activeCC&&state.cc.find(*state.activeCC)==state.cc.end()){state.activeCC.reset();state.curve.reset();state.drawCurve=false;}
            rows.push_back({t.kind,t.index,channel,groups+L": "+std::to_wstring(std::uint64_t(channel)+1)+L" ("+std::to_wstring(std::uint64_t(channel)+1)+L"): "+part.name,state.expanded?rowHeight+pitchHeight*pitchRows+cc_section_height(state):rowHeight});}}
        else rows.push_back({t.kind,t.index,{},groups+L": "+name});
        if(t.kind==SegmentTrackKind::mute){std::vector<std::uint32_t> channels;for(std::uint32_t c=0;c<16;++c)channels.push_back(c);for(const auto& e:s.doc().mutes(t.index))channels.push_back(e.channel);std::sort(channels.begin(),channels.end());channels.erase(std::unique(channels.begin(),channels.end()),channels.end());for(auto c:channels)rows.push_back({t.kind,t.index,c,L"PChannel "+std::to_wstring(std::uint64_t(c)+1)});}
    }return rows;
}
int rows_height(const std::vector<Row>& rows){int height=0;for(const auto& row:rows)height+=row.height;return height;}
int row_top(const std::vector<Row>& rows,size_t index){int y=0;for(size_t i=0;i<index;++i)y+=rows[i].height;return y;}
std::optional<size_t> row_at(const std::vector<Row>& rows,int y){if(y<0)return {};for(size_t i=0;i<rows.size();++i){if(y<rows[i].height)return i;y-=rows[i].height;}return {};}
std::wstring chord_label(const ChordEvent& e){
    const wchar_t* names[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
    const auto base=chord_level(e,0);if(!base||e.subchords[*base].chordRoot>23)return e.name;
    const auto root=e.subchords[*base].chordRoot;return std::to_wstring(root/12+1)+names[root%12]+L" "+e.name;
}
bool fine_beats(const Timeline& axis,std::int32_t begin,std::int32_t end){unsigned smallest=768;for(const auto& e:axis.meters())smallest=std::min(smallest,3072u/e.denominator);return (std::int64_t(end)-begin)/smallest+2<=4096;}
struct Viewport {int right;double scale;std::int32_t begin,end,page;};
Viewport viewport(HWND w,Session& s){
    RECT r{};GetClientRect(w,&r);const int width=std::max(1,static_cast<int>(r.right)-labelWidth-2);
    const auto length=std::max(1,s.doc().length());
    if(!s.zoomInitialized&&width>1){s.zoom=std::clamp(double(length)/(double(width)*32.),1.,32.);s.zoomInitialized=true;}
    const auto page=std::max(1,static_cast<int>(std::ceil(length/s.zoom)));
    s.scroll=std::clamp(s.scroll,0,std::max(0,length-page));
    const auto end=static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::min<std::int64_t>(std::int64_t(length)+1,std::int64_t(s.scroll)+page+1)));
    return {static_cast<int>(r.right)-2,double(width)*s.zoom/length,s.scroll,end,page};
}
int pixel(const Viewport& v,std::int32_t clock){return labelWidth+static_cast<int>(std::clamp<std::int64_t>(std::llround((std::int64_t(clock)-v.begin)*v.scale),INT32_MIN+4096,INT32_MAX-4096));}
bool fine_grid(const Timeline& axis,const Viewport& v){
    unsigned smallest=768;for(const auto& meter:axis.meters())smallest=std::min(smallest,3072u/meter.denominator);
    return smallest*v.scale>=6&&fine_beats(axis,v.begin,v.end);
}
bool follow_play_cursor(HWND w,Session& s){
    const auto v=viewport(w,s);const auto previous=s.scroll;
    if(s.cursor<v.begin||std::int64_t(s.cursor)>=std::int64_t(v.begin)+v.page)
        s.scroll=static_cast<std::int32_t>(std::clamp<std::int64_t>(std::int64_t(s.cursor)-v.page/8,0,std::max(0,s.doc().length()-v.page)));
    return s.scroll!=previous;
}
std::wstring band_label(const BandEvent& event){BandDocument band;band.load(event.band);return band.name().empty()?L"Band":band.name();}
int band_label_right(HDC dc,const Viewport& v,int left,std::int32_t next,const std::wstring& value){
    SIZE extent{};GetTextExtentPoint32W(dc,value.c_str(),static_cast<int>(value.size()),&extent);
    return std::min({v.right,pixel(v,next)-2,left+std::max(8,static_cast<int>(extent.cx)+5)});
}
HWND control(HWND w,const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int width,int height,UINT id){
    auto c=CreateWindowW(cls,title,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Timeline control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
int strip_bottom(HWND w){RECT r{};GetClientRect(w,&r);return std::max(rulerHeight+1,static_cast<int>(r.bottom)-footerHeight);}
int strip_page(HWND w){return strip_bottom(w)-rulerHeight;}
void layout(HWND w){RECT r{};GetClientRect(w,&r);const int y=strip_bottom(w)+4;
    // The position and actions have separate rows: neither depends on the
    // number of visible tracks, or on how the MDI client tiles its children.
    MoveWindow(GetDlgItem(w,SelectionText),8,y,std::max(1,static_cast<int>(r.right)-16),28,TRUE);
    const int x=std::max(8,static_cast<int>(r.right)-380);
    MoveWindow(GetDlgItem(w,ZoomOut),x,y+32,80,28,TRUE);MoveWindow(GetDlgItem(w,ZoomIn),x+86,y+32,80,28,TRUE);
    MoveWindow(GetDlgItem(w,Fit),x+172,y+32,60,28,TRUE);MoveWindow(GetDlgItem(w,Properties),x+238,y+32,134,28,TRUE);
}
void refresh(HWND w,Session& s){
    if(s.updating)return;struct Guard{bool& b;Guard(bool& x):b(x){b=true;}~Guard(){b=false;}} guard(s.updating);
    const auto v=viewport(w,s);auto& d=s.doc();const auto group=d.selected_groups();
    s.cursor=std::clamp(s.cursor,0,std::max(0,d.length()-1));
    if(s.muteCell&&(s.muteCell->groups!=group||s.muteCell->clock>=d.length()))s.muteCell.reset();
    if(s.muteCell){const auto actual=d.tracks();if(std::none_of(actual.begin(),actual.end(),[&](const auto& track){return track.kind==SegmentTrackKind::mute&&track.index==s.muteCell->track;}))s.muteCell.reset();}
    if(s.bandCell){const auto actual=d.tracks();if(s.bandCell->groups!=group||s.bandCell->beat>=d.length()||std::none_of(actual.begin(),actual.end(),[&](const auto& track){return track.kind==SegmentTrackKind::band&&track.index==s.bandCell->track;}))s.bandCell.reset();}
    if(s.selected){auto& target=*s.selected;bool valid=target.groups==group;
        if(valid)switch(target.kind){case TimelineEventKind::sequence_note:{const auto owned=track_document(s,SegmentTrackKind::sequence,target.track);valid=target.event<owned.notes().size();break;}case TimelineEventKind::band:{const auto owned=track_document(s,SegmentTrackKind::band,target.track);valid=target.event<owned.band_events().size();break;}case TimelineEventKind::chord:valid=target.track<d.chord_track_count()&&target.event<d.chords(target.track).size();break;case TimelineEventKind::meter:valid=target.track==d.selected_meter_index()&&target.event<d.timeline().meters().size();break;case TimelineEventKind::tempo:valid=target.event<d.tempos().size();break;case TimelineEventKind::lyric:valid=target.event<d.lyrics(target.track).size();break;case TimelineEventKind::marker:valid=target.event<d.markers(target.track).size();break;}
        if(!valid)s.selected.reset();
    }
    auto name=d.name();if(name.empty()){const auto& path=s.host.documents().at(s.document).path;name=path.empty()?L"Untitled Segment "+std::to_wstring(s.document+1):std::filesystem::path(path).stem().wstring();}
    const auto title=L"Segment: "+name;SetWindowTextW(w,title.c_str());std::wstring text=L"No event selected";
    if(s.selected){const auto& target=*s.selected;std::int32_t time=0;std::wstring kind;
        switch(target.kind){case TimelineEventKind::sequence_note:{kind=L"Note";time=track_document(s,SegmentTrackKind::sequence,target.track).notes().at(target.event).time;break;}case TimelineEventKind::band:{kind=L"Band";const auto owned=track_document(s,SegmentTrackKind::band,target.track);time=owned.band_events().at(target.event).physicalTime;break;}case TimelineEventKind::chord:kind=L"Chord";time=d.chords(target.track)[target.event].time;break;case TimelineEventKind::meter:kind=L"Time Signature";time=d.timeline().clocks({d.timeline().meters()[target.event].measure,0,0});break;case TimelineEventKind::tempo:kind=L"Tempo";time=d.tempos()[target.event].time;break;case TimelineEventKind::lyric:kind=L"Lyric";time=d.lyrics(target.track)[target.event].physical;break;case TimelineEventKind::marker:{const auto e=d.markers(target.track)[target.event];kind=e.kind==MarkerKind::play?L"Marker":L"Enter Switch Point";time=e.time;break;}}
        const auto p=s.selected->kind==TimelineEventKind::band?band_start_position(d.timeline(),time):d.timeline().position(time);text=kind+L" — "+std::to_wstring(p.measure+1)+L":"+std::to_wstring(p.beat+1)+L":"+std::to_wstring(p.tick)+L" ("+std::to_wstring(time)+L" clocks)";
    }
    if(s.muteCell){const auto& cell=*s.muteCell;const auto p=d.timeline().position(cell.clock);auto map=cell.channel;for(const auto& e:d.mutes(cell.track))if(e.channel==cell.channel&&e.time<=cell.clock)map=e.map;text=L"Mute — PChannel "+std::to_wstring(std::uint64_t(cell.channel)+1)+L" — "+std::to_wstring(p.measure+1)+L":"+std::to_wstring(p.beat+1)+(map==mute_channel_silent?L" — Muted":L" — Unmuted");}
    const auto cp=d.timeline().position(s.cursor);text=L"Cursor "+std::to_wstring(cp.measure+1)+L":"+std::to_wstring(cp.beat+1)+L":"+std::to_wstring(cp.tick)+L" ("+std::to_wstring(s.cursor)+L" clocks) — "+text;
    SetDlgItemTextW(w,SelectionText,text.c_str());EnableWindow(GetDlgItem(w,Properties),s.selected.has_value());
    EnableWindow(GetDlgItem(w,ZoomIn),s.zoom<32);EnableWindow(GetDlgItem(w,ZoomOut),s.zoom>1);
    SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS,0,std::max(0,d.length()-1),static_cast<UINT>(v.page),s.scroll,0};SetScrollInfo(w,SB_HORZ,&info,TRUE);
    const auto page=strip_page(w),height=rows_height(view_rows(s));s.verticalScroll=std::clamp(s.verticalScroll,0,std::max(0,height-page));
    SCROLLINFO vertical{sizeof(vertical),SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL,0,height-1,static_cast<UINT>(page),s.verticalScroll,0};SetScrollInfo(w,SB_VERT,&vertical,TRUE);InvalidateRect(w,nullptr,TRUE);
}
std::optional<TimelineEventTarget> hit(HWND w,Session& s,POINT p){
    const auto v=viewport(w,s);if(p.x<labelWidth||p.x>v.right||p.y<rulerHeight||p.y>=strip_bottom(w))return {};
    const auto rows=view_rows(s);const auto slot=row_at(rows,p.y+s.verticalScroll-rulerHeight);if(!slot)return {};const auto& row=rows[*slot];
    std::optional<TimelineEventTarget> target;int distance=8;const auto groups=s.doc().selected_groups();
    const auto consider=[&](TimelineEventKind kind,size_t index,std::int32_t clock){const int x=pixel(v,clock);if(x<labelWidth||x>v.right)return;const auto delta=std::abs(p.x-x);if(delta<distance){distance=delta;target=TimelineEventTarget{kind,s.document,row.track,index,groups};}};
    if(row.kind==SegmentTrackKind::tempo){const auto d=track_document(s,row.kind,row.track);for(size_t i=0;i<d.tempos().size();++i)if(d.tempos()[i].bpm>0)consider(TimelineEventKind::tempo,i,d.tempos()[i].time);}
    if(row.kind==SegmentTrackKind::meter){const auto d=track_document(s,row.kind,row.track);const auto axis=d.timeline();const auto& events=axis.meters();for(size_t i=0;i<events.size();++i)consider(TimelineEventKind::meter,i,axis.clocks({events[i].measure,0,0}));}
    if(row.kind==SegmentTrackKind::lyric){const auto events=s.doc().lyrics(row.track);for(size_t i=0;i<events.size();++i)consider(TimelineEventKind::lyric,i,events[i].physical);}
    if(row.kind==SegmentTrackKind::marker){const auto events=s.doc().markers(row.track);const bool upper=(p.y+s.verticalScroll-rulerHeight-row_top(rows,*slot))<rowHeight/2;for(size_t i=0;i<events.size();++i)if((events[i].kind==MarkerKind::play)==upper)consider(TimelineEventKind::marker,i,events[i].time);}
    if(row.kind==SegmentTrackKind::band){const auto d=track_document(s,row.kind,row.track);const auto events=d.band_events();auto dc=GetDC(w);const auto oldFont=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));for(size_t i=0;i<events.size();++i){const int left=pixel(v,std::max(0,events[i].physicalTime));if(left<labelWidth||left>v.right)continue;const int right=band_label_right(dc,v,left,i+1<events.size()?std::max(0,events[i+1].physicalTime):s.doc().length(),band_label(events[i]));if(p.x>=left&&p.x<right)target=TimelineEventTarget{TimelineEventKind::band,s.document,row.track,i,groups};}SelectObject(dc,oldFont);ReleaseDC(w,dc);}
    if(row.kind==SegmentTrackKind::chord){const auto events=s.doc().chords(row.track);for(size_t i=0;i<events.size();++i){const int left=pixel(v,events[i].time),right=pixel(v,i+1<events.size()?events[i+1].time:s.doc().length());if(p.x>=left&&p.x<right)target=TimelineEventTarget{TimelineEventKind::chord,s.document,row.track,i,groups};}}
    return target;
}
void select(HWND w,Session& s,std::optional<TimelineEventTarget> target,bool show=false){
    if(!target&&s.selected&&s.selected->kind==TimelineEventKind::tempo)s.doc().select(std::vector<size_t>{});
    s.selected=target;s.muteCell.reset();s.bandCell.reset();if(target&&target->kind==TimelineEventKind::band)s.doc().select_track_group(s.doc().selected_groups(),s.doc().selected_tempo_index(),s.doc().selected_meter_index(),s.doc().selected_sequence_index(),target->track);if(target&&target->kind==TimelineEventKind::meter)s.doc().select_track_group(s.doc().selected_groups(),s.doc().selected_tempo_index(),target->track,s.doc().selected_sequence_index(),s.doc().selected_band_index());if(target&&target->kind==TimelineEventKind::tempo){s.doc().select_track_group(s.doc().selected_groups(),target->track,s.doc().selected_meter_index(),s.doc().selected_sequence_index(),s.doc().selected_band_index());s.doc().select(target->event);}
    SetFocus(w);refresh(w,s);s.publish(show);
}
bool click_mute(HWND w,Session& s,POINT p){
    const auto v=viewport(w,s);if(p.x<labelWidth||p.x>=v.right||p.y<rulerHeight||p.y>=strip_bottom(w))return false;
    const auto rows=view_rows(s);const auto index=row_at(rows,p.y+s.verticalScroll-rulerHeight);if(!index||rows[*index].kind!=SegmentTrackKind::mute||!rows[*index].channel)return false;const auto& row=rows[*index];
    const auto axis=s.doc().timeline();if(!fine_beats(axis,v.begin,v.end))return true;
    const auto clock=std::int64_t(v.begin)+static_cast<std::int64_t>(std::floor((p.x-labelWidth)/v.scale));if(clock<0||clock>=s.doc().length())return true;
    const auto pos=axis.position(static_cast<std::int32_t>(clock));const auto begin=axis.clocks({pos.measure,pos.beat,0});
    if(s.doc().toggle_mute_beat(*row.channel,begin,row.track)){if(s.selected&&s.selected->kind==TimelineEventKind::tempo)s.doc().select(std::vector<size_t>{});s.selected.reset();s.muteCell=Session::MuteCell{*row.channel,s.doc().selected_groups(),begin,row.track};SetFocus(w);refresh(w,s);s.publish();}return true;
}

bool click_sequence(HWND,Session&,POINT,bool doubleClick=false,bool context=false);
void select_point(HWND w,Session& s,POINT p,bool context=false){
    if(click_sequence(w,s,p,false,context))return;s.activeSequence.reset();
    const auto v=viewport(w,s);
    if(!context&&p.x>=labelWidth&&p.x<v.right&&p.y>=0&&p.y<strip_bottom(w)){
        const auto raw=std::clamp<std::int64_t>(std::int64_t(v.begin)+static_cast<std::int64_t>(std::floor((p.x-labelWidth)/v.scale)),0,std::max(0,s.doc().length()-1));
        auto grid=s.doc().timeline().grid_position(static_cast<std::int32_t>(raw));grid.tick=0;s.cursor=s.doc().timeline().grid_clocks(grid);}
    select(w,s,hit(w,s,p));
    if(p.x<labelWidth||p.x>=v.right||p.y<rulerHeight||p.y>=strip_bottom(w))return;
    const auto rows=view_rows(s);const auto at=row_at(rows,p.y+s.verticalScroll-rulerHeight);if(!at)return;
    const auto raw=std::clamp<std::int64_t>(std::int64_t(v.begin)+static_cast<std::int64_t>(std::floor((p.x-labelWidth)/v.scale)),0,std::max(0,s.doc().length()-1));
    const auto axis=s.doc().timeline();auto pClock=axis.position(static_cast<std::int32_t>(raw));
    if(rows[*at].kind==SegmentTrackKind::band){pClock.tick=0;s.cursor=axis.clocks(pClock);s.bandCell=Session::BandCell{rows[*at].track,s.doc().selected_groups(),s.cursor};refresh(w,s);}
}
bool can_insert_band(Session& s){
    if(!s.bandCell||s.bandCell->groups!=s.doc().selected_groups()||s.bandCell->beat<0||s.bandCell->beat>=s.doc().length())return false;
    const auto d=track_document(s,SegmentTrackKind::band,s.bandCell->track);const auto axis=d.timeline();const auto events=d.band_events();
    return std::none_of(events.begin(),events.end(),[&](const auto& e){const auto p=axis.position(e.logicalTime);return axis.clocks({p.measure,p.beat,0})==s.bandCell->beat;});
}
void insert_band(HWND w,Session& s){
    if(!can_insert_band(s))return;const auto cell=*s.bandCell;auto& d=s.doc();
    d.select_track_group(d.selected_groups(),d.selected_tempo_index(),d.selected_meter_index(),d.selected_sequence_index(),cell.track);
    size_t index=0;if(d.insert_default_band(cell.beat,&index))select(w,s,TimelineEventTarget{TimelineEventKind::band,s.document,cell.track,index,cell.groups},true);
}
void delete_band(HWND w,Session& s){
    if(!s.selected||s.selected->kind!=TimelineEventKind::band)return;const auto target=*s.selected;auto& d=s.doc();
    d.select_track_group(target.groups,d.selected_tempo_index(),d.selected_meter_index(),d.selected_sequence_index(),target.track);
    if(d.delete_band(target.event))select(w,s,{});
}
void line(HDC dc,int x,int y,int end,int width,COLORREF color){auto p=CreatePen(PS_SOLID,width,color);auto old=SelectObject(dc,p);MoveToEx(dc,x,y,nullptr);LineTo(dc,x,end);SelectObject(dc,old);DeleteObject(p);}
void text(HDC dc,const std::wstring& value,RECT r){DrawTextW(dc,value.c_str(),static_cast<int>(value.size()),&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);}
void fill(HDC dc,RECT r,COLORREF color){auto brush=CreateSolidBrush(color);FillRect(dc,&r,brush);DeleteObject(brush);}
bool black_pitch(unsigned pitch){const auto key=pitch%12;return key==1||key==3||key==6||key==8||key==10;}
RECT sequence_note_rect(const Viewport& v,int top,int topPitch,const Note& note){
    const int bottom=top+rowHeight+(topPitch-int(note.pitch)+1)*pitchHeight;
    return {pixel(v,note.time),bottom-std::max(2,int(note.velocity)*pitchHeight/127),std::max(pixel(v,note.time)+2,pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(note.time)+note.duration)))),bottom-1};
}
std::optional<Row> active_sequence(Session& s){if(!s.activeSequence)return {};for(const auto& row:view_rows(s))if(row.kind==SegmentTrackKind::sequence&&sequence_key(s,row)==*s.activeSequence)return row;return {};}
struct CcLane {Session::SequenceKey key;SequenceController controller;int top;bool expanded;};
std::optional<CcLane> cc_lane_at(HWND w,Session& s,POINT p){
    if(p.y<rulerHeight||p.y>=strip_bottom(w))return {};const auto rows=view_rows(s);const auto slot=row_at(rows,p.y+s.verticalScroll-rulerHeight);if(!slot||rows[*slot].kind!=SegmentTrackKind::sequence)return {};
    const auto key=sequence_key(s,rows[*slot]);const auto& state=s.sequences[key];if(!state.expanded)return {};int top=rulerHeight+row_top(rows,*slot)-s.verticalScroll+rowHeight+pitchRows*pitchHeight;
    for(const auto& lane:state.cc){const auto height=ccHeight+(lane.second?ccGraphHeight:0);if(p.y>=top&&p.y<top+height)return CcLane{key,lane.first,top,lane.second};top+=height;}return {};
}
int cc_value_y(const CcLane& lane,double value){const auto low=sequence_curve_minimum(lane.controller),high=sequence_curve_maximum(lane.controller);return lane.top+ccHeight+ccGraphHeight-4-static_cast<int>(std::lround((value-low)*(ccGraphHeight-8)/(high-low)));}
int cc_y_value(const CcLane& lane,int y){const auto low=sequence_curve_minimum(lane.controller),high=sequence_curve_maximum(lane.controller);return std::clamp(static_cast<int>(std::lround(low+double(lane.top+ccHeight+ccGraphHeight-4-y)*(high-low)/(ccGraphHeight-8))),low,high);}
std::int32_t cc_clock(Session& s,const Viewport& v,int x){return sequence_snap_clock(s.doc().timeline(),static_cast<std::int32_t>(std::clamp<std::int64_t>(std::int64_t(v.begin)+static_cast<std::int64_t>(std::llround((x-labelWidth)/v.scale)),0,s.doc().length()-1)),s.doc().length(),s.sequences[*s.activeSequence].snap);}
std::optional<size_t> cc_curve_at(Session& s,const CcLane& lane,const Viewport& v,POINT p){
    if(!lane.expanded||p.x<labelWidth||p.y<lane.top+ccHeight)return {};const auto curves=track_document(s,SegmentTrackKind::sequence,std::get<1>(lane.key)).curves();
    for(size_t i=curves.size();i>0;--i){const auto& c=curves[i-1];if(c.channel!=std::get<2>(lane.key)||!(c.controller==lane.controller)||!valid_sequence_curve(c))continue;
        const int x1=pixel(v,c.time),x2=pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(c.time)+c.duration))),y1=cc_value_y(lane,c.start),y2=cc_value_y(lane,c.end);
        RECT r{x1-3,std::min(y1,y2)-4,std::max(x1+4,x2+4),std::max(y1,y2)+5};if(PtInRect(&r,p))return i-1;
    }return {};
}
void add_cc_lane(HWND w,Session& s){const auto row=active_sequence(s);if(!row)return;auto& state=s.sequences[*s.activeSequence];std::vector<SequenceController> existing;for(const auto& c:state.cc)existing.push_back(c.first);const auto chosen=show_sequence_cc_editor(w,existing);if(!chosen)return;
    s.doc().select_track_group(s.doc().selected_groups(),s.doc().selected_tempo_index(),s.doc().selected_meter_index(),row->track,s.doc().selected_band_index());
    s.doc().add_cc_lane(*row->channel,*chosen);state.cc.try_emplace(*chosen,false);state.activeCC=*chosen;state.curve.reset();state.expanded=true;refresh(w,s);s.publish();
}
void arm_curve(HWND w,Session& s){if(!s.activeSequence)return;auto& state=s.sequences[*s.activeSequence];if(!state.activeCC)return;state.cc[*state.activeCC]=true;state.drawCurve=true;state.curve.reset();refresh(w,s);s.publish();}
void delete_curve(HWND w,Session& s){const auto row=active_sequence(s);if(!row)return;auto& state=s.sequences[*s.activeSequence];if(state.curve&&s.doc().delete_curve(*state.curve)){state.curve.reset();refresh(w,s);s.publish();}}
void edit_curve(HWND w,Session& s){const auto row=active_sequence(s);if(!row)return;auto& state=s.sequences[*s.activeSequence];const auto curves=s.doc().curves();if(!state.curve||*state.curve>=curves.size())return;const auto edited=show_sequence_curve_editor(w,curves[*state.curve],s.doc().length());if(!edited)return;size_t after=0;if(s.doc().edit_curve(*state.curve,*edited,&after)){state.curve=after;refresh(w,s);s.publish();}}
void cancel_curve_drag(HWND w,Session& s){if(!s.curveDrag)return;s.curveDrag.reset();if(GetCapture()==w)ReleaseCapture();refresh(w,s);s.publish();}
bool begin_curve_drag(HWND w,Session& s,POINT p){
    const auto lane=cc_lane_at(w,s,p);if(!lane||!lane->expanded||p.y<lane->top+ccHeight||p.x<labelWidth)return false;auto& state=s.sequences[lane->key];const auto v=viewport(w,s);std::optional<size_t> index;SequenceCurve curve;int handle=0;
    if(state.drawCurve){curve.time=cc_clock(s,v,p.x);curve.duration=1;curve.channel=std::get<2>(lane->key);curve.controller=lane->controller;curve.start=curve.end=static_cast<short>(cc_y_value(*lane,p.y));curve.reset=static_cast<short>(lane->controller.type==3?0:lane->controller.type==4&&lane->controller.data==11?127:0);curve.shape=4;}
    else{index=cc_curve_at(s,*lane,v,p);if(!index)return false;curve=s.doc().curves()[*index];handle=curve.duration==0?3:std::abs(p.x-pixel(v,curve.time))<=5?1:std::abs(p.x-pixel(v,curve.time+curve.duration))<=5?2:3;}
    s.curveDrag=Session::CurveDrag{lane->key,lane->controller,index,curve,curve,handle,p,p,s.doc().save_bytes()};state.curve=index;SetCapture(w);refresh(w,s);return true;
}
void preview_curve_drag(HWND w,Session& s,POINT p){
    if(!s.curveDrag)return;auto& drag=*s.curveDrag;drag.current=p;const auto lane=cc_lane_at(w,s,drag.start);if(!lane||!(lane->controller==drag.controller))return;const auto v=viewport(w,s);auto next=drag.original;
    const int value=cc_y_value(*lane,p.y);const auto at=cc_clock(s,v,p.x),end=next.time+next.duration;
    if(!drag.index){const auto first=cc_clock(s,v,drag.start.x);next.time=std::min(first,at);next.duration=std::max(1,std::abs(at-first));if(at>=first)next.end=static_cast<short>(value);else{next.start=static_cast<short>(value);next.end=drag.original.start;}}
    else if(drag.handle==1){next.time=std::min(at,end-1);next.duration=end-next.time;next.start=static_cast<short>(value);}
    else if(drag.handle==2){next.duration=std::max(1,at-next.time);next.end=static_cast<short>(value);}
    else{const auto dt=std::int64_t(at)-cc_clock(s,v,drag.start.x);next.time=static_cast<std::int32_t>(std::clamp<std::int64_t>(std::int64_t(next.time)+dt,0,s.doc().length()-next.duration));const int dv=std::clamp(value-cc_y_value(*lane,drag.start.y),sequence_curve_minimum(next.controller)-std::min(next.start,next.end),sequence_curve_maximum(next.controller)-std::max(next.start,next.end));next.start=static_cast<short>(next.start+dv);next.end=static_cast<short>(next.end+dv);}
    next.duration=std::min(next.duration,s.doc().length()-next.time);drag.preview=next;refresh(w,s);
}
void commit_curve_drag(HWND w,Session& s,POINT p){
    if(!s.curveDrag)return;preview_curve_drag(w,s,p);const auto drag=*s.curveDrag;s.curveDrag.reset();if(GetCapture()==w)ReleaseCapture();auto& state=s.sequences[drag.key];state.drawCurve=false;
    if(s.doc().save_bytes()!=drag.before||(!drag.index&&p.x==drag.start.x&&p.y==drag.start.y)){refresh(w,s);s.publish();return;}
    s.doc().select_track_group(std::get<0>(drag.key),s.doc().selected_tempo_index(),s.doc().selected_meter_index(),std::get<1>(drag.key),s.doc().selected_band_index());size_t after=0;
    const bool changed=drag.index?s.doc().edit_curve(*drag.index,drag.preview,&after):s.doc().add_curve(drag.preview,&after);if(changed)state.curve=after;refresh(w,s);s.publish();
}
void follow_sequence_pitch(Session::SequenceStrip& state){if(int(state.pitch)>state.topPitch)state.topPitch=int(state.pitch);if(int(state.pitch)<=state.topPitch-pitchRows)state.topPitch=int(state.pitch)+pitchRows-1;state.topPitch=std::clamp(state.topPitch,pitchRows-1,127);}
void insert_sequence_note(HWND w,Session& s){
    const auto row=active_sequence(s);if(!row)return;auto& state=s.sequences[*s.activeSequence];if(!state.expanded)return;
    auto& d=s.doc();d.select_track_group(d.selected_groups(),d.selected_tempo_index(),d.selected_meter_index(),row->track,d.selected_band_index());
    const auto duration=std::min(sequence_default_duration(d.timeline(),s.cursor,state.snap),d.length()-s.cursor);if(duration<=0)return;
    Note note{s.cursor,duration,*row->channel,static_cast<std::uint8_t>(state.pitch),96};size_t index=0;if(!d.add_note(note,&index))return;
    state.note=index;s.selected=TimelineEventTarget{TimelineEventKind::sequence_note,s.document,row->track,index,d.selected_groups()};
    const auto next=std::int64_t(s.cursor)+sequence_grid_span(d.timeline(),s.cursor);if(next<d.length())s.cursor=static_cast<std::int32_t>(next);
    refresh(w,s);s.publish();if(s.audition)s.audition(s.document,note);
}
void delete_sequence_note(HWND w,Session& s){const auto row=active_sequence(s);if(!row)return;auto& state=s.sequences[*s.activeSequence];if(!state.note)return;
    auto& d=s.doc();d.select_track_group(d.selected_groups(),d.selected_tempo_index(),d.selected_meter_index(),row->track,d.selected_band_index());
    if(d.delete_note(*state.note)){state.note.reset();s.selected.reset();refresh(w,s);s.publish();}}
void edit_sequence_velocity(HWND w,Session& s){
    const auto row=active_sequence(s);if(!row)return;const auto selected=s.sequences[*s.activeSequence].note;
    const auto command=show_sequence_velocity_editor(w,selected.has_value());if(!command)return;
    struct Target {size_t track,index;Note note;};std::vector<Target> targets;
    for(const auto& track:s.doc().tracks())if(track.kind==SegmentTrackKind::sequence&&(command->target==SequenceVelocityTarget::allParts||track.index==row->track)){
        const auto notes=track_document(s,track.kind,track.index).notes();for(size_t i=0;i<notes.size();++i){
            if(command->target!=SequenceVelocityTarget::allParts&&notes[i].channel!=*row->channel)continue;
            if(command->target==SequenceVelocityTarget::selected&&(!selected||i!=*selected))continue;targets.push_back({track.index,i,notes[i]});}}
    if(targets.empty())return;std::int32_t first=INT32_MAX,last=0;for(const auto& t:targets){first=std::min(first,t.note.time);last=std::max(last,t.note.time);}
    std::vector<SequenceVelocityEdit> edits;for(const auto& t:targets)edits.push_back({s.doc().selected_groups(),t.track,t.index,sequence_velocity_value(t.note,first,last,command->options)});
    if(s.doc().edit_note_velocities(edits)){refresh(w,s);s.publish();if(selected&&s.audition){const auto notes=track_document(s,SegmentTrackKind::sequence,row->track).notes();if(*selected<notes.size())s.audition(s.document,notes[*selected]);}}
}
void quantize_sequence_notes(HWND w,Session& s){
    const auto row=active_sequence(s);if(!row||!s.sequences[*s.activeSequence].expanded||s.sequences[*s.activeSequence].ccContext)return;
    const auto selected=s.sequences[*s.activeSequence].note;const auto command=show_sequence_quantize_editor(w,selected.has_value());if(!command)return;
    std::vector<SequenceTimingEdit> edits;
    for(const auto& track:s.doc().tracks())if(track.kind==SegmentTrackKind::sequence&&(command->target==SequenceQuantizeTarget::allParts||track.index==row->track)){
        const auto context=track_document(s,track.kind,track.index);const auto notes=context.notes();for(size_t i=0;i<notes.size();++i){
            if(command->target!=SequenceQuantizeTarget::allParts&&notes[i].channel!=*row->channel)continue;
            if(command->target==SequenceQuantizeTarget::selected&&(!selected||i!=*selected))continue;
            const auto note=sequence_quantize_note(context.timeline(),s.doc().length(),notes[i],command->options);edits.push_back({s.doc().selected_groups(),track.index,i,note.time,note.duration});
        }
    }
    std::vector<size_t> indices;if(!s.doc().edit_note_timings(edits,&indices))return;
    // Original indexes are invalid after a multi-part reorder. Keep a single
    // selected target through the returned mapping; require reselection for bulk.
    for(auto& pair:s.sequences)pair.second.note.reset();s.selected.reset();
    if(command->target==SequenceQuantizeTarget::selected&&indices.size()==1){s.sequences[*s.activeSequence].note=indices.front();s.selected=TimelineEventTarget{TimelineEventKind::sequence_note,s.document,row->track,indices.front(),s.doc().selected_groups()};}
    refresh(w,s);s.publish();
}
struct NoteHit {Session::SequenceKey key;size_t index;Note value;SequenceNoteHandle handle;};
std::optional<NoteHit> sequence_note_hit(HWND w,Session& s,POINT p){
    const auto rows=view_rows(s);const auto slot=row_at(rows,p.y+s.verticalScroll-rulerHeight);
    if(!slot||p.x<labelWidth||p.y<rulerHeight||p.y>=strip_bottom(w))return {};const auto& row=rows[*slot];if(row.kind!=SegmentTrackKind::sequence)return {};
    const auto key=sequence_key(s,row);const auto& state=s.sequences[key];if(!state.expanded)return {};
    const int top=rulerHeight+row_top(rows,*slot)-s.verticalScroll;const auto v=viewport(w,s);const auto notes=track_document(s,row.kind,row.track).notes();
    for(size_t i=notes.size();i>0;--i){const auto& n=notes[i-1];if(n.channel!=*row.channel)continue;const auto r=sequence_note_rect(v,top,state.topPitch,n);if(!PtInRect(&r,p))continue;
        const auto handle=p.x-r.left<2?SequenceNoteHandle::start:r.right-p.x<=2?SequenceNoteHandle::end:p.y-r.top<2?SequenceNoteHandle::velocity:SequenceNoteHandle::move;
        return NoteHit{key,i-1,n,handle};}return {};
}
void cancel_note_drag(HWND w,Session& s){if(!s.noteDrag)return;s.noteDrag.reset();if(GetCapture()==w)ReleaseCapture();refresh(w,s);s.publish();}
bool begin_note_drag(HWND w,Session& s,POINT p){
    if(GetKeyState(VK_CONTROL)&0x8000)return false;const auto hit=sequence_note_hit(w,s,p);if(!hit)return false;
    s.noteDrag=Session::NoteDrag{hit->key,hit->index,hit->value,hit->value,hit->handle,p,p,s.doc().save_bytes()};SetCapture(w);return true;
}
void preview_note_drag(HWND w,Session& s,POINT p){
    if(!s.noteDrag)return;auto& drag=*s.noteDrag;drag.current=p;if(p.x==drag.start.x&&p.y==drag.start.y&&!drag.fineClock&&!drag.finePitch&&!drag.fineVelocity)return;const auto v=viewport(w,s);const auto axis=s.doc().timeline();
    const auto delta=static_cast<std::int64_t>(std::llround((p.x-drag.start.x)/v.scale));
    const auto pitch=static_cast<int>(std::lround(double(drag.start.y-p.y)/pitchHeight));
    const auto velocity=static_cast<int>(std::lround((drag.start.y-p.y)*127./pitchHeight/5))*5;
    const auto snap=s.sequences[drag.key].snap;
    auto next=sequence_drag_note(axis,s.doc().length(),drag.original,drag.handle,delta,pitch,velocity,snap);
    next=sequence_drag_note(axis,s.doc().length(),next,drag.handle,drag.fineClock,drag.finePitch,drag.fineVelocity,SequenceSnap::none);
    if(next.time==drag.preview.time&&next.duration==drag.preview.duration&&next.pitch==drag.preview.pitch&&next.velocity==drag.preview.velocity)return;
    drag.preview=next;s.cursor=next.time;s.sequences[drag.key].pitch=next.pitch;refresh(w,s);s.publish();if(s.audition)s.audition(s.document,next);
}
void commit_note_drag(HWND w,Session& s,POINT p){
    if(!s.noteDrag)return;preview_note_drag(w,s,p);const auto drag=*s.noteDrag;s.noteDrag.reset();if(GetCapture()==w)ReleaseCapture();
    if(s.doc().save_bytes()!=drag.before){refresh(w,s);s.publish();return;}
    s.doc().select_track_group(std::get<0>(drag.key),s.doc().selected_tempo_index(),s.doc().selected_meter_index(),std::get<1>(drag.key),s.doc().selected_band_index());size_t index=drag.index;
    if(s.doc().edit_note(drag.index,drag.preview,&index)){s.sequences[drag.key].note=index;s.selected=TimelineEventTarget{TimelineEventKind::sequence_note,s.document,std::get<1>(drag.key),index,std::get<0>(drag.key)};}
    refresh(w,s);s.publish();
}
bool note_drag_key(HWND w,Session& s,WPARAM key){
    if(!s.noteDrag)return false;if(key==VK_ESCAPE){cancel_note_drag(w,s);return true;}auto& d=*s.noteDrag;
    const int horizontal=key==VK_LEFT?-1:key==VK_RIGHT?1:0,vertical=key==VK_UP?1:key==VK_DOWN?-1:0;if(!horizontal&&!vertical)return false;
    if(d.handle==SequenceNoteHandle::velocity)d.fineVelocity+=vertical;
    else if(d.handle==SequenceNoteHandle::start||d.handle==SequenceNoteHandle::end)d.fineClock+=horizontal;
    else {SequenceCursor cursor{d.preview.time,d.preview.pitch,s.sequences[d.key].snap};move_sequence_cursor(s.doc().timeline(),s.doc().length(),cursor,horizontal,vertical);d.fineClock+=std::int64_t(cursor.clock)-d.preview.time;d.finePitch+=int(cursor.pitch)-d.preview.pitch;}
    preview_note_drag(w,s,d.current);return true;
}
void sequence_instrument_menu(HWND w,Session& s,const Row& row,bool patches,POINT point){
    auto menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Instrument menu creation failed");
    const auto current=current_sequence_band(s.doc(),s.cursor);const auto assigned=sequence_patch(s.doc(),s.cursor,*row.channel);
    if(!patches){AppendMenuW(menu,MF_STRING|(current?0:MF_GRAYED),OpenSequenceBand,L"Open Band Editor");AppendMenuW(menu,MF_STRING,NewInstrument,L"Insert New Instrument");AppendMenuW(menu,MF_STRING|(current?0:MF_GRAYED),LocateSequenceBand,L"Locate Band");}
    else{const auto& instruments=gm_instruments();const bool drums=*row.channel==9;
        for(unsigned family=0;family<17;++family){if((family==16)!=drums)continue;auto group=CreatePopupMenu(),gs=CreatePopupMenu();bool any=false,hasGs=false;
            for(size_t i=0;i<instruments.size();++i){const auto& item=instruments[i];if(item.family!=family)continue;const bool variant=(item.patch&0x7f7f00u)!=0&&!item.percussion;const auto checked=assigned&&assigned->collectionReference.empty()&&assigned->patch==item.patch?MF_CHECKED:0;
                AppendMenuW(variant?gs:group,MF_STRING|checked,FirstPatch+static_cast<UINT>(i),item.name);any=true;hasGs=hasGs||variant;}
            if(hasGs)InsertMenuW(group,0,MF_BYPOSITION|MF_POPUP,reinterpret_cast<UINT_PTR>(gs),L"GS");else DestroyMenu(gs);
            if(any&&drums){DestroyMenu(group);for(size_t i=0;i<instruments.size();++i)if(instruments[i].percussion)AppendMenuW(menu,MF_STRING|(assigned&&assigned->collectionReference.empty()&&assigned->patch==instruments[i].patch?MF_CHECKED:0),FirstPatch+static_cast<UINT>(i),instruments[i].name);}
            else if(any)AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(group),gm_instrument_family(family));else DestroyMenu(group);}
        AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,OtherDls,L"Other DLS...");}
    const auto chosen=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,w,nullptr);DestroyMenu(menu);
    if(chosen==OtherDls){const auto selection=show_sequence_dls_editor(w,s.host,*row.channel,assigned);if(selection&&s.host.assign_sequence_collection_patch(s.document,s.cursor,*row.channel,selection->collection,selection->bank,selection->program)){refresh(w,s);s.publish();}return;}
    if(chosen==NewInstrument||chosen>=FirstPatch){const auto& table=gm_instruments();std::uint32_t patch=*row.channel==9?0x80000000u:0;
        if(chosen>=FirstPatch){const auto index=chosen-FirstPatch;if(index>=table.size())return;patch=table[index].patch;}
        if(assign_sequence_gm_patch(s.doc(),s.cursor,*row.channel,patch)){refresh(w,s);s.publish();}return;}
    if(current&&(chosen==OpenSequenceBand||chosen==LocateSequenceBand)){auto target=make_embedded_band_target(s.host,s.document,s.doc().selected_groups(),current->track,current->event);
        if(chosen==OpenSequenceBand)show_embedded_band_editor(workspace,s.host,target,s.changed,s.history,s.mixStarted);
        else{s.activeSequence.reset();select(w,s,TimelineEventTarget{TimelineEventKind::band,s.document,current->track,current->event,s.doc().selected_groups()});}}
}
bool click_sequence(HWND w,Session& s,POINT p,bool doubleClick,bool context){
    const auto rows=view_rows(s);const auto slot=row_at(rows,p.y+s.verticalScroll-rulerHeight);if(!slot||p.y<rulerHeight||p.y>=strip_bottom(w)||rows[*slot].kind!=SegmentTrackKind::sequence)return false;
    const auto row=rows[*slot];const auto key=sequence_key(s,row);auto& state=s.sequences[key];const int top=rulerHeight+row_top(rows,*slot)-s.verticalScroll;const auto v=viewport(w,s);
    if(s.selected&&s.selected->kind==TimelineEventKind::tempo)s.doc().select(std::vector<size_t>{});
    s.doc().select_track_group(s.doc().selected_groups(),s.doc().selected_tempo_index(),s.doc().selected_meter_index(),row.track,s.doc().selected_band_index());
    s.activeSequence=key;s.selected.reset();s.muteCell.reset();s.bandCell.reset();SetFocus(w);
    if(state.note&&*state.note<s.doc().notes().size())s.selected=TimelineEventTarget{TimelineEventKind::sequence_note,s.document,row.track,*state.note,s.doc().selected_groups()};
    const bool header=p.y<top+rowHeight;
    if(!context&&((p.x>=v.right-22&&p.y<top+22)||(doubleClick&&(!state.expanded||(header&&p.x<labelWidth))))){
        state.expanded=!state.expanded;refresh(w,s);s.publish();return true;}
    if(!state.expanded){refresh(w,s);s.publish();return true;}
    if(p.y>=top+rowHeight+pitchHeight*pitchRows){
        state.ccContext=true;const auto lane=cc_lane_at(w,s,p);state.note.reset();state.curve.reset();state.activeCC=lane?std::optional<SequenceController>(lane->controller):std::nullopt;
        if(lane){const bool laneHeader=p.y<lane->top+ccHeight;
            if(!context&&laneHeader&&((p.x>=v.right-22)||doubleClick)){state.cc[lane->controller]=!lane->expanded;refresh(w,s);s.publish();return true;}
            if(!laneHeader){state.curve=cc_curve_at(s,*lane,v,p);if(!context&&doubleClick&&state.curve){edit_curve(w,s);return true;}}
        }refresh(w,s);s.publish();return true;
    }
    state.ccContext=false;state.activeCC.reset();state.curve.reset();state.drawCurve=false;
    if(!context&&header&&p.x<labelWidth){if(p.x<labelWidth-30&&p.y>=top+18){if(sequence_patch(s.doc(),s.cursor,*row.channel)){POINT anchor{0,top+rowHeight};ClientToScreen(w,&anchor);sequence_instrument_menu(w,s,row,true,anchor);}}
        else if(p.x>=labelWidth-30&&p.y>=top+18){POINT anchor{labelWidth-30,top+rowHeight};ClientToScreen(w,&anchor);sequence_instrument_menu(w,s,row,false,anchor);}refresh(w,s);s.publish();return true;}
    if(!context&&!header&&p.x>=labelWidth-20&&p.x<labelWidth){const int direction=p.y<top+rowHeight+20?1:p.y>=top+rowHeight+pitchRows*pitchHeight-20?-1:0;if(direction)state.topPitch=std::clamp(state.topPitch+direction*12,pitchRows-1,127);refresh(w,s);s.publish();return true;}
    const int relative=p.y-top-rowHeight;
    if(relative<0||relative>=pitchHeight*pitchRows){refresh(w,s);s.publish();return true;}
    state.pitch=static_cast<unsigned>(std::clamp(state.topPitch-relative/pitchHeight,0,127));
    if(p.x>=labelWidth&&p.x<=v.right){const auto raw=std::clamp<std::int64_t>(std::int64_t(v.begin)+static_cast<std::int64_t>(std::floor((p.x-labelWidth)/v.scale)),0,s.doc().length()-1);s.cursor=sequence_snap_clock(s.doc().timeline(),static_cast<std::int32_t>(raw),s.doc().length(),state.snap);
        const auto notes=s.doc().notes();state.note.reset();for(size_t i=notes.size();i>0;--i){const auto& note=notes[i-1];if(note.channel!=*row.channel)continue;const auto rect=sequence_note_rect(v,top,state.topPitch,note);if(PtInRect(&rect,p)){state.note=i-1;state.pitch=note.pitch;break;}}
        s.selected.reset();if(state.note)s.selected=TimelineEventTarget{TimelineEventKind::sequence_note,s.document,row.track,*state.note,s.doc().selected_groups()};
        if(!context&&(GetKeyState(VK_CONTROL)&0x8000)){insert_sequence_note(w,s);return true;}
        refresh(w,s);s.publish();if(!context&&state.note&&s.audition)s.audition(s.document,notes[*state.note]);
    }else{refresh(w,s);s.publish();if(!context&&s.audition)s.audition(s.document,Note{s.cursor,384,*row.channel,static_cast<std::uint8_t>(state.pitch),96});}return true;
}
void paint_cc_curve(HDC dc,const CcLane& lane,const Viewport& v,const SequenceCurve& c,bool selected,bool preview){
    const int left=pixel(v,c.time),right=std::max(left+1,pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(c.time)+c.duration))));if(right<labelWidth||left>v.right)return;
    const int begin=std::max(left,labelWidth),end=std::min(right,v.right);std::vector<POINT> points;
    for(int x=begin;x<=end;++x)points.push_back({x,cc_value_y(lane,sequence_curve_value(c,double(x-left)/(right-left)))});
    const auto color=selected?RGB(255,0,0):RGB(0,0,255);auto pen=CreatePen(preview?PS_DOT:PS_SOLID,1,preview?RGB(0,0,0):color);auto oldPen=SelectObject(dc,pen);
    if(preview){if(points.size()>1)Polyline(dc,points.data(),static_cast<int>(points.size()));}
    else{points.push_back({end,cc_value_y(lane,lane.controller.type==3?0:sequence_curve_minimum(lane.controller))});points.push_back({begin,points.back().y});auto brush=CreateSolidBrush(color);auto oldBrush=SelectObject(dc,brush);Polygon(dc,points.data(),static_cast<int>(points.size()));SelectObject(dc,oldBrush);DeleteObject(brush);}
    SelectObject(dc,oldPen);DeleteObject(pen);
    if(selected||preview){const int y1=cc_value_y(lane,c.start),y2=cc_value_y(lane,c.end);RECT box{left,std::min(y1,y2)-1,right+1,std::max(y1,y2)+2};pen=CreatePen(preview?PS_DOT:PS_SOLID,1,RGB(0,0,0));oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,box.left,box.top,box.right,box.bottom);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
        if(!preview)for(auto pt:{POINT{left,y1},POINT{right,y2}})fill(dc,{pt.x-2,pt.y-2,pt.x+3,pt.y+3},RGB(0,0,0));}
}
void paint_cc(HDC dc,HWND w,Session& s,const Row& row,int top,const Viewport& v,bool active){
    const auto key=sequence_key(s,row);auto& state=s.sequences[key];const auto curves=track_document(s,SegmentTrackKind::sequence,row.track).curves();
    if(state.cc.empty()){fill(dc,{0,top,v.right,top+ccHeight},active?RGB(255,255,220):GetSysColor(COLOR_BTNFACE));text(dc,L"CC: No CC Tracks",{4,top,v.right,top+ccHeight});return;}
    for(const auto& entry:state.cc){const CcLane lane{key,entry.first,top,entry.second};const bool selected=active&&state.activeCC&&*state.activeCC==entry.first;
        fill(dc,{0,top,labelWidth,top+ccHeight},RGB(224,224,255));fill(dc,{labelWidth,top,v.right,top+ccHeight},GetSysColor(COLOR_BTNFACE));text(dc,L"CC: "+sequence_controller_name(entry.first),{4,top,labelWidth-3,top+ccHeight});
        RECT toggle{v.right-21,top+1,v.right-1,top+ccHeight-1};DrawFrameControl(dc,&toggle,DFC_BUTTON,DFCS_BUTTONPUSH);text(dc,entry.second?L"−":L"□",{v.right-17,top+1,v.right-3,top+ccHeight-1});
        if(entry.second){const int clip=SaveDC(dc);IntersectClipRect(dc,0,top+ccHeight,v.right,top+ccHeight+ccGraphHeight);
            fill(dc,{0,top+ccHeight,v.right,top+ccHeight+ccGraphHeight},selected?RGB(255,255,220):RGB(255,255,255));text(dc,std::to_wstring(sequence_curve_maximum(entry.first)),{4,top+ccHeight,labelWidth-2,top+ccHeight+18});text(dc,std::to_wstring(sequence_curve_minimum(entry.first)),{4,top+ccHeight+ccGraphHeight-18,labelWidth-2,top+ccHeight+ccGraphHeight});
            const int zero=cc_value_y(lane,entry.first.type==3?0:sequence_curve_minimum(entry.first));auto grid=CreatePen(PS_SOLID,1,RGB(160,160,160));auto old=SelectObject(dc,grid);MoveToEx(dc,labelWidth,zero,nullptr);LineTo(dc,v.right,zero);SelectObject(dc,old);DeleteObject(grid);
            if(fine_beats(s.doc().timeline(),v.begin,v.end))for(const auto& tick:s.doc().timeline().beat_ticks(v.begin,v.end))line(dc,pixel(v,tick.clocks),top+ccHeight,top+ccHeight+ccGraphHeight,1,RGB(160,160,255));
            for(size_t i=0;i<curves.size();++i)if(curves[i].channel==*row.channel&&curves[i].controller==entry.first&&valid_sequence_curve(curves[i]))paint_cc_curve(dc,lane,v,curves[i],selected&&state.curve&&*state.curve==i,false);
            if(s.curveDrag&&s.curveDrag->key==key&&s.curveDrag->controller==entry.first)paint_cc_curve(dc,lane,v,s.curveDrag->preview,true,true);RestoreDC(dc,clip);
        }top+=ccHeight+(entry.second?ccGraphHeight:0);
    }
}
void paint_time_grid(HDC dc,const Timeline& axis,const Viewport& v,int top,int bottom,bool ruler){
    const int clip=SaveDC(dc);IntersectClipRect(dc,labelWidth,top,v.right+1,bottom);
    if(fine_grid(axis,v)){int last=labelWidth-48;for(const auto& tick:axis.beat_ticks(v.begin,v.end)){
        const int x=pixel(v,tick.clocks);if(x<labelWidth||x>v.right)continue;
        line(dc,x,top,bottom,tick.beat==0?2:1,tick.beat==0?RGB(0,0,0):ruler?RGB(0,0,255):RGB(0,0,180));
        if(ruler&&tick.beat==0&&x-last>=48){text(dc,std::to_wstring(tick.measure+1),{x+3,2,std::min(v.right,x+60),rulerHeight-2});last=x;}
    }}else{
        const auto begin=axis.position(v.begin).measure,end=axis.position(v.end-1).measure;
        const auto step=std::max<std::int64_t>(1,(std::int64_t(end)-begin+1)/std::max(1,(v.right-labelWidth)/48)+1);
        for(auto measure=(std::int64_t(begin)+step-1)/step*step;measure<=end;measure+=step){
            const int x=pixel(v,axis.clocks({static_cast<std::int32_t>(measure),0,0}));if(x<labelWidth||x>v.right)continue;
            line(dc,x,top,bottom,2,RGB(0,0,0));if(ruler)text(dc,std::to_wstring(measure+1),{x+3,2,std::min(v.right,x+70),rulerHeight-2});
        }
    }
    RestoreDC(dc,clip);
}
void paint_sequence(HDC dc,HWND w,Session& s,const Row& row,int top,const Viewport& v){
    auto& state=s.sequences[sequence_key(s,row)];const bool active=s.activeSequence&&*s.activeSequence==sequence_key(s,row)&&GetFocus()==w;
    const int clip=SaveDC(dc);IntersectClipRect(dc,0,std::max(rulerHeight,top),v.right+1,std::min(strip_bottom(w),top+row.height));
    fill(dc,{0,top,labelWidth,top+row.height},GetSysColor(COLOR_BTNFACE));fill(dc,{0,top,labelWidth-1,top+18},RGB(224,224,255));text(dc,row.label,{4,top,labelWidth-2,top+18});
    RECT toggle{v.right-21,top+1,v.right-1,top+21};DrawFrameControl(dc,&toggle,DFC_BUTTON,DFCS_BUTTONPUSH);text(dc,state.expanded?L"−":L"□",{v.right-17,top+1,v.right-3,top+20});
    const auto d=track_document(s,SegmentTrackKind::sequence,row.track);const auto notes=d.notes();
    if(!state.expanded){IntersectClipRect(dc,labelWidth,top,v.right+1,top+row.height);for(size_t i=0;i<notes.size();++i)if(notes[i].channel==*row.channel){const auto& n=notes[i];const int y=top+20+(127-n.pitch)*12/128;fill(dc,{pixel(v,n.time),y,std::max(pixel(v,n.time)+2,pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(n.time)+n.duration)))),y+2},state.note&&*state.note==i?RGB(255,0,0):RGB(0,0,255));}RestoreDC(dc,clip);return;}
    const auto patch=sequence_patch(s.doc(),s.cursor,*row.channel);RECT instrument{4,top+18,labelWidth-31,top+36};DrawFrameControl(dc,&instrument,DFC_BUTTON,DFCS_BUTTONPUSH|(patch?0:DFCS_INACTIVE));text(dc,patch?sequence_instrument_label(s.host,*patch):L"No Instrument",{6,top+18,labelWidth-33,top+36});
    RECT band{labelWidth-29,top+18,labelWidth-3,top+36};DrawFrameControl(dc,&band,DFC_BUTTON,DFCS_BUTTONPUSH);text(dc,L"♪",{labelWidth-24,top+18,labelWidth-5,top+36});
    const int noteTop=top+rowHeight,noteBottom=noteTop+pitchRows*pitchHeight;
    for(int i=0;i<pitchRows;++i){const int pitch=state.topPitch-i,y=noteTop+i*pitchHeight;const bool black=black_pitch(static_cast<unsigned>(pitch));
        fill(dc,{labelWidth,y,v.right,y+pitchHeight},black?RGB(198,198,198):active?RGB(255,255,224):RGB(255,255,255));
        fill(dc,{0,y,labelWidth-21,y+pitchHeight},active?RGB(255,255,232):RGB(255,255,255));
        if(black)fill(dc,{0,y,labelWidth-21,y+pitchHeight-1},RGB(0,0,0));
        if(*row.channel==9){const auto name=gm_drum_name(static_cast<unsigned>(pitch));SetTextColor(dc,black?RGB(255,255,232):RGB(0,0,0));text(dc,name.empty()?L"":(pitch%12==0?L"C"+std::to_wstring(pitch/12)+L": ":L"")+name,{3,y,labelWidth-24,y+pitchHeight});SetTextColor(dc,RGB(0,0,0));}
        else if(!black&&pitch%12==0)text(dc,L"C "+std::to_wstring(pitch/12),{3,y,labelWidth-24,y+pitchHeight});
        MoveToEx(dc,0,y+pitchHeight,nullptr);LineTo(dc,v.right,y+pitchHeight);}
    text(dc,L"▲",{labelWidth-20,noteTop,labelWidth-1,noteTop+20});text(dc,L"▼",{labelWidth-20,noteBottom-20,labelWidth-1,noteBottom});
    const int notesClip=SaveDC(dc);IntersectClipRect(dc,labelWidth,noteTop,v.right+1,noteBottom);
    const auto axis=d.timeline();if(fine_grid(axis,v)){
        auto grid=sequence_snap_clock(axis,v.begin,d.length(),SequenceSnap::grid);unsigned count=0;while(grid<v.end&&count++<8192){const auto span=sequence_grid_span(axis,grid);if(span*v.scale>=6)line(dc,pixel(v,grid),noteTop,noteBottom,1,RGB(110,110,210));const auto next=std::int64_t(grid)+span;if(next>INT32_MAX)break;grid=static_cast<std::int32_t>(next);}}
    paint_time_grid(dc,axis,v,noteTop,noteBottom,false);
    if(active){const int y=noteTop+(state.topPitch-int(state.pitch))*pitchHeight;const auto span=sequence_default_duration(axis,s.cursor,state.snap);RECT ghost{pixel(v,s.cursor),y+2,pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(s.cursor)+span))),y+pitchHeight-1};fill(dc,ghost,RGB(190,190,255));}
    for(size_t i=0;i<notes.size();++i)if(notes[i].channel==*row.channel){const auto& note=s.noteDrag&&s.noteDrag->key==sequence_key(s,row)&&s.noteDrag->index==i?s.noteDrag->preview:notes[i];const auto rect=sequence_note_rect(v,top,state.topPitch,note);fill(dc,rect,state.note&&*state.note==i?RGB(255,0,0):RGB(0,0,255));}RestoreDC(dc,notesClip);
    paint_cc(dc,w,s,row,noteBottom,v,active);
    MoveToEx(dc,0,top+row.height-1,nullptr);LineTo(dc,v.right,top+row.height-1);RestoreDC(dc,clip);
}
void paint(HWND w,Session& s){
    struct Painting{HWND w;PAINTSTRUCT ps{};HDC dc;Painting(HWND x):w(x),dc(BeginPaint(x,&ps)){}~Painting(){EndPaint(w,&ps);}} painting(w);
    auto dc=painting.dc;const auto oldFont=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));
    const auto v=viewport(w,s);RECT body{0,0,v.right+1,strip_bottom(w)};FillRect(dc,&body,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));const auto rows=view_rows(s);
    const auto top=[&](size_t row){return rulerHeight+row_top(rows,row)-s.verticalScroll;};
    const int labelClip=SaveDC(dc);IntersectClipRect(dc,0,rulerHeight,v.right+1,body.bottom);
    for(size_t i=0;i<rows.size();++i){RECT label{0,top(i),labelWidth-1,top(i+1)};if(label.bottom<=rulerHeight||label.top>=body.bottom)continue;FillRect(dc,&label,reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1));label.left=6;text(dc,rows[i].label,label);MoveToEx(dc,0,label.bottom,nullptr);LineTo(dc,v.right,label.bottom);}RestoreDC(dc,labelClip);
    line(dc,labelWidth-1,0,body.bottom,2,RGB(0,0,0));const auto axis=s.doc().timeline();const bool fine=fine_beats(axis,v.begin,v.end);
    paint_time_grid(dc,axis,v,0,body.bottom,true);
    const auto selected=[&](TimelineEventKind kind,size_t event,size_t track){return s.selected&&s.selected->kind==kind&&s.selected->event==event&&s.selected->track==track;};
    for(size_t rowIndex=0;rowIndex<rows.size();++rowIndex){const auto& row=rows[rowIndex];const int y=top(rowIndex);if(y+row.height<=rulerHeight||y>=body.bottom)continue;
        if(row.kind==SegmentTrackKind::sequence){paint_sequence(dc,w,s,row,y,v);continue;}
        const int clip=SaveDC(dc);IntersectClipRect(dc,labelWidth,std::max(rulerHeight,y),v.right+1,std::min(static_cast<int>(body.bottom),y+row.height));
        const auto eventLabel=[&](TimelineEventKind kind,size_t event,std::int32_t clock,std::int32_t next,const std::wstring& value){const int x=pixel(v,clock);if(x<labelWidth||x>v.right)return;const int right=kind==TimelineEventKind::band?band_label_right(dc,v,x,next,value):std::min(v.right,pixel(v,next)-2);RECT label{x+3,y+2,right,y+rowHeight-2};if(selected(kind,event,row.track)){RECT mark{x,y+2,std::min(v.right,x+8),y+rowHeight-2};FillRect(dc,&mark,reinterpret_cast<HBRUSH>(COLOR_HIGHLIGHT+1));}if(label.right>label.left)text(dc,value,label);};
        if(row.kind==SegmentTrackKind::tempo){const auto d=track_document(s,row.kind,row.track);const auto& events=d.tempos();for(size_t i=0;i<events.size();++i){auto value=std::to_wstring(events[i].bpm);while(value.size()>1&&value.back()==L'0')value.pop_back();if(value.back()==L'.')value.pop_back();eventLabel(TimelineEventKind::tempo,i,events[i].time,i+1<events.size()?events[i+1].time:s.doc().length(),value);}}
        if(row.kind==SegmentTrackKind::meter){const auto d=track_document(s,row.kind,row.track);const auto timeline=d.timeline();const auto& events=timeline.meters();for(size_t i=0;i<events.size();++i)eventLabel(TimelineEventKind::meter,i,timeline.clocks({events[i].measure,0,0}),i+1<events.size()?timeline.clocks({events[i+1].measure,0,0}):s.doc().length(),std::to_wstring(events[i].beats)+L" / "+std::to_wstring(events[i].denominator));}
        if(row.kind==SegmentTrackKind::sequence){const auto d=track_document(s,row.kind,row.track);for(const auto& n:d.notes()){const int left=pixel(v,n.time),right=pixel(v,static_cast<std::int32_t>(std::min<std::int64_t>(INT32_MAX,std::int64_t(n.time)+n.duration)));RECT rect{left,y+8,std::max(left+2,right),y+rowHeight-8};FillRect(dc,&rect,reinterpret_cast<HBRUSH>(COLOR_WINDOWTEXT+1));}}
        if(row.kind==SegmentTrackKind::band){const auto d=track_document(s,row.kind,row.track);const auto events=d.band_events();for(size_t i=0;i<events.size();++i)eventLabel(TimelineEventKind::band,i,std::max(0,events[i].physicalTime),i+1<events.size()?std::max(0,events[i+1].physicalTime):s.doc().length(),band_label(events[i]));}
        if(row.kind==SegmentTrackKind::lyric){const auto events=s.doc().lyrics(row.track);for(size_t i=0;i<events.size();++i)eventLabel(TimelineEventKind::lyric,i,events[i].physical,i+1<events.size()?events[i+1].physical:s.doc().length(),events[i].text);}
        if(row.kind==SegmentTrackKind::chord){const auto events=s.doc().chords(row.track);for(size_t i=0;i<events.size();++i)eventLabel(TimelineEventKind::chord,i,events[i].time,i+1<events.size()?events[i+1].time:s.doc().length(),chord_label(events[i]));}
        if(row.kind==SegmentTrackKind::marker){const auto events=s.doc().markers(row.track);for(size_t i=0;i<events.size();++i){const int x=pixel(v,events[i].time);if(x<labelWidth||x>v.right)continue;auto brush=CreateSolidBrush(events[i].kind==MarkerKind::play?RGB(0,0,255):RGB(0,192,0));RECT r{x,events[i].kind==MarkerKind::play?y+2:y+rowHeight-14,std::min(v.right,x+6),events[i].kind==MarkerKind::play?y+14:y+rowHeight-2};FillRect(dc,&r,brush);DeleteObject(brush);if(selected(TimelineEventKind::marker,i,row.track)){InflateRect(&r,2,2);FrameRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));}}}
        if(row.channel&&fine){const auto pos=axis.position(v.begin);const auto ticks=axis.beat_ticks(axis.clocks({pos.measure,pos.beat,0}),v.end);const auto events=s.doc().mutes(row.track);auto map=*row.channel;size_t event=0;for(const auto& tick:ticks){while(event<events.size()&&(events[event].channel<*row.channel||(events[event].channel==*row.channel&&events[event].time<=tick.clocks))){if(events[event].channel==*row.channel)map=events[event].map;++event;}auto next=std::int64_t(tick.clocks);for(const auto& meter:axis.meters())if(meter.measure<=tick.measure)next=std::min<std::int64_t>(s.doc().length(),std::int64_t(tick.clocks)+3072/meter.denominator);const bool mixed=event<events.size()&&events[event].channel==*row.channel&&events[event].time<next;if(map!=mute_channel_silent&&!mixed)continue;const int left=std::max(labelWidth,pixel(v,tick.clocks)),right=std::min(v.right,pixel(v,static_cast<std::int32_t>(next)));if(right>left)text(dc,mixed?L"…":L"X",{left+3,y+2,right,y+rowHeight-2});}}
        RestoreDC(dc,clip);
    }
    const int x=pixel(v,s.cursor);if(x>=labelWidth&&x<=v.right)line(dc,x,0,body.bottom,1,RGB(200,0,0));SelectObject(dc,oldFont);
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){const auto creation=static_cast<MDICREATESTRUCTW*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);s=reinterpret_cast<Session*>(creation->lParam);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));control(w,L"STATIC",L"No event selected",SS_LEFTNOWORDWRAP,8,260,420,28,SelectionText);control(w,L"BUTTON",L"Zoom Out",WS_TABSTOP,440,260,80,28,ZoomOut);control(w,L"BUTTON",L"Zoom In",WS_TABSTOP,526,260,80,28,ZoomIn);control(w,L"BUTTON",L"Fit",WS_TABSTOP,612,260,60,28,Fit);control(w,L"BUTTON",L"Properties (F11)",WS_TABSTOP,678,260,134,28,Properties);layout(w);refresh(w,*s);return 0;}
    if(message==WM_GETMINMAXINFO){const auto result=DefMDIChildProcW(w,message,wp,lp);auto limits=reinterpret_cast<MINMAXINFO*>(lp);limits->ptMinTrackSize.x=std::max<LONG>(limits->ptMinTrackSize.x,440);limits->ptMinTrackSize.y=std::max<LONG>(limits->ptMinTrackSize.y,220);return result;}
    if(message==WM_SIZE&&s){layout(w);refresh(w,*s);return DefMDIChildProcW(w,message,wp,lp);}
    if(message==WM_MDIACTIVATE&&s&&reinterpret_cast<HWND>(lp)==w){s->publish();return DefMDIChildProcW(w,message,wp,lp);}
    if(message==WM_PAINT&&s){paint(w,*s);return 0;}
    if((message==WM_SETFOCUS||message==WM_KILLFOCUS)&&s){InvalidateRect(w,nullptr,TRUE);return DefMDIChildProcW(w,message,wp,lp);}
    if(message==WM_LBUTTONDOWN&&s){POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};
        if(!click_mute(w,*s,p)){select_point(w,*s,p);if(!begin_curve_drag(w,*s,p))begin_note_drag(w,*s,p);}return 0;}
    if(message==WM_MOUSEMOVE&&s&&s->curveDrag){preview_curve_drag(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))});return 0;}
    if(message==WM_LBUTTONUP&&s&&s->curveDrag){commit_curve_drag(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))});return 0;}
    if((message==WM_CANCELMODE||message==WM_CAPTURECHANGED)&&s&&s->curveDrag){cancel_curve_drag(w,*s);return 0;}
    if(message==WM_MOUSEMOVE&&s&&s->noteDrag){preview_note_drag(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))});return 0;}
    if(message==WM_LBUTTONUP&&s&&s->noteDrag){commit_note_drag(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))});return 0;}
    if((message==WM_CANCELMODE||message==WM_CAPTURECHANGED)&&s&&s->noteDrag){cancel_note_drag(w,*s);return 0;}
    if(message==WM_SETCURSOR&&s&&LOWORD(lp)==HTCLIENT){POINT p{};GetCursorPos(&p);ScreenToClient(w,&p);if(const auto lane=cc_lane_at(w,*s,p)){auto& state=s->sequences[lane->key];if(lane->expanded&&p.y>=lane->top+ccHeight&&p.x>=labelWidth){auto index=cc_curve_at(*s,*lane,viewport(w,*s),p);if(state.drawCurve||s->curveDrag||index){SetCursor(LoadCursorW(nullptr,state.drawCurve?IDC_CROSS:IDC_SIZEALL));return TRUE;}}}auto hit=sequence_note_hit(w,*s,p);if(hit||s->noteDrag){const auto handle=s->noteDrag?s->noteDrag->handle:hit->handle;SetCursor(LoadCursorW(nullptr,handle==SequenceNoteHandle::velocity?IDC_SIZENS:handle==SequenceNoteHandle::move?IDC_SIZEALL:IDC_SIZEWE));return TRUE;}}

    if(message==WM_LBUTTONDBLCLK&&s){if(click_sequence(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))},true,false))return 0;const auto target=hit(w,*s,{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))});select(w,*s,target,target&&target->kind!=TimelineEventKind::band);if(target&&target->kind==TimelineEventKind::band)show_embedded_band_editor(workspace,s->host,make_embedded_band_target(s->host,target->document,target->groups,target->track,target->event),s->changed,s->history,s->mixStarted);return 0;}
    if(message==WM_CONTEXTMENU&&s){
        POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};if(p.x==-1&&p.y==-1){RECT r{};GetWindowRect(w,&r);p={r.left+labelWidth+20,r.top+rulerHeight+20};}else{POINT local=p;ScreenToClient(w,&local);select_point(w,*s,local,true);}
        auto menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Timeline context creation failed");
        if(s->bandCell||(s->selected&&s->selected->kind==TimelineEventKind::band)){
            AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Cut");AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Copy");AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Paste");
            AppendMenuW(menu,MF_STRING|(can_insert_band(*s)?0:MF_GRAYED),InsertBand,L"&Insert\tIns");AppendMenuW(menu,MF_STRING|(s->selected&&s->selected->kind==TimelineEventKind::band?0:MF_GRAYED),DeleteBand,L"Delete\tDel");
            AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Cycle Bands");
        }
        if(const auto row=active_sequence(*s)){const auto& state=s->sequences[*s->activeSequence];
            AppendMenuW(menu,MF_STRING,AddCC,L"Add New CC Track...");
            if(state.ccContext){AppendMenuW(menu,MF_STRING|(state.activeCC?0:MF_GRAYED),DrawCurve,L"Draw Curve\tIns");AppendMenuW(menu,MF_STRING|(state.curve?0:MF_GRAYED),DeleteCurve,L"Delete Curve\tDel");AppendMenuW(menu,MF_STRING|(state.curve?0:MF_GRAYED),CurveProperties,L"Curve Properties...\tF11");}
            else{
            AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Cut");AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Copy");AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"Paste");
            AppendMenuW(menu,MF_STRING|(state.expanded?0:MF_GRAYED),InsertNote,L"Insert Note\tIns");AppendMenuW(menu,MF_STRING|(state.note?0:MF_GRAYED),DeleteNote,L"Delete\tDel");
            AppendMenuW(menu,MF_STRING|(state.expanded?0:MF_GRAYED),Quantize,L"Quantize...\tCtrl+Q");AppendMenuW(menu,MF_STRING|(state.expanded?0:MF_GRAYED),EditVelocity,L"Edit Velocity...\tCtrl+L");
            auto snap=CreatePopupMenu();const wchar_t* labels[]={L"None\tCtrl+E",L"Grid\tCtrl+G",L"Beat\tCtrl+T",L"Barline\tCtrl+B"};for(unsigned i=0;i<4;++i)AppendMenuW(snap,MF_STRING|(static_cast<unsigned>(state.snap)==i?MF_CHECKED:0),SnapNone+i,labels[i]);AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(snap),L"Snap To");
            AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,AddSequencePart,L"Add Sequence Part(s)...");}}
        AppendMenuW(menu,MF_STRING,AddTracks,L"Add Track(s)...\tCtrl+Ins");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING|(s->selected.has_value()?0:MF_GRAYED),Properties,L"&Properties\tF11");
        const auto chosen=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,w,nullptr);DestroyMenu(menu);if(chosen)SendMessageW(w,WM_COMMAND,chosen,0);return 0;
    }
    if(message==WM_COMMAND&&s&&!s->updating&&LOWORD(wp)==AddSequencePart){if(s->noteDrag)cancel_note_drag(w,*s);if(s->curveDrag)cancel_curve_drag(w,*s);show_sequence_part_editor(w,s->doc(),[&](size_t track,std::uint32_t channel){s->doc().select_track_group(s->doc().selected_groups(),s->doc().selected_tempo_index(),s->doc().selected_meter_index(),track,s->doc().selected_band_index());s->activeSequence=Session::SequenceKey{s->doc().selected_groups(),track,channel};s->selected.reset();s->muteCell.reset();s->bandCell.reset();refresh(w,*s);s->publish();});refresh(w,*s);return 0;}
    if(message==WM_COMMAND&&s&&!s->updating){const auto id=LOWORD(wp);if(s->noteDrag)cancel_note_drag(w,*s);if(s->curveDrag)cancel_curve_drag(w,*s);if(id==AddCC){add_cc_lane(w,*s);return 0;}if(id==DrawCurve){arm_curve(w,*s);return 0;}if(id==DeleteCurve){delete_curve(w,*s);return 0;}if(id==CurveProperties){edit_curve(w,*s);return 0;}if(id==EditVelocity){edit_sequence_velocity(w,*s);return 0;}if(id==Quantize){quantize_sequence_notes(w,*s);return 0;}if(id==InsertNote){insert_sequence_note(w,*s);return 0;}if(id==DeleteNote){delete_sequence_note(w,*s);return 0;}if(id>=SnapNone&&id<=SnapBar){if(s->activeSequence){auto& state=s->sequences[*s->activeSequence];state.snap=static_cast<SequenceSnap>(id-SnapNone);refresh(w,*s);s->publish();}return 0;}if(id==InsertBand){insert_band(w,*s);return 0;}if(id==DeleteBand){delete_band(w,*s);return 0;}if(id==AddTracks){const auto tracks=show_add_tracks(w,s->doc().name().empty()?L"Segment":s->doc().name());if(tracks)s->doc().add_tracks(*tracks);refresh(w,*s);s->publish();return 0;}if(id==Properties){if(s->selected.has_value())s->publish(true);return 0;}if(id==ZoomIn||id==ZoomOut||id==Fit){const auto v=viewport(w,*s);const auto anchor=std::clamp<std::int64_t>(s->cursor,v.begin,std::int64_t(v.begin)+v.page-1);const auto offset=anchor-v.begin;s->zoom=id==Fit?1:std::clamp(s->zoom*(id==ZoomIn?2:.5),1.,32.);const auto next=viewport(w,*s);s->scroll=id==Fit?0:static_cast<std::int32_t>(std::max<std::int64_t>(0,anchor-offset*next.page/v.page));refresh(w,*s);return 0;}}
    if(message==WM_HSCROLL&&s){auto v=viewport(w,*s);SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(w,SB_HORZ,&info);auto next=std::int64_t(s->scroll);switch(LOWORD(wp)){case SB_LINELEFT:next-=768;break;case SB_LINERIGHT:next+=768;break;case SB_PAGELEFT:next-=v.page/2;break;case SB_PAGERIGHT:next+=v.page/2;break;case SB_THUMBPOSITION:case SB_THUMBTRACK:next=info.nTrackPos;break;case SB_LEFT:next=0;break;case SB_RIGHT:next=s->doc().length()-v.page;break;default:return 0;}s->scroll=static_cast<std::int32_t>(std::clamp<std::int64_t>(next,0,std::max(0,s->doc().length()-v.page)));refresh(w,*s);return 0;}
    if(message==WM_VSCROLL&&s){SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(w,SB_VERT,&info);int next=s->verticalScroll;const int page=strip_page(w),height=rows_height(view_rows(*s));switch(LOWORD(wp)){case SB_LINEUP:next-=rowHeight;break;case SB_LINEDOWN:next+=rowHeight;break;case SB_PAGEUP:next-=page;break;case SB_PAGEDOWN:next+=page;break;case SB_THUMBPOSITION:case SB_THUMBTRACK:next=info.nTrackPos;break;case SB_TOP:next=0;break;case SB_BOTTOM:next=height-page;break;default:return 0;}s->verticalScroll=std::clamp(next,0,std::max(0,height-page));refresh(w,*s);return 0;}
    if(message==WM_MOUSEWHEEL&&s&&!(LOWORD(wp)&(MK_CONTROL|MK_SHIFT))){s->wheelRemainder+=static_cast<short>(HIWORD(wp));const int steps=s->wheelRemainder/WHEEL_DELTA;s->wheelRemainder%=WHEEL_DELTA;if(steps){s->verticalScroll=std::clamp(s->verticalScroll-steps*rowHeight,0,std::max(0,rows_height(view_rows(*s))-strip_page(w)));refresh(w,*s);}return 0;}
    if(message==WM_NCDESTROY&&s){timelineWindows.erase(std::remove(timelineWindows.begin(),timelineWindows.end(),w),timelineWindows.end());if(workspace)PostMessageW(GetParent(workspace),SegmentWorkspaceChangedMessage,0,0);SetWindowLongPtrW(w,GWLP_USERDATA,0);if(s->owned)delete s;const auto result=DefMDIChildProcW(w,message,wp,lp);if(timelineWindows.empty()&&!embedded_band_editor_count()&&workspace)ShowWindow(workspace,SW_HIDE);return result;}
}catch(const std::exception& e){MessageBoxA(w,e.what(),"Timeline view failed",MB_OK|MB_ICONERROR);return 0;}return DefMDIChildProcW(w,message,wp,lp);}
HWND message_window(HWND target){for(auto w:timelineWindows)if(target==w||IsChild(w,target))return w;return nullptr;}
void show_workspace(){SetWindowPos(workspace,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);}
}
void layout_segment_timeline_workspace(HWND parent,int editorOffset,int toolbarHeight){
    if(!workspace||!IsWindow(workspace))return;RECT r{};GetClientRect(parent,&r);
    const int top=78+toolbarHeight,bottom=std::min(642+toolbarHeight,static_cast<int>(r.bottom)-8);
    MoveWindow(workspace,editorOffset+8,top,std::max(400,static_cast<int>(r.right)-editorOffset-16),std::max(300,bottom-top),TRUE);
    MoveWindow(GetDlgItem(parent,400),editorOffset+582,14+toolbarHeight,72,27,TRUE);
    MoveWindow(GetDlgItem(parent,401),editorOffset+666,14+toolbarHeight,72,27,TRUE);
}
void initialize_segment_timeline_workspace(HWND parent,int editorOffset,int toolbarHeight){
    if(workspace&&IsWindow(workspace))return;workspace=nullptr;timelineWindows.clear();
    auto menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Window menu creation failed");
    AppendMenuW(menu,MF_STRING,Cascade,L"&Cascade");AppendMenuW(menu,MF_STRING,TileHorizontal,L"Tile &Horizontally");AppendMenuW(menu,MF_STRING,TileVertical,L"Tile &Vertically");AppendMenuW(menu,MF_STRING,CloseAll,L"&Close All");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    if(!AppendMenuW(GetMenu(parent),MF_POPUP,reinterpret_cast<UINT_PTR>(menu),L"&Window")){DestroyMenu(menu);throw std::runtime_error("Window menu installation failed");}
    CLIENTCREATESTRUCT creation{menu,FirstChild};workspace=CreateWindowExW(0,L"MDICLIENT",L"Editing Workspace",WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_HSCROLL|WS_VSCROLL|MDIS_ALLCHILDSTYLES,0,0,0,0,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(WorkspaceControl)),GetModuleHandleW(nullptr),&creation);
    if(!workspace)throw std::runtime_error("Editing workspace creation failed");layout_segment_timeline_workspace(parent,editorOffset,toolbarHeight);DrawMenuBar(parent);
}
bool segment_timeline_workspace_has_editors(){return embedded_band_editor_count()||std::any_of(timelineWindows.begin(),timelineWindows.end(),[](HWND child){return IsWindow(child)!=FALSE;});}
void hide_segment_timeline_workspace(){if(workspace&&IsWindow(workspace))ShowWindow(workspace,SW_HIDE);}
std::optional<std::int32_t> segment_timeline_cursor(size_t document){
    for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(s&&s->document==document)return std::clamp(s->cursor,0,std::max(0,s->doc().length()-1));}return {};
}
void update_segment_timeline_play_cursor(size_t document,std::int64_t clocks){
    for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
        if(!s||s->document!=document||s->updating||s->noteDrag||s->curveDrag)continue;
        const auto next=static_cast<std::int32_t>(std::clamp<std::int64_t>(clocks,0,std::max(0,s->doc().length()-1)));
        const bool moved=s->cursor!=next;s->cursor=next;const bool scrolled=follow_play_cursor(w,*s);
        if(moved||scrolled)refresh(w,*s);
    }
}
bool focus_active_segment_timeline(){
    if(!workspace||!IsWindowVisible(workspace))return false;
    const auto child=reinterpret_cast<HWND>(SendMessageW(workspace,WM_MDIGETACTIVE,0,0));
    if(!child||!message_window(child)||!IsWindowVisible(child)||!IsWindowEnabled(child)||IsIconic(child))return false;
    SetFocus(child);return GetFocus()==child;
}
bool activate_segment_timeline_document(size_t document){for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(s&&s->document==document){show_workspace();if(IsIconic(w))ShowWindow(w,SW_RESTORE);SendMessageW(workspace,WM_MDIACTIVATE,reinterpret_cast<WPARAM>(w),0);SetFocus(w);s->publish();return true;}}return false;}
bool segment_timeline_workspace_command(HWND parent,UINT command){
    if(!workspace||!IsWindow(workspace))return false;
    if(command>=FirstChild&&command<FirstChild+timelineWindows.size()+embedded_band_editor_count()){show_workspace();DefFrameProcW(parent,workspace,WM_COMMAND,command,0);return true;}
    if(command==Cascade){SendMessageW(workspace,WM_MDICASCADE,0,0);return true;}
    if(command==TileHorizontal||command==TileVertical){SendMessageW(workspace,WM_MDITILE,command==TileHorizontal?MDITILE_HORIZONTAL:MDITILE_VERTICAL,0);return true;}
    if(command==CloseAll){release_segment_timeline();return true;}return false;
}
LRESULT segment_timeline_frame_proc(HWND parent,UINT message,WPARAM wp,LPARAM lp){return workspace&&IsWindow(workspace)&&IsWindowVisible(workspace)?DefFrameProcW(parent,workspace,message,wp,lp):DefWindowProcW(parent,message,wp,lp);}
void show_segment_timeline(HWND parent,Framework& host,size_t document,TimelineSelectionChanged changed,TimelineHistoryRequested history,TimelineNoteAudition audition,TimelineBandMixStarted mixStarted){
    (void)host.document(document).timeline();if(activate_segment_timeline_document(document))return;if(!workspace||!IsWindow(workspace))throw std::runtime_error("Editing workspace is not initialized");static bool registered=false;const auto instance=GetModuleHandleW(nullptr);if(!registered){WNDCLASSW c{};c.style=CS_DBLCLKS;c.lpfnWndProc=proc;c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hIcon=LoadIconW(nullptr,IDI_APPLICATION);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerSegmentTimeline";if(!RegisterClassW(&c))throw std::runtime_error("Timeline class registration failed");registered=true;}
    auto session=std::make_unique<Session>(Session{host,document,std::move(changed),std::move(history),std::move(audition),std::move(mixStarted)});show_workspace();auto window=CreateMDIWindowW(L"SourceProducerSegmentTimeline",L"Segment Timeline",WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,850,420,workspace,instance,reinterpret_cast<LPARAM>(session.get()));if(!window)throw std::runtime_error("Timeline window creation failed");timelineWindows.push_back(window);PostMessageW(parent,SegmentWorkspaceChangedMessage,0,0);session->owned=true;session.release();SendMessageW(workspace,WM_MDIACTIVATE,reinterpret_cast<WPARAM>(window),0);SetFocus(window);UpdateWindow(window);
}
void refresh_segment_timeline(){refresh_embedded_band_editors();for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(s)refresh(w,*s);}}
void select_segment_timeline_event(TimelineEventKind kind,size_t document,size_t track,size_t event){for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(s&&s->document==document&&s->selected&&s->selected->kind==kind&&s->selected->track==track){s->selected->event=event;if(kind==TimelineEventKind::sequence_note&&s->activeSequence)s->sequences[*s->activeSequence].note=event;refresh(w,*s);}}}
void clear_segment_timeline_note_selection(size_t document,std::uint32_t groups,size_t track){
    for(auto w:timelineWindows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(!s||s->document!=document)continue;
        if(s->selected&&s->selected->kind==TimelineEventKind::sequence_note&&s->selected->groups==groups&&s->selected->track==track)s->selected.reset();
        for(auto& item:s->sequences)if(std::get<0>(item.first)==groups&&std::get<1>(item.first)==track)item.second.note.reset();refresh(w,*s);
    }
}
bool translate_segment_timeline(MSG& message){
    if(workspace&&IsWindow(workspace)&&IsWindowVisible(workspace)&&TranslateMDISysAccel(workspace,&message))return true;
    if(translate_embedded_band_editor(message))return true;
    const auto timelineWindow=message_window(message.hwnd);if(!timelineWindow)return false;
    if(message.message==WM_KEYDOWN&&IsWindowEnabled(timelineWindow)&&IsWindowEnabled(GetAncestor(timelineWindow,GA_ROOT))){
        const bool control=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0,alt=(GetKeyState(VK_MENU)&0x8000)!=0;
        if(control&&!shift&&!alt&&(message.wParam=='E'||message.wParam=='G'||message.wParam=='T'||message.wParam=='B')){
            auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));
            wchar_t className[64]{};GetClassNameW(message.hwnd,className,64);
            if(s&&active_sequence(*s)&&!s->sequences[*s->activeSequence].ccContext&&lstrcmpiW(className,L"Edit")!=0){
                const auto command=message.wParam=='E'?SnapNone:message.wParam=='G'?SnapGrid:message.wParam=='T'?SnapBeat:SnapBar;
                SendMessageW(timelineWindow,WM_COMMAND,command,0);return true;
            }
        }
        if(!control&&!shift&&!alt){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));if(s&&message.wParam==VK_ESCAPE){if(s->curveDrag){cancel_curve_drag(timelineWindow,*s);return true;}if(s->activeSequence&&s->sequences[*s->activeSequence].drawCurve){s->sequences[*s->activeSequence].drawCurve=false;refresh(timelineWindow,*s);return true;}}if(s&&note_drag_key(timelineWindow,*s,message.wParam))return true;}
        if(control&&!shift&&!alt&&message.wParam=='L'){SendMessageW(timelineWindow,WM_COMMAND,EditVelocity,0);return true;}
        if(control&&!shift&&!alt&&message.wParam=='Q'){SendMessageW(timelineWindow,WM_COMMAND,Quantize,0);return true;}
        if(control&&!shift&&!alt&&(message.wParam=='Z'||message.wParam=='Y')){
            wchar_t className[64]{};GetClassNameW(message.hwnd,className,64);
            // Any future text fields keep their local edit buffer. Timeline
            // history belongs to the Session document, even after Main switches.
            if(lstrcmpiW(className,L"Edit")==0||(lstrcmpiW(className,L"ComboBox")==0&&(GetWindowLongPtrW(message.hwnd,GWL_STYLE)&3)!=CBS_DROPDOWNLIST))return false;
            auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));
            if(s){cancel_note_drag(timelineWindow,*s);cancel_curve_drag(timelineWindow,*s);s->publish();if(s->history)s->history(message.wParam=='Y');refresh(timelineWindow,*s);s->publish();}return true;
        }
        if(!control&&!shift&&!alt&&(message.wParam==VK_LEFT||message.wParam==VK_RIGHT||message.wParam==VK_UP||message.wParam==VK_DOWN)){
            auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));if(s&&active_sequence(*s)){auto& state=s->sequences[*s->activeSequence];if(state.expanded&&!state.ccContext){SequenceCursor cursor{s->cursor,state.pitch,state.snap};
                if(move_sequence_cursor(s->doc().timeline(),s->doc().length(),cursor,message.wParam==VK_RIGHT?1:message.wParam==VK_LEFT?-1:0,message.wParam==VK_UP?1:message.wParam==VK_DOWN?-1:0)){s->cursor=cursor.clock;state.pitch=cursor.pitch;follow_sequence_pitch(state);refresh(timelineWindow,*s);s->publish();}return true;}}
        }
        if(!shift&&!alt&&message.wParam==VK_INSERT){auto state=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));SendMessageW(timelineWindow,WM_COMMAND,control?AddTracks:state&&active_sequence(*state)?(state->sequences[*state->activeSequence].ccContext?DrawCurve:InsertNote):InsertBand,0);return true;}
        if(!control&&!shift&&!alt&&message.wParam==VK_DELETE){auto state=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));SendMessageW(timelineWindow,WM_COMMAND,state&&active_sequence(*state)?(state->sequences[*state->activeSequence].ccContext?DeleteCurve:DeleteNote):DeleteBand,0);return true;}
        if(!control&&!shift&&!alt&&message.wParam==VK_F11){auto state=reinterpret_cast<Session*>(GetWindowLongPtrW(timelineWindow,GWLP_USERDATA));SendMessageW(timelineWindow,WM_COMMAND,state&&state->activeSequence&&state->sequences[*state->activeSequence].ccContext?CurveProperties:Properties,0);return true;}
        if(!control&&shift&&!alt&&message.wParam==VK_F10){SendMessageW(timelineWindow,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(timelineWindow),MAKELPARAM(-1,-1));return true;}
    }
    return IsDialogMessageW(timelineWindow,&message);
}
void release_segment_timeline(){release_embedded_band_editors();const auto windows=timelineWindows;for(auto w:windows)if(IsWindow(w))SendMessageW(workspace,WM_MDIDESTROY,reinterpret_cast<WPARAM>(w),0);hide_segment_timeline_workspace();}
}
