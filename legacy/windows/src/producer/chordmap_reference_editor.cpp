#include "chordmap_reference_editor.h"
#include <algorithm>
#include <stdexcept>
namespace producer::app {namespace {
enum:UINT {Track=10,SelectTrack,Maps,Time,Set,Clear,Undo,Redo,Status};
struct Session {Framework& host;size_t segment,track=0;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("ChordMap reference control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
unsigned number(HWND w,UINT id){wchar_t text[32]{};GetDlgItemTextW(w,id,text,32);const std::wstring v=text;if(v.empty()||!std::all_of(v.begin(),v.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';})||std::stoull(v)>INT32_MAX)throw std::runtime_error("Enter a nonnegative music clock or track number");return static_cast<unsigned>(std::stoull(v));}
void refresh(HWND w,Session& s){
    const auto refs=s.host.document(s.segment).chordmap_references(s.track);SetDlgItemTextW(w,Track,std::to_wstring(s.track+1).c_str());if(refs.size()==1)SetDlgItemTextW(w,Time,std::to_wstring(refs[0].time).c_str());
    EnableWindow(GetDlgItem(w,Set),refs.size()<=1&&!s.host.chordmaps().empty());EnableWindow(GetDlgItem(w,Clear),refs.size()==1);
    auto text=std::wstring(s.host.document(s.segment).dirty()?L"Modified. ":L"Saved. ")+L"References: "+std::to_wstring(refs.size());
    if(refs.size()==1)text+=L" — "+refs[0].name;
    if(refs.size()>1)text+=L". Imported multiple references are preserved; single-reference replacement is disabled.";
    text+=L"\nSave Segment and Project from the main File menu.";SetDlgItemTextW(w,Status,text.c_str());
}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
 if(msg==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
 control(w,L"STATIC",L"ChordMap reference track in current group (1-based)",0,16,16,484,22);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,44,170,26,Track);control(w,L"BUTTON",L"Select track",WS_TABSTOP,206,44,294,28,SelectTrack);
 control(w,L"STATIC",L"Owned ChordMap",0,16,92,484,22);auto maps=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,120,484,240,Maps);
 for(const auto& o:s->host.chordmaps()){const auto n=o.document->name()+L" — "+(o.path.empty()?L"Unsaved":std::filesystem::path(o.path).filename().wstring());SendMessageW(maps,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(n.c_str()));}if(!s->host.chordmaps().empty())SendMessageW(maps,CB_SETCURSEL,0,0);
 control(w,L"STATIC",L"Music clocks",0,16,168,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,196,484,26,Time);
 control(w,L"BUTTON",L"Set reference",WS_TABSTOP,16,246,232,30,Set);control(w,L"BUTTON",L"Clear reference",WS_TABSTOP,268,246,232,30,Clear);
 control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,298,232,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,268,298,232,30,Redo);control(w,L"STATIC",L"",0,16,348,484,94,Status);refresh(w,*s);return 0;
 }
 if(msg==WM_COMMAND&&s){switch(LOWORD(wp)){
 case SelectTrack:{auto n=number(w,Track);if(!n)throw std::runtime_error("Track numbers start at 1");(void)s->host.document(s->segment).chordmap_references(n-1);s->track=n-1;break;}
 case Set:{auto m=SendDlgItemMessageW(w,Maps,CB_GETCURSEL,0,0);if(m<0)throw std::runtime_error("Select an owned ChordMap");if(!s->host.assign_chordmap_reference(s->segment,static_cast<size_t>(m),number(w,Time),s->track))throw std::runtime_error("Reference unchanged or outside Segment");break;}
 case Clear:s->host.document(s->segment).clear_chordmap_reference(s->track);break;
 case Undo:s->host.undo_segment(s->segment);break;case Redo:s->host.redo_segment(s->segment);break;default:return 0;}refresh(w,*s);return 0;}
 }catch(const std::exception& e){const std::string t=e.what();const std::wstring v(t.begin(),t.end());MessageBoxW(w,v.c_str(),L"ChordMap reference edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,msg,wp,lp);
}}
void show_chordmap_reference_editor(HWND parent,Framework& host,size_t segment){
 const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerChordMapReferences";if(!RegisterClassW(&c))throw std::runtime_error("ChordMap reference window registration failed");registered=true;}
 Session s{host,segment};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerChordMapReferences",L"Segment ChordMap References",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,0,540,500,nullptr,nullptr,instance,&s);if(!w)throw std::runtime_error("ChordMap reference window creation failed");bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("ChordMap reference message loop failed");
}}
