#include "dls_editor.h"
#include "runtime_settings_editor.h"
#include "articulation_editor.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <commdlg.h>

namespace producer::app {
namespace {
constexpr UINT Articulation=92,RuntimeSave=93,RuntimeSettings=94;
enum : UINT {Instrument=10,Bank,Program,SetInstrument,Region=20,KeyLow,KeyHigh,VelocityLow,VelocityHigh,KeyGroup,Cue,SetRegion,DuplicateRegion,DeleteRegion,CreateRegion,Wave=40,Percent,Scale,DuplicateWave,ImportPcm,ExportPcm,DeleteWave,UseReplacement,ReplacementCue,Save=60,Undo,Redo,Info,Status,SaveAs,WaveLoop=70,WaveLoopEnabled,WaveLoopType,WaveLoopStart,WaveLoopLength,ApplyWaveLoop,RegionLoop=80,RegionLoopEnabled,RegionLoopType,RegionLoopStart,RegionLoopLength,ApplyRegionLoop,InheritSample,SampleSource,AddPcm=90,AddInstrument};
std::wstring pcm_path(HWND owner,bool save){wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=owner;o.lpstrFilter=L"PCM WAV (*.wav)\0*.wav\0\0";o.lpstrFile=file;o.nMaxFile=32768;o.lpstrDefExt=L"wav";o.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);const auto ok=save?GetSaveFileNameW(&o):GetOpenFileNameW(&o);if(!ok&&CommDlgExtendedError())throw std::runtime_error("PCM file dialog failed");return ok?file:L"";}
std::wstring dls_save_path(HWND owner,bool runtime=false){wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=owner;o.lpstrFilter=runtime?L"Runtime DLS\0*.dls\0\0":L"DLS collection (*.dlp;*.dls)\0*.dlp;*.dls\0\0";o.lpstrFile=file;o.nMaxFile=32768;o.lpstrDefExt=runtime?L"dls":L"dlp";o.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;const auto ok=GetSaveFileNameW(&o);if(!ok&&CommDlgExtendedError())throw std::runtime_error("DLS save dialog failed");return ok?file:L"";}
struct Session {Framework& framework;size_t index;bool savedAs=false;DlsDocument& doc(){return framework.collection_document(index);}};
HWND add(HWND w,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,UINT id=0){const auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("DLS control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
void text(HWND w,UINT id,unsigned long long value){SetWindowTextW(GetDlgItem(w,id),std::to_wstring(value).c_str());}
size_t selected(HWND w,UINT id){const auto at=SendMessageW(GetDlgItem(w,id),CB_GETCURSEL,0,0);if(at<0)throw std::runtime_error("Select an item");return static_cast<size_t>(at);}
unsigned number(HWND w,UINT id,unsigned maximum){const auto c=GetDlgItem(w,id);const auto length=GetWindowTextLengthW(c);std::wstring s(static_cast<size_t>(length)+1,L'\0');GetWindowTextW(c,s.data(),length+1);s.resize(length);size_t end=0;const auto value=std::stoll(s,&end);if(end!=s.size()||value<0||static_cast<unsigned long long>(value)>maximum)throw std::runtime_error("Field is outside its allowed range");return static_cast<unsigned>(value);}
void populate(HWND w,UINT id,size_t count,const wchar_t* prefix){const auto c=GetDlgItem(w,id);const auto old=SendMessageW(c,CB_GETCURSEL,0,0);SendMessageW(c,CB_RESETCONTENT,0,0);for(size_t i=0;i<count;++i){const auto label=std::wstring(prefix)+L" "+std::to_wstring(i+1);SendMessageW(c,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}if(count)SendMessageW(c,CB_SETCURSEL,old>=0&&static_cast<size_t>(old)<count?old:0,0);EnableWindow(c,count!=0);}
void loop_fields(HWND w,UINT id,const std::vector<DlsLoop>& loops,bool available){
    populate(w,id,available?loops.size()+1:0,L"Loop");const auto combo=GetDlgItem(w,id);
    if(available){const auto at=selected(w,id);SendMessageW(combo,CB_DELETESTRING,loops.size(),0);SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"New loop"));SendMessageW(combo,CB_SETCURSEL,at,0);const auto value=at<loops.size()?loops[at]:DlsLoop{0,0,1};text(w,id+2,value.type);text(w,id+3,value.start);text(w,id+4,value.length);}
    SendMessageW(GetDlgItem(w,id+1),BM_SETCHECK,loops.empty()?BST_UNCHECKED:BST_CHECKED,0);
    for(UINT offset=1;offset<=5;++offset)EnableWindow(GetDlgItem(w,id+offset),available);
}
std::vector<DlsLoop> edited_loops(HWND w,UINT id,std::vector<DlsLoop> loops){
    if(SendMessageW(GetDlgItem(w,id+1),BM_GETCHECK,0,0)!=BST_CHECKED)return {};
    const auto at=selected(w,id);const DlsLoop value{number(w,id+2,1),number(w,id+3,UINT32_MAX),number(w,id+4,UINT32_MAX)};
    if(at<loops.size())loops[at]=value;else if(at==loops.size())loops.push_back(value);else throw std::runtime_error("Loop selection changed");return loops;
}
void add_loop_panel(HWND w,UINT id,int y,const wchar_t* label){
    add(w,L"STATIC",label,0,760,y,300,20);add(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,760,y+26,285,160,id);
    add(w,L"BUTTON",L"Loop playback",BS_AUTOCHECKBOX|WS_TABSTOP,760,y+60,285,24,id+1);
    add(w,L"STATIC",L"Type: 0 forward, 1 loop and release",0,760,y+90,310,20);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,760,y+112,285,26,id+2);
    add(w,L"STATIC",L"Start / length (sample frames)",0,760,y+148,310,20);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,760,y+172,135,26,id+3);add(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,910,y+172,135,26,id+4);
    add(w,L"BUTTON",L"Apply loop settings",WS_TABSTOP,760,y+210,285,30,id+5);
}
void refresh(HWND w,Session& s){
    const auto instruments=s.doc().instruments();const auto waves=s.doc().waves();populate(w,Instrument,instruments.size(),L"Instrument");populate(w,Wave,waves.size(),L"Wave");
    const bool hasInstrument=!instruments.empty();for(const auto id:{Bank,Program})EnableWindow(GetDlgItem(w,id),hasInstrument||!waves.empty());EnableWindow(GetDlgItem(w,SetInstrument),hasInstrument);EnableWindow(GetDlgItem(w,AddInstrument),!waves.empty());
    if(hasInstrument){const auto& i=instruments.at(selected(w,Instrument));text(w,Bank,i.bank);text(w,Program,i.program);populate(w,Region,i.regions.size(),L"Region");if(!i.regions.empty()){const auto& r=i.regions.at(selected(w,Region));text(w,KeyLow,r.keyLow);text(w,KeyHigh,r.keyHigh);text(w,VelocityLow,r.velocityLow);text(w,VelocityHigh,r.velocityHigh);text(w,KeyGroup,r.keyGroup);text(w,Cue,r.tableIndex);}else{text(w,KeyLow,0);text(w,KeyHigh,127);text(w,VelocityLow,0);text(w,VelocityHigh,127);text(w,KeyGroup,0);text(w,Cue,0);}for(const auto id:{KeyLow,KeyHigh,VelocityLow,VelocityHigh,KeyGroup,Cue})EnableWindow(GetDlgItem(w,id),!i.regions.empty()||!waves.empty());for(const auto id:{SetRegion,DuplicateRegion,DeleteRegion})EnableWindow(GetDlgItem(w,id),!i.regions.empty());}
    else{populate(w,Region,0,L"Region");for(const auto id:{KeyLow,KeyHigh,VelocityLow,VelocityHigh,KeyGroup,Cue,SetRegion,DuplicateRegion,DeleteRegion})EnableWindow(GetDlgItem(w,id),FALSE);text(w,Bank,0);text(w,Program,0);text(w,Cue,0);EnableWindow(GetDlgItem(w,Cue),!waves.empty());}
    EnableWindow(GetDlgItem(w,CreateRegion),hasInstrument&&!waves.empty());
    EnableWindow(GetDlgItem(w,Articulation),hasInstrument);
    std::wstring detail=L"No waves";bool pcm=false;if(!waves.empty()){const auto& wave=waves.at(selected(w,Wave));pcm=wave.format==1&&(wave.bits==8||wave.bits==16)&&wave.blockAlign==wave.channels*(wave.bits/8);detail=std::to_wstring(wave.channels)+L" channels, "+std::to_wstring(wave.sampleRate)+L" Hz, "+std::to_wstring(wave.bits)+L" bits, "+std::to_wstring(wave.frames)+(wave.format==1?L" frames":L" encoded blocks");}SetWindowTextW(GetDlgItem(w,Info),detail.c_str());EnableWindow(GetDlgItem(w,Percent),pcm);EnableWindow(GetDlgItem(w,Scale),pcm);
    EnableWindow(GetDlgItem(w,DuplicateWave),!waves.empty());
    EnableWindow(GetDlgItem(w,DeleteWave),!waves.empty());EnableWindow(GetDlgItem(w,UseReplacement),!waves.empty());EnableWindow(GetDlgItem(w,ReplacementCue),!waves.empty()&&SendMessageW(GetDlgItem(w,UseReplacement),BM_GETCHECK,0,0)==BST_CHECKED);
    EnableWindow(GetDlgItem(w,ImportPcm),pcm);EnableWindow(GetDlgItem(w,ExportPcm),pcm);
    loop_fields(w,WaveLoop,waves.empty()?std::vector<DlsLoop>{}:s.doc().wave_loops(selected(w,Wave)),pcm);
    const bool hasRegion=hasInstrument&&!instruments.at(selected(w,Instrument)).regions.empty();
    loop_fields(w,RegionLoop,hasRegion?s.doc().effective_region_loops(selected(w,Instrument),selected(w,Region)):std::vector<DlsLoop>{},hasRegion);
    const bool inherited=hasRegion&&s.doc().region_inherits_wave_sample(selected(w,Instrument),selected(w,Region));EnableWindow(GetDlgItem(w,InheritSample),hasRegion&&!inherited);
    SetWindowTextW(GetDlgItem(w,SampleSource),!hasRegion?L"No Region":inherited?L"Using Wave sample settings":L"Explicit Region sample settings");
    const auto title=L"DLS — "+std::filesystem::path(s.framework.collections().at(s.index).path).filename().wstring()+(s.doc().dirty()?L" *":L"");SetWindowTextW(w,title.c_str());SetWindowTextW(GetDlgItem(w,Status),s.doc().dirty()?L"Unsaved changes. Save this DLS before saving the project. Changes apply on the next Play.":s.savedAs?L"Saved under the new name. Save changed Band, Style and Segment documents before saving the project.":L"Saved. Closing keeps this document in the project. Changes apply on the next Play.");
}
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Session*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try{switch(message){
    case WM_CREATE:{s=static_cast<Session*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        add(w,L"BUTTON",L"Articulation...",WS_TABSTOP,265,280,450,26,Articulation);
        add(w,L"STATIC",L"Instrument",0,16,12,170,20);add(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,36,230,200,Instrument);
        add(w,L"STATIC",L"MIDI bank (including drum bit)",0,265,12,225,20);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,265,36,145,26,Bank);add(w,L"STATIC",L"Program (0..127)",0,425,12,150,20);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,425,36,110,26,Program);add(w,L"BUTTON",L"Set Instrument",WS_TABSTOP,555,34,160,30,SetInstrument);
        add(w,L"BUTTON",L"Create Region",WS_TABSTOP,265,76,140,26,CreateRegion);add(w,L"BUTTON",L"Add Instrument (full range)",WS_TABSTOP,410,76,305,26,AddInstrument);add(w,L"STATIC",L"Region",0,16,82,200,20);add(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,108,230,200,Region);
        const UINT ids[]={KeyLow,KeyHigh,VelocityLow,VelocityHigh,KeyGroup,Cue};const wchar_t* labels[]={L"Key low",L"Key high",L"Velocity low",L"Velocity high",L"Key group",L"Wave pool cue"};for(int i=0;i<6;++i){const auto x=16+(i%3)*245,y=154+(i/3)*72;add(w,L"STATIC",labels[i],0,x,y,190,20);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,x,y+24,200,26,ids[i]);}add(w,L"BUTTON",L"Set Region",WS_TABSTOP,555,108,160,30,SetRegion);add(w,L"BUTTON",L"Duplicate Region",WS_TABSTOP,265,108,140,30,DuplicateRegion);add(w,L"BUTTON",L"Delete Region",WS_TABSTOP,410,108,140,30,DeleteRegion);
        add(w,L"STATIC",L"Wave",0,16,308,200,20);add(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,334,230,200,Wave);add(w,L"STATIC",L"",0,265,336,450,42,Info);
        add(w,L"BUTTON",L"Delete Wave...",WS_TABSTOP,16,382,210,30,DeleteWave);add(w,L"BUTTON",L"Replace references with cue:",WS_TABSTOP|BS_AUTOCHECKBOX,265,386,240,24,UseReplacement);add(w,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,510,382,190,26,ReplacementCue);
        add(w,L"STATIC",L"PCM volume (% of current samples, 0..400)",0,16,422,430,20);add(w,L"EDIT",L"50",WS_BORDER|WS_TABSTOP,16,450,200,26,Percent);add(w,L"BUTTON",L"Scale Wave",WS_TABSTOP,265,448,190,30,Scale);
        add(w,L"BUTTON",L"Duplicate Wave",WS_TABSTOP,480,448,230,30,DuplicateWave);
        add(w,L"BUTTON",L"Import PCM WAV",WS_TABSTOP,16,484,210,30,ImportPcm);add(w,L"BUTTON",L"Export PCM WAV",WS_TABSTOP,265,484,210,30,ExportPcm);
        add(w,L"BUTTON",L"Add PCM Wave...",WS_TABSTOP,480,308,230,26,AddPcm);
        add(w,L"BUTTON",L"Save DLS As...",WS_TABSTOP,510,484,190,30,SaveAs);
        add(w,L"BUTTON",L"Save DLS",WS_TABSTOP,16,530,190,32,Save);add(w,L"BUTTON",L"Undo",WS_TABSTOP,265,530,190,32,Undo);add(w,L"BUTTON",L"Redo",WS_TABSTOP,510,530,190,32,Redo);
        // Keep runtime actions in the left column, clear of both loop panels.
        add(w,L"BUTTON",L"Runtime Save As...",WS_TABSTOP,16,574,220,32,RuntimeSave);add(w,L"BUTTON",L"Runtime Properties...",WS_TABSTOP,265,574,220,32,RuntimeSettings);add(w,L"STATIC",L"",0,16,614,710,50,Status);
        add_loop_panel(w,WaveLoop,12,L"Wave loops");add_loop_panel(w,RegionLoop,280,L"Region loops (apply creates an override)");add(w,L"STATIC",L"",0,760,526,310,20,SampleSource);add(w,L"BUTTON",L"Use Wave sample settings...",WS_TABSTOP,760,554,285,30,InheritSample);add(w,L"STATIC",L"Resets Region root, tuning, volume and loops.",0,760,590,310,32);refresh(w,*s);return 0;}
    case WM_COMMAND:{if(!s)return 0;const auto id=LOWORD(wp);const auto notification=HIWORD(wp);if(id==Instrument||id==Region||id==Wave||id==WaveLoop||id==RegionLoop){if(notification==CBN_SELCHANGE){if(id==Instrument)SendMessageW(GetDlgItem(w,Region),CB_SETCURSEL,0,0);refresh(w,*s);}return 0;}if(notification!=BN_CLICKED)return 0;bool changed=true;
        switch(id){case UseReplacement:EnableWindow(GetDlgItem(w,ReplacementCue),SendMessageW(GetDlgItem(w,UseReplacement),BM_GETCHECK,0,0)==BST_CHECKED);return 0;
        case DeleteWave:{const auto index=selected(w,Wave);const bool replace=SendMessageW(GetDlgItem(w,UseReplacement),BM_GETCHECK,0,0)==BST_CHECKED;const auto cue=replace?std::optional<std::uint32_t>(number(w,ReplacementCue,UINT32_MAX)):std::nullopt;
            // Validate a copy before offering deletion; cancellation publishes
            // neither bytes nor history, and malformed references fail first.
            auto next=s->doc();if(!next.remove_wave(index,cue)){changed=false;break;}
            const auto prompt=L"Delete Wave "+std::to_wstring(index+1)+(cue?L" and replace its Region references with pool cue "+std::to_wstring(*cue):L" (not referenced by any Region)")+L"?\nThis edits the owned DLS document. Save DLS writes it to disk. Undo restores the Wave.";
            if(MessageBoxW(w,prompt.c_str(),L"Delete DLS Wave",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;s->doc()=std::move(next);break;}
        case AddInstrument:{const auto bank=number(w,Bank,UINT32_MAX),program=number(w,Program,127),cue=number(w,Cue,UINT32_MAX);const auto name=std::string("Instrument ")+std::to_string(bank)+", "+std::to_string(program);changed=s->doc().create_instrument(bank,program,name,cue);if(changed){const auto count=s->doc().instruments().size();populate(w,Instrument,count,L"Instrument");SendMessageW(GetDlgItem(w,Instrument),CB_SETCURSEL,count-1,0);SendMessageW(GetDlgItem(w,Region),CB_SETCURSEL,0,0);}break;}
        case Articulation:{const auto instrument=selected(w,Instrument);const auto regions=s->doc().instruments().at(instrument).regions;show_articulation_editor(w,s->framework,s->index,instrument,regions.empty()?std::nullopt:std::optional<size_t>(selected(w,Region)));break;}
        case SetInstrument:changed=s->doc().set_instrument(selected(w,Instrument),number(w,Bank,UINT32_MAX),number(w,Program,127));break;
        case SetRegion:{const DlsRegion r{number(w,KeyLow,127),number(w,KeyHigh,127),number(w,VelocityLow,127),number(w,VelocityHigh,127),number(w,KeyGroup,15),number(w,Cue,UINT32_MAX)};changed=s->doc().set_region(selected(w,Instrument),selected(w,Region),r);break;}
        case CreateRegion:{const DlsRegion r{number(w,KeyLow,127),number(w,KeyHigh,127),number(w,VelocityLow,127),number(w,VelocityHigh,127),number(w,KeyGroup,15),number(w,Cue,UINT32_MAX)};changed=s->doc().create_region(selected(w,Instrument),r);if(changed){const auto count=s->doc().instruments().at(selected(w,Instrument)).regions.size();populate(w,Region,count,L"Region");SendMessageW(GetDlgItem(w,Region),CB_SETCURSEL,count-1,0);}break;}
        case DuplicateRegion:changed=s->doc().duplicate_region(selected(w,Instrument),selected(w,Region));if(changed){const auto count=s->doc().instruments().at(selected(w,Instrument)).regions.size();populate(w,Region,count,L"Region");SendMessageW(GetDlgItem(w,Region),CB_SETCURSEL,count-1,0);}break;
        case DeleteRegion:changed=s->doc().remove_region(selected(w,Instrument),selected(w,Region));break;
        case ApplyWaveLoop:case ApplyRegionLoop:{const bool wave=id==ApplyWaveLoop;const auto panel=wave?WaveLoop:RegionLoop;const auto old=wave?s->doc().wave_loops(selected(w,Wave)):s->doc().effective_region_loops(selected(w,Instrument),selected(w,Region));const auto loops=edited_loops(w,panel,old);auto next=s->doc();changed=wave?next.set_wave_loops(selected(w,Wave),loops):next.set_region_loops(selected(w,Instrument),selected(w,Region),loops);if(!changed)break;if(loops.empty()&&!old.empty()&&MessageBoxW(w,L"Disable all loops for this sample? Undo restores the previous settings.",L"Disable sample loops",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;s->doc()=std::move(next);break;}
        case InheritSample:{auto next=s->doc();changed=next.inherit_wave_sample(selected(w,Instrument),selected(w,Region));if(!changed)break;if(MessageBoxW(w,L"Use the Wave sample settings for this Region?\nThis removes its root note, tuning, volume and loop override. Undo restores it.",L"Use Wave sample settings",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;s->doc()=std::move(next);break;}
        case Scale:changed=s->doc().scale_wave(selected(w,Wave),number(w,Percent,400));break;
        case DuplicateWave:changed=s->doc().duplicate_wave(selected(w,Wave));break;
        case AddPcm:{const auto path=pcm_path(w,false);if(path.empty())return 0;const auto wide=std::filesystem::path(path).stem().wstring();if(wide.empty()||std::any_of(wide.begin(),wide.end(),[](wchar_t c){return c<32||c>126;}))throw std::runtime_error("New Wave requires an ASCII file name");changed=s->doc().add_wave_pcm(read_file(path),std::string(wide.begin(),wide.end()));if(changed){const auto count=s->doc().waves().size();populate(w,Wave,count,L"Wave");SendMessageW(GetDlgItem(w,Wave),CB_SETCURSEL,count-1,0);}break;}
        case ImportPcm:{const auto path=pcm_path(w,false);if(path.empty())return 0;changed=s->doc().import_wave_pcm(selected(w,Wave),read_file(path));break;}
        case ExportPcm:{const auto bytes=s->doc().export_wave_pcm(selected(w,Wave));const auto path=pcm_path(w,true);if(path.empty())return 0;write_file_atomic(path,bytes);break;}
        case Save:{auto path=s->framework.collections().at(s->index).path;if(path.empty())path=dls_save_path(w);if(path.empty())return 0;s->framework.save_collection(s->index,path);break;}
        case RuntimeSettings:show_runtime_settings(w,s->framework,RuntimeDocumentKind::Collection,s->index);break;
        case RuntimeSave:{const auto path=dls_save_path(w,true);if(path.empty())return 0;s->framework.save_runtime_as(RuntimeDocumentKind::Collection,s->index,path);break;}
        case SaveAs:{const auto path=dls_save_path(w);if(path.empty())return 0;s->framework.save_collection(s->index,path);s->savedAs=true;break;}
        case Undo:s->doc().undo();break;case Redo:s->doc().redo();break;default:return 0;}
        if(!changed)throw std::runtime_error("Edit unchanged or invalid; check ranges, unique MIDI locale and wave cue");refresh(w,*s);return 0;}
    case WM_CLOSE:DestroyWindow(w);return 0;
    }}catch(const std::exception& e){const std::string detail=e.what();const std::wstring wide(detail.begin(),detail.end());MessageBoxW(w,wide.c_str(),L"DLS editor",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}return DefWindowProcW(w,message,wp,lp);
}
}
namespace {
struct Picker {std::vector<DlsInstrument> instruments;std::optional<size_t> result;};
LRESULT CALLBACK picker_proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
    auto s=reinterpret_cast<Picker*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    try{switch(message){
    case WM_CREATE:{s=static_cast<Picker*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
        add(w,L"STATIC",L"Choose the DLS instrument for this Band PChannel",0,16,16,520,24);
        const auto list=add(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,16,52,520,300,1000);
        for(size_t i=0;i<s->instruments.size();++i){const auto& ins=s->instruments[i];const auto label=L"Instrument "+std::to_wstring(i+1)+L" — Bank "+std::to_wstring(ins.bank)+L", Program "+std::to_wstring(ins.program)+L", Regions "+std::to_wstring(ins.regions.size());if(SendMessageW(list,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()))==CB_ERR)throw std::runtime_error("Instrument list failed");}
        SendMessageW(list,CB_SETCURSEL,0,0);add(w,L"STATIC",L"The collection reference and MIDI locale change together.\nUndo restores the previous Band assignment.",0,16,92,520,46);
        add(w,L"BUTTON",L"Assign Instrument",WS_TABSTOP|BS_DEFPUSHBUTTON,16,154,245,32,IDOK);add(w,L"BUTTON",L"Cancel",WS_TABSTOP,290,154,245,32,IDCANCEL);return 0;}
    case WM_COMMAND:if(LOWORD(wp)==IDOK&&s){const auto i=selected(w,1000);if(i>=s->instruments.size())throw std::runtime_error("Invalid DLS selection");s->result=i;DestroyWindow(w);return 0;}if(LOWORD(wp)==IDCANCEL){DestroyWindow(w);return 0;}break;
    case WM_CLOSE:DestroyWindow(w);return 0;
    }}catch(const std::exception& e){const std::string detail=e.what();MessageBoxW(w,std::wstring(detail.begin(),detail.end()).c_str(),L"DLS instrument",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;}return DefWindowProcW(w,message,wp,lp);
}
}
std::optional<size_t> choose_dls_instrument(HWND parent,const DlsDocument& document){
    Picker session{document.instruments(),{}};if(session.instruments.empty())throw std::runtime_error("DLS collection has no instruments");
    static bool registered=false;const auto instance=GetModuleHandleW(nullptr);if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=picker_proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerDlsPicker";if(!RegisterClassW(&c))throw std::runtime_error("DLS picker registration failed");registered=true;}
    const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerDlsPicker",L"Assign DLS Instrument",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,570,240,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("DLS picker creation failed");
    const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;
    while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("DLS picker message loop failed");return session.result;
}
void show_dls_editor(HWND parent,Framework& framework,size_t index){
    // Validate typed views before creating controls; lossless unsupported DLS
    // ownership can remain in the project without presenting a false editor.
    (void)framework.collection_document(index).instruments();(void)framework.collection_document(index).waves();
    static bool registered=false;const auto instance=GetModuleHandleW(nullptr);if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerDls";if(!RegisterClassW(&c))throw std::runtime_error("DLS window registration failed");registered=true;}
    // The editor is an independently targetable task window. The synchronous loop
    // and disabled main window still keep Framework document indices stable.
    Session session{framework,index};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceProducerDls",L"DLS",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1100,700,nullptr,nullptr,instance,&session);if(!w)throw std::runtime_error("DLS editor window creation failed");const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);
    MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0){if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));if(result<0)throw std::runtime_error("DLS editor message loop failed");
}
}
