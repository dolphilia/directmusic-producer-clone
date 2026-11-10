#include "farm_player_window.h"
#include "framework.h"
#include "conductor.h"
#include <commdlg.h>
#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace producer::app {namespace {
enum:UINT {Documents=10,OpenScore,Initialize,Variables,Number,GetNumber,SetNumber,Status,FirstRoutine=100};
struct Binding {const wchar_t* label;const wchar_t* routine;};
// Farm.cpp PlayButton's twelve named calls; All Stop calls dmAllStop, which
// stops the background selected by the score's PlayFlag, not every secondary.
constexpr std::array<Binding,12> bindings{{
    {L"Cougar",L"dmSfxCougar"},{L"Cow",L"dmSfxCow"},{L"Rooster",L"dmSfxRooster"},
    {L"Sheep",L"dmSfxSheep"},{L"Wolf",L"dmSfxWolf"},{L"Alarm",L"dmSfxAlarm"},
    {L"Night",L"dmBGNight"},{L"Predawn",L"dmBGPredawn"},{L"Dawn",L"dmBGDawn"},
    {L"End",L"dmEnding"},{L"Bird",L"dmSSBird"},{L"All Stop",L"dmAllStop"}
}};
struct Session {Framework& host;Conductor player;size_t selected=0;std::optional<size_t> initializedIndex;Bytes initializedBytes;std::vector<std::wstring> routines;};
std::wstring text(HWND w,UINT id){const auto c=GetDlgItem(w,id);const auto n=GetWindowTextLengthW(c);std::wstring v(n+1,L'\0');GetWindowTextW(c,v.data(),n+1);v.resize(n);return v;}
std::wstring lower(std::wstring v){std::transform(v.begin(),v.end(),v.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});return v;}
void control(HWND w,const wchar_t* cls,const wchar_t* label,DWORD style,int x,int y,int width,int height,UINT id){const auto c=CreateWindowW(cls,label,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);if(!c)throw std::runtime_error("Farm control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);}
bool ready(const Session& s){return s.initializedIndex&&*s.initializedIndex==s.selected&&s.selected<s.host.scripts().size()&&s.initializedBytes==s.host.script_document(s.selected).save_bytes();}
void enable(HWND w,const Session& s){
    const bool initialized=ready(s);
    for(size_t i=0;i<bindings.size();++i){const auto wanted=lower(bindings[i].routine);const bool found=std::any_of(s.routines.begin(),s.routines.end(),[&](const auto& n){return lower(n)==wanted;});EnableWindow(GetDlgItem(w,FirstRoutine+static_cast<UINT>(i)),initialized&&found);}
    for(const auto id:{Variables,Number,GetNumber,SetNumber})EnableWindow(GetDlgItem(w,id),initialized);
    EnableWindow(GetDlgItem(w,Initialize),!s.host.scripts().empty());
}
void refresh(HWND w,Session& s){
    SendDlgItemMessageW(w,Documents,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<s.host.scripts().size();++i){const auto& e=s.host.scripts()[i];auto name=e.path.empty()?s.host.script_document(i).name():std::filesystem::path(e.path).filename().wstring();if(s.host.script_document(i).dirty())name+=L" *";SendDlgItemMessageW(w,Documents,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));}
    if(s.selected>=s.host.scripts().size())s.selected=0;
    if(!s.host.scripts().empty())SendDlgItemMessageW(w,Documents,CB_SETCURSEL,s.selected,0);
    enable(w,s);
}
void result(HWND w,const ScriptResult& r,const std::wstring& success){
    if(r.passed()){SetDlgItemTextW(w,Status,success.c_str());return;}
    std::wostringstream out;out<<L"Score operation failed (0x"<<std::hex<<static_cast<unsigned long>(r.result)<<L"). ";
    const auto end=std::find(std::begin(r.error.description),std::end(r.error.description),L'\0');out<<std::wstring(std::begin(r.error.description),end);SetDlgItemTextW(w,Status,out.str().c_str());
}
std::wstring file(HWND w){wchar_t name[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=name;o.nMaxFile=static_cast<DWORD>(std::size(name));o.lpstrFilter=L"DirectMusic Script\0*.spt;*.spp\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;return GetOpenFileNameW(&o)?name:L"";}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,16,560,180,Documents);
        control(w,L"BUTTON",L"Open Score...",WS_TABSTOP,16,54,180,28,OpenScore);control(w,L"BUTTON",L"Initialize Score",WS_TABSTOP,210,54,180,28,Initialize);
        control(w,L"STATIC",L"Sound effects",0,16,98,240,24,0);control(w,L"STATIC",L"Background / secondary",0,306,98,250,24,0);
        for(size_t i=0;i<bindings.size();++i)control(w,L"BUTTON",bindings[i].label,WS_TABSTOP,i<6?16:306,128+static_cast<int>(i%6)*38,270,30,FirstRoutine+static_cast<UINT>(i));
        control(w,L"STATIC",L"Score variable",0,16,374,150,24,0);control(w,L"COMBOBOX",L"",CBS_DROPDOWN|WS_TABSTOP,16,404,230,170,Variables);
        control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,260,404,110,26,Number);control(w,L"BUTTON",L"Get",WS_TABSTOP,386,402,85,28,GetNumber);control(w,L"BUTTON",L"Set",WS_TABSTOP,486,402,90,28,SetNumber);
        control(w,L"EDIT",L"Open a score, then initialize it. End and All Stop use the score's background routines.",WS_BORDER|ES_MULTILINE|ES_READONLY,16,452,560,85,Status);refresh(w,*s);return 0;
    }
    if(message==WM_COMMAND&&s){const auto id=LOWORD(wp);
        if(id==Documents&&HIWORD(wp)==CBN_SELCHANGE){const auto selected=SendDlgItemMessageW(w,Documents,CB_GETCURSEL,0,0);if(selected!=CB_ERR)s->selected=static_cast<size_t>(selected);enable(w,*s);SetDlgItemTextW(w,Status,L"Initialize the selected score before calling its routines.");return 0;}
        if(HIWORD(wp)!=BN_CLICKED)return 0;
        if(id==OpenScore){const auto p=file(w);if(!p.empty()){s->selected=s->host.open_script(p);refresh(w,*s);}return 0;}
        if(id==Initialize){s->player.shutdown();s->initializedIndex.reset();s->routines.clear();s->initializedBytes.clear();enable(w,*s);
            if(s->host.scripts().empty())return 0;const auto& e=s->host.scripts().at(s->selected);const auto bytes=s->host.script_document(s->selected).save_bytes();
            const auto r=s->player.load_script(bytes,e.path.empty()?L"":std::filesystem::path(e.path).parent_path().wstring(),w);result(w,r,L"Score initialized.");if(!r.passed())return 0;
            s->routines=s->player.script_session().routines();s->initializedBytes=bytes;s->initializedIndex=s->selected;
            SendDlgItemMessageW(w,Variables,CB_RESETCONTENT,0,0);for(const auto& n:s->player.script_session().variables())SendDlgItemMessageW(w,Variables,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(n.c_str()));SendDlgItemMessageW(w,Variables,CB_SETCURSEL,0,0);enable(w,*s);return 0;
        }
        if(id>=FirstRoutine&&id<FirstRoutine+bindings.size()){
            if(!ready(*s))throw std::runtime_error("Score changed; initialize it again");const auto& b=bindings[id-FirstRoutine];result(w,s->player.script_session().call(b.routine),std::wstring(b.label)+L" requested.");return 0;
        }
        if(id==GetNumber||id==SetNumber){if(!ready(*s))throw std::runtime_error("Score changed; initialize it again");const auto name=text(w,Variables);auto& script=s->player.script_session();
            if(id==GetNumber){LONG n=0;const auto r=script.get_number(name,n);if(r.passed())SetDlgItemTextW(w,Number,std::to_wstring(n).c_str());result(w,r,L"Variable read.");}
            else{const auto value=text(w,Number);size_t end=0;const auto n=std::stoll(value,&end);if(end!=value.size()||n<std::numeric_limits<LONG>::min()||n>std::numeric_limits<LONG>::max())throw std::runtime_error("Score number must be a signed32-bit integer");result(w,script.set_number(name,static_cast<LONG>(n)),L"Variable set.");}return 0;
        }
    }
    if(message==WM_CLOSE){if(s)s->player.shutdown();DestroyWindow(w);return 0;}
    if(message==WM_DESTROY){if(s)s->player.shutdown();return 0;}
}catch(const std::exception& e){if(s)enable(w,*s);SetDlgItemTextW(w,Status,(L"Score operation failed: "+std::wstring(e.what(),e.what()+std::char_traits<char>::length(e.what()))).c_str());if(message==WM_CREATE)return -1;return 0;}
return DefWindowProcW(w,message,wp,lp);}
}
void show_farm_player(HWND parent,Framework& host){
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerFarmPlayer";if(!RegisterClassW(&c))throw std::runtime_error("Farm player registration failed");registered=true;}
    Session session{host};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerFarmPlayer",L"Farm Score Player",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,610,590,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("Farm player creation failed");
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);session.player.shutdown();EnableWindow(parent,enabled);SetActiveWindow(parent);if(!result)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Farm player message loop failed");
}
}
