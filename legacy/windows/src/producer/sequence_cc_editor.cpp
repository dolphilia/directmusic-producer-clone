#include "sequence_cc_editor.h"
#include <array>
#include <algorithm>
#include <cwchar>
#include <stdexcept>
namespace producer::app { namespace {
enum {Types=10,Parameter=11,StartTime=12,Duration=13,StartValue=14,EndValue=15,Shape=16,Reset=17,ResetDuration=18,ResetValue=19,FromCurrent=20,Merge=21};
struct Dialog {std::vector<SequenceController> existing;std::optional<SequenceController> controller;std::optional<SequenceCurve> curve;SequenceCurve original;std::int32_t length=0;bool properties=false;};
HWND item(HWND w,const wchar_t* cls,const std::wstring& text,DWORD style,int x,int y,int width,int height,int id=0){
    RECT r{x,y,x+width,y+height};MapDialogRect(w,&r);auto c=CreateWindowExW(std::wstring(cls)==L"EDIT"?WS_EX_CLIENTEDGE:0,cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("CC control creation failed");SendMessageW(c,WM_SETFONT,SendMessageW(w,WM_GETFONT,0,0),TRUE);return c;
}
void number(HWND w,const wchar_t* label,int id,int y,int value){item(w,L"STATIC",label,0,10,y+2,126,12);item(w,L"EDIT",std::to_wstring(value),ES_AUTOHSCROLL|WS_TABSTOP,142,y,74,14,id);}
int read_number(HWND w,int id,int minimum,int maximum){wchar_t text[32]{};GetDlgItemTextW(w,id,text,32);auto p=text;if(*p==L'+'||*p==L'-')++p;if(!*p)throw std::runtime_error("Enter a numeric value");for(;*p;++p)if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter a numeric value");const auto value=std::wcstoll(text,nullptr,10);if(value<minimum||value>maximum)throw std::runtime_error("Value is outside the controller or time range");return static_cast<int>(value);}
SequenceController type_at(int index){if(index<0||index>132)throw std::runtime_error("Select a controller type");return index<5?SequenceController{static_cast<std::uint8_t>(index==0?3:index==1?5:index==2?6:index==3?7:8),0,0}:SequenceController{4,static_cast<std::uint8_t>(index-5),0};}
void enable_parameter(HWND w){const auto index=static_cast<int>(SendDlgItemMessageW(w,Types,CB_GETCURSEL,0,0));const bool parameter=index==3||index==4;EnableWindow(GetDlgItem(w,Parameter),parameter);SetDlgItemTextW(w,100,L"Parameter (0–16383)");}
INT_PTR CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto d=reinterpret_cast<Dialog*>(GetWindowLongPtrW(w,DWLP_USER));
    if(msg==WM_INITDIALOG){d=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);try{
        if(!d->properties){item(w,L"STATIC",L"Specify the type of Control Change\nto be used in this track:",0,10,10,220,26);auto list=item(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,10,40,242,190,Types);
            for(int i=0;i<133;++i){const auto type=type_at(i);auto label=sequence_controller_name(type);if(std::find(d->existing.begin(),d->existing.end(),type)==d->existing.end())label+=L" - create";SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(list,CB_SETCURSEL,0,0);
            item(w,L"STATIC",L"Parameter (0–16383)",0,10,65,130,12,100);item(w,L"EDIT",L"0",ES_AUTOHSCROLL|WS_TABSTOP,142,63,74,14,Parameter);enable_parameter(w);
        }else{item(w,L"STATIC",sequence_controller_name(d->original.controller),0,10,8,240,14);number(w,L"Start (clocks)",StartTime,28,d->original.time);number(w,L"Duration (clocks)",Duration,47,d->original.duration);number(w,L"Start value",StartValue,66,d->original.start);number(w,L"End value",EndValue,85,d->original.end);
            item(w,L"STATIC",L"Shape",0,10,106,126,12);auto shape=item(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,142,104,100,90,Shape);for(const auto* label:{L"Linear",L"Instant",L"Exponential",L"Logarithmic",L"Sine"})SendMessageW(shape,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));SendMessageW(shape,CB_SETCURSEL,d->original.shape,0);
            item(w,L"BUTTON",L"Start From Current",BS_AUTOCHECKBOX|WS_TABSTOP,10,127,220,14,FromCurrent);CheckDlgButton(w,FromCurrent,d->original.flags&2?BST_CHECKED:BST_UNCHECKED);
            item(w,L"BUTTON",L"Enable Reset",BS_AUTOCHECKBOX|WS_TABSTOP,10,147,220,14,Reset);CheckDlgButton(w,Reset,d->original.flags&1?BST_CHECKED:BST_UNCHECKED);
            number(w,L"Reset duration (clocks)",ResetDuration,168,d->original.resetDuration);number(w,L"Reset value",ResetValue,187,d->original.reset);number(w,L"Additive Curve ID",Merge,206,d->original.merge);
        }
        const int y=d->properties?235:89;item(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,107,y,68,18,IDOK);item(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,184,y,68,18,IDCANCEL);return TRUE;
    }catch(const std::exception& e){MessageBoxA(w,e.what(),"CC editor",MB_OK|MB_ICONERROR);EndDialog(w,IDCANCEL);return TRUE;}}
    if(msg==WM_COMMAND&&d){const auto id=LOWORD(wp);if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}if(id==Types&&HIWORD(wp)==CBN_SELCHANGE){enable_parameter(w);return TRUE;}if(id==IDOK){try{
        if(!d->properties){const auto index=static_cast<int>(SendDlgItemMessageW(w,Types,CB_GETCURSEL,0,0));auto type=type_at(index);if(index==3||index==4){const auto number=read_number(w,Parameter,0,16383);type.parameter=static_cast<std::uint16_t>(((number>>7)<<8)|(number&127));}d->controller=type;}
        else{auto curve=d->original;curve.time=read_number(w,StartTime,0,d->length-1);curve.shape=static_cast<BYTE>(SendDlgItemMessageW(w,Shape,CB_GETCURSEL,0,0));curve.duration=read_number(w,Duration,curve.shape==1?0:1,d->length-curve.time);const auto low=sequence_curve_minimum(curve.controller),high=sequence_curve_maximum(curve.controller);curve.start=static_cast<short>(read_number(w,StartValue,low,high));curve.end=static_cast<short>(read_number(w,EndValue,low,high));curve.shape=static_cast<BYTE>(SendDlgItemMessageW(w,Shape,CB_GETCURSEL,0,0));curve.flags=static_cast<BYTE>((curve.flags&~3)|(IsDlgButtonChecked(w,Reset)==BST_CHECKED?1:0)|(IsDlgButtonChecked(w,FromCurrent)==BST_CHECKED?2:0));curve.resetDuration=read_number(w,ResetDuration,0,INT32_MAX);curve.reset=static_cast<short>(read_number(w,ResetValue,low,high));curve.merge=static_cast<WORD>(read_number(w,Merge,0,65535));if(!valid_sequence_curve(curve))throw std::runtime_error("Invalid curve fields");d->curve=curve;}
        EndDialog(w,IDOK);
    }catch(const std::exception& e){MessageBoxA(w,e.what(),"CC editor",MB_OK|MB_ICONINFORMATION);}return TRUE;}}return FALSE;
}
void show(HWND owner,Dialog& d){std::array<DWORD,128> storage{};auto dialog=reinterpret_cast<DLGTEMPLATE*>(storage.data());dialog->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;dialog->cx=264;dialog->cy=d.properties?263:118;auto p=reinterpret_cast<WORD*>(dialog+1);*p++=0;*p++=0;for(auto c:std::wstring(d.properties?L"Curve Properties":L"Add New CC Track"))*p++=c;*p++=0;*p++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*p++=c;*p=0;
    if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),dialog,owner,proc,reinterpret_cast<LPARAM>(&d))==-1)throw std::runtime_error("CC dialog creation failed");}
}
std::wstring sequence_controller_name(SequenceController c){switch(c.type){case 3:return L"PBend";case 5:return L"Mono AT";case 6:return L"Poly AT";case 7:return L"RPN "+std::to_wstring(((c.parameter>>8)<<7)|(c.parameter&127));case 8:return L"NRPN "+std::to_wstring(((c.parameter>>8)<<7)|(c.parameter&127));case 4:{const wchar_t* name=L"";switch(c.data){case 1:name=L" Mod Wheel";break;case 2:name=L" Breath Controller";break;case 4:name=L" Foot Controller";break;case 5:name=L" Portamento Time";break;case 7:name=L" Volume";break;case 8:name=L" Balance";break;case 10:name=L" Pan";break;case 11:name=L" Expression Controller";break;case 64:name=L" Sustain";break;case 91:name=L" Reverb Depth";break;case 93:name=L" Chorus Depth";break;}return std::to_wstring(c.data)+name;}default:return L"Unknown";}}
std::optional<SequenceController> show_sequence_cc_editor(HWND owner,const std::vector<SequenceController>& existing){Dialog d;d.existing=existing;show(owner,d);return d.controller;}
std::optional<SequenceCurve> show_sequence_curve_editor(HWND owner,const SequenceCurve& curve,std::int32_t length){Dialog d;d.properties=true;d.original=curve;d.length=length;show(owner,d);return d.curve;}
}
