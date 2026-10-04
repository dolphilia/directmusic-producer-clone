#include "playback_window.h"
#include <stdexcept>
#include <algorithm>
namespace producer::app {
namespace {
constexpr int Sessions=100,StopSelected=101,StopAll=102,Status=103;
HWND playbackWindow=nullptr;
HWND ownerWindow=nullptr;
Conductor* player=nullptr;
std::vector<PlaybackId> displayed;
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){
    const auto c=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Playback control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
void refresh(HWND w){
    const auto list=GetDlgItem(w,Sessions);const auto selected=SendMessageW(list,LB_GETCURSEL,0,0);
    const auto selectedId=selected>=0&&static_cast<size_t>(selected)<displayed.size()?displayed[static_cast<size_t>(selected)]:0;
    const auto sessions=player->playback_sessions();std::vector<std::wstring> labels;std::vector<PlaybackId> ids;
    for(const auto& s:sessions){const auto state=s.position.playing?L"Playing":s.position.clocks<s.position.start?L"Scheduled":L"Ended";labels.push_back(std::wstring(s.secondary?L"Secondary":L"Primary")+L" — "+s.name+L" — "+state);ids.push_back(s.id);}
    bool changed=ids!=displayed||SendMessageW(list,LB_GETCOUNT,0,0)!=static_cast<LRESULT>(labels.size());
    for(size_t i=0;!changed&&i<labels.size();++i){const auto n=SendMessageW(list,LB_GETTEXTLEN,i,0);std::wstring text(static_cast<size_t>(n)+1,L'\0');SendMessageW(list,LB_GETTEXT,i,reinterpret_cast<LPARAM>(text.data()));text.resize(static_cast<size_t>(n));changed=text!=labels[i];}
    if(changed){SendMessageW(list,WM_SETREDRAW,FALSE,0);SendMessageW(list,LB_RESETCONTENT,0,0);for(const auto& label:labels)SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));displayed=std::move(ids);const auto found=std::find(displayed.begin(),displayed.end(),selectedId);if(!displayed.empty())SendMessageW(list,LB_SETCURSEL,found==displayed.end()?0:static_cast<WPARAM>(found-displayed.begin()),0);SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,TRUE);}
    EnableWindow(GetDlgItem(w,StopSelected),!displayed.empty());EnableWindow(GetDlgItem(w,StopAll),!displayed.empty());
    const auto text=displayed.empty()?L"No playback sessions.":L"Select a playback to stop it. Other sessions continue.";SetWindowTextW(GetDlgItem(w,Status),text);
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    try{
        if(message==WM_CREATE){control(w,L"STATIC",L"Current playback",0,16,16,590,24,0);control(w,L"LISTBOX",L"",WS_BORDER|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY,16,48,590,150,Sessions);control(w,L"BUTTON",L"Stop Selected",WS_TABSTOP,16,214,160,32,StopSelected);control(w,L"BUTTON",L"Stop All",WS_TABSTOP,194,214,120,32,StopAll);control(w,L"STATIC",L"",0,16,266,590,46,Status);refresh(w);SetTimer(w,1,200,nullptr);return 0;}
        if(message==WM_TIMER){refresh(w);return 0;}
        if(message==WM_COMMAND){if(LOWORD(wp)==StopSelected){const auto index=SendMessageW(GetDlgItem(w,Sessions),LB_GETCURSEL,0,0);if(index>=0&&static_cast<size_t>(index)<displayed.size()){const auto id=displayed[static_cast<size_t>(index)];const auto ids=player->playback_ids();if(std::find(ids.begin(),ids.end(),id)!=ids.end()){player->stop(id);SendMessageW(ownerWindow,PlaybackStoppedMessage,0,0);}}refresh(w);return 0;}if(LOWORD(wp)==StopAll){player->stop();SendMessageW(ownerWindow,PlaybackStoppedMessage,0,0);refresh(w);return 0;}}
        if(message==WM_CLOSE){DestroyWindow(w);return 0;}
        if(message==WM_DESTROY){KillTimer(w,1);playbackWindow=nullptr;ownerWindow=nullptr;player=nullptr;displayed.clear();return 0;}
    }catch(const std::exception& e){KillTimer(w,1);MessageBoxA(w,e.what(),"Playback Sessions",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}
    return DefWindowProcW(w,message,wp,lp);
}
}
void show_playback_window(HWND parent,Conductor& conductor){
    if(playbackWindow&&IsWindow(playbackWindow)){ShowWindow(playbackWindow,SW_SHOW);SetForegroundWindow(playbackWindow);return;}
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourcePlaybackSessions";if(!RegisterClassW(&c))throw std::runtime_error("Playback window registration failed");registered=true;}
    ownerWindow=parent;player=&conductor;displayed.clear();playbackWindow=CreateWindowExW(WS_EX_APPWINDOW,L"SourcePlaybackSessions",L"Playback Sessions",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,650,370,parent,nullptr,instance,nullptr);if(!playbackWindow){ownerWindow=nullptr;player=nullptr;throw std::runtime_error("Playback window creation failed");}ShowWindow(playbackWindow,SW_SHOW);UpdateWindow(playbackWindow);
}
}
