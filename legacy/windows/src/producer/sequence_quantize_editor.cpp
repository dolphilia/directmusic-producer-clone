#include "sequence_quantize_editor.h"
#include <commctrl.h>
#include <array>
#include <stdexcept>
namespace producer::app { namespace {
enum {Selected=10,Part=11,AllParts=12,Start=13,Duration=14,Divisions=15,Strength=16};
struct Dialog {bool hasSelection;std::optional<SequenceQuantizeCommand> accepted;};
HWND item(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0){
    RECT r{x,y,x+width,y+height};MapDialogRect(w,&r);
    auto c=CreateWindowExW(std::wstring(cls)==L"EDIT"?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Quantize control creation failed");SendMessageW(c,WM_SETFONT,SendMessageW(w,WM_GETFONT,0,0),TRUE);return c;
}
INT_PTR CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(w,DWLP_USER));
    if(message==WM_INITDIALOG){d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);try{
        item(w,L"BUTTON",L"Quantize Target",BS_GROUPBOX,8,7,182,45);
        item(w,L"BUTTON",L"Selected Notes",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,23,19,154,11,Selected);
        item(w,L"BUTTON",L"Part",BS_AUTORADIOBUTTON|WS_TABSTOP,23,30,154,11,Part);
        item(w,L"BUTTON",L"All Parts in Sequence",BS_AUTORADIOBUTTON|WS_TABSTOP,23,41,158,11,AllParts);
        EnableWindow(GetDlgItem(w,Selected),d->hasSelection);CheckRadioButton(w,Selected,AllParts,d->hasSelection?Selected:Part);
        item(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,221,9,46,14,IDOK);
        item(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,221,26,46,14,IDCANCEL);
        item(w,L"BUTTON",L"Change",BS_GROUPBOX,8,59,182,26);
        item(w,L"BUTTON",L"Start Time(s)",BS_AUTOCHECKBOX|WS_GROUP|WS_TABSTOP,22,71,72,11,Start);
        item(w,L"BUTTON",L"Duration(s)",BS_AUTOCHECKBOX|WS_TABSTOP,101,71,76,11,Duration);CheckDlgButton(w,Start,BST_CHECKED);
        item(w,L"STATIC",L"Divide Beat into:",0,8,101,68,12);
        const auto combo=item(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,76,98,37,130,Divisions);
        for(unsigned i=1;i<=24;++i){const auto label=std::to_wstring(i);SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(combo,CB_SETCURSEL,1,0);
        item(w,L"STATIC",L"Strength:",0,119,101,39,12);const auto edit=item(w,L"EDIT",L"100",ES_AUTOHSCROLL|ES_NUMBER|WS_TABSTOP,157,98,36,14,Strength);
        SendMessageW(edit,EM_SETLIMITTEXT,8,0);const auto spin=CreateWindowExW(0,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,0,0,0,0,w,nullptr,GetModuleHandleW(nullptr),nullptr);
        if(!spin)throw std::runtime_error("Quantize spin creation failed");SendMessageW(spin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(edit),0);SendMessageW(spin,UDM_SETRANGE32,0,100);SendMessageW(spin,UDM_SETPOS32,0,100);return TRUE;
    }catch(const std::exception& e){MessageBoxA(w,e.what(),"Quantize",MB_OK|MB_ICONERROR);EndDialog(w,IDCANCEL);return TRUE;}}
    if(message==WM_COMMAND&&d){const auto id=LOWORD(wp);if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}
        if(id==IDOK){try{SequenceQuantizeCommand result;result.target=IsDlgButtonChecked(w,Selected)==BST_CHECKED?SequenceQuantizeTarget::selected:IsDlgButtonChecked(w,Part)==BST_CHECKED?SequenceQuantizeTarget::part:SequenceQuantizeTarget::allParts;
            result.options.start=IsDlgButtonChecked(w,Start)==BST_CHECKED;result.options.duration=IsDlgButtonChecked(w,Duration)==BST_CHECKED;
            const auto division=SendDlgItemMessageW(w,Divisions,CB_GETCURSEL,0,0);if(division<0||division>=24)throw std::runtime_error("Select a beat division from 1 to 24");result.options.divisions=static_cast<unsigned>(division)+1;
            wchar_t text[16]{};GetDlgItemTextW(w,Strength,text,16);if(!*text)throw std::runtime_error("Enter Strength from 0 to 100");unsigned value=0;
            for(const auto* p=text;*p;++p){if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter Strength from 0 to 100");value=value*10+(*p-L'0');if(value>100)throw std::runtime_error("Enter Strength from 0 to 100");}result.options.strength=value;
            d->accepted=result;EndDialog(w,IDOK);
        }catch(const std::exception& e){MessageBoxA(w,e.what(),"Quantize",MB_OK|MB_ICONINFORMATION);SetFocus(GetDlgItem(w,Strength));}return TRUE;}
    }return FALSE;
}
}
std::optional<SequenceQuantizeCommand> show_sequence_quantize_editor(HWND owner,bool hasSelection){
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&controls))throw std::runtime_error("Quantize controls initialization failed");
    std::array<DWORD,128> storage{};auto dialog=reinterpret_cast<DLGTEMPLATE*>(storage.data());dialog->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;dialog->cx=276;dialog->cy=119;
    auto text=reinterpret_cast<WORD*>(dialog+1);*text++=0;*text++=0;for(auto c:std::wstring(L"Quantize"))*text++=c;*text++=0;*text++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*text++=c;*text=0;
    Dialog session{hasSelection,{}};if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),dialog,owner,proc,reinterpret_cast<LPARAM>(&session))==-1)throw std::runtime_error("Quantize dialog creation failed");return session.accepted;
}
}
