// Read-only diagnostics for a Producer process started from the integration copy.
// This does not send input, dismiss dialogs, register modules or patch the process.
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <string>

static std::string quoted(const wchar_t* text){
    const int count=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);
    std::string utf8(static_cast<size_t>(count),0);
    if(count)WideCharToMultiByte(CP_UTF8,0,text,-1,utf8.data(),count,nullptr,nullptr);
    std::string result="\"";
    for(char c:utf8){if(!c)break;switch(c){case '\\':result+="\\\\";break;case '"':result+="\\\"";break;case '\n':result+="\\n";break;case '\r':result+="\\r";break;case '\t':result+="\\t";break;default:if(static_cast<unsigned char>(c)>=32)result+=c;}}
    return result+'"';
}
static void report(HWND window,const char* kind){
    wchar_t title[2048]{},type[128]{};GetWindowTextW(window,title,2048);GetClassNameW(window,type,128);
    const DWORD thread=GetWindowThreadProcessId(window,nullptr);
    wchar_t desktop[128]{};DWORD needed=0;
    GetUserObjectInformationW(GetThreadDesktop(thread),UOI_NAME,desktop,sizeof(desktop),&needed);
    std::printf("{\"operation\":\"producer_window\",\"kind\":\"%s\",\"handle\":%llu,\"thread\":%lu,\"visible\":%s,\"enabled\":%s,\"owner\":%llu,\"title\":%s,\"class\":%s,\"desktop\":%s,\"style\":%lu,\"extended_style\":%lu}\n",kind,static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(window)),thread,IsWindowVisible(window)?"true":"false",IsWindowEnabled(window)?"true":"false",static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(GetWindow(window,GW_OWNER))),quoted(title).c_str(),quoted(type).c_str(),quoted(desktop).c_str(),static_cast<DWORD>(GetWindowLongW(window,GWL_STYLE)),static_cast<DWORD>(GetWindowLongW(window,GWL_EXSTYLE)));
}
static BOOL CALLBACK child(HWND window,LPARAM){report(window,"child");return TRUE;}
struct Enumeration {DWORD process;unsigned windows=0;};
static BOOL CALLBACK top(HWND window,LPARAM argument){
    auto& state=*reinterpret_cast<Enumeration*>(argument);DWORD process=0;
    GetWindowThreadProcessId(window,&process);if(process!=state.process)return TRUE;
    ++state.windows;report(window,"top");EnumChildWindows(window,child,0);return TRUE;
}
int wmain(int count,wchar_t** arguments){
    if(count!=3){std::fputs("Usage: producer_startup_probe PID exact-executable-path\n",stderr);return 2;}
    const DWORD processId=wcstoul(arguments[1],nullptr,10);
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,processId);
    if(!process){std::printf("{\"operation\":\"producer_process_open\",\"error\":%lu}\n",GetLastError());return 1;}
    wchar_t path[32768]{};DWORD size=32768;
    const bool same=QueryFullProcessImageNameW(process,0,path,&size)&&_wcsicmp(path,arguments[2])==0;
    std::printf("{\"operation\":\"producer_process_identity\",\"same\":%s,\"path\":%s}\n",same?"true":"false",quoted(path).c_str());
    CloseHandle(process);if(!same)return 1;
    Enumeration state{processId};EnumWindows(top,reinterpret_cast<LPARAM>(&state));
    std::printf("{\"operation\":\"producer_window_count\",\"count\":%u}\n",state.windows);
    return 0;
}
