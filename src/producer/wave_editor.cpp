#include "wave_editor.h"
#include "wave_playback.h"
#include <commctrl.h>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <stdexcept>
namespace producer::app {
namespace {
enum : UINT {Items=10,Time,Offset,Duration,Volume,Pitch,Change,Undo,Redo,Status,Copy,Paste,Delete,OwnedWave,Variations,InsertRef,Tabs,LoopEnabled,LoopStart,LoopEnd,LoopApply,LoopLength};
struct Session {
    Framework& host;size_t document;WaveEditorContext& context;size_t selected=0;
    std::vector<WaveItem> items;std::vector<size_t> waveIndices;
    std::vector<HWND> wavePage,loopPage;
    SegmentDocument& doc(){return host.document(document);}
};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id){
    auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);
    if(!c)throw std::runtime_error("Wave control creation failed");
    SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
std::int64_t number(HWND w,UINT id){wchar_t b[80]{};GetDlgItemTextW(w,id,b,80);std::wstring text=b;size_t end=0;auto n=std::stoll(text,&end);if(end!=text.size())throw std::runtime_error("Enter a whole number");return n;}
std::int32_t int32(HWND w,UINT id){auto n=number(w,id);if(n<INT32_MIN||n>INT32_MAX)throw std::runtime_error("Volume or pitch outside signed 32-bit range");return static_cast<std::int32_t>(n);}
std::uint32_t sample_number(HWND w,UINT id){
    wchar_t b[80]{};GetDlgItemTextW(w,id,b,80);const std::wstring text=b;
    if(text.empty()||text.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Loop samples require nonnegative whole numbers");
    const auto value=std::stoull(text);if(value>UINT32_MAX)throw std::runtime_error("Loop sample exceeds the supported range");return static_cast<std::uint32_t>(value);
}
void loop_fields(HWND w,Session& s){
    const bool enabled=!s.items.empty()&&IsDlgButtonChecked(w,LoopEnabled)==BST_CHECKED;
    for(auto id:{LoopStart,LoopEnd})EnableWindow(GetDlgItem(w,id),enabled);
}
void show_page(HWND w,Session& s){
    const bool loop=TabCtrl_GetCurSel(GetDlgItem(w,Tabs))==1;
    for(auto c:s.wavePage)ShowWindow(c,loop?SW_HIDE:SW_SHOW);
    for(auto c:s.loopPage)ShowWindow(c,loop?SW_SHOW:SW_HIDE);
}
void refresh(HWND w,Session& s){
    auto sources=GetDlgItem(w,OwnedWave);const auto prior=SendMessageW(sources,CB_GETCURSEL,0,0);SendMessageW(sources,CB_RESETCONTENT,0,0);s.waveIndices.clear();
    for(size_t i=0;i<s.host.waves().size();++i){const auto path=std::filesystem::path(s.host.waves()[i].path);auto ext=path.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});if(ext==L".wvp"){s.waveIndices.push_back(i);SendMessageW(sources,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(path.filename().c_str()));}}
    if(!s.waveIndices.empty())SendMessageW(sources,CB_SETCURSEL,prior>=0&&static_cast<size_t>(prior)<s.waveIndices.size()?prior:0,0);
    EnableWindow(GetDlgItem(w,InsertRef),!s.waveIndices.empty());s.items=s.doc().waves();auto list=GetDlgItem(w,Items);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(const auto& e:s.items){auto text=e.filename+L" / part "+std::to_wstring(e.part+1)+L" / item "+std::to_wstring(e.index+1)+L" / PChannel "+std::to_wstring(static_cast<std::uint64_t>(e.pchannel)+1);SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    for(auto id:{Change,Copy,Delete,LoopEnabled,LoopApply})EnableWindow(GetDlgItem(w,id),!s.items.empty());
    EnableWindow(GetDlgItem(w,Paste),!s.context.clipboard.empty());EnableWindow(GetDlgItem(w,Undo),s.doc().can_undo());EnableWindow(GetDlgItem(w,Redo),s.doc().can_redo());
    if(s.items.empty()){CheckDlgButton(w,LoopEnabled,BST_UNCHECKED);for(auto id:{LoopStart,LoopEnd,LoopLength})SetDlgItemTextW(w,id,L"0");loop_fields(w,s);SetDlgItemTextW(w,Status,L"No Wave events. Insert a Project WVP into the retained clock-time part, or paste a copied event.");return;}
    s.selected=std::min(s.selected,s.items.size()-1);SendMessageW(list,CB_SETCURSEL,s.selected,0);s.context.part=s.items[s.selected].part;const auto& item=s.items[s.selected];const auto& e=item.placement;
    for(auto pair:{std::pair<UINT,std::int64_t>{Time,e.time},{Offset,e.startOffset},{Duration,e.duration},{Volume,e.volume},{Pitch,e.pitch}})SetDlgItemTextW(w,pair.first,std::to_wstring(pair.second).c_str());
    CheckDlgButton(w,LoopEnabled,item.loop.enabled()?BST_CHECKED:BST_UNCHECKED);
    SetDlgItemTextW(w,LoopStart,std::to_wstring(item.loop.start).c_str());SetDlgItemTextW(w,LoopEnd,std::to_wstring(item.loop.end).c_str());
    SetDlgItemTextW(w,LoopLength,std::to_wstring(item.loop.enabled()?static_cast<std::uint64_t>(item.loop.end)-item.loop.start+1:0).c_str());loop_fields(w,s);
    SetDlgItemTextW(w,Status,s.doc().dirty()?L"Modified. Save the Segment or Project from File.":L"Saved. Loop settings belong to this Segment; the source Wave is unchanged.");
}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
    if(msg==WM_CREATE){
        s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,16,570,250,Items);
        auto tabs=control(w,WC_TABCONTROLW,L"",WS_TABSTOP|WS_CLIPSIBLINGS,16,52,570,644,Tabs);TCITEMW tab{};tab.mask=TCIF_TEXT;tab.pszText=const_cast<wchar_t*>(L"Wave");TabCtrl_InsertItem(tabs,0,&tab);tab.pszText=const_cast<wchar_t*>(L"Loop");TabCtrl_InsertItem(tabs,1,&tab);
        auto page=[&](std::vector<HWND>& p,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id){p.push_back(control(w,cls,text,flags,x,y,width,height,id));};
        const wchar_t* labels[]={L"Start time (100 ns; clock-time tracks)",L"Start offset into audio (100 ns)",L"Duration (100 ns; clock-time tracks)",L"Volume (hundredths of dB, at most 0)",L"Pitch offset (hundredths of semitone)"};
        for(unsigned i=0;i<5;++i){page(s->wavePage,L"STATIC",labels[i],0,28,88+56*i,546,22,0);page(s->wavePage,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,28,112+56*i,546,26,Time+i);}
        page(s->wavePage,L"BUTTON",L"Apply",WS_TABSTOP,28,385,158,30,Change);
        control(w,L"BUTTON",L"Undo",WS_TABSTOP,216,385,170,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,404,385,170,30,Redo);
        page(s->wavePage,L"BUTTON",L"Copy event",WS_TABSTOP,28,430,158,30,Copy);page(s->wavePage,L"BUTTON",L"Paste at start time",WS_TABSTOP,216,430,170,30,Paste);page(s->wavePage,L"BUTTON",L"Delete event",WS_TABSTOP,404,430,170,30,Delete);
        page(s->wavePage,L"STATIC",L"Project Wave to insert",0,28,476,546,22,0);page(s->wavePage,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,28,500,546,180,OwnedWave);
        page(s->wavePage,L"STATIC",L"Variation mask (nonzero, in this part)",0,28,542,340,22,0);page(s->wavePage,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,28,570,158,26,Variations);page(s->wavePage,L"BUTTON",L"Insert selected Wave",WS_TABSTOP,216,565,358,32,InsertRef);
        page(s->loopPage,L"BUTTON",L"Loop",BS_AUTOCHECKBOX|WS_TABSTOP,28,88,170,28,LoopEnabled);
        page(s->loopPage,L"STATIC",L"Start sample",0,28,136,546,22,0);page(s->loopPage,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,28,160,546,26,LoopStart);
        page(s->loopPage,L"STATIC",L"End sample (last sample in the loop)",0,28,208,546,22,0);page(s->loopPage,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,28,232,546,26,LoopEnd);
        page(s->loopPage,L"STATIC",L"Length in samples",0,28,280,546,22,0);page(s->loopPage,L"STATIC",L"0",0,28,304,546,26,LoopLength);
        page(s->loopPage,L"BUTTON",L"Apply loop",WS_TABSTOP,28,385,158,30,LoopApply);
        control(w,L"STATIC",L"",0,16,712,570,48,Status);refresh(w,*s);show_page(w,*s);return 0;
    }
    if(msg==WM_NOTIFY&&s&&reinterpret_cast<NMHDR*>(lp)->idFrom==Tabs&&reinterpret_cast<NMHDR*>(lp)->code==TCN_SELCHANGE){show_page(w,*s);return 0;}
    if(msg==WM_COMMAND&&s){switch(LOWORD(wp)){
    case Items:if(HIWORD(wp)==CBN_SELCHANGE){s->selected=static_cast<size_t>(SendDlgItemMessageW(w,Items,CB_GETCURSEL,0,0));refresh(w,*s);}return 0;
    case LoopEnabled:if(HIWORD(wp)==BN_CLICKED)loop_fields(w,*s);return 0;
    case LoopApply:{
        const auto& item=s->items.at(s->selected);WaveItemLoop loop;
        if(IsDlgButtonChecked(w,LoopEnabled)==BST_CHECKED){loop={sample_number(w,LoopStart),sample_number(w,LoopEnd)};if(!loop.enabled())throw std::runtime_error("End sample must be after Start sample");}
        auto next=s->doc();if(next.edit_wave_loop(item.part,item.index,loop)){
            // The clone owns its history. Reject invalid PCM bounds before
            // publishing any document/dirty/UndoRedo change to the host.
            (void)prepare_wave_playback(next.save_bytes(),s->host.playback_waves(s->document));s->doc()=std::move(next);
        }break;
    }
    case Change:{const auto& item=s->items.at(s->selected);WavePlacement e{number(w,Time),number(w,Offset),number(w,Duration),int32(w,Volume),int32(w,Pitch)};if(!s->doc().edit_wave(item.part,item.index,e))throw std::runtime_error("Placement unchanged or invalid: time/offset must be nonnegative, duration positive, volume at most 0, end time must not overflow");break;}
    case InsertRef:{const auto selected=SendDlgItemMessageW(w,OwnedWave,CB_GETCURSEL,0,0);if(selected<0||static_cast<size_t>(selected)>=s->waveIndices.size())throw std::runtime_error("Open a saved Project WVP before insertion");const auto variations=number(w,Variations);if(variations<=0||variations>UINT32_MAX)throw std::runtime_error("Choose a nonzero DWORD variation mask");size_t after=0;if(!s->host.insert_segment_wave(s->document,s->waveIndices[selected],s->context.part,number(w,Time),static_cast<std::uint32_t>(variations),&after))throw std::runtime_error("Select an existing clock-time Wave part and valid start time");const auto values=s->doc().waves();const auto item=std::find_if(values.begin(),values.end(),[&](const auto& e){return e.part==s->context.part&&e.index==after;});if(item==values.end())throw std::runtime_error("Inserted Wave selection missing");s->selected=static_cast<size_t>(item-values.begin());break;}
    case Copy:{const auto& e=s->items.at(s->selected);s->context.clipboard=s->doc().copy_wave(e.part,e.index);break;}
    case Paste:{size_t after=0;if(!s->doc().paste_wave(s->context.clipboard,s->context.part,number(w,Time),0,&after))throw std::runtime_error("Invalid Wave paste time or duration");const auto values=s->doc().waves();auto item=std::find_if(values.begin(),values.end(),[&](const auto& e){return e.part==s->context.part&&e.index==after;});s->selected=static_cast<size_t>(item-values.begin());break;}
    case Delete:{const auto& e=s->items.at(s->selected);if(!s->doc().delete_wave(e.part,e.index))throw std::runtime_error("Wave event not found");break;}
    case Undo:s->doc().undo();break;case Redo:s->doc().redo();break;default:return 0;
    }refresh(w,*s);return 0;}
    }catch(const std::exception& e){std::string text=e.what();std::wstring wide(text.begin(),text.end());MessageBoxW(w,wide.c_str(),L"Wave edit failed",MB_OK|MB_ICONERROR);return 0;}
    return DefWindowProcW(w,msg,wp,lp);
}
}
void show_wave_editor(HWND parent,Framework& host,size_t index,WaveEditorContext& context){
    (void)host.document(index).waves();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TAB_CLASSES};if(!InitCommonControlsEx(&controls))throw std::runtime_error("Wave tab controls unavailable");
    const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerWave";if(!RegisterClassW(&c))throw std::runtime_error("Wave class registration failed");registered=true;}
    Session session{host,index,context};auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerWave",L"Segment Wave Properties",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,625,820,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("Wave window creation failed");
    bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(!result)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Wave message loop failed");
}
}
