#include "style_reference_editor.h"
#include <algorithm>
#include <stdexcept>
namespace producer::app {
namespace {
enum : UINT {Track=10,SelectTrack,Events,Styles,Time,Add,Change,Delete,Undo,Redo,Status};
struct Session {Framework& host;size_t segment,track=0,event=0;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Style reference control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
unsigned number(HWND w,UINT id){wchar_t text[32]{};GetDlgItemTextW(w,id,text,32);const std::wstring value=text;if(value.empty()||!std::all_of(value.begin(),value.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';})||std::stoull(value)>INT32_MAX)throw std::runtime_error("Enter a nonnegative music clock or track number");return static_cast<unsigned>(std::stoull(value));}
void refresh(HWND w,Session& s){
    const auto refs=s.host.document(s.segment).style_references(s.track);SetDlgItemTextW(w,Track,std::to_wstring(s.track+1).c_str());const auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<refs.size();++i){const auto& r=refs[i];const auto label=std::to_wstring(i+1)+L": clocks "+std::to_wstring(r.time)+L" — "+(!r.name.empty()?r.name:!r.filename.empty()?r.filename:L"Style GUID");SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    if(!refs.empty()){s.event=std::min(s.event,refs.size()-1);SendMessageW(list,CB_SETCURSEL,s.event,0);SetDlgItemTextW(w,Time,std::to_wstring(refs[s.event].time).c_str());}
    for(auto id:{Change,Delete})EnableWindow(GetDlgItem(w,id),!refs.empty());EnableWindow(GetDlgItem(w,Add),!s.host.style_documents().empty());
    const auto text=std::wstring(s.host.document(s.segment).dirty()?L"Modified. ":L"Saved. ")+L"Save the Segment and Project from the main File menu. Style-derived meter starts at clock 0 and changes on measure boundaries.";SetDlgItemTextW(w,Status,text.c_str());
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"STATIC",L"Style reference track in current group (1-based)",0,16,16,500,22);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,44,170,26,Track);control(w,L"BUTTON",L"Select track",WS_TABSTOP,206,44,294,28,SelectTrack);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,92,484,240,Events);
        control(w,L"STATIC",L"Owned Style",0,16,138,484,22);const auto styles=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,164,484,240,Styles);
        for(const auto& owned:s->host.style_documents()){const auto name=owned.path.empty()?L"Untitled Style":std::filesystem::path(owned.path).filename().wstring();SendMessageW(styles,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));}if(!s->host.style_documents().empty())SendMessageW(styles,CB_SETCURSEL,0,0);
        control(w,L"STATIC",L"Music clocks",0,16,208,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,234,484,26,Time);
        control(w,L"BUTTON",L"Add",WS_TABSTOP,16,282,148,30,Add);control(w,L"BUTTON",L"Change",WS_TABSTOP,184,282,148,30,Change);control(w,L"BUTTON",L"Delete",WS_TABSTOP,352,282,148,30,Delete);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,334,232,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,268,334,232,30,Redo);control(w,L"STATIC",L"",0,16,386,484,90,Status);refresh(w,*s);return 0;
    }
    if(message==WM_COMMAND&&s){switch(LOWORD(wp)){
        case Events:if(HIWORD(wp)==CBN_SELCHANGE){s->event=static_cast<size_t>(SendDlgItemMessageW(w,Events,CB_GETCURSEL,0,0));refresh(w,*s);}return 0;
        case SelectTrack:{const auto n=number(w,Track);if(!n)throw std::runtime_error("Track numbers start at 1");(void)s->host.document(s->segment).style_references(n-1);s->track=n-1;s->event=0;break;}
        case Add:case Change:{const auto style=SendDlgItemMessageW(w,Styles,CB_GETCURSEL,0,0);if(style<0)throw std::runtime_error("Select an owned Style");if(!s->host.assign_style_reference(s->segment,static_cast<size_t>(style),static_cast<std::int32_t>(number(w,Time)),LOWORD(wp)==Change?std::optional<size_t>(s->event):std::nullopt,s->track))throw std::runtime_error("Style reference unchanged, duplicate or outside Segment");break;}
        case Delete:if(!s->host.delete_style_reference(s->segment,s->event,s->track))throw std::runtime_error("Select a Style reference");break;
        case Undo:s->host.undo_segment(s->segment);break;case Redo:s->host.redo_segment(s->segment);break;default:return 0;
    }refresh(w,*s);return 0;}
}catch(const std::exception& e){const std::string text=e.what();const std::wstring wide(text.begin(),text.end());MessageBoxW(w,wide.c_str(),L"Style reference edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);}
}
void show_style_reference_editor(HWND parent,Framework& host,size_t segment){
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=proc;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.lpszClassName=L"SourceProducerStyleReferences";if(!RegisterClassW(&cls))throw std::runtime_error("Style reference window registration failed");registered=true;}
    Session s{host,segment};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerStyleReferences",L"Segment Style References",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,540,520,nullptr,nullptr,instance,&s);if(!w)throw std::runtime_error("Style reference window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Style reference message loop failed");
}
}
