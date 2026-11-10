#include "performance_options_editor.h"
#include <commctrl.h>
#include <array>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace producer::app {
namespace {
enum { Voices=10,Latency=11,Persist=12,DefaultLatency=13,Synth=14,Input=15,EchoDevice=16,EchoOutput=17,EchoSpin=18,EchoEnabled=19,RateFirst=20,OtherRate=26 };
constexpr std::array<DWORD,5> rates{11025,22050,44100,48000,96000};
struct Session {PerformanceSettings settings;const std::vector<PerformanceSynthesizer>& synths;MidiInputSelection input;const std::vector<MidiInputDevice>& devices;MidiEchoSettings echo;std::optional<PerformanceOptionsSelection> accepted;};
HWND control(HWND owner,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    RECT bounds{x,y,x+w,y+h};MapDialogRect(owner,&bounds);
    auto child=CreateWindowExW(cls==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,owner,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!child)throw std::runtime_error("Performance Options control creation failed");SendMessageW(child,WM_SETFONT,SendMessageW(owner,WM_GETFONT,0,0),TRUE);return child;
}
void disabled(HWND owner,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h){auto child=control(owner,cls,text,style,x,y,w,h);if(std::wstring(cls)==L"COMBOBOX"){SendMessageW(child,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));SendMessageW(child,CB_SETCURSEL,0,0);}EnableWindow(child,FALSE);}
void spin(HWND owner,HWND buddy,DWORD maximum){
    auto up=CreateWindowExW(0,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS,0,0,0,0,owner,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!up)throw std::runtime_error("Performance Options spin creation failed");SendMessageW(up,UDM_SETBUDDY,reinterpret_cast<WPARAM>(buddy),0);SendMessageW(up,UDM_SETRANGE32,0,maximum);
}
DWORD number(HWND owner,int id){
    wchar_t text[64]{};GetDlgItemTextW(owner,id,text,std::size(text));if(!*text)throw std::runtime_error("Enter a numeric value");
    unsigned long long n=0;for(auto p=text;*p;++p){if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter a numeric value");n=n*10+(*p-L'0');if(n>std::numeric_limits<DWORD>::max())throw std::runtime_error("Numeric value is too large");}return static_cast<DWORD>(n);
}
std::wstring echo_range(DWORD first){return std::to_wstring(static_cast<unsigned long long>(first)+1)+L" - "+std::to_wstring(static_cast<unsigned long long>(first)+16);}
DWORD echo_first(HWND window){
    wchar_t text[80]{};GetDlgItemTextW(window,EchoOutput,text,std::size(text));const wchar_t* p=text;
    const auto number=[&](){while(*p==L' ')++p;if(*p<L'0'||*p>L'9')throw std::runtime_error("Enter an Output PChannels range such as 17 - 32");unsigned long long value=0;while(*p>=L'0'&&*p<=L'9'){value=value*10+(*p++-L'0');if(value>0xffffffffull)throw std::runtime_error("Output PChannel range is too large");}while(*p==L' ')++p;return value;};
    const auto first=number();if(*p++!=L'-')throw std::runtime_error("Enter a 16-PChannel range");const auto last=number();if(*p||!first||last!=first+15||((first-1)%16)||first-1>0xffffffe0u)throw std::runtime_error("Output PChannels must be an aligned block of 16 non-reserved channels");return static_cast<DWORD>(first-1);
}
void populate_input(HWND combo,const MidiInputSelection& current,const std::vector<MidiInputDevice>& devices){
    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"None"));int selected=0;for(size_t i=0;i<devices.size();++i){SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(devices[i].key.name.c_str()));if(current&&*current==devices[i].key)selected=static_cast<int>(i+1);}if(current&&!selected){const auto missing=L"Unavailable: "+current->name;SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(missing.c_str()));selected=static_cast<int>(devices.size()+1);}SendMessageW(combo,CB_SETCURSEL,selected,0);
}
MidiInputSelection selected_input(HWND window,int id,const MidiInputSelection& previous,const std::vector<MidiInputDevice>& devices){
    const auto selected=SendDlgItemMessageW(window,id,CB_GETCURSEL,0,0);if(selected==0)return {};if(selected>0&&static_cast<size_t>(selected)<=devices.size())return devices[static_cast<size_t>(selected-1)].key;if(selected>0&&static_cast<size_t>(selected)==devices.size()+1&&previous)return previous;throw std::runtime_error("Select a MIDI input device");
}
INT_PTR CALLBACK dialog_proc(HWND window,UINT message,WPARAM w,LPARAM l){
    auto session=reinterpret_cast<Session*>(GetWindowLongPtrW(window,DWLP_USER));
    if(message==WM_INITDIALOG){session=reinterpret_cast<Session*>(l);SetWindowLongPtrW(window,DWLP_USER,l);
        try {
            control(window,L"BUTTON",L"MIDI Input",BS_GROUPBOX,7,5,170,35);
            auto input=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,13,19,158,100,Input);
            SendMessageW(input,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"None"));int selectedInput=0;
            for(size_t i=0;i<session->devices.size();++i){const auto& device=session->devices[i];SendMessageW(input,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(device.key.name.c_str()));if(session->input&&*session->input==device.key)selectedInput=static_cast<int>(i+1);}
            if(session->input&&!selectedInput){const auto missing=L"Unavailable: "+session->input->name;SendMessageW(input,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(missing.c_str()));selectedInput=static_cast<int>(session->devices.size()+1);}
            SendMessageW(input,CB_SETCURSEL,selectedInput,0);
            control(window,L"BUTTON",L"Echo MIDI Input",BS_GROUPBOX,7,49,170,81);
            control(window,L"STATIC",L"Echo MIDI Input Device:",0,13,61,155,12);
            auto echo=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,13,74,158,100,EchoDevice);populate_input(echo,session->echo.device,session->devices);
            control(window,L"BUTTON",L"Echo MIDI In",BS_AUTOCHECKBOX|WS_TABSTOP,13,92,75,12,EchoEnabled);CheckDlgButton(window,EchoEnabled,session->echo.enabled?BST_CHECKED:BST_UNCHECKED);
            control(window,L"STATIC",L"Output PChannels:",0,88,92,83,12);
            disabled(window,L"BUTTON",L"Advanced",BS_PUSHBUTTON,13,105,65,16);
            auto output=control(window,L"EDIT",echo_range(session->echo.firstPchannel).c_str(),ES_AUTOHSCROLL|WS_TABSTOP,88,105,83,16,EchoOutput);
            auto outputSpin=CreateWindowExW(0,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|UDS_ALIGNRIGHT|UDS_ARROWKEYS,0,0,0,0,window,reinterpret_cast<HMENU>(EchoSpin),GetModuleHandleW(nullptr),nullptr);if(!outputSpin)throw std::runtime_error("MIDI Echo range spin creation failed");SendMessageW(outputSpin,UDM_SETBUDDY,reinterpret_cast<WPARAM>(output),0);SendMessageW(outputSpin,UDM_SETRANGE32,0,0xffffffe0u/16);SendMessageW(outputSpin,UDM_SETPOS32,0,session->echo.firstPchannel/16);
            control(window,L"BUTTON",L"Adjust Latency",BS_GROUPBOX,7,143,170,43);
            auto latency=control(window,L"EDIT",std::to_wstring(session->settings.latencyMs).c_str(),ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,13,155,43,13,Latency);spin(window,latency,9999);
            control(window,L"STATIC",L"ms",0,59,156,18,12);control(window,L"BUTTON",L"Set to Default",BS_PUSHBUTTON|WS_TABSTOP,84,154,78,15,DefaultLatency);
            control(window,L"BUTTON",L"Persist to all Audiopaths",BS_AUTOCHECKBOX|WS_TABSTOP,13,172,158,12,Persist);CheckDlgButton(window,Persist,session->settings.persistLatency?BST_CHECKED:BST_UNCHECKED);
            control(window,L"BUTTON",L"Audio Initialization Settings",BS_GROUPBOX,187,5,186,125);
            control(window,L"STATIC",L"Number of Voices:",0,194,19,85,12);
            auto voices=control(window,L"EDIT",std::to_wstring(session->settings.voices).c_str(),ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,282,16,35,13,Voices);spin(window,voices,0x7fffffff);
            control(window,L"STATIC",L"Sample Rate:",0,194,37,63,12);
            const std::array<const wchar_t*,5> names{L"11 kHz",L"22 kHz",L"44 kHz",L"48 kHz",L"96 kHz"};int selected=5;
            for(size_t i=0;i<rates.size();++i){control(window,L"BUTTON",names[i],BS_AUTORADIOBUTTON|WS_TABSTOP|(i==0?WS_GROUP:0),260,37+static_cast<int>(i)*11,50,12,RateFirst+static_cast<int>(i));if(session->settings.sampleRate==rates[i])selected=static_cast<int>(i);}
            control(window,L"BUTTON",L"Other",BS_AUTORADIOBUTTON|WS_TABSTOP,314,37,47,12,RateFirst+5);
            control(window,L"EDIT",std::to_wstring(session->settings.sampleRate).c_str(),ES_NUMBER|ES_AUTOHSCROLL|WS_TABSTOP,314,48,35,13,OtherRate);control(window,L"STATIC",L"Hz",0,351,49,20,12);CheckRadioButton(window,RateFirst,RateFirst+5,RateFirst+selected);EnableWindow(GetDlgItem(window,OtherRate),selected==5);
            control(window,L"STATIC",L"Default Synthesizer:",0,194,100,115,12);
            auto synth=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,194,113,173,100,Synth);
            SendMessageW(synth,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Not Specified (Use DirectMusic Default)"));int selectedSynth=0;
            for(size_t i=0;i<session->synths.size();++i){SendMessageW(synth,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(session->synths[i].name.c_str()));if(IsEqualGUID(session->settings.defaultSynth,session->synths[i].id))selectedSynth=static_cast<int>(i+1);}
            SendMessageW(synth,CB_SETCURSEL,selectedSynth,0);
            control(window,L"BUTTON",L"Download Status",BS_GROUPBOX,187,143,186,43);
            disabled(window,L"STATIC",L"Download",0,268,151,50,12);disabled(window,L"STATIC",L"Unload",0,315,151,50,12);
            disabled(window,L"STATIC",L"GM Set",0,194,163,65,12);disabled(window,L"BUTTON",L"",BS_AUTORADIOBUTTON,280,163,12,12);disabled(window,L"BUTTON",L"",BS_AUTORADIOBUTTON,322,163,12,12);
            disabled(window,L"STATIC",L"Custom DLS Sets",0,194,174,82,12);disabled(window,L"BUTTON",L"",BS_AUTORADIOBUTTON,280,174,12,12);disabled(window,L"BUTTON",L"",BS_AUTORADIOBUTTON,322,174,12,12);
            control(window,L"BUTTON",L"OK",BS_DEFPUSHBUTTON|WS_TABSTOP,244,197,61,15,IDOK);control(window,L"BUTTON",L"Cancel",BS_PUSHBUTTON|WS_TABSTOP,312,197,61,15,IDCANCEL);
            return TRUE;
        }catch(const std::exception& e){MessageBoxA(window,e.what(),"MIDI/Performance Options",MB_OK|MB_ICONERROR);EndDialog(window,IDCANCEL);return TRUE;}
    }
    if(message==WM_NOTIFY&&session){const auto notice=reinterpret_cast<NMHDR*>(l);if(notice->idFrom==EchoSpin&&notice->code==UDN_DELTAPOS){try{const auto delta=reinterpret_cast<NMUPDOWN*>(l);const auto next=std::clamp(static_cast<long long>(echo_first(window))+static_cast<long long>(delta->iDelta)*16,0ll,static_cast<long long>(0xffffffe0u));SetDlgItemTextW(window,EchoOutput,echo_range(static_cast<DWORD>(next)).c_str());}catch(const std::exception& e){MessageBoxA(window,e.what(),"MIDI/Performance Options",MB_OK|MB_ICONINFORMATION);}SetWindowLongPtrW(window,DWLP_MSGRESULT,TRUE);return TRUE;}}
    if(message==WM_COMMAND&&session){const int id=LOWORD(w);
        if(id==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}
        if(id==DefaultLatency){SetDlgItemTextW(window,Latency,L"55");return TRUE;}
        if(id>=RateFirst&&id<=RateFirst+5){EnableWindow(GetDlgItem(window,OtherRate),id==RateFirst+5);return TRUE;}
        if(id==IDOK){try{auto next=session->settings;next.voices=number(window,Voices);next.latencyMs=number(window,Latency);next.persistLatency=IsDlgButtonChecked(window,Persist)==BST_CHECKED;
                next.sampleRate=number(window,OtherRate);for(size_t i=0;i<rates.size();++i)if(IsDlgButtonChecked(window,RateFirst+static_cast<int>(i))==BST_CHECKED)next.sampleRate=rates[i];
                const auto selected=SendDlgItemMessageW(window,Synth,CB_GETCURSEL,0,0);if(selected<0||static_cast<size_t>(selected)>session->synths.size())throw std::runtime_error("Select a default synthesizer");next.defaultSynth=selected?session->synths[static_cast<size_t>(selected-1)].id:GUID_NULL;
                const auto input=SendDlgItemMessageW(window,Input,CB_GETCURSEL,0,0);MidiInputSelection nextInput;
                if(input<0)throw std::runtime_error("Select a MIDI input device");
                if(input>0&&static_cast<size_t>(input)<=session->devices.size())nextInput=session->devices[static_cast<size_t>(input-1)].key;
                else if(input>0&&static_cast<size_t>(input)==session->devices.size()+1&&session->input)nextInput=session->input;
                else if(input>0)throw std::runtime_error("MIDI input selection is unavailable");
                auto nextEcho=session->echo;nextEcho.device=selected_input(window,EchoDevice,session->echo.device,session->devices);nextEcho.enabled=IsDlgButtonChecked(window,EchoEnabled)==BST_CHECKED;nextEcho.firstPchannel=echo_first(window);validate_midi_echo(nextEcho);validate_performance_settings(next);session->accepted=PerformanceOptionsSelection{next,nextInput,nextEcho};EndDialog(window,IDOK);
            }catch(const std::exception& e){MessageBoxA(window,e.what(),"MIDI/Performance Options",MB_OK|MB_ICONINFORMATION);}return TRUE;}
    }return FALSE;
}
}
std::optional<PerformanceOptionsSelection> show_performance_options(HWND owner,const PerformanceSettings& current,const std::vector<PerformanceSynthesizer>& synths,const MidiInputSelection& input,const std::vector<MidiInputDevice>& devices,const MidiEchoSettings& echo){
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_UPDOWN_CLASS};if(!InitCommonControlsEx(&common))throw std::runtime_error("Performance Options spin initialization failed");
    std::array<DWORD,128> storage{};auto dialog=reinterpret_cast<DLGTEMPLATE*>(storage.data());dialog->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER|DS_SETFONT;dialog->cx=380;dialog->cy=218;
    auto text=reinterpret_cast<WORD*>(dialog+1);*text++=0;*text++=0;for(auto c:std::wstring(L"MIDI/Performance Options"))*text++=c;*text++=0;*text++=8;for(auto c:std::wstring(L"MS Shell Dlg"))*text++=c;*text=0;
    Session session{current,synths,input,devices,echo,{}};if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),dialog,owner,dialog_proc,reinterpret_cast<LPARAM>(&session))==-1)throw std::runtime_error("Performance Options dialog creation failed");return session.accepted;
}
}
