#include "wave_document_editor.h"
#include "wave_viewport.h"
#include <commdlg.h>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <vector>
#include <climits>
namespace producer::app {namespace {
enum:UINT{Documents=10,Open,Save,Name,Root,Apply,Undo,Redo,Status,Loops,LoopType,LoopStart,LoopLength,ApplyLoop,RemoveLoop,DisableLoops,Canvas,SelectionBegin,SelectionEnd,Insertion,CopyPcm,CutPcm,DeletePcm,PastePcm,SelectAll,SnapZero,SetLoopSelection,CloneLoop,ZoomIn,ZoomOut,WaveScroll,FindBestLoopEnd};
bool snapToZero=true;
struct Session{Framework& host;size_t selected=0,selectedLoop=0;std::uint64_t begin=0,end=0,insertion=0,anchor=0;std::vector<WaveViewport> views;};
void control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id=0){CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);}
WaveViewport& viewport(Session& s,std::uint64_t frames){
    if(s.views.size()<=s.selected)s.views.resize(s.selected+1);
    auto& view=s.views[s.selected];view.synchronize(frames);return view;
}
RECT waveform_area(HWND w){RECT r{};GetClientRect(w,&r);r.bottom=std::max<LONG>(1,r.bottom-GetSystemMetrics(SM_CYHSCROLL));return r;}
void update_view(HWND w,Session& s){
    const bool present=!s.host.waves().empty();
    const auto frames=present?s.host.wave_document(s.selected).format().frames:0;
    WaveViewport empty;auto& view=present?viewport(s,frames):empty;
    RECT r{};GetClientRect(w,&r);const int bar=GetSystemMetrics(SM_CYHSCROLL),button=GetSystemMetrics(SM_CXHSCROLL);
    MoveWindow(GetDlgItem(w,WaveScroll),0,std::max(0,int(r.bottom)-bar),std::max(1,int(r.right)-2*button),bar,TRUE);
    MoveWindow(GetDlgItem(w,ZoomIn),std::max(0,int(r.right)-2*button),std::max(0,int(r.bottom)-bar),button,bar,TRUE);
    MoveWindow(GetDlgItem(w,ZoomOut),std::max(0,int(r.right)-button),std::max(0,int(r.bottom)-bar),button,bar,TRUE);
    const auto maximum=frames?static_cast<int>(std::min<std::uint64_t>(frames-1,INT_MAX-1)):0;
    WaveViewport scale;scale.fit(frames);SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL};info.nMax=maximum;
    info.nPage=static_cast<UINT>(std::max(1,scale.pixel_at(view.count,maximum+1)));info.nPos=scale.pixel_at(view.first,maximum+1);
    SetScrollInfo(GetDlgItem(w,WaveScroll),SB_CTL,&info,TRUE);
    EnableWindow(GetDlgItem(w,ZoomIn),view.count>1);EnableWindow(GetDlgItem(w,ZoomOut),view.count<view.total);
    const auto label=L"Waveform, visible frames "+std::to_wstring(view.first)+L" to "+std::to_wstring(view.first+view.count);
    SetWindowTextW(w,label.c_str());InvalidateRect(w,nullptr,FALSE);
}
void draw_zoom_button(const DRAWITEMSTRUCT& item){
    auto r=item.rcItem;DrawFrameControl(item.hDC,&r,DFC_BUTTON,DFCS_BUTTONPUSH|((item.itemState&ODS_SELECTED)?DFCS_PUSHED:0)|((item.itemState&ODS_DISABLED)?DFCS_INACTIVE:0));
    const auto pen=CreatePen(PS_SOLID,1,GetSysColor((item.itemState&ODS_DISABLED)?COLOR_GRAYTEXT:COLOR_BTNTEXT));const auto previous=SelectObject(item.hDC,pen),brush=SelectObject(item.hDC,GetStockObject(HOLLOW_BRUSH));
    const int size=std::max(3,std::min(int(r.right-r.left),int(r.bottom-r.top))/3),x=int(r.left+r.right)/2-2,y=int(r.top+r.bottom)/2-2;
    Ellipse(item.hDC,x-size,y-size,x+size,y+size);MoveToEx(item.hDC,x+size-1,y+size-1,nullptr);LineTo(item.hDC,x+size+3,y+size+3);
    MoveToEx(item.hDC,x-size+2,y,nullptr);LineTo(item.hDC,x+size-1,y);if(item.CtlID==ZoomIn){MoveToEx(item.hDC,x,y-size+2,nullptr);LineTo(item.hDC,x,y+size-1);}
    SelectObject(item.hDC,brush);SelectObject(item.hDC,previous);DeleteObject(pen);
    if(item.itemState&ODS_FOCUS){InflateRect(&r,-2,-2);DrawFocusRect(item.hDC,&r);}
}
std::wstring text(HWND w,UINT id){const auto c=GetDlgItem(w,id);std::wstring value(GetWindowTextLengthW(c)+1,0);GetWindowTextW(c,value.data(),static_cast<int>(value.size()));value.pop_back();return value;}
std::wstring file(HWND w,bool save){wchar_t path[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=w;f.lpstrFile=path;f.nMaxFile=32768;f.lpstrFilter=L"Producer Wave\0*.wvp;*.wav\0";f.lpstrDefExt=L"wvp";f.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileNameW(&f):GetOpenFileNameW(&f))?path:L"";}

std::uint64_t frame_value(HWND w,UINT id){const auto value=text(w,id);if(value.empty()||value.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Frame fields require nonnegative integers");return std::stoull(value);}
void selection_fields(HWND w,Session& s){SetDlgItemTextW(w,SelectionBegin,std::to_wstring(s.begin).c_str());SetDlgItemTextW(w,SelectionEnd,std::to_wstring(s.end).c_str());SetDlgItemTextW(w,Insertion,std::to_wstring(s.insertion).c_str());InvalidateRect(GetDlgItem(w,Canvas),nullptr,FALSE);}
void read_selection(HWND w,Session& s,bool applySnap=true){
    const auto& d=s.host.wave_document(s.selected);const auto f=d.format();
    auto a=frame_value(w,SelectionBegin),b=frame_value(w,SelectionEnd),p=frame_value(w,Insertion);
    if(a>b||b>f.frames||p>f.frames)throw std::runtime_error("Selection or insertion is outside Wave");
    if(applySnap&&snapToZero){a=d.nearest_zero_frame(a);b=d.nearest_zero_frame(b);p=d.nearest_zero_frame(p);}
    s.begin=a;s.end=b;s.insertion=p;
}
void waveform_menu(HWND w,Session& s,LPARAM lp){
    if(s.host.waves().empty())return;
    read_selection(GetParent(w),s,false);
    const auto& d=s.host.wave_document(s.selected);const auto f=d.format();
    const bool pcm=f.tag==1&&(f.bits==8||f.bits==16||f.bits==24||f.bits==32),selection=s.begin<s.end;
    const auto loops=d.sample_loops();const auto menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Wave menu creation failed");
    const auto add=[&](UINT id,const wchar_t* label,bool enabled){AppendMenuW(menu,MF_STRING|(enabled?MF_ENABLED:MF_GRAYED),id,label);};
    add(CutPcm,L"Cut\tCtrl+X",pcm&&selection);add(CopyPcm,L"Copy\tCtrl+C",pcm&&selection);add(PastePcm,L"Paste\tCtrl+V",pcm&&IsClipboardFormatAvailable(CF_WAVE));
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING|(snapToZero?MF_CHECKED:MF_UNCHECKED),SnapZero,L"Snap to Zero");
    add(SelectAll,L"Select All\tCtrl+A",pcm);add(SetLoopSelection,L"Set Loop From Selection",pcm&&selection);add(FindBestLoopEnd,L"Find Best Loop End From Selection",pcm&&selection&&s.selectedLoop<loops.size()&&s.end>loops[s.selectedLoop].start);add(CloneLoop,L"Clone Loop",pcm&&s.selectedLoop<loops.size());add(DeletePcm,L"Delete\tDel",pcm&&selection);
    POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};if(point.x==-1&&point.y==-1){RECT r{};GetClientRect(w,&r);point={r.right/2,r.bottom/2};ClientToScreen(w,&point);}
    SetFocus(w);const auto id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,w,nullptr);DestroyMenu(menu);
    if(id)SendMessageW(GetParent(w),WM_COMMAND,id,0);
}
void put_wave_clipboard(HWND w,const Bytes& bytes){const auto mem=GlobalAlloc(GMEM_MOVEABLE,bytes.size());if(!mem)throw std::runtime_error("Wave clipboard allocation failed");auto data=GlobalLock(mem);if(!data){GlobalFree(mem);throw std::runtime_error("Wave clipboard lock failed");}std::memcpy(data,bytes.data(),bytes.size());GlobalUnlock(mem);if(!OpenClipboard(w)){GlobalFree(mem);throw std::runtime_error("Wave clipboard unavailable");}const bool done=EmptyClipboard()&&SetClipboardData(CF_WAVE,mem);CloseClipboard();if(!done){GlobalFree(mem);throw std::runtime_error("Wave clipboard publication failed");}}
Bytes get_wave_clipboard(HWND w){if(!OpenClipboard(w))throw std::runtime_error("Wave clipboard unavailable");const auto mem=GetClipboardData(CF_WAVE);const auto size=mem?GlobalSize(mem):0;auto data=mem?static_cast<const std::uint8_t*>(GlobalLock(mem)):nullptr;Bytes bytes;try{if(!data||size<12)throw std::runtime_error("Clipboard has no RIFF Wave");Bytes header(data,data+8);const auto length=std::uint64_t(read32(header,4))+8;if(length>size)throw std::runtime_error("Wave clipboard truncated");bytes.assign(data,data+static_cast<size_t>(length));}catch(...){if(data)GlobalUnlock(mem);CloseClipboard();throw;}GlobalUnlock(mem);CloseClipboard();return bytes;}
LRESULT CALLBACK waveform(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(GetParent(w),GWLP_USERDATA));
    if(!s)return DefWindowProcW(w,msg,wp,lp);
    try{
        if(msg==WM_CREATE){
            control(w,L"SCROLLBAR",L"",SBS_HORZ|WS_TABSTOP,0,0,1,1,WaveScroll);
            control(w,L"BUTTON",L"Zoom in",BS_OWNERDRAW|WS_TABSTOP,0,0,1,1,ZoomIn);
            control(w,L"BUTTON",L"Zoom out",BS_OWNERDRAW|WS_TABSTOP,0,0,1,1,ZoomOut);
            update_view(w,*s);return 0;
        }
        if(msg==WM_SIZE){update_view(w,*s);return 0;}
        if(msg==WM_DRAWITEM){
            const auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if(item&&(item->CtlID==ZoomIn||item->CtlID==ZoomOut)){draw_zoom_button(*item);return TRUE;}
        }
        if(msg==WM_COMMAND&&(LOWORD(wp)==ZoomIn||LOWORD(wp)==ZoomOut)){
            if(!s->host.waves().empty()){
                auto& view=viewport(*s,s->host.wave_document(s->selected).format().frames);
                view.zoom(LOWORD(wp)==ZoomIn,s->insertion);update_view(w,*s);
            }return 0;
        }
        if(msg==WM_HSCROLL&&reinterpret_cast<HWND>(lp)==GetDlgItem(w,WaveScroll)){
            if(s->host.waves().empty())return 0;
            auto& view=viewport(*s,s->host.wave_document(s->selected).format().frames);
            SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_TRACKPOS};GetScrollInfo(GetDlgItem(w,WaveScroll),SB_CTL,&info);
            switch(LOWORD(wp)){
            case SB_LINELEFT:view.scroll(false,false);break;
            case SB_LINERIGHT:view.scroll(true,false);break;
            case SB_PAGELEFT:view.scroll(false,true);break;
            case SB_PAGERIGHT:view.scroll(true,true);break;
            case SB_LEFT:view.scroll_to(0);break;
            case SB_RIGHT:view.scroll_to(view.total);break;
            case SB_THUMBTRACK:case SB_THUMBPOSITION:
                view.scroll_to(WaveViewport::proportion(view.total,static_cast<unsigned>(std::max(0,info.nTrackPos)),static_cast<unsigned>(info.nMax+1)));break;
            default:return 0;
            }update_view(w,*s);return 0;
        }
        if(msg==WM_CONTEXTMENU){waveform_menu(w,*s,lp);return 0;}
        if(msg==WM_LBUTTONDOWN||msg==WM_MOUSEMOVE||msg==WM_LBUTTONUP){
            if(s->host.waves().empty())return 0;
            const auto r=waveform_area(w);const auto f=s->host.wave_document(s->selected).format();
            const auto& view=viewport(*s,f.frames);const int x=static_cast<short>(LOWORD(lp));
            auto at=view.frame_at(x,int(r.right));
            if(snapToZero)at=s->host.wave_document(s->selected).nearest_zero_frame(at);
            if(msg==WM_LBUTTONDOWN){SetFocus(w);SetCapture(w);s->anchor=s->begin=s->end=s->insertion=at;}
            else if(GetCapture()==w){s->begin=std::min(at,s->anchor);s->end=std::max(at,s->anchor);if(msg==WM_LBUTTONUP)ReleaseCapture();}
            else return 0;
            selection_fields(GetParent(w),*s);return 0;
        }
        if(msg==WM_KEYDOWN){
            UINT id=0;
            if(GetKeyState(VK_CONTROL)<0){if(wp=='Z')id=Undo;else if(wp=='Y')id=Redo;else if(wp=='C')id=CopyPcm;else if(wp=='X')id=CutPcm;else if(wp=='V')id=PastePcm;else if(wp=='A')id=SelectAll;}
            else if(wp==VK_DELETE)id=DeletePcm;
            if(id){SendMessageW(GetParent(w),WM_COMMAND,id,0);return 0;}
        }
        if(msg==WM_PAINT){
            PAINTSTRUCT paint{};const auto dc=BeginPaint(w,&paint);const auto r=waveform_area(w);
            FillRect(dc,&r,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
            if(!s->host.waves().empty()){
                const auto& d=s->host.wave_document(s->selected);const auto f=d.format();auto& view=viewport(*s,f.frames);
                if(f.tag==1&&(f.bits==8||f.bits==16||f.bits==24||f.bits==32)&&view.count){
                    const auto whole=Chunk::parse(d.copy_pcm(view.first,view.first+view.count));const auto& b=whole.find("data")->data;
                    const int width=std::max(1,int(r.right)),height=int(r.bottom);
                    RECT selection{view.pixel_at(s->begin,width),0,view.pixel_at(s->end,width),height};
                    const auto selectionBrush=CreateSolidBrush(RGB(255,0,0));FillRect(dc,&selection,selectionBrush);DeleteObject(selectionBrush);
                    const auto pen=CreatePen(PS_SOLID,1,RGB(0,0,0));const auto previous=SelectObject(dc,pen);
                    const auto sample=[&](std::uint64_t frame,unsigned channel){
                        const auto p=size_t((frame-view.first)*f.blockAlign+channel*(f.bits/8));std::int64_t value=0;
                        if(f.bits==8)value=int(b[p])-128;
                        else{for(unsigned j=0;j<f.bits/8;++j)value|=std::int64_t(b[p+j])<<(j*8);if(value&(std::int64_t(1)<<(f.bits-1)))value-=std::int64_t(1)<<f.bits;}
                        return double(value)/double(std::uint64_t(1)<<(f.bits-1));
                    };
                    for(unsigned channel=0;channel<f.channels;++channel){
                        const int top=height*channel/f.channels,bottom=height*(channel+1)/f.channels,center=(top+bottom)/2,extent=std::max(1,(bottom-top)/2-2);
                        for(int x=0;x<width;++x){
                            const auto a=std::min(view.frame_at(x,width),view.first+view.count-1);
                            if(view.count<static_cast<unsigned>(width)){
                                const int y=center-int(sample(a,channel)*extent);
                                if(!x)MoveToEx(dc,x,y,nullptr);else LineTo(dc,x,y);
                            }else{
                                const auto end=std::max(a+1,view.frame_at(x+1,width));double low=1,high=-1;
                                for(auto frame=a;frame<end;++frame){const auto value=sample(frame,channel);low=std::min(low,value);high=std::max(high,value);}
                                MoveToEx(dc,x,center-int(high*extent),nullptr);LineTo(dc,x,center-int(low*extent)+1);
                            }
                        }
                    }
                    SelectObject(dc,previous);DeleteObject(pen);
                    const auto line=[&](std::uint64_t frame,COLORREF color,int thickness){
                        if(frame<view.first||frame>view.first+view.count)return;
                        const auto marker=CreatePen(PS_SOLID,thickness,color);const auto old=SelectObject(dc,marker);
                        const int x=view.pixel_at(frame,width);MoveToEx(dc,x,0,nullptr);LineTo(dc,x,height);SelectObject(dc,old);DeleteObject(marker);
                    };
                    line(s->insertion,RGB(220,0,0),1);
                    const auto loops=d.sample_loops();
                    if(s->selectedLoop<loops.size()){const auto& loop=loops[s->selectedLoop];line(loop.start,RGB(0,160,0),2);line(std::uint64_t(loop.start)+loop.length,RGB(0,0,220),2);}
                }
            }
            EndPaint(w,&paint);return 0;
        }
    }catch(const std::exception& e){MessageBoxA(GetParent(w),e.what(),"Wave selection failed",MB_OK|MB_ICONERROR);}
    return DefWindowProcW(w,msg,wp,lp);
}

void refresh(HWND w,Session& s){SendDlgItemMessageW(w,Documents,CB_RESETCONTENT,0,0);for(const auto& e:s.host.waves()){const auto label=(e.document->name().empty()?std::filesystem::path(e.path).filename().wstring():e.document->name())+(e.document->dirty()?L" *":L"");SendDlgItemMessageW(w,Documents,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}const bool present=!s.host.waves().empty();for(const auto id:{Save,Name,Root,Apply,Undo,Redo,Loops,LoopType,LoopStart,LoopLength,ApplyLoop,RemoveLoop,DisableLoops,SelectionBegin,SelectionEnd,Insertion,CopyPcm,CutPcm,DeletePcm,PastePcm,SelectAll})EnableWindow(GetDlgItem(w,id),present);if(!present){update_view(GetDlgItem(w,Canvas),s);SetWindowTextW(GetDlgItem(w,Status),L"Open a Wave document.");return;}if(s.selected>=s.host.waves().size())s.selected=s.host.waves().size()-1;SendDlgItemMessageW(w,Documents,CB_SETCURSEL,s.selected,0);const auto& d=s.host.wave_document(s.selected);SetWindowTextW(GetDlgItem(w,Name),d.name().c_str());SetWindowTextW(GetDlgItem(w,Root),std::to_wstring(d.root_note()).c_str());const auto values=d.sample_loops();s.selectedLoop=std::min(s.selectedLoop,values.size());SendDlgItemMessageW(w,Loops,CB_RESETCONTENT,0,0);for(size_t i=0;i<values.size();++i){const auto label=L"Loop "+std::to_wstring(i+1);SendDlgItemMessageW(w,Loops,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendDlgItemMessageW(w,Loops,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"New loop"));SendDlgItemMessageW(w,Loops,CB_SETCURSEL,s.selectedLoop,0);const auto l=s.selectedLoop<values.size()?values[s.selectedLoop]:WaveSampleLoop{};SetDlgItemTextW(w,LoopType,std::to_wstring(l.type).c_str());SetDlgItemTextW(w,LoopStart,std::to_wstring(l.start).c_str());SetDlgItemTextW(w,LoopLength,std::to_wstring(l.length).c_str());EnableWindow(GetDlgItem(w,RemoveLoop),s.selectedLoop<values.size());EnableWindow(GetDlgItem(w,DisableLoops),!values.empty());const auto f=d.format();s.begin=std::min(s.begin,f.frames);s.end=std::min(s.end,f.frames);s.insertion=std::min(s.insertion,f.frames);selection_fields(w,s);update_view(GetDlgItem(w,Canvas),s);const auto description=(d.dirty()?L"Modified. ":L"Saved. ")+std::to_wstring(f.channels)+L" channels, "+std::to_wstring(f.sampleRate)+L" Hz, "+std::to_wstring(f.bits)+L" bits, "+std::to_wstring(f.frames)+L" frames. Default loops: "+std::to_wstring(values.size())+L". Save Wave, then Save Project.";SetWindowTextW(GetDlgItem(w,Status),description.c_str());}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{if(msg==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,16,560,180,Documents);control(w,L"BUTTON",L"Open...",WS_TABSTOP,16,56,130,28,Open);control(w,L"BUTTON",L"Save As...",WS_TABSTOP,156,56,130,28,Save);control(w,L"BUTTON",L"Undo",WS_TABSTOP,296,56,130,28,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,436,56,130,28,Redo);control(w,L"STATIC",L"Wave name",0,16,100,200,22);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,16,126,550,26,Name);control(w,L"STATIC",L"Root note (MIDI 0-127)",0,16,164,300,22);control(w,L"EDIT",L"60",WS_BORDER|WS_TABSTOP,16,190,180,26,Root);control(w,L"BUTTON",L"Apply",WS_TABSTOP,216,190,150,28,Apply);control(w,L"STATIC",L"Default WSMP sample loops (0 forward, 1 loop and release)",0,16,220,550,24);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,246,550,150,Loops);control(w,L"STATIC",L"Type",0,16,282,150,22);control(w,L"STATIC",L"Start sample",0,196,282,170,22);control(w,L"STATIC",L"Length in samples",0,386,282,180,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,308,150,26,LoopType);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,196,308,170,26,LoopStart);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,386,308,180,26,LoopLength);control(w,L"BUTTON",L"Apply loop",WS_TABSTOP,16,348,170,28,ApplyLoop);control(w,L"BUTTON",L"Remove loop",WS_TABSTOP,206,348,170,28,RemoveLoop);control(w,L"BUTTON",L"Disable all loops",WS_TABSTOP,396,348,170,28,DisableLoops);control(w,L"SourceProducerWaveCanvas",L"",WS_BORDER|WS_TABSTOP|WS_CLIPCHILDREN,16,386,550,100,Canvas);control(w,L"STATIC",L"Selection start / exclusive end / insertion frame",0,16,500,550,24);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,530,170,26,SelectionBegin);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,206,530,170,26,SelectionEnd);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,396,530,170,26,Insertion);control(w,L"BUTTON",L"Copy PCM",WS_TABSTOP,16,570,100,28,CopyPcm);control(w,L"BUTTON",L"Cut PCM",WS_TABSTOP,128,570,100,28,CutPcm);control(w,L"BUTTON",L"Delete PCM",WS_TABSTOP,240,570,100,28,DeletePcm);control(w,L"BUTTON",L"Paste PCM",WS_TABSTOP,352,570,100,28,PastePcm);control(w,L"BUTTON",L"Select All",WS_TABSTOP,464,570,100,28,SelectAll);control(w,L"STATIC",L"",0,16,610,550,70,Status);refresh(w,*s);return 0;}if(msg==WM_COMMAND&&s){const auto id=LOWORD(wp);if(id==Documents&&HIWORD(wp)==CBN_SELCHANGE){s->selected=static_cast<size_t>(SendDlgItemMessageW(w,Documents,CB_GETCURSEL,0,0));refresh(w,*s);return 0;}if(id==Loops&&HIWORD(wp)==CBN_SELCHANGE){s->selectedLoop=static_cast<size_t>(SendDlgItemMessageW(w,Loops,CB_GETCURSEL,0,0));refresh(w,*s);return 0;}if(HIWORD(wp)!=BN_CLICKED)return 0;if(id==SnapZero){snapToZero=!snapToZero;selection_fields(w,*s);return 0;}if(id==SetLoopSelection){read_selection(w,*s);if(s->begin==s->end)throw std::runtime_error("Select a nonempty region for the loop");if(s->end>UINT32_MAX)throw std::runtime_error("Loop position exceeds capacity");auto loops=s->host.wave_document(s->selected).sample_loops();const WaveSampleLoop value{s->selectedLoop<loops.size()?loops[s->selectedLoop].type:0,static_cast<std::uint32_t>(s->begin),static_cast<std::uint32_t>(s->end-s->begin)};if(s->selectedLoop<loops.size())loops[s->selectedLoop]=value;else{s->selectedLoop=loops.size();loops.push_back(value);}s->host.wave_document(s->selected).set_sample_loops(loops);refresh(w,*s);return 0;}if(id==FindBestLoopEnd){read_selection(w,*s,false);s->host.wave_document(s->selected).find_best_sample_loop_end(s->selectedLoop,s->begin,s->end);refresh(w,*s);return 0;}if(id==CloneLoop){s->host.wave_document(s->selected).clone_sample_loop(s->selectedLoop);refresh(w,*s);return 0;}if(id==Open){const auto p=file(w,false);if(p.empty())return 0;s->selected=s->host.open_wave(p);}else if(id==Save){const auto p=file(w,true);if(p.empty())return 0;s->host.save_wave(s->selected,p);}else if(id==Undo)s->host.wave_document(s->selected).undo();else if(id==Redo)s->host.wave_document(s->selected).redo();else if(id==CopyPcm||id==CutPcm||id==DeletePcm||id==PastePcm||id==SelectAll){auto& d=s->host.wave_document(s->selected);if(id==SelectAll){s->begin=0;s->end=d.format().frames;}else{read_selection(w,*s);if(id==PastePcm)d.paste_pcm(s->insertion,get_wave_clipboard(w));else if(id==DeletePcm)d.erase_pcm(s->begin,s->end);else{const auto bytes=d.copy_pcm(s->begin,s->end);auto next=d;if(id==CutPcm)next.erase_pcm(s->begin,s->end);put_wave_clipboard(w,bytes);if(id==CutPcm)d=std::move(next);}}}else if(id==ApplyLoop||id==RemoveLoop||id==DisableLoops){auto loops=s->host.wave_document(s->selected).sample_loops();if(id==DisableLoops)loops.clear();else if(id==RemoveLoop){if(s->selectedLoop>=loops.size())throw std::runtime_error("Select an existing sample loop");s->host.wave_document(s->selected).remove_sample_loop(s->selectedLoop);refresh(w,*s);return 0;}else{auto number=[&](UINT field){const auto value=text(w,field);if(value.empty()||value.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Sample loop fields require nonnegative DWORD integers");const auto n=std::stoull(value);if(n>UINT32_MAX)throw std::runtime_error("Sample loop field exceeds DWORD");return static_cast<std::uint32_t>(n);};const WaveSampleLoop value{number(LoopType),number(LoopStart),number(LoopLength)};if(s->selectedLoop<loops.size())loops[s->selectedLoop]=value;else if(s->selectedLoop==loops.size())loops.push_back(value);else throw std::runtime_error("Sample loop selection changed");}s->host.wave_document(s->selected).set_sample_loops(loops);}else if(id==Apply){const auto input=text(w,Root);if(input.empty()||input.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Root note requires an integer 0-127");const auto value=std::stoul(input);if(value>127)throw std::runtime_error("Root note requires 0-127");auto next=s->host.wave_document(s->selected);const auto name=text(w,Name);if(name!=next.name()&&!next.set_name(name))throw std::runtime_error("Wave name is not representable in this document encoding");if(value!=next.root_note()&&!next.set_root_note(value))throw std::runtime_error("Root note edit rejected");if(!s->host.wave_document(s->selected).set_properties(name,static_cast<unsigned>(value)))throw std::runtime_error("Wave edit unchanged or invalid");}else return 0;refresh(w,*s);return 0;}if(msg==WM_CLOSE){DestroyWindow(w);return 0;}}catch(const std::exception& e){MessageBoxA(w,e.what(),"Wave document operation failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,msg,wp,lp);}
}
void show_wave_documents(HWND parent,Framework& host,size_t initialIndex){const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW canvas{};canvas.hInstance=instance;canvas.lpfnWndProc=waveform;canvas.hCursor=LoadCursorW(nullptr,IDC_CROSS);canvas.lpszClassName=L"SourceProducerWaveCanvas";if(!RegisterClassW(&canvas))throw std::runtime_error("Waveform class registration failed");WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerWaveDocuments";if(!RegisterClassW(&c))throw std::runtime_error("Wave document class registration failed");registered=true;}Session s{host};s.selected=initialIndex;const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerWaveDocuments",L"Wave Documents",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,615,730,nullptr,nullptr,instance,&s);if(!w)throw std::runtime_error("Wave document window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(!result)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Wave document message loop failed");}
}
