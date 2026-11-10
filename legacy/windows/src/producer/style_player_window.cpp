#include "style_player_window.h"
#include "style_player_session.h"
#include <commdlg.h>
#include <algorithm>
#include <stdexcept>
namespace producer::app {namespace {
enum:UINT {Style=10,Map,Shape,Band,Intro,End,Measures,Activity,Apply,Compose,Play,Stop,Adopt,Status,Motif1=30};
const wchar_t* shapes[]={L"Falling",L"Level",L"Loopable",L"Loud",L"Quiet",L"Peaking",L"Random",L"Rising",L"Song"};
struct WindowSession {Framework& host;Conductor& conductor;size_t style;std::unique_ptr<StylePlayerSession> player;std::optional<size_t> adopted;};
HWND control(HWND w,const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int width,int height,UINT id=0){auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|flags,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);if(!c)throw std::runtime_error("StylePlayer control creation failed");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;}
int row(HWND w,UINT id){const auto r=SendDlgItemMessageW(w,id,CB_GETCURSEL,0,0);if(r==CB_ERR)throw std::runtime_error("Select a StylePlayer value");return static_cast<int>(r);}
WORD number(HWND w,UINT id,WORD low,WORD high){wchar_t text[32]{};GetDlgItemTextW(w,id,text,32);const std::wstring s=text;if(s.empty()||s.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Enter a whole number");const auto n=std::stoul(s);if(n<low||n>high)throw std::runtime_error("StylePlayer number out of range");return static_cast<WORD>(n);}
std::optional<Bytes> map(HWND w,WindowSession& s){const auto r=row(w,Map);return r?std::optional<Bytes>(s.host.chordmap_document(r-1).save_bytes()):std::nullopt;}
std::vector<ChordMapCatalogEntry> maps(WindowSession& s){std::vector<ChordMapCatalogEntry> result;for(const auto& owned:s.host.chordmaps())result.push_back({owned.path,owned.document->save_bytes()});return result;}
void status(HWND w,WindowSession& s){const auto& p=*s.player;const auto text=std::wstring(p.playing()?L"Playing":L"Stopped")+L"; compositions "+std::to_wstring(p.compositions())+L"; restarts "+std::to_wstring(p.restarts())+L"; band changes "+std::to_wstring(p.band_changes())+L"; motifs "+std::to_wstring(p.motif_requests())+L"; primary "+std::to_wstring(p.primary_id());SetDlgItemTextW(w,Status,text.c_str());EnableWindow(GetDlgItem(w,Adopt),p.composition()!=nullptr);}
void lists(HWND w,WindowSession& s){SendDlgItemMessageW(w,Band,CB_RESETCONTENT,0,0);for(const auto& b:s.player->band_names())SendDlgItemMessageW(w,Band,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(b.c_str()));const auto names=s.player->band_names();const auto selected=std::find(names.begin(),names.end(),s.player->band());SendDlgItemMessageW(w,Band,CB_SETCURSEL,selected==names.end()?0:static_cast<WPARAM>(selected-names.begin()),0);const auto motifs=s.player->motif_names();for(size_t i=0;i<4;++i){SetDlgItemTextW(w,Motif1+static_cast<UINT>(i),i<motifs.size()?motifs[i].c_str():L"No Motif");EnableWindow(GetDlgItem(w,Motif1+static_cast<UINT>(i)),i<motifs.size());}}
LRESULT CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<WindowSession*>(GetWindowLongPtrW(w,GWLP_USERDATA));try{
 if(msg==WM_CREATE){s=static_cast<WindowSession*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));
  for(const auto& field:std::vector<std::pair<const wchar_t*,UINT>>{{L"Style",Style},{L"ChordMap",Map},{L"Shape",Shape},{L"Band",Band}}){const int y=16+static_cast<int>(field.second-Style)*46;control(w,L"STATIC",field.first,0,16,y,110,24);control(w,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,130,y,430,220,field.second);}
  for(const auto& d:s->host.style_documents()){const auto name=d.document->name().empty()?std::filesystem::path(d.path).filename().wstring():d.document->name();SendDlgItemMessageW(w,Style,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));}SendDlgItemMessageW(w,Style,CB_SETCURSEL,s->style,0);
  SendDlgItemMessageW(w,Map,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Style default ChordMap"));for(const auto& d:s->host.chordmaps()){const auto name=d.document->name();SendDlgItemMessageW(w,Map,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str()));}SendDlgItemMessageW(w,Map,CB_SETCURSEL,s->host.chordmaps().empty()?0:1,0);
  for(const auto name:shapes)SendDlgItemMessageW(w,Shape,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));SendDlgItemMessageW(w,Shape,CB_SETCURSEL,7,0);
  control(w,L"BUTTON",L"Intro",BS_AUTOCHECKBOX|WS_TABSTOP,16,208,100,28,Intro);control(w,L"BUTTON",L"End",BS_AUTOCHECKBOX|WS_TABSTOP,124,208,100,28,End);
  control(w,L"STATIC",L"Measures",0,250,210,80,24);control(w,L"EDIT",L"8",WS_BORDER|WS_TABSTOP,330,208,64,28,Measures);control(w,L"STATIC",L"Activity 0–3",0,410,210,90,24);control(w,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,510,208,50,28,Activity);
  control(w,L"BUTTON",L"Apply length / activity",WS_TABSTOP,16,254,200,32,Apply);control(w,L"BUTTON",L"Re-Compose",WS_TABSTOP,232,254,140,32,Compose);control(w,L"BUTTON",L"Play",WS_TABSTOP,388,254,78,32,Play);control(w,L"BUTTON",L"Stop",WS_TABSTOP,482,254,78,32,Stop);
  for(UINT i=0;i<4;++i)control(w,L"BUTTON",L"No Motif",WS_TABSTOP,16+static_cast<int>(i)*140,306,132,36,Motif1+i);
  control(w,L"BUTTON",L"Save composed Segment...",WS_TABSTOP,16,366,270,34,Adopt);control(w,L"STATIC",L"",0,16,424,544,66,Status);
  s->player=std::make_unique<StylePlayerSession>(s->conductor,w,s->host.style_playback_snapshot(s->style),s->host.style_playback_collections(s->style),map(w,*s),StylePlayerSettings{},maps(*s));lists(w,*s);s->player->play();lists(w,*s);status(w,*s);SetTimer(w,1,250,nullptr);return 0;
 }
 if(msg==WM_TIMER){status(w,*s);return 0;}
 if(msg==WM_COMMAND){const auto id=LOWORD(wp);const auto code=HIWORD(wp);auto& p=*s->player;
  if(code==CBN_SELCHANGE){if(id==Style){const auto i=row(w,Style);p.set_style(s->host.style_playback_snapshot(i),s->host.style_playback_collections(i),map(w,*s),maps(*s));s->style=i;lists(w,*s);}else if(id==Map)p.set_chordmap(map(w,*s));else if(id==Band){const auto names=p.band_names();p.select_band(names.at(row(w,Band)));}else if(id==Shape){auto settings=p.settings();settings.shape=static_cast<StyleShape>(row(w,Shape));p.set_settings(settings);}status(w,*s);return 0;}
  if(code!=BN_CLICKED)return 0;
  if(id==Intro||id==End||id==Apply){auto settings=p.settings();settings.intro=SendDlgItemMessageW(w,Intro,BM_GETCHECK,0,0)==BST_CHECKED;settings.end=SendDlgItemMessageW(w,End,BM_GETCHECK,0,0)==BST_CHECKED;settings.measures=number(w,Measures,1,65535);settings.activity=number(w,Activity,0,3);p.set_settings(settings);}
  else if(id==Compose)p.recompose();else if(id==Play)p.play();else if(id==Stop)p.stop();else if(id>=Motif1&&id<Motif1+4)p.play_motif(p.motif_names().at(id-Motif1));
  else if(id==Adopt){if(!p.composition())throw std::runtime_error("Compose a Segment first");wchar_t path[32768]{};OPENFILENAMEW f{};f.lStructSize=sizeof(f);f.hwndOwner=w;f.lpstrFilter=L"Native Segment\0*.sgp\0\0";f.lpstrFile=path;f.nMaxFile=32768;f.lpstrDefExt=L"sgp";f.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;if(!GetSaveFileNameW(&f))return 0;
   for(const auto& dependency:p.composition()->maps)if(!dependency.path.empty()){
    bool owned=false;for(const auto& entry:s->host.chordmaps())if(entry.document->has_object_id()&&entry.document->object_id()==dependency.reference.objectId){if(entry.document->save_bytes()!=dependency.bytes)throw std::runtime_error("Composed default ChordMap differs from current owned document");owned=true;}
    if(!owned){if(read_file(dependency.path)!=dependency.bytes)throw std::runtime_error("Composed default ChordMap source changed before save");const auto i=s->host.open_chordmap(dependency.path);if(s->host.chordmap_document(i).save_bytes()!=dependency.bytes)throw std::runtime_error("Default ChordMap adoption changed source bytes");}
   }
   const auto index=s->host.adopt_composed_segment(p.composition()->segment);s->host.save_segment(index,path);s->adopted=index;p.stop();DestroyWindow(w);return 0;}
  status(w,*s);return 0;
 }
 if(msg==WM_CLOSE){if(s->player)s->player->stop();DestroyWindow(w);return 0;}
 if(msg==WM_DESTROY){KillTimer(w,1);return 0;}
 }catch(const std::exception& e){MessageBoxA(w,e.what(),"StylePlayer",MB_OK|MB_ICONERROR);if(msg==WM_CREATE)return -1;}return DefWindowProcW(w,msg,wp,lp);}
}
std::optional<size_t> show_style_player(HWND parent,Framework& host,Conductor& conductor,std::optional<size_t> initialStyle){
 if(host.style_documents().empty())throw std::runtime_error("Open a Style before starting StylePlayer");
 const auto instance=GetModuleHandleW(nullptr);static bool registered=false;if(!registered){WNDCLASSW c{};c.hInstance=instance;c.lpfnWndProc=proc;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceStylePlayer";if(!RegisterClassW(&c))throw std::runtime_error("StylePlayer window registration failed");registered=true;}
 size_t selected=initialStyle.value_or(0);if(!initialStyle){GUID random{};if(FAILED(CoCreateGuid(&random)))throw std::runtime_error("Style selection random seed unavailable");selected=random.Data1%host.style_documents().size();}
 WindowSession s{host,conductor,selected};const auto w=CreateWindowExW(WS_EX_APPWINDOW,L"SourceStylePlayer",L"StylePlayer",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,600,540,parent,nullptr,instance,&s);if(!w)throw std::runtime_error("StylePlayer window creation failed");
 const bool enabled=IsWindowEnabled(parent)!=FALSE;EnableWindow(parent,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);MSG msg{};BOOL result=1;while(IsWindow(w)&&(result=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}if(s.player)s.player->stop();if(IsWindow(w))DestroyWindow(w);EnableWindow(parent,enabled);SetActiveWindow(parent);if(!result)PostQuitMessage(static_cast<int>(msg.wParam));if(result==-1)throw std::runtime_error("StylePlayer message loop failed");return s.adopted;
}
}
