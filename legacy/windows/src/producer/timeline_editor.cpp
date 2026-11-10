#include "timeline_editor.h"
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cstring>
#include <utility>
namespace producer::app {namespace {
enum:UINT{Begin=10,End,At,Tempo,Sequence,Select,Copy,Cut,Delete,Merge,Overwrite,Undo,Redo,Status,Move,MoveDrag,Lyric,LyricTrack,Marker,Mute,MarkerTrack,MuteTrack,SelectAll};
struct Session{SegmentDocument& doc;UINT format=0;int drag=-1;bool moving=false;};
void control(HWND w,const wchar_t* cls,const wchar_t* title,DWORD flags,int x,int y,int width,int height,UINT id=0){CreateWindowW(cls,title,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);}
std::int32_t number(HWND w,UINT id){wchar_t text[64]{};GetWindowTextW(GetDlgItem(w,id),text,64);wchar_t* end=nullptr;const auto value=wcstoll(text,&end,10);if(end==text||*end||value<0||value>INT32_MAX)throw std::runtime_error("Enter a nonnegative clock within LONG range");return static_cast<std::int32_t>(value);}
void set_number(HWND w,UINT id,std::int32_t value){SetWindowTextW(GetDlgItem(w,id),std::to_wstring(value).c_str());}
void selection(HWND w,Session& s){
    TimelineSelection value{number(w,Begin),number(w,End),0};
    for(auto item:{std::pair<UINT,std::uint32_t>{Tempo,TimelineTempo},{Sequence,TimelineSequence},{Lyric,TimelineLyric},{Marker,TimelineMarker},{Mute,TimelineMute}})if(SendDlgItemMessageW(w,item.first,BM_GETCHECK,0,0)==BST_CHECKED)value.strips|=item.second;
    const auto index=[&](UINT id){const auto n=number(w,id);if(!n)throw std::runtime_error("Track numbers are 1-based");return static_cast<size_t>(n-1);};
    value.lyricTrack=index(LyricTrack);value.markerTrack=index(MarkerTrack);value.muteTrack=index(MuteTrack);
    if(!s.doc.select_range(value))throw std::runtime_error("Select strips and a nonempty range inside this Segment");
}
void refresh(HWND w,Session& s){
    auto value=s.doc.selected_range();set_number(w,Begin,value.begin);set_number(w,End,value.end);
    for(auto item:{std::pair<UINT,std::uint32_t>{Tempo,TimelineTempo},{Sequence,TimelineSequence},{Lyric,TimelineLyric},{Marker,TimelineMarker},{Mute,TimelineMute}})SendDlgItemMessageW(w,item.first,BM_SETCHECK,value.strips&item.second?BST_CHECKED:BST_UNCHECKED,0);
    set_number(w,LyricTrack,static_cast<std::int32_t>(value.lyricTrack+1));set_number(w,MarkerTrack,static_cast<std::int32_t>(value.markerTrack+1));set_number(w,MuteTrack,static_cast<std::int32_t>(value.muteTrack+1));
    auto status=L"Range ["+std::to_wstring(value.begin)+L", "+std::to_wstring(value.end)+L") clocks; Tempo "+std::to_wstring(s.doc.tempos().size())+L"; notes "+std::to_wstring(s.doc.notes().size())+L"; lyrics "+std::to_wstring(s.doc.lyrics((value.strips&TimelineLyric)?value.lyricTrack:0).size())+L"; markers "+std::to_wstring(s.doc.markers((value.strips&TimelineMarker)?value.markerTrack:0).size())+L"; mutes "+std::to_wstring(s.doc.mutes((value.strips&TimelineMute)?value.muteTrack:0).size())+(s.doc.dirty()?L"; modified":L"; saved");
    SetWindowTextW(GetDlgItem(w,Status),status.c_str());InvalidateRect(w,nullptr,TRUE);
}
void copy(HWND w,Session& s){const auto bytes=s.doc.copy_range();auto memory=GlobalAlloc(GMEM_MOVEABLE,bytes.size());if(!memory)throw std::runtime_error("Clipboard allocation failed");auto data=GlobalLock(memory);if(!data){GlobalFree(memory);throw std::runtime_error("Clipboard lock failed");}memcpy(data,bytes.data(),bytes.size());GlobalUnlock(memory);if(!OpenClipboard(w)){GlobalFree(memory);throw std::runtime_error("Clipboard is busy");}const bool ok=EmptyClipboard()&&SetClipboardData(s.format,memory);CloseClipboard();if(!ok){GlobalFree(memory);throw std::runtime_error("Clipboard copy failed");}}
Bytes clipboard(HWND w,Session& s){if(!OpenClipboard(w))throw std::runtime_error("Clipboard is busy");Bytes result;try{auto memory=GetClipboardData(s.format);auto size=memory?GlobalSize(memory):0;if(size<12||size>64*1024*1024)throw std::runtime_error("No source Timeline range on clipboard");auto data=GlobalLock(memory);if(!data)throw std::runtime_error("Clipboard lock failed");result.assign(static_cast<const std::uint8_t*>(data),static_cast<const std::uint8_t*>(data)+size);GlobalUnlock(memory);const auto count=std::uint64_t(read32(result,4))+8;if(count>result.size())throw std::runtime_error("Truncated Timeline clipboard");result.resize(static_cast<size_t>(count));CloseClipboard();return result;}catch(...){CloseClipboard();throw;}}
std::int32_t clock_at(Session& s,int x){return static_cast<std::int32_t>(std::int64_t(std::max(20,std::min(720,x))-20)*s.doc.length()/700);}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){
        s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        s->format=RegisterClipboardFormatW(L"Producer.Source.TimelineRange.v1");if(!s->format)throw std::runtime_error("Clipboard registration failed");
        control(w,L"STATIC",L"Drag to select. Ctrl+C copies, Ctrl+V merges, Ctrl+A selects all, Ctrl+Z/Y undo/redo.",0,20,16,710,24);
        control(w,L"BUTTON",L"Tempo",BS_AUTOCHECKBOX|WS_TABSTOP,20,55,130,25,Tempo);
        control(w,L"BUTTON",L"Sequence / controllers",BS_AUTOCHECKBOX|WS_TABSTOP,170,55,250,25,Sequence);
        control(w,L"BUTTON",L"Lyric",BS_AUTOCHECKBOX|WS_TABSTOP,430,55,110,25,Lyric);
        control(w,L"BUTTON",L"Move by drag",BS_AUTOCHECKBOX|WS_TABSTOP,550,55,170,25,MoveDrag);
        control(w,L"BUTTON",L"Marker / Enter",BS_AUTOCHECKBOX|WS_TABSTOP,20,90,190,25,Marker);
        control(w,L"BUTTON",L"Mute / remap",BS_AUTOCHECKBOX|WS_TABSTOP,230,90,200,25,Mute);
        int row=130;for(auto item:{std::pair<UINT,const wchar_t*>{LyricTrack,L"Lyric track (1-based, selected groups)"},{MarkerTrack,L"Marker track (1-based, selected groups)"},{MuteTrack,L"Mute track (1-based, selected groups)"}}){control(w,L"STATIC",item.second,0,20,row,330,24);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,360,row,140,25,item.first);row+=35;}
        control(w,L"STATIC",L"Begin",0,20,260,65,24);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,85,260,140,25,Begin);
        control(w,L"STATIC",L"End (exclusive)",0,245,260,130,24);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,380,260,140,25,End);control(w,L"BUTTON",L"Select Range",WS_TABSTOP,540,258,180,28,Select);
        control(w,L"STATIC",L"Target at",0,20,400,70,24);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,140,400,140,25,At);
        control(w,L"BUTTON",L"Paste Merge",WS_TABSTOP,300,398,190,30,Merge);control(w,L"BUTTON",L"Paste Overwrite",WS_TABSTOP,500,398,220,30,Overwrite);
        int x=20;for(auto item:{std::pair<UINT,const wchar_t*>{Copy,L"Copy"},{Cut,L"Cut"},{Delete,L"Delete"},{Undo,L"Undo"},{Redo,L"Redo"}}){control(w,L"BUTTON",item.second,WS_TABSTOP,x,445,130,30,item.first);x+=140;}
        control(w,L"BUTTON",L"Move Selection",WS_TABSTOP,20,495,200,30,Move);control(w,L"STATIC",L"",0,260,495,480,60,Status);
        if(!s->doc.selected_range().strips)s->doc.select_range({0,s->doc.length(),3});refresh(w,*s);return 0;
    }
    if(message==WM_PAINT&&s){
        PAINTSTRUCT paint{};auto dc=BeginPaint(w,&paint);RECT bar{20,315,720,380};FillRect(dc,&bar,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));FrameRect(dc,&bar,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));auto value=s->doc.selected_range();
        if(s->doc.length()>0){RECT range{20+static_cast<int>(std::int64_t(value.begin)*700/s->doc.length()),316,20+static_cast<int>(std::int64_t(value.end)*700/s->doc.length()),379};FillRect(dc,&range,reinterpret_cast<HBRUSH>(COLOR_HIGHLIGHT+1));
            const auto tick=[&](std::int32_t time,int top,int bottom){const int x=20+static_cast<int>(std::int64_t(time)*700/s->doc.length());MoveToEx(dc,x,top,nullptr);LineTo(dc,x,bottom);};
            for(const auto& e:s->doc.tempos())if(e.bpm>0)tick(e.time,317,329);
            for(const auto& e:s->doc.notes())tick(e.time,331,343);
            for(const auto& e:s->doc.lyrics((value.strips&TimelineLyric)?value.lyricTrack:0))tick(e.physical,345,356);
            for(const auto& e:s->doc.markers((value.strips&TimelineMarker)?value.markerTrack:0))tick(e.time,358,367);
            for(const auto& e:s->doc.mutes((value.strips&TimelineMute)?value.muteTrack:0))tick(e.time,371,378);
        }EndPaint(w,&paint);return 0;
    }
    if(message==WM_LBUTTONDOWN&&s){const int x=static_cast<short>(LOWORD(lp)),y=static_cast<short>(HIWORD(lp));if(x>=20&&x<=720&&y>=315&&y<=380){SetFocus(w);s->drag=clock_at(*s,x);const auto range=s->doc.selected_range();s->moving=((GetKeyState(VK_MENU)&0x8000)||SendDlgItemMessageW(w,MoveDrag,BM_GETCHECK,0,0)==BST_CHECKED)&&s->drag>=range.begin&&s->drag<range.end;SetCapture(w);}return 0;}
    if(message==WM_LBUTTONUP&&s&&s->drag>=0){auto end=clock_at(*s,static_cast<short>(LOWORD(lp)));auto begin=s->drag;s->drag=-1;const bool moving=s->moving;s->moving=false;ReleaseCapture();if(moving){const auto at=std::int64_t(s->doc.selected_range().begin)+end-begin;if(at<0||at>INT32_MAX||!s->doc.move_range(static_cast<std::int32_t>(at)))throw std::runtime_error("Move rejected: unchanged, empty, incompatible or outside Segment bounds");refresh(w,*s);return 0;}if(begin>end)std::swap(begin,end);set_number(w,Begin,begin);set_number(w,End,end);selection(w,*s);refresh(w,*s);return 0;}
    if(message==WM_COMMAND&&s&&HIWORD(wp)==BN_CLICKED){const auto id=LOWORD(wp);if(id==Tempo||id==Sequence||id==Lyric||id==Marker||id==Mute||id==MoveDrag)return 0;if(id==Undo)s->doc.undo();else if(id==Redo)s->doc.redo();else{if(id==SelectAll){set_number(w,Begin,0);set_number(w,End,s->doc.length());}selection(w,*s);if(id==Copy||id==Cut)copy(w,*s);if(id==Cut||id==Delete){if(!s->doc.delete_range())throw std::runtime_error("No data changed in this range");}else if(id==Move){if(!s->doc.move_range(number(w,At)))throw std::runtime_error("Move rejected: unchanged, empty, incompatible or outside Segment bounds");}else if(id==Merge||id==Overwrite){if(!s->doc.paste_range(clipboard(w,*s),number(w,At),id==Overwrite))throw std::runtime_error("Paste rejected: selection, range, record compatibility or Segment bounds");}else if(id!=Select&&id!=SelectAll&&id!=Copy)return 0;}refresh(w,*s);return 0;}
    if(message==WM_CAPTURECHANGED&&s){s->drag=-1;s->moving=false;return 0;}
    if(message==WM_CLOSE){DestroyWindow(w);return 0;}
}catch(const std::exception& e){MessageBoxA(w,e.what(),"Timeline range operation failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);}
bool keyboard_command(HWND window,const MSG& message){
    if(message.message!=WM_KEYDOWN||(message.hwnd!=window&&!IsChild(window,message.hwnd))||!(GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_MENU)&0x8000))return false;
    // Numeric fields keep native Edit clipboard, Select All and Undo behavior.
    wchar_t name[32]{};const auto focus=GetFocus();if(focus&&GetClassNameW(focus,name,32)&&_wcsicmp(name,L"Edit")==0)return false;
    UINT command=0;switch(message.wParam){case 'C':command=Copy;break;case 'V':command=Merge;break;case 'A':command=SelectAll;break;case 'Z':command=Undo;break;case 'Y':command=Redo;break;default:return false;}
    if(message.lParam&(1L<<30))return true; // One history operation per key press.
    SendMessageW(window,WM_COMMAND,MAKEWPARAM(command,BN_CLICKED),0);return true;
}
}
void show_timeline_range(HWND parent,SegmentDocument& document){static bool registered=false;auto instance=GetModuleHandleW(nullptr);if(!registered){WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerTimelineRange";if(!RegisterClassW(&c))throw std::runtime_error("Timeline range class registration failed");registered=true;}Session s{document};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerTimelineRange",L"Timeline Range",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,760,600,nullptr,nullptr,instance,&s);if(!w)throw std::runtime_error("Timeline window creation failed");bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);MSG message{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&message,nullptr,0,0))>0)if(!keyboard_command(w,message)&&!IsDialogMessageW(w,&message)){TranslateMessage(&message);DispatchMessageW(&message);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(!result)PostQuitMessage(static_cast<int>(message.wParam));if(result<0)throw std::runtime_error("Timeline message loop failed");}
}
