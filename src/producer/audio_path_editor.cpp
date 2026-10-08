#include "audio_path_editor.h"
#include "runtime_settings_editor.h"
#include <commdlg.h>
#include <filesystem>
#include <stdexcept>
#include <cstring>
#include <algorithm>
namespace producer::app {namespace {
enum:UINT {Documents=10,New,Open,Save,Name,NameSet,Routes,Buffers,Connect,Undo,Redo,Segments,Embed,Remove,Status,PortBase,PortCount,RouteBase,RouteCount,PortSet,RouteSet,RuntimeSave,RuntimeSettings,FileOutput,MixinChannels,MixinAdd,Sends,SendDestinations,SendSet,WavesReverb,SendAdd,SendVolume,EnvironmentalAdd};
struct Session{Framework& host;size_t selected=0;std::vector<std::pair<size_t,size_t>> routes,sends;std::vector<size_t> destinations;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int a,int b,UINT id=0){const auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,a,b,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("AudioPath control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
std::wstring value(HWND w,UINT id){const auto c=GetDlgItem(w,id);const auto n=GetWindowTextLengthW(c);std::wstring s(n+1,L'\0');GetWindowTextW(c,s.data(),n+1);s.resize(n);return s;}
std::uint32_t number(HWND w,UINT id){const auto text=value(w,id);if(text.empty()||text.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Enter an unsigned PChannel base or count");const auto n=std::stoull(text);if(n>UINT32_MAX)throw std::runtime_error("PChannel value exceeds 32 bits");return static_cast<std::uint32_t>(n);}
std::wstring guid_label(const AudioBufferId& id){GUID guid{};std::memcpy(&guid,id.data(),16);wchar_t text[40]{};StringFromGUID2(guid,text,40);return text;}
enum:UINT {SendSource=100,SendTarget,SendPosition,SendLevel,SendHint};
struct SendEdit {
    AudioPathDocument& document;
    bool inserting;
    size_t buffer,effect;
    std::vector<size_t> destinations,positions;
};
std::int32_t send_level(HWND w) {
    const auto text=value(w,SendLevel);
    const auto digits=!text.empty()&&text[0]==L'-'?text.substr(1):text;
    if(digits.empty()||digits.find_first_not_of(L"0123456789")!=std::wstring::npos)
        throw std::runtime_error("Enter an integer from -10000 through 0; -600 means -6 dB");
    const auto level=std::stoll(text);
    if(level < -10000 || level > 0)
        throw std::runtime_error("Send attenuation must be from -10000 through 0; -600 means -6 dB");
    return static_cast<std::int32_t>(level);
}
void send_choices(HWND w,SendEdit& s) {
    const auto row=SendDlgItemMessageW(w,SendSource,CB_GETCURSEL,0,0);
    s.destinations.clear();s.positions.clear();
    SendDlgItemMessageW(w,SendTarget,CB_RESETCONTENT,0,0);
    SendDlgItemMessageW(w,SendPosition,CB_RESETCONTENT,0,0);
    if(row==CB_ERR){EnableWindow(GetDlgItem(w,IDOK),FALSE);return;}
    s.buffer=static_cast<size_t>(row);
    const auto details=s.document.buffer_details();
    s.destinations=s.document.available_send_destinations(s.buffer);
    for(const auto destination:s.destinations){
        const auto text=(s.document.environmental_reverb_buffer()==destination?L"Standard Env. Reverb — ":L"Buffer ")+std::to_wstring(destination+1)+L", "+
            std::to_wstring(details.at(destination).channels)+L" channels — "+guid_label(details.at(destination).id);
        SendDlgItemMessageW(w,SendTarget,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    const auto preferred=s.document.default_send_destination(s.buffer);size_t choice=0;
    if(preferred){const auto it=std::find(s.destinations.begin(),s.destinations.end(),*preferred);if(it!=s.destinations.end())choice=static_cast<size_t>(it-s.destinations.begin());}
    SendDlgItemMessageW(w,SendTarget,CB_SETCURSEL,choice,0);
    for(const auto& effect:s.document.effects())if(effect.buffer==s.buffer){
        s.positions.push_back(effect.index);
        const auto text=L"Before effect "+std::to_wstring(effect.index+1);
        SendDlgItemMessageW(w,SendPosition,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    s.positions.push_back(static_cast<size_t>(-1));
    SendDlgItemMessageW(w,SendPosition,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"After the last effect"));
    SendDlgItemMessageW(w,SendPosition,CB_SETCURSEL,s.positions.size()-1,0);
    EnableWindow(GetDlgItem(w,IDOK),!s.destinations.empty());
    SetWindowTextW(GetDlgItem(w,SendHint),s.destinations.empty()?
        L"Add a compatible mix-in buffer to this AudioPath before inserting a Send.":
        L"Effects after this Send are heard only on the source buffer.");
}
LRESULT CALLBACK send_edit_proc(HWND w,UINT message,WPARAM wp,LPARAM lp) {
    auto s=reinterpret_cast<SendEdit*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try {
        if(message==WM_CREATE){
            s=static_cast<SendEdit*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
            int y=20;
            if(s->inserting){
                control(w,L"STATIC",L"Source buffer",0,16,y,548,20);
                control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,y+24,548,200,SendSource);
                const auto details=s->document.buffer_details();
                for(size_t i=0;i<details.size();++i){
                    const auto text=L"Buffer "+std::to_wstring(i+1)+L", "+std::to_wstring(details[i].channels)+L" channels";
                    SendDlgItemMessageW(w,SendSource,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
                }
                SendDlgItemMessageW(w,SendSource,CB_SETCURSEL,s->buffer,0);
                y+=62;
                control(w,L"STATIC",L"Destination mix-in",0,16,y,548,20);
                control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,y+24,548,200,SendTarget);
                y+=62;
                control(w,L"STATIC",L"Position in the effect chain",0,16,y,548,20);
                control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,y+24,548,200,SendPosition);
                y+=62;
            }
            control(w,L"STATIC",L"Attenuation (1/100 dB, -10000 to 0)",0,16,y,360,20);
            const auto level=s->inserting?0:s->document.send_attenuation(s->buffer,s->effect).value();
            control(w,L"EDIT",std::to_wstring(level).c_str(),WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,390,y-2,174,26,SendLevel);
            SendDlgItemMessageW(w,SendLevel,EM_SETLIMITTEXT,6,0);
            control(w,L"STATIC",L"-600 means -6 dB. Zero keeps the Send at full level.",0,16,y+34,548,26);
            if(s->inserting)control(w,L"STATIC",L"",0,16,y+62,548,34,SendHint);
            const auto buttonY=y+(s->inserting?108:76);
            control(w,L"BUTTON",s->inserting?L"Insert Send":L"OK",WS_TABSTOP|BS_DEFPUSHBUTTON,344,buttonY,110,28,IDOK);
            control(w,L"BUTTON",L"Cancel",WS_TABSTOP,464,buttonY,100,28,IDCANCEL);
            if(s->inserting)send_choices(w,*s);
            return 0;
        }
        if(message==WM_COMMAND&&s){
            const auto id=LOWORD(wp);
            if(s->inserting&&id==SendSource&&HIWORD(wp)==CBN_SELCHANGE){send_choices(w,*s);return 0;}
            if(id==IDCANCEL){DestroyWindow(w);return 0;}
            if(id==IDOK){
                const auto level=send_level(w);
                if(s->inserting){
                    const auto destination=SendDlgItemMessageW(w,SendTarget,CB_GETCURSEL,0,0);
                    const auto position=SendDlgItemMessageW(w,SendPosition,CB_GETCURSEL,0,0);
                    if(destination==CB_ERR||position==CB_ERR||
                       !s->document.add_send(s->buffer,s->destinations.at(destination),s->positions.at(position),level))
                        throw std::runtime_error("Select a compatible destination and a valid effect position");
                }else if(s->document.send_attenuation(s->buffer,s->effect)!=level&&
                         !s->document.set_send_attenuation(s->buffer,s->effect,level))
                    throw std::runtime_error("This Send volume cannot be edited");
                DestroyWindow(w);return 0;
            }
        }
        if(message==WM_CLOSE){DestroyWindow(w);return 0;}
    }catch(const std::exception& error){
        const std::string detail=error.what();
        MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"Send",MB_OK|MB_ICONERROR);
        if(message==WM_CREATE)return -1;
    }
    return DefWindowProcW(w,message,wp,lp);
}
void show_send_edit(HWND parent,AudioPathDocument& document,bool inserting,size_t buffer,size_t effect=0) {
    if(!inserting&&!document.send_attenuation(buffer,effect))
        throw std::runtime_error("Select a Send with supported volume parameters");
    const auto instance=GetModuleHandleW(nullptr);
    static bool registered=false;
    if(!registered){
        WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=send_edit_proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);
        c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerSendEdit";
        if(!RegisterClassW(&c))throw std::runtime_error("Send window registration failed");registered=true;
    }
    SendEdit session{document,inserting,buffer,effect};RECT bounds{};GetWindowRect(parent,&bounds);
    const int height=inserting?400:214;
    const auto w=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT,L"SourceProducerSendEdit",
        inserting?L"Insert Send":L"Send Volume",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        bounds.left+(bounds.right-bounds.left-596)/2,bounds.top+(bounds.bottom-bounds.top-height)/2,
        596,height,parent,nullptr,instance,&session);
    if(!w)throw std::runtime_error("Send window creation failed");
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);
    MSG message{};BOOL result=1;
    while(IsWindow(w)&&(result=GetMessageW(&message,nullptr,0,0))>0)
        if(!IsDialogMessageW(w,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);
    if(result==0)PostQuitMessage(static_cast<int>(message.wParam));
    if(result==-1)throw std::runtime_error("Send message loop failed");
}
void send_selection(HWND w,Session& s){
    SendDlgItemMessageW(w,SendDestinations,CB_RESETCONTENT,0,0);s.destinations.clear();
    const auto row=SendDlgItemMessageW(w,Sends,CB_GETCURSEL,0,0);
    if(s.host.audio_paths().empty()||row==CB_ERR||static_cast<size_t>(row)>=s.sends.size()){EnableWindow(GetDlgItem(w,SendSet),FALSE);EnableWindow(GetDlgItem(w,SendVolume),FALSE);return;}
    auto& document=s.host.audio_path_document(s.selected);const auto [buffer,effect]=s.sends[row];
    const auto details=document.buffer_details();const auto effects=document.effects();
    AudioBufferId current{};for(const auto& f:effects)if(f.buffer==buffer&&f.index==effect)current=f.sendBuffer;
    s.destinations=document.send_destinations(buffer,effect);size_t choice=0;
    for(size_t i=0;i<s.destinations.size();++i){const auto at=s.destinations[i];const auto text=(document.environmental_reverb_buffer()==at?L"Standard Env. Reverb — ":L"Buffer ")+std::to_wstring(at+1)+L", "+std::to_wstring(details[at].channels)+L" channels — "+guid_label(details[at].id);
        SendDlgItemMessageW(w,SendDestinations,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));if(details[at].id==current)choice=i;
    }
    SendDlgItemMessageW(w,SendDestinations,CB_SETCURSEL,choice,0);EnableWindow(GetDlgItem(w,SendSet),!s.destinations.empty());EnableWindow(GetDlgItem(w,SendVolume),document.send_attenuation(buffer,effect).has_value());
}
std::wstring file(HWND w,bool save,bool runtime=false){wchar_t path[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=w;f.lpstrFilter=runtime?L"Runtime AudioPath\0*.aud\0\0":L"AudioPath\0*.aup;*.aud\0\0";f.lpstrFile=path;f.nMaxFile=32768;f.lpstrDefExt=runtime?L"aud":L"aup";f.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);if(!(save?GetSaveFileNameW(&f):GetOpenFileNameW(&f)))return L"";return path;}
void route_selection(HWND w,Session& s){if(s.host.audio_paths().empty())return;const auto row=SendDlgItemMessageW(w,Routes,CB_GETCURSEL,0,0);if(row==CB_ERR||static_cast<size_t>(row)>=s.routes.size())return;const auto [p,r]=s.routes[row];const auto ids=s.host.audio_path_document(s.selected).buffers();const auto port=s.host.audio_path_document(s.selected).ports().at(p);const auto route=port.routes.at(r);SetWindowTextW(GetDlgItem(w,PortBase),std::to_wstring(port.base).c_str());SetWindowTextW(GetDlgItem(w,PortCount),std::to_wstring(port.count).c_str());SetWindowTextW(GetDlgItem(w,RouteBase),std::to_wstring(route.base).c_str());SetWindowTextW(GetDlgItem(w,RouteCount),std::to_wstring(route.count).c_str());for(size_t i=0;i<ids.size();++i)if(!route.buffers.empty()&&ids[i]==route.buffers[0])SendDlgItemMessageW(w,Buffers,CB_SETCURSEL,i,0);}
void refresh(HWND w,Session& s){SendDlgItemMessageW(w,Documents,CB_RESETCONTENT,0,0);const auto& owned=s.host.audio_paths();for(const auto& o:owned){const auto text=o.document->name()+(o.document->dirty()?L" *":L"")+L" — "+(o.path.empty()?L"unsaved":std::filesystem::path(o.path).filename().wstring());SendDlgItemMessageW(w,Documents,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    const bool present=!owned.empty();for(const auto id:{Save,Name,NameSet,Routes,Buffers,Connect,Undo,Redo,PortBase,PortCount,RouteBase,RouteCount,PortSet,RouteSet,RuntimeSave,RuntimeSettings,FileOutput,MixinChannels,MixinAdd,Sends,SendDestinations,SendSet,WavesReverb,SendAdd,SendVolume,EnvironmentalAdd})EnableWindow(GetDlgItem(w,id),present);SendDlgItemMessageW(w,Routes,CB_RESETCONTENT,0,0);SendDlgItemMessageW(w,Buffers,CB_RESETCONTENT,0,0);s.routes.clear();s.sends.clear();s.destinations.clear();SendDlgItemMessageW(w,Sends,CB_RESETCONTENT,0,0);SendDlgItemMessageW(w,SendDestinations,CB_RESETCONTENT,0,0);SendDlgItemMessageW(w,Segments,CB_RESETCONTENT,0,0);for(size_t i=0;i<s.host.documents().size();++i){const auto label=L"Segment "+std::to_wstring(i+1)+L" — "+std::filesystem::path(s.host.documents()[i].path).filename().wstring();SendDlgItemMessageW(w,Segments,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendDlgItemMessageW(w,Segments,CB_SETCURSEL,0,0);EnableWindow(GetDlgItem(w,Embed),present&&!s.host.documents().empty());EnableWindow(GetDlgItem(w,Remove),!s.host.documents().empty());if(!present){SetWindowTextW(GetDlgItem(w,Name),L"");return;}if(s.selected>=owned.size())s.selected=owned.size()-1;SendDlgItemMessageW(w,Documents,CB_SETCURSEL,s.selected,0);auto& document=s.host.audio_path_document(s.selected);SetWindowTextW(GetDlgItem(w,Name),document.name().c_str());const auto ports=document.ports();
    for(size_t p=0;p<ports.size();++p)for(size_t r=0;r<ports[p].routes.size();++r){const auto& route=ports[p].routes[r];s.routes.push_back({p,r});const auto label=L"Port "+std::to_wstring(p+1)+L", PChannel "+std::to_wstring(route.base)+L" count "+std::to_wstring(route.count)+L", buffers "+std::to_wstring(route.buffers.size());SendDlgItemMessageW(w,Routes,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    const auto details=document.buffer_details();for(size_t i=0;i<details.size();++i){const auto& d=details[i];const auto text=L"Buffer "+std::to_wstring(i+1)+(d.flags&8?L" (mix-in), ":L", ")+std::to_wstring(d.channels)+L" channels — "+guid_label(d.id);SendDlgItemMessageW(w,Buffers,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    for(const auto& effect:document.effects())if(effect.sendBuffer!=AudioBufferId{}){s.sends.push_back({effect.buffer,effect.index});const auto text=L"Buffer "+std::to_wstring(effect.buffer+1)+L", effect "+std::to_wstring(effect.index+1)+L" -> "+guid_label(effect.sendBuffer);SendDlgItemMessageW(w,Sends,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}SendDlgItemMessageW(w,Sends,CB_SETCURSEL,0,0);send_selection(w,s);SendDlgItemMessageW(w,Routes,CB_SETCURSEL,0,0);route_selection(w,s);SetWindowTextW(GetDlgItem(w,Status),(L"Save AudioPath, then Project. Effects: "+std::to_wstring(document.effects().size())).c_str());
}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{if(msg==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,16,620,250,Documents);control(w,L"BUTTON",L"New",WS_TABSTOP,16,56,100,28,New);control(w,L"BUTTON",L"Open...",WS_TABSTOP,126,56,100,28,Open);control(w,L"BUTTON",L"Save As...",WS_TABSTOP,236,56,110,28,Save);control(w,L"BUTTON",L"Undo",WS_TABSTOP,356,56,100,28,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,466,56,100,28,Redo);control(w,L"BUTTON",L"Runtime Save As...",WS_TABSTOP,430,90,200,28,RuntimeSave);control(w,L"STATIC",L"Name",0,16,98,100,22);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,16,124,400,26,Name);control(w,L"BUTTON",L"Set Name",WS_TABSTOP,430,124,136,28,NameSet);control(w,L"STATIC",L"PChannel route",0,16,172,180,22);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,198,620,250,Routes);control(w,L"STATIC",L"Destination buffer",0,16,246,180,22);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,272,620,250,Buffers);control(w,L"BUTTON",L"Connect to Selected Buffer",WS_TABSTOP,16,318,280,30,Connect);control(w,L"BUTTON",L"Runtime Properties...",WS_TABSTOP,310,318,280,30,RuntimeSettings);control(w,L"STATIC",L"Target Segment",0,16,352,180,22);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,378,620,200,Segments);control(w,L"BUTTON",L"Copy into Selected Segment",WS_TABSTOP,16,420,280,30,Embed);control(w,L"BUTTON",L"Remove Embedded AudioPath",WS_TABSTOP,310,420,280,30,Remove);control(w,L"STATIC",L"Port base / count",0,16,466,190,22);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,210,462,110,26,PortBase);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,330,462,110,26,PortCount);control(w,L"BUTTON",L"Set Port Range",WS_TABSTOP,450,462,180,28,PortSet);control(w,L"STATIC",L"Route base / count",0,16,504,190,22);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,210,500,110,26,RouteBase);control(w,L"EDIT",L"",WS_BORDER|WS_TABSTOP,330,500,110,26,RouteCount);control(w,L"BUTTON",L"Set Route Range",WS_TABSTOP,450,500,180,28,RouteSet);control(w,L"STATIC",L"",0,16,542,620,30,Status);control(w,L"BUTTON",L"Add FileOutput to Selected Buffer",WS_TABSTOP,16,580,380,28,FileOutput);
        control(w,L"BUTTON",L"Add Waves Reverb",WS_TABSTOP,410,580,225,28,WavesReverb);
        control(w,L"STATIC",L"New mix-in audio channels",0,16,624,240,22);
        control(w,L"EDIT",L"2",WS_BORDER|WS_TABSTOP,260,620,70,26,MixinChannels);
        control(w,L"BUTTON",L"Add Mix-in",WS_TABSTOP,350,620,128,28,MixinAdd);
        control(w,L"BUTTON",L"Add Env. Reverb",WS_TABSTOP,490,620,146,28,EnvironmentalAdd);
        control(w,L"STATIC",L"Existing Send effect",0,16,660,240,22);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,16,686,620,180,Sends);
        control(w,L"STATIC",L"Send destination mix-in",0,16,722,240,22);
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,16,748,620,180,SendDestinations);
        control(w,L"BUTTON",L"Set Send Destination",WS_TABSTOP,16,788,220,28,SendSet);
        control(w,L"BUTTON",L"Insert Send...",WS_TABSTOP,246,788,190,28,SendAdd);
        control(w,L"BUTTON",L"Send Volume...",WS_TABSTOP,446,788,190,28,SendVolume);
        refresh(w,*s);return 0;}
    if(msg==WM_COMMAND&&s){const auto id=LOWORD(wp);if(id==Documents&&HIWORD(wp)==CBN_SELCHANGE){s->selected=SendDlgItemMessageW(w,Documents,CB_GETCURSEL,0,0);refresh(w,*s);return 0;}if(id==Routes&&HIWORD(wp)==CBN_SELCHANGE){route_selection(w,*s);return 0;}if(id==Sends&&HIWORD(wp)==CBN_SELCHANGE){send_selection(w,*s);return 0;}if(HIWORD(wp)!=BN_CLICKED)return 0;
        switch(id){case SendAdd:{const auto row=SendDlgItemMessageW(w,Buffers,CB_GETCURSEL,0,0);if(row==CB_ERR)throw std::runtime_error("Select a source buffer");show_send_edit(w,s->host.audio_path_document(s->selected),true,static_cast<size_t>(row));break;}
        case SendVolume:{const auto row=SendDlgItemMessageW(w,Sends,CB_GETCURSEL,0,0);if(row==CB_ERR)throw std::runtime_error("Select an existing Send");const auto [buffer,effect]=s->sends.at(row);show_send_edit(w,s->host.audio_path_document(s->selected),false,buffer,effect);break;}
        case EnvironmentalAdd:{auto& document=s->host.audio_path_document(s->selected);if(!document.add_environmental_reverb_buffer())throw std::runtime_error("This AudioPath already contains Standard Env. Reverb");break;}
        case MixinAdd:{const auto channels=number(w,MixinChannels);auto& document=s->host.audio_path_document(s->selected);if(!document.add_mixin_buffer(channels))throw std::runtime_error("Audio channel count must be 1 through 65535");refresh(w,*s);SendDlgItemMessageW(w,Buffers,CB_SETCURSEL,document.buffers().size()-1,0);return 0;}
        case WavesReverb:{const auto selected=SendDlgItemMessageW(w,Buffers,CB_GETCURSEL,0,0);if(selected==CB_ERR||!s->host.audio_path_document(s->selected).add_waves_reverb(static_cast<size_t>(selected)))throw std::runtime_error("Select a supported buffer without Waves Reverb; stereo predefined buffers can be materialized");break;}
        case SendSet:{const auto row=SendDlgItemMessageW(w,Sends,CB_GETCURSEL,0,0),destination=SendDlgItemMessageW(w,SendDestinations,CB_GETCURSEL,0,0);if(row==CB_ERR||destination==CB_ERR)throw std::runtime_error("Select an existing Send effect and compatible destination");const auto [buffer,effect]=s->sends.at(row);if(!s->host.audio_path_document(s->selected).set_send_destination(buffer,effect,s->destinations.at(destination)))throw std::runtime_error("Send unchanged or invalid; destination must be a compatible mix-in without PChannels, buses or a local cycle");break;}
        case RuntimeSettings:show_runtime_settings(w,s->host,RuntimeDocumentKind::AudioPath,s->selected);break;case FileOutput:{const auto selected=SendDlgItemMessageW(w,Buffers,CB_GETCURSEL,0,0);if(selected==CB_ERR||!s->host.audio_path_document(s->selected).add_file_output(static_cast<size_t>(selected)))throw std::runtime_error("Select a supported buffer without FileOutput; stereo predefined buffers can be materialized");break;}case RuntimeSave:{const auto path=file(w,true,true);if(path.empty())return 0;s->host.save_runtime_as(RuntimeDocumentKind::AudioPath,s->selected,path);break;}case PortSet:case RouteSet:{const auto row=SendDlgItemMessageW(w,Routes,CB_GETCURSEL,0,0);if(row==CB_ERR)throw std::runtime_error("Select a PChannel route");const auto [p,r]=s->routes.at(row);auto& doc=s->host.audio_path_document(s->selected);const auto changed=id==PortSet?doc.set_port_range(p,number(w,PortBase),number(w,PortCount)):doc.set_route_range(p,r,number(w,RouteBase),number(w,RouteCount));if(!changed)throw std::runtime_error("Range unchanged, overlapping, or outside its port; port resizing must retain all routes");break;}case Embed:{const auto target=SendDlgItemMessageW(w,Segments,CB_GETCURSEL,0,0);if(target==CB_ERR||!s->host.assign_audio_path(target,s->selected))throw std::runtime_error("Select a Segment; assignment must change its configuration");break;}case Remove:{const auto target=SendDlgItemMessageW(w,Segments,CB_GETCURSEL,0,0);if(target==CB_ERR||!s->host.document(target).remove_audio_path())throw std::runtime_error("Select a Segment containing an AudioPath");break;}case New:s->selected=s->host.new_audio_path();break;case Open:{const auto path=file(w,false);if(path.empty())return 0;s->selected=s->host.open_audio_path(path);break;}case Save:{const auto path=file(w,true);if(path.empty())return 0;s->host.save_audio_path(s->selected,path);break;}case NameSet:if(!s->host.audio_path_document(s->selected).set_name(value(w,Name)))throw std::runtime_error("Name unchanged or invalid");break;case Undo:s->host.audio_path_document(s->selected).undo();break;case Redo:s->host.audio_path_document(s->selected).redo();break;case Connect:{const auto row=SendDlgItemMessageW(w,Routes,CB_GETCURSEL,0,0),buffer=SendDlgItemMessageW(w,Buffers,CB_GETCURSEL,0,0);if(row==CB_ERR||buffer==CB_ERR)throw std::runtime_error("Select a route and buffer");const auto [p,r]=s->routes.at(row);if(!s->host.audio_path_document(s->selected).set_route_buffers(p,r,{s->host.audio_path_document(s->selected).buffers().at(buffer)}))throw std::runtime_error("Route unchanged or invalid");break;}default:return 0;}refresh(w,*s);return 0;
    }if(msg==WM_CLOSE){DestroyWindow(w);return 0;}}catch(const std::exception& e){const std::string detail=e.what();MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"AudioPath",MB_OK|MB_ICONERROR);if(msg==WM_CREATE)return -1;}return DefWindowProcW(w,msg,wp,lp);}
}
void show_audio_path_editor(HWND parent,Framework& host,size_t initialIndex){const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerAudioPaths";if(!RegisterClassW(&c))throw std::runtime_error("AudioPath window registration failed");registered=true;}Session session{host};session.selected=initialIndex;const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerAudioPaths",L"AudioPath Documents",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,680,880,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("AudioPath window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result==-1)throw std::runtime_error("AudioPath message loop failed");}
}
