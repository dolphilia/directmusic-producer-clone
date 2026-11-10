#include "chord_keyboard.h"
#include <windowsx.h>
#include <algorithm>
#include <string>
#include <stdexcept>

namespace producer::app {
namespace {
bool black(unsigned note){const auto n=note%12;return n==1||n==3||n==6||n==8||n==10;}
RECT key_rect(unsigned note,int width,int height,bool scale){
    const int count=scale?7:28;int whites=0;for(unsigned i=0;i<note;++i)if(!black(i))++whites;
    if(!black(note))return {whites*width/count,0,(whites+1)*width/count,height};
    const int boundary=whites*width/count,keyWidth=std::max(2,width/count*2/3);
    return {boundary-keyWidth/2,0,boundary+(keyWidth+1)/2,height*3/5};
}
void fill(HDC dc,const RECT& r,COLORREF color){const auto brush=CreateSolidBrush(color);FillRect(dc,&r,brush);DeleteObject(brush);FrameRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));}
}
void ChordKeyboard::create(HWND owner,UINT id,bool scale,int x,int y,int width,int height,std::function<void(unsigned)> clicked){
    static bool registered=false;if(!registered){WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.hCursor=LoadCursorW(nullptr,IDC_HAND);c.lpszClassName=L"SourceProducerChordKeyboard";if(!RegisterClassW(&c))throw std::runtime_error("Chord keyboard registration failed");registered=true;}
    scale_=scale;clicked_=std::move(clicked);window_=CreateWindowW(L"SourceProducerChordKeyboard",scale?L"Scale":L"Chord structure",WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,width,height,owner,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),GetModuleHandleW(nullptr),this);
    if(!window_)throw std::runtime_error("Chord keyboard creation failed");
}
void ChordKeyboard::set(std::uint64_t selected,std::uint64_t missing,bool enabled){
    selected_=selected;missing_=missing;EnableWindow(window_,enabled);std::wstring title=scale_?L"Scale pitches:":L"Chord tones:";
    const wchar_t* names[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};
    for(unsigned note=0;note<(scale_?12u:48u);++note)if(selected&(std::uint64_t(1)<<note))title+=L" "+(scale_?L"":std::to_wstring(note/12+1))+names[note%12];
    if(!enabled)title+=L" (unavailable)";SetWindowTextW(window_,title.c_str());InvalidateRect(window_,nullptr,FALSE);
}
void ChordKeyboard::show(bool visible)const{ShowWindow(window_,visible?SW_SHOW:SW_HIDE);}
int ChordKeyboard::hit(int x,int y)const{
    RECT client{};GetClientRect(window_,&client);POINT point{x,y};
    for(int pass=0;pass<2;++pass)for(unsigned note=0;note<(scale_?12u:48u);++note)if(black(note)==(pass==0)){const auto rect=key_rect(note,client.right,client.bottom,scale_);if(PtInRect(&rect,point))return static_cast<int>(note);}
    return -1;
}
void ChordKeyboard::paint()const{
    PAINTSTRUCT ps{};const auto dc=BeginPaint(window_,&ps);RECT client{};GetClientRect(window_,&client);const bool enabled=IsWindowEnabled(window_)!=FALSE;
    for(int pass=0;pass<2;++pass)for(unsigned note=0;note<(scale_?12u:48u);++note)if(black(note)==(pass==1)){
        const auto rect=key_rect(note,client.right,client.bottom,scale_);const auto bit=std::uint64_t(1)<<note;
        const COLORREF color=!enabled?GetSysColor(COLOR_BTNFACE):selected_&bit?RGB(255,0,0):missing_&bit?RGB(0,96,255):black(note)?RGB(0,0,0):!scale_&&note<24?RGB(255,255,128):RGB(255,255,255);
        fill(dc,rect,color);
        if(!scale_&&note%12==0){auto label=rect;label.top=label.bottom-13;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(0,0,0));const auto value=std::to_wstring(note/12+1);DrawTextW(dc,value.c_str(),-1,&label,DT_CENTER|DT_BOTTOM|DT_SINGLELINE);}
    }
    if(GetFocus()==window_){auto rect=key_rect(caret_,client.right,client.bottom,scale_);InflateRect(&rect,-2,-2);DrawFocusRect(dc,&rect);}EndPaint(window_,&ps);
}
LRESULT CALLBACK ChordKeyboard::proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    auto self=reinterpret_cast<ChordKeyboard*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE){self=static_cast<ChordKeyboard*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window_=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(self){
        if(message==WM_PAINT){self->paint();return 0;}
        if(message==WM_ERASEBKGND)return 1;
        if(message==WM_GETDLGCODE)return DLGC_WANTARROWS|DLGC_WANTCHARS;
        if(message==WM_SETFOCUS||message==WM_KILLFOCUS||message==WM_ENABLE){InvalidateRect(window,nullptr,FALSE);return 0;}
        if(message==WM_LBUTTONDOWN){const auto note=self->hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));if(note>=0&&IsWindowEnabled(window)){SetFocus(window);self->caret_=static_cast<unsigned>(note);if(self->clicked_)self->clicked_(self->caret_);}return 0;}
        if(message==WM_KEYDOWN){if(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_HOME||wp==VK_END){const auto last=self->scale_?11u:47u;self->caret_=wp==VK_HOME?0:wp==VK_END?last:wp==VK_LEFT?(self->caret_?self->caret_-1:0):std::min(last,self->caret_+1);InvalidateRect(window,nullptr,FALSE);return 0;}if(wp==VK_SPACE&&self->clicked_){self->clicked_(self->caret_);return 0;}}
        if(message==WM_NCDESTROY){self->window_=nullptr;SetWindowLongPtrW(window,GWLP_USERDATA,0);}
    }
    return DefWindowProcW(window,message,wp,lp);
}
}
