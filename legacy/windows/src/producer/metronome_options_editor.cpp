#include "metronome_options_editor.h"
#include "gm_instruments.h"
#include <commctrl.h>
#include <array>
#include <limits>
#include <stdexcept>
namespace producer::app {
namespace {
enum { FirstVelocity=10,FirstPitch,OtherVelocity,OtherPitch,Channel,Bars,RecordOnly,AllPlayback };
struct Session {MetronomeSettings current;std::optional<MetronomeSettings> accepted;};
HWND add(HWND owner,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0){
    RECT bounds{x,y,x+width,y+height};MapDialogRect(owner,&bounds);
    auto child=CreateWindowExW(std::wstring(cls)==L"EDIT"?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,owner,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!child)throw std::runtime_error("Metronome control creation failed");SendMessageW(child,WM_SETFONT,SendMessageW(owner,WM_GETFONT,0,0),TRUE);return child;
}
void numeric(HWND owner,int id,std::uint32_t value,int x,int y,int width,std::uint32_t maximum){
    auto edit=add(owner,L"EDIT",std::to_wstring(value).c_str(),WS_TABSTOP|ES_NUMBER|ES_AUTOHSCROLL,x,y,width,14,id);SendMessageW(edit,EM_SETLIMITTEXT,10,0);
    auto spin=add(owner,UPDOWN_CLASSW,L"",UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,0,0,0,0);
    SendMessageW(spin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(edit),0);SendMessageW(spin,UDM_SETRANGE32,0,maximum);
}
std::uint32_t number(HWND owner,int id){
    wchar_t text[32]{};GetDlgItemTextW(owner,id,text,std::size(text));if(!*text)throw std::runtime_error("Enter a numeric value");std::uint64_t value=0;
    for(auto p=text;*p;++p){if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter a numeric value");value=value*10+(*p-L'0');if(value>UINT32_MAX)throw std::runtime_error("Numeric value is too large");}return static_cast<std::uint32_t>(value);
}
void pitch(HWND owner,int id,unsigned value,int x,int y){
    auto combo=add(owner,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,x,y,119,170,id);
    constexpr const wchar_t* names[]{L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
    for(unsigned n=0;n<128;++n){auto label=std::wstring(names[n%12])+L" "+std::to_wstring(static_cast<int>(n/12));auto drum=gm_drum_name(n);if(!drum.empty())label+=L" - "+drum;SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(combo,CB_SETCURSEL,value,0);
}
INT_PTR CALLBACK proc(HWND window,UINT message,WPARAM w,LPARAM l){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(window,DWLP_USER));
    if(message==WM_INITDIALOG){s=reinterpret_cast<Session*>(l);SetWindowLongPtrW(window,DWLP_USER,l);try{
        add(window,L"BUTTON",L"Beat One",BS_GROUPBOX,9,7,201,35);add(window,L"BUTTON",L"Velocity",BS_GROUPBOX,14,14,52,24);numeric(window,FirstVelocity,s->current.firstVelocity,21,23,39,127);
        add(window,L"BUTTON",L"Note value",BS_GROUPBOX,77,14,128,24);pitch(window,FirstPitch,s->current.firstPitch,82,23);
        add(window,L"BUTTON",L"Other Beats",BS_GROUPBOX,9,47,201,35);add(window,L"BUTTON",L"Velocity",BS_GROUPBOX,14,54,52,24);numeric(window,OtherVelocity,s->current.otherVelocity,21,63,39,127);
        add(window,L"BUTTON",L"Note value",BS_GROUPBOX,77,54,128,24);pitch(window,OtherPitch,s->current.otherPitch,82,63);
        add(window,L"BUTTON",L"PChannel",BS_GROUPBOX,9,89,52,24);numeric(window,Channel,s->current.pchannel+1,16,98,40,INT32_MAX);
        add(window,L"BUTTON",L"Count-In Bars",BS_GROUPBOX,72,89,128,28);numeric(window,Bars,s->current.countInBars,79,101,30,INT32_MAX);
        add(window,L"BUTTON",L"Record Only",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,122,97,76,10,RecordOnly);add(window,L"BUTTON",L"All Playback",BS_AUTORADIOBUTTON|WS_TABSTOP,122,108,76,10,AllPlayback);
        CheckRadioButton(window,RecordOnly,AllPlayback,s->current.recordOnly?RecordOnly:AllPlayback);
        add(window,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,38,128,58,16,IDOK);add(window,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,113,128,58,16,IDCANCEL);SetFocus(GetDlgItem(window,FirstVelocity));SendDlgItemMessageW(window,FirstVelocity,EM_SETSEL,0,-1);return FALSE;
    }catch(const std::exception& e){MessageBoxA(window,e.what(),"Metronome Options",MB_OK|MB_ICONERROR);EndDialog(window,IDCANCEL);return TRUE;}}
    if(message==WM_COMMAND&&s){if(LOWORD(w)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}if(LOWORD(w)==IDOK){try{
        auto next=s->current;next.firstVelocity=number(window,FirstVelocity);next.otherVelocity=number(window,OtherVelocity);next.countInBars=number(window,Bars);const auto channel=number(window,Channel);if(!channel)throw std::runtime_error("PChannel numbering starts at 1");next.pchannel=channel-1;
        const auto first=SendDlgItemMessageW(window,FirstPitch,CB_GETCURSEL,0,0),other=SendDlgItemMessageW(window,OtherPitch,CB_GETCURSEL,0,0);if(first<0||other<0)throw std::runtime_error("Select both notes");next.firstPitch=static_cast<unsigned>(first);next.otherPitch=static_cast<unsigned>(other);next.recordOnly=IsDlgButtonChecked(window,RecordOnly)==BST_CHECKED;validate_metronome_settings(next);s->accepted=next;EndDialog(window,IDOK);
    }catch(const std::exception& e){MessageBoxA(window,e.what(),"Metronome Options",MB_OK|MB_ICONINFORMATION);}return TRUE;}}return FALSE;
}
}
std::optional<MetronomeSettings> show_metronome_options(HWND owner,const MetronomeSettings& current){
    validate_metronome_settings(current);INITCOMMONCONTROLSEX common{sizeof(common),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&common))throw std::runtime_error("Metronome spin initialization failed");
    std::array<DWORD,128> storage{};auto d=reinterpret_cast<DLGTEMPLATE*>(storage.data());d->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;d->cx=242;d->cy=151;
    auto text=reinterpret_cast<WORD*>(d+1);*text++=0;*text++=0;for(auto c:std::wstring(L"Metronome Options"))*text++=c;*text++=0;*text++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*text++=c;*text=0;
    Session session{current,{}};if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),d,owner,proc,reinterpret_cast<LPARAM>(&session))==-1)throw std::runtime_error("Metronome Options creation failed");return session.accepted;
}
}
