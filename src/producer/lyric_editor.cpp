#include "lyric_editor.h"
#include <algorithm>
#include <stdexcept>
namespace producer::app {
namespace {
enum : UINT {Track=10,Apply,Events,Physical,Logical,Timing,Text,Add,Change,Delete,Copy,Paste,Undo,Redo,Status};
constexpr unsigned delivery[]={4,8,16,12,20,24,28};
struct Session {Framework& host;size_t index;LyricEditorContext& context;
    SegmentDocument& doc(){return host.document(index);}
    size_t& track(){return context.selection.tracks[doc().selected_groups()];}
    size_t& selected(){return context.selection.events[{doc().selected_groups(),track()}];}
};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Lyric control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
std::wstring text(HWND w,UINT id){auto h=GetDlgItem(w,id);std::wstring out(static_cast<size_t>(GetWindowTextLengthW(h))+1,L'\0');GetWindowTextW(h,out.data(),static_cast<int>(out.size()));out.resize(out.size()-1);return out;}
unsigned number(HWND w,UINT id){size_t end=0;const auto s=text(w,id);auto n=std::stoll(s,&end);if(end!=s.size()||n<0||n>INT32_MAX)throw std::runtime_error("Use nonnegative clocks or track numbers");return static_cast<unsigned>(n);}
void refresh(HWND w,Session& s){const auto events=s.doc().lyrics(s.track());SetDlgItemTextW(w,Track,std::to_wstring(s.track()+1).c_str());auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<events.size();++i){const auto& e=events[i];const auto label=std::to_wstring(i+1)+L": "+e.text+L"; start "+std::to_wstring(e.physical)+L", belongs to "+std::to_wstring(e.logical);SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    LyricEvent e;if(!events.empty()){s.selected()=std::min(s.selected(),events.size()-1);SendMessageW(list,CB_SETCURSEL,s.selected(),0);e=events[s.selected()];}
    SetDlgItemTextW(w,Physical,std::to_wstring(e.physical).c_str());SetDlgItemTextW(w,Logical,std::to_wstring(e.logical).c_str());SetDlgItemTextW(w,Text,e.text.c_str());const auto choice=std::find(std::begin(delivery),std::end(delivery),e.timing);SendDlgItemMessageW(w,Timing,CB_SETCURSEL,choice-std::begin(delivery),0);
    for(auto id:{Change,Delete,Copy})EnableWindow(GetDlgItem(w,id),!events.empty());EnableWindow(GetDlgItem(w,Paste),!s.context.clipboard.empty());
    const auto status=std::wstring(s.doc().dirty()?L"Modified. ":L"Saved. ")+std::to_wstring(events.size())+L" lyrics. Save the Segment or Project from the main File menu.";SetDlgItemTextW(w,Status,status.c_str());
}
LyricEvent input(HWND w){auto timing=SendDlgItemMessageW(w,Timing,CB_GETCURSEL,0,0);if(timing<0||timing>=7)throw std::runtime_error("Choose a Lyric delivery setting");return {static_cast<std::int32_t>(number(w,Physical)),static_cast<std::int32_t>(number(w,Logical)),delivery[timing],text(w,Text)};}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"STATIC",L"Lyric track in current group selection (1-based)",0,16,16,500,22);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,44,170,26,Track);control(w,L"BUTTON",L"Select track",WS_TABSTOP,206,44,294,28,Apply);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,92,484,240,Events);
        control(w,L"STATIC",L"Start time (clocks)",0,16,138,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,164,484,26,Physical);
        control(w,L"STATIC",L"Belongs to (clocks)",0,16,202,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,228,484,26,Logical);
        control(w,L"STATIC",L"Tool delivery",0,16,266,484,22);auto timings=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,292,484,260,Timing);
        for(auto label:{L"Quick response",L"Before Time Stamp",L"At Time Stamp",L"Quick response + Before Time Stamp",L"Quick response + At Time Stamp",L"Before + At Time Stamp",L"All delivery times"})SendMessageW(timings,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
        control(w,L"STATIC",L"Lyric",0,16,330,484,22);auto edit=control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,16,356,484,28,Text);SendMessageW(edit,EM_SETLIMITTEXT,0,0);
        control(w,L"BUTTON",L"Add",WS_TABSTOP,16,402,148,30,Add);control(w,L"BUTTON",L"Change",WS_TABSTOP,184,402,148,30,Change);control(w,L"BUTTON",L"Delete",WS_TABSTOP,352,402,148,30,Delete);
        control(w,L"BUTTON",L"Copy",WS_TABSTOP,16,448,232,30,Copy);control(w,L"BUTTON",L"Paste at start / belongs to",WS_TABSTOP,268,448,232,30,Paste);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,494,232,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,268,494,232,30,Redo);control(w,L"STATIC",L"",0,16,540,484,60,Status);refresh(w,*s);return 0;
    }
    if(message==WM_COMMAND&&s){size_t after=0;switch(LOWORD(wp)){
        case Events:if(HIWORD(wp)==CBN_SELCHANGE){s->selected()=static_cast<size_t>(SendDlgItemMessageW(w,Events,CB_GETCURSEL,0,0));refresh(w,*s);}return 0;
        case Apply:{const auto n=number(w,Track);if(!n)throw std::runtime_error("Track numbers start at 1");(void)s->doc().lyrics(n-1);s->track()=n-1;s->selected()=0;break;}
        case Add:if(!s->doc().add_lyric(input(w),s->track(),&after))throw std::runtime_error("Invalid Lyric input");s->selected()=after;break;
        case Change:if(!s->doc().edit_lyric(s->selected(),input(w),s->track(),&after))throw std::runtime_error("Lyric unchanged or invalid");s->selected()=after;break;
        case Delete:s->doc().delete_lyric(s->selected(),s->track());break;
        case Copy:s->context.clipboard=s->doc().copy_lyric(s->selected(),s->track());break;
        case Paste:if(!s->doc().paste_lyric(s->context.clipboard,static_cast<std::int32_t>(number(w,Physical)),static_cast<std::int32_t>(number(w,Logical)),s->track(),&after))throw std::runtime_error("Invalid Lyric paste");s->selected()=after;break;
        case Undo:s->doc().undo();break;case Redo:s->doc().redo();break;default:return 0;
    }refresh(w,*s);return 0;}
}catch(const std::exception& e){const std::string t=e.what();const std::wstring msg(t.begin(),t.end());MessageBoxW(w,msg.c_str(),L"Lyric edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);}
}
void show_lyric_editor(HWND parent,Framework& host,size_t index,LyricEditorContext& context){(void)host.document(index).lyrics(context.selection.tracks[host.document(index).selected_groups()]);const auto instance=GetModuleHandleW(nullptr);static bool registered=false;
    if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerLyrics";if(!RegisterClassW(&c))throw std::runtime_error("Lyric window registration failed");registered=true;}
    Session session{host,index,context};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerLyrics",L"Segment Lyrics",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,540,650,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("Lyric window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Lyric message loop failed");
}
}
