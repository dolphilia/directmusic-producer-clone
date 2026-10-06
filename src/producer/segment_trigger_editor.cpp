#include "segment_trigger_editor.h"
#include <stdexcept>
namespace producer::app {
namespace {
enum:UINT{Track=10,SelectTrack,Events,Sources,Logical,Physical,Flags,Add,Change,Delete,Undo,Redo,Status};
struct Source{size_t index;bool motif;std::wstring name;};
struct Session{Framework& host;size_t owner,track=0,event=0;std::vector<Source> sources;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Trigger control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
unsigned number(HWND w,UINT id){wchar_t text[64]{};GetDlgItemTextW(w,id,text,64);size_t end=0;std::wstring s=text;const auto n=std::stoull(s,&end,0);if(end!=s.size()||n>INT32_MAX||s.find(L'-')!=std::wstring::npos)throw std::runtime_error("Use nonnegative clocks or hexadecimal play flags");return static_cast<unsigned>(n);}
void refresh(HWND w,Session& s){const auto events=s.host.document(s.owner).triggers(s.track);const auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);for(const auto& e:events){const auto text=std::to_wstring(e.physical)+L" / "+std::to_wstring(e.logical)+L" : "+e.filename+(e.itemFlags&1?L" / "+e.motif:L"");SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    if(s.event>=events.size())s.event=events.empty()?0:events.size()-1;if(!events.empty()){SendMessageW(list,CB_SETCURSEL,s.event,0);const auto& e=events[s.event];SetDlgItemTextW(w,Logical,std::to_wstring(e.logical).c_str());SetDlgItemTextW(w,Physical,std::to_wstring(e.physical).c_str());SetDlgItemTextW(w,Flags,std::to_wstring(e.playFlags).c_str());
        for(size_t i=0;i<s.sources.size();++i){const auto& source=s.sources[i];if(source.motif!=(bool)(e.itemFlags&1)||(source.motif&&source.name!=e.motif))continue;
            const auto root=Chunk::parse(source.motif?s.host.style_document(source.index).save_bytes():s.host.document(source.index).save_bytes());const auto id=root.find("guid");
            if(e.hasId&&id&&id->data==Bytes(e.objectId.begin(),e.objectId.end())){SendDlgItemMessageW(w,Sources,CB_SETCURSEL,i,0);break;}}
    }
    const auto text=std::to_wstring(events.size())+L" triggers, group "+std::to_wstring(s.host.document(s.owner).selected_groups())+(s.host.document(s.owner).dirty()?L", modified":L", saved");SetDlgItemTextW(w,Status,text.c_str());}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=reinterpret_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"STATIC",L"Track (within selected group)",0,16,12,360,22);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,38,180,26,Track);control(w,L"BUTTON",L"Select Track",WS_TABSTOP,218,38,280,28,SelectTrack);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,80,484,240,Events);
        control(w,L"STATIC",L"Owned Segment or Style Motif",0,16,122,484,22);auto sources=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,148,484,250,Sources);
        for(size_t i=0;i<s->host.documents().size();++i)if(i!=s->owner){s->sources.push_back({i,false,{}});const auto text=std::filesystem::path(s->host.documents()[i].path).filename().wstring();SendMessageW(sources,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
        for(size_t i=0;i<s->host.style_documents().size();++i)for(const auto& p:s->host.style_document(i).patterns())if(p.embellishment&16){s->sources.push_back({i,true,p.name});const auto text=std::filesystem::path(s->host.style_documents()[i].path).filename().wstring()+L" / "+p.name;SendMessageW(sources,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
        if(!s->sources.empty())SendMessageW(sources,CB_SETCURSEL,0,0);
        control(w,L"STATIC",L"Belongs To (logical clocks)",0,16,190,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,214,484,26,Logical);
        control(w,L"STATIC",L"Start Time (physical clocks)",0,16,256,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,280,484,26,Physical);
        control(w,L"STATIC",L"Play flags: Secondary=0x80, Control=0x200, Beat=0x1000",0,16,322,484,22);control(w,L"EDIT",L"128",WS_BORDER|WS_TABSTOP,16,346,484,26,Flags);
        control(w,L"BUTTON",L"Add",WS_TABSTOP,16,396,148,30,Add);control(w,L"BUTTON",L"Change",WS_TABSTOP,184,396,148,30,Change);control(w,L"BUTTON",L"Delete",WS_TABSTOP,352,396,148,30,Delete);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,450,232,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,268,450,232,30,Redo);control(w,L"STATIC",L"",0,16,502,484,72,Status);refresh(w,*s);return 0;}
    if(message==WM_COMMAND&&s){switch(LOWORD(wp)){
        case Events:if(HIWORD(wp)==CBN_SELCHANGE){s->event=SendDlgItemMessageW(w,Events,CB_GETCURSEL,0,0);refresh(w,*s);}return 0;
        case SelectTrack:{const auto t=number(w,Track);if(!t)throw std::runtime_error("Track numbers start at 1");(void)s->host.document(s->owner).triggers(t-1);s->track=t-1;s->event=0;break;}
        case Add:case Change:{const auto n=SendDlgItemMessageW(w,Sources,CB_GETCURSEL,0,0);if(n<0)throw std::runtime_error("Select an owned source");const auto source=s->sources.at(n);if(!s->host.assign_segment_trigger(s->owner,source.index,source.motif,source.name,number(w,Logical),number(w,Physical),number(w,Flags),LOWORD(wp)==Change?std::optional<size_t>(s->event):std::nullopt,s->track))throw std::runtime_error("Trigger unchanged, invalid flags/position, or existing Project binding cannot be retargeted");if(LOWORD(wp)==Add)s->event=s->host.document(s->owner).triggers(s->track).size()-1;break;}
        case Delete:if(!s->host.document(s->owner).delete_trigger(s->event,s->track))throw std::runtime_error("Select a Trigger");break;
        case Undo:s->host.undo_segment(s->owner);break;case Redo:s->host.redo_segment(s->owner);break;default:return 0;}refresh(w,*s);return 0;}
}catch(const std::exception& e){std::string text=e.what();std::wstring wide(text.begin(),text.end());MessageBoxW(w,wide.c_str(),L"Segment Trigger edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);}
}
void show_segment_trigger_editor(HWND parent,Framework& host,size_t owner){const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=proc;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.lpszClassName=L"SourceProducerSegmentTriggers";if(!RegisterClassW(&cls))throw std::runtime_error("Trigger window registration failed");registered=true;}
    Session s{host,owner};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerSegmentTriggers",L"Segment Triggers",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,540,620,nullptr,nullptr,instance,&s);if(!w)throw std::runtime_error("Trigger window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Trigger message loop failed");}
}
