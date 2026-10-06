#include "chord_editor.h"
#include <commdlg.h>
#include <algorithm>
#include <stdexcept>
namespace producer::app {
namespace {
enum : UINT {Track=10,Apply,Events,Level,Time,Name,Flags,Pattern,Scale,Inversion,Levels,Root,ScaleRoot,Add,Change,Delete,AddLevel,DeleteLevel,Undo,Redo,Save,Status};
struct Session {
    Framework& framework;size_t documentIndex;CommandEditorContext& context;size_t level=0;
    SegmentDocument& doc(){return framework.document(documentIndex);}
    size_t& track(){return context.tracks[doc().selected_groups()];}
    size_t& selected(){return context.events[{doc().selected_groups(),track()}];}
};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){const auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("Chord control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
std::wstring text(HWND w,UINT id){const auto c=GetDlgItem(w,id);const int n=GetWindowTextLengthW(c);std::wstring s(static_cast<size_t>(n)+1,L'\0');GetWindowTextW(c,s.data(),n+1);s.resize(n);return s;}
std::uint32_t number(HWND w,UINT id,std::uint32_t maximum){const auto s=text(w,id);if(s.empty()||s[0]==L'-')throw std::runtime_error("Chord field requires a nonnegative number");size_t end=0;const auto n=std::stoull(s,&end,0);if(end!=s.size()||n>maximum)throw std::runtime_error("Chord field outside its allowed range");return static_cast<std::uint32_t>(n);}
void set(HWND w,UINT id,std::uint32_t value){SetWindowTextW(GetDlgItem(w,id),std::to_wstring(value).c_str());}
void refresh(HWND w,Session& s){
    const auto events=s.doc().chords(s.track());set(w,Track,static_cast<unsigned>(s.track()+1));const auto list=GetDlgItem(w,Events);SendMessageW(list,CB_RESETCONTENT,0,0);
    for(size_t i=0;i<events.size();++i){const auto& e=events[i];const auto label=std::to_wstring(i+1)+L": "+e.name+L", measure "+std::to_wstring(e.measure+1)+L", beat "+std::to_wstring(e.beat+1);SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    ChordEvent e;if(!events.empty()){s.selected()=std::min(s.selected(),events.size()-1);SendMessageW(list,CB_SETCURSEL,s.selected(),0);e=events[s.selected()];}
    set(w,Time,static_cast<std::uint32_t>(e.time));SetWindowTextW(GetDlgItem(w,Name),e.name.c_str());set(w,Flags,e.flags);
    const auto levels=GetDlgItem(w,Level);SendMessageW(levels,CB_RESETCONTENT,0,0);for(size_t i=0;i<e.subchords.size();++i){const auto label=L"Subchord "+std::to_wstring(i+1);SendMessageW(levels,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    s.level=std::min(s.level,e.subchords.size()-1);SendMessageW(levels,CB_SETCURSEL,s.level,0);const auto& sub=e.subchords[s.level];set(w,Pattern,sub.chordPattern);set(w,Scale,sub.scalePattern);set(w,Inversion,sub.inversionPoints);set(w,Levels,sub.levels);set(w,Root,sub.chordRoot);set(w,ScaleRoot,sub.scaleRoot);
    for(const auto id:{Change,Delete,AddLevel,DeleteLevel})EnableWindow(GetDlgItem(w,id),!events.empty());
    const auto message=std::wstring(s.doc().dirty()?L"Modified. ":L"Saved. ")+std::to_wstring(events.size())+L" chords. One chord per beat; placing a chord on an occupied beat replaces it. Roots 0-23; patterns use 24 bits. Numeric fields accept decimal or 0x hex.";SetWindowTextW(GetDlgItem(w,Status),message.c_str());
}
ChordEvent input(HWND w,Session& s,bool editing){
    const auto events=s.doc().chords(s.track());ChordEvent e;if(editing){if(s.selected()>=events.size())throw std::runtime_error("Select a chord");e=events[s.selected()];}
    e.time=static_cast<std::int32_t>(number(w,Time,INT32_MAX));e.name=text(w,Name);e.flags=static_cast<std::uint8_t>(number(w,Flags,255));const size_t i=editing?s.level:0;auto& sub=e.subchords.at(i);sub.chordPattern=number(w,Pattern,UINT32_MAX);sub.scalePattern=number(w,Scale,UINT32_MAX);sub.inversionPoints=number(w,Inversion,UINT32_MAX);sub.levels=number(w,Levels,UINT32_MAX);sub.chordRoot=static_cast<std::uint8_t>(number(w,Root,255));sub.scaleRoot=static_cast<std::uint8_t>(number(w,ScaleRoot,255));return e;
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));try {
        if(message==WM_CREATE){s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
            control(w,L"STATIC",L"Chord track in current group selection (1-based)",0,16,16,390,20);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,420,16,90,26,Track);control(w,L"BUTTON",L"Select track",WS_TABSTOP,530,16,130,28,Apply);
            control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,58,644,220,Events);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,100,644,220,Level);
            const wchar_t* labels[]={L"Clocks (snapped to beat)",L"Chord name (16 characters)",L"Flags (0 normal, 1 silent)",L"Chord pattern",L"Scale pattern",L"Inversion points",L"Levels mask",L"Chord root (0-23)",L"Scale root (0-23)"};
            for(unsigned i=0;i<9;++i){const int y=148+static_cast<int>(i)*34;control(w,L"STATIC",labels[i],0,16,y,390,22);control(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,420,y,240,26,Time+i);}
            control(w,L"BUTTON",L"Place Chord",WS_TABSTOP,16,470,200,30,Add);control(w,L"BUTTON",L"Change Chord",WS_TABSTOP,236,470,200,30,Change);control(w,L"BUTTON",L"Delete Chord...",WS_TABSTOP,456,470,204,30,Delete);
            control(w,L"BUTTON",L"Add Subchord",WS_TABSTOP,16,512,310,30,AddLevel);control(w,L"BUTTON",L"Delete Subchord",WS_TABSTOP,346,512,314,30,DeleteLevel);
            control(w,L"BUTTON",L"Undo",WS_TABSTOP,16,554,120,30,Undo);control(w,L"BUTTON",L"Redo",WS_TABSTOP,156,554,120,30,Redo);control(w,L"BUTTON",L"Save Segment As...",WS_TABSTOP,296,554,364,30,Save);control(w,L"STATIC",L"",0,16,600,644,90,Status);refresh(w,*s);return 0;}
        if(message==WM_COMMAND&&s){const auto id=LOWORD(wp),notification=HIWORD(wp);switch(id){
            case Events:if(notification==CBN_SELCHANGE){s->selected()=static_cast<size_t>(SendMessageW(GetDlgItem(w,Events),CB_GETCURSEL,0,0));s->level=0;refresh(w,*s);}return 0;
            case Level:if(notification==CBN_SELCHANGE){s->level=static_cast<size_t>(SendMessageW(GetDlgItem(w,Level),CB_GETCURSEL,0,0));refresh(w,*s);}return 0;
            case Apply:{const auto n=number(w,Track,INT32_MAX);if(!n)throw std::runtime_error("Track numbers start at 1");(void)s->doc().chords(n-1);s->track()=n-1;s->selected()=0;s->level=0;break;}
            case Add:{auto e=input(w,*s,false);if(!s->doc().set_chord(e,s->track()))throw std::runtime_error("Chord unchanged or invalid");const auto p=s->doc().timeline().position(e.time);const auto events=s->doc().chords(s->track());for(size_t i=0;i<events.size();++i)if(events[i].measure==p.measure&&events[i].beat==p.beat)s->selected()=i;s->level=0;break;}
            case Change:{size_t index=0;if(!s->doc().edit_chord(s->selected(),input(w,*s,true),s->track(),&index))throw std::runtime_error("Chord unchanged or invalid");s->selected()=index;break;}
            case Delete:if(MessageBoxW(w,L"Delete the selected chord? Undo restores it.",L"Delete Chord",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES)s->doc().delete_chord(s->selected(),s->track());break;
            case AddLevel:case DeleteLevel:{auto e=input(w,*s,true);if(id==AddLevel){if(e.subchords.size()>=8)throw std::runtime_error("At most eight subchords");e.subchords.push_back(SubChord{});s->level=e.subchords.size()-1;}else{if(e.subchords.size()<=1)throw std::runtime_error("Keep at least one subchord");e.subchords.erase(e.subchords.begin()+s->level);s->level=0;}if(!s->doc().edit_chord(s->selected(),e,s->track()))throw std::runtime_error("Subchord change rejected");break;}
            case Undo:s->doc().undo();break;case Redo:s->doc().redo();break;
            case Save:{wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFilter=L"Segment (*.sgp)\0*.sgp\0\0";o.lpstrFile=file;o.nMaxFile=32768;o.lpstrDefExt=L"sgp";o.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;if(GetSaveFileNameW(&o))s->framework.save_segment(s->documentIndex,file);else if(CommDlgExtendedError())throw std::runtime_error("Chord save dialog failed");break;}
            default:return 0;
        }refresh(w,*s);return 0;}
    }catch(const std::exception& e){const std::string a=e.what();const std::wstring messageText(a.begin(),a.end());MessageBoxW(w,messageText.c_str(),L"Chord edit failed",MB_OK|MB_ICONERROR);return 0;}return DefWindowProcW(w,message,wp,lp);
}
}
void show_chord_editor(HWND parent,Framework& f,size_t index,CommandEditorContext& context){
    const auto initial=f.document(index).chords(context.tracks[f.document(index).selected_groups()]);if(std::any_of(initial.begin(),initial.end(),[](const ChordEvent& e){return e.subchords.empty();}))throw std::runtime_error("Imported empty SubChord arrays are retained but cannot be edited in this window");static bool registered=false;const auto instance=GetModuleHandleW(nullptr);
    if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerChords";if(!RegisterClassW(&c))throw std::runtime_error("Chord window registration failed");registered=true;}
    Session session{f,index,context};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerChords",L"Segment Chords",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,710,750,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("Chord window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("Chord message loop failed");
}
}
