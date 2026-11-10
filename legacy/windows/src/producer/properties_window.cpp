#include "properties_window.h"
#include "new_files.h"
#include "wave_property_values.h"
#include "sequence_note_properties.h"
#include <commctrl.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cwchar>

namespace producer::app {
namespace {
constexpr UINT Name=3300,ChordType=3440,SegmentLengthButton=3490;
constexpr UINT ChordRoot=3441,ChordSpin=3442,ChordSharps=3443,ChordFlats=3444,ChordKeys=3450,ScaleKeys=3460;
constexpr UINT TempoValue=3310,TempoSpin=3320;
constexpr UINT LyricValue=3340,LyricSpin=3350,LyricTiming=3360;
constexpr UINT MarkerValue=3370,MarkerSpin=3380;
constexpr UINT BandValue=3470,BandSpin=3480;
constexpr UINT SequenceValue=3500,SequenceSpin=3520;
constexpr UINT LoopValue=3550,LoopSpin=3570,LoopInfinite=3590,LoopReset=3591,LoopClock=3592,LoopClockStart=3593,LoopClockEnd=3594;
constexpr UINT SegmentStartValue=3600,SegmentStartSpin=3610,SegmentTimeSigGroup=3620,SegmentTimeSigGroupSpin=3621,SegmentExtendTimeSig=3622,SegmentEmbeddedAudioPath=3623;
constexpr UINT SegmentInfoValue=3630,SegmentInfoGuid=3638;
constexpr UINT BoundaryMarker=3650,BoundaryList=3651,BoundaryQuick=3652,BoundaryPrepare=3653;
constexpr UINT BoundaryInvalidation=3654;
constexpr UINT BoundaryAlignment=3657;
constexpr UINT WaveValue=3390,WaveCheck=3400,WaveSpin=3410;
constexpr UINT MeterButton=3420,MeterMeasure=3421,MeterSpin=3422;
constexpr UINT MeterBeats=3430,MeterBeat=3431,MeterGrid=3432;
constexpr unsigned lyricDelivery[]={4,8,16};
std::wstring text(HWND window){
    const int count=GetWindowTextLengthW(window);std::wstring value(static_cast<size_t>(count)+1,L'\0');
    GetWindowTextW(window,value.data(),count+1);value.resize(count);return value;
}
HWND child(HWND parent,const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int w,int h,UINT id=0){
    const auto result=CreateWindowW(type,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);
    if(!result)throw std::runtime_error("Properties control creation failed");
    SendMessageW(result,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return result;
}
struct MeterDialog {meter::Event value;};
INT_PTR CALLBACK meter_dialog_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    auto state=reinterpret_cast<MeterDialog*>(GetWindowLongPtrW(window,DWLP_USER));
    try{
        if(message==WM_INITDIALOG){
            state=reinterpret_cast<MeterDialog*>(lp);SetWindowLongPtrW(window,DWLP_USER,lp);
            RECT size{0,0,326,176};AdjustWindowRectEx(&size,WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME,FALSE,WS_EX_DLGMODALFRAME);
            RECT owner{};GetWindowRect(GetParent(window),&owner);SetWindowPos(window,nullptr,owner.left+(owner.right-owner.left-size.right+size.left)/2,owner.top+24,size.right-size.left,size.bottom-size.top,SWP_NOZORDER);
            child(window,L"BUTTON",L"Time Signature",BS_GROUPBOX,12,8,206,78);
            child(window,L"STATIC",L"Beats Per Measure:",0,40,30,100,18);
            const auto beats=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_NUMBER,142,28,48,20,MeterBeats);SendMessageW(beats,EM_SETLIMITTEXT,3,0);
            const auto spin=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS|UDS_SETBUDDYINT,0,0,16,20);
            SendMessageW(spin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(beats),0);SendMessageW(spin,UDM_SETRANGE32,1,100);
            child(window,L"STATIC",L"Beat:",0,40,56,100,18);
            const auto beat=child(window,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,142,53,48,160,MeterBeat);
            int selected=0;for(unsigned i=0,v=1;i<6;++i,v*=2){const auto value=std::to_wstring(v);SendMessageW(beat,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value.c_str()));SendMessageW(beat,CB_SETITEMDATA,i,v);if(v==state->value.denominator)selected=static_cast<int>(i);}SendMessageW(beat,CB_SETCURSEL,selected,0);
            child(window,L"BUTTON",L"Grid",BS_GROUPBOX,12,112,206,52);child(window,L"STATIC",L"Divide Beat:",0,40,136,98,18);
            const auto grid=child(window,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWN|WS_VSCROLL,142,132,48,180,MeterGrid);
            for(unsigned i=1;i<=32;++i){const auto value=std::to_wstring(i);SendMessageW(grid,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value.c_str()));}
            SetWindowTextW(beats,std::to_wstring(state->value.beats).c_str());SetWindowTextW(grid,std::to_wstring(state->value.grids).c_str());
            child(window,L"BUTTON",L"OK",WS_TABSTOP|BS_DEFPUSHBUTTON,254,10,60,24,IDOK);child(window,L"BUTTON",L"Cancel",WS_TABSTOP,254,40,60,24,IDCANCEL);
            SetFocus(beats);SendMessageW(beats,EM_SETSEL,0,-1);return FALSE;
        }
        if(message==WM_CLOSE){EndDialog(window,IDCANCEL);return TRUE;}
        if(message==WM_COMMAND&&state){
            if(LOWORD(wp)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}
            if(LOWORD(wp)==IDOK){
                const auto b=text(GetDlgItem(window,MeterBeats)),g=text(GetDlgItem(window,MeterGrid));
                if(b.empty()||g.empty()||b.find_first_not_of(L"0123456789")!=std::wstring::npos||g.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Invalid Time Signature");
                const auto beats=std::stoul(b),grids=std::stoul(g);const auto beat=GetDlgItem(window,MeterBeat);const auto selection=SendMessageW(beat,CB_GETCURSEL,0,0);if(selection==CB_ERR||beats<1||beats>100||grids<1||grids>65535)throw std::runtime_error("Invalid Time Signature");
                state->value.beats=static_cast<BYTE>(beats);state->value.denominator=static_cast<BYTE>(SendMessageW(beat,CB_GETITEMDATA,selection,0));state->value.grids=static_cast<WORD>(grids);EndDialog(window,IDOK);return TRUE;
            }
        }
    }catch(const std::exception&){if(message==WM_INITDIALOG){EndDialog(window,-1);return TRUE;}MessageBeep(MB_ICONWARNING);return TRUE;}
    return FALSE;
}
bool meter_dialog(HWND owner,meter::Event& value){
    struct Template {DLGTEMPLATE dialog;WORD menu=0,windowClass=0;wchar_t title[34]=L"Set Time Signature and Grid";} t{};
    t.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;t.dialog.dwExtendedStyle=WS_EX_DLGMODALFRAME;t.dialog.cx=214;t.dialog.cy=122;
    MeterDialog state{value};const auto result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),&t.dialog,owner,meter_dialog_proc,reinterpret_cast<LPARAM>(&state));
    if(result==-1)throw std::runtime_error("Time Signature dialog creation failed");if(result!=IDOK)return false;value=state.value;return true;
}
}
void PropertiesWindow::initialize(HWND owner,Framework& host,std::function<void()> changed){owner_=owner;framework_=&host;changed_=std::move(changed);}
bool PropertiesWindow::supports(const std::optional<OpenedDocument>& target)const{
    if(!framework_||!target)return false;
    return (target->kind==DocumentKind::Segment&&target->index<framework_->documents().size())||
           (target->kind==DocumentKind::Style&&target->index<framework_->style_documents().size())||
           (target->kind==DocumentKind::Band&&target->index<framework_->band_documents().size())||
           (target->kind==DocumentKind::Wave&&target->index<framework_->waves().size());
}
std::wstring PropertiesWindow::target_name()const{
    if(target_->kind==DocumentKind::Band)return framework_->band_document(target_->index).name();
    if(target_->kind==DocumentKind::Wave)return framework_->wave_document(target_->index).name();
    return target_->kind==DocumentKind::Segment?framework_->document(target_->index).name():framework_->style_document(target_->index).name();
}
void PropertiesWindow::clear_page_background(){
    if(!window_||!tabs_)return;
    // WS_CLIPCHILDREN and the Tab's sibling clipping prevent the parent from
    // erasing pixels underneath a newly visible transparent group box. Clear
    // the old page while all overlapping controls are hidden, then lay out and
    // show the selected page. Focus loss during this layout cannot commit edits.
    const bool wasUpdating=updating_;updating_=true;
    for(auto control=GetWindow(window_,GW_CHILD);control;control=GetWindow(control,GW_HWNDNEXT))if(control!=tabs_)ShowWindow(control,SW_HIDE);
    RedrawWindow(window_,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    updating_=wasUpdating;
}
void PropertiesWindow::synchronize(bool selectionChanged){
    if(!window_)return;
    struct Guard {bool& value;bool previous;explicit Guard(bool& flag):value(flag),previous(flag){value=true;}~Guard(){value=previous;}} guard(updating_);
    const bool supported=supports(target_);
    const bool chordPage=chordTarget_.has_value(),bandPage=bandTarget_.has_value(),sequencePage=sequenceTarget_.has_value();
    const bool tempoPage=tempoDocument_.has_value(),lyricPage=lyricTarget_.has_value(),markerPage=markerTarget_.has_value(),meterPage=meterTarget_.has_value();
    const bool tempoSupported=tempoPage&&supports_tempo(*tempoDocument_);
    const bool wavePage=supported&&target_->kind==DocumentKind::Wave;
    const bool segmentDocument=supported&&target_->kind==DocumentKind::Segment;
    const int tabFamily=wavePage?1:segmentDocument?2:0,tabCount=segmentDocument?4:wavePage?2:1;
    if(TabCtrl_GetItemCount(tabs_)!=tabCount||tabFamily_!=tabFamily){clear_page_background();TabCtrl_DeleteAllItems(tabs_);for(int i=0;i<tabCount;++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(segmentDocument?(i==3?L"Boundary":i==2?L"Info":i?L"Loop":L"Segment"):(i?L"Info":L"Wave"));TabCtrl_InsertItem(tabs_,i,&item);}TabCtrl_SetCurSel(tabs_,0);tabFamily_=tabFamily;}
    const bool loopPage=segmentDocument&&TabCtrl_GetCurSel(tabs_)==1;
    const bool segmentInfo=segmentDocument&&TabCtrl_GetCurSel(tabs_)==2;
    const bool segmentBoundary=segmentDocument&&TabCtrl_GetCurSel(tabs_)==3;
    const bool segmentPage=segmentDocument&&TabCtrl_GetCurSel(tabs_)==0;
    const bool waveInfo=wavePage&&TabCtrl_GetCurSel(tabs_)==1;
    const auto tabStyle=GetWindowLongPtrW(tabs_,GWL_STYLE);SetWindowLongPtrW(tabs_,GWL_STYLE,(tabStyle&~WS_TABSTOP)|(tabFamily?WS_TABSTOP:0));
    RECT client{};GetClientRect(window_,&client);const int width=wavePage||chordPage?396:segmentDocument||sequencePage?389:380;
    const int height=sequencePage?200:segmentDocument?212:210;
    if(client.right!=width||client.bottom!=height){RECT rect{0,0,width,height};AdjustWindowRectEx(&rect,WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_BORDER,FALSE,0);SetWindowPos(window_,nullptr,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);}
    MoveWindow(tabs_,8,6,width-16,height-14,TRUE);
    if(!supported)target_.reset();
    ShowWindow(tabs_,supported||tempoPage||lyricPage||markerPage||meterPage||chordPage||bandPage||sequencePage?SW_SHOW:SW_HIDE);ShowWindow(label_,supported&&!loopPage&&!segmentInfo&&!segmentBoundary&&(!wavePage||waveInfo)?SW_SHOW:SW_HIDE);ShowWindow(name_,supported&&!loopPage&&!segmentInfo&&!segmentBoundary&&(!wavePage||waveInfo)?SW_SHOW:SW_HIDE);
    for(const auto control:loopControls_)ShowWindow(control,loopPage?SW_SHOW:SW_HIDE);
    for(const auto control:segmentInfoControls_)ShowWindow(control,segmentInfo?SW_SHOW:SW_HIDE);
    for(const auto control:segmentBoundaryControls_)if(control!=segmentBoundaryFallback_||!segmentBoundary)ShowWindow(control,segmentBoundary?SW_SHOW:SW_HIDE);
    for(size_t i=0;i<8;++i){const bool show=wavePage&&(i<5?!waveInfo:waveInfo);ShowWindow(waveFields_[i],show?SW_SHOW:SW_HIDE);ShowWindow(waveLabels_[i],show?SW_SHOW:SW_HIDE);}
    for(auto check:waveChecks_)ShowWindow(check,wavePage&&!waveInfo?SW_SHOW:SW_HIDE);
    for(auto spin:waveSpins_)ShowWindow(spin,wavePage&&!waveInfo?SW_SHOW:SW_HIDE);
    for(size_t i=0;i<4;++i){ShowWindow(tempoFields_[i],tempoPage?SW_SHOW:SW_HIDE);ShowWindow(tempoLabels_[i],tempoPage?SW_SHOW:SW_HIDE);ShowWindow(tempoSpins_[i],tempoPage?SW_SHOW:SW_HIDE);}
    for(size_t i=0;i<6;++i){ShowWindow(lyricFields_[i],lyricPage?SW_SHOW:SW_HIDE);ShowWindow(lyricLabels_[i],lyricPage?SW_SHOW:SW_HIDE);}
    for(auto spin:lyricSpins_)ShowWindow(spin,lyricPage?SW_SHOW:SW_HIDE);
    ShowWindow(lyricTimingGroup_,lyricPage?SW_SHOW:SW_HIDE);for(auto radio:lyricTiming_)ShowWindow(radio,lyricPage?SW_SHOW:SW_HIDE);
    for(size_t i=0;i<5;++i){ShowWindow(markerFields_[i],markerPage?SW_SHOW:SW_HIDE);ShowWindow(markerLabels_[i],markerPage?SW_SHOW:SW_HIDE);}
    for(auto spin:markerSpins_)ShowWindow(spin,markerPage?SW_SHOW:SW_HIDE);
    ShowWindow(segmentLengthLabel_,segmentPage?SW_SHOW:SW_HIDE);ShowWindow(segmentLengthButton_,segmentPage?SW_SHOW:SW_HIDE);
    for(const auto control:segmentStartControls_)ShowWindow(control,segmentPage?SW_SHOW:SW_HIDE);
    if(segmentPage){
        SetWindowTextW(segmentLengthLabel_,L"Length");MoveWindow(segmentLengthLabel_,256,36,40,20,TRUE);MoveWindow(segmentLengthButton_,296,34,78,18,TRUE);
        try{const auto value=framework_->document(target_->index).length_settings();const auto caption=value.clockTime?std::to_wstring(value.milliseconds/1000)+L" s...":std::to_wstring(value.measures)+L" Bars";SetWindowTextW(segmentLengthButton_,caption.c_str());EnableWindow(segmentLengthButton_,TRUE);}catch(const std::exception&){SetWindowTextW(segmentLengthButton_,L"Unavailable");EnableWindow(segmentLengthButton_,FALSE);}
        try{
            const auto& d=framework_->document(target_->index);const auto start=d.play_start();if(start<0||start>=d.length())throw std::runtime_error("Segment start unavailable");
            auto context=d;context.select_track_group(1);const auto point=context.timeline().grid_position(start);const auto loop=d.loop_settings();const bool editable=loop.music_time();
            const std::int32_t values[]={point.measure+1,point.beat+1,point.grid+1,point.tick};
            for(unsigned i=0;i<4;++i){const auto value=std::to_wstring(values[i]);if((selectionChanged||finalizing_||GetFocus()!=segmentStartFields_[i])&&text(segmentStartFields_[i])!=value)SetWindowTextW(segmentStartFields_[i],value.c_str());EnableWindow(segmentStartFields_[i],editable);EnableWindow(segmentStartSpins_[i],editable);}
        }catch(const std::exception&){for(unsigned i=0;i<4;++i){SetWindowTextW(segmentStartFields_[i],L"");EnableWindow(segmentStartFields_[i],FALSE);EnableWindow(segmentStartSpins_[i],FALSE);}}
    }
    for(auto control:{meterButton_,meterMeasure_,meterLabel_,meterSpin_})ShowWindow(control,meterPage?SW_SHOW:SW_HIDE);
    ShowWindow(chordType_,chordPage?SW_SHOW:SW_HIDE);ShowWindow(chordLabel_,SW_HIDE);
    for(auto control:{chordRoot_,chordSpin_,chordNotation_[0],chordNotation_[1]})ShowWindow(control,chordPage?SW_SHOW:SW_HIDE);
    for(size_t i=0;i<4;++i){chordKeys_[i].show(chordPage);scaleKeys_[i].show(chordPage);ShowWindow(chordLevels_[i],chordPage?SW_SHOW:SW_HIDE);}
    for(size_t i=0;i<6;++i){ShowWindow(bandFields_[i],bandPage?SW_SHOW:SW_HIDE);ShowWindow(bandLabels_[i],bandPage?SW_SHOW:SW_HIDE);}
    for(auto spin:bandSpins_)ShowWindow(spin,bandPage?SW_SHOW:SW_HIDE);
    for(auto edit:sequenceFields_)ShowWindow(edit,sequencePage?SW_SHOW:SW_HIDE);
    for(auto label:sequenceLabels_)ShowWindow(label,sequencePage?SW_SHOW:SW_HIDE);
    for(auto spin:sequenceSpins_)ShowWindow(spin,sequencePage?SW_SHOW:SW_HIDE);
    if(segmentBoundary){
        const auto& d=framework_->document(target_->index);SetWindowTextW(window_,(d.name()+L" Segment Properties").c_str());SetWindowTextW(segmentBoundaryName_,d.name().c_str());
        const auto raw=d.default_resolution();const auto boundary=d.performance_boundary();
        SendMessageW(segmentBoundaryList_,CB_SETCURSEL,boundary?static_cast<WPARAM>(*boundary):static_cast<WPARAM>(-1),0);EnableWindow(segmentBoundaryList_,boundary.has_value());
        const bool markerAvailable=(raw&0x10100u)==0;EnableWindow(segmentBoundaryMarker_,markerAvailable);
        SendMessageW(segmentBoundaryMarker_,BM_SETCHECK,markerAvailable&&(raw&0x1000000)?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(segmentBoundaryDefault_,BM_SETCHECK,(raw&0x4000)?BST_CHECKED:BST_UNCHECKED,0);
        const auto alignment=d.alignment();EnableWindow(segmentAlignmentDescription_,alignment.has_value());
        for(unsigned i=0;i<4;++i){EnableWindow(segmentAlignment_[i],alignment.has_value());SendMessageW(segmentAlignment_[i],BM_SETCHECK,alignment&&static_cast<unsigned>(*alignment)==i?BST_CHECKED:BST_UNCHECKED,0);}
        const bool showFallback=markerAvailable&&(raw&0x1000000)!=0;const bool fallbackVisibilityChanged=(IsWindowVisible(segmentBoundaryFallback_)!=FALSE)!=showFallback;
        if(fallbackVisibilityChanged)ShowWindow(segmentBoundaryFallback_,showFallback?SW_SHOW:SW_HIDE);
        for(unsigned i=0;i<2;++i)SendMessageW(segmentBoundaryTiming_[i],BM_SETCHECK,((raw&0x400)!=0)==(i==1)?BST_CHECKED:BST_UNCHECKED,0);
        const auto invalidation=d.invalidation();
        for(unsigned i=0;i<3;++i){SendMessageW(segmentInvalidation_[i],BM_SETCHECK,invalidation&&static_cast<unsigned>(*invalidation)==i?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(segmentInvalidation_[i],invalidation.has_value());}
        // The tab and group boxes overlap the label; redraw their background
        // when it is hidden instead of leaving the old static text behind.
        if(fallbackVisibilityChanged)RedrawWindow(window_,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
        return;
    }
    if(segmentInfo){
        const auto& d=framework_->document(target_->index);SetWindowTextW(window_,(d.name()+L" Segment Properties").c_str());
        const SegmentInfoField kinds[]={SegmentInfoField::subject,SegmentInfoField::author,SegmentInfoField::copyright,SegmentInfoField::information};
        for(unsigned i=0;i<4;++i){try{const auto value=d.info_text(kinds[i]);if(selectionChanged||finalizing_||GetFocus()!=segmentInfoFields_[i])if(text(segmentInfoFields_[i])!=value)SetWindowTextW(segmentInfoFields_[i],value.c_str());EnableWindow(segmentInfoFields_[i],TRUE);}catch(const std::exception&){SetWindowTextW(segmentInfoFields_[i],L"");EnableWindow(segmentInfoFields_[i],FALSE);}}
        try{const auto version=d.info_version();for(unsigned i=0;i<4;++i){const auto value=std::to_wstring(version[i]);if(selectionChanged||finalizing_||GetFocus()!=segmentInfoFields_[i+4])if(text(segmentInfoFields_[i+4])!=value)SetWindowTextW(segmentInfoFields_[i+4],value.c_str());EnableWindow(segmentInfoFields_[i+4],TRUE);}}
        catch(const std::exception&){for(unsigned i=4;i<8;++i){SetWindowTextW(segmentInfoFields_[i],L"");EnableWindow(segmentInfoFields_[i],FALSE);}}return;
    }
    if(loopPage){
        const auto& d=framework_->document(target_->index);const auto loop=d.loop_settings();const bool editable=loop.music_time(),infinite=loop.repeats==UINT32_MAX;
        SetWindowTextW(window_,(d.name()+L" Segment Properties").c_str());CheckDlgButton(window_,LoopInfinite,infinite?BST_CHECKED:BST_UNCHECKED);CheckDlgButton(window_,LoopClock,(loop.flags&2u)?BST_CHECKED:BST_UNCHECKED);
        if(!infinite)finiteLoopRepeats_[target_->index]=loop.repeats;
        auto context=d;context.select_track_group(1);const auto axis=context.timeline();
        std::wstring values[9];values[0]=std::to_wstring(finiteLoopRepeats_[target_->index]);
        const GridPosition points[]={axis.grid_position(std::max(0,loop.start)),axis.grid_position(loop.end?std::max(0,loop.end):d.length())};
        for(unsigned row=0;row<2;++row){const auto p=points[row];values[1+row*4]=std::to_wstring(p.measure+1);values[2+row*4]=std::to_wstring(p.beat+1);values[3+row*4]=std::to_wstring(p.grid+1);values[4+row*4]=std::to_wstring(p.tick);}
        for(unsigned i=0;i<9;++i){if((selectionChanged||finalizing_||GetFocus()!=loopFields_[i])&&text(loopFields_[i])!=values[i])SetWindowTextW(loopFields_[i],values[i].c_str());EnableWindow(loopFields_[i],editable&&(i||!infinite));EnableWindow(loopSpins_[i],editable&&(i||!infinite));}
        EnableWindow(loopInfinite_,editable);EnableWindow(loopReset_,editable);EnableWindow(loopClock_,FALSE);
        const auto header=Chunk::parse(d.save_bytes());const auto& h=header.find("segh")->data;
        for(unsigned i=0;i<2;++i){const size_t at=40+i*8;const auto reference=h.size()>=at+8?(std::uint64_t(read32(h,at))|(std::uint64_t(read32(h,at+4))<<32)):0;SetWindowTextW(loopClockFields_[i],std::to_wstring(reference/10000).c_str());EnableWindow(loopClockFields_[i],FALSE);}return;
    }
    if(sequencePage){
        SetWindowTextW(window_,L"Sequence Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Sequence");TabCtrl_SetItem(tabs_,0,&item);
        const bool valid=supports_sequence(*sequenceTarget_);for(auto edit:sequenceFields_)EnableWindow(edit,valid);for(auto spin:sequenceSpins_)EnableWindow(spin,valid);
        if(!valid){for(auto edit:sequenceFields_)SetWindowTextW(edit,L"");return;}
        const auto& d=framework_->document(sequenceTarget_->document);const auto position=d.note_positions().at(sequenceTarget_->event);const auto n=position.note;const auto axis=d.timeline();
        const wchar_t* names[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
        std::wstring values[17];values[0]=names[n.pitch%12];values[1]=std::to_wstring(n.pitch/12);values[2]=std::to_wstring(n.velocity);
        const GridPosition positions[]={sequence_property_position(axis,n.time),sequence_property_position(axis,n.time+n.duration),sequence_property_duration(axis,n)};
        for(unsigned row=0;row<3;++row){const auto p=positions[row];const int base=row==2?0:1;values[3+row*4]=std::to_wstring(p.measure+base);values[4+row*4]=std::to_wstring(p.beat+base);values[5+row*4]=std::to_wstring(p.grid+base);values[6+row*4]=std::to_wstring(p.tick);}
        const auto belongs=axis.position(position.logicalTime);values[15]=std::to_wstring(belongs.measure+1);values[16]=std::to_wstring(belongs.beat+1);
        for(unsigned i=0;i<17;++i)if((selectionChanged||finalizing_||GetFocus()!=sequenceFields_[i])&&text(sequenceFields_[i])!=values[i])SetWindowTextW(sequenceFields_[i],values[i].c_str());return;
    }
    if(bandPage){
        SetWindowTextW(window_,L"Band Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Band");TabCtrl_SetItem(tabs_,0,&item);
        const auto value=read_embedded_band(*framework_,*bandTarget_);for(auto edit:bandFields_)EnableWindow(edit,value.has_value());for(auto spin:bandSpins_)EnableWindow(spin,value.has_value());
        if(!value){SetWindowTextW(bandFields_[0],L"None");for(size_t i=1;i<6;++i)SetWindowTextW(bandFields_[i],L"");return;}
        bandTarget_=value->target;const auto start=band_start_position(value->timeline,value->event.physicalTime),belongs=value->timeline.position(value->event.logicalTime);
        const std::wstring values[]={value->band.name(),std::to_wstring(start.measure+1),std::to_wstring(start.beat+1),std::to_wstring(start.tick),std::to_wstring(belongs.measure+1),std::to_wstring(belongs.beat+1)};
        for(size_t i=0;i<6;++i)if((selectionChanged||finalizing_||GetFocus()!=bandFields_[i])&&text(bandFields_[i])!=values[i])SetWindowTextW(bandFields_[i],values[i].c_str());return;
    }
    if(chordPage){
        SetWindowTextW(window_,L"Chord Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Chord/Scale");TabCtrl_SetItem(tabs_,0,&item);
        const bool valid=supports_chord(*chordTarget_);EnableWindow(chordType_,valid);
        const auto value=valid?framework_->document(chordTarget_->document).chords(chordTarget_->track).at(chordTarget_->event).name:L"";
        if((selectionChanged||finalizing_||GetFocus()!=chordType_)&&text(chordType_)!=value)SetWindowTextW(chordType_,value.c_str());
        const auto event=valid?framework_->document(chordTarget_->document).chords(chordTarget_->track).at(chordTarget_->event):ChordEvent{};
        const auto base=valid?chord_level(event,0):std::nullopt;const bool rootValid=base&&event.subchords[*base].chordRoot<=23;
        const wchar_t* sharp[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
        const wchar_t* flat[]={L"C",L"Db",L"D",L"Eb",L"E",L"F",L"Gb",L"G",L"Ab",L"A",L"Bb",L"B"};
        const auto root=rootValid?event.subchords[*base].chordRoot:0;const auto rootText=rootValid?std::to_wstring(root/12+1)+(chordFlats_?flat[root%12]:sharp[root%12]):L"";
        SetWindowTextW(chordRoot_,rootText.c_str());EnableWindow(chordRoot_,rootValid);EnableWindow(chordSpin_,rootValid);
        SendMessageW(chordNotation_[0],BM_SETCHECK,chordFlats_?BST_UNCHECKED:BST_CHECKED,0);SendMessageW(chordNotation_[1],BM_SETCHECK,chordFlats_?BST_CHECKED:BST_UNCHECKED,0);
        for(unsigned level=0;level<4;++level){
            const auto index=valid?chord_level(event,level):std::nullopt;const auto sub=index?event.subchords[*index]:SubChord{};
            const bool chordValid=index&&sub.chordRoot<=23&&!(sub.chordPattern&0xff000000u),scaleValid=index&&sub.scaleRoot<=23&&!(sub.scalePattern&0xff000000u);
            const auto tones=chordValid?std::uint64_t(sub.chordPattern)<<sub.chordRoot:0;
            std::uint64_t scale=0,chord=0;for(unsigned note=0;note<48;++note){if(tones&(std::uint64_t(1)<<note))chord|=std::uint64_t(1)<<(note%12);if(scaleValid&&note<24&&(sub.scalePattern&(1u<<note)))scale|=std::uint64_t(1)<<((note+sub.scaleRoot)%12);}
            chordKeys_[level].set(tones,0,chordValid);scaleKeys_[level].set(scale,chord&~scale,scaleValid);
        }return;
    }
    if(meterPage){
        SetWindowTextW(window_,L"Time Signature Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Time Signature");TabCtrl_SetItem(tabs_,0,&item);
        const bool valid=supports_meter(*meterTarget_);const bool editable=valid&&framework_->document(meterTarget_->document).can_edit_meter();
        EnableWindow(meterMeasure_,editable);EnableWindow(meterSpin_,editable);EnableWindow(meterButton_,editable);
        if(!valid){SetWindowTextW(meterButton_,L"Time Signature...");SetWindowTextW(meterMeasure_,L"");return;}
        const auto e=framework_->document(meterTarget_->document).timeline().meters().at(meterTarget_->event);
        const auto caption=std::to_wstring(e.beats)+L" / "+std::to_wstring(e.denominator)+L"...";SetWindowTextW(meterButton_,caption.c_str());
        EnableWindow(meterButton_,editable&&e.beats<=100&&e.denominator<=32);
        const auto measure=std::to_wstring(e.measure+1);if((selectionChanged||finalizing_||GetFocus()!=meterMeasure_)&&text(meterMeasure_)!=measure)SetWindowTextW(meterMeasure_,measure.c_str());return;
    }
    if(markerPage){
        SetWindowTextW(window_,L"Marker/Enter Switch Point Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Marker");TabCtrl_SetItem(tabs_,0,&item);
        const bool valid=supports_marker(*markerTarget_);for(size_t i=1;i<5;++i)EnableWindow(markerFields_[i],valid);for(auto spin:markerSpins_)EnableWindow(spin,valid);
        if(!valid){SetWindowTextW(markerFields_[0],L"None");for(size_t i=1;i<5;++i)SetWindowTextW(markerFields_[i],L"");return;}
        const auto& d=framework_->document(markerTarget_->document);const auto e=d.markers(markerTarget_->track).at(markerTarget_->event);
        SetWindowTextW(markerFields_[0],e.kind==MarkerKind::play?L"Marker":L"Enter Switch Point");
        try{const auto p=d.timeline().grid_position(e.time);const std::wstring values[]={std::to_wstring(p.measure+1),std::to_wstring(p.beat+1),std::to_wstring(p.grid+1),std::to_wstring(p.tick)};
            for(size_t i=0;i<4;++i)if((selectionChanged||finalizing_||GetFocus()!=markerFields_[i+1])&&text(markerFields_[i+1])!=values[i])SetWindowTextW(markerFields_[i+1],values[i].c_str());
        }catch(const std::exception&){for(size_t i=1;i<5;++i){EnableWindow(markerFields_[i],FALSE);SetWindowTextW(markerFields_[i],L"");}for(auto spin:markerSpins_)EnableWindow(spin,FALSE);}
        return;
    }
    if(lyricPage){
        SetWindowTextW(window_,L"Lyric Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Lyric");TabCtrl_SetItem(tabs_,0,&item);
        const bool valid=supports_lyric(*lyricTarget_);for(auto edit:lyricFields_)EnableWindow(edit,valid);for(auto spin:lyricSpins_)EnableWindow(spin,valid);for(auto radio:lyricTiming_)EnableWindow(radio,valid);
        if(!valid){SetWindowTextW(lyricFields_[0],L"None");for(size_t i=1;i<6;++i)SetWindowTextW(lyricFields_[i],L"");for(auto radio:lyricTiming_)SendMessageW(radio,BM_SETCHECK,BST_UNCHECKED,0);return;}
        const auto& d=framework_->document(lyricTarget_->document);const auto e=d.lyrics(lyricTarget_->track).at(lyricTarget_->event);const auto axis=d.timeline();const auto start=axis.position(e.physical),belongs=axis.position(e.logical);
        const std::wstring values[]={e.text,std::to_wstring(start.measure+1),std::to_wstring(start.beat+1),std::to_wstring(start.tick),std::to_wstring(belongs.measure+1),std::to_wstring(belongs.beat+1)};
        for(size_t i=0;i<6;++i)if((selectionChanged||finalizing_||GetFocus()!=lyricFields_[i])&&text(lyricFields_[i])!=values[i])SetWindowTextW(lyricFields_[i],values[i].c_str());
        for(size_t i=0;i<3;++i)SendMessageW(lyricTiming_[i],BM_SETCHECK,e.timing==lyricDelivery[i]?BST_CHECKED:BST_UNCHECKED,0);return;
    }
    if(tempoPage){
        SetWindowTextW(window_,L"Tempo Properties");TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Tempo");TabCtrl_SetItem(tabs_,0,&item);
        const auto* d=tempoSupported?&framework_->document(*tempoDocument_):nullptr;
        const auto count=d?std::count_if(d->tempos().begin(),d->tempos().end(),[](const auto& e){return e.selected&&e.bpm>0;}):0;
        const bool single=count==1;
        for(size_t i=0;i<4;++i){EnableWindow(tempoFields_[i],single);EnableWindow(tempoSpins_[i],single);}
        if(!single){SetWindowTextW(tempoFields_[0],count>1?L"Multiple Tempos Selected":L"None");for(size_t i=1;i<4;++i)SetWindowTextW(tempoFields_[i],L"");return;}
        const auto selected=std::find_if(d->tempos().begin(),d->tempos().end(),[](const auto& e){return e.selected&&e.bpm>0;});
        const auto position=d->timeline().position(selected->time);
        wchar_t bpm[32]{};swprintf_s(bpm,L"%.2f",selected->bpm);
        const std::wstring values[]={bpm,std::to_wstring(position.measure+1),std::to_wstring(position.beat+1),std::to_wstring(position.tick)};
        for(size_t i=0;i<4;++i)if((finalizing_||GetFocus()!=tempoFields_[i])&&text(tempoFields_[i])!=values[i])SetWindowTextW(tempoFields_[i],values[i].c_str());
        return;
    }
    if(!supported){SetWindowTextW(window_,L"Properties");return;}
    if(wavePage){
        const auto& d=framework_->wave_document(target_->index);const auto f=d.format();const auto loops=d.sample_loops();const bool looped=!loops.empty();
        const auto remembered=disabledWaveLoops_.find(target_->index);
        const auto loop=looped?loops.front():remembered!=disabledWaveLoops_.end()&&!remembered->second.empty()?remembered->second.front():WaveSampleLoop{0,0,static_cast<std::uint32_t>(std::min<std::uint64_t>(f.frames,UINT32_MAX))};
        if(looped)disabledWaveLoops_[target_->index]=loops;
        const auto title=L"Wave: "+d.name()+L" Properties";SetWindowTextW(window_,title.c_str());
        SetWindowTextW(label_,L"Subject:");MoveWindow(label_,24,38,50,20,TRUE);MoveWindow(name_,75,36,115,20,TRUE);
        if(selectionChanged||finalizing_||GetFocus()!=name_)if(text(name_)!=d.name())SetWindowTextW(name_,d.name().c_str());
        wchar_t duration[64]{};swprintf_s(duration,L"%.3f seconds",static_cast<double>(f.frames)/f.sampleRate);
        const std::wstring values[]={wave_pitch_name(d.root_note()),std::to_wstring(f.frames),std::to_wstring(loop.start),std::to_wstring(std::uint64_t(loop.start)+loop.length-(loop.length?1:0)),std::to_wstring(loop.length),std::to_wstring(f.sampleRate)+L" Hz",std::to_wstring(f.bits)+L" bit",duration};
        for(size_t i=0;i<8;++i)if((selectionChanged||finalizing_||GetFocus()!=waveFields_[i])&&text(waveFields_[i])!=values[i])SetWindowTextW(waveFields_[i],values[i].c_str());
        SendMessageW(waveChecks_[0],BM_SETCHECK,looped?BST_CHECKED:BST_UNCHECKED,0);
        SendMessageW(waveChecks_[1],BM_SETCHECK,loop.type==0?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(waveChecks_[2],BM_SETCHECK,loop.type==1?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(waveChecks_[3],BM_SETCHECK,waveLoopLocked_?BST_CHECKED:BST_UNCHECKED,0);
        const bool editable=f.tag==1&&f.frames>0;
        EnableWindow(waveChecks_[0],editable);for(size_t i=1;i<4;++i)EnableWindow(waveChecks_[i],editable&&looped);
        for(size_t i=2;i<5;++i)EnableWindow(waveFields_[i],editable&&looped&&(i!=4||!waveLoopLocked_));
        for(size_t i=1;i<4;++i)EnableWindow(waveSpins_[i],editable&&looped&&(i!=3||!waveLoopLocked_));return;
    }
    const bool band=target_->kind==DocumentKind::Band;
    SetWindowTextW(label_,band?L"Band Name:":segmentPage?L"Name":L"Name:");MoveWindow(label_,20,segmentPage?36:45,band?80:40,segmentPage?20:22,TRUE);
    const auto value=target_name();
    const wchar_t* kind=band?L"Band":target_->kind==DocumentKind::Segment?L"Segment":L"Style";
    const auto title=(value.empty()?L"":value+L" ")+kind+L" Properties";
    SetWindowTextW(window_,title.c_str());
    TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(kind);TabCtrl_SetItem(tabs_,0,&item);
    const bool style=target_->kind==DocumentKind::Style;
    MoveWindow(name_,band?104:style?76:54,segmentPage?34:42,band?240:style?132:194,segmentPage?16:24,TRUE);
    if(text(name_)!=value)SetWindowTextW(name_,value.c_str());
}
void PropertiesWindow::follow(std::optional<OpenedDocument> target){sequenceTarget_.reset();bandTarget_.reset();const bool switched=!target_||!target||target_->kind!=target->kind||target_->index!=target->index;if(!target)disabledWaveLoops_.clear();tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();target_=supports(target)?target:std::nullopt;synchronize(switched);}
bool PropertiesWindow::supports_tempo(size_t index)const{
    if(!framework_||index>=framework_->documents().size())return false;
    const auto& events=framework_->document(index).tempos();return std::any_of(events.begin(),events.end(),[](const auto& e){return e.selected&&e.bpm>0;});
}
void PropertiesWindow::follow_tempo(size_t index){sequenceTarget_.reset();bandTarget_.reset();target_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();tempoDocument_=index;synchronize();}
void PropertiesWindow::show_tempo(size_t index){if(!supports_tempo(index))return;follow_tempo(index);ensure_window();synchronize();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);if(IsWindowEnabled(tempoFields_[0]))SetFocus(tempoFields_[0]);}
void PropertiesWindow::show(std::optional<OpenedDocument> target){sequenceTarget_.reset();bandTarget_.reset();
    if(!supports(target))return;
    tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();target_=target;ensure_window();
    synchronize(true);ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);(void)focus_existing();
}
bool PropertiesWindow::supports_lyric(const LyricPropertiesTarget& target)const{
    if(!framework_||target.document>=framework_->documents().size())return false;
    const auto& d=framework_->document(target.document);if(d.selected_groups()!=target.groups)return false;
    try{return target.event<d.lyrics(target.track).size();}catch(const std::exception&){return false;}
}
void PropertiesWindow::follow_lyric(LyricPropertiesTarget target,std::function<void(size_t)> selected){sequenceTarget_.reset();bandTarget_.reset();
    const bool switched=!lyricTarget_||lyricTarget_->document!=target.document||lyricTarget_->track!=target.track||lyricTarget_->event!=target.event||lyricTarget_->groups!=target.groups;
    target_.reset();tempoDocument_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();lyricTarget_=target;lyricSelectionChanged_=std::move(selected);
    // A selection change replaces even the focused field. Otherwise its old
    // text can be committed to the new event when the field later loses focus.
    synchronize(switched);
}
void PropertiesWindow::show_lyric(LyricPropertiesTarget target,std::function<void(size_t)> selected){if(!supports_lyric(target))return;follow_lyric(target,std::move(selected));ensure_window();synchronize();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(lyricFields_[0]);}
bool PropertiesWindow::supports_marker(const MarkerPropertiesTarget& target)const{
    if(!framework_||target.document>=framework_->documents().size())return false;
    const auto& d=framework_->document(target.document);if(d.selected_groups()!=target.groups)return false;
    try{return target.event<d.markers(target.track).size();}catch(const std::exception&){return false;}
}
void PropertiesWindow::follow_marker(MarkerPropertiesTarget target,std::function<void(size_t)> selected){sequenceTarget_.reset();bandTarget_.reset();
    const bool switched=!markerTarget_||markerTarget_->document!=target.document||markerTarget_->track!=target.track||markerTarget_->event!=target.event||markerTarget_->groups!=target.groups;
    target_.reset();tempoDocument_.reset();lyricTarget_.reset();meterTarget_.reset();chordTarget_.reset();markerTarget_=target;markerSelectionChanged_=std::move(selected);synchronize(switched);
}
void PropertiesWindow::show_marker(MarkerPropertiesTarget target,std::function<void(size_t)> selected){if(!supports_marker(target))return;follow_marker(target,std::move(selected));ensure_window();synchronize();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(markerFields_[1]);}
bool PropertiesWindow::supports_meter(const MeterPropertiesTarget& target)const{
    if(!framework_||target.document>=framework_->documents().size())return false;
    const auto& d=framework_->document(target.document);if(d.selected_groups()!=target.groups||d.selected_meter_index()!=target.track)return false;
    try{return target.event<d.timeline().meters().size();}catch(const std::exception&){return false;}
}
void PropertiesWindow::follow_meter(MeterPropertiesTarget target,std::function<void(size_t)> selected){sequenceTarget_.reset();bandTarget_.reset();
    const bool switched=!meterTarget_||meterTarget_->document!=target.document||meterTarget_->track!=target.track||meterTarget_->event!=target.event||meterTarget_->groups!=target.groups;
    target_.reset();tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();chordTarget_.reset();meterTarget_=target;meterSelectionChanged_=std::move(selected);synchronize(switched);
}
void PropertiesWindow::show_meter(MeterPropertiesTarget target,std::function<void(size_t)> selected){if(!supports_meter(target))return;follow_meter(target,std::move(selected));ensure_window();synchronize();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(IsWindowEnabled(meterMeasure_)?meterMeasure_:window_);}
bool PropertiesWindow::supports_chord(const ChordPropertiesTarget& target)const{
    if(!framework_||target.document>=framework_->documents().size())return false;
    const auto& d=framework_->document(target.document);if(d.selected_groups()!=target.groups)return false;
    try{const auto events=d.chords(target.track);return target.event<events.size()&&valid_chord(events[target.event],&events[target.event]);}catch(const std::exception&){return false;}
}
void PropertiesWindow::follow_chord(ChordPropertiesTarget target,std::function<void(size_t)> selected){sequenceTarget_.reset();bandTarget_.reset();
    const bool switched=!chordTarget_||chordTarget_->document!=target.document||chordTarget_->track!=target.track||chordTarget_->event!=target.event||chordTarget_->groups!=target.groups;
    target_.reset();tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_=target;chordSelectionChanged_=std::move(selected);synchronize(switched);
}
void PropertiesWindow::show_chord(ChordPropertiesTarget target,std::function<void(size_t)> selected){if(!supports_chord(target))return;follow_chord(target,std::move(selected));ensure_window();synchronize();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(chordType_);}

bool PropertiesWindow::supports_band(const EmbeddedBandTarget& target)const{return framework_&&read_embedded_band(*framework_,target).has_value();}
void PropertiesWindow::follow_band(EmbeddedBandTarget target,std::function<void(const EmbeddedBandTarget&)> selected){
    const bool switched=!bandTarget_||bandTarget_->document!=target.document||bandTarget_->track!=target.track||bandTarget_->groups!=target.groups||bandTarget_->logical!=target.logical||bandTarget_->identity!=target.identity;
    sequenceTarget_.reset();target_.reset();tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();bandTarget_=std::move(target);bandSelectionChanged_=std::move(selected);synchronize(switched);
}
void PropertiesWindow::show_band(EmbeddedBandTarget target,std::function<void(const EmbeddedBandTarget&)> selected){
    if(!supports_band(target))return;follow_band(std::move(target),std::move(selected));ensure_window();synchronize(true);ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(bandFields_[0]);
}
bool PropertiesWindow::supports_sequence(const SequenceNotePropertiesTarget& target)const{
    if(!framework_||target.document>=framework_->documents().size())return false;
    const auto& d=framework_->document(target.document);
    if(d.selected_groups()!=target.groups||d.selected_sequence_index()!=target.track)return false;
    try{const auto positions=d.note_positions();if(target.event>=positions.size())return false;const auto& p=positions[target.event];const auto& n=p.note;return p.logicalTime>=0&&p.logicalTime<d.length()&&n.time>=0&&n.duration>0&&std::int64_t(n.time)+n.duration<=d.length()&&n.pitch<=127&&n.velocity>0&&n.velocity<=127;}catch(const std::exception&){return false;}
}
void PropertiesWindow::follow_sequence(SequenceNotePropertiesTarget target,std::function<void(size_t)> selected){
    const bool switched=!sequenceTarget_||sequenceTarget_->document!=target.document||sequenceTarget_->track!=target.track||sequenceTarget_->event!=target.event||sequenceTarget_->groups!=target.groups;
    target_.reset();tempoDocument_.reset();lyricTarget_.reset();markerTarget_.reset();meterTarget_.reset();chordTarget_.reset();bandTarget_.reset();sequenceTarget_=target;sequenceSelectionChanged_=std::move(selected);synchronize(switched);
}
void PropertiesWindow::show_sequence(SequenceNotePropertiesTarget target,std::function<void(size_t)> selected){
    if(!supports_sequence(target))return;follow_sequence(target,std::move(selected));ensure_window();synchronize(true);ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);SetFocus(sequenceFields_[0]);
}
void PropertiesWindow::edit_sequence(UINT field,int delta){
    if(updating_||finalizing_||!sequenceTarget_||!supports_sequence(*sequenceTarget_)||field<SequenceValue||field>=SequenceValue+17)return;
    auto& d=framework_->document(sequenceTarget_->document);auto note=d.notes().at(sequenceTarget_->event);const auto index=field-SequenceValue;finalizing_=true;
    try{
        size_t selected=sequenceTarget_->event;bool edited=false;
        if(index>=15){
            const auto input=text(sequenceFields_[index]);size_t end=0;const auto parsed=std::stoll(input,&end);if(end!=input.size()||parsed<INT32_MIN||parsed>INT32_MAX)throw std::runtime_error("Invalid Sequence logical value");
            const auto logical=sequence_property_logical_time(d.timeline(),d.length(),d.note_positions().at(selected),index-15,parsed+delta);edited=d.edit_note_logical_time(selected,logical,&selected);
        }else{
            if(!index){const auto pitch=static_cast<int>(note.pitch)+delta;if(pitch<0||pitch>127)throw std::runtime_error("Invalid MIDI note");note.pitch=static_cast<std::uint8_t>(pitch);}
            else {const auto input=text(sequenceFields_[index]);size_t end=0;const auto parsed=std::stoll(input,&end);if(end!=input.size()||parsed<INT32_MIN||parsed>INT32_MAX)throw std::runtime_error("Invalid Sequence value");note=sequence_property_edit(d.timeline(),d.length(),note,index,parsed+delta);}
            edited=d.edit_note(selected,note,&selected);
        }
        if(edited){sequenceTarget_->event=selected;if(sequenceSelectionChanged_)sequenceSelectionChanged_(selected);if(changed_)changed_();}
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_band(UINT field,int delta){
    if(updating_||finalizing_||!bandTarget_||field<BandValue||field>=BandValue+6)return;
    auto value=read_embedded_band(*framework_,*bandTarget_);if(!value)return;finalizing_=true;
    try{
        const auto index=field-BandValue;auto logical=value->event.logicalTime,physical=value->event.physicalTime;
        if(!index)value->band.set_name(text(bandFields_[0]));
        else{
            const auto input=text(bandFields_[index]);size_t end=0;const auto parsed=std::stoll(input,&end);
            if(end!=input.size()||parsed<INT32_MIN||parsed>INT32_MAX)throw std::runtime_error("Invalid Band position");
            const auto n=parsed+delta;if(n<INT32_MIN||n>INT32_MAX||(index!=3&&n<1))throw std::runtime_error("Invalid Band position");
            if(index<=3){auto p=band_start_position(value->timeline,physical);if(index==1)p.measure=static_cast<std::int32_t>(n-1);if(index==2)p.beat=static_cast<std::int32_t>(n-1);if(index==3)p.tick=static_cast<std::int32_t>(n);physical=band_start_clocks(value->timeline,p);}
            else{auto p=value->timeline.position(logical);if(index==4)p.measure=static_cast<std::int32_t>(n-1);else p.beat=static_cast<std::int32_t>(n-1);logical=value->timeline.clocks(p);}
        }
        if(update_embedded_band(*framework_,*bandTarget_,logical,physical,value->band.save_bytes())){if(bandSelectionChanged_)bandSelectionChanged_(*bandTarget_);if(changed_)changed_();}
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_chord_type(){
    if(updating_||!chordTarget_||!supports_chord(*chordTarget_))return;
    auto& d=framework_->document(chordTarget_->document);auto e=d.chords(chordTarget_->track).at(chordTarget_->event);finalizing_=true;
    try{e.name=text(chordType_);size_t selected=chordTarget_->event;
        if(d.edit_chord(selected,e,chordTarget_->track,&selected)){chordTarget_->event=selected;if(chordSelectionChanged_)chordSelectionChanged_(selected);if(changed_)changed_();}
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_chord_structure(unsigned level,unsigned note,bool scale){
    if(updating_||!chordTarget_||!supports_chord(*chordTarget_))return;
    try{auto& d=framework_->document(chordTarget_->document);auto e=d.chords(chordTarget_->track).at(chordTarget_->event);
        const bool edited=scale?toggle_chord_scale(e,level,note):toggle_chord_tone(e,level,note);if(!edited){MessageBeep(MB_ICONWARNING);return;}
        size_t selected=chordTarget_->event;if(d.edit_chord(selected,e,chordTarget_->track,&selected)){chordTarget_->event=selected;if(chordSelectionChanged_)chordSelectionChanged_(selected);if(changed_)changed_();}synchronize(true);
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
}
void PropertiesWindow::transpose_chord_root(int delta){
    if(updating_||!chordTarget_||!supports_chord(*chordTarget_))return;
    try{auto& d=framework_->document(chordTarget_->document);auto e=d.chords(chordTarget_->track).at(chordTarget_->event);
        if(!transpose_chord(e,delta)){MessageBeep(MB_ICONWARNING);return;}size_t selected=chordTarget_->event;
        if(d.edit_chord(selected,e,chordTarget_->track,&selected)){chordTarget_->event=selected;if(chordSelectionChanged_)chordSelectionChanged_(selected);if(changed_)changed_();}synchronize(true);
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
}
void PropertiesWindow::edit_meter_measure(int delta){
    if(updating_||!meterTarget_||!supports_meter(*meterTarget_))return;
    auto& d=framework_->document(meterTarget_->document);if(!d.can_edit_meter())return;finalizing_=true;
    try{const auto value=text(meterMeasure_);size_t end=0;const auto measure=std::stoll(value,&end)+delta;
        if(end!=value.size()||measure<1||measure>INT32_MAX)throw std::runtime_error("Invalid Time Signature measure");
        const auto e=d.timeline().meters().at(meterTarget_->event);size_t selected=meterTarget_->event;
        if(d.edit_meter(selected,static_cast<std::int32_t>(measure-1),e.beats,e.denominator,e.grids,&selected)){meterTarget_->event=selected;if(meterSelectionChanged_)meterSelectionChanged_(selected);if(changed_)changed_();}
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_meter_signature(){
    if(updating_||!meterTarget_||!supports_meter(*meterTarget_))return;
    auto& d=framework_->document(meterTarget_->document);if(!d.can_edit_meter())return;
    auto e=d.timeline().meters().at(meterTarget_->event);if(!meter_dialog(window_,e))return;
    size_t selected=meterTarget_->event;if(d.edit_meter(selected,e.measure,e.beats,e.denominator,e.grids,&selected)){meterTarget_->event=selected;if(meterSelectionChanged_)meterSelectionChanged_(selected);if(changed_)changed_();}synchronize(true);
}
void PropertiesWindow::edit_marker(UINT field,int delta){
    if(updating_||!markerTarget_||!supports_marker(*markerTarget_)||field<=MarkerValue||field>=MarkerValue+5)return;
    auto& d=framework_->document(markerTarget_->document);auto e=d.markers(markerTarget_->track).at(markerTarget_->event);finalizing_=true;
    try{const auto index=field-MarkerValue;const auto value=text(markerFields_[index]);size_t end=0;const auto n=std::stoll(value,&end)+delta;
        if(end!=value.size()||n<0||n>INT32_MAX||(index<4&&!n))throw std::runtime_error("Invalid Marker position");
        const auto axis=d.timeline();auto p=axis.grid_position(e.time);
        if(index==1)p.measure=static_cast<std::int32_t>(n-1);if(index==2)p.beat=static_cast<std::int32_t>(n-1);if(index==3)p.grid=static_cast<std::int32_t>(n-1);if(index==4)p.tick=static_cast<std::int32_t>(n);
        e.time=axis.grid_clocks(p);size_t selected=markerTarget_->event;
        if(d.edit_marker(selected,e,markerTarget_->track,&selected)){markerTarget_->event=selected;if(markerSelectionChanged_)markerSelectionChanged_(selected);if(changed_)changed_();}
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_lyric(UINT field,int delta){
    if(updating_||!lyricTarget_||!supports_lyric(*lyricTarget_))return;
    auto& d=framework_->document(lyricTarget_->document);auto e=d.lyrics(lyricTarget_->track).at(lyricTarget_->event);finalizing_=true;
    try{
        if(field==LyricValue)e.text=text(lyricFields_[0]);
        else if(field>LyricValue&&field<LyricValue+6){
            const auto index=field-LyricValue;const auto value=text(lyricFields_[index]);size_t end=0;const auto n=std::stoll(value,&end)+delta;if(end!=value.size()||n<0||n>INT32_MAX)throw std::runtime_error("Invalid Lyric position");
            const bool logical=index>=4;const auto axis=d.timeline();auto p=axis.position(logical?e.logical:e.physical);
            if(index==1||index==4){if(!n)throw std::runtime_error("Bars start at1");p.measure=static_cast<std::int32_t>(n-1);}
            if(index==2||index==5){if(!n)throw std::runtime_error("Beats start at1");p.beat=static_cast<std::int32_t>(n-1);}
            if(index==3)p.tick=static_cast<std::int32_t>(n);
            // The logical clock can contain a tick absent from the original
            // Belongs To page. Preserve it while editing its bar or beat.
            if(logical)e.logical=axis.clocks(p);else e.physical=axis.clocks(p);
        }else if(field>=LyricTiming&&field<LyricTiming+3)e.timing=lyricDelivery[field-LyricTiming];else{finalizing_=false;return;}
        size_t selected=lyricTarget_->event;if(d.edit_lyric(selected,e,lyricTarget_->track,&selected)){
            lyricTarget_->event=selected;if(lyricSelectionChanged_)lyricSelectionChanged_(selected);if(changed_)changed_();
        }
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_segment_length(){
    if(updating_||!supports(target_)||target_->kind!=DocumentKind::Segment)return;
    const auto document=target_->index;auto& d=framework_->document(document);const auto before=d.length_settings();const auto accepted=show_segment_length(window_,d);
    // Preserve imported partial measures and sub-millisecond reference lengths
    // when OK is pressed without editing the displayed fields.
    if(accepted&&(accepted->clockTime!=before.clockTime||(accepted->clockTime?accepted->milliseconds!=before.milliseconds:accepted->measures!=before.measures))){if(d.set_length(*accepted)&&changed_)changed_();}
    synchronize();
}
void PropertiesWindow::ensure_window(){
    if(!window_){
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TAB_CLASSES|ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&controls))throw std::runtime_error("Properties tab initialization failed");
        WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);c.lpszClassName=L"SourceProducerProperties";
        if(!RegisterClassW(&c)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Properties registration failed");
        RECT rect{0,0,380,210};AdjustWindowRectEx(&rect,WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_BORDER,FALSE,0);
        RECT ownerRect{};GetWindowRect(owner_,&ownerRect);const int width=rect.right-rect.left,height=rect.bottom-rect.top;
        // Group boxes do not erase their interiors. Let the owner erase the
        // background beneath them when a contained sibling label is hidden.
        window_=CreateWindowW(c.lpszClassName,L"Properties",WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_BORDER,ownerRect.right-width-16,ownerRect.top+80,width,height,owner_,nullptr,c.hInstance,this);
        if(!window_)throw std::runtime_error("Properties window creation failed");
    }
}
void PropertiesWindow::edit_tempo(UINT field,bool final,int delta){
    if(updating_||!tempoDocument_||!supports_tempo(*tempoDocument_)||field<TempoValue||field>=TempoValue+4)return;
    auto& d=framework_->document(*tempoDocument_);
    if(std::count_if(d.tempos().begin(),d.tempos().end(),[](const auto& e){return e.selected&&e.bpm>0;})!=1)return;
    const auto selected=std::find_if(d.tempos().begin(),d.tempos().end(),[](const auto& e){return e.selected&&e.bpm>0;});
    const auto index=field-TempoValue;bool changed=false;
    finalizing_=final;
    try{
        const auto value=text(tempoFields_[index]);size_t end=0;
        if(!index){
            double bpm=std::stod(value,&end);if(end!=value.size()||!std::isfinite(bpm))throw std::runtime_error("Invalid Tempo");
            bpm+=delta;
            if(final)bpm=std::clamp(bpm,1.0,1000.0);
            if(bpm>=1&&bpm<=1000)changed=d.change_selected(std::round(bpm*100.0)/100.0);
        }else if(final){
            auto number=std::stoll(value,&end);if(end!=value.size())throw std::runtime_error("Invalid Tempo position");number+=delta;
            auto position=d.timeline().position(selected->time);
            if(index==1)position.measure=static_cast<std::int32_t>(std::clamp(number,1LL,32767LL)-1);
            if(index==2)position.beat=static_cast<std::int32_t>(std::clamp(number,1LL,256LL)-1);
            if(index==3)position.tick=static_cast<std::int32_t>(std::clamp(number,0LL,32767LL));
            changed=d.move_selected_tempo(d.timeline().clocks(position));
        }
    }catch(const std::exception&){if(final)MessageBeep(MB_ICONWARNING);}
    try{if(changed&&changed_)changed_();if(final)synchronize();}catch(...){finalizing_=false;throw;}
    finalizing_=false;
}
void PropertiesWindow::edit_wave(UINT field,int delta){
    if(updating_||!supports(target_)||target_->kind!=DocumentKind::Wave)return;
    auto& d=framework_->wave_document(target_->index);bool changed=false;finalizing_=true;
    try{
        if(field==WaveValue){const auto value=wave_pitch_value(text(waveFields_[0]));const auto adjusted=std::clamp(static_cast<int>(value)+delta,0,127);changed=d.set_root_note(static_cast<unsigned>(adjusted));}
        else if(field==WaveCheck+3)waveLoopLocked_=SendMessageW(waveChecks_[3],BM_GETCHECK,0,0)==BST_CHECKED;
        else {
            auto loops=d.sample_loops();
            if(field==WaveCheck){
                if(SendMessageW(waveChecks_[0],BM_GETCHECK,0,0)==BST_CHECKED){
                    if(loops.empty()){const auto remembered=disabledWaveLoops_.find(target_->index);if(remembered!=disabledWaveLoops_.end())loops=remembered->second;
                        if(loops.empty()){const auto frames=d.format().frames;if(!frames||frames>UINT32_MAX)throw std::runtime_error("Wave length exceeds loop capacity");loops.push_back({0,0,static_cast<std::uint32_t>(frames)});}}
                    changed=d.set_sample_loops(loops);
                }else {if(!loops.empty())disabledWaveLoops_[target_->index]=loops;changed=d.set_sample_loops({});}
            }else if(!loops.empty()){
                if(field==WaveCheck+1||field==WaveCheck+2)loops[0].type=field-(WaveCheck+1);
                else if(field>=WaveValue+2&&field<=WaveValue+4){
                    const auto input=text(waveFields_[field-WaveValue]);if(input.empty()||input.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Loop field requires a nonnegative integer");
                    auto value=std::stoull(input);if(delta<0){if(value<static_cast<unsigned>(-delta))throw std::runtime_error("Loop field below zero");value-=static_cast<unsigned>(-delta);}else {if(value>UINT64_MAX-static_cast<unsigned>(delta))throw std::runtime_error("Loop field overflow");value+=static_cast<unsigned>(delta);}
                    const auto kind=field==WaveValue+2?WaveLoopField::start:field==WaveValue+3?WaveLoopField::end:WaveLoopField::length;
                    loops[0]=wave_loop_value(loops[0],kind,value,d.format().frames,waveLoopLocked_);
                }else {finalizing_=false;return;}
                changed=d.set_sample_loops(loops);
            }
        }
        if(changed&&changed_)changed_();
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize(true);finalizing_=false;
}
void PropertiesWindow::edit_segment_info(UINT field){
    if(updating_||finalizing_||!supports(target_)||target_->kind!=DocumentKind::Segment||field<SegmentInfoValue||field>=SegmentInfoValue+8)return;
    const auto index=field-SegmentInfoValue;auto& d=framework_->document(target_->index);finalizing_=true;
    try{
        bool changed=false;const auto value=text(segmentInfoFields_[index]);
        if(index<4){const SegmentInfoField kinds[]={SegmentInfoField::subject,SegmentInfoField::author,SegmentInfoField::copyright,SegmentInfoField::information};changed=d.set_info_text(kinds[index],value);}
        else{if(value.empty()||value.size()>4||value.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Version requires up to four digits");auto version=d.info_version();version[index-4]=static_cast<std::uint16_t>(std::stoul(value));changed=d.set_info_version(version);}
        if(changed&&changed_)changed_();
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_segment_boundary(UINT field){
    if(updating_||finalizing_||!supports(target_)||target_->kind!=DocumentKind::Segment||TabCtrl_GetCurSel(tabs_)!=3)return;
    auto& d=framework_->document(target_->index);finalizing_=true;
    try{
        bool changed=false;
        if(field==BoundaryList){const auto selected=SendMessageW(segmentBoundaryList_,CB_GETCURSEL,0,0);if(selected<0||selected>8)throw std::runtime_error("Boundary selection missing");changed=d.set_performance_boundary(static_cast<SegmentPerformanceBoundary>(selected));}
        else if(field==BoundaryMarker)changed=d.set_boundary_marker(SendMessageW(segmentBoundaryMarker_,BM_GETCHECK,0,0)==BST_CHECKED);
        else if(field==BoundaryQuick||field==BoundaryPrepare)changed=d.set_boundary_after_prepare(field==BoundaryPrepare);
        else if(field>=BoundaryInvalidation&&field<BoundaryInvalidation+3)changed=d.set_invalidation(static_cast<SegmentInvalidation>(field-BoundaryInvalidation));
        else if(field>=BoundaryAlignment&&field<BoundaryAlignment+4)changed=d.set_alignment(static_cast<SegmentAlignment>(field-BoundaryAlignment));
        if(changed&&changed_)changed_();
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_segment_start(UINT field,int delta){
    if(updating_||finalizing_||!supports(target_)||target_->kind!=DocumentKind::Segment||field<SegmentStartValue||field>=SegmentStartValue+4)return;
    auto& d=framework_->document(target_->index);if(!d.loop_settings().music_time())return;finalizing_=true;
    try{
        const auto column=field-SegmentStartValue;const auto input=text(segmentStartFields_[column]);
        if(input.empty()||input.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Start values require nonnegative integers");
        const auto parsed=std::stoull(input);if(parsed>INT32_MAX)throw std::runtime_error("Start position out of range");const auto value=static_cast<std::int64_t>(parsed)+delta;
        if(value<0||value>INT32_MAX||(column!=3&&value<1))throw std::runtime_error("Start bars beats and grids start at1");
        auto context=d;context.select_track_group(1);const auto axis=context.timeline();auto point=axis.grid_position(d.play_start());const auto n=static_cast<std::int32_t>(value-(column==3?0:1));
        if(column==0)point.measure=n;else if(column==1)point.beat=n;else if(column==2)point.grid=n;else point.tick=n;
        const auto clock=axis.grid_clocks(point);const auto roundTrip=axis.grid_position(clock);
        if(point.measure!=roundTrip.measure||point.beat!=roundTrip.beat||point.grid!=roundTrip.grid||point.tick!=roundTrip.tick)throw std::runtime_error("Start position is outside its measure beat or grid");
        if(d.set_music_start(clock)&&changed_)changed_();
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
void PropertiesWindow::edit_segment_loop(UINT field,int delta){
    if(updating_||finalizing_||!supports(target_)||target_->kind!=DocumentKind::Segment)return;
    if((field<LoopValue||field>=LoopValue+9)&&field!=LoopInfinite&&field!=LoopReset)return;
    auto& d=framework_->document(target_->index);auto loop=d.loop_settings();if(!loop.music_time())return;finalizing_=true;
    try{
        if(field==LoopInfinite){
            if(loop.repeats!=UINT32_MAX)finiteLoopRepeats_[target_->index]=loop.repeats;
            loop.repeats=IsDlgButtonChecked(window_,LoopInfinite)==BST_CHECKED?UINT32_MAX:finiteLoopRepeats_[target_->index];
        }else if(field==LoopReset){loop.start=0;loop.end=0;}
        else{
            const auto index=field-LoopValue;const auto input=text(loopFields_[index]);
            if(input.empty()||input.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Loop values require nonnegative integers");
            const auto parsed=std::stoull(input);if(parsed>UINT32_MAX)throw std::runtime_error("Loop value exceeds DWORD");const auto value=static_cast<std::int64_t>(parsed)+delta;
            if(value<0||value>UINT32_MAX)throw std::runtime_error("Loop value out of range");
            if(!index){if(value==UINT32_MAX)throw std::runtime_error("Use Infinite for unlimited repetitions");loop.repeats=static_cast<std::uint32_t>(value);}
            else{
                if(value>INT32_MAX)throw std::runtime_error("Loop position out of range");const auto row=(index-1)/4,column=(index-1)%4;if(column!=3&&value<1)throw std::runtime_error("Loop bars beats and grids start at1");
                auto context=d;context.select_track_group(1);const auto axis=context.timeline();auto position=axis.grid_position(row?(loop.end?loop.end:d.length()):loop.start);const auto n=static_cast<std::int32_t>(value-(column==3?0:1));
                if(column==0)position.measure=n;else if(column==1)position.beat=n;else if(column==2)position.grid=n;else position.tick=n;
                const auto clock=axis.grid_clocks(position);const auto roundTrip=axis.grid_position(clock);
                if(position.measure!=roundTrip.measure||position.beat!=roundTrip.beat||position.grid!=roundTrip.grid||position.tick!=roundTrip.tick)throw std::runtime_error("Loop position is outside its measure beat or grid");
                if(!row)loop.start=clock;else if(loop.end||clock!=d.length())loop.end=clock;
            }
        }
        if(d.set_music_loop(loop.repeats,loop.start,loop.end)&&changed_)changed_();
    }catch(const std::exception&){MessageBeep(MB_ICONWARNING);}
    synchronize();finalizing_=false;
}
bool PropertiesWindow::focus_existing()const{
    if(!window_||!IsWindowVisible(window_)||!IsWindowEnabled(window_))return false;
    const auto boundaryField=IsWindowEnabled(segmentBoundaryMarker_)?segmentBoundaryMarker_:segmentBoundaryList_;
    const auto field=sequenceTarget_?sequenceFields_[0]:bandTarget_?bandFields_[0]:chordTarget_?chordType_:meterTarget_?meterMeasure_:markerTarget_?markerFields_[1]:lyricTarget_?lyricFields_[0]:tempoDocument_?tempoFields_[0]:target_&&target_->kind==DocumentKind::Segment&&TabCtrl_GetCurSel(tabs_)==3?boundaryField:target_&&target_->kind==DocumentKind::Segment&&TabCtrl_GetCurSel(tabs_)==2?segmentInfoFields_[0]:target_&&target_->kind==DocumentKind::Segment&&TabCtrl_GetCurSel(tabs_)==1?loopFields_[1]:target_&&target_->kind==DocumentKind::Wave&&TabCtrl_GetCurSel(tabs_)==0?waveFields_[0]:name_;
    const auto target=field&&IsWindowVisible(field)&&IsWindowEnabled(field)?field:window_;
    SetForegroundWindow(window_);SetFocus(target);return GetFocus()==target;
}
bool PropertiesWindow::translate(MSG& message){
    if(!owns(message.hwnd))return false;
    const auto id=GetDlgCtrlID(message.hwnd);
    if(message.message==WM_KEYDOWN&&message.wParam==VK_TAB&&(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&supports(target_)&&(target_->kind==DocumentKind::Wave||target_->kind==DocumentKind::Segment)){
        // Commit the current sample field before changing pages, just as a
        // mouse focus change does. The same modeless window remains active.
        if(id==WaveValue||(id>=WaveValue+2&&id<=WaveValue+4))edit_wave(id);
        if(id>=LoopValue&&id<LoopValue+9)edit_segment_loop(id);
        if(id>=SegmentStartValue&&id<SegmentStartValue+4)edit_segment_start(id);
        if(id>=SegmentInfoValue&&id<SegmentInfoValue+8)edit_segment_info(id);
        const auto count=TabCtrl_GetItemCount(tabs_),direction=(GetKeyState(VK_SHIFT)&0x8000)?-1:1;
        TabCtrl_SetCurSel(tabs_,(TabCtrl_GetCurSel(tabs_)+direction+count)%count);clear_page_background();synchronize(true);
        RedrawWindow(window_,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
        if(target_->kind==DocumentKind::Segment)(void)focus_existing();else SetFocus(TabCtrl_GetCurSel(tabs_)==0?waveFields_[0]:name_);return true;
    }
    if(message.message==WM_KEYDOWN&&message.wParam==VK_TAB&&!(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&id>=SegmentInfoValue&&id<SegmentInfoValue+8){
        // Information is multiline, but Tab navigates the property form.
        // Use the native visible/enabled tab order for both directions.
        edit_segment_info(id);
        if(const auto next=GetNextDlgTabItem(window_,message.hwnd,(GetKeyState(VK_SHIFT)&0x8000)!=0)){SetFocus(next);SendMessageW(next,EM_SETSEL,0,-1);}
        return true;
    }
    if(message.message==WM_KEYDOWN&&message.wParam=='A'&&(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&
       ((id>=SegmentInfoValue&&id<SegmentInfoValue+8)||(id>=SegmentStartValue&&id<SegmentStartValue+4)||(id>=LoopValue&&id<LoopValue+9)||(id>=SequenceValue&&id<SequenceValue+17)||(id>=BandValue&&id<BandValue+6)||id==Name||id==ChordType||id==MeterMeasure||(id>=TempoValue&&id<TempoValue+4)||(id>=LyricValue&&id<LyricValue+6)||(id>MarkerValue&&id<MarkerValue+5)||(id>=WaveValue&&id<WaveValue+8))){
        SendMessageW(message.hwnd,EM_SETSEL,0,-1);return true;
    }
    if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN){if(sequenceTarget_&&id>=SequenceValue&&id<SequenceValue+17){edit_sequence(id);return true;}if(bandTarget_&&id>=BandValue&&id<BandValue+6){edit_band(id);return true;}if(chordTarget_&&id==ChordType){edit_chord_type();return true;}if(meterTarget_&&id==MeterMeasure){edit_meter_measure();return true;}if(tempoDocument_&&id>=TempoValue&&id<TempoValue+4){edit_tempo(id,true);return true;}if(lyricTarget_&&id>=LyricValue&&id<LyricValue+6){edit_lyric(id);return true;}if(markerTarget_&&id>MarkerValue&&id<MarkerValue+5){edit_marker(id);return true;}}
    if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN&&(id==WaveValue||(id>=WaveValue+2&&id<=WaveValue+4))){edit_wave(id);return true;}
    if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN&&id>=LoopValue&&id<LoopValue+9){edit_segment_loop(id);return true;}
    if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN&&id>=SegmentStartValue&&id<SegmentStartValue+4){edit_segment_start(id);return true;}
    if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN&&id>=SegmentInfoValue&&id<SegmentInfoValue+8){if(id==SegmentInfoValue+3&&!(GetKeyState(VK_CONTROL)&0x8000))return false;edit_segment_info(id);return true;}
    return IsDialogMessageW(window_,&message);
}
void PropertiesWindow::release(){sequenceTarget_.reset();sequenceSelectionChanged_={};bandTarget_.reset();bandSelectionChanged_={};chordTarget_.reset();chordSelectionChanged_={};if(window_)DestroyWindow(window_);target_.reset();tempoDocument_.reset();lyricTarget_.reset();lyricSelectionChanged_={};markerTarget_.reset();markerSelectionChanged_={};}
LRESULT CALLBACK PropertiesWindow::proc(HWND window,UINT message,WPARAM w,LPARAM l){
    auto self=reinterpret_cast<PropertiesWindow*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    try{
        if(message==WM_CREATE){
            self=static_cast<PropertiesWindow*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));self->window_=window;
            self->tabs_=child(window,WC_TABCONTROLW,L"",WS_CLIPSIBLINGS,8,6,364,196);
            TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L"Segment");if(TabCtrl_InsertItem(self->tabs_,0,&item)<0)throw std::runtime_error("Properties tab creation failed");
            self->label_=child(window,L"STATIC",L"Name:",0,20,45,40,22);
            self->name_=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,64,42,292,24,Name);SendMessageW(self->name_,EM_SETLIMITTEXT,255,0);
            self->segmentLengthLabel_=child(window,L"STATIC",L"Length:",0,255,45,44,22);
            self->segmentLengthButton_=child(window,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,300,42,62,24,SegmentLengthButton);
            // The compact English Start Time labels use the classic dialog
            // face; DEFAULT_GUI_FONT can select wider locale-dependent glyphs.
            self->segmentStartFont_=CreateFontW(-MulDiv(8,static_cast<int>(GetDpiForWindow(window)),72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,L"MS Sans Serif");
            if(!self->segmentStartFont_)throw std::runtime_error("Start Time font creation failed");
            const auto startControl=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,UINT id=0){const auto control=child(window,type,title,style,x,y,width,height,id);SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(self->segmentStartFont_),TRUE);self->segmentStartControls_.push_back(control);return control;};
            startControl(L"BUTTON",L"Start Time",BS_GROUPBOX,20,88,352,66);
            const wchar_t* startColumns[]={L"Bar",L"Beat",L"Grid",L"Tick"};for(unsigned i=0;i<4;++i)startControl(L"STATIC",startColumns[i],0,98+static_cast<int>(i)*60,94,50,16);
            startControl(L"STATIC",L"Start at:",0,40,111,56,18);
            for(unsigned i=0;i<4;++i){self->segmentStartFields_[i]=startControl(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,98+static_cast<int>(i)*60,109,52,18,SegmentStartValue+i);SendMessageW(self->segmentStartFields_[i],EM_SETLIMITTEXT,11,0);self->segmentStartSpins_[i]=startControl(UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,18,SegmentStartSpin+i);SendMessageW(self->segmentStartSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->segmentStartFields_[i]),0);SendMessageW(self->segmentStartSpins_[i],UDM_SETRANGE32,i==3?0:1,INT32_MAX);}
            startControl(L"STATIC",L"Use TimeSig of Track Group",0,40,136,144,18);const auto timeSigGroup=startControl(L"EDIT",L"1",WS_BORDER|ES_READONLY,182,133,36,18,SegmentTimeSigGroup);EnableWindow(timeSigGroup,FALSE);
            const auto timeSigSpin=startControl(UPDOWN_CLASSW,L"",UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,18,SegmentTimeSigGroupSpin);SendMessageW(timeSigSpin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(timeSigGroup),0);EnableWindow(timeSigSpin,FALSE);
            EnableWindow(startControl(L"BUTTON",L"Extend Primary TimeSig",BS_AUTOCHECKBOX,234,134,134,18,SegmentExtendTimeSig),FALSE);
            EnableWindow(startControl(L"BUTTON",L"Play on new instance of embedded audiopath",BS_AUTOCHECKBOX,20,170,340,18,SegmentEmbeddedAudioPath),FALSE);
            const auto infoControl=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,UINT id=0){const auto control=child(window,type,title,style,x,y,width,height,id);SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(self->segmentStartFont_),TRUE);self->segmentInfoControls_.push_back(control);return control;};
            const wchar_t* infoLabels[]={L"Subject:",L"Author:",L"Copyright:"};
            for(unsigned i=0;i<3;++i){const int y=34+static_cast<int>(i)*24;infoControl(L"STATIC",infoLabels[i],0,16,y+2,58,16);self->segmentInfoFields_[i]=infoControl(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,76,y,298,18,SegmentInfoValue+i);SendMessageW(self->segmentInfoFields_[i],EM_SETLIMITTEXT,32767,0);}
            infoControl(L"STATIC",L"Version:",0,16,110,58,16);
            for(unsigned i=0;i<4;++i){self->segmentInfoFields_[i+4]=infoControl(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,76+static_cast<int>(i)*36,108,32,18,SegmentInfoValue+i+4);SendMessageW(self->segmentInfoFields_[i+4],EM_SETLIMITTEXT,4,0);}
            EnableWindow(infoControl(L"BUTTON",L"Edit GUID...",BS_PUSHBUTTON,248,106,76,22,SegmentInfoGuid),FALSE);
            infoControl(L"STATIC",L"Information:",0,16,134,58,16);self->segmentInfoFields_[3]=infoControl(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,76,132,298,62,SegmentInfoValue+3);SendMessageW(self->segmentInfoFields_[3],EM_SETLIMITTEXT,32767,0);
            const auto boundaryControl=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,UINT id=0){const auto control=child(window,type,title,style,x,y,width,height,id);SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(self->segmentStartFont_),TRUE);self->segmentBoundaryControls_.push_back(control);return control;};
            self->segmentBoundaryName_=boundaryControl(L"STATIC",L"",0,16,31,360,16);
            boundaryControl(L"BUTTON",L"Performance Boundary",BS_GROUPBOX,16,48,188,103);
            self->segmentBoundaryMarker_=boundaryControl(L"BUTTON",L"&Start at next marker",WS_TABSTOP|BS_AUTOCHECKBOX,26,68,170,18,BoundaryMarker);
            self->segmentBoundaryFallback_=boundaryControl(L"STATIC",L"If no markers, start:",0,26,89,168,16);
            self->segmentBoundaryDefault_=boundaryControl(L"BUTTON",L"Use Segment's &Default",BS_AUTOCHECKBOX,26,106,170,18);EnableWindow(self->segmentBoundaryDefault_,FALSE);
            self->segmentBoundaryList_=boundaryControl(WC_COMBOBOXW,L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,26,127,168,180,BoundaryList);
            for(const auto caption:{L"Immediate",L"Grid",L"Beat",L"Barline",L"Align to Beat",L"Align to Barline",L"Align to Segment",L"End of Segment",L"End of Segment Queue"})SendMessageW(self->segmentBoundaryList_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(caption));
            boundaryControl(L"BUTTON",L"Align Options",BS_GROUPBOX,212,48,165,103);
            self->segmentAlignmentDescription_=boundaryControl(L"STATIC",L"If no enter switchpoints\r\n(destination), switch:",0,222,62,146,24);
            const wchar_t* alignCaptions[]={L"at an&y Time",L"&on any Grid",L"o&n any Beat",L"on any &Barline"};
            for(unsigned i=0;i<4;++i)self->segmentAlignment_[i]=boundaryControl(L"BUTTON",alignCaptions[i],WS_TABSTOP|BS_AUTORADIOBUTTON|(i?0:WS_GROUP),234,86+static_cast<int>(i)*16,136,16,BoundaryAlignment+i);
            const wchar_t* invalidationCaptions[]={L"&Don't Cut Off",L"Do&n't Cut Off Secondary Segs",L"Cut Off (Regenerate &2ndary Segs)"};
            for(unsigned i=0;i<3;++i)self->segmentInvalidation_[i]=boundaryControl(L"BUTTON",invalidationCaptions[i],WS_TABSTOP|BS_AUTORADIOBUTTON|(i?0:WS_GROUP),16,154+static_cast<int>(i)*16,190,16,BoundaryInvalidation+i);
            boundaryControl(L"BUTTON",L"Timing",BS_GROUPBOX,212,155,165,46);
            for(unsigned i=0;i<2;++i)self->segmentBoundaryTiming_[i]=boundaryControl(L"BUTTON",i?L"After &Prepare Time":L"&Quick Response",WS_TABSTOP|BS_AUTORADIOBUTTON|(i?0:WS_GROUP),234,168+static_cast<int>(i)*16,138,16,i?BoundaryPrepare:BoundaryQuick);
            const auto loopControl=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,UINT id=0){const auto control=child(window,type,title,style,x,y,width,height,id);self->loopControls_.push_back(control);return control;};
            loopControl(L"BUTTON",L"",BS_GROUPBOX,18,42,142,48);
            loopControl(L"BUTTON",L"Clock Time Looping (Mls)",BS_GROUPBOX,170,42,200,48);
            loopControl(L"BUTTON",L"Music Time Looping",BS_GROUPBOX,18,96,352,104);
            const auto loopField=[&](unsigned i,int x,int y,int width){
                self->loopFields_[i]=loopControl(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,x,y,width,20,LoopValue+i);SendMessageW(self->loopFields_[i],EM_SETLIMITTEXT,10,0);
                self->loopSpins_[i]=loopControl(UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,LoopSpin+i);SendMessageW(self->loopSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->loopFields_[i]),0);SendMessageW(self->loopSpins_[i],UDM_SETRANGE32,static_cast<WPARAM>(INT32_MIN),INT32_MAX);
            };
            // Stock Edit accessibility uses the immediately preceding label.
            // The Repeat label also supplies the group caption in the same place.
            loopControl(L"STATIC",L"Repeat",0,26,42,43,16);loopField(0,34,61,52);
            self->loopInfinite_=loopControl(L"BUTTON",L"Infinite",WS_TABSTOP|BS_AUTOCHECKBOX,96,62,62,20,LoopInfinite);
            self->loopClock_=loopControl(L"BUTTON",L"",BS_AUTOCHECKBOX,180,63,18,18,LoopClock);
            for(unsigned i=0;i<2;++i){loopControl(L"STATIC",i?L"End":L"Start",0,i?278:200,64,i?28:30,18);self->loopClockFields_[i]=loopControl(L"EDIT",L"0",WS_BORDER|ES_READONLY,230+static_cast<int>(i)*78,62,i?50:44,20,LoopClockStart+i);}
            const wchar_t* loopColumns[]={L"Bar",L"Beat",L"Grid",L"Tick"};
            for(unsigned i=0;i<4;++i)loopControl(L"STATIC",loopColumns[i],0,116+static_cast<int>(i)*60,108,50,18);
            for(unsigned row=0;row<2;++row){loopControl(L"STATIC",row?L"Loop End":L"Loop Start",0,30,132+static_cast<int>(row)*24,82,18);for(unsigned column=0;column<4;++column)loopField(1+row*4+column,116+static_cast<int>(column)*60,130+static_cast<int>(row)*24,56);}
            self->loopReset_=loopControl(L"BUTTON",L"Reset Loop to Entire Segment",WS_TABSTOP|BS_PUSHBUTTON,146,177,212,20,LoopReset);
            const wchar_t* sequenceLabels[]={L"MIDI Note",L"",L"Velocity:",L"Bar",L"Beat",L"Grid",L"Tick",L"Start Time:",L"End Time",L"Duration",L"Belongs to Beat"};
            for(unsigned i=3;i<7;++i)self->sequenceLabels_[i]=child(window,L"STATIC",sequenceLabels[i],0,100+static_cast<int>(i-3)*60,74,54,18);
            for(unsigned i=0;i<17;++i){const int x=i>=15?100+static_cast<int>(i-15)*60:i==0?108:i==1?154:i==2?270:100+static_cast<int>((i-3)%4)*60;const int y=i>=15?160:i<3?36:94+static_cast<int>((i-3)/4)*22;
                // The preceding label supplies the stock Edit accessible name.
                if(i<3)self->sequenceLabels_[i]=child(window,L"STATIC",sequenceLabels[i],0,i==0?24:i==1?154:222,y+2,72,18);
                else if(i==15)self->sequenceLabels_[10]=child(window,L"STATIC",sequenceLabels[10],0,16,y,84,18);
                else if(i<15&&(i-3)%4==0){const auto label=7+(i-3)/4;self->sequenceLabels_[label]=child(window,L"STATIC",sequenceLabels[label],0,16,y,72,18);}
                self->sequenceFields_[i]=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|(i?0:ES_READONLY),x,y,i<3?44:56,20,SequenceValue+i);SendMessageW(self->sequenceFields_[i],EM_SETLIMITTEXT,11,0);
                self->sequenceSpins_[i]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,SequenceSpin+i);SendMessageW(self->sequenceSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->sequenceFields_[i]),0);SendMessageW(self->sequenceSpins_[i],UDM_SETRANGE32,static_cast<WPARAM>(INT32_MIN),INT32_MAX);
            }
            const wchar_t* labels[]={L"Tempo:",L"Measure",L"Beat",L"Tick"};
            for(size_t i=0;i<4;++i){
                const int y=40+static_cast<int>(i)*24;
                self->tempoLabels_[i]=child(window,L"STATIC",labels[i],0,40,y+2,48,20);
                self->tempoFields_[i]=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|(i==1||i==2?ES_NUMBER:0),88,y,64,20,TempoValue+static_cast<UINT>(i));
                SendMessageW(self->tempoFields_[i],EM_SETLIMITTEXT,i==0?7:i==1?5:i==2?3:5,0);
                self->tempoSpins_[i]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,TempoSpin+static_cast<UINT>(i));
                SendMessageW(self->tempoSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->tempoFields_[i]),0);
                SendMessageW(self->tempoSpins_[i],UDM_SETRANGE32,i==3?0:1,i==0?1000:i==2?256:32767);
            }
            const wchar_t* lyricLabels[]={L"Lyric:",L"Bar",L"Beat",L"Tick",L"Start Time:",L"Belongs To:"};
            const int labelX[]={20,82,150,218,20,20},labelY[]={42,68,68,68,94,120};
            const int fieldX[]={82,82,150,218,82,150},fieldY[]={40,92,92,92,118,118};
            for(size_t i=0;i<6;++i){self->lyricLabels_[i]=child(window,L"STATIC",lyricLabels[i],0,labelX[i],labelY[i],i>=4?62:54,20);self->lyricFields_[i]=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,fieldX[i],fieldY[i],i?60:278,20,LyricValue+static_cast<UINT>(i));SendMessageW(self->lyricFields_[i],EM_SETLIMITTEXT,i?10:0,0);}
            for(size_t i=0;i<5;++i){self->lyricSpins_[i]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,LyricSpin+static_cast<UINT>(i));SendMessageW(self->lyricSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->lyricFields_[i+1]),0);SendMessageW(self->lyricSpins_[i],UDM_SETRANGE32,i==2?0:1,i==1||i==4?256:32767);}
            const wchar_t* bandLabels[]={L"Band:",L"Bar",L"Beat",L"Tick",L"Start Time:",L"Belongs To:"};
            for(size_t i=0;i<6;++i){self->bandLabels_[i]=child(window,L"STATIC",bandLabels[i],0,labelX[i],labelY[i],i>=4?62:54,20);self->bandFields_[i]=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,fieldX[i],fieldY[i],i?60:278,20,BandValue+static_cast<UINT>(i));SendMessageW(self->bandFields_[i],EM_SETLIMITTEXT,i?11:255,0);}
            for(size_t i=0;i<5;++i){self->bandSpins_[i]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,BandSpin+static_cast<UINT>(i));SendMessageW(self->bandSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->bandFields_[i+1]),0);SendMessageW(self->bandSpins_[i],UDM_SETRANGE32,i==2?INT32_MIN:1,INT32_MAX);}
            self->lyricTimingGroup_=child(window,L"BUTTON",L"Timing",BS_GROUPBOX,82,142,196,56);
            const wchar_t* deliveryLabels[]={L"Quick response",L"Before Time Stamp",L"At Time Stamp"};
            for(size_t i=0;i<3;++i)self->lyricTiming_[i]=child(window,L"BUTTON",deliveryLabels[i],BS_AUTORADIOBUTTON|WS_TABSTOP|(i?0:WS_GROUP),118,148+static_cast<int>(i)*16,154,16,LyricTiming+static_cast<UINT>(i));
            const wchar_t* markerLabels[]={L"Type:",L"Measure:",L"Beat:",L"Grid:",L"Tick:"};
            for(size_t i=0;i<5;++i){const int y=i?78+static_cast<int>(i-1)*24:40;
                self->markerLabels_[i]=child(window,L"STATIC",markerLabels[i],0,30,y+2,60,20);
                self->markerFields_[i]=child(window,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL|(i?WS_TABSTOP:ES_READONLY),94,y,i?64:256,20,MarkerValue+static_cast<UINT>(i));
                if(i){SendMessageW(self->markerFields_[i],EM_SETLIMITTEXT,10,0);self->markerSpins_[i-1]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,MarkerSpin+static_cast<UINT>(i-1));SendMessageW(self->markerSpins_[i-1],UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->markerFields_[i]),0);SendMessageW(self->markerSpins_[i-1],UDM_SETRANGE32,i==4?0:1,32767);}
            }
            self->chordLabel_=child(window,L"STATIC",L"Chord type:",0,24,45,76,20);
            self->chordType_=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,78,40,126,20,ChordType);SendMessageW(self->chordType_,EM_SETLIMITTEXT,16,0);
            self->chordRoot_=child(window,L"EDIT",L"",WS_BORDER|ES_READONLY,32,40,44,20,ChordRoot);
            self->chordSpin_=child(window,UPDOWN_CLASSW,L"",UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,ChordSpin);SendMessageW(self->chordSpin_,UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->chordRoot_),0);SendMessageW(self->chordSpin_,UDM_SETRANGE32,0,23);
            self->chordNotation_[0]=child(window,L"BUTTON",L"Sharps",WS_TABSTOP|WS_GROUP|BS_AUTORADIOBUTTON,212,40,66,20,ChordSharps);
            self->chordNotation_[1]=child(window,L"BUTTON",L"Flats",WS_TABSTOP|BS_AUTORADIOBUTTON,278,40,58,20,ChordFlats);
            for(unsigned level=0;level<4;++level){const int y=66+static_cast<int>(3-level)*32;self->chordLevels_[level]=child(window,L"STATIC",std::to_wstring(level+1).c_str(),0,22,y+12,18,18);
                self->chordKeys_[level].create(window,ChordKeys+level,false,44,y,240,32,[self,level](unsigned note){self->edit_chord_structure(level,note,false);});
                self->scaleKeys_[level].create(window,ScaleKeys+level,true,300,y,60,32,[self,level](unsigned note){self->edit_chord_structure(level,note,true);});}
            self->meterButton_=child(window,L"BUTTON",L"Time Signature...",WS_TABSTOP,40,40,130,26,MeterButton);
            self->meterLabel_=child(window,L"STATIC",L"Measure:",0,40,85,60,20);self->meterMeasure_=child(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_NUMBER,104,82,64,20,MeterMeasure);SendMessageW(self->meterMeasure_,EM_SETLIMITTEXT,10,0);
            self->meterSpin_=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,MeterSpin);SendMessageW(self->meterSpin_,UDM_SETBUDDY,reinterpret_cast<WPARAM>(self->meterMeasure_),0);SendMessageW(self->meterSpin_,UDM_SETRANGE32,1,32767);
            const wchar_t* waveLabels[]={L"Root Note",L"Sample Length:",L"Start",L"End",L"Length",L"Sample Rate:",L"Sample Size:",L"Length:"};
            const int wx[]={101,281,80,164,248,75,75,75},wy[]={74,74,135,135,135,102,138,170};
            const int lx[]={30,197,110,192,275,12,12,30},ly[]={76,76,121,121,121,104,140,172};
            for(size_t i=0;i<8;++i){const bool editable=i==0||(i>=2&&i<=4),twoLine=i==5||i==6;
                // The original Info page wraps these captions onto two lines.
                // Reserve both lines at the inherited font size.
                self->waveLabels_[i]=child(window,L"STATIC",waveLabels[i],twoLine?SS_RIGHT:0,lx[i],ly[i],i==1?84:twoLine?55:62,twoLine?34:18);
                self->waveFields_[i]=child(window,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL|(editable?WS_TABSTOP:ES_READONLY),wx[i],wy[i],i==0?48:i==1?86:i<5?78:115,20,WaveValue+static_cast<UINT>(i));SendMessageW(self->waveFields_[i],EM_SETLIMITTEXT,20,0);}
            const wchar_t* loopLabels[]={L"Loop",L"Forward",L"Loop and Release",L"Lock"};const int cx[]={30,103,177,331},cy[]={104,104,104,137},cw[]={66,74,149,54};
            for(size_t i=0;i<4;++i)self->waveChecks_[i]=child(window,L"BUTTON",loopLabels[i],WS_TABSTOP|(i==1||i==2?BS_AUTORADIOBUTTON:BS_AUTOCHECKBOX)|(i==1?WS_GROUP:0),cx[i],cy[i],cw[i],18,WaveCheck+static_cast<UINT>(i));
            for(size_t i=0;i<4;++i){self->waveSpins_[i]=child(window,UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS,0,0,16,20,WaveSpin+static_cast<UINT>(i));const auto field=self->waveFields_[i?i+1:0];SendMessageW(self->waveSpins_[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(field),0);SendMessageW(self->waveSpins_[i],UDM_SETRANGE32,0,i?INT_MAX:127);}
            // The page controls are siblings of the tab. Keep its display area
            // behind them when live name changes repaint the selected tab.
            if(!SetWindowPos(self->tabs_,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE))throw std::runtime_error("Properties tab ordering failed");
            return 0;
        }
        if(message==WM_COMMAND&&self&&LOWORD(w)==SegmentLengthButton&&HIWORD(w)==BN_CLICKED){self->edit_segment_length();return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=SequenceValue&&LOWORD(w)<SequenceValue+17&&HIWORD(w)==EN_KILLFOCUS){self->edit_sequence(LOWORD(w));return 0;}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=SequenceSpin&&spin->hdr.idFrom<SequenceSpin+17){self->edit_sequence(SequenceValue+static_cast<UINT>(spin->hdr.idFrom)-SequenceSpin,spin->iDelta);return 1;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=BandValue&&LOWORD(w)<BandValue+6&&HIWORD(w)==EN_KILLFOCUS){self->edit_band(LOWORD(w));return 0;}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=BandSpin&&spin->hdr.idFrom<BandSpin+5){self->edit_band(BandValue+1+static_cast<UINT>(spin->hdr.idFrom)-BandSpin,spin->iDelta);return 1;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)==ChordType&&HIWORD(w)==EN_KILLFOCUS){self->edit_chord_type();return 0;}
        if(message==WM_COMMAND&&self&&(LOWORD(w)==ChordSharps||LOWORD(w)==ChordFlats)&&HIWORD(w)==BN_CLICKED){self->chordFlats_=LOWORD(w)==ChordFlats;self->synchronize();return 0;}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom==ChordSpin){self->transpose_chord_root(spin->iDelta);return 1;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)==MeterMeasure&&HIWORD(w)==EN_KILLFOCUS){self->edit_meter_measure();return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)==MeterButton&&HIWORD(w)==BN_CLICKED){self->edit_meter_signature();return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=LoopValue&&LOWORD(w)<LoopValue+9&&HIWORD(w)==EN_KILLFOCUS){self->edit_segment_loop(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=SegmentStartValue&&LOWORD(w)<SegmentStartValue+4&&HIWORD(w)==EN_KILLFOCUS){self->edit_segment_start(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&((LOWORD(w)==BoundaryList&&HIWORD(w)==CBN_SELCHANGE)||((LOWORD(w)==BoundaryMarker||LOWORD(w)==BoundaryQuick||LOWORD(w)==BoundaryPrepare||(LOWORD(w)>=BoundaryInvalidation&&LOWORD(w)<BoundaryInvalidation+3)||(LOWORD(w)>=BoundaryAlignment&&LOWORD(w)<BoundaryAlignment+4))&&HIWORD(w)==BN_CLICKED))){self->edit_segment_boundary(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=SegmentInfoValue&&LOWORD(w)<SegmentInfoValue+8&&HIWORD(w)==EN_KILLFOCUS){self->edit_segment_info(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&(LOWORD(w)==LoopInfinite||LOWORD(w)==LoopReset)&&HIWORD(w)==BN_CLICKED){self->edit_segment_loop(LOWORD(w));return 0;}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=LoopSpin&&spin->hdr.idFrom<LoopSpin+9){self->edit_segment_loop(LoopValue+static_cast<UINT>(spin->hdr.idFrom)-LoopSpin,spin->iDelta);return 1;}}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=SegmentStartSpin&&spin->hdr.idFrom<SegmentStartSpin+4){self->edit_segment_start(SegmentStartValue+static_cast<UINT>(spin->hdr.idFrom)-SegmentStartSpin,spin->iDelta);return 1;}}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom==MeterSpin){self->edit_meter_measure(spin->iDelta);return 1;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=TempoValue&&LOWORD(w)<TempoValue+4){if(HIWORD(w)==EN_KILLFOCUS||(LOWORD(w)==TempoValue&&HIWORD(w)==EN_CHANGE)){self->edit_tempo(LOWORD(w),HIWORD(w)==EN_KILLFOCUS);return 0;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=LyricValue&&LOWORD(w)<LyricValue+6&&HIWORD(w)==EN_KILLFOCUS){self->edit_lyric(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=LyricTiming&&LOWORD(w)<LyricTiming+3&&HIWORD(w)==BN_CLICKED){self->edit_lyric(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>MarkerValue&&LOWORD(w)<MarkerValue+5&&HIWORD(w)==EN_KILLFOCUS){self->edit_marker(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&(LOWORD(w)==WaveValue||(LOWORD(w)>=WaveValue+2&&LOWORD(w)<=WaveValue+4))&&HIWORD(w)==EN_KILLFOCUS){self->edit_wave(LOWORD(w));return 0;}
        if(message==WM_COMMAND&&self&&LOWORD(w)>=WaveCheck&&LOWORD(w)<WaveCheck+4&&HIWORD(w)==BN_CLICKED){self->edit_wave(LOWORD(w));return 0;}
        if(message==WM_NOTIFY&&self&&l){const auto header=reinterpret_cast<NMHDR*>(l);if(header->hwndFrom==self->tabs_&&header->code==TCN_SELCHANGE){self->clear_page_background();self->synchronize(true);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);if(self->target_&&self->target_->kind==DocumentKind::Segment)(void)self->focus_existing();return 0;}const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(header->code==UDN_DELTAPOS&&header->idFrom>=WaveSpin&&header->idFrom<WaveSpin+4){const auto offset=static_cast<UINT>(header->idFrom)-WaveSpin;self->edit_wave(WaveValue+(offset?offset+1:0),spin->iDelta);return 1;}}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=MarkerSpin&&spin->hdr.idFrom<MarkerSpin+4){self->edit_marker(MarkerValue+1+static_cast<UINT>(spin->hdr.idFrom)-MarkerSpin,spin->iDelta);return 1;}}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=TempoSpin&&spin->hdr.idFrom<TempoSpin+4){self->edit_tempo(TempoValue+static_cast<UINT>(spin->hdr.idFrom)-TempoSpin,true,spin->iDelta);return 1;}}
        if(message==WM_NOTIFY&&self&&l){const auto spin=reinterpret_cast<NMUPDOWN*>(l);if(spin->hdr.code==UDN_DELTAPOS&&spin->hdr.idFrom>=LyricSpin&&spin->hdr.idFrom<LyricSpin+5){self->edit_lyric(LyricValue+1+static_cast<UINT>(spin->hdr.idFrom)-LyricSpin,spin->iDelta);return 1;}}
        if(message==WM_COMMAND&&self&&LOWORD(w)==Name&&!self->updating_&&self->supports(self->target_)){
            if(HIWORD(w)==EN_KILLFOCUS){self->synchronize();return 0;}
            if(HIWORD(w)==EN_CHANGE){
                const auto value=text(self->name_);bool changed=false;
                if(self->target_->kind==DocumentKind::Segment)changed=self->framework_->document(self->target_->index).set_name(value);
                else if(self->target_->kind==DocumentKind::Wave)changed=self->framework_->wave_document(self->target_->index).set_name(value);
                else if(self->target_->kind==DocumentKind::Band)changed=self->framework_->set_band_name(self->target_->index,value);
                else changed=self->framework_->set_style_name(self->target_->index,value);
                if(changed&&self->changed_)self->changed_();return 0;
            }
        }
        if(message==WM_CLOSE){DestroyWindow(window);return 0;}
        if(message==WM_NCDESTROY&&self){self->sequenceTarget_.reset();self->sequenceSelectionChanged_={};for(auto& edit:self->sequenceFields_)edit=nullptr;for(auto& label:self->sequenceLabels_)label=nullptr;for(auto& spin:self->sequenceSpins_)spin=nullptr;self->bandTarget_.reset();self->bandSelectionChanged_={};for(size_t i=0;i<6;++i)self->bandFields_[i]=self->bandLabels_[i]=nullptr;for(auto& spin:self->bandSpins_)spin=nullptr;}
        if(message==WM_NCDESTROY&&self){self->loopControls_.clear();for(auto& field:self->loopFields_)field=nullptr;for(auto& spin:self->loopSpins_)spin=nullptr;for(auto& field:self->loopClockFields_)field=nullptr;self->loopInfinite_=self->loopReset_=self->loopClock_=nullptr;self->finiteLoopRepeats_.clear();self->tabFamily_=0;}
        if(message==WM_NCDESTROY&&self){self->segmentStartControls_.clear();for(auto& field:self->segmentStartFields_)field=nullptr;for(auto& spin:self->segmentStartSpins_)spin=nullptr;if(self->segmentStartFont_){DeleteObject(self->segmentStartFont_);self->segmentStartFont_=nullptr;}}
        if(message==WM_NCDESTROY&&self){self->segmentInfoControls_.clear();for(auto& field:self->segmentInfoFields_)field=nullptr;}
        if(message==WM_NCDESTROY&&self){self->segmentBoundaryControls_.clear();self->segmentBoundaryName_=self->segmentBoundaryMarker_=self->segmentBoundaryFallback_=self->segmentBoundaryDefault_=self->segmentBoundaryList_=self->segmentAlignmentDescription_=nullptr;for(auto& field:self->segmentBoundaryTiming_)field=nullptr;for(auto& field:self->segmentInvalidation_)field=nullptr;for(auto& field:self->segmentAlignment_)field=nullptr;}
        if(message==WM_NCDESTROY&&self){for(size_t i=0;i<8;++i)self->waveFields_[i]=self->waveLabels_[i]=nullptr;for(size_t i=0;i<4;++i)self->waveChecks_[i]=self->waveSpins_[i]=nullptr;self->disabledWaveLoops_.clear();self->waveLoopLocked_=false;self->chordRoot_=self->chordSpin_=nullptr;for(auto& control:self->chordNotation_)control=nullptr;for(auto& control:self->chordLevels_)control=nullptr;for(unsigned i=0;i<4;++i){self->chordKeys_[i].release();self->scaleKeys_[i].release();}}
        if(message==WM_NCDESTROY&&self){self->window_=self->tabs_=self->name_=self->label_=nullptr;self->segmentLengthLabel_=self->segmentLengthButton_=nullptr;self->chordType_=self->chordLabel_=nullptr;self->chordSelectionChanged_={};self->meterButton_=self->meterMeasure_=self->meterLabel_=self->meterSpin_=nullptr;self->meterTarget_.reset();self->chordTarget_.reset();self->meterSelectionChanged_={};self->target_.reset();self->tempoDocument_.reset();self->lyricTarget_.reset();self->lyricSelectionChanged_={};self->markerTarget_.reset();self->markerSelectionChanged_={};for(auto& edit:self->markerFields_)edit=nullptr;for(auto& label:self->markerLabels_)label=nullptr;for(auto& spin:self->markerSpins_)spin=nullptr;for(size_t i=0;i<4;++i)self->tempoFields_[i]=self->tempoLabels_[i]=self->tempoSpins_[i]=nullptr;for(size_t i=0;i<6;++i)self->lyricFields_[i]=self->lyricLabels_[i]=nullptr;for(auto& spin:self->lyricSpins_)spin=nullptr;for(auto& radio:self->lyricTiming_)radio=nullptr;self->lyricTimingGroup_=nullptr;SetWindowLongPtrW(window,GWLP_USERDATA,0);}
    }catch(const std::exception& e){if(message==WM_CREATE)return -1;MessageBoxA(window,e.what(),"Properties operation failed",MB_OK|MB_ICONERROR);return 0;}
    return DefWindowProcW(window,message,w,l);
}
}
