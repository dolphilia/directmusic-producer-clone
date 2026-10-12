#include "embedded_band_editor.h"
#include "band_mixer.h"
#include "gm_instruments.h"
#include <commctrl.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace producer::app { namespace {
constexpr UINT Channels=4800,Readout=4801,FirstChannel=4810,SelectionChanged=WM_APP+51;
std::vector<HWND> windows;
struct Drag {POINT start{};BandMixerArea area{};Bytes ownerBytes;std::vector<BandInstrument> instruments;std::vector<std::uint32_t> selected;std::vector<BandMixValue> preview,restore,lastSent;TimelineBandMixUpdate live;int dx=0,dy=0;};
struct Marquee {POINT start{},end{};std::vector<std::uint32_t> previous;bool additive=false;};
struct Session {
    Framework& host;EmbeddedBandTarget target;TimelineSelectionChanged changed;TimelineHistoryRequested history;TimelineBandMixStarted mixStarted;
    HWND list=nullptr,readout=nullptr;std::vector<HWND> buttons;std::vector<std::uint32_t> selected;std::vector<BandInstrument> instruments;
    std::optional<Drag> drag;std::optional<Marquee> marquee;Bytes displayedBand;bool displayInitialized=false,updating=false,owned=false,selectionPending=false;
    void publish(bool properties=false){if(const auto view=read_embedded_band(host,target)){target=view->target;if(changed)changed(TimelineEventTarget{TimelineEventKind::band,target.document,target.track,target.event,target.groups},properties);}}
};
HWND control(HWND parent,const wchar_t* type,const wchar_t* title,DWORD style,UINT id){auto w=CreateWindowW(type,title,WS_CHILD|WS_VISIBLE|style,0,0,1,1,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);if(!w)throw std::runtime_error("Band control creation failed");SendMessageW(w,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return w;}
RECT grid(HWND w){RECT r{};GetClientRect(w,&r);return {r.right*54/100,52,r.right-12,std::max<LONG>(150,r.bottom*70/100)};}
BandMixerArea area(HWND w){const auto r=grid(w);return {int(r.left)+8,int(r.top)+8,int(r.right)-8,int(r.bottom)-8,24,22};}
bool selected(const Session& s,std::uint32_t channel){return std::find(s.selected.begin(),s.selected.end(),channel)!=s.selected.end();}
BandInstrument display(const Session& s,size_t i){auto value=s.instruments.at(i);if(s.drag)for(const auto& v:s.drag->preview)if(v.pchannel==value.pchannel){value.pan=v.pan;value.volume=v.volume;value.flags|=0x60u;break;}return value;}
void readout(Session& s,std::optional<BandMixValue> value={}){if(!value&&!s.selected.empty()){const auto i=std::find_if(s.instruments.begin(),s.instruments.end(),[&](const auto& v){return v.pchannel==s.selected.front();});if(i!=s.instruments.end())value=BandMixValue{i->pchannel,i->pan,i->volume};}const auto text=value?L"Pan "+band_mixer_pan_label(value->pan)+L" Vol "+std::to_wstring(value->volume):std::wstring{};SetWindowTextW(s.readout,text.c_str());}
void layout(HWND w,Session& s){RECT r{};GetClientRect(w,&r);const auto g=grid(w);const auto a=area(w);MoveWindow(s.list,8,8,std::max(1,int(g.left)-16),std::max(1,int(g.bottom)-8),TRUE);MoveWindow(s.readout,std::max(0,int(r.right)-200),std::max(0,int(r.bottom)-24),188,22,TRUE);for(size_t i=0;i<s.buttons.size();++i){const auto p=band_mixer_position(display(s,i),i,a);MoveWindow(s.buttons[i],p.x,p.y,a.width,a.height,TRUE);}InvalidateRect(w,nullptr,TRUE);}
void selection_ui(Session& s){s.updating=true;for(size_t i=0;i<s.instruments.size();++i)ListView_SetItemState(s.list,static_cast<int>(i),selected(s,s.instruments[i].pchannel)?LVIS_SELECTED:0,LVIS_SELECTED);s.updating=false;for(auto b:s.buttons)InvalidateRect(b,nullptr,TRUE);readout(s);}
void choose(Session& s,std::uint32_t channel,bool ctrl){if(ctrl){const auto it=std::find(s.selected.begin(),s.selected.end(),channel);if(it==s.selected.end())s.selected.push_back(channel);else s.selected.erase(it);}else s.selected={channel};selection_ui(s);s.publish();}
void refresh(HWND,Session&);
void live_values(Drag& d,const std::vector<BandMixValue>& values){
    if(!d.live||values.empty())return;
    if(values.size()==d.lastSent.size()&&std::equal(values.begin(),values.end(),d.lastSent.begin(),[](const auto& a,const auto& b){return a.pchannel==b.pchannel&&a.pan==b.pan&&a.volume==b.volume;}))return;
    d.live(values);d.lastSent=values;
}
void rollback(Session& s,Drag& d){
    if(d.lastSent.empty())return;
    if(s.host.document(s.target.document).save_bytes()==d.ownerBytes)live_values(d,d.restore);
    else if(const auto view=read_embedded_band(s.host,s.target))if(const auto values=band_mixer_explicit_values(view->band.instruments(),d.selected))live_values(d,*values);
}
void cancel(HWND w,Session& s){auto d=std::move(s.drag);s.drag.reset();s.marquee.reset();if(GetCapture()==w)ReleaseCapture();if(d)rollback(s,*d);refresh(w,s);layout(w,s);}
void begin_drag(HWND w,Session& s,size_t index,POINT p){if(index>=s.instruments.size())return;const auto channel=s.instruments[index].pchannel;const bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;if(ctrl||!selected(s,channel))choose(s,channel,ctrl);if(!selected(s,channel))return;Drag d;d.start=p;d.area=area(w);d.ownerBytes=s.host.document(s.target.document).save_bytes();d.instruments=s.instruments;d.selected=s.selected;if(const auto restore=band_mixer_explicit_values(d.instruments,d.selected)){d.restore=*restore;if(s.mixStarted)d.live=s.mixStarted(s.target);}s.drag=std::move(d);SetCapture(w);}
void preview(HWND w,Session& s,POINT p){if(!s.drag)return;auto& d=*s.drag;d.dx=p.x-d.start.x;d.dy=p.y-d.start.y;if(!d.dx&&!d.dy)d.preview.clear();else d.preview=band_mixer_drag(d.instruments,d.selected,d.area,d.dx,d.dy);live_values(d,d.preview.empty()?d.restore:d.preview);layout(w,s);if(!d.preview.empty())readout(s,d.preview.front());else readout(s);}
void commit(HWND w,Session& s,POINT p){
    if(!s.drag)return;preview(w,s,p);auto d=std::move(*s.drag);s.drag.reset();if(GetCapture()==w)ReleaseCapture();bool committed=false;
    try{
        if((d.dx||d.dy)&&s.host.document(s.target.document).save_bytes()==d.ownerBytes)if(const auto view=read_embedded_band(s.host,s.target)){
            auto band=view->band;
            if(band.set_mix(d.preview)&&update_embedded_band(s.host,s.target,view->event.logicalTime,view->event.physicalTime,band.save_bytes())){
                committed=true;if(!d.live&&s.mixStarted)if(const auto live=s.mixStarted(view->target))live(d.preview);s.publish();
            }
        }
    }catch(...){if(!committed)rollback(s,d);throw;}
    if(!committed)rollback(s,d);refresh(w,s);layout(w,s);
}
LRESULT CALLBACK button_proc(HWND button,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){if(message==WM_LBUTTONDOWN){const auto parent=GetParent(button);auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(parent,GWLP_USERDATA));if(s){POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};MapWindowPoints(button,parent,&p,1);SetFocus(button);try{begin_drag(parent,*s,static_cast<size_t>(GetDlgCtrlID(button)-FirstChannel),p);}catch(const std::exception& e){MessageBoxA(parent,e.what(),"Band selection failed",MB_OK|MB_ICONERROR);}return 0;}}if(message==WM_NCDESTROY)RemoveWindowSubclass(button,button_proc,1);return DefSubclassProc(button,message,wp,lp);}
void refresh(HWND w,Session& s){
    if(s.updating)return;if(s.drag){if(s.host.document(s.target.document).save_bytes()==s.drag->ownerBytes)return;cancel(w,s);return;}
    const auto view=read_embedded_band(s.host,s.target);
    if(view&&s.displayInitialized&&view->event.band==s.displayedBand){
        // Selection notifications refresh the host while a native ListView click is
        // still in progress. Keep its items, focus and selection anchor intact.
        s.target=view->target;selection_ui(s);return;
    }
    s.displayedBand=view?view->event.band:Bytes{};s.displayInitialized=view.has_value();
    s.updating=true;s.instruments=view?view->band.instruments():std::vector<BandInstrument>{};if(view)s.target=view->target;
    s.selected.erase(std::remove_if(s.selected.begin(),s.selected.end(),[&](auto c){return std::none_of(s.instruments.begin(),s.instruments.end(),[&](const auto& i){return i.pchannel==c;});}),s.selected.end());
    SetWindowTextW(w,view?(L"Band: "+view->band.name()).c_str():L"Band: unavailable");EnableWindow(s.list,view.has_value());ListView_DeleteAllItems(s.list);
    for(size_t i=0;i<s.instruments.size();++i){const auto& instrument=s.instruments[i];const auto channel=std::to_wstring(std::uint64_t(instrument.pchannel)+1);LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=static_cast<int>(i);item.pszText=const_cast<wchar_t*>(channel.c_str());ListView_InsertItem(s.list,&item);const std::wstring values[]={channel,gm_instrument_name(instrument.patch),(instrument.flags&128)?L"—":L"0",(instrument.flags&128)?L"—":L"None",(instrument.flags&2048)?L"—":L"Standard"};for(int column=1;column<6;++column)ListView_SetItemText(s.list,item.iItem,column,const_cast<wchar_t*>(values[column-1].c_str()));if(selected(s,instrument.pchannel))ListView_SetItemState(s.list,item.iItem,LVIS_SELECTED,LVIS_SELECTED);}
    if(s.buttons.size()!=s.instruments.size()){for(auto button:s.buttons)DestroyWindow(button);s.buttons.clear();for(size_t i=0;i<s.instruments.size();++i){if(i>=65535-FirstChannel)throw std::runtime_error("Band editor control capacity exceeded");const auto label=std::to_wstring(std::uint64_t(s.instruments[i].pchannel)+1);auto b=control(w,L"BUTTON",label.c_str(),WS_TABSTOP|BS_OWNERDRAW,FirstChannel+static_cast<UINT>(i));SetWindowSubclass(b,button_proc,1,0);s.buttons.push_back(b);}}
    for(size_t i=0;i<s.buttons.size();++i){const auto label=std::to_wstring(std::uint64_t(s.instruments[i].pchannel)+1);SetWindowTextW(s.buttons[i],label.c_str());EnableWindow(s.buttons[i],view.has_value());InvalidateRect(s.buttons[i],nullptr,TRUE);}s.updating=false;layout(w,s);readout(s);
}
void marquee(HWND w,Session& s,POINT p){auto& m=*s.marquee;m.end=p;RECT r{std::min(m.start.x,p.x),std::min(m.start.y,p.y),std::max(m.start.x,p.x),std::max(m.start.y,p.y)};s.selected=m.additive?m.previous:std::vector<std::uint32_t>{};const auto a=area(w);for(size_t i=0;i<s.instruments.size();++i){const auto q=band_mixer_position(s.instruments[i],i,a);RECT b{q.x,q.y,q.x+a.width,q.y+a.height},intersection{};if(IntersectRect(&intersection,&r,&b)&&!selected(s,s.instruments[i].pchannel))s.selected.push_back(s.instruments[i].pchannel);}selection_ui(s);InvalidateRect(w,nullptr,TRUE);}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=reinterpret_cast<Session*>(static_cast<MDICREATESTRUCTW*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams)->lParam);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));s->list=control(w,WC_LISTVIEWW,L"PChannels",WS_BORDER|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS,Channels);ListView_SetExtendedListViewStyle(s->list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);const wchar_t* names[]={L"PCh#",L"PCh Name",L"Instrument",L"Oct",L"Trans",L"Priority"};const int widths[]={45,95,115,42,50,80};for(int i=0;i<6;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.pszText=const_cast<wchar_t*>(names[i]);c.cx=widths[i];ListView_InsertColumn(s->list,i,&c);}s->readout=control(w,L"STATIC",L"",WS_BORDER|SS_LEFT,Readout);refresh(w,*s);return 0;}
    if(message==WM_SIZE&&s){if(s->drag||s->marquee)cancel(w,*s);layout(w,*s);return DefMDIChildProcW(w,message,wp,lp);}
    if(message==WM_MDIACTIVATE&&s&&reinterpret_cast<HWND>(lp)==w){s->publish();return DefMDIChildProcW(w,message,wp,lp);}
    if(message==WM_LBUTTONDOWN&&s){POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};const auto r=grid(w);if(PtInRect(&r,p)){SetFocus(w);s->marquee=Marquee{p,p,s->selected,(GetKeyState(VK_CONTROL)&0x8000)!=0};marquee(w,*s,p);SetCapture(w);return 0;}}
    if(message==WM_MOUSEMOVE&&s){POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};if(s->drag){preview(w,*s,p);return 0;}if(s->marquee){marquee(w,*s,p);return 0;}}
    if(message==WM_LBUTTONUP&&s){POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};if(s->drag){commit(w,*s,p);return 0;}if(s->marquee){marquee(w,*s,p);s->marquee.reset();ReleaseCapture();s->publish();return 0;}}
    if((message==WM_CANCELMODE||message==WM_CAPTURECHANGED)&&s&&(s->drag||s->marquee)){if(s->marquee)s->selected=s->marquee->previous;cancel(w,*s);return 0;}
    if(message==WM_KEYDOWN&&s&&wp==VK_ESCAPE&&(s->drag||s->marquee)){if(s->marquee)s->selected=s->marquee->previous;cancel(w,*s);return 0;}
    if(message==WM_CLOSE&&s&&(s->drag||s->marquee))cancel(w,*s);
    if(message==WM_COMMAND&&s&&LOWORD(wp)>=FirstChannel&&size_t(LOWORD(wp)-FirstChannel)<s->instruments.size()&&HIWORD(wp)==BN_CLICKED){choose(*s,s->instruments[LOWORD(wp)-FirstChannel].pchannel,(GetKeyState(VK_CONTROL)&0x8000)!=0);return 0;}
    if(message==SelectionChanged&&s){s->selectionPending=false;s->publish();return 0;}
    if(message==WM_NOTIFY&&s&&lp){const auto header=reinterpret_cast<NMHDR*>(lp);if(header->hwndFrom==s->list){if(header->code==LVN_ITEMCHANGED&&!s->updating){const auto change=reinterpret_cast<NMLISTVIEW*>(lp);if((change->uChanged&LVIF_STATE)&&((change->uNewState^change->uOldState)&LVIS_SELECTED)){s->selected.clear();for(int at=ListView_GetNextItem(s->list,-1,LVNI_SELECTED);at>=0;at=ListView_GetNextItem(s->list,at,LVNI_SELECTED))if(size_t(at)<s->instruments.size())s->selected.push_back(s->instruments[at].pchannel);for(auto b:s->buttons)InvalidateRect(b,nullptr,TRUE);readout(*s);if(!s->selectionPending){s->selectionPending=true;PostMessageW(w,SelectionChanged,0,0);}}}
        if(header->code==NM_CUSTOMDRAW){auto draw=reinterpret_cast<NMLVCUSTOMDRAW*>(lp);if(draw->nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;if(draw->nmcd.dwDrawStage==CDDS_ITEMPREPAINT)return CDRF_NOTIFYSUBITEMDRAW;if(draw->nmcd.dwDrawStage==(CDDS_ITEMPREPAINT|CDDS_SUBITEM)){const auto i=draw->nmcd.dwItemSpec;if(i<s->instruments.size()&&draw->iSubItem>=2&&!(s->instruments[i].flags&0x28fbu))draw->clrText=GetSysColor(COLOR_GRAYTEXT);return CDRF_NEWFONT;}}}}
    if(message==WM_DRAWITEM&&s){const auto draw=reinterpret_cast<DRAWITEMSTRUCT*>(lp);const auto i=draw->CtlID-FirstChannel;if(draw->CtlID>=FirstChannel&&i<s->instruments.size()){const auto instrument=display(*s,i);const bool active=selected(*s,instrument.pchannel);auto brush=CreateSolidBrush(active?RGB(255,0,0):GetSysColor(COLOR_BTNFACE));FillRect(draw->hDC,&draw->rcItem,brush);DeleteObject(brush);RECT edge=draw->rcItem;DrawEdge(draw->hDC,&edge,EDGE_RAISED,BF_RECT);SetBkMode(draw->hDC,TRANSPARENT);SetTextColor(draw->hDC,active?RGB(255,255,255):GetSysColor(band_mixer_active(instrument)?COLOR_BTNTEXT:COLOR_GRAYTEXT));const auto label=std::to_wstring(std::uint64_t(instrument.pchannel)+1);DrawTextW(draw->hDC,label.c_str(),-1,&edge,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(draw->itemState&ODS_FOCUS)DrawFocusRect(draw->hDC,&edge);return TRUE;}}
    if(message==WM_PAINT&&s){PAINTSTRUCT p{};const auto dc=BeginPaint(w,&p);auto r=grid(w);FillRect(dc,&r,reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1));DrawEdge(dc,&r,EDGE_SUNKEN,BF_RECT);const auto pen=CreatePen(PS_DOT,1,GetSysColor(COLOR_GRAYTEXT));const auto old=SelectObject(dc,pen);for(int x=r.left+8;x<r.right;x+=8){MoveToEx(dc,x,r.top,nullptr);LineTo(dc,x,r.bottom);}for(int y=r.top+8;y<r.bottom;y+=8){MoveToEx(dc,r.left,y,nullptr);LineTo(dc,r.right,y);}SelectObject(dc,old);DeleteObject(pen);const auto center=CreatePen(PS_SOLID,1,RGB(128,128,0));SelectObject(dc,center);const int x=(r.left+r.right)/2,y=(r.top+r.bottom)/2;MoveToEx(dc,x,r.top,nullptr);LineTo(dc,x,r.bottom);MoveToEx(dc,r.left,y,nullptr);LineTo(dc,r.right,y);SelectObject(dc,old);DeleteObject(center);if(s->marquee){const auto& m=*s->marquee;RECT selection{std::min(m.start.x,m.end.x),std::min(m.start.y,m.end.y),std::max(m.start.x,m.end.x),std::max(m.start.y,m.end.y)};DrawFocusRect(dc,&selection);}EndPaint(w,&p);return 0;}
    if(message==WM_NCDESTROY&&s){if(s->drag){auto d=std::move(*s->drag);s->drag.reset();rollback(*s,d);}windows.erase(std::remove(windows.begin(),windows.end(),w),windows.end());const auto parent=GetParent(w);SetWindowLongPtrW(w,GWLP_USERDATA,0);if(s->owned)delete s;const auto result=DefMDIChildProcW(w,message,wp,lp);if(parent){PostMessageW(GetParent(parent),SegmentWorkspaceChangedMessage,0,0);if(!segment_timeline_workspace_has_editors())ShowWindow(parent,SW_HIDE);}return result;}
}catch(const std::exception& e){if(message==WM_CREATE)return -1;if(s&&(s->drag||s->marquee))cancel(w,*s);MessageBoxA(w,e.what(),"Band editor operation failed",MB_OK|MB_ICONERROR);}return DefMDIChildProcW(w,message,wp,lp);}
}
size_t embedded_band_editor_count(){return windows.size();}
void show_embedded_band_editor(HWND workspace,Framework& host,EmbeddedBandTarget target,TimelineSelectionChanged changed,TimelineHistoryRequested history,TimelineBandMixStarted mixStarted){
    if(!read_embedded_band(host,target))return;for(auto w:windows){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(s&&s->target.document==target.document&&s->target.groups==target.groups&&s->target.track==target.track){const auto view=read_embedded_band(host,s->target);if(view&&view->target.event==target.event){ShowWindow(workspace,SW_SHOW);if(IsIconic(w))ShowWindow(w,SW_RESTORE);SendMessageW(workspace,WM_MDIACTIVATE,reinterpret_cast<WPARAM>(w),0);SetFocus(s->list);return;}}}
    static bool registered=false;if(!registered){WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);c.lpszClassName=L"SourceProducerEmbeddedBand";if(!RegisterClassW(&c))throw std::runtime_error("Band editor class registration failed");registered=true;}
    auto session=std::make_unique<Session>(Session{host,std::move(target),std::move(changed),std::move(history),std::move(mixStarted)});SetWindowPos(workspace,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);const auto window=CreateMDIWindowW(L"SourceProducerEmbeddedBand",L"Band",WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,940,430,workspace,GetModuleHandleW(nullptr),reinterpret_cast<LPARAM>(session.get()));if(!window)throw std::runtime_error("Band editor creation failed");windows.push_back(window);session->owned=true;session.release();PostMessageW(GetParent(workspace),SegmentWorkspaceChangedMessage,0,0);SendMessageW(workspace,WM_MDIACTIVATE,reinterpret_cast<WPARAM>(window),0);SetFocus(GetDlgItem(window,Channels));
}
void refresh_embedded_band_editors(){for(auto w:windows)if(auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA)))refresh(w,*s);}
bool translate_embedded_band_editor(MSG& message){for(auto w:windows)if(message.hwnd==w||IsChild(w,message.hwnd)){if(!IsWindowEnabled(w)||!IsWindowEnabled(GetAncestor(w,GA_ROOT)))return false;auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(message.message==WM_KEYDOWN){if(message.wParam==VK_ESCAPE&&(s->drag||s->marquee)){SendMessageW(w,WM_KEYDOWN,VK_ESCAPE,0);return true;}if(!(GetKeyState(VK_MENU)&0x8000)&&!(GetKeyState(VK_SHIFT)&0x8000)){if(message.wParam==VK_F11&&!(GetKeyState(VK_CONTROL)&0x8000)){s->publish(true);return true;}if((message.wParam=='Z'||message.wParam=='Y')&&(GetKeyState(VK_CONTROL)&0x8000)){cancel(w,*s);s->publish();if(s->history)s->history(message.wParam=='Y');refresh(w,*s);s->publish();return true;}}}return IsDialogMessageW(w,&message);}return false;}
void release_embedded_band_editors(){const auto closing=windows;for(auto w:closing)if(IsWindow(w))SendMessageW(GetParent(w),WM_MDIDESTROY,reinterpret_cast<WPARAM>(w),0);}
}
