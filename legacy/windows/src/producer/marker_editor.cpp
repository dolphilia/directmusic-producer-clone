#include "marker_editor.h"
#include <algorithm>
#include <stdexcept>
#include <memory>
namespace producer::app {
namespace {
enum : UINT {Track=10,Apply,Events,Time,Kind,Add,Change,Delete,Copy,Paste,Undo,Redo,Status,RangeBegin,RangeEnd,Division,MarkRange,UnmarkRange,MarkAll,UnmarkAll,Properties};
HWND editorWindow=nullptr;
struct Session {Framework& host;size_t index;MarkerEditorContext& context;MarkerPropertiesChanged changed;bool updating=false,ownedByWindow=false;
    SegmentDocument& doc(){return host.document(index);}
    size_t& track(){return context.selection.tracks[doc().selected_groups()];}
    size_t& selected(){return context.selection.events[{doc().selected_groups(),track()}];}
    void publish(bool show=false){if(changed){if(doc().markers(track()).empty())changed(std::nullopt,show);else changed(MarkerPropertiesTarget{index,track(),selected(),doc().selected_groups()},show);}}
};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Marker control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
unsigned number(HWND w,UINT id){wchar_t text[64]{};GetDlgItemTextW(w,id,text,64);size_t end=0;const std::wstring s=text;auto n=std::stoll(s,&end);if(end!=s.size()||n<0||n>INT32_MAX)throw std::runtime_error("Use a nonnegative clock or track number");return static_cast<unsigned>(n);}
void refresh(HWND w,Session& s){struct Guard{bool& value;explicit Guard(bool& flag):value(flag){value=true;}~Guard(){value=false;}} guard(s.updating);const auto events=s.doc().markers(s.track());SetDlgItemTextW(w,Track,std::to_wstring(s.track()+1).c_str());auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<events.size();++i){const auto& e=events[i];const auto p=s.doc().timeline().position(e.time);const auto label=std::wstring(e.kind==MarkerKind::play?L"Marker":L"Enter SwitchPoint")+L" "+std::to_wstring(i+1)+L", clocks "+std::to_wstring(e.time)+L", measure "+std::to_wstring(p.measure+1)+L", beat "+std::to_wstring(p.beat+1);SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    MarkerEvent e;if(!events.empty()){s.selected()=std::min(s.selected(),events.size()-1);SendMessageW(list,CB_SETCURSEL,s.selected(),0);e=events[s.selected()];}
    SetDlgItemTextW(w,Time,std::to_wstring(e.time).c_str());SendDlgItemMessageW(w,Kind,CB_SETCURSEL,e.kind==MarkerKind::play?0:1,0);
    for(auto id:{Change,Delete,Copy,Properties})EnableWindow(GetDlgItem(w,id),!events.empty());EnableWindow(GetDlgItem(w,Paste),!s.context.clipboard.empty());EnableWindow(GetDlgItem(w,Undo),s.doc().can_undo());EnableWindow(GetDlgItem(w,Redo),s.doc().can_redo());
    const auto text=std::wstring(s.doc().dirty()?L"Modified. ":L"Saved. ")+std::to_wstring(events.size())+L" events. Exact clocks retain grid/tick positions; overlapping events are allowed. Save the Segment or Project from the main File menu.";SetDlgItemTextW(w,Status,text.c_str());
}
MarkerEvent input(HWND w){auto kind=SendDlgItemMessageW(w,Kind,CB_GETCURSEL,0,0);if(kind<0||kind>1)throw std::runtime_error("Choose a Marker type");return {static_cast<std::int32_t>(number(w,Time)),kind?MarkerKind::enter:MarkerKind::play};}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"STATIC",L"Marker track in current group selection (1-based)",0,16,16,500,22);
        control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,44,170,26,Track);control(w,L"BUTTON",L"Select track",WS_TABSTOP,206,44,294,28,Apply);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,92,484,240,Events);
        control(w,L"STATIC",L"Clocks",0,16,138,484,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,164,484,26,Time);
        control(w,L"STATIC",L"Type",0,16,206,484,22);auto kinds=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,232,484,140,Kind);SendMessageW(kinds,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Marker"));SendMessageW(kinds,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Enter SwitchPoint"));
        control(w,L"BUTTON",L"Add",WS_TABSTOP,16,280,148,30,Add);control(w,L"BUTTON",L"Change",WS_TABSTOP,184,280,148,30,Change);control(w,L"BUTTON",L"Delete",WS_TABSTOP,352,280,148,30,Delete);
        control(w,L"BUTTON",L"Copy",WS_TABSTOP,16,330,232,30,Copy);control(w,L"BUTTON",L"Paste at clocks",WS_TABSTOP,268,330,232,30,Paste);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,380,232,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,268,380,232,30,Redo);
        control(w,L"STATIC",L"Range clocks [begin, end), matching boundaries",0,16,426,484,22);
        control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,454,148,26,RangeBegin);control(w,L"EDIT",std::to_wstring(s->doc().length()).c_str(),WS_BORDER|WS_TABSTOP,184,454,148,26,RangeEnd);
        auto divisions=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,352,454,148,140,Division);for(auto text:{L"Measures",L"Beats",L"Grids"})SendMessageW(divisions,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));SendMessageW(divisions,CB_SETCURSEL,0,0);
        control(w,L"BUTTON",L"Mark range",WS_TABSTOP,16,498,232,30,MarkRange);control(w,L"BUTTON",L"Unmark range",WS_TABSTOP,268,498,232,30,UnmarkRange);
        control(w,L"BUTTON",L"Mark all",WS_TABSTOP,16,542,232,30,MarkAll);control(w,L"BUTTON",L"Unmark all",WS_TABSTOP,268,542,232,30,UnmarkAll);
        control(w,L"BUTTON",L"Properties (F11)",WS_TABSTOP,16,586,484,30,Properties);control(w,L"STATIC",L"",0,16,632,484,50,Status);refresh(w,*s);return 0;
    }
    if(message==WM_COMMAND&&s&&!s->updating){size_t after=0;switch(LOWORD(wp)){
        case Events:if(HIWORD(wp)==CBN_SELCHANGE){s->selected()=static_cast<size_t>(SendDlgItemMessageW(w,Events,CB_GETCURSEL,0,0));refresh(w,*s);s->publish();}return 0;
        case Properties:s->publish(true);return 0;
        case Apply:{const auto n=number(w,Track);if(!n)throw std::runtime_error("Track numbers start at 1");(void)s->doc().markers(n-1);s->track()=n-1;s->selected()=0;break;}
        case Add:if(!s->doc().add_marker(input(w),s->track(),&after))throw std::runtime_error("Invalid Marker position");s->selected()=after;break;
        case Change:if(!s->doc().edit_marker(s->selected(),input(w),s->track(),&after))throw std::runtime_error("Marker unchanged or invalid");s->selected()=after;break;
        case Delete:s->doc().delete_marker(s->selected(),s->track());break;
        case Copy:s->context.clipboard=s->doc().copy_marker(s->selected(),s->track());break;
        case Paste:if(!s->doc().paste_marker(s->context.clipboard,static_cast<std::int32_t>(number(w,Time)),s->track(),&after))throw std::runtime_error("Invalid Marker paste or incompatible record extension");s->selected()=after;break;
        case MarkRange:case UnmarkRange:case MarkAll:case UnmarkAll:{
            const auto action=LOWORD(wp);const bool all=action==MarkAll||action==UnmarkAll;const auto division=SendDlgItemMessageW(w,Division,CB_GETCURSEL,0,0);if(division<0||division>2)throw std::runtime_error("Choose Measures, Beats or Grids");
            const auto begin=all?0:static_cast<std::int32_t>(number(w,RangeBegin));const auto end=all?s->doc().length():static_cast<std::int32_t>(number(w,RangeEnd));if(begin<0||end<=begin||end>s->doc().length())throw std::runtime_error("Range must be inside the Segment");
            s->doc().mark_boundaries(input(w).kind,static_cast<unsigned>(division),begin,end,action==MarkRange||action==MarkAll,s->track());break;
        }
        case Undo:s->doc().undo();break;case Redo:s->doc().redo();break;default:return 0;
    }refresh(w,*s);s->publish();return 0;}
    if(message==WM_CONTEXTMENU&&s&&!s->doc().markers(s->track()).empty()){
        POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};if(point.x==-1&&point.y==-1){RECT r{};GetWindowRect(GetDlgItem(w,Events),&r);point={r.left+24,r.top+20};}
        auto menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Marker context creation failed");AppendMenuW(menu,MF_STRING,Properties,L"&Properties\tF11");const auto chosen=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,w,nullptr);DestroyMenu(menu);if(chosen==Properties)s->publish(true);return 0;
    }
    if(message==WM_CLOSE){DestroyWindow(w);return 0;}
    if(message==WM_NCDESTROY&&s){editorWindow=nullptr;SetWindowLongPtrW(w,GWLP_USERDATA,0);if(s->ownedByWindow)delete s;return DefWindowProcW(w,message,wp,lp);}
}catch(const std::exception& e){const std::string t=e.what();const std::wstring text(t.begin(),t.end());MessageBoxW(w,text.c_str(),L"Marker edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);}
}
void show_marker_editor(HWND parent,Framework& host,size_t index,MarkerEditorContext& context,MarkerPropertiesChanged changed){(void)host.document(index).markers(context.selection.tracks[host.document(index).selected_groups()]);release_marker_editor();const auto instance=GetModuleHandleW(nullptr);static bool registered=false;
    if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerMarkers";if(!RegisterClassW(&c))throw std::runtime_error("Marker window registration failed");registered=true;}
    auto session=std::make_unique<Session>(Session{host,index,context,std::move(changed)});auto w=CreateWindowExW(0,L"SourceProducerMarkers",L"Segment Markers / Enter SwitchPoints",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,540,760,parent,nullptr,instance,session.get());if(!w)throw std::runtime_error("Marker window creation failed");editorWindow=w;session->ownedByWindow=true;auto s=session.release();ShowWindow(w,SW_SHOW);UpdateWindow(w);s->publish();
}
void refresh_marker_editor(){if(editorWindow){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(editorWindow,GWLP_USERDATA));if(s&&!s->updating)refresh(editorWindow,*s);}}
void select_marker_editor_event(size_t event){if(editorWindow){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(editorWindow,GWLP_USERDATA));if(s){s->selected()=event;refresh(editorWindow,*s);}}}
bool translate_marker_editor(MSG& message){if(!editorWindow||(message.hwnd!=editorWindow&&!IsChild(editorWindow,message.hwnd)))return false;if(message.message==WM_KEYDOWN){const auto id=GetDlgCtrlID(message.hwnd);if(message.wParam==VK_F11){SendMessageW(editorWindow,WM_COMMAND,Properties,0);return true;}if(message.wParam=='A'&&(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&(id==Track||id==Time||id==RangeBegin||id==RangeEnd)){SendMessageW(message.hwnd,EM_SETSEL,0,-1);return true;}}return IsDialogMessageW(editorWindow,&message);}
void release_marker_editor(){if(editorWindow)DestroyWindow(editorWindow);}
}
