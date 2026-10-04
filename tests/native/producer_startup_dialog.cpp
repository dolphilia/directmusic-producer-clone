// User-requested helper for the integration copy's known startup warnings.
// Captures only the verified Producer dialog. Default mode never sends input.
// Exact process path, title, class, message, button ID and owning PID are checked.
#include <windows.h>
#include <gdiplus.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

static std::wstring text(HWND window) {
    wchar_t value[2048]{};GetWindowTextW(window,value,2048);return value;
}
static bool type(HWND window,const wchar_t* expected) {
    wchar_t value[128]{};GetClassNameW(window,value,128);return wcscmp(value,expected)==0;
}
static std::string quoted(const std::wstring& value) {
    const int size=WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,nullptr,0,nullptr,nullptr);
    std::string utf8(static_cast<size_t>(size),0);
    if(size)WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,utf8.data(),size,nullptr,nullptr);
    std::string out="\"";
    for(char c:utf8){if(!c)break;if(c=='\\'||c=='"'){out+='\\';out+=c;}else if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if(static_cast<unsigned char>(c)>=32)out+=c;}
    return out+'"';
}
static bool same_process(HWND window,DWORD expected) {
    DWORD actual=0;GetWindowThreadProcessId(window,&actual);return actual==expected;
}
static BOOL CALLBACK children(HWND window,LPARAM arg) {
    auto& values=*reinterpret_cast<std::vector<std::wstring>*>(arg);
    if(type(window,L"Static")){const auto value=text(window);if(!value.empty())values.push_back(value);}return TRUE;
}
static bool known(HWND window,DWORD pid,HWND& button,std::wstring& body) {
    button=nullptr;
    if(!same_process(window,pid)||!IsWindowVisible(window)||!IsWindowEnabled(window)||
       !type(window,L"#32770")||text(window)!=L"DirectMusic Producer")return false;
    std::vector<std::wstring> values;EnumChildWindows(window,children,reinterpret_cast<LPARAM>(&values));
    if(values.size()!=1)return false;
    body=values[0];
    const bool allowed=body==L"Failed to update the system registry.\nPlease try using REGEDIT."||
        body==L"Unable to load DirectMusic Producer Components.  Please run Setup and reinstall.";
    if(!allowed)return false;
    button=GetDlgItem(window,IDOK);
    return button&&same_process(button,pid)&&type(button,L"Button")&&text(button)==L"OK"&&
        IsWindowVisible(button)&&IsWindowEnabled(button)&&GetParent(button)==window;
}
struct Enumeration {DWORD pid;std::vector<HWND> matches;};
static BOOL CALLBACK windows(HWND window,LPARAM arg) {
    auto& state=*reinterpret_cast<Enumeration*>(arg);HWND button=nullptr;std::wstring body;
    if(known(window,state.pid,button,body))state.matches.push_back(window);return TRUE;
}
static bool screenshot(HWND window,const std::wstring& destination,const RECT& rectangle) {
    const int width=rectangle.right-rectangle.left,height=rectangle.bottom-rectangle.top;
    if(width<=0||height<=0||width>4096||height>4096)return false;
    HDC source=GetWindowDC(window);if(!source)return false;
    HDC target=CreateCompatibleDC(source);HBITMAP bitmap=CreateCompatibleBitmap(source,width,height);
    if(!target||!bitmap){if(target)DeleteDC(target);if(bitmap)DeleteObject(bitmap);ReleaseDC(window,source);return false;}
    auto previous=SelectObject(target,bitmap);
    // PrintWindow captures this window even when another app is in front.
    const BOOL captured=PrintWindow(window,target,0);
    SelectObject(target,previous);DeleteDC(target);ReleaseDC(window,source);
    bool saved=false;
    if(captured){
        Gdiplus::Bitmap image(bitmap,nullptr);
        const CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
        saved=image.GetLastStatus()==Gdiplus::Ok&&image.Save(destination.c_str(),&png,nullptr)==Gdiplus::Ok;
    }
    DeleteObject(bitmap);return saved;
}
static void rect(const char* name,const RECT& r) {
    std::printf("\"%s\":{\"left\":%ld,\"top\":%ld,\"right\":%ld,\"bottom\":%ld}",name,r.left,r.top,r.right,r.bottom);
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=5||(wcscmp(argv[4],L"observe")&&wcscmp(argv[4],L"click")))return 2;
    SetProcessDPIAware();const DWORD pid=wcstoul(argv[1],nullptr,10);const bool click=wcscmp(argv[4],L"click")==0;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid);
    if(!process)return 1;
    wchar_t actual[32768]{};DWORD size=32768;
    const bool identity=QueryFullProcessImageNameW(process,0,actual,&size)&&_wcsicmp(actual,argv[2])==0;
    std::printf("{\"operation\":\"process_identity\",\"pid\":%lu,\"same\":%s,\"path\":%s}\n",pid,identity?"true":"false",quoted(actual).c_str());
    if(!identity){CloseHandle(process);return 1;}
    Enumeration state{pid,{}};EnumWindows(windows,reinterpret_cast<LPARAM>(&state));
    if(state.matches.size()!=1){std::printf("{\"operation\":\"known_dialog_count\",\"count\":%zu,\"clicked\":false}\n",state.matches.size());CloseHandle(process);return 1;}
    HWND dialog=state.matches[0],button=nullptr;std::wstring body;
    if(!known(dialog,pid,button,body)){CloseHandle(process);return 1;}
    RECT dialogRect{},buttonRect{};GetWindowRect(dialog,&dialogRect);GetWindowRect(button,&buttonRect);
    RECT relative{buttonRect.left-dialogRect.left,buttonRect.top-dialogRect.top,buttonRect.right-dialogRect.left,buttonRect.bottom-dialogRect.top};
    ULONG_PTR token=0;Gdiplus::GdiplusStartupInput startup;
    if(Gdiplus::GdiplusStartup(&token,&startup,nullptr)!=Gdiplus::Ok){CloseHandle(process);return 1;}
    const bool captured=screenshot(dialog,argv[3],dialogRect);Gdiplus::GdiplusShutdown(token);
    std::printf("{\"operation\":\"dialog_snapshot\",\"pid\":%lu,\"handle\":%llu,\"title\":\"DirectMusic Producer\",\"body\":%s,\"screenshot\":%s,\"captured\":%s,",pid,static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(dialog)),quoted(body).c_str(),quoted(argv[3]).c_str(),captured?"true":"false");
    rect("dialogScreen",dialogRect);std::putchar(',');rect("okScreen",buttonRect);std::putchar(',');rect("okInScreenshot",relative);
    std::printf(",\"okCenterInScreenshot\":{\"x\":%ld,\"y\":%ld},\"clickRequested\":%s}\n",(relative.left+relative.right)/2,(relative.top+relative.bottom)/2,click?"true":"false");std::fflush(stdout);
    bool ok=captured;
    if(click&&captured){
        HWND verifiedButton=nullptr;std::wstring verifiedBody;
        if(!known(dialog,pid,verifiedButton,verifiedBody)||verifiedButton!=button||verifiedBody!=body)ok=false;
        else {
            DWORD_PTR response=0;
            // Standard dialog button command: targets only this verified process;
            // does not depend on focus or send clicks to a different foreground app.
            const bool sent=SendMessageTimeoutW(dialog,WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED),reinterpret_cast<LPARAM>(button),SMTO_ABORTIFHUNG|SMTO_BLOCK,3000,&response)!=0;
            const DWORD error=sent?0:GetLastError();
            const bool dismissed=!IsWindow(dialog);
            std::printf("{\"operation\":\"dialog_ok\",\"pid\":%lu,\"method\":\"WM_COMMAND/IDOK\",\"sent\":%s,\"dialogDismissed\":%s,\"error\":%lu}\n",pid,sent?"true":"false",dismissed?"true":"false",error);
            ok=sent&&dismissed;
        }
    }
    CloseHandle(process);return ok?0:1;
}
