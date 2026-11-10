#include "command_editor.h"
#include <commdlg.h>
#include <algorithm>
#include <cwctype>
#include <map>
#include <stdexcept>

namespace producer::app {
namespace {
enum : UINT { Groups=10,Track,Apply,Events=20,Time,Type,Groove,Range,Repeat,Add,Change,Delete,Undo,Redo,Save,Status };
struct Session {
    Framework& framework;size_t documentIndex;CommandEditorContext& context;
    size_t& track(){return context.tracks[doc().selected_groups()];}
    SegmentDocument& doc(){return framework.document(documentIndex);}
    size_t& selected(){return context.events[{doc().selected_groups(),track()}];}
};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){
    const auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);
    if(!c)throw std::runtime_error("Command control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
std::wstring value(HWND w,UINT id){const auto c=GetDlgItem(w,id);const int n=GetWindowTextLengthW(c);std::wstring v(static_cast<size_t>(n)+1,L'\0');GetWindowTextW(c,v.data(),n+1);v.resize(n);return v;}
unsigned number(HWND w,UINT id,unsigned maximum){const auto v=value(w,id);size_t end=0;const auto n=std::stoll(v,&end);if(end!=v.size()||n<0||static_cast<unsigned long long>(n)>maximum)throw std::runtime_error("Command field outside its allowed range");return static_cast<unsigned>(n);}
void set(HWND w,UINT id,unsigned n){SetWindowTextW(GetDlgItem(w,id),std::to_wstring(n).c_str());}
std::wstring group_text(std::uint32_t mask){std::wstring out;for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t(1)<<i)){if(!out.empty())out+=L",";out+=std::to_wstring(i+1);}return out;}
std::uint32_t groups(HWND w){const auto v=value(w,Groups);std::uint32_t mask=0;size_t at=0;while(at<v.size()){const auto comma=v.find(L',',at);const auto token=v.substr(at,comma==std::wstring::npos?std::wstring::npos:comma-at);size_t end=0;const auto n=std::stoul(token,&end);while(end<token.size()&&iswspace(token[end]))++end;if(end!=token.size()||n<1||n>32)throw std::runtime_error("Groups must be numbers 1 to 32 separated by commas");mask|=std::uint32_t(1)<<(n-1);if(comma==std::wstring::npos)break;at=comma+1;if(at==v.size())throw std::runtime_error("Missing group after comma");}if(!mask)throw std::runtime_error("Choose a group");return mask;}
CommandEvent input(HWND w){return {static_cast<std::int32_t>(number(w,Time,INT32_MAX)),0,0,static_cast<BYTE>(number(w,Type,255)),static_cast<BYTE>(number(w,Groove,255)),static_cast<BYTE>(number(w,Range,255)),static_cast<BYTE>(number(w,Repeat,255))};}
void refresh(HWND w,Session& s){
    SetWindowTextW(GetDlgItem(w,Groups),group_text(s.doc().selected_groups()).c_str());set(w,Track,static_cast<unsigned>(s.track()+1));
    const auto events=s.doc().commands(s.track());const auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<events.size();++i){const auto& e=events[i];const auto label=L"Command "+std::to_wstring(i+1)+L": "+std::to_wstring(e.time)+L" clocks, measure "+std::to_wstring(e.measure+1)+L", beat "+std::to_wstring(e.beat+1);SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    if(!events.empty()){s.selected()=std::min(s.selected(),events.size()-1);SendMessageW(list,CB_SETCURSEL,s.selected(),0);const auto& e=events[s.selected()];SetWindowTextW(GetDlgItem(w,Time),std::to_wstring(e.time).c_str());set(w,Type,e.type);set(w,Groove,e.groove);set(w,Range,e.range);set(w,Repeat,e.repeat);}
    else{set(w,Time,0);set(w,Type,0);set(w,Groove,50);set(w,Range,0);set(w,Repeat,0);}
    for(const auto id:{Events,Change,Delete})EnableWindow(GetDlgItem(w,id),!events.empty());
    const auto message=std::wstring(s.doc().dirty()?L"Modified. ":L"Saved. ")+std::to_wstring(events.size())+L" commands. New values: type 0-5, groove/range 0-100, repeat 0-5. Imported unknown values remain if unchanged.";
    SetWindowTextW(GetDlgItem(w,Status),message.c_str());
}
std::wstring save_path(HWND w){wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFilter=L"Segment (*.sgp)\0*.sgp\0\0";o.lpstrFile=file;o.nMaxFile=32768;o.lpstrDefExt=L"sgp";o.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;const auto ok=GetSaveFileNameW(&o);if(!ok&&CommDlgExtendedError())throw std::runtime_error("Command save dialog failed");return ok?file:L"";}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try{if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"STATIC",L"Track groups (1-32, comma separated)",0,16,16,300,20);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,16,40,240,26,Groups);
        control(w,L"STATIC",L"Command track (1-based)",0,276,16,180,20);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,276,40,120,26,Track);control(w,L"BUTTON",L"Apply selection",WS_TABSTOP,416,40,150,28,Apply);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,90,550,250,Events);
        const wchar_t* labels[]={L"Clocks",L"Type (0 Groove, 1 Fill, 2 Intro, 3 Break, 4 End, 5 End + Intro)",L"Groove",L"Groove range",L"Repeat"};
        for(unsigned i=0;i<5;++i){const int y=132+static_cast<int>(i)*48;control(w,L"STATIC",labels[i],0,16,y,410,20);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,436,y,130,26,Time+i);}
        control(w,L"BUTTON",L"Add Command",WS_TABSTOP,16,384,170,30,Add);control(w,L"BUTTON",L"Change Command",WS_TABSTOP,206,384,170,30,Change);control(w,L"BUTTON",L"Delete Command...",WS_TABSTOP,396,384,170,30,Delete);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,432,110,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,146,432,110,30,Redo);control(w,L"BUTTON",L"Save Segment As...",WS_TABSTOP,276,432,290,30,Save);
        control(w,L"STATIC",L"",0,16,482,550,65,Status);refresh(w,*s);return 0;}
        if(message==WM_COMMAND&&s){const auto id=LOWORD(wp),notification=HIWORD(wp);
            switch(id){
            case Events:if(notification==CBN_SELCHANGE){const auto i=SendMessageW(GetDlgItem(w,Events),CB_GETCURSEL,0,0);if(i>=0)s->selected()=static_cast<size_t>(i);refresh(w,*s);}return 0;
            case Apply:{try{const auto t=number(w,Track,INT32_MAX);if(!t)throw std::runtime_error("Track numbers start at 1");auto next=s->doc();next.select_track_group(groups(w),next.selected_tempo_index(),next.selected_meter_index(),next.selected_sequence_index(),next.selected_band_index());(void)next.commands(t-1);s->doc()=std::move(next);s->track()=t-1;}catch(...){refresh(w,*s);throw;}break;}
            case Add:{const auto e=input(w);const auto events=s->doc().commands(s->track());size_t after=0;for(const auto& old:events){if(old.time>e.time)break;++after;}if(!s->doc().add_command(e,s->track()))throw std::runtime_error("Command rejected: use known values and a time inside the Segment");s->selected()=after;break;}
            case Change:{size_t after=0;if(!s->doc().edit_command(s->selected(),input(w),s->track(),&after))throw std::runtime_error("Command unchanged or rejected; unknown fields may only retain their original value");s->selected()=after;break;}
            case Delete:{auto next=s->doc();if(!next.delete_command(s->selected(),s->track()))throw std::runtime_error("Select a Command");if(MessageBoxW(w,L"Delete the selected Command? Undo restores it.",L"Delete Command",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;s->doc()=std::move(next);break;}
            case Undo:s->doc().undo();break;
            case Redo:s->doc().redo();break;
            case Save:{const auto path=save_path(w);if(path.empty())return 0;s->framework.save_segment(s->documentIndex,path);break;}
            default:return 0;
            }refresh(w,*s);return 0;
        }
        if(message==WM_CLOSE){DestroyWindow(w);return 0;}
    }catch(const std::exception& e){const std::string detail=e.what();MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"Command editor",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}
    return DefWindowProcW(w,message,wp,lp);
}
}
void show_command_editor(HWND parent,Framework& framework,size_t documentIndex,CommandEditorContext& context){
    (void)framework.document(documentIndex).commands(context.tracks[framework.document(documentIndex).selected_groups()]);const auto instance=GetModuleHandleW(nullptr);static bool registered=false;
    if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerCommands";if(!RegisterClassW(&c))throw std::runtime_error("Command window registration failed");registered=true;}
    Session session{framework,documentIndex,context};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerCommands",L"Segment Commands",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,610,600,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("Command window creation failed");
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;
    while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Command message loop failed");
}
}
