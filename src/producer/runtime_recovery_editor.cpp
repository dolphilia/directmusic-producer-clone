#include "runtime_recovery_editor.h"
#include <commdlg.h>
#include <stdexcept>

namespace producer::app { namespace {
enum : UINT { Journal=10,Browse,Inspect,Restore,Details,Status };
struct Session { Framework& host; std::wstring journal; Bytes preview; RuntimeRecoveryOrigin origin; bool ready=false; };
std::wstring wide(const std::string& text) {
    const auto n=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring result(n,L'\0');if(n)MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),n);return result;
}
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id) {
    auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);
    if(!c)throw std::runtime_error("Recovery control creation failed");
    SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
std::wstring path(HWND w) {auto c=GetDlgItem(w,Journal);auto n=GetWindowTextLengthW(c);std::wstring p(n+1,L'\0');GetWindowTextW(c,p.data(),n+1);p.resize(n);return p;}
void invalidate(HWND w,Session& s) {s.ready=false;EnableWindow(GetDlgItem(w,Restore),FALSE);}
void inspect(HWND w,Session& s) {
    invalidate(w,s);s.journal=path(w);s.preview=read_file(s.journal);s.origin=runtime_update_recovery_origin(s.preview);
    auto states=s.host.validate_runtime_recovery(s.journal,s.origin.outputRoot,s.origin.configured);
    if(read_file(s.journal)!=s.preview)throw std::runtime_error("Journal changed during inspection");
    std::wstring text=L"Project: "+s.origin.projectPath+L"\r\nScope: "+s.origin.outputRoot+L"\r\nMode: "+(s.origin.configured?std::wstring(L"Configured defaults"):std::wstring(L"Explicit output folder"))+L"\r\n\r\n";
    size_t replace=0,remove=0;bool conflict=false;
    for(const auto& state:states){text+=state.file.target+L"\r\n  ";if(state.state==RuntimeRecoveryState::Before)text+=L"Already restored";else if(state.state==RuntimeRecoveryState::Conflict){text+=L"CONFLICT: "+wide(state.reason);conflict=true;}else{if(state.file.existed){++replace;text+=L"Restore previous bytes and metadata";}else{++remove;text+=L"Remove output created by this transaction";}}text+=L"\r\n";}
    SetWindowTextW(GetDlgItem(w,Details),text.c_str());s.ready=!conflict&&(replace+remove)>0;EnableWindow(GetDlgItem(w,Restore),s.ready);
    const auto status=conflict?std::wstring(L"Conflict: no restoration allowed. External changes are preserved."):s.ready?L"Ready: "+std::to_wstring(replace)+L" existing outputs to restore; "+std::to_wstring(remove)+L" newly created outputs to remove.":std::wstring(L"All outputs already restored. No changes needed.");
    SetWindowTextW(GetDlgItem(w,Status),status.c_str());
}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try {
        if(msg==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
            control(w,L"STATIC",L"Recovery journal (RTUP v3). Open its saved source Project first.",0,16,14,730,24,0);
            control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,16,42,560,28,Journal);
            control(w,L"BUTTON",L"Browse...",WS_TABSTOP,590,42,140,28,Browse);
            control(w,L"BUTTON",L"Inspect",WS_TABSTOP,16,82,140,30,Inspect);
            control(w,L"BUTTON",L"Restore Outputs",WS_TABSTOP,172,82,180,30,Restore);
            control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL|WS_VSCROLL|WS_HSCROLL,16,124,714,280,Details);
            control(w,L"STATIC",L"Inspection is read-only. The saved Project, source files and mapped targets must match. Journals and staging folders remain as evidence.",0,16,420,714,72,Status);invalidate(w,*s);return 0;}
        if(msg==WM_COMMAND&&s){const auto id=LOWORD(wp);if(id==Journal&&HIWORD(wp)==EN_CHANGE){invalidate(w,*s);SetWindowTextW(GetDlgItem(w,Details),L"");SetWindowTextW(GetDlgItem(w,Status),L"Inspect the selected journal before restoring.");return 0;}
            if(HIWORD(wp)!=BN_CLICKED)return 0;
            if(id==Browse){wchar_t buffer[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=w;f.lpstrFile=buffer;f.nMaxFile=32768;f.lpstrFilter=L"Recovery journal\0*.riff\0All files\0*.*\0";f.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetOpenFileNameW(&f))SetWindowTextW(GetDlgItem(w,Journal),buffer);else if(CommDlgExtendedError())throw std::runtime_error("Recovery journal selection failed");return 0;}
            if(id==Inspect){inspect(w,*s);return 0;}
            if(id==Restore){if(!s->ready||path(w)!=s->journal||read_file(s->journal)!=s->preview)throw std::runtime_error("Preview changed; inspect the journal again");
                // Framework revalidates the source Project and each target before writing.
                const auto current=s->host.validate_runtime_recovery(s->journal,s->origin.outputRoot,s->origin.configured);
                for(const auto& target:current)if(target.state==RuntimeRecoveryState::Conflict)throw std::runtime_error("Output changed since preview; inspect the journal again");
                if(MessageBoxW(w,L"Restore the listed outputs to their pre-update state? Newly created outputs listed above will be removed. Source files, journal and staging folders are retained.",L"Confirm Runtime Recovery",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;
                if(read_file(s->journal)!=s->preview)throw std::runtime_error("Journal changed during confirmation; inspect again");
                s->host.recover_runtime_update(s->journal,s->origin.outputRoot,s->origin.configured);inspect(w,*s);return 0;}
        }
        if(msg==WM_CLOSE){DestroyWindow(w);return 0;}
    }catch(const std::exception& e){if(s)invalidate(w,*s);SetWindowTextW(GetDlgItem(w,Status),wide(e.what()).c_str());if(msg==WM_CREATE)return -1;return 0;}
    return DefWindowProcW(w,msg,wp,lp);
}
}
void show_runtime_recovery(HWND parent,Framework& host) {
    auto instance=GetModuleHandleW(nullptr);static bool registered=false;
    if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerRuntimeRecovery";if(!RegisterClassW(&c))throw std::runtime_error("Recovery window registration failed");registered=true;}
    Session session{host};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerRuntimeRecovery",L"Runtime Recovery",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,770,540,nullptr,nullptr,instance,&session);
    if(!w)throw std::runtime_error("Recovery window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;
    while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Recovery message loop failed");
}
}
