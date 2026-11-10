#include "motif_editor.h"
#include "dls_editor.h"
#include <stdexcept>
#include <algorithm>
namespace producer::app {
namespace {
struct PlaybackDialog {const Framework& host;std::optional<MotifPlaybackChoice> result;};
LRESULT CALLBACK playback_proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<PlaybackDialog*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(message==WM_CREATE){s=static_cast<PlaybackDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));return 0;}
    if(message==WM_COMMAND&&s&&LOWORD(wp)==IDOK){
        wchar_t text[32]{};GetWindowTextW(GetDlgItem(w,103),text,32);const std::wstring value=text;
        if(value.empty()||value.size()>10||!std::all_of(value.begin(),value.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';})||std::stoull(value)>INT32_MAX){MessageBoxW(w,L"Enter a delay from 0 to 2147483647 music clocks.",L"Motif playback",MB_OK);return 0;}
        const auto at=SendMessageW(GetDlgItem(w,100),CB_GETCURSEL,0,0);
        if(at<0||at>4)return 0;
        const auto path=SendMessageW(GetDlgItem(w,104),CB_GETCURSEL,0,0);if(path<0||static_cast<size_t>(path)>s->host.audio_paths().size())return 0;
        s->result=MotifPlaybackChoice{PlaybackOptions{static_cast<PlaybackBoundary>(at),SendMessageW(GetDlgItem(w,101),BM_GETCHECK,0,0)==BST_CHECKED,SendMessageW(GetDlgItem(w,102),BM_GETCHECK,0,0)==BST_CHECKED,static_cast<LONG>(std::stoull(value))},path?std::optional<size_t>(static_cast<size_t>(path)-1):std::nullopt};DestroyWindow(w);return 0;
    }
    if(message==WM_CLOSE||(message==WM_COMMAND&&LOWORD(wp)==IDCANCEL)){DestroyWindow(w);return 0;}
    return DefWindowProcW(w,message,wp,lp);
}
}
std::optional<MotifPlaybackChoice> choose_motif_playback(HWND parent,const Framework& host){
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=playback_proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerMotifPlayback";if(!RegisterClassW(&c))throw std::runtime_error("Motif playback window registration failed");registered=true;}
    PlaybackDialog s{host,{}};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerMotifPlayback",L"Play Selected Motif",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,510,410,parent,nullptr,instance,&s);if(!w)throw std::runtime_error("Motif playback window creation failed");
    auto control=[&](const wchar_t* cls,const wchar_t* label,DWORD flags,int x,int y,int width,int height,UINT id){const auto c=CreateWindowW(cls,label,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),instance,nullptr);if(!c)throw std::runtime_error("Playback control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;};
    control(L"STATIC",L"Start boundary",0,16,20,190,22,0);const auto boundary=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,220,20,250,180,100);for(const auto label:{L"As soon as possible",L"Saved Motif boundary",L"Grid",L"Beat",L"Measure"})SendMessageW(boundary,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));SendMessageW(boundary,CB_SETCURSEL,1,0);
    const auto prepare=control(L"BUTTON",L"Wait for preparation",BS_AUTOCHECKBOX|WS_TABSTOP,16,68,430,26,101);SendMessageW(prepare,BM_SETCHECK,BST_CHECKED,0);
    control(L"BUTTON",L"Play as a secondary segment",BS_AUTOCHECKBOX|WS_TABSTOP,16,104,430,26,102);
    control(L"STATIC",L"Delay (music clocks)",0,16,152,200,24,0);const auto delay=control(L"EDIT",L"0",WS_BORDER|ES_NUMBER|WS_TABSTOP,220,152,250,26,103);SendMessageW(delay,EM_SETLIMITTEXT,10,0);
    control(L"STATIC",L"AudioPath",0,16,202,190,24,0);const auto paths=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,220,200,250,180,104);SendMessageW(paths,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Transport default"));
    for(size_t i=0;i<host.audio_paths().size();++i){const auto label=std::to_wstring(i+1)+L": "+host.audio_path_document(i).name();SendMessageW(paths,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(paths,CB_SETCURSEL,0,0);
    control(L"STATIC",L"Uses a copy for this playback. Saved documents stay unchanged.",0,16,244,455,38,0);
    control(L"BUTTON",L"Play",BS_DEFPUSHBUTTON|WS_TABSTOP,220,294,110,30,IDOK);control(L"BUTTON",L"Cancel",WS_TABSTOP,350,294,120,30,IDCANCEL);
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Playback dialog message loop failed");return s.result;
}
namespace {
enum : UINT {Repeats=10,PlayStart,LoopStart,LoopEnd,Resolution,Apply=20,Undo,Redo,Save,Status};
struct Session {Framework& host;size_t style,pattern;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* label,DWORD flags,int x,int y,int width,int height,UINT id=0){
    const auto c=CreateWindowW(cls,label,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Motif control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
std::uint32_t number(HWND w,UINT id,std::uint32_t maximum){wchar_t text[64]{};GetWindowTextW(GetDlgItem(w,id),text,64);const std::wstring value=text;if(value.empty()||value.size()>10||!std::all_of(value.begin(),value.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';}))throw std::runtime_error("Enter an unsigned decimal integer");const auto n=std::stoull(value);if(n>maximum)throw std::runtime_error("Motif value is outside the allowed range");return static_cast<std::uint32_t>(n);}
void refresh(HWND w,Session& s){
    const auto patterns=s.host.style_document(s.style).patterns();const bool selected=s.pattern<patterns.size()&&(patterns[s.pattern].embellishment&16);
    for(UINT id=Repeats;id<=Resolution;++id)EnableWindow(GetDlgItem(w,id),selected);EnableWindow(GetDlgItem(w,Apply),selected);
    if(!selected){SetWindowTextW(GetDlgItem(w,Status),L"The selected Motif is absent. Redo can restore it.");return;}
    const auto settings=s.host.style_document(s.style).motif_settings(s.pattern);const auto value=settings.value_or(StyleMotifSettings{});
    const std::uint32_t values[]={value.repeats,static_cast<std::uint32_t>(value.playStart),static_cast<std::uint32_t>(value.loopStart),static_cast<std::uint32_t>(value.loopEnd),value.resolution};for(unsigned i=0;i<5;++i)SetWindowTextW(GetDlgItem(w,Repeats+i),std::to_wstring(values[i]).c_str());
    SetWindowTextW(w,(L"Motif Playback Settings — "+patterns[s.pattern].name).c_str());
    const auto message=std::wstring(s.host.style_document(s.style).dirty()?L"Modified. ":L"Saved. ")+(settings?L"":L"No stored settings; Apply stores the displayed defaults. ")+L"Loop end 0 uses the full Motif. Repeat 4294967295 loops indefinitely. Resolution retains its full flag value.";SetWindowTextW(GetDlgItem(w,Status),message.c_str());
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try{
        if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
            const wchar_t* labels[]={L"Repeats after the first play",L"Playback start (clocks)",L"Loop start (clocks)",L"Loop end (clocks, 0 = full Motif)",L"Resolution flags (decimal)"};
            for(unsigned i=0;i<5;++i){const int y=20+static_cast<int>(i)*48;control(w,L"STATIC",labels[i],0,16,y,310,22);const auto c=control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,336,y,190,26,Repeats+i);SendMessageW(c,EM_SETLIMITTEXT,10,0);}
            control(w,L"BUTTON",L"Apply Settings",WS_TABSTOP,16,274,160,30,Apply);control(w,L"BUTTON",L"Undo",WS_TABSTOP,196,274,90,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,306,274,90,30,Redo);control(w,L"BUTTON",L"Save Style",WS_TABSTOP,416,274,110,30,Save);control(w,L"STATIC",L"",0,16,326,510,80,Status);refresh(w,*s);return 0;
        }
        if(message==WM_COMMAND&&s){switch(LOWORD(wp)){
            case Apply:{StyleMotifSettings v{number(w,Repeats,UINT32_MAX),static_cast<std::int32_t>(number(w,PlayStart,INT32_MAX)),static_cast<std::int32_t>(number(w,LoopStart,INT32_MAX)),static_cast<std::int32_t>(number(w,LoopEnd,INT32_MAX)),number(w,Resolution,UINT32_MAX)};if(!s->host.set_style_motif_settings(s->style,s->pattern,v))throw std::runtime_error("Settings unchanged or invalid. Playback/loop start must be inside the Motif; loop end is 0 or greater than loop start and at most the Motif length.");break;}
            case Undo:s->host.undo_style(s->style);break;
            case Redo:s->host.redo_style(s->style);break;
            case Save:{const auto path=s->host.style_documents().at(s->style).path;if(path.empty())throw std::runtime_error("Use Save Document As in the main window first");s->host.save_style(s->style,path);break;}
            default:return 0;
        }refresh(w,*s);return 0;}
        if(message==WM_CLOSE){DestroyWindow(w);return 0;}
    }catch(const std::exception& e){const std::string detail=e.what();MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"Motif settings",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}
    return DefWindowProcW(w,message,wp,lp);
}
}
namespace {
enum : UINT {Instrument=30,Patch,Channel,Pan,Volume,BandApply=40,Collection=50,CollectionAssign};
void refresh_instrument(HWND w,Session& s){
    const auto band=s.host.style_document(s.style).motif_band(s.pattern);const auto at=SendMessageW(GetDlgItem(w,Instrument),CB_GETCURSEL,0,0);
    const auto instruments=band?band->instruments():std::vector<BandInstrument>{};const bool valid=at>=0&&static_cast<size_t>(at)<instruments.size();
    for(UINT id=Patch;id<=Volume;++id)EnableWindow(GetDlgItem(w,id),valid);EnableWindow(GetDlgItem(w,BandApply),valid);
    EnableWindow(GetDlgItem(w,CollectionAssign),valid&&!s.host.collections().empty());
    if(valid){const auto& i=instruments[at];const std::uint32_t values[]={i.patch,i.pchannel,i.pan,i.volume};for(unsigned n=0;n<4;++n)SetWindowTextW(GetDlgItem(w,Patch+n),std::to_wstring(values[n]).c_str());}
    SetWindowTextW(GetDlgItem(w,Status),s.host.style_document(s.style).dirty()?L"Modified. Only this Motif's Band copy is edited. Style Bands remain unchanged.":L"Saved. Only this Motif's Band copy is edited. Style Bands remain unchanged.");
}
void refresh_band(HWND w,Session& s){
    const auto combo=GetDlgItem(w,Instrument);const auto old=SendMessageW(combo,CB_GETCURSEL,0,0);SendMessageW(combo,CB_RESETCONTENT,0,0);
    const auto band=s.host.style_document(s.style).motif_band(s.pattern);const auto instruments=band?band->instruments():std::vector<BandInstrument>{};
    for(size_t i=0;i<instruments.size();++i){const auto label=L"Instrument "+std::to_wstring(i+1)+L", PChannel "+std::to_wstring(instruments[i].pchannel);SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    if(!instruments.empty())SendMessageW(combo,CB_SETCURSEL,old>=0&&static_cast<size_t>(old)<instruments.size()?old:0,0);refresh_instrument(w,s);
}
LRESULT CALLBACK band_proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try{
        if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
            control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,16,510,200,Instrument);
            const auto collections=control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,410,330,200,Collection);
            for(const auto& c:s->host.collections())SendMessageW(collections,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(c.path.c_str()));
            if(!s->host.collections().empty())SendMessageW(collections,CB_SETCURSEL,0,0);
            control(w,L"BUTTON",L"Assign DLS Instrument...",WS_TABSTOP,356,410,170,30,CollectionAssign);
            const wchar_t* labels[]={L"Patch (packed banks)",L"PChannel",L"Pan (0..127)",L"Volume (0..127)"};for(unsigned i=0;i<4;++i){const int y=64+static_cast<int>(i)*48;control(w,L"STATIC",labels[i],0,16,y,310,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,336,y,190,26,Patch+i);}
            control(w,L"BUTTON",L"Apply Instrument",WS_TABSTOP,16,274,160,30,BandApply);control(w,L"BUTTON",L"Undo",WS_TABSTOP,196,274,90,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,306,274,90,30,Redo);control(w,L"BUTTON",L"Save Style",WS_TABSTOP,416,274,110,30,Save);control(w,L"STATIC",L"",0,16,326,510,80,Status);refresh_band(w,*s);return 0;
        }
        if(message==WM_COMMAND&&s){switch(LOWORD(wp)){
            case Instrument:if(HIWORD(wp)==CBN_SELCHANGE)refresh_instrument(w,*s);return 0;
            case BandApply:{const auto i=SendMessageW(GetDlgItem(w,Instrument),CB_GETCURSEL,0,0);if(i<0||!s->host.set_style_motif_band_instrument(s->style,s->pattern,static_cast<size_t>(i),number(w,Patch,UINT32_MAX),number(w,Channel,UINT32_MAX),number(w,Pan,127),number(w,Volume,127)))throw std::runtime_error("Instrument unchanged or invalid");break;}
            case CollectionAssign:{const auto i=SendMessageW(GetDlgItem(w,Instrument),CB_GETCURSEL,0,0),c=SendMessageW(GetDlgItem(w,Collection),CB_GETCURSEL,0,0);if(i<0||c<0)throw std::runtime_error("Select an instrument and an open DLS collection");const auto chosen=choose_dls_instrument(w,s->host.collection_document(static_cast<size_t>(c)));if(chosen&&!s->host.set_style_motif_band_collection_instrument(s->style,s->pattern,static_cast<size_t>(i),static_cast<size_t>(c),*chosen))throw std::runtime_error("DLS assignment unchanged or invalid");break;}
            case Undo:s->host.undo_style(s->style);break;case Redo:s->host.redo_style(s->style);break;
            case Save:{const auto path=s->host.style_documents().at(s->style).path;if(path.empty())throw std::runtime_error("Use Save Document As in the main window first");s->host.save_style(s->style,path);break;}
            default:return 0;
        }refresh_band(w,*s);return 0;}
        if(message==WM_CLOSE){DestroyWindow(w);return 0;}
    }catch(const std::exception& e){const std::string detail=e.what();MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"Motif Band",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}
    return DefWindowProcW(w,message,wp,lp);
}
}
void show_motif_band_editor(HWND parent,Framework& host,size_t styleIndex,size_t patternIndex){
    if(!host.style_document(styleIndex).motif_band(patternIndex))throw std::runtime_error("Assign a Band to the selected Motif first");
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=band_proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerMotifBand";if(!RegisterClassW(&c))throw std::runtime_error("Motif Band window registration failed");registered=true;}
    Session s{host,styleIndex,patternIndex};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerMotifBand",L"Motif Band Instruments",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,570,510,parent,nullptr,instance,&s);if(!w)throw std::runtime_error("Motif Band editor creation failed");
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Motif Band message loop failed");
}
void show_motif_editor(HWND parent,Framework& host,size_t styleIndex,size_t patternIndex){
    const auto patterns=host.style_document(styleIndex).patterns();if(patternIndex>=patterns.size()||!(patterns[patternIndex].embellishment&16))throw std::runtime_error("Select a Motif");(void)host.style_document(styleIndex).motif_settings(patternIndex);
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerMotifSettings";if(!RegisterClassW(&c))throw std::runtime_error("Motif window registration failed");registered=true;}
    RECT bounds{};if(!GetWindowRect(parent,&bounds))throw std::runtime_error("Motif parent bounds unavailable");MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);if(!GetMonitorInfoW(MonitorFromWindow(parent,MONITOR_DEFAULTTONEAREST),&monitor))throw std::runtime_error("Motif monitor bounds unavailable");
    const int x=std::max(static_cast<int>(monitor.rcWork.left),std::min(static_cast<int>(bounds.left+(bounds.right-bounds.left-570)/2),static_cast<int>(monitor.rcWork.right-570))),y=std::max(static_cast<int>(monitor.rcWork.top),std::min(static_cast<int>(bounds.top+(bounds.bottom-bounds.top-460)/2),static_cast<int>(monitor.rcWork.bottom-460)));
    Session s{host,styleIndex,patternIndex};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerMotifSettings",L"Motif Playback Settings",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,x,y,570,460,parent,nullptr,instance,&s);if(!w)throw std::runtime_error("Motif editor creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Motif message loop failed");
}
}
