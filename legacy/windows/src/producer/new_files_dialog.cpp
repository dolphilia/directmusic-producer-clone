#include "new_files.h"
#include <commctrl.h>
#include <algorithm>
#include <array>
#include <limits>

namespace producer::app {namespace {
enum {List=10,Name=11,Defaults=12,MeasuresMode=20,ClockMode=21,Measures=22,Minutes=23,Seconds=24,Milliseconds=25,ClockLabel=26,MeasuresLabel=27};
constexpr std::array<DocumentKind,11> kinds{DocumentKind::Project,DocumentKind::AudioPath,DocumentKind::Band,DocumentKind::ChordMap,DocumentKind::Container,DocumentKind::Collection,DocumentKind::Script,DocumentKind::Segment,DocumentKind::Style,DocumentKind::ToolGraph,DocumentKind::Wave};
constexpr std::array<const wchar_t*,11> names{L"Project",L"Audiopath",L"Band",L"Chordmap",L"Container",L"DLS Collection",L"Script",L"Segment",L"Style",L"Toolgraph",L"Wave"};
HWND control(HWND owner,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0){
    RECT r{x,y,x+width,y+height};MapDialogRect(owner,&r);auto child=CreateWindowExW(lstrcmpW(cls,L"EDIT")==0||lstrcmpW(cls,L"LISTBOX")==0?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,owner,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!child)throw std::runtime_error("Dialog control creation failed");SendMessageW(child,WM_SETFONT,SendMessageW(owner,WM_GETFONT,0,0),TRUE);return child;
}
void spin(HWND owner,HWND buddy,int maximum){auto up=CreateWindowExW(0,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,0,0,0,0,owner,reinterpret_cast<HMENU>(static_cast<INT_PTR>(1000+GetDlgCtrlID(buddy))),GetModuleHandleW(nullptr),nullptr);if(!up)throw std::runtime_error("Spin creation failed");SendMessageW(up,UDM_SETBUDDY,reinterpret_cast<WPARAM>(buddy),0);SendMessageW(up,UDM_SETRANGE32,0,maximum);}
std::uint64_t number(HWND w,int id){wchar_t text[48]{};GetDlgItemTextW(w,id,text,48);if(!*text)throw std::runtime_error("Enter a number");std::uint64_t value=0;for(auto p=text;*p;++p){if(*p<L'0'||*p>L'9'||value>UINT32_MAX)throw std::runtime_error("Number is out of range");value=value*10+(*p-L'0');}if(value>UINT32_MAX)throw std::runtime_error("Number is out of range");return value;}
INT_PTR modal(HWND owner,const std::wstring& title,short width,short height,DLGPROC proc,void* session){
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&common))throw std::runtime_error("Dialog controls unavailable");
    std::array<DWORD,256> storage{};auto d=reinterpret_cast<DLGTEMPLATE*>(storage.data());d->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;d->cx=width;d->cy=height;
    auto p=reinterpret_cast<WORD*>(d+1);*p++=0;*p++=0;for(auto c:title)*p++=c;*p++=0;*p++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*p++=c;*p=0;
    const auto result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),d,owner,proc,reinterpret_cast<LPARAM>(session));if(result==-1)throw std::runtime_error("Dialog creation failed");return result;
}
void buttons(HWND w,int x,int y,int width=57){control(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,x,y,width,15,IDOK);control(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,x+width+18,y,width,15,IDCANCEL);}
struct FileSession{DocumentKind initial;std::optional<NewFileSelection> accepted;};
INT_PTR CALLBACK files_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<FileSession*>(GetWindowLongPtrW(w,DWLP_USER));try{
    if(msg==WM_INITDIALOG){s=reinterpret_cast<FileSession*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);control(w,L"STATIC",L"File Types:",0,10,8,138,12);auto list=control(w,L"LISTBOX",L"",LBS_EXTENDEDSEL|LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP,10,23,138,115,List);for(size_t i=0;i<names.size();++i){SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(names[i]));if(kinds[i]==s->initial){SendMessageW(list,LB_SETSEL,TRUE,i);SendMessageW(list,LB_SETCARETINDEX,i,FALSE);}}control(w,L"STATIC",L"Name:",0,10,146,138,12);auto name=control(w,L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,10,160,138,14,Name);SendMessageW(name,EM_SETLIMITTEXT,255,0);EnableWindow(name,FALSE);control(w,L"BUTTON",L"Use Default Names",BS_AUTOCHECKBOX|WS_TABSTOP,10,178,138,12,Defaults);CheckDlgButton(w,Defaults,BST_CHECKED);buttons(w,10,197,60);SetFocus(list);return FALSE;}
    if(msg==WM_COMMAND&&s){
        const auto id=LOWORD(wp);
        if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}
        if(id==Defaults){
            const bool customName=IsDlgButtonChecked(w,Defaults)!=BST_CHECKED;
            const auto name=GetDlgItem(w,Name);EnableWindow(name,customName);
            if(customName)SetFocus(name);
            return TRUE;
        }
        if(id==IDOK||(id==List&&HIWORD(wp)==LBN_DBLCLK)){
            NewFileSelection value;
            for(size_t i=0;i<kinds.size();++i)if(SendDlgItemMessageW(w,List,LB_GETSEL,i,0)>0)value.kinds.push_back(kinds[i]);
            if(value.kinds.empty())throw std::runtime_error("Select a file type");
            value.defaultNames=IsDlgButtonChecked(w,Defaults)==BST_CHECKED;
            if(!value.defaultNames){
                wchar_t name[256]{};GetDlgItemTextW(w,Name,name,256);value.name=name;
                if(value.name.empty()){
                    MessageBoxW(w,L"You must type a name to replace the default names.",L"DirectMusic Producer",MB_OK|MB_ICONEXCLAMATION);
                    return TRUE;
                }
            }
            s->accepted=std::move(value);EndDialog(w,IDOK);return TRUE;
        }
    }
}catch(const std::exception& e){MessageBoxA(w,e.what(),"Create New File(s)",MB_OK|MB_ICONINFORMATION);if(msg==WM_INITDIALOG)EndDialog(w,IDCANCEL);}return FALSE;}
struct LengthSession{bool updating=false;const SegmentDocument* document=nullptr;SegmentCreationLength initial;std::optional<SegmentCreationLength> accepted;};
SegmentLengthPreview length_preview(const LengthSession& s,const SegmentCreationLength& value){if(s.document)return s.document->preview_length(value);const auto clocks=initial_segment_clocks(value);return {clocks,static_cast<std::uint32_t>((std::int64_t(clocks)+3071)/3072),value.clockTime?value.milliseconds:std::uint64_t(value.measures)*2000};}
SegmentCreationLength length_value(HWND w,const LengthSession& session){SegmentCreationLength value;value.clockTime=IsDlgButtonChecked(w,ClockMode)==BST_CHECKED;if(value.clockTime){const auto minute=number(w,Minutes),second=number(w,Seconds),ms=number(w,Milliseconds);if(second>59||ms>999)throw std::runtime_error("Seconds must be0..59 and milliseconds0..999");value.milliseconds=minute*60000+second*1000+ms;}else value.measures=static_cast<std::uint32_t>(number(w,Measures));(void)length_preview(session,value);return value;}
void update_length(HWND w,LengthSession& s){if(s.updating)return;s.updating=true;try{const bool clock=IsDlgButtonChecked(w,ClockMode)==BST_CHECKED;EnableWindow(GetDlgItem(w,Measures),!clock);EnableWindow(GetDlgItem(w,1000+Measures),!clock);for(auto id:{Minutes,Seconds,Milliseconds}){EnableWindow(GetDlgItem(w,id),clock);EnableWindow(GetDlgItem(w,1000+id),clock);}const auto value=length_value(w,s);const auto preview=length_preview(s,value);const auto ms=preview.milliseconds;SetDlgItemTextW(w,ClockLabel,(L"( "+std::to_wstring(ms/60000)+L":"+(ms/1000%60<10?L"0":L"")+std::to_wstring(ms/1000%60)+L":"+(ms%1000<100?L"0":L"")+(ms%1000<10?L"0":L"")+std::to_wstring(ms%1000)+L" )").c_str());SetDlgItemTextW(w,MeasuresLabel,(L"( "+std::to_wstring(preview.measures)+L" Measures )").c_str());if(!clock){SetDlgItemInt(w,Minutes,static_cast<UINT>(ms/60000),FALSE);SetDlgItemInt(w,Seconds,static_cast<UINT>(ms/1000%60),FALSE);SetDlgItemInt(w,Milliseconds,static_cast<UINT>(ms%1000),FALSE);}else SetDlgItemInt(w,Measures,preview.measures,FALSE);}catch(...){SetDlgItemTextW(w,ClockLabel,L"");SetDlgItemTextW(w,MeasuresLabel,L"");}s.updating=false;}
INT_PTR CALLBACK length_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<LengthSession*>(GetWindowLongPtrW(w,DWLP_USER));try{
    if(msg==WM_INITDIALOG){s=reinterpret_cast<LengthSession*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);s->updating=true;control(w,L"BUTTON",L"Number Measures",BS_AUTORADIOBUTTON|WS_GROUP|WS_TABSTOP,9,29,88,13,MeasuresMode);control(w,L"BUTTON",L"Clocktime",BS_AUTORADIOBUTTON|WS_TABSTOP,9,67,80,13,ClockMode);CheckRadioButton(w,MeasuresMode,ClockMode,MeasuresMode);control(w,L"STATIC",L"Measures",0,102,17,60,12);auto measures=control(w,L"EDIT",L"20",ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,102,29,45,14,Measures);spin(w,measures,INT32_MAX);control(w,L"STATIC",L"",0,153,29,110,14,ClockLabel);const std::array<const wchar_t*,3> labels{L"Minute",L"Second",L"Millisecond"};const std::array<const wchar_t*,3> values{L"0",L"40",L"0"};for(int i=0;i<3;++i){const int x=72+i*53;control(w,L"STATIC",labels[i],0,x,55,50,12);auto edit=control(w,L"EDIT",values[i],ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,x,67,45,14,Minutes+i);spin(w,edit,i==0?INT32_MAX:i==1?59:999);}control(w,L"STATIC",L"",0,232,67,101,14,MeasuresLabel);auto extensionLabel=control(w,L"STATIC",L"Extension bars:",0,9,105,63,12);auto extension=control(w,L"EDIT",L"0",ES_NUMBER,72,104,40,14);auto pickup=control(w,L"BUTTON",L"Display pick-up bar",BS_AUTOCHECKBOX,9,122,130,13);EnableWindow(extensionLabel,FALSE);EnableWindow(extension,FALSE);EnableWindow(pickup,FALSE);CheckRadioButton(w,MeasuresMode,ClockMode,s->initial.clockTime?ClockMode:MeasuresMode);SetDlgItemInt(w,Measures,s->initial.measures,FALSE);SetDlgItemInt(w,Minutes,static_cast<UINT>(s->initial.milliseconds/60000),FALSE);SetDlgItemInt(w,Seconds,static_cast<UINT>(s->initial.milliseconds/1000%60),FALSE);SetDlgItemInt(w,Milliseconds,static_cast<UINT>(s->initial.milliseconds%1000),FALSE);control(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,274,8,58,15,IDOK);control(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,274,27,58,15,IDCANCEL);s->updating=false;update_length(w,*s);return TRUE;}
    if(msg==WM_COMMAND&&s){const int id=LOWORD(wp);if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}if(id==IDOK){s->accepted=length_value(w,*s);EndDialog(w,IDOK);return TRUE;}if(id==MeasuresMode||id==ClockMode||((id>=Measures&&id<=Milliseconds)&&HIWORD(wp)==EN_CHANGE)){update_length(w,*s);return TRUE;}}
}catch(const std::exception& e){MessageBoxA(w,e.what(),"Segment Length",MB_OK|MB_ICONINFORMATION);if(msg==WM_INITDIALOG)EndDialog(w,IDCANCEL);}return FALSE;}
struct TracksSession{std::optional<std::vector<SegmentTrackKind>> accepted;};
INT_PTR CALLBACK tracks_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<TracksSession*>(GetWindowLongPtrW(w,DWLP_USER));try{
    if(msg==WM_INITDIALOG){
        s=reinterpret_cast<TracksSession*>(lp);SetWindowLongPtrW(w,DWLP_USER,lp);
        auto list=control(w,L"LISTBOX",L"",LBS_EXTENDEDSEL|LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|WS_VSCROLL|WS_TABSTOP,7,8,116,163,List);
        const auto& types=segment_track_descriptors();
        for(size_t i=0;i<types.size();++i){
            SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(types[i].name));
            if(types[i].kind==SegmentTrackKind::band&&types[i].canCreate){
                SendMessageW(list,LB_SETSEL,TRUE,i);SendMessageW(list,LB_SETCARETINDEX,i,FALSE);
            }
        }
        control(w,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,138,8,58,15,IDOK);
        control(w,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,138,27,58,15,IDCANCEL);SetFocus(list);return FALSE;
    }
    if(msg==WM_MEASUREITEM){auto m=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);HDC dc=GetDC(w);auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(w,WM_GETFONT,0,0)));TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);m->itemHeight=tm.tmHeight;SelectObject(dc,old);ReleaseDC(w,dc);return TRUE;}
    if(msg==WM_DRAWITEM){const auto d=reinterpret_cast<DRAWITEMSTRUCT*>(lp);const auto& types=segment_track_descriptors();if(d->itemID>=types.size())return TRUE;const bool available=types[d->itemID].canCreate,selected=available&&(d->itemState&ODS_SELECTED);FillRect(d->hDC,&d->rcItem,reinterpret_cast<HBRUSH>((selected?COLOR_HIGHLIGHT:COLOR_WINDOW)+1));SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,GetSysColor(!available?COLOR_GRAYTEXT:selected?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));RECT r=d->rcItem;r.left+=2;DrawTextW(d->hDC,types[d->itemID].name,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);return TRUE;}
    if(msg==WM_COMMAND&&s){const int id=LOWORD(wp);if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}const auto& types=segment_track_descriptors();if(id==List&&HIWORD(wp)==LBN_SELCHANGE){for(size_t i=0;i<types.size();++i)if(!types[i].canCreate)SendDlgItemMessageW(w,List,LB_SETSEL,FALSE,i);return TRUE;}if(id==IDOK){std::vector<SegmentTrackKind> value;for(size_t i=0;i<types.size();++i)if(SendDlgItemMessageW(w,List,LB_GETSEL,i,0)>0){if(!types[i].canCreate)throw std::runtime_error("Select an available track type");value.push_back(types[i].kind);}s->accepted=std::move(value);EndDialog(w,IDOK);return TRUE;}}
}catch(const std::exception& e){MessageBoxA(w,e.what(),"Add New Tracks",MB_OK|MB_ICONINFORMATION);if(msg==WM_INITDIALOG)EndDialog(w,IDCANCEL);}return FALSE;}
}
std::optional<NewFileSelection> show_new_files(HWND owner,DocumentKind initial){FileSession s{initial,{}};modal(owner,L"Create New File(s)",160,218,files_proc,&s);return s.accepted;}
std::optional<SegmentCreationLength> show_segment_creation_length(HWND owner){LengthSession s;modal(owner,L"Segment Length",340,144,length_proc,&s);return s.accepted;}
std::optional<SegmentCreationLength> show_segment_length(HWND owner,const SegmentDocument& document){LengthSession s;s.document=&document;s.initial=document.length_settings();modal(owner,L"Segment Length",340,144,length_proc,&s);return s.accepted;}
std::optional<std::vector<SegmentTrackKind>> show_add_tracks(HWND owner,const std::wstring& name){TracksSession s;modal(owner,name+L" - Add New Tracks",204,180,tracks_proc,&s);return s.accepted;}
}
