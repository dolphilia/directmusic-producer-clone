#pragma once
#include <windows.h>
#include <cstdio>

namespace drop_menu_boundary {
inline unsigned calls=0;
inline bool passed=true;
// Observe the real menu assembled by the DLL and dismiss it at the display
// boundary. Command routing and interactive menu input are outside this probe.
inline BOOL WINAPI dismiss(HMENU menu,UINT flags,int x,int y,int reserved,HWND owner,const RECT* rectangle){
    ++calls;DWORD process=0;GetWindowThreadProcessId(owner,&process);
    const bool owned=IsWindow(owner)&&process==GetCurrentProcessId();
    const bool hidden=owned&&!IsWindowVisible(owner);
    const int count=GetMenuItemCount(menu);
    std::printf("{\"operation\":\"drop_menu_popup\",\"flags\":%u,\"x\":%d,\"y\":%d,\"reserved\":%d,\"rectangle\":%s,\"owned\":%s,\"hidden\":%s,\"count\":%d}\n",flags,x,y,reserved,rectangle?"true":"false",owned?"true":"false",hidden?"true":"false",count);
    passed=owned&&hidden&&count==4&&flags==TPM_RIGHTBUTTON&&passed;
    const UINT expected[]={0x8026,0x8028,0,0x8027};
    for(int i=0;i<count;++i){char label[32]{};GetMenuStringA(menu,static_cast<UINT>(i),label,sizeof(label),MF_BYPOSITION);
        const UINT id=GetMenuItemID(menu,i),state=GetMenuState(menu,static_cast<UINT>(i),MF_BYPOSITION);
        std::printf("{\"operation\":\"drop_menu_item\",\"index\":%d,\"id\":%u,\"state\":%u,\"label\":\"%s\"}\n",i,id,state,label);
        passed=i<4&&id==expected[i]&&passed;
    }
    return FALSE;
}
}
