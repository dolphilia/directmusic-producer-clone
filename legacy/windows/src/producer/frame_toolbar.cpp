#include "frame_toolbar.h"
#include <commctrl.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cwchar>
#include <cmath>

namespace producer::app { namespace {
void place_toolbar_control(HWND control,int x,int y,int width,int height){
    // Embedded fields are siblings of the toolbar so their notifications go
    // directly to the frame. Keep them above its full-width hit-test surface.
    SetWindowPos(control,HWND_TOP,x,y,width,height,SWP_NOACTIVATE);
}
bool change_toolbar_flag(HWND window,UINT command,BYTE flag,bool value,UINT message){
    const auto state=SendMessageW(window,TB_GETSTATE,command,0);
    if(state<0||bool(state&flag)==value)return false;
    SendMessageW(window,message,command,MAKELPARAM(value,0));return true;
}
void enable_control(HWND window,bool value){if(bool(IsWindowEnabled(window))!=value)EnableWindow(window,value);}
void select_combo(HWND window,WPARAM value){
    if(SendMessageW(window,CB_GETCURSEL,0,0)!=static_cast<LRESULT>(value))SendMessageW(window,CB_SETCURSEL,value,0);
}
int window_icon(HIMAGELIST images,int kind){
    // Small source-drawn window glyphs; no Producer PE/resource dependency.
    const auto screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    const auto bitmap=CreateCompatibleBitmap(screen,16,16);ReleaseDC(nullptr,screen);
    if(!dc||!bitmap){if(dc)DeleteDC(dc);if(bitmap)DeleteObject(bitmap);throw std::runtime_error("Toolbar glyph allocation failed");}
    const auto old=SelectObject(dc,bitmap);const COLORREF mask=RGB(255,0,255);
    const auto background=CreateSolidBrush(mask);RECT bounds{0,0,16,16};FillRect(dc,&bounds,background);DeleteObject(background);
    const auto fill=CreateSolidBrush(RGB(255,255,224)),blue=CreateSolidBrush(RGB(0,0,128));
    RECT frame{1,2,15,14};FillRect(dc,&frame,fill);FrameRect(dc,&frame,blue);
    RECT title{2,3,14,5};FillRect(dc,&title,blue);
    if(kind==3){
        ImageList_Draw(images,STD_FILESAVE,dc,0,0,ILD_TRANSPARENT);
        ImageList_Draw(images,STD_FILESAVE,dc,3,2,ILD_TRANSPARENT);
    }else if(kind==0){
        const auto pen=CreatePen(PS_SOLID,1,RGB(0,0,0));const auto previous=SelectObject(dc,pen);
        MoveToEx(dc,5,7,nullptr);LineTo(dc,5,12);MoveToEx(dc,5,8,nullptr);LineTo(dc,10,8);MoveToEx(dc,5,11,nullptr);LineTo(dc,10,11);
        SelectObject(dc,previous);DeleteObject(pen);
    }else if(kind==1){RECT line{2,8,14,10};FillRect(dc,&line,blue);}
    else {RECT line{7,5,9,13};FillRect(dc,&line,blue);}
    DeleteObject(fill);DeleteObject(blue);SelectObject(dc,old);DeleteDC(dc);
    const int result=ImageList_AddMasked(images,bitmap,mask);DeleteObject(bitmap);
    if(result<0)throw std::runtime_error("Toolbar glyph insertion failed");return result;
}
}
void StandardToolbar::create(HWND parent,UINT controlId,const StandardToolbarCommands& commands){
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_BAR_CLASSES};
    if(!InitCommonControlsEx(&common))throw std::runtime_error("Toolbar control initialization failed");
    commands_=commands;
    window_=CreateWindowExW(0,TOOLBARCLASSNAMEW,L"Standard Toolbar",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|TBSTYLE_LIST|CCS_NODIVIDER|CCS_NORESIZE|CCS_NOPARENTALIGN,
        0,0,1,30,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId)),GetModuleHandleW(nullptr),nullptr);
    if(!window_)throw std::runtime_error("Standard Toolbar creation failed");
    SendMessageW(window_,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
    // Keep names available to accessibility/tooltips without drawing captions.
    SendMessageW(window_,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);
    SendMessageW(window_,TB_SETMAXTEXTROWS,0,0);
    SendMessageW(window_,TB_SETBITMAPSIZE,0,MAKELPARAM(16,16));
    SendMessageW(window_,TB_SETBUTTONSIZE,0,MAKELPARAM(25,26));
    SendMessageW(window_,TB_LOADIMAGES,IDB_STD_SMALL_COLOR,reinterpret_cast<LPARAM>(HINST_COMMCTRL));
    const auto images=reinterpret_cast<HIMAGELIST>(SendMessageW(window_,TB_GETIMAGELIST,0,0));
    if(!images)throw std::runtime_error("Standard Toolbar images unavailable");
    const int tree=window_icon(images,0),horizontal=window_icon(images,1),vertical=window_icon(images,2),projectSave=window_icon(images,3);
    std::array<TBBUTTON,10> buttons{};size_t count=0;
    const auto add=[&](UINT id,int image,const wchar_t* label,BYTE style=BTNS_BUTTON){
        auto& b=buttons[count++];b.iBitmap=image;b.idCommand=id;b.fsState=TBSTATE_ENABLED;b.fsStyle=style;
        b.iString=SendMessageW(window_,TB_ADDSTRINGW,0,reinterpret_cast<LPARAM>(label));
    };
    const auto separator=[&]{auto& b=buttons[count++];b.fsStyle=BTNS_SEP;b.iBitmap=8;};
    add(commands.newFile,STD_FILENEW,L"New\0");add(commands.openProject,STD_FILEOPEN,L"Open Project\0");
    add(commands.insertFiles,STD_COPY,L"Insert Files\0");separator();
    add(commands.saveProject,projectSave,L"Save Project\0");add(commands.saveFile,STD_FILESAVE,L"Save File\0");separator();
    add(commands.projectTree,tree,L"Toggle Project Tree\0",BTNS_CHECK);
    add(commands.tileHorizontal,horizontal,L"Tile Horizontally\0");add(commands.tileVertical,vertical,L"Tile Vertically\0");
    if(!SendMessageW(window_,TB_ADDBUTTONSW,count,reinterpret_cast<LPARAM>(buttons.data())))throw std::runtime_error("Standard Toolbar buttons failed");
}
void StandardToolbar::show(bool visible){visible_=visible;if(window_)ShowWindow(window_,visible?SW_SHOW:SW_HIDE);}
void StandardToolbar::layout(int width){if(window_)MoveWindow(window_,0,0,std::max(1,width),30,TRUE);}
POINT StandardToolbar::new_menu_position()const{
    RECT bounds{};SendMessageW(window_,TB_GETRECT,commands_.newFile,reinterpret_cast<LPARAM>(&bounds));
    POINT point{bounds.left,bounds.bottom};MapWindowPoints(window_,nullptr,&point,1);return point;
}
void StandardToolbar::refresh(bool canSaveFile,bool projectTreeVisible,bool canTile){
    if(!window_)return;
    SendMessageW(window_,TB_ENABLEBUTTON,commands_.saveFile,MAKELPARAM(canSaveFile,0));
    SendMessageW(window_,TB_CHECKBUTTON,commands_.projectTree,MAKELPARAM(projectTreeVisible,0));
    SendMessageW(window_,TB_ENABLEBUTTON,commands_.tileHorizontal,MAKELPARAM(canTile,0));
    SendMessageW(window_,TB_ENABLEBUTTON,commands_.tileVertical,MAKELPARAM(canTile,0));
}
namespace {
enum class TransportGlyph {Play,FromStart,Stop,BoundaryStop};
int transport_icon(HIMAGELIST images,TransportGlyph kind){
    const auto screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    const auto bitmap=CreateCompatibleBitmap(screen,16,16);ReleaseDC(nullptr,screen);
    if(!dc||!bitmap){if(dc)DeleteDC(dc);if(bitmap)DeleteObject(bitmap);throw std::runtime_error("Transport glyph allocation failed");}
    const auto old=SelectObject(dc,bitmap);const COLORREF mask=RGB(255,0,255);
    const auto background=CreateSolidBrush(mask);RECT rect{0,0,16,16};FillRect(dc,&rect,background);DeleteObject(background);
    const bool stop=kind==TransportGlyph::Stop||kind==TransportGlyph::BoundaryStop;
    const auto color=stop?RGB(180,0,0):RGB(0,130,0);
    const auto brush=CreateSolidBrush(color);
    const auto pen=CreatePen(PS_SOLID,1,color);
    const auto oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);
    if(stop)Rectangle(dc,3,3,12,13);else {POINT points[3]{{4,2},{13,8},{4,14}};Polygon(dc,points,3);}
    // The start marker and boundary marker make each paired command distinct.
    // These are source-drawn glyphs, with no original PE/resource dependency.
    if(kind==TransportGlyph::FromStart){RECT marker{1,2,3,14};FillRect(dc,&marker,brush);}
    if(kind==TransportGlyph::BoundaryStop){RECT marker{13,3,15,13};FillRect(dc,&marker,brush);}
    SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);DeleteObject(brush);SelectObject(dc,old);DeleteDC(dc);
    const int result=ImageList_AddMasked(images,bitmap,mask);DeleteObject(bitmap);
    if(result<0)throw std::runtime_error("Transport glyph insertion failed");return result;
}
int transport_option_icon(HIMAGELIST images,unsigned kind){
    const auto screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    const auto bitmap=CreateCompatibleBitmap(screen,16,16);ReleaseDC(nullptr,screen);
    if(!dc||!bitmap){if(dc)DeleteDC(dc);if(bitmap)DeleteObject(bitmap);throw std::runtime_error("Transport option glyph allocation failed");}
    const auto old=SelectObject(dc,bitmap);const COLORREF mask=RGB(255,0,255);
    const auto background=CreateSolidBrush(mask);RECT bounds{0,0,16,16};FillRect(dc,&bounds,background);DeleteObject(background);
    const auto pen=CreatePen(PS_SOLID,1,kind==2?RGB(96,96,0):RGB(0,0,0));
    const auto brush=CreateSolidBrush(kind==0?RGB(128,128,128):kind==2?RGB(128,128,0):RGB(0,0,0));
    const auto oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,brush);
    if(kind==0){
        POINT body[]{{3,13},{7,2},{12,13}};Polygon(dc,body,3);
        MoveToEx(dc,5,3,nullptr);LineTo(dc,13,13);MoveToEx(dc,3,14,nullptr);LineTo(dc,13,14);
    }else if(kind==1){
        for(int x:{3,7,11}){MoveToEx(dc,x,x==3?3:2,nullptr);LineTo(dc,x,14);}
        for(int y=3;y<14;++y)SetPixelV(dc,3,y,RGB(160,160,160));
        MoveToEx(dc,12,2,nullptr);LineTo(dc,12,14);
    }else if(kind==2){
        Ellipse(dc,1,8,6,12);Ellipse(dc,9,10,14,14);
        MoveToEx(dc,5,10,nullptr);LineTo(dc,5,2);LineTo(dc,13,4);LineTo(dc,13,12);
        MoveToEx(dc,5,4,nullptr);LineTo(dc,13,6);
    }else{
        Ellipse(dc,1,1,15,15);
        const auto notchBrush=CreateSolidBrush(mask);RECT notch{6,0,10,4};FillRect(dc,&notch,notchBrush);DeleteObject(notchBrush);
        for(const POINT p:{POINT{4,6},POINT{5,10},POINT{8,12},POINT{11,10},POINT{12,6}}){
            SetPixelV(dc,p.x,p.y,RGB(255,255,255));SetPixelV(dc,p.x,p.y+1,RGB(255,255,255));
        }
    }
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);
    SelectObject(dc,old);DeleteDC(dc);const int result=ImageList_AddMasked(images,bitmap,mask);DeleteObject(bitmap);
    if(result<0)throw std::runtime_error("Transport option glyph insertion failed");return result;
}
struct AlternateTempoSession{unsigned current;std::optional<unsigned> accepted;};
INT_PTR CALLBACK alternate_tempo_proc(HWND window,UINT message,WPARAM w,LPARAM l){
    auto session=reinterpret_cast<AlternateTempoSession*>(GetWindowLongPtrW(window,DWLP_USER));
    if(message==WM_INITDIALOG){
        session=reinterpret_cast<AlternateTempoSession*>(l);SetWindowLongPtrW(window,DWLP_USER,l);
        const auto add=[&](const wchar_t* cls,const wchar_t* text,DWORD style,RECT rect,UINT id){
            MapDialogRect(window,&rect);const auto control=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,window,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
            SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return control;
        };
        add(L"STATIC",L"Alternate Tempo",0,{10,10,57,20},0);
        const auto edit=add(L"EDIT",std::to_wstring(session->current).c_str(),WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL,{59,10,90,18},10);
        SendMessageW(edit,EM_SETLIMITTEXT,5,0);
        const auto spin=add(UPDOWN_CLASSW,L"",UDS_ARROWKEYS|UDS_ALIGNRIGHT|UDS_AUTOBUDDY|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,{0,0,0,0},11);
        SendMessageW(spin,UDM_SETRANGE32,1,10000);SendMessageW(spin,UDM_SETPOS32,0,session->current);
        add(L"STATIC",L"%",0,{92,10,100,20},0);
        add(L"BUTTON",L"OK",WS_TABSTOP|BS_DEFPUSHBUTTON,{114,5,157,14},IDOK);
        add(L"BUTTON",L"Cancel",WS_TABSTOP,{114,16,157,25},IDCANCEL);
        SetFocus(edit);SendMessageW(edit,EM_SETSEL,0,-1);return FALSE;
    }
    if(message==WM_COMMAND&&session){
        if(LOWORD(w)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}
        if(LOWORD(w)==IDOK){
            wchar_t text[32]{};GetDlgItemTextW(window,10,text,std::size(text));unsigned value=0;bool valid=*text!=0;
            for(const wchar_t* p=text;*p;++p){if(*p<L'0'||*p>L'9'){valid=false;break;}value=value*10+(*p-L'0');if(value>10000){valid=false;break;}}
            if(!valid||value<1){MessageBoxW(window,L"Enter a percentage from 1 to 10000.",L"Alternate Tempo",MB_OK|MB_ICONINFORMATION);SetFocus(GetDlgItem(window,10));return TRUE;}
            session->accepted=value;EndDialog(window,IDOK);return TRUE;
        }
    }
    return FALSE;
}
}
void TransportToolbar::create(HWND parent,UINT controlId,const TransportToolbarCommands& commands){
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&common))throw std::runtime_error("Tempo spin initialization failed");
    commands_=commands;window_=CreateWindowExW(0,TOOLBARCLASSNAMEW,L"Transport Controls",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|TBSTYLE_LIST|CCS_NODIVIDER|CCS_NORESIZE|CCS_NOPARENTALIGN,0,0,1,30,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId)),GetModuleHandleW(nullptr),nullptr);
    if(!window_)throw std::runtime_error("Transport toolbar creation failed");
    SendMessageW(window_,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);SendMessageW(window_,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);SendMessageW(window_,TB_SETMAXTEXTROWS,1,0);SendMessageW(window_,TB_SETBITMAPSIZE,0,MAKELPARAM(16,16));SendMessageW(window_,TB_SETBUTTONSIZE,0,MAKELPARAM(25,26));
    SendMessageW(window_,TB_LOADIMAGES,IDB_STD_SMALL_COLOR,reinterpret_cast<LPARAM>(HINST_COMMCTRL));
    const auto images=reinterpret_cast<HIMAGELIST>(SendMessageW(window_,TB_GETIMAGELIST,0,0));
    if(!images)throw std::runtime_error("Transport images unavailable");
    playImage_=transport_icon(images,TransportGlyph::Play);stopImage_=transport_icon(images,TransportGlyph::Stop);
    const auto fromStartImage=transport_icon(images,TransportGlyph::FromStart),boundaryStopImage=transport_icon(images,TransportGlyph::BoundaryStop);
    std::array<TBBUTTON,7> buttons{};
    buttons[0].idCommand=commands.primaryList;buttons[0].fsStyle=BTNS_SEP;buttons[0].iBitmap=156;
    buttons[1]={fromStartImage,static_cast<int>(commands.fromStart),TBSTATE_ENABLED,BTNS_BUTTON,{0},0,SendMessageW(window_,TB_ADDSTRINGW,0,reinterpret_cast<LPARAM>(L"Play From Start\0"))};
    buttons[2]={playImage_,static_cast<int>(commands.play),TBSTATE_ENABLED,BTNS_BUTTON,{0},0,SendMessageW(window_,TB_ADDSTRINGW,0,reinterpret_cast<LPARAM>(L"Play\0"))};
    buttons[3]={stopImage_,static_cast<int>(commands.stop),TBSTATE_HIDDEN,BTNS_BUTTON,{0},0,SendMessageW(window_,TB_ADDSTRINGW,0,reinterpret_cast<LPARAM>(L"Stop Immediately\0"))};
    buttons[4]={boundaryStopImage,static_cast<int>(commands.boundaryStop),TBSTATE_HIDDEN,BTNS_BUTTON,{0},0,SendMessageW(window_,TB_ADDSTRINGW,0,reinterpret_cast<LPARAM>(L"Stop at Boundary\0"))};
    // Reserve the Current Tempo field before the audition speed modifier,
    // matching the original left-to-right command order.
    buttons[5].idCommand=commands.tempoEdit;buttons[5].fsStyle=BTNS_SEP;buttons[5].iBitmap=72;
    buttons[6]={I_IMAGENONE,static_cast<int>(commands.tempoModifier),TBSTATE_ENABLED,static_cast<BYTE>(BTNS_CHECK|BTNS_SHOWTEXT|BTNS_AUTOSIZE),{0},0,reinterpret_cast<INT_PTR>(L"x 50%")};
    if(!SendMessageW(window_,TB_ADDBUTTONSW,buttons.size(),reinterpret_cast<LPARAM>(buttons.data())))throw std::runtime_error("Transport buttons failed");
    tempo_=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,140,7,65,22,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(commands.tempoEdit)),GetModuleHandleW(nullptr),nullptr);
    tempoSpin_=CreateWindowW(UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ARROWKEYS|UDS_NOTHOUSANDS,205,7,16,22,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(commands.tempoSpin)),GetModuleHandleW(nullptr),nullptr);
    primaryList_=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST,4,7,150,200,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(commands.primaryList)),GetModuleHandleW(nullptr),nullptr);
    if(!tempo_||!tempoSpin_||!primaryList_)throw std::runtime_error("Transport embedded control failed");
    for(const auto control:{tempo_,primaryList_})SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
    SendMessageW(tempo_,EM_SETLIMITTEXT,16,0);SendMessageW(tempoSpin_,UDM_SETBUDDY,reinterpret_cast<WPARAM>(tempo_),0);SendMessageW(tempoSpin_,UDM_SETRANGE32,1,1000);
}
void TransportToolbar::show(bool visible){visible_=visible;for(const auto child:{window_,tempo_,tempoSpin_,primaryList_})if(child)ShowWindow(child,visible?SW_SHOW:SW_HIDE);}
void TransportToolbar::layout_tempo(){
    if(!window_||!tempo_)return;
    RECT slot{};SendMessageW(window_,TB_GETRECT,commands_.tempoEdit,reinterpret_cast<LPARAM>(&slot));
    place_toolbar_control(tempo_,slot.left+4,top_+4,48,22);
    place_toolbar_control(tempoSpin_,slot.left+52,top_+4,16,22);
    if(primaryList_)place_toolbar_control(primaryList_,4,top_+4,150,200);
}
void TransportToolbar::layout(int width,int top){top_=top;if(window_)MoveWindow(window_,0,top,std::max(1,width),30,TRUE);layout_tempo();}
bool TransportToolbar::modifier_context(const NMHDR& notification)const{
    return notification.hwndFrom==window_&&notification.code==NM_RCLICK&&reinterpret_cast<const NMMOUSE&>(notification).dwItemSpec==commands_.tempoModifier;
}
void TransportToolbar::refresh(bool canPlay,bool playing,bool modifierEnabled,unsigned percent,std::optional<double> tempo,bool restorePending){
    if(!window_)return;
    bool geometryChanged=false;
    geometryChanged|=change_toolbar_flag(window_,commands_.fromStart,TBSTATE_HIDDEN,playing,TB_HIDEBUTTON);
    geometryChanged|=change_toolbar_flag(window_,commands_.play,TBSTATE_HIDDEN,playing,TB_HIDEBUTTON);
    geometryChanged|=change_toolbar_flag(window_,commands_.stop,TBSTATE_HIDDEN,!playing,TB_HIDEBUTTON);
    geometryChanged|=change_toolbar_flag(window_,commands_.boundaryStop,TBSTATE_HIDDEN,!playing,TB_HIDEBUTTON);
    change_toolbar_flag(window_,commands_.fromStart,TBSTATE_ENABLED,canPlay&&!playing,TB_ENABLEBUTTON);
    change_toolbar_flag(window_,commands_.play,TBSTATE_ENABLED,canPlay&&!playing,TB_ENABLEBUTTON);
    change_toolbar_flag(window_,commands_.boundaryStop,TBSTATE_ENABLED,playing,TB_ENABLEBUTTON);
    change_toolbar_flag(window_,commands_.stop,TBSTATE_ENABLED,playing,TB_ENABLEBUTTON);
    change_toolbar_flag(window_,commands_.tempoModifier,TBSTATE_CHECKED,modifierEnabled,TB_CHECKBUTTON);
    const auto text=L"x "+std::to_wstring(percent)+L"%";wchar_t current[32]{};
    TBBUTTONINFOW info{sizeof(info),TBIF_TEXT};info.pszText=current;info.cchText=std::size(current);
    if(SendMessageW(window_,TB_GETBUTTONINFOW,commands_.tempoModifier,reinterpret_cast<LPARAM>(&info))<0||text!=current){
        info.pszText=const_cast<wchar_t*>(text.c_str());SendMessageW(window_,TB_SETBUTTONINFOW,commands_.tempoModifier,reinterpret_cast<LPARAM>(&info));geometryChanged=true;
    }
    noPrimary_=!canPlay;displayedTempo_=tempo;enable_control(tempo_,tempo.has_value()&&!playing);enable_control(tempoSpin_,tempo.has_value()&&!playing);
    // The100ms playback timer must not keep moving sibling fields to HWND_TOP
    // or erasing unchanged text. Frame resize/show still lays them out explicitly.
    if(geometryChanged)layout_tempo();
    if(restorePending||GetFocus()!=tempo_)restore_tempo();
}
void TransportToolbar::refresh_primary_list(const std::vector<std::wstring>& labels,std::optional<size_t> selected,bool enabled){
    if(!primaryList_)return;
    if(labels!=primaryLabels_){
        SendMessageW(primaryList_,CB_RESETCONTENT,0,0);
        for(const auto& label:labels)SendMessageW(primaryList_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));
        primaryLabels_=labels;
    }
    select_combo(primaryList_,selected&&*selected<labels.size()?static_cast<WPARAM>(*selected):static_cast<WPARAM>(-1));
    enable_control(primaryList_,enabled&&!labels.empty());
}
std::optional<size_t> TransportToolbar::primary_selection()const{
    const auto selected=primaryList_?SendMessageW(primaryList_,CB_GETCURSEL,0,0):CB_ERR;
    return selected>=0?std::optional<size_t>(static_cast<size_t>(selected)):std::nullopt;
}
std::optional<double> TransportToolbar::tempo_value()const{
    wchar_t text[32]{};GetWindowTextW(tempo_,text,std::size(text));double value=0,scale=1;bool point=false,digit=false;unsigned decimals=0;
    for(const auto* p=text;*p;++p){if(*p==L'.'&&!point){point=true;continue;}if(*p<L'0'||*p>L'9')return {};digit=true;if(point){if(++decimals>2)return {};scale*=10;value+=(*p-L'0')/scale;}else value=value*10+(*p-L'0');}
    // Numeric UI input is normalized to the original's1..1000 range.
    // Empty/malformed text is rejected; the document API still validates
    // every caller rather than accepting an out-of-range stored tempo.
    return digit?std::optional<double>(std::clamp(value,1.0,1000.0)):std::nullopt;
}
void TransportToolbar::restore_tempo(){std::wstring label=noPrimary_?L"120":L"";if(displayedTempo_){wchar_t text[32]{};swprintf_s(text,L"%.2f",*displayedTempo_);label=text;}wchar_t current[64]{};GetWindowTextW(tempo_,current,std::size(current));if(label!=current)SetWindowTextW(tempo_,label.c_str());}
bool TransportToolbar::translate(const MSG& message)const{
    if(message.hwnd!=tempo_||message.message!=WM_KEYDOWN||(GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_MENU)&0x8000))return false;
    if(message.wParam!=VK_RETURN&&message.wParam!=VK_ESCAPE)return false;
    // The original keeps pending text on Escape; leaving the field later
    // commits it. Escape must neither edit the document nor restore the text.
    if(message.wParam==VK_RETURN)SendMessageW(GetParent(tempo_),WM_COMMAND,commands_.tempoCommit,0);return true;
}
bool TransportToolbar::spin(const NMHDR& notification){
    if(notification.hwndFrom!=tempoSpin_||notification.code!=UDN_DELTAPOS)return false;
    const auto& delta=reinterpret_cast<const NMUPDOWN&>(notification);if(!IsWindowEnabled(tempoSpin_))return true;
    const auto value=std::clamp(std::floor(tempo_value().value_or(displayedTempo_.value_or(120)))+delta.iDelta,1.0,1000.0);
    SetWindowTextW(tempo_,std::to_wstring(static_cast<unsigned>(value)).c_str());SendMessageW(GetParent(tempo_),WM_COMMAND,commands_.tempoCommit,0);return true;
}
void TransportOptionsToolbar::create(HWND parent,UINT controlId,const TransportOptionsCommands& commands){
    commands_=commands;
    window_=CreateWindowExW(0,TOOLBARCLASSNAMEW,L"Transport Options",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|TBSTYLE_LIST|CCS_NODIVIDER|CCS_NORESIZE|CCS_NOPARENTALIGN,0,0,1,30,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId)),GetModuleHandleW(nullptr),nullptr);
    if(!window_)throw std::runtime_error("Transport Options toolbar creation failed");SendMessageW(window_,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);SendMessageW(window_,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);SendMessageW(window_,TB_SETMAXTEXTROWS,0,0);
    SendMessageW(window_,TB_SETBITMAPSIZE,0,MAKELPARAM(16,16));SendMessageW(window_,TB_SETBUTTONSIZE,0,MAKELPARAM(23,26));
    SendMessageW(window_,TB_LOADIMAGES,IDB_STD_SMALL_COLOR,reinterpret_cast<LPARAM>(HINST_COMMCTRL));
    const auto images=reinterpret_cast<HIMAGELIST>(SendMessageW(window_,TB_GETIMAGELIST,0,0));
    if(!images)throw std::runtime_error("Transport Options images unavailable");
    std::array<TBBUTTON,5> buttons{};buttons[0].fsStyle=BTNS_SEP;buttons[0].iBitmap=156;
    const std::array<UINT,4> ids{commands.metronome,commands.playCursor,commands.midiEcho,commands.midiOptions};const std::array<const wchar_t*,4> texts{L"Metronome",L"Play Cursor",L"MIDI Echo",L"MIDI/Performance Options"};
    for(size_t i=0;i<ids.size();++i)buttons[i+1]={transport_option_icon(images,static_cast<unsigned>(i)),static_cast<int>(ids[i]),static_cast<BYTE>(TBSTATE_ENABLED),static_cast<BYTE>(i<=2?BTNS_CHECK:BTNS_BUTTON),{0},0,reinterpret_cast<INT_PTR>(texts[i])};
    if(!SendMessageW(window_,TB_ADDBUTTONSW,buttons.size(),reinterpret_cast<LPARAM>(buttons.data())))throw std::runtime_error("Transport Options buttons failed");
    paths_=CreateWindowExW(0,L"COMBOBOX",L"AudioPath",WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,8,0,148,300,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(commands.audioPath)),GetModuleHandleW(nullptr),nullptr);
    if(!paths_)throw std::runtime_error("AudioPath list creation failed");SendMessageW(paths_,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
}
void TransportOptionsToolbar::show(bool show){visible_=show;if(window_)ShowWindow(window_,show?SW_SHOW:SW_HIDE);if(paths_)ShowWindow(paths_,show?SW_SHOW:SW_HIDE);}
void TransportOptionsToolbar::layout(int width,int top){if(window_)MoveWindow(window_,0,top,std::max(1,width),30,TRUE);if(paths_)place_toolbar_control(paths_,8,top+3,148,300);}
bool TransportOptionsToolbar::metronome_context(const NMHDR& n)const{return n.hwndFrom==window_&&n.code==NM_RCLICK&&reinterpret_cast<const NMMOUSE&>(n).dwItemSpec==commands_.metronome;}
void TransportOptionsToolbar::refresh(const std::vector<std::wstring>& labels,size_t selected,bool metronome,bool playCursor,bool midiEcho){
    if(window_){change_toolbar_flag(window_,commands_.metronome,TBSTATE_CHECKED,metronome,TB_CHECKBUTTON);change_toolbar_flag(window_,commands_.playCursor,TBSTATE_CHECKED,playCursor,TB_CHECKBUTTON);change_toolbar_flag(window_,commands_.midiEcho,TBSTATE_CHECKED,midiEcho,TB_CHECKBUTTON);}
    if(!paths_)return;if(labels_!=labels){labels_=labels;SendMessageW(paths_,CB_RESETCONTENT,0,0);for(const auto& text:labels)SendMessageW(paths_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}select_combo(paths_,selected);
}
size_t TransportOptionsToolbar::selection()const{return static_cast<size_t>(SendMessageW(paths_,CB_GETCURSEL,0,0));}
std::optional<unsigned> show_alternate_tempo(HWND owner,unsigned percent){
    // DWORD-aligned, zero-child template; controls use dialog units at init.
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&common))throw std::runtime_error("Alternate Tempo spin initialization failed");
    std::array<DWORD,64> storage{};auto dialog=reinterpret_cast<DLGTEMPLATE*>(storage.data());
    dialog->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER;dialog->cx=163;dialog->cy=31;
    auto text=reinterpret_cast<WORD*>(dialog+1);*text++=0;*text++=0;
    for(const auto c:std::wstring(L"Alternate Tempo"))*text++=c;*text=0;
    AlternateTempoSession session{percent,{}};if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),dialog,owner,alternate_tempo_proc,reinterpret_cast<LPARAM>(&session))==-1)throw std::runtime_error("Alternate Tempo dialog creation failed");
    return session.accepted;
}

}
