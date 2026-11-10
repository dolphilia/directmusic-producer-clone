#include "sequence_velocity_editor.h"
#include <commctrl.h>
#include <array>
#include <cwchar>
#include <stdexcept>
namespace producer::app { namespace {
enum {Selected=10,Part=11,AllParts=12,Offset=13,Limit=14,Percent=15,Linear=16,Start=17,End=18,Minimum=19,Maximum=20};
struct Dialog {bool hasSelection;std::optional<SequenceVelocityCommand> accepted;};
HWND item(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0){
    RECT r{x,y,x+width,y+height};MapDialogRect(w,&r);
    auto c=CreateWindowExW(std::wstring(cls)==L"EDIT"?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Velocity control creation failed");SendMessageW(c,WM_SETFONT,SendMessageW(w,WM_GETFONT,0,0),TRUE);return c;
}
void number(HWND w,int id,int x,int y,const wchar_t* value,int minimum,int maximum){
    auto edit=item(w,L"EDIT",value,ES_AUTOHSCROLL|WS_TABSTOP,x,y,53,14,id);
    auto spin=CreateWindowExW(0,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,0,0,0,0,w,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!spin)throw std::runtime_error("Velocity spin creation failed");SendMessageW(spin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(edit),0);SendMessageW(spin,UDM_SETRANGE32,minimum,maximum);
}
int read_number(HWND w,int id,int minimum,int maximum){
    wchar_t text[32]{};GetDlgItemTextW(w,id,text,32);auto p=text;if(*p==L'+'||*p==L'-')++p;if(!*p)throw std::runtime_error("Enter a numeric velocity value");
    for(;*p;++p)if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter a numeric velocity value");
    wchar_t* end=nullptr;const auto n=std::wcstoll(text,&end,10);if(n<minimum||n>maximum)throw std::runtime_error("Velocity value is outside the allowed range");return static_cast<int>(n);
}
void enable_fields(HWND w){const bool offset=IsDlgButtonChecked(w,Offset)==BST_CHECKED;for(int id:{Start,End,Percent,Linear})EnableWindow(GetDlgItem(w,id),offset);for(int id:{Minimum,Maximum})EnableWindow(GetDlgItem(w,id),!offset);}
INT_PTR CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(w,DWLP_USER));
    if(message==WM_INITDIALOG){d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);try{
        item(w,L"BUTTON",L"Edit Velocity Target",BS_GROUPBOX,10,8,176,72);
        item(w,L"BUTTON",L"Selected Notes",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,30,27,135,12,Selected);
        item(w,L"BUTTON",L"Part",BS_AUTORADIOBUTTON|WS_TABSTOP,30,42,135,12,Part);
        item(w,L"BUTTON",L"All Parts in Sequence",BS_AUTORADIOBUTTON|WS_TABSTOP,30,57,147,12,AllParts);
        EnableWindow(GetDlgItem(w,Selected),d->hasSelection);CheckRadioButton(w,Selected,AllParts,d->hasSelection?Selected:Part);
        item(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,275,16,86,20,IDOK);item(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,275,41,86,20,IDCANCEL);
        item(w,L"BUTTON",L"Offset",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,44,101,62,14,Offset);
        item(w,L"STATIC",L"Start",0,112,102,31,12);number(w,Start,144,99,L"+0",-1000,1000);
        item(w,L"STATIC",L"End",0,207,102,28,12);number(w,End,236,99,L"+0",-1000,1000);
        item(w,L"BUTTON",L"%",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,300,93,62,12,Percent);item(w,L"BUTTON",L"linear",BS_AUTORADIOBUTTON|WS_TABSTOP,300,110,62,12,Linear);CheckRadioButton(w,Percent,Linear,Percent);
        item(w,L"BUTTON",L"Limit Range",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,12,137,94,14,Limit);
        item(w,L"STATIC",L"Min",0,112,138,31,12);number(w,Minimum,144,135,L"1",1,127);
        item(w,L"STATIC",L"Max",0,207,138,28,12);number(w,Maximum,236,135,L"127",1,127);
        CheckRadioButton(w,Offset,Limit,Offset);enable_fields(w);return TRUE;
    }catch(const std::exception& e){MessageBoxA(w,e.what(),"Edit Velocity",MB_OK|MB_ICONERROR);EndDialog(w,IDCANCEL);return TRUE;}}
    if(message==WM_COMMAND&&d){const auto id=LOWORD(wp);if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}
        if(id==Offset||id==Limit){CheckRadioButton(w,Offset,Limit,id);enable_fields(w);return TRUE;}
        if(id==IDOK){try{SequenceVelocityCommand result{};result.target=IsDlgButtonChecked(w,Selected)==BST_CHECKED?SequenceVelocityTarget::selected:IsDlgButtonChecked(w,Part)==BST_CHECKED?SequenceVelocityTarget::part:SequenceVelocityTarget::allParts;
            result.options.limit=IsDlgButtonChecked(w,Limit)==BST_CHECKED;result.options.percent=IsDlgButtonChecked(w,Percent)==BST_CHECKED;
            if(result.options.limit){result.options.minimum=read_number(w,Minimum,1,127);result.options.maximum=read_number(w,Maximum,1,127);if(result.options.minimum>result.options.maximum)throw std::runtime_error("Min must not exceed Max");}
            else{result.options.start=read_number(w,Start,-1000,1000);result.options.end=read_number(w,End,-1000,1000);}
            d->accepted=result;EndDialog(w,IDOK);
        }catch(const std::exception& e){MessageBoxA(w,e.what(),"Edit Velocity",MB_OK|MB_ICONINFORMATION);}return TRUE;}
    }return FALSE;
}
}
std::optional<SequenceVelocityCommand> show_sequence_velocity_editor(HWND owner,bool hasSelection){
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&controls))throw std::runtime_error("Velocity controls initialization failed");
    std::array<DWORD,128> storage{};auto dialog=reinterpret_cast<DLGTEMPLATE*>(storage.data());dialog->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;dialog->cx=375;dialog->cy=176;
    auto text=reinterpret_cast<WORD*>(dialog+1);*text++=0;*text++=0;for(auto c:std::wstring(L"Edit Velocity"))*text++=c;*text++=0;*text++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*text++=c;*text=0;
    Dialog session{hasSelection,{}};if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),dialog,owner,proc,reinterpret_cast<LPARAM>(&session))==-1)throw std::runtime_error("Velocity dialog creation failed");return session.accepted;
}
}
