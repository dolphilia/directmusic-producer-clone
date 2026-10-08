#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include "framework.h"
#include "conductor.h"
#include "dls_editor.h"
#include "command_editor.h"
#include "chord_editor.h"
#include "signpost_editor.h"
#include "marker_editor.h"
#include "lyric_editor.h"
#include "mute_editor.h"
#include "param_control_editor.h"
#include "style_reference_editor.h"
#include "chordmap_reference_editor.h"
#include "segment_trigger_editor.h"
#include "script_track_editor.h"
#include "wave_editor.h"
#include "wave_document_editor.h"
#include "script_editor.h"
#include "tool_graph_editor.h"
#include "source_tools.h"
#include "container_editor.h"
#include "timeline_editor.h"
#include "audio_path_editor.h"
#include "chordmap_editor.h"
#include "runtime_settings_editor.h"
#include "runtime_recovery_editor.h"
#include "motif_editor.h"
#include "playback_window.h"
#include "playback_monitor.h"
#include "style_player_session.h"
#include "style_player_window.h"
#include "farm_player_window.h"
#include "envelope_parameters.h"
#include "compat/playback_runtime.h"
#include "compat/waves_reverb.h"
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <optional>
#include <cstring>
#include <map>
#include <tuple>

namespace {
using namespace producer::app;
enum : UINT { AudioPathEditor=1500,ChordMapEditor,StyleReferenceEditor,SegmentTriggerEditor,ScriptTrackEditor,ChordMapReferenceEditor,NewProject=100,NewSegment,Open,SaveSegment,SaveProject,Exit,NewPlaybackTest,NewStyle,NewBand,SaveDocumentAs,NewDls,Add=200,Change,Delete,Copy,Paste,Undo,Redo,Documents,Events,MeterSet=300,MeterDelete,Play=400,Stop,NoteAdd,GrooveBottom=500,GrooveTop,GrooveSet,PatternLayout=600,StylePartSelect=700,StyleNoteSelect,StyleNoteSet,StyleNoteInsert,StyleNoteDelete,StyleVariationSelect,StyleVariationSet };
Framework framework;
Conductor conductor;
HMENU transportMenu=nullptr;
constexpr UINT TransportPath=2000;
constexpr UINT StartFileOutput=3100,StopFileOutput=3101;
constexpr UINT ImportMidi=3102,MessageWindowCommand=3103;
constexpr UINT StylePlayerCommand=3104;
constexpr UINT FarmPlayerCommand=3105;
constexpr UINT WavesReverbCommand=3106;
HWND messageWindow=nullptr,messageText=nullptr;size_t lyricCursor=0,scriptMessageCursor=0,scriptDiagnosticCursor=0,scriptCallCursor=0;
static_assert(StartFileOutput>=TransportPath+1000 && StopFileOutput>=TransportPath+1000,
              "Recording commands must not overlap the dynamic AudioPath menu");
PlaybackMonitor playbackMonitor;
size_t active=0;
size_t activeStyle=0,activeBand=0;bool styleMode=false,bandMode=false;
HWND docs,events,timeEdit,bpmEdit,status;
HWND measureEdit,beatsEdit,denominatorEdit,gridsEdit;
constexpr UINT GroupApply=900,GroupMask=901,GroupTempo=902,GroupMeter=903,GroupSequence=904,GroupBand=905;
HWND groupLabel,groupEdit,groupTempoLabel,groupTempoEdit,groupMeterLabel,groupMeterEdit,groupSequenceLabel,groupSequenceEdit,groupBandLabel,groupBandEdit;
HWND pitchEdit,durationEdit,velocityEdit;
constexpr UINT CommandEditor=920;
constexpr UINT ChordEditor=921;
constexpr UINT SignpostEditor=922;
constexpr UINT MarkerEditor=923;
constexpr UINT LyricEditor=924;
constexpr UINT MuteEditor=925;
constexpr UINT WaveEditor=926;
constexpr UINT WaveDocuments=927;
constexpr UINT ScriptDocuments=928;
constexpr UINT ToolGraphDocuments=929;
constexpr UINT ContainerDocuments=931;
constexpr UINT TimelineRangeEditor=932;
constexpr UINT ParamControlEditor=930;
constexpr UINT PatternDuplicate=610,PatternUnshare=611,PatternNew=614,PatternDelete=615;
constexpr UINT MotifNew=616;
constexpr UINT MotifSettings=617;
constexpr UINT MotifPlay=618;
constexpr UINT MotifBandAssign=619;
constexpr UINT MotifBandEdit=620;
constexpr UINT StopCurrentPlayback=621;
constexpr UINT PlaybackSessions=622;
constexpr UINT PatternProperties=612;
constexpr UINT CopyProject=613;
constexpr UINT RuntimeSaveAll=623,RuntimeSaveAs=624,RuntimeSettings=625,RuntimeSaveDefaults=626,RuntimeRecovery=627;
HWND patternNameLabel,patternNameEdit,patternKindSelect;
const unsigned patternKinds[]={0,1,4,2,8};
constexpr UINT SequenceNoteSelect=910,SequenceNoteTime=911,SequenceNoteChannel=912,SequenceNoteChange=913,SequenceNoteDelete=914;
HWND sequenceNoteSelect,sequenceNoteTime,sequenceNoteChannel,sequenceNoteLabel,sequenceTimeLabel,sequenceChannelLabel;
using NoteContext=std::tuple<size_t,std::uint32_t,size_t>;
std::map<NoteContext,size_t> noteSelections;
std::map<size_t,CommandEditorContext> commandSelections;
std::map<size_t,CommandEditorContext> chordSelections;
std::map<size_t,CommandEditorContext> signpostSelections;
std::map<size_t,MarkerEditorContext> markerSelections;
std::map<size_t,WaveEditorContext> waveSelections;
std::map<size_t,LyricEditorContext> lyricSelections;
std::map<size_t,MuteEditorContext> muteSelections;
std::map<size_t,ParamControlEditorContext> paramSelections;
HWND grooveBottomEdit,grooveTopEdit,grooveLabel;
HWND patternBeats,patternDenominator,patternGrids,patternMeasures,patternLayoutLabel;
HWND stylePartSelect,styleNoteSelect,stylePartLabel,styleNoteInfo;
HWND bandSelect,bandLabel,bandPatch,bandChannel,bandPan,bandVolume;
std::vector<std::pair<size_t,size_t>> bandAssignments;
constexpr UINT BandSelect=800,BandSet=801,BandAdd=802;
constexpr UINT SegmentBandSelect=803,SegmentBandAssign=804;
HWND segmentBandSelect;
struct SegmentBandSource {bool style;size_t document,band;};
std::vector<SegmentBandSource> segmentBandSources;
constexpr UINT BandEventSelect=805,BandEventMove=806,BandEventDelete=807;
constexpr UINT BandCollectionAssign=808;
HWND bandEventSelect,bandLogical,bandPhysical,bandEventLabel;
HWND styleGrid,styleOffset,styleMusic,styleVariation,styleFieldsLabel,partVariationSelect,partVariationChoices,partVariationLabel;
std::vector<size_t> selectedPartIndexes;
std::wstring playbackStatus=L"Stopped";
UINT clipboardFormat,patternClipboardFormat;
std::wstring choose(HWND owner,bool save,const wchar_t* filter,const wchar_t* extension) {
    wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=owner;o.lpstrFilter=filter;o.lpstrFile=file;o.nMaxFile=32768;o.lpstrDefExt=extension;o.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    const BOOL ok=save?GetSaveFileNameW(&o):GetOpenFileNameW(&o);if(!ok&&CommDlgExtendedError())throw std::runtime_error("File dialog failed");return ok?file:L"";
}
SegmentDocument& document() {if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a segment first");return framework.document(active);}
std::wstring control_text(HWND control) {const int count=GetWindowTextLengthW(control);std::wstring text(static_cast<size_t>(count)+1,L'\0');GetWindowTextW(control,text.data(),count+1);text.resize(count);return text;}
std::int32_t clock_input() {const auto s=control_text(timeEdit);size_t end=0;const auto v=std::stoll(s,&end);if(end!=s.size()||v<0||v>INT32_MAX)throw std::runtime_error("Time must be a nonnegative 32-bit clock value");return static_cast<std::int32_t>(v);}
double bpm_input() {const auto s=control_text(bpmEdit);size_t end=0;const auto v=std::stod(s,&end);if(end!=s.size())throw std::runtime_error("Invalid tempo value");return v;}
unsigned meter_input(HWND control,bool zero=false) {const auto s=control_text(control);size_t end=0;const auto v=std::stoll(s,&end);if(end!=s.size()||v<(zero?0:1)||v>INT32_MAX)throw std::runtime_error("Invalid integer field");return static_cast<unsigned>(v);}
std::int64_t integer_input(HWND control,std::int64_t low,std::int64_t high){const auto s=control_text(control);size_t end=0;const auto v=std::stoll(s,&end);if(end!=s.size()||v<low||v>high)throw std::runtime_error("Note field out of range");return v;}
StyleNoteEdit style_note_input(){return {static_cast<std::int32_t>(integer_input(styleGrid,INT32_MIN,INT32_MAX)),static_cast<std::int32_t>(meter_input(durationEdit)),static_cast<std::int16_t>(integer_input(styleOffset,INT16_MIN,INT16_MAX)),static_cast<std::uint32_t>(integer_input(styleVariation,0,UINT32_MAX)),static_cast<std::uint16_t>(integer_input(styleMusic,0,65535)),meter_input(velocityEdit)};}
std::wstring group_text(std::uint32_t mask){std::wstring text;for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t(1)<<i)){if(!text.empty())text+=L",";text+=std::to_wstring(i+1);}return text;}
std::uint32_t group_input(){const auto text=control_text(groupEdit);std::uint32_t mask=0;size_t at=0;while(at<text.size()){const auto comma=text.find(L',',at);const auto token=text.substr(at,comma==std::wstring::npos?std::wstring::npos:comma-at);size_t end=0;const auto value=std::stoul(token,&end);while(end<token.size()&&iswspace(token[end]))++end;if(end!=token.size()||value<1||value>32)throw std::runtime_error("Track groups are numbers 1 to 32 separated by commas");mask|=std::uint32_t(1)<<(value-1);if(comma==std::wstring::npos)break;at=comma+1;if(at==text.size())throw std::runtime_error("Missing track group after comma");}if(!mask)throw std::runtime_error("Choose at least one track group");return mask;}
void refresh_group_fields(HWND window,bool meterFields=false){const bool segment=!styleMode&&!bandMode&&!framework.documents().empty();for(const auto field:{groupLabel,groupEdit,groupTempoLabel,groupTempoEdit,groupMeterLabel,groupMeterEdit,groupSequenceLabel,groupSequenceEdit,groupBandLabel,groupBandEdit,GetDlgItem(window,GroupApply)}){ShowWindow(field,segment?SW_SHOW:SW_HIDE);EnableWindow(field,segment);}if(!segment)return;const auto& d=document();SetWindowTextW(groupEdit,group_text(d.selected_groups()).c_str());SetWindowTextW(groupTempoEdit,std::to_wstring(d.selected_tempo_index()+1).c_str());SetWindowTextW(groupMeterEdit,std::to_wstring(d.selected_meter_index()+1).c_str());SetWindowTextW(groupSequenceEdit,std::to_wstring(d.selected_sequence_index()+1).c_str());SetWindowTextW(groupBandEdit,std::to_wstring(d.selected_band_index()+1).c_str());if(meterFields){const auto view=d.timeline();const auto& m=view.meters().front();SetWindowTextW(measureEdit,L"1");SetWindowTextW(beatsEdit,std::to_wstring(m.beats).c_str());SetWindowTextW(denominatorEdit,std::to_wstring(m.denominator).c_str());SetWindowTextW(gridsEdit,std::to_wstring(m.grids).c_str());}}
void refresh_band_fields(HWND window){
    const auto index=SendMessageW(bandSelect,CB_GETCURSEL,0,0);const bool selected=(styleMode||bandMode)&&index>=0&&static_cast<size_t>(index)<bandAssignments.size();
    for(const auto field:{bandPatch,bandChannel,bandPan,bandVolume})EnableWindow(field,selected||bandMode||styleMode);EnableWindow(GetDlgItem(window,BandSet),selected);EnableWindow(GetDlgItem(window,BandAdd),bandMode||styleMode);EnableWindow(GetDlgItem(window,BandCollectionAssign),selected&&(bandMode||styleMode));
    if(selected){const auto [bi,ii]=bandAssignments[index];const auto instrument=bandMode?framework.band_document(activeBand).instruments().at(ii):framework.style_document(activeStyle).bands().at(bi).instruments().at(ii);SetWindowTextW(bandPatch,std::to_wstring(instrument.patch).c_str());SetWindowTextW(bandChannel,std::to_wstring(instrument.pchannel).c_str());SetWindowTextW(bandPan,std::to_wstring(instrument.pan).c_str());SetWindowTextW(bandVolume,std::to_wstring(instrument.volume).c_str());}
    else if(bandMode){SetWindowTextW(bandPatch,L"0");SetWindowTextW(bandChannel,L"0");SetWindowTextW(bandPan,L"64");SetWindowTextW(bandVolume,L"100");}
}
void refresh_bands(HWND window){
    const bool segment=!styleMode&&!bandMode;ShowWindow(segmentBandSelect,segment?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(window,SegmentBandAssign),segment?SW_SHOW:SW_HIDE);
    const auto previous=SendMessageW(segmentBandSelect,CB_GETCURSEL,0,0);SendMessageW(segmentBandSelect,CB_RESETCONTENT,0,0);segmentBandSources.clear();
    for(size_t i=0;i<framework.band_documents().size();++i){const auto& b=framework.band_documents()[i];const auto label=b.path.empty()?std::wstring(L"Untitled Band"):std::filesystem::path(b.path).filename().wstring();SendMessageW(segmentBandSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));segmentBandSources.push_back({false,i,0});}
    for(size_t i=0;i<framework.style_documents().size();++i){const auto& s=framework.style_documents()[i];const auto bands=s.document->bands();for(size_t j=0;j<bands.size();++j){const auto label=(s.path.empty()?std::wstring(L"Untitled Style"):std::filesystem::path(s.path).filename().wstring())+L" / Band "+std::to_wstring(j+1);SendMessageW(segmentBandSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));segmentBandSources.push_back({true,i,j});}}
    if(!segmentBandSources.empty())SendMessageW(segmentBandSelect,CB_SETCURSEL,previous>=0&&static_cast<size_t>(previous)<segmentBandSources.size()?previous:0,0);EnableWindow(GetDlgItem(window,SegmentBandAssign),segment&&!segmentBandSources.empty());
    for(const auto c:{bandEventSelect,bandLogical,bandPhysical,bandEventLabel,GetDlgItem(window,BandEventMove),GetDlgItem(window,BandEventDelete)})ShowWindow(c,segment?SW_SHOW:SW_HIDE);
    const auto selected=SendMessageW(bandEventSelect,CB_GETCURSEL,0,0);SendMessageW(bandEventSelect,CB_RESETCONTENT,0,0);std::vector<BandEvent> changes;
    if(segment&&!framework.documents().empty()){try{changes=document().band_events();}catch(const std::exception&){SetWindowTextW(bandEventLabel,L"BandTrack selection unavailable: ambiguous tracks or groups");}}
    for(const auto& e:changes){const auto label=std::to_wstring(e.logicalTime)+L" / "+std::to_wstring(e.physicalTime);SendMessageW(bandEventSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    if(!changes.empty()){const auto at=selected>=0&&static_cast<size_t>(selected)<changes.size()?static_cast<size_t>(selected):0;SendMessageW(bandEventSelect,CB_SETCURSEL,at,0);SetWindowTextW(bandLogical,std::to_wstring(changes[at].logicalTime).c_str());SetWindowTextW(bandPhysical,std::to_wstring(changes[at].physicalTime).c_str());}
    for(const auto c:{bandEventSelect,bandLogical,bandPhysical,GetDlgItem(window,BandEventMove),GetDlgItem(window,BandEventDelete)})EnableWindow(c,segment&&!changes.empty());
    for(const auto field:{bandSelect,bandLabel,bandPatch,bandChannel,bandPan,bandVolume,GetDlgItem(window,BandSet)})ShowWindow(field,(styleMode||bandMode)?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(window,BandAdd),(bandMode||styleMode)?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(window,BandCollectionAssign),(bandMode||styleMode)?SW_SHOW:SW_HIDE);
    const auto old=SendMessageW(bandSelect,CB_GETCURSEL,0,0);SendMessageW(bandSelect,CB_RESETCONTENT,0,0);bandAssignments.clear();std::vector<BandDocument> bands;
    if(bandMode)bands.push_back(framework.band_document(activeBand));else if(styleMode)bands=framework.style_document(activeStyle).bands();
    for(size_t bi=0;bi<bands.size();++bi){const auto instruments=bands[bi].instruments();for(size_t ii=0;ii<instruments.size();++ii){bandAssignments.push_back({bi,ii});const auto label=L"Band "+std::to_wstring(bi+1)+L", PChannel "+std::to_wstring(instruments[ii].pchannel);SendMessageW(bandSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}}
    if(!bandAssignments.empty())SendMessageW(bandSelect,CB_SETCURSEL,old>=0&&static_cast<size_t>(old)<bandAssignments.size()?old:0,0);EnableWindow(bandSelect,!bandAssignments.empty());refresh_band_fields(window);
}
NoteContext note_context(){return {active,document().selected_groups(),document().selected_sequence_index()};}
void refresh_sequence_note_fields(HWND window){
    const bool segment=!styleMode&&!bandMode&&!framework.documents().empty();
    for(const auto field:{sequenceNoteSelect,sequenceNoteTime,sequenceNoteChannel,sequenceNoteLabel,sequenceTimeLabel,sequenceChannelLabel,GetDlgItem(window,SequenceNoteChange),GetDlgItem(window,SequenceNoteDelete)})ShowWindow(field,segment?SW_SHOW:SW_HIDE);
    SendMessageW(sequenceNoteSelect,CB_RESETCONTENT,0,0);bool selected=false;
    if(segment){const auto notes=document().notes();for(size_t i=0;i<notes.size();++i){const auto label=L"Note "+std::to_wstring(i+1)+L": "+std::to_wstring(notes[i].time)+L" clocks, pitch "+std::to_wstring(notes[i].pitch);SendMessageW(sequenceNoteSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
        if(!notes.empty()){auto& index=noteSelections[note_context()];index=std::min(index,notes.size()-1);SendMessageW(sequenceNoteSelect,CB_SETCURSEL,index,0);const auto& n=notes[index];SetWindowTextW(sequenceNoteTime,std::to_wstring(n.time).c_str());SetWindowTextW(sequenceNoteChannel,std::to_wstring(n.channel+1).c_str());SetWindowTextW(pitchEdit,std::to_wstring(n.pitch).c_str());SetWindowTextW(durationEdit,std::to_wstring(n.duration).c_str());SetWindowTextW(velocityEdit,std::to_wstring(n.velocity).c_str());selected=true;}
        else{SetWindowTextW(sequenceNoteTime,L"0");SetWindowTextW(sequenceNoteChannel,L"1");SetWindowTextW(pitchEdit,L"60");SetWindowTextW(durationEdit,L"384");SetWindowTextW(velocityEdit,L"96");}}
    EnableWindow(sequenceNoteSelect,selected);EnableWindow(sequenceNoteTime,selected);EnableWindow(sequenceNoteChannel,selected);EnableWindow(GetDlgItem(window,SequenceNoteChange),selected);EnableWindow(GetDlgItem(window,SequenceNoteDelete),selected);
}
void refresh_part_variation(HWND window){
    const auto part=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0),variation=SendMessageW(partVariationSelect,CB_GETCURSEL,0,0);const bool valid=styleMode&&part>=0&&static_cast<size_t>(part)<selectedPartIndexes.size()&&variation>=0&&variation<32;
    if(valid)SetWindowTextW(partVariationChoices,std::to_wstring(framework.style_document(activeStyle).parts().at(selectedPartIndexes[part]).variationChoices[variation]).c_str());
    for(const auto c:{partVariationSelect,partVariationChoices,GetDlgItem(window,StyleVariationSet)})EnableWindow(c,valid);
}
void refresh_style_note_fields(HWND window){
    refresh_part_variation(window);
    bool selected=false;const auto part=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0),note=SendMessageW(styleNoteSelect,CB_GETCURSEL,0,0);
    SetWindowTextW(styleNoteInfo,L"");if(styleMode&&part>=0&&static_cast<size_t>(part)<selectedPartIndexes.size()&&note>=0){const auto notes=framework.style_document(activeStyle).part_notes(selectedPartIndexes[part]);if(static_cast<size_t>(note)<notes.size()){const auto& n=notes[note];SetWindowTextW(durationEdit,std::to_wstring(n.duration).c_str());SetWindowTextW(velocityEdit,std::to_wstring(n.velocity).c_str());const auto text=L"Grid "+std::to_wstring(n.gridStart)+L", offset "+std::to_wstring(n.timeOffset)+L"\nVariation mask "+std::to_wstring(n.variation)+L"\nMusic value "+std::to_wstring(n.musicValue);SetWindowTextW(styleNoteInfo,text.c_str());selected=true;}}
    if(selected){const auto n=framework.style_document(activeStyle).part_notes(selectedPartIndexes[part]).at(static_cast<size_t>(note));SetWindowTextW(styleGrid,std::to_wstring(n.gridStart).c_str());SetWindowTextW(styleOffset,std::to_wstring(n.timeOffset).c_str());SetWindowTextW(styleMusic,std::to_wstring(n.musicValue).c_str());SetWindowTextW(styleVariation,std::to_wstring(n.variation).c_str());}
    const bool hasPart=styleMode&&part>=0&&static_cast<size_t>(part)<selectedPartIndexes.size();
    if(hasPart&&!selected){SetWindowTextW(durationEdit,L"384");SetWindowTextW(velocityEdit,L"96");SetWindowTextW(styleGrid,L"0");SetWindowTextW(styleOffset,L"0");SetWindowTextW(styleMusic,L"0");SetWindowTextW(styleVariation,L"4294967295");}
    if(styleMode){EnableWindow(durationEdit,hasPart);EnableWindow(velocityEdit,hasPart);}for(const auto field:{styleGrid,styleOffset,styleMusic,styleVariation})EnableWindow(field,hasPart);EnableWindow(GetDlgItem(window,StyleNoteSet),selected);EnableWindow(GetDlgItem(window,StyleNoteDelete),selected);EnableWindow(GetDlgItem(window,StyleNoteInsert),hasPart);SetWindowTextW(GetDlgItem(window,StyleNoteInsert),selected?L"Clone Note":L"Add Note");
}
void refresh_style_notes(HWND window,bool rebuildParts=true){
    for(const auto c:{partVariationSelect,partVariationChoices,partVariationLabel,GetDlgItem(window,StyleVariationSet),stylePartSelect,styleNoteSelect,stylePartLabel,styleNoteInfo,styleGrid,styleOffset,styleMusic,styleVariation,styleFieldsLabel,GetDlgItem(window,StyleNoteSet),GetDlgItem(window,StyleNoteInsert),GetDlgItem(window,StyleNoteDelete)})ShowWindow(c,styleMode?SW_SHOW:SW_HIDE);
    const auto oldPart=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0),oldNote=SendMessageW(styleNoteSelect,CB_GETCURSEL,0,0);
    if(rebuildParts){SendMessageW(stylePartSelect,CB_RESETCONTENT,0,0);selectedPartIndexes.clear();if(styleMode){const auto pattern=SendMessageW(events,LB_GETCURSEL,0,0);if(pattern>0){const auto& style=framework.style_document(activeStyle);const auto parts=style.parts();for(const auto& ref:style.part_references(static_cast<size_t>(pattern-1))){selectedPartIndexes.push_back(ref.partIndex);const auto text=L"Part "+std::to_wstring(ref.partIndex+1)+L": "+parts[ref.partIndex].name;SendMessageW(stylePartSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}}}if(!selectedPartIndexes.empty())SendMessageW(stylePartSelect,CB_SETCURSEL,oldPart>=0&&static_cast<size_t>(oldPart)<selectedPartIndexes.size()?oldPart:0,0);}
    SendMessageW(styleNoteSelect,CB_RESETCONTENT,0,0);const auto part=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0);size_t count=0;if(styleMode&&part>=0&&static_cast<size_t>(part)<selectedPartIndexes.size()){const auto notes=framework.style_document(activeStyle).part_notes(selectedPartIndexes[part]);count=notes.size();for(size_t i=0;i<count;++i){const auto text=L"Note "+std::to_wstring(i+1)+L", grid "+std::to_wstring(notes[i].gridStart);SendMessageW(styleNoteSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}if(count)SendMessageW(styleNoteSelect,CB_SETCURSEL,oldNote>=0&&static_cast<size_t>(oldNote)<count?oldNote:0,0);}EnableWindow(stylePartSelect,!selectedPartIndexes.empty());EnableWindow(styleNoteSelect,count!=0);refresh_style_note_fields(window);
}
void refresh_pattern(HWND window){
    EnableWindow(GetDlgItem(window,Copy),!bandMode&&(styleMode?SendMessageW(events,LB_GETCURSEL,0,0)>0:!framework.documents().empty()));
    EnableWindow(GetDlgItem(window,Paste),!bandMode&&(!framework.documents().empty()||styleMode));
    for(const auto c:{patternNameLabel,patternNameEdit,patternKindSelect,GetDlgItem(window,PatternProperties)})ShowWindow(c,styleMode?SW_SHOW:SW_HIDE);
    EnableMenuItem(GetMenu(window),PatternDuplicate,MF_BYCOMMAND|(styleMode&&SendMessageW(events,LB_GETCURSEL,0,0)>0?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),PatternNew,MF_BYCOMMAND|(styleMode?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),MotifNew,MF_BYCOMMAND|(styleMode?MF_ENABLED:MF_GRAYED));
    bool motifSelected=false;if(styleMode){const auto i=SendMessageW(events,LB_GETCURSEL,0,0);const auto patterns=framework.style_document(activeStyle).patterns();motifSelected=i>0&&static_cast<size_t>(i)<=patterns.size()&&(patterns[static_cast<size_t>(i)-1].embellishment&16);}
    EnableMenuItem(GetMenu(window),MotifSettings,MF_BYCOMMAND|(motifSelected?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),MotifPlay,MF_BYCOMMAND|(motifSelected?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),MotifBandAssign,MF_BYCOMMAND|(motifSelected&&!bandAssignments.empty()?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),MotifBandEdit,MF_BYCOMMAND|(motifSelected?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),PatternDelete,MF_BYCOMMAND|(styleMode&&SendMessageW(events,LB_GETCURSEL,0,0)>0?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(GetMenu(window),PatternUnshare,MF_BYCOMMAND|(styleMode&&SendMessageW(events,LB_GETCURSEL,0,0)>0?MF_ENABLED:MF_GRAYED));
    for(const auto control:{grooveLabel,grooveBottomEdit,grooveTopEdit,GetDlgItem(window,GrooveSet),patternLayoutLabel,patternBeats,patternDenominator,patternGrids,patternMeasures,GetDlgItem(window,PatternLayout)})ShowWindow(control,styleMode?SW_SHOW:SW_HIDE);
    bool selected=false;if(styleMode){const auto index=SendMessageW(events,LB_GETCURSEL,0,0);if(index>0){const auto patterns=framework.style_document(activeStyle).patterns();if(static_cast<size_t>(index)<=patterns.size()){const auto& pattern=patterns[index-1];SetWindowTextW(grooveBottomEdit,std::to_wstring(pattern.grooveBottom).c_str());SetWindowTextW(grooveTopEdit,std::to_wstring(pattern.grooveTop).c_str());selected=true;}}}
    if(selected){const auto p=framework.style_document(activeStyle).patterns().at(static_cast<size_t>(SendMessageW(events,LB_GETCURSEL,0,0)-1));SetWindowTextW(patternBeats,std::to_wstring(p.meter.beats).c_str());SetWindowTextW(patternDenominator,std::to_wstring(p.meter.denominator).c_str());SetWindowTextW(patternGrids,std::to_wstring(p.meter.grids).c_str());SetWindowTextW(patternMeasures,std::to_wstring(p.measures).c_str());}
    for(const auto input:{grooveBottomEdit,grooveTopEdit,GetDlgItem(window,GrooveSet),patternBeats,patternDenominator,patternGrids,patternMeasures,GetDlgItem(window,PatternLayout)})EnableWindow(input,selected);
    const bool styleRoot=styleMode&&SendMessageW(events,LB_GETCURSEL,0,0)==0;
    SetWindowTextW(patternNameLabel,styleRoot?L"Style name":L"Pattern name / type");SetWindowTextW(GetDlgItem(window,PatternProperties),styleRoot?L"Set Style Name":L"Set Pattern Properties");
    EnableWindow(patternNameEdit,selected||styleRoot);EnableWindow(patternKindSelect,selected);EnableWindow(GetDlgItem(window,PatternProperties),selected||styleRoot);
    SendMessageW(patternKindSelect,CB_RESETCONTENT,0,0);
    if(selected){const auto p=framework.style_document(activeStyle).patterns().at(static_cast<size_t>(SendMessageW(events,LB_GETCURSEL,0,0)-1));SetWindowTextW(patternNameEdit,p.name.c_str());
        const wchar_t* labels[]={L"Normal",L"Fill",L"Intro",L"Break",L"End"};int kind=-1;for(int i=0;i<5;++i){SendMessageW(patternKindSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(labels[i]));if(patternKinds[i]==p.embellishment)kind=i;}
        if(kind<0){SendMessageW(patternKindSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(p.embellishment==16?L"Motif":L"Other (unchanged)"));kind=5;}SendMessageW(patternKindSelect,CB_SETCURSEL,kind,0);
    }else SetWindowTextW(patternNameEdit,styleRoot?framework.style_document(activeStyle).name().c_str():L"");
    refresh_style_notes(window);
}
void set_status(std::wstring message) {
    for(const auto& warning:framework.warnings())message+=L"\r\nProject limitation: "+warning;
    SetWindowTextW(status,message.c_str());
}
void refresh_status() {
    auto message=playbackStatus;
    if(bandMode){set_status(L"Save Band first, then Assign DLS Collection. Copy this Band to a Segment to play it. Save Project includes the collection. Select the DLS document to edit instrument, region and PCM volume; save the DLS before the project.");return;}
    if(styleMode){const auto& style=framework.style_document(activeStyle);const auto meter=style.meter();message+=L". Style tempo: "+std::to_wstring(style.tempo())+L" BPM, meter "+std::to_wstring(meter.beats)+L"/"+std::to_wstring(meter.denominator)+L", grids "+std::to_wstring(meter.grids)+L". Change edits tempo; Set Meter edits Style default. Select a Pattern to edit groove, meter and length. Shared Part note fields edit all referring Patterns. Clone/Add and Delete support Undo/Redo. Music value follows harmony; Band patch/channel/pan/volume apply on next Play. Owned collections load on the next Play.";set_status(message);return;}
    if(!framework.documents().empty())message+=L". Applied groups: "+group_text(document().selected_groups())+L"; Tempo track "+std::to_wstring(document().selected_tempo_index()+1)+L"; Time signature track "+std::to_wstring(document().selected_meter_index()+1)+L". Notes: "+std::to_wstring(document().notes().size());
    if(!framework.documents().empty()) {
        try {const auto view=document().timeline();const auto& meter=view.meters().front();message+=L". Initial meter: "+std::to_wstring(meter.beats)+L"/"+std::to_wstring(meter.denominator)+L", grids "+std::to_wstring(meter.grids);}
        catch(const std::exception&){message+=L". Style meter unavailable: unresolved or unsupported references. Musical positions are hidden.";}
    }
    message+=L". Edits take effect on the next Play. Copy Band at clocks embeds the selected Band; later Band edits require another copy. Owned DLS collections load on the next Play.";
    set_status(message);
}
void refresh(HWND window) {
    const auto previous=SendMessageW(events,LB_GETCURSEL,0,0);
    SendMessageW(docs,CB_RESETCONTENT,0,0);
    for(const auto& d:framework.documents()) {const auto label=(d.path.empty()?L"Untitled segment":std::filesystem::path(d.path).filename().wstring())+(d.document->dirty()?L" *":L"");SendMessageW(docs,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    for(const auto& d:framework.style_documents()){const auto label=L"Style: "+(d.path.empty()?std::wstring(L"Untitled"):std::filesystem::path(d.path).filename().wstring())+(d.document->dirty()?L" *":L"");SendMessageW(docs,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    for(const auto& d:framework.band_documents()){const auto label=L"Band: "+(d.path.empty()?std::wstring(L"Untitled"):std::filesystem::path(d.path).filename().wstring())+(d.document->dirty()?L" *":L"");SendMessageW(docs,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    for(const auto& c:framework.collections()){const auto label=L"DLS: "+std::filesystem::path(c.path).filename().wstring()+(c.document.dirty()?L" *":L"");SendMessageW(docs,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    if(framework.documents().empty()&&!framework.style_documents().empty()&&!bandMode)styleMode=true;
    if(framework.documents().empty()&&framework.style_documents().empty()&&!framework.band_documents().empty()){bandMode=true;styleMode=false;}
    if(bandMode&&!framework.band_documents().empty()){activeBand=std::min(activeBand,framework.band_documents().size()-1);SendMessageW(docs,CB_SETCURSEL,framework.documents().size()+framework.style_documents().size()+activeBand,0);}
    else if(styleMode&&!framework.style_documents().empty()){activeStyle=std::min(activeStyle,framework.style_documents().size()-1);SendMessageW(docs,CB_SETCURSEL,framework.documents().size()+activeStyle,0);}
    else if(!framework.documents().empty()) {active=std::min(active,framework.documents().size()-1);SendMessageW(docs,CB_SETCURSEL,active,0);}
    const bool segmentAvailable=!styleMode&&!bandMode&&!framework.documents().empty();
    for(const auto id:{Add,Delete,Copy,Paste,MeterDelete,Play,NoteAdd})EnableWindow(GetDlgItem(window,id),segmentAvailable);
    EnableWindow(timeEdit,!styleMode&&!bandMode);for(const auto edit:{measureEdit,pitchEdit,durationEdit,velocityEdit})EnableWindow(edit,!styleMode&&!bandMode);for(const auto edit:{bpmEdit,beatsEdit,denominatorEdit,gridsEdit,GetDlgItem(window,Change),GetDlgItem(window,MeterSet)})EnableWindow(edit,!bandMode);
    SendMessageW(events,LB_RESETCONTENT,0,0);
    if(bandMode){for(const auto& i:framework.band_document(activeBand).instruments()){const auto text=L"PChannel "+std::to_wstring(i.pchannel)+L"   patch "+std::to_wstring(i.patch)+L"   pan "+std::to_wstring(i.pan)+L"   volume "+std::to_wstring(i.volume);SendMessageW(events,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}}
    else if(styleMode){const auto& style=framework.style_document(activeStyle);const auto text=L"Style tempo: "+std::to_wstring(style.tempo())+L" BPM";SendMessageW(events,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));SetWindowTextW(bpmEdit,std::to_wstring(style.tempo()).c_str());SetWindowTextW(timeEdit,L"");const auto meter=style.meter();SetWindowTextW(beatsEdit,std::to_wstring(meter.beats).c_str());SetWindowTextW(denominatorEdit,std::to_wstring(meter.denominator).c_str());SetWindowTextW(gridsEdit,std::to_wstring(meter.grids).c_str());
        const auto patterns=style.patterns();for(size_t i=0;i<patterns.size();++i){const auto& p=patterns[i];const auto label=L"Pattern "+std::to_wstring(i+1)+L": "+p.name+L"   groove "+std::to_wstring(p.grooveBottom)+L".."+std::to_wstring(p.grooveTop)+L"   "+std::to_wstring(p.measures)+L" measures";SendMessageW(events,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(events,LB_SETCURSEL,previous>0&&static_cast<size_t>(previous)<=patterns.size()?previous:0,0);
    }
    else if(!framework.documents().empty()) {
        auto& d=document();std::optional<Timeline> timeline;try{timeline=d.timeline();}catch(const std::exception&){}
        for(const auto& e:d.tempos()) {std::wostringstream label;label<<e.time<<L" clocks   "<<e.bpm<<L" BPM   ";if(timeline){const auto p=timeline->position(e.time);label<<p.measure+1<<L":"<<p.beat+1<<L":"<<p.tick;}else label<<L"musical position unavailable";const auto s=label.str();const auto i=SendMessageW(events,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));if(e.selected)SendMessageW(events,LB_SETCURSEL,i,0);}
        auto selected=SendMessageW(events,LB_GETCURSEL,0,0);
        if(selected<0&&!d.tempos().empty()) {selected=std::min<LRESULT>(std::max<LRESULT>(previous,0),static_cast<LRESULT>(d.tempos().size()-1));d.select(static_cast<size_t>(selected));SendMessageW(events,LB_SETCURSEL,selected,0);}
        if(selected>=0) {const auto& e=d.tempos().at(static_cast<size_t>(selected));SetWindowTextW(timeEdit,std::to_wstring(e.time).c_str());SetWindowTextW(bpmEdit,std::to_wstring(e.bpm).c_str());}
        else {SetWindowTextW(timeEdit,L"0");SetWindowTextW(bpmEdit,L"120");}
    }
    ShowWindow(GetDlgItem(window,SignpostEditor),!styleMode&&!bandMode?SW_SHOW:SW_HIDE);EnableWindow(GetDlgItem(window,SignpostEditor),!styleMode&&!bandMode&&!framework.documents().empty());ShowWindow(GetDlgItem(window,ChordEditor),!styleMode&&!bandMode?SW_SHOW:SW_HIDE);EnableWindow(GetDlgItem(window,ChordEditor),!styleMode&&!bandMode&&!framework.documents().empty());EnableWindow(GetDlgItem(window,CommandEditor),!styleMode&&!bandMode&&!framework.documents().empty());refresh_group_fields(window);refresh_bands(window);refresh_pattern(window);refresh_sequence_note_fields(window);refresh_status();InvalidateRect(window,nullptr,TRUE);
}
bool allow_discard(HWND window) {return !framework.dirty()||MessageBoxW(window,L"Discard unsaved documents?",L"Producer",MB_YESNO|MB_ICONQUESTION)==IDYES;}
void clipboard_copy(HWND window) {
    if(bandMode)throw std::runtime_error("Select a Segment or Style");
    const auto selected=SendMessageW(events,LB_GETCURSEL,0,0);if(styleMode&&selected<=0)throw std::runtime_error("Select a Pattern");
    const auto bytes=styleMode?framework.style_document(activeStyle).copy_pattern(static_cast<size_t>(selected-1)):document().copy();if(bytes.empty())throw std::runtime_error("Select an event");
    HGLOBAL data=GlobalAlloc(GMEM_MOVEABLE,bytes.size());if(!data)throw std::bad_alloc();void* p=GlobalLock(data);if(!p){GlobalFree(data);throw std::runtime_error("Clipboard allocation failed");}std::copy(bytes.begin(),bytes.end(),static_cast<std::uint8_t*>(p));GlobalUnlock(data);
    if(!OpenClipboard(window)){GlobalFree(data);throw std::runtime_error("Clipboard busy");}
    const bool ok=EmptyClipboard()&&SetClipboardData(styleMode?patternClipboardFormat:clipboardFormat,data);CloseClipboard();if(!ok){GlobalFree(data);throw std::runtime_error("Clipboard copy failed");}
}
void clipboard_paste(HWND window) {
    if(bandMode)throw std::runtime_error("Select a Segment or Style");
    const auto at=styleMode?0:clock_input();if(!OpenClipboard(window))throw std::runtime_error("Clipboard busy");
    Bytes bytes;const auto data=GetClipboardData(styleMode?patternClipboardFormat:clipboardFormat);const auto count=data?GlobalSize(data):0;
    if(count&&count<=64*1024*1024) {const auto p=static_cast<const std::uint8_t*>(GlobalLock(data));if(p){bytes.assign(p,p+count);GlobalUnlock(data);}}
    CloseClipboard();
    // GlobalSize may include allocation slack; the chunk header fixes length.
    if(bytes.size()>=8) {const auto n=read32(bytes,4);if(n<=bytes.size()-8)bytes.resize(n+8);}
    if(styleMode){const auto name=L"Pasted Pattern "+std::to_wstring(framework.style_document(activeStyle).patterns().size()+1);if(!framework.paste_style_pattern(activeStyle,bytes,name))throw std::runtime_error("Pattern paste rejected");}
    else if(!document().paste(bytes,at))throw std::runtime_error("Paste rejected or unchanged");
}
void append_message_line(const std::wstring& text){const auto line=text+L"\r\n";SendMessageW(messageText,EM_SETSEL,static_cast<WPARAM>(-1),static_cast<LPARAM>(-1));SendMessageW(messageText,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(line.c_str()));}
std::wstring script_error_line(const std::wstring& name,const ScriptResult& result){
    const auto& error=result.error;std::wostringstream out;out<<L"Script "<<name<<L" error 0x"<<std::hex<<static_cast<unsigned long>(result.result)<<std::dec<<L"; line "<<error.line<<L", character "<<error.character<<L": ";
    out.write(error.description,std::find(std::begin(error.description),std::end(error.description),wchar_t{})-std::begin(error.description));return out.str();
}
void collect_lyric_messages(){
    if(!messageText)return;
    const auto lyrics=conductor.observed_lyrics(),traces=conductor.observed_script_messages();
    for(;lyricCursor<lyrics.size();++lyricCursor)if(lyrics[lyricCursor].visible)append_message_line(lyrics[lyricCursor].text);
    for(;scriptMessageCursor<traces.size();++scriptMessageCursor)if(traces[scriptMessageCursor].visible)append_message_line(traces[scriptMessageCursor].text);
    const auto diagnostics=conductor.script_diagnostics();
    for(;scriptDiagnosticCursor<diagnostics.size();++scriptDiagnosticCursor){const auto& d=diagnostics[scriptDiagnosticCursor];append_message_line(script_error_line(d.operation+L" "+d.name,d.result));}
    const auto calls=conductor.track_script_calls();
    for(;scriptCallCursor<calls.size();++scriptCallCursor)if(calls[scriptCallCursor].messageVisible&&!calls[scriptCallCursor].result.passed())append_message_line(script_error_line(calls[scriptCallCursor].routine,calls[scriptCallCursor].result));
    if(conductor.lyric_observation_failed()||conductor.script_message_observation_failed()||conductor.script_diagnostic_overflow()||conductor.track_script_observation_overflow())SetWindowTextW(messageWindow,L"Message Window — delivery observation incomplete");
}
LRESULT CALLBACK message_window_proc(HWND window,UINT message,WPARAM w,LPARAM l){
    switch(message){
    case WM_CREATE:messageText=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,8,8,600,300,window,nullptr,nullptr,nullptr);SendMessageW(messageText,EM_SETLIMITTEXT,1048576,0);CreateWindowW(L"BUTTON",L"Clear",WS_CHILD|WS_VISIBLE|WS_TABSTOP,8,316,90,28,window,reinterpret_cast<HMENU>(1),nullptr,nullptr);SetTimer(window,1,100,nullptr);return 0;
    case WM_TIMER:try{collect_lyric_messages();}catch(...){SetWindowTextW(window,L"Message Window — delivery observation incomplete");}return 0;
    case WM_SIZE:if(messageText){MoveWindow(messageText,8,8,LOWORD(l)-16,HIWORD(l)-52,TRUE);MoveWindow(GetDlgItem(window,1),8,HIWORD(l)-36,90,28,TRUE);}return 0;
    case WM_COMMAND:if(LOWORD(w)==1){SetWindowTextW(messageText,L"");lyricCursor=conductor.observed_lyrics().size();scriptMessageCursor=conductor.observed_script_messages().size();scriptDiagnosticCursor=conductor.script_diagnostics().size();scriptCallCursor=conductor.track_script_calls().size();}return 0;
    case WM_CLOSE:ShowWindow(window,SW_HIDE);return 0;
    case WM_DESTROY:messageText=nullptr;messageWindow=nullptr;return 0;
    }return DefWindowProcW(window,message,w,l);
}
void show_message_window(HWND owner){
    if(!messageWindow){WNDCLASSW c{};c.lpfnWndProc=message_window_proc;c.hInstance=GetModuleHandleW(nullptr);c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducerMessages";if(!RegisterClassW(&c)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Message Window registration failed");messageWindow=CreateWindowW(c.lpszClassName,L"Message Window",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,660,430,owner,nullptr,c.hInstance,nullptr);if(!messageWindow)throw std::runtime_error("Message Window creation failed");}
    ShowWindow(messageWindow,SW_SHOW);collect_lyric_messages();SetForegroundWindow(messageWindow);
}
void command(HWND window,UINT id,UINT notification) {
    if(id>=TransportPath&&id<TransportPath+1000){
        const auto index=static_cast<size_t>(id-TransportPath);
        if(index>framework.audio_paths().size())throw std::runtime_error("Transport AudioPath is no longer available");
        conductor.set_default_audio_path(index?framework.audio_path_document(index-1).save_bytes():Bytes{});
        return;
    }
    switch(id) {
    case MessageWindowCommand:show_message_window(window);break;
    case WaveDocuments:show_wave_documents(window,framework);break;
    case ScriptDocuments:show_script_documents(window,framework,conductor);break;
    case ToolGraphDocuments:show_tool_graph_documents(window,framework);break;
    case ContainerDocuments:show_container_documents(window,framework);break;
    case TimelineRangeEditor:if(!styleMode&&!bandMode&&!framework.documents().empty()){show_timeline_range(window,document());refresh(window);}else throw std::runtime_error("Select a Segment for Timeline range editing");break;
    case ChordMapEditor:show_chordmap_editor(window,framework);break;
    case StylePlayerCommand:{const auto adopted=show_style_player(window,framework,conductor,styleMode?std::optional<size_t>(activeStyle):std::nullopt);if(adopted){active=*adopted;styleMode=false;bandMode=false;refresh_group_fields(window,true);}break;}
    case FarmPlayerCommand:show_farm_player(window,framework);break;
    case AudioPathEditor:show_audio_path_editor(window,framework);break;
    case ScriptTrackEditor:if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_script_track_editor(window,framework,active);break;
    case SegmentTriggerEditor:if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_segment_trigger_editor(window,framework,active);break;
    case ChordMapReferenceEditor:if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_chordmap_reference_editor(window,framework,active);break;
    case StyleReferenceEditor:if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_style_reference_editor(window,framework,active);break;
    case NewProject: if(allow_discard(window)){conductor.stop();playbackMonitor.clear();KillTimer(window,1);playbackStatus=L"Stopped";framework.new_project();noteSelections.clear();commandSelections.clear();chordSelections.clear();signpostSelections.clear();markerSelections.clear();waveSelections.clear();lyricSelections.clear();muteSelections.clear();paramSelections.clear();styleMode=false;bandMode=false;active=framework.new_segment();refresh_group_fields(window,true);}break;
    case NewSegment: styleMode=false;bandMode=false;active=framework.new_segment();refresh_group_fields(window,true);break;
    case ImportMidi: {const auto path=choose(window,false,L"MIDI files\0*.mid;*.midi\0",L"mid");if(path.empty())return;
        active=framework.import_midi_segment(path);styleMode=false;bandMode=false;refresh_group_fields(window,true);break;}
    case NewStyle: activeStyle=framework.new_style();styleMode=true;bandMode=false;break;
    case NewBand: activeBand=framework.new_band();bandMode=true;styleMode=false;break;
    case NewDls: {const auto collection=framework.new_collection();show_dls_editor(window,framework,collection);break;}
    case NewPlaybackTest: styleMode=false;bandMode=false;active=framework.new_segment();framework.document(active)=SegmentDocument::playback_test();refresh_group_fields(window,true);break;
    case Open: {
        const auto path=choose(window,false,L"Producer documents\0*.sgp;*.sgt;*.stp;*.sty;*.bnp;*.bnd;*.dls;*.dlp;*.aup;*.aud;*.cdp;*.cdm;*.wav;*.wvp;*.spp;*.spt;*.cop;*.con;*.tgp;*.tgr;*.pro;*.dmpj\0All files\0*.*\0",L"sgp");if(path.empty())return;
        const auto kind=Framework::document_kind(path);
        if(kind==DocumentKind::Project&&!allow_discard(window))return;
        const auto opened=framework.open_document(path);
        switch(opened.kind){
        case DocumentKind::Project:{noteSelections.clear();commandSelections.clear();chordSelections.clear();signpostSelections.clear();markerSelections.clear();waveSelections.clear();lyricSelections.clear();muteSelections.clear();paramSelections.clear();active=0;activeStyle=0;activeBand=0;styleMode=framework.documents().empty()&&!framework.style_documents().empty();bandMode=framework.documents().empty()&&framework.style_documents().empty()&&!framework.band_documents().empty();}break;
        case DocumentKind::Segment:active=opened.index;styleMode=false;bandMode=false;break;
        case DocumentKind::Style:activeStyle=opened.index;styleMode=true;bandMode=false;break;
        case DocumentKind::Band:activeBand=opened.index;bandMode=true;styleMode=false;break;
        case DocumentKind::Collection:show_dls_editor(window,framework,opened.index);break;
        case DocumentKind::AudioPath:show_audio_path_editor(window,framework,opened.index);break;
        case DocumentKind::ChordMap:show_chordmap_editor(window,framework,opened.index);break;
        case DocumentKind::Wave:show_wave_documents(window,framework,opened.index);break;
        case DocumentKind::Script:show_script_documents(window,framework,conductor,opened.index);break;
        case DocumentKind::Container:show_container_documents(window,framework,opened.index);break;
        case DocumentKind::ToolGraph:show_tool_graph_documents(window,framework,opened.index);break;
        }refresh_group_fields(window,true);break;
    }
    case SaveSegment: case SaveDocumentAs: {
        const bool saveAs=id==SaveDocumentAs;
        if(bandMode){auto path=framework.band_documents().at(activeBand).path;if(saveAs||path.empty())path=choose(window,true,L"Band\0*.bnp;*.bnd\0",L"bnp");if(!path.empty())framework.save_band(activeBand,path);break;}
        if(styleMode){auto path=framework.style_documents().at(activeStyle).path;if(saveAs||path.empty())path=choose(window,true,L"Style\0*.stp;*.sty\0",L"stp");if(!path.empty())framework.save_style(activeStyle,path);break;}
        auto path=framework.documents().at(active).path;if(saveAs||path.empty())path=choose(window,true,L"Segment\0*.sgp\0",L"sgp");if(!path.empty())framework.save_segment(active,path);break;
    }
    case SaveProject: {const auto path=choose(window,true,L"Product project\0*.dmpj\0Native Producer project (name must match folder)\0*.pro\0",L"dmpj");if(!path.empty())framework.save_project(path);break;}
    case CopyProject: {if(framework.project_path().empty())throw std::runtime_error("Save Project before copying");const auto extension=std::filesystem::path(framework.project_path()).extension().wstring();const auto path=choose(window,true,L"Project copy (creates a new named folder)\0*.pro;*.dmpj\0",extension==L".pro"?L"pro":L"dmpj");if(!path.empty()){const auto requested=std::filesystem::path(path);framework.copy_project((requested.parent_path()/requested.stem()/requested.filename()).wstring());}break;}
    case RuntimeSettings:show_runtime_settings(window,framework,bandMode?RuntimeDocumentKind::Band:styleMode?RuntimeDocumentKind::Style:RuntimeDocumentKind::Segment,bandMode?activeBand:styleMode?activeStyle:active);break;
    case RuntimeSaveAs: {const auto kind=bandMode?RuntimeDocumentKind::Band:styleMode?RuntimeDocumentKind::Style:RuntimeDocumentKind::Segment;const auto index=bandMode?activeBand:styleMode?activeStyle:active;const auto ext=bandMode?L"bnd":styleMode?L"sty":L"sgt";const auto filter=bandMode?L"Runtime Band\0*.bnd\0":styleMode?L"Runtime Style\0*.sty\0":L"Runtime Segment\0*.sgt\0";const auto path=choose(window,true,filter,ext);if(!path.empty())framework.save_runtime_as(kind,index,path);break;}
    case RuntimeSaveDefaults: {framework.export_runtime_defaults();break;}
    case RuntimeRecovery:show_runtime_recovery(window,framework);break;
    case RuntimeSaveAll: {const auto path=choose(window,true,L"Runtime output folder (new or existing)\0*.*\0",L"");if(!path.empty())framework.export_runtime(path);break;}
    case Exit: SendMessageW(window,WM_CLOSE,0,0);return;
    case Add: if(!document().add_tempo(clock_input(),bpm_input()))throw std::runtime_error("Tempo insert rejected or unchanged (range: 1 to 1000 BPM)");break;
    case Change: if(styleMode){if(!framework.set_style_tempo(activeStyle,bpm_input()))throw std::runtime_error("Style tempo rejected or unchanged (1 to 1000 BPM)");}else if(!document().change_selected(bpm_input()))throw std::runtime_error("Select an event and enter 1 to 1000 BPM");break;
    case Delete: document().delete_selected();break;
    case Copy: clipboard_copy(window);break;
    case Paste: clipboard_paste(window);if(styleMode){refresh(window);SendMessageW(events,LB_SETCURSEL,framework.style_document(activeStyle).patterns().size(),0);refresh_pattern(window);return;}break;
    case Undo: if(bandMode)framework.undo_band(activeBand);else if(styleMode)framework.undo_style(activeStyle);else framework.undo_segment(active);break;
    case Redo: if(bandMode)framework.redo_band(activeBand);else if(styleMode)framework.redo_style(activeStyle);else framework.redo_segment(active);break;
    case CommandEditor: show_command_editor(window,framework,active,commandSelections[active]);break;
    case ChordEditor: show_chord_editor(window,framework,active,chordSelections[active]);break;
    case SignpostEditor: show_signpost_editor(window,framework,active,signpostSelections[active]);break;
    case MarkerEditor: if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_marker_editor(window,framework,active,markerSelections[active]);break;
    case LyricEditor: if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_lyric_editor(window,framework,active,lyricSelections[active]);break;
    case ParamControlEditor: if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_param_control_editor(window,framework,active,paramSelections[active]);break;
    case MuteEditor: if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_mute_editor(window,framework,active,muteSelections[active]);break;
    case WaveEditor: if(styleMode||bandMode||framework.documents().empty())throw std::runtime_error("Select a Segment first");show_wave_editor(window,framework,active,waveSelections[active]);break;
    case MotifBandAssign:{const auto i=SendMessageW(events,LB_GETCURSEL,0,0),b=SendMessageW(bandSelect,CB_GETCURSEL,0,0);if(!styleMode||i<=0||b<0||static_cast<size_t>(b)>=bandAssignments.size()||!framework.assign_style_motif_band(activeStyle,static_cast<size_t>(i)-1,bandAssignments[b].first))throw std::runtime_error("Select a Motif and Style Band; assignment rejected or unchanged");break;}
    case MotifBandEdit:{const auto i=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||i<=0)throw std::runtime_error("Select a Motif");show_motif_band_editor(window,framework,activeStyle,static_cast<size_t>(i)-1);break;}
    case MotifSettings:{const auto i=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||i<=0)throw std::runtime_error("Select a Motif");show_motif_editor(window,framework,activeStyle,static_cast<size_t>(i)-1);break;}
    case MotifPlay:{const auto i=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||i<=0)throw std::runtime_error("Select a Motif");const auto options=choose_motif_playback(window,framework);if(!options)break;const auto name=framework.style_document(activeStyle).patterns().at(static_cast<size_t>(i)-1).name;conductor.play_motif(framework.style_playback_snapshot(activeStyle),name,window,framework.style_playback_collections(activeStyle),options->options,options->audioPath?framework.audio_path_document(*options->audioPath).save_bytes():Bytes{});playbackStatus=L"Scheduled Motif: "+name;SetTimer(window,1,100,nullptr);break;}
    case GroupApply: {try{document().select_track_group(group_input(),meter_input(groupTempoEdit)-1,meter_input(groupMeterEdit)-1,meter_input(groupSequenceEdit)-1,meter_input(groupBandEdit)-1);refresh_group_fields(window,true);}catch(...){refresh_group_fields(window);throw;}break;}
    case MeterSet: if(styleMode){if(!framework.set_style_meter(activeStyle,meter_input(beatsEdit),meter_input(denominatorEdit),meter_input(gridsEdit)))throw std::runtime_error("Style meter rejected or unchanged");}else if(!document().set_meter(static_cast<std::int32_t>(meter_input(measureEdit)-1),meter_input(beatsEdit),meter_input(denominatorEdit),meter_input(gridsEdit)))throw std::runtime_error("Meter edit rejected or unchanged. Edit the referenced Style for Style-backed documents.");break;
    case GrooveSet: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||selected<=0||!framework.set_pattern_groove(activeStyle,static_cast<size_t>(selected-1),meter_input(grooveBottomEdit,true),meter_input(grooveTopEdit,true)))throw std::runtime_error("Select a Pattern; groove is 0..100 with bottom <= top");break;}
    case PatternLayout: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||selected<=0||!framework.set_pattern_layout(activeStyle,static_cast<size_t>(selected-1),meter_input(patternBeats),meter_input(patternDenominator),meter_input(patternGrids),meter_input(patternMeasures)))throw std::runtime_error("Select a Pattern; layout rejected or unchanged");break;}
    case PatternDuplicate: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||selected<=0)throw std::runtime_error("Select a Pattern");const auto& style=framework.style_document(activeStyle);const auto name=style.patterns().at(static_cast<size_t>(selected-1)).name+L" Copy";if(!framework.duplicate_style_pattern(activeStyle,static_cast<size_t>(selected-1),name))throw std::runtime_error("Pattern copy rejected");refresh(window);SendMessageW(events,LB_SETCURSEL,framework.style_document(activeStyle).patterns().size(),0);refresh_pattern(window);return;}
    case PatternNew: {if(!styleMode)throw std::runtime_error("Select a Style");const auto name=L"New Pattern "+std::to_wstring(framework.style_document(activeStyle).patterns().size()+1);if(!framework.new_style_pattern(activeStyle,name))throw std::runtime_error("Pattern creation rejected");refresh(window);SendMessageW(events,LB_SETCURSEL,framework.style_document(activeStyle).patterns().size(),0);refresh_pattern(window);return;}
    case MotifNew: {if(!styleMode)throw std::runtime_error("Select a Style");const auto name=L"New Motif "+std::to_wstring(framework.style_document(activeStyle).patterns().size()+1);if(!framework.new_style_motif(activeStyle,name))throw std::runtime_error("Motif creation rejected");refresh(window);SendMessageW(events,LB_SETCURSEL,framework.style_document(activeStyle).patterns().size(),0);refresh_pattern(window);return;}
    case PatternDelete: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0);if(!styleMode||selected<=0)throw std::runtime_error("Select a Pattern");if(MessageBoxW(window,L"Delete the selected Pattern and its unshared Parts?",L"Delete Pattern",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;if(!framework.delete_style_pattern(activeStyle,static_cast<size_t>(selected-1)))throw std::runtime_error("Pattern deletion rejected");break;}
    case PatternUnshare: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0),ref=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0);if(!styleMode||selected<=0||ref<0)throw std::runtime_error("Select a Pattern Part");if(!framework.unshare_style_pattern_part(activeStyle,static_cast<size_t>(selected-1),static_cast<size_t>(ref)))throw std::runtime_error("This Part is already independent");break;}
    case PatternProperties: {const auto selected=SendMessageW(events,LB_GETCURSEL,0,0),kind=SendMessageW(patternKindSelect,CB_GETCURSEL,0,0);if(styleMode&&selected==0){if(!framework.set_style_name(activeStyle,control_text(patternNameEdit)))throw std::runtime_error("Style name unchanged or invalid; name must be 1..255 characters");break;}if(!styleMode||selected<=0||kind<0||kind>5)throw std::runtime_error("Select a Pattern and type");const auto index=static_cast<size_t>(selected-1);const auto value=kind<5?patternKinds[kind]:framework.style_document(activeStyle).patterns().at(index).embellishment;if(!framework.set_pattern_properties(activeStyle,index,control_text(patternNameEdit),value))throw std::runtime_error("Pattern properties unchanged or unsupported; name must be 1..255 characters");break;}
    case StylePartSelect: if(notification==CBN_SELCHANGE){refresh_style_notes(window,false);return;}return;
    case BandSelect: if(notification==CBN_SELCHANGE){refresh_band_fields(window);return;}return;
    case BandCollectionAssign: {if(!bandMode&&!styleMode)throw std::runtime_error("Select a Band or Style");const auto selected=SendMessageW(bandSelect,CB_GETCURSEL,0,0);if(selected<0)throw std::runtime_error("Select an instrument");if((styleMode?framework.style_documents().at(activeStyle).path:framework.band_documents().at(activeBand).path).empty())throw std::runtime_error("Save Band or Style before assigning a collection");const auto path=choose(window,false,L"DLS collection\0*.dls\0",L"dls");if(path.empty())return;const auto collection=framework.open_collection(path);const auto assignment=bandAssignments.at(selected);const auto chosen=choose_dls_instrument(window,framework.collection_document(collection));if(!chosen){refresh(window);return;}if(!(styleMode?framework.set_style_band_collection_instrument(activeStyle,assignment.first,assignment.second,collection,*chosen):framework.set_band_collection_instrument(activeBand,assignment.second,collection,*chosen)))throw std::runtime_error("Collection unchanged or reference invalid");break;}
    case SegmentBandSelect: return;
    case BandEventSelect: if(notification==CBN_SELCHANGE){const auto index=SendMessageW(bandEventSelect,CB_GETCURSEL,0,0);const auto e=document().band_events().at(index);SetWindowTextW(bandLogical,std::to_wstring(e.logicalTime).c_str());SetWindowTextW(bandPhysical,std::to_wstring(e.physicalTime).c_str());}return;
    case BandEventMove: case BandEventDelete: {const auto index=SendMessageW(bandEventSelect,CB_GETCURSEL,0,0);if(index<0)throw std::runtime_error("Select a Band event");const bool changed=id==BandEventDelete?document().delete_band(index):document().move_band(index,static_cast<std::int32_t>(integer_input(bandLogical,0,INT32_MAX)),static_cast<std::int32_t>(integer_input(bandPhysical,INT32_MIN,INT32_MAX)));if(!changed)throw std::runtime_error("Band time unchanged, duplicate, or outside Segment");break;}
    case SegmentBandAssign: {const auto bi=SendMessageW(segmentBandSelect,CB_GETCURSEL,0,0);if(styleMode||bandMode||bi<0||static_cast<size_t>(bi)>=segmentBandSources.size())throw std::runtime_error("Select a Segment and owned Band or Style Band");const auto source=segmentBandSources[bi];const auto time=static_cast<std::int32_t>(integer_input(timeEdit,0,INT32_MAX));const bool changed=source.style?framework.assign_style_band(active,source.document,source.band,time):framework.assign_band(active,source.document,time);if(!changed)throw std::runtime_error("Band copy unchanged or time outside Segment");break;}
    case BandAdd: case BandSet: {
        const auto patch=static_cast<std::uint32_t>(integer_input(bandPatch,0,UINT32_MAX)),channel=static_cast<std::uint32_t>(integer_input(bandChannel,0,UINT32_MAX));const auto pan=static_cast<unsigned>(integer_input(bandPan,0,127)),volume=static_cast<unsigned>(integer_input(bandVolume,0,127));bool changed=false;
        if(id==BandAdd&&bandMode)changed=framework.add_band_gm_instrument(activeBand,patch,channel,pan,volume);
        else if(id==BandAdd&&styleMode){const auto selected=SendMessageW(bandSelect,CB_GETCURSEL,0,0);std::optional<size_t> target;if(selected>=0&&static_cast<size_t>(selected)<bandAssignments.size())target=bandAssignments[selected].first;else if(!framework.style_document(activeStyle).bands().empty())target=0;changed=framework.add_style_band_gm_instrument(activeStyle,target,patch,channel,pan,volume);}
        else {const auto selected=SendMessageW(bandSelect,CB_GETCURSEL,0,0);if((!styleMode&&!bandMode)||selected<0||static_cast<size_t>(selected)>=bandAssignments.size())throw std::runtime_error("Select a Band instrument");const auto [bi,ii]=bandAssignments[selected];changed=bandMode?framework.set_band_instrument(activeBand,ii,patch,channel,pan,volume):framework.set_style_band_instrument(activeStyle,bi,ii,patch,channel,pan,volume);}
        if(!changed)throw std::runtime_error("Band assignment unchanged or invalid: 7-bit patch/banks plus percussion bit31, unique PChannel, pan/volume 0..127");break;
    }
    case StyleVariationSelect: if(notification==CBN_SELCHANGE)refresh_part_variation(window);return;
    case StyleVariationSet:{const auto part=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0),variation=SendMessageW(partVariationSelect,CB_GETCURSEL,0,0);if(!styleMode||part<0||static_cast<size_t>(part)>=selectedPartIndexes.size()||variation<0||variation>=32)throw std::runtime_error("Select a Part variation");if(!framework.set_style_part_variation_choice(activeStyle,selectedPartIndexes[part],static_cast<size_t>(variation),static_cast<std::uint32_t>(integer_input(partVariationChoices,0,UINT32_MAX))))throw std::runtime_error("Variation choices unchanged");break;}
    case StyleNoteSelect: if(notification==CBN_SELCHANGE){refresh_style_note_fields(window);return;}return;
    case StyleNoteSet: case StyleNoteInsert: case StyleNoteDelete: {
        const auto part=SendMessageW(stylePartSelect,CB_GETCURSEL,0,0),note=SendMessageW(styleNoteSelect,CB_GETCURSEL,0,0);if(!styleMode||part<0||static_cast<size_t>(part)>=selectedPartIndexes.size())throw std::runtime_error("Select a Pattern Part");const auto pi=selectedPartIndexes[part];bool changed=false;
        if(id==StyleNoteInsert){const auto position=note<0?0:static_cast<size_t>(note)+1;changed=framework.insert_style_part_note(activeStyle,pi,position,style_note_input(),note<0?std::optional<size_t>{}:static_cast<size_t>(note));if(changed){refresh(window);SendMessageW(styleNoteSelect,CB_SETCURSEL,position,0);refresh_style_note_fields(window);return;}}
        else if(note>=0){if(id==StyleNoteDelete)changed=framework.delete_style_part_note(activeStyle,pi,static_cast<size_t>(note));else changed=framework.edit_style_part_note(activeStyle,pi,static_cast<size_t>(note),style_note_input());}
        if(!changed)throw std::runtime_error("Select a Part note; duration positive,velocity1..127; unchanged or invalid edit rejected");break;
    }
    case MeterDelete: if(!document().delete_meter(static_cast<std::int32_t>(meter_input(measureEdit)-1)))throw std::runtime_error("Cannot remove the initial meter or edit a Style-backed document");break;
    case Play: {const auto& path=framework.documents().at(active).path;conductor.play(document().save_bytes(),path.empty()?L"":std::filesystem::path(path).parent_path().wstring(),window,document().styles(),framework.playback_collections(active),{},framework.playback_waves(active),framework.trigger_playback(active),framework.playback_chordmaps(active));playbackStatus=L"Playing document snapshot";SetTimer(window,1,100,nullptr);break;}
    case StartFileOutput:{Bytes bytes=document().audio_path();if(bytes.empty())bytes=conductor.default_audio_path();if(bytes.empty())throw std::runtime_error("Select an AudioPath with FileOutput first");const auto output=choose(window,true,L"Buffer recording WAV\0*.wav\0",L"wav");if(!output.empty()){conductor.start_file_output(bytes,output,window);playbackStatus=L"Buffer recording started; Play and Stop leave recording active";}break;}
    case WavesReverbCommand:{
        const auto bytes=conductor.playback_bytes();if(bytes.empty())throw std::runtime_error("Play a Segment with Waves Reverb first");
        const auto root=Chunk::parse(bytes);const auto config=root.find("RIFF","DMAP");if(!config)throw std::runtime_error("The playing Segment has no owned AudioPath");
        AudioPathDocument path;path.load(config->encode());const auto ids=path.buffers();const auto effects=path.effects();
        std::vector<size_t> seen;std::wostringstream text;
        for(const auto& port:path.ports())for(const auto& route:port.routes)for(size_t i=0;i<route.buffers.size();++i){
            const auto found=std::find(ids.begin(),ids.end(),route.buffers[i]);const auto buffer=static_cast<size_t>(found-ids.begin());
            if(std::find(seen.begin(),seen.end(),buffer)!=seen.end())continue;seen.push_back(buffer);DWORD occurrence=0;
            for(const auto& effect:effects)if(effect.buffer==buffer){GUID cls{};std::memcpy(&cls,effect.classId.data(),16);if(!IsEqualGUID(cls,producer::compat::wavesReverbClass))continue;
                const auto p=conductor.waves_reverb_parameters(route.base,static_cast<DWORD>(i),occurrence++);
                text<<L"Buffer "<<buffer+1<<L", Waves Reverb "<<occurrence<<L":\nInput gain "<<p.inputGain<<L" dB\nReverb mix "<<p.reverbMix<<L" dB\nReverb time "<<p.reverbTime<<L" ms\nHigh-frequency time ratio "<<p.highFrequencyRatio<<L"\n\n";
            }
        }
        if(text.str().empty())throw std::runtime_error("The playing AudioPath has no Waves Reverb on a PChannel buffer");
        MessageBoxW(window,text.str().c_str(),L"Playing Waves Reverb",MB_OK|MB_ICONINFORMATION);break;
    }
    case StopFileOutput:conductor.stop_file_output();playbackStatus=L"Buffer recording stopped and WAV finalized";break;
    case Stop: conductor.stop();collect_lyric_messages();playbackMonitor.clear();KillTimer(window,1);playbackStatus=L"Stopped";break;
    case PlaybackSessions: show_playback_window(window,conductor);break;
    case StopCurrentPlayback: {const auto playbackId=conductor.current_playback_id();if(playbackId){conductor.stop(playbackId);playbackMonitor.forget(playbackId);}playbackStatus=conductor.current_playback_id()?L"Stopped selected playback; other playback retained":L"Stopped";if(!conductor.current_playback_id())KillTimer(window,1);break;}
    case NoteAdd: {const auto velocity=meter_input(velocityEdit);
        const auto text=control_text(pitchEdit);size_t end=0;const auto note=std::stoll(text,&end);if(end!=text.size()||note<0||note>127||velocity>127)throw std::runtime_error("Pitch is 0 to 127; velocity is 1 to 127");
        if(!document().add_note({clock_input(),static_cast<std::int32_t>(meter_input(durationEdit)),0,static_cast<BYTE>(note),static_cast<BYTE>(velocity)}))throw std::runtime_error("Note must fit inside the segment");break;}
    case SequenceNoteSelect: if(notification==CBN_SELCHANGE){const auto i=SendMessageW(sequenceNoteSelect,CB_GETCURSEL,0,0);if(i>=0)noteSelections[note_context()]=static_cast<size_t>(i);refresh_sequence_note_fields(window);}return;
    case SequenceNoteChange:{const auto i=SendMessageW(sequenceNoteSelect,CB_GETCURSEL,0,0);if(i<0)throw std::runtime_error("Select a Sequence note");const Note n{static_cast<std::int32_t>(integer_input(sequenceNoteTime,0,INT32_MAX)),static_cast<std::int32_t>(integer_input(durationEdit,1,INT32_MAX)),static_cast<std::uint32_t>(integer_input(sequenceNoteChannel,1,0xfffffffcu)-1),static_cast<BYTE>(integer_input(pitchEdit,0,127)),static_cast<BYTE>(integer_input(velocityEdit,1,127))};size_t after=0;if(!document().edit_note(static_cast<size_t>(i),n,&after))throw std::runtime_error("Note edit unchanged or outside the Segment");noteSelections[note_context()]=after;break;}
    case SequenceNoteDelete:{const auto i=SendMessageW(sequenceNoteSelect,CB_GETCURSEL,0,0);if(i<0)throw std::runtime_error("Select a Sequence note");auto next=document();if(!next.delete_note(static_cast<size_t>(i)))throw std::runtime_error("Note selection changed");if(MessageBoxW(window,L"Delete the selected Sequence note? Undo restores it.",L"Delete Sequence Note",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;document()=std::move(next);break;}
    case Documents: if(notification==CBN_SELCHANGE){const auto selected=SendMessageW(docs,CB_GETCURSEL,0,0);if(selected<0)return;const auto si=static_cast<size_t>(selected);const auto collectionStart=framework.documents().size()+framework.style_documents().size()+framework.band_documents().size();if(si>=collectionStart){show_dls_editor(window,framework,si-collectionStart);refresh(window);return;}bandMode=si>=framework.documents().size()+framework.style_documents().size();styleMode=!bandMode&&si>=framework.documents().size();if(bandMode)activeBand=si-framework.documents().size()-framework.style_documents().size();else if(styleMode)activeStyle=si-framework.documents().size();else active=si;SendMessageW(events,LB_SETCURSEL,static_cast<WPARAM>(-1),0);refresh_group_fields(window,true);}else return;break;
    case Events: if(bandMode)return;if(styleMode){if(notification==LBN_SELCHANGE)refresh_pattern(window);return;}if(notification==LBN_SELCHANGE){const auto i=SendMessageW(events,LB_GETCURSEL,0,0);if(i>=0){document().select(static_cast<size_t>(i));const auto& e=document().tempos().at(static_cast<size_t>(i));SetWindowTextW(timeEdit,std::to_wstring(e.time).c_str());SetWindowTextW(bpmEdit,std::to_wstring(e.bpm).c_str());}}else return;break;
    default:return;
    }
    refresh(window);
}
HWND control(HWND parent,const wchar_t* type,const wchar_t* text,DWORD style,int x,int y,int w,int h,UINT id=0) {
    const auto c=CreateWindowW(type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),nullptr,nullptr);
    if(!c)throw std::runtime_error("Unable to create editor control");SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    try {
        switch(message) {
        case WM_CREATE: {
            conductor.set_tool_factories(source_tool_factories());
            conductor.enable_lyric_observation();conductor.enable_script_message_observation();
            docs=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,12,550,250,Documents);
            events=control(window,L"LISTBOX",L"",LBS_NOTIFY|WS_BORDER|WS_VSCROLL|WS_TABSTOP,16,50,550,180,Events);
            control(window,L"STATIC",L"Clocks",0,16,246,50,22);timeEdit=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,70,242,110,25);
            control(window,L"STATIC",L"BPM",0,195,246,45,22);bpmEdit=control(window,L"EDIT",L"120",WS_BORDER|WS_TABSTOP,240,242,100,25);
            const wchar_t* labels[]={L"Add",L"Change",L"Delete",L"Copy",L"Paste",L"Undo",L"Redo"};
            for(UINT i=0;i<7;++i)control(window,L"BUTTON",labels[i],WS_TABSTOP,16+static_cast<int>(i)*78,282,72,28,Add+i);
            control(window,L"STATIC",L"Measure",0,16,326,60,22);measureEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,80,322,65,25);
            control(window,L"STATIC",L"Beats",0,155,326,45,22);beatsEdit=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,202,322,55,25);
            control(window,L"STATIC",L"Denominator",0,270,326,90,22);denominatorEdit=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,364,322,55,25);
            control(window,L"STATIC",L"Grids",0,432,326,45,22);gridsEdit=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,480,322,70,25);
            control(window,L"BUTTON",L"Set Meter",WS_TABSTOP,16,358,120,28,MeterSet);control(window,L"BUTTON",L"Delete Meter",WS_TABSTOP,148,358,120,28,MeterDelete);
            grooveLabel=control(window,L"STATIC",L"Pattern groove",0,16,398,90,24);grooveBottomEdit=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,112,394,60,26,GrooveBottom);grooveTopEdit=control(window,L"EDIT",L"100",WS_BORDER|WS_TABSTOP,180,394,60,26,GrooveTop);control(window,L"BUTTON",L"Set Groove",WS_TABSTOP,248,394,100,28,GrooveSet);
            patternLayoutLabel=control(window,L"STATIC",L"Pattern: beats / denominator / grids / measures",0,16,430,540,22);
            patternBeats=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,16,456,65,26);patternDenominator=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,92,456,65,26);patternGrids=control(window,L"EDIT",L"4",WS_BORDER|WS_TABSTOP,168,456,65,26);patternMeasures=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,244,456,65,26);control(window,L"BUTTON",L"Set Pattern Layout",WS_TABSTOP,324,456,160,28,PatternLayout);
            control(window,L"BUTTON",L"Play",WS_TABSTOP,582,50,74,28,Play);control(window,L"BUTTON",L"Stop",WS_TABSTOP,666,50,74,28,Stop);
            control(window,L"STATIC",L"MIDI pitch",0,582,96,140,22);pitchEdit=control(window,L"EDIT",L"60",WS_BORDER|WS_TABSTOP,582,120,130,25);
            control(window,L"STATIC",L"Duration (clocks)",0,582,154,150,22);durationEdit=control(window,L"EDIT",L"384",WS_BORDER|WS_TABSTOP,582,178,130,25);
            control(window,L"STATIC",L"Velocity",0,582,212,140,22);velocityEdit=control(window,L"EDIT",L"96",WS_BORDER|WS_TABSTOP,582,236,130,25);
            control(window,L"BUTTON",L"Add Note (channel 1)",WS_TABSTOP,582,280,155,28,NoteAdd);
            sequenceNoteLabel=control(window,L"STATIC",L"Sequence note",0,582,312,174,22);sequenceNoteSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,582,336,174,260,SequenceNoteSelect);
            sequenceTimeLabel=control(window,L"STATIC",L"Note clocks",0,582,370,174,22);sequenceNoteTime=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,582,394,174,25,SequenceNoteTime);
            sequenceChannelLabel=control(window,L"STATIC",L"PChannel (1-4294967292)",0,582,430,174,22);sequenceNoteChannel=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,582,454,174,25,SequenceNoteChannel);
            control(window,L"BUTTON",L"Change Note",WS_TABSTOP,582,490,174,28,SequenceNoteChange);control(window,L"BUTTON",L"Delete Sequence Note...",WS_TABSTOP,582,520,174,26,SequenceNoteDelete);
            stylePartLabel=control(window,L"STATIC",L"Pattern Part (shared)",0,582,312,165,22);stylePartSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,582,336,155,260,StylePartSelect);styleNoteSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,582,374,155,260,StyleNoteSelect);control(window,L"BUTTON",L"Change Part Note",WS_TABSTOP,582,412,155,28,StyleNoteSet);styleNoteInfo=control(window,L"STATIC",L"",0,582,444,170,65);
            control(window,L"BUTTON",L"Clone Note",WS_TABSTOP,582,444,74,28,StyleNoteInsert);control(window,L"BUTTON",L"Delete Note",WS_TABSTOP,666,444,74,28,StyleNoteDelete);MoveWindow(styleNoteInfo,582,480,170,65,FALSE);
            partVariationLabel=control(window,L"STATIC",L"Part variation / chord choices",0,780,100,174,36);partVariationSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,780,138,174,240,StyleVariationSelect);for(unsigned i=1;i<=32;++i){const auto label=L"Variation "+std::to_wstring(i);SendMessageW(partVariationSelect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(partVariationSelect,CB_SETCURSEL,0,0);partVariationChoices=control(window,L"EDIT",L"4294967295",WS_BORDER|WS_TABSTOP,780,198,174,26);control(window,L"BUTTON",L"Set Chord Choices",WS_TABSTOP,780,238,174,28,StyleVariationSet);
            styleFieldsLabel=control(window,L"STATIC",L"Grid start / offset (clocks) / music value / variation mask",0,16,490,540,22);styleGrid=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,514,90,26);styleOffset=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,116,514,90,26);styleMusic=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,216,514,90,26);styleVariation=control(window,L"EDIT",L"4294967295",WS_BORDER|WS_TABSTOP,316,514,160,26);
            segmentBandSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,574,270,250,SegmentBandSelect);control(window,L"BUTTON",L"Copy Band at clocks",WS_TABSTOP,310,574,170,28,SegmentBandAssign);
            bandEventLabel=control(window,L"STATIC",L"Band source; below: event / logical clocks / physical clocks",0,16,550,740,22);bandEventSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,608,110,240,BandEventSelect);bandLogical=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,138,608,100,26);bandPhysical=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,248,608,100,26);control(window,L"BUTTON",L"Move Band",WS_TABSTOP,360,608,100,28,BandEventMove);control(window,L"BUTTON",L"Delete Band",WS_TABSTOP,470,608,100,28,BandEventDelete);
            control(window,L"BUTTON",L"Add GM Instrument",WS_TABSTOP,310,574,170,28,BandAdd);control(window,L"BUTTON",L"Assign DLS Collection",WS_TABSTOP,500,574,190,28,BandCollectionAssign);bandLabel=control(window,L"STATIC",L"Band instrument: patch (packed banks) / PChannel / pan / volume",0,16,550,740,22);bandSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,16,574,270,250,BandSelect);bandPatch=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,16,608,110,26);bandChannel=control(window,L"EDIT",L"0",WS_BORDER|WS_TABSTOP,138,608,110,26);bandPan=control(window,L"EDIT",L"64",WS_BORDER|WS_TABSTOP,260,608,65,26);bandVolume=control(window,L"EDIT",L"100",WS_BORDER|WS_TABSTOP,336,608,65,26);control(window,L"BUTTON",L"Set Band Instrument",WS_TABSTOP,420,608,170,28,BandSet);
            groupLabel=control(window,L"STATIC",L"Track groups (1-32, comma separated)",0,780,100,174,36);groupEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,780,138,170,25,GroupMask);
            groupTempoLabel=control(window,L"STATIC",L"Tempo track (1-based)",0,780,174,174,22);groupTempoEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,780,198,170,25,GroupTempo);
            groupMeterLabel=control(window,L"STATIC",L"Time signature track",0,780,234,174,22);groupMeterEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,780,258,170,25,GroupMeter);groupSequenceLabel=control(window,L"STATIC",L"Sequence track (1-based)",0,780,294,174,22);groupSequenceEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,780,318,170,25,GroupSequence);
            groupBandLabel=control(window,L"STATIC",L"Band track (1-based)",0,780,354,174,22);groupBandEdit=control(window,L"EDIT",L"1",WS_BORDER|WS_TABSTOP,780,378,170,25,GroupBand);control(window,L"BUTTON",L"Apply Track Selection",WS_TABSTOP,780,414,174,28,GroupApply);
            control(window,L"BUTTON",L"Edit Commands...",WS_TABSTOP,780,458,174,28,CommandEditor);
            control(window,L"BUTTON",L"Edit Chords...",WS_TABSTOP,780,492,174,28,ChordEditor);
            control(window,L"BUTTON",L"Edit SignPosts...",WS_TABSTOP,780,526,174,28,SignpostEditor);
            patternNameLabel=control(window,L"STATIC",L"Pattern name / type",0,780,500,174,22);patternNameEdit=control(window,L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,780,524,174,25);SendMessageW(patternNameEdit,EM_SETLIMITTEXT,255,0);
            patternKindSelect=control(window,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,780,560,174,180);control(window,L"BUTTON",L"Set Pattern Properties",WS_TABSTOP,780,600,174,28,PatternProperties);
            status=control(window,L"EDIT",L"",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_TABSTOP,16,650,940,65);
            // A project loaded at startup already owns its documents. Do not
            // add an unsaved placeholder to the restored project.
            if(framework.project_path().empty())active=framework.new_segment();
            refresh(window);return 0;
        }
        case WM_INITMENUPOPUP:
            if(reinterpret_cast<HMENU>(w)==transportMenu){
                while(GetMenuItemCount(transportMenu)>0)DeleteMenu(transportMenu,0,MF_BYPOSITION);
                AppendMenuW(transportMenu,MF_STRING|(conductor.default_audio_path().empty()?MF_CHECKED:0),TransportPath,L"Standard stereo (next playback)");
                for(size_t i=0;i<framework.audio_paths().size()&&i<999;++i){const auto& path=framework.audio_path_document(i);const auto label=path.name()+L" (next playback copy)";AppendMenuW(transportMenu,MF_STRING|(path.save_bytes()==conductor.default_audio_path()?MF_CHECKED:0),TransportPath+static_cast<UINT>(i)+1,label.c_str());}
            }return 0;
        case WM_COMMAND:command(window,LOWORD(w),HIWORD(w));return 0;
        case PlaybackStoppedMessage:
            playbackStatus=conductor.current_playback_id()?L"Stopped selected playback; other playback retained":L"Stopped";
            if(!conductor.current_playback_id()){KillTimer(window,1);playbackMonitor.clear();}refresh_status();return 0;
        case WM_TIMER: {
            if(w==1){
                collect_lyric_messages();
                const auto ownedIds=conductor.playback_ids();
                for(const auto& event:conductor.notifications()){
                    if(IsEqualGUID(event.type,producer::runtime::segmentNotification)&&(event.option==producer::runtime::segmentStarted||event.option==producer::runtime::segmentEnded)&&std::find(ownedIds.begin(),ownedIds.end(),event.playbackId)!=ownedIds.end())playbackMonitor.observed_start(event.playbackId);
                    if(event.currentSegment&&event.option<=1&&IsEqualGUID(event.type,producer::runtime::commandNotification)){
                        playbackStatus=event.option==0?L"Playing — Groove changed":L"Playing — embellishment changed";refresh_status();
                    }
                }
                if(const auto activeId=conductor.current_playback_id()){
                    const auto calls=conductor.track_script_calls(activeId);
                    if(!calls.empty()){
                        const auto& last=calls.back();
                        auto status=last.result.passed()?L"Playing — Script routine: "+last.routine:L"Script routine failed: "+last.routine;
                        if(!last.result.passed()){
                            const auto& detail=last.result.error.description;
                            size_t count=0;while(count<std::size(detail)&&detail[count])++count;
                            if(count)status+=L" — "+std::wstring(detail,count);
                        }
                        if(playbackStatus!=status){playbackStatus=std::move(status);refresh_status();}
                    }
                }
                std::vector<PlaybackMonitorSample> samples;
                for(const auto& session:conductor.playback_sessions())samples.push_back({session.id,session.position.playing,session.position.clocks,session.position.start});
                const auto completed=playbackMonitor.update(samples,GetTickCount64());
                for(const auto& event:completed){conductor.stop(event.id);playbackMonitor.forget(event.id);}
                if(!conductor.current_playback_id()){KillTimer(window,1);playbackMonitor.clear();}
                if(!completed.empty()){const auto timedOut=std::any_of(completed.begin(),completed.end(),[](const auto& event){return event.completion==PlaybackCompletion::StartTimedOut;});playbackStatus=timedOut?L"Playback did not start within five seconds after its scheduled time":L"Stopped (segment ended)";if(conductor.current_playback_id())playbackStatus+=L"; other playback retained";refresh_status();}
            }return 0;
        }
        case WM_PAINT: {
            struct Paint { HWND window;PAINTSTRUCT state{};HDC dc;explicit Paint(HWND w):window(w),dc(BeginPaint(w,&state)){}~Paint(){EndPaint(window,&state);} } paint(window);
            HDC dc=paint.dc;SetBkMode(dc,TRANSPARENT);
            if(!styleMode&&!bandMode&&!framework.documents().empty()) {
                const auto& d=document();const double zoom=520.0/std::max(1,d.length());const Timeline t; // Raw clock axis; pixel conversion does not depend on meter.
                MoveToEx(dc,20,440,nullptr);LineTo(dc,550,440);
                const auto tempos=d.tempos();
                for(size_t i=0;i<tempos.size();++i) {
                    const auto& e=tempos[i];const int x=20+t.pixel(e.time,zoom,0);
                    MoveToEx(dc,x,410,nullptr);LineTo(dc,x,460);
                    auto text=std::to_wstring(e.bpm);
                    while(text.size()>1&&text.back()==L'0')text.pop_back();
                    if(text.back()==L'.')text.pop_back();
                    // Keep labels inside their clock interval. Dense events
                    // remain available in the complete Tempo list above.
                    const int end=i+1<tempos.size()?20+t.pixel(tempos[i+1].time,zoom,0):550;
                    RECT label{std::max(20,x),465,std::min(550,end-4),485};
                    if(label.right>label.left)DrawTextW(dc,text.data(),static_cast<int>(text.size()),&label,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
                }
            }
            return 0;
        }
        case WM_CLOSE:if(allow_discard(window))DestroyWindow(window);return 0;
        case WM_DESTROY:conductor.shutdown();framework.new_project();PostQuitMessage(0);return 0;
        }
    } catch(const std::exception& e) {
        // MessageBox dispatches messages while modal. Suspend a failed monitor
        // before reporting it so subsequent timer ticks cannot open nested
        // error dialogs. Explicit Stop remains available for owned playback.
        if(message==WM_TIMER){KillTimer(window,1);playbackMonitor.clear();playbackStatus=L"Playback monitor failed; Stop remains available";refresh_status();}
        MessageBoxA(window,e.what(),"Producer operation failed",MB_OK|MB_ICONERROR);if(message==WM_CREATE)return -1;
    }
    return DefWindowProcW(window,message,w,l);
}
std::string module_report() {
    const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());if(snapshot==INVALID_HANDLE_VALUE)throw std::runtime_error("Module inventory failed");
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);std::string result="";
    BOOL more=Module32FirstW(snapshot,&entry);if(!more){CloseHandle(snapshot);throw std::runtime_error("Module inventory empty");}
    while(more) {const auto path=utf16(entry.szExePath); // Hex UTF-16 avoids JSON escaping/encoding ambiguity.
        const char digits[]="0123456789abcdef";std::string hex;for(auto b:path){hex.push_back(digits[b>>4]);hex.push_back(digits[b&15]);}if(!result.empty())result+=",";result+="\""+hex+"\"";more=Module32NextW(snapshot,&entry);
    }
    CloseHandle(snapshot);return result;
}
int smoke(const std::wstring& directory) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework f;const auto i=f.new_segment();if(!f.document(i).add_tempo(768,137))throw std::runtime_error("Smoke edit failed");
    const auto segment=(base/L"smoke.sgp").wstring(),project=(base/L"smoke.dmpj").wstring();f.save_segment(i,segment);const auto saved=f.document(i).save_bytes();f.save_project(project);f.new_project();f.open_project(project);
    if(f.documents().size()!=1||f.document(0).save_bytes()!=saved)throw std::runtime_error("Smoke reload failed");
    const auto report="{\"scope\":\"same-process source-only framework smoke; UI, separate-process reload and audio untested\",\"passed\":true,\"modulePathsUtf16Hex\":["+module_report()+"]}\n";
    write_file_atomic((base/L"smoke.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
bool filename_style_playback(Conductor& player,const SegmentDocument& source,const std::filesystem::path& base,const std::filesystem::path& runtimeDirectory){
    const auto waitPlaying=[&player](){for(unsigned attempt=0;attempt<40;++attempt){if(player.position().playing)return true;Sleep(50);}return false;};
    const auto references=source.style_references();if(references.size()!=1||references[0].filename.empty())throw std::runtime_error("Filename case requires one named Style");
    auto root=Chunk::parse(source.save_bytes());for(auto& track:root.find("LIST","trkl")->children)if(auto list=track.find("LIST","sttr"))for(auto& ref:list->children)if(ref.id=="LIST"&&ref.type=="strf"){
        auto descriptor=ref.find("LIST","DMRF");auto header=descriptor->find("refh");put32(header->data,16,read32(header->data,16)&~1u);auto& children=descriptor->children;children.erase(std::remove_if(children.begin(),children.end(),[](const Chunk& c){return c.id=="guid";}),children.end());
    }
    const auto owned=base/L"filename-case"/L"owned",moved=base/L"filename-case"/L"moved";std::filesystem::create_directories(owned);std::filesystem::create_directories(moved);
    const auto relative=std::filesystem::path(references[0].filename);const auto stylePath=owned/relative,movedStyle=moved/relative;std::filesystem::create_directories(stylePath.parent_path());std::filesystem::create_directories(movedStyle.parent_path());
    write_file_atomic((owned/L"filename.sgp").wstring(),root.encode());write_file_atomic(stylePath.wstring(),source.styles()[0].bytes);Framework project;const auto si=project.open_style(stylePath.wstring());const auto di=project.open_segment((owned/L"filename.sgp").wstring());
    if(!project.set_style_meter(si,3,4,2)||!project.set_style_tempo(si,132)||!project.set_pattern_groove(si,0,5,50)||!project.set_pattern_layout(si,0,3,4,2,2))throw std::runtime_error("Edited filename Style fixture rejected");project.save_style(si,stylePath.wstring());project.save_project((owned/L"project.dmpj").wstring());
    const auto partIndex=project.style_document(si).part_references(0).at(0).partIndex;if(!project.set_style_part_note(si,partIndex,0,576,88))throw std::runtime_error("Part note fixture edit rejected");const auto noteProof="{\"partIndex\":"+std::to_string(partIndex)+",\"noteIndex\":0,\"duration\":576,\"velocity\":88}\n";write_file_atomic((base/L"edited-note.json").wstring(),Bytes(noteProof.begin(),noteProof.end()));project.save_style(si,stylePath.wstring());
    // Keep filename-only dependencies contained throughout the move. Stage the
    // Segment in the common parent before moving its owned Style to a sibling.
    project.save_segment(di,(base/L"filename-case"/L"staging.sgp").wstring());project.save_style(si,movedStyle.wstring());project.save_segment(di,(moved/L"filename.sgp").wstring());project.save_project((moved/L"project.dmpj").wstring());Framework reloaded;reloaded.open_project((moved/L"project.dmpj").wstring());const auto& document=reloaded.document(0);
    write_file_atomic((base/L"filename-source.sgp").wstring(),document.save_bytes());write_file_atomic((base/L"filename-owned-style.stp").wstring(),document.styles()[0].bytes);player.play(document.save_bytes(),runtimeDirectory.wstring(),GetDesktopWindow(),document.styles());
    write_file_atomic((base/L"filename-runtime.sgp").wstring(),player.playback_bytes());write_file_atomic((base/L"filename-runtime-style.stp").wstring(),player.playback_styles()[0].bytes);const auto meter=player.segment_meter(0);const bool mapped=meter.beats==3&&meter.denominator==4&&meter.grids==2&&waitPlaying();player.stop();
    auto missing=document.styles();auto styleRoot=Chunk::parse(missing[0].bytes);auto& chunks=styleRoot.children;chunks.erase(std::remove_if(chunks.begin(),chunks.end(),[](const Chunk& c){return c.id=="guid";}),chunks.end());missing[0].bytes=styleRoot.encode();write_file_atomic((base/L"missing-guid-source.stp").wstring(),missing[0].bytes);
    player.play(document.save_bytes(),runtimeDirectory.wstring(),GetDesktopWindow(),missing);write_file_atomic((base/L"generated-runtime.sgp").wstring(),player.playback_bytes());write_file_atomic((base/L"generated-runtime-style.stp").wstring(),player.playback_styles()[0].bytes);const auto generatedMeter=player.segment_meter(0);const bool generated=generatedMeter.beats==3&&generatedMeter.denominator==4&&generatedMeter.grids==2&&waitPlaying();player.stop();return mapped&&generated;
}
int style_playback_smoke(const std::wstring& directory,const std::wstring& file,const std::wstring& styleDirectory) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    const auto runtimeDirectory=base/L"runtime-empty";std::filesystem::create_directory(runtimeDirectory);
    Conductor player;bool played=false,stopped=false,meterMatched=false,rejectedContext=false,alteredMemoryMeter=false,filenameMapped=false;std::string error,modules,samples;unsigned styleCount=0;
    try {
        SegmentDocument document;document.load(read_file(file));document.resolve_style_context(styleDirectory);styleCount=static_cast<unsigned>(document.styles().size());if(!styleCount)throw std::runtime_error("Style playback test requires references");
        write_file_atomic((base/L"input.sgp").wstring(),document.save_bytes());for(size_t i=0;i<document.styles().size();++i)write_file_atomic((base/(L"style-"+std::to_wstring(i)+L".stp")).wstring(),document.styles()[i].bytes);
        player.play(document.save_bytes(),runtimeDirectory.wstring(),GetDesktopWindow(),document.styles());modules=module_report();
        write_file_atomic((base/L"runtime-input.sgp").wstring(),player.playback_bytes());write_file_atomic((base/L"runtime-style-0.stp").wstring(),player.playback_styles()[0].bytes);
        const auto meter=player.segment_meter(0);const auto expected=document.timeline().meters().front();meterMatched=meter.beats==expected.beats&&meter.denominator==expected.denominator&&meter.grids==expected.grids;
        for(unsigned i=0;i<12;++i){Sleep(100);const auto position=player.position();played=played||position.playing;if(!samples.empty())samples+=",";samples+="{\"playing\":"+std::string(position.playing?"true":"false")+",\"tempo\":"+std::to_string(position.tempo)+"}";}
        // Unresolved replacement is rejected before stopping the live document.
        try{player.play(document.save_bytes(),runtimeDirectory.wstring(),GetDesktopWindow());}catch(const std::exception&){rejectedContext=player.position().playing;}
        player.stop();stopped=!player.position().playing;
        // Distinguish memory-backed loading from reopening the unchanged 4/4
        // file: keep GUID/filename, change only the owned header to 3/4 grids2.
        if(document.styles().size()!=1)throw std::runtime_error("Memory meter probe currently requires one Style reference");
        auto changed=document.styles();StyleDocument editedStyle;editedStyle.load(changed[0].bytes);if(!editedStyle.set_meter(3,4,2))throw std::runtime_error("Style meter edit failed");changed[0].bytes=editedStyle.save_bytes();changed[0].meter=editedStyle.meter();
        write_file_atomic((base/L"memory-meter-probe.stp").wstring(),changed[0].bytes);
        player.play(document.save_bytes(),runtimeDirectory.wstring(),GetDesktopWindow(),changed);const auto memoryMeter=player.segment_meter(0);alteredMemoryMeter=memoryMeter.beats==3&&memoryMeter.denominator==4&&memoryMeter.grids==2;Sleep(100);player.stop();filenameMapped=filename_style_playback(player,document,base,runtimeDirectory);player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string calls;bool callsPassed=true;for(const auto& call:player.calls()){callsPassed=callsPassed&&SUCCEEDED(call.result);if(!calls.empty())calls+=",";calls+="{\"operation\":\""+call.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(call.result))+"}";}
    const bool passed=error.empty()&&played&&stopped&&meterMatched&&rejectedContext&&alteredMemoryMeter&&filenameMapped&&callsPassed;
    const auto report="{\"scope\":\"owned Style snapshot, empty Style search directory, runtime header/meter/play/stop; audio unverified\",\"passed\":"+std::string(passed?"true":"false")+",\"styleCount\":"+std::to_string(styleCount)+",\"played\":"+(played?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"meterMatched\":"+(meterMatched?"true":"false")+",\"alteredMemoryMeter3_4\":"+(alteredMemoryMeter?"true":"false")+",\"unresolvedReplacementRetainedPlayback\":"+(rejectedContext?"true":"false")+",\"error\":\""+error+"\",\"calls\":["+calls+"],\"samples\":["+samples+"],\"modulePathsUtf16Hex\":["+modules+"],\"modulesAfterCleanupUtf16Hex\":["+module_report()+"]}\n";
    const auto finalReport=report.substr(0,report.size()-2)+",\"editedRelocatedFilenameAndMissingGuidPassed\":"+(filenameMapped?"true":"false")+"}\n";
    write_file_atomic((base/L"playback.json").wstring(),Bytes(finalReport.begin(),finalReport.end()));return passed?0:1;
}
int group_playback_smoke(const std::wstring& directory,const std::wstring& input) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    const auto bytes=read_file(input);const auto root=Chunk::parse(bytes);const auto tracks=root.find("LIST","trkl");
    if(!tracks)throw std::runtime_error("Group playback input has no track list");
    write_file_atomic((base/L"input.sgp").wstring(),bytes);
    Conductor player;std::string queries,parameters,error,modules;bool exact=false,sourceExact=false,started=false,stopped=false,orderMatched=true;unsigned count=0;
    try {
        player.play(bytes,L"",GetDesktopWindow());exact=player.playback_bytes()==prepare_command_playback(bytes);sourceExact=player.playback_bytes()==bytes;
        write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());modules=module_report();
        // Type index is relative to the intersecting group mask, not the
        // absolute RIFF position. Shared masks contribute one track each.
        for(const DWORD mask:{0xffffffffu,1u,2u,3u,4u,8u}) {
            std::vector<Bytes> types;
            for(const auto& track:tracks->children){const auto header=track.find("trkh");if(!header||header->data.size()<32)throw std::runtime_error("Invalid group playback trkh");
                Bytes type(header->data.begin(),header->data.begin()+16);if(std::find(types.begin(),types.end(),type)==types.end())types.push_back(type);}
            for(const auto& type:types){DWORD index=0;GUID clsid{};std::memcpy(&clsid,type.data(),16);std::vector<DWORD> expectedGroups,actualGroups;
                for(const auto& track:tracks->children){const auto& header=track.find("trkh")->data;const auto expected=read32(header,20);
                    if(!(expected&mask)||!std::equal(type.begin(),type.end(),header.begin()))continue;
                    const auto actual=player.segment_track_group(clsid,mask,index);
                    if(const auto data=track.find("tetr")){producer::tempo::Track parsed;if(parsed.load(data->encode())!=producer::tempo::LoadResult::ok)throw std::runtime_error("Tempo parameter input unsupported");
                        for(const auto& event:parsed.events()){const auto tempo=player.segment_tempo(event.time,mask,index);if(!parameters.empty())parameters+=",";parameters+="{\"kind\":\"tempo\",\"mask\":"+std::to_string(mask)+",\"index\":"+std::to_string(index)+",\"time\":"+std::to_string(event.time)+",\"expected\":"+std::to_string(event.bpm)+",\"actual\":"+std::to_string(tempo)+"}";if(tempo!=event.bpm)throw std::runtime_error("Loaded selected Tempo differs from source");}}
                    if(const auto data=track.find("cmnd")){for(const auto& event:command_events(data->data)){const auto command=player.segment_command(event.time,mask,index);if(!parameters.empty())parameters+=",";
                        parameters+="{\"kind\":\"command\",\"mask\":"+std::to_string(mask)+",\"index\":"+std::to_string(index)+",\"time\":"+std::to_string(event.time)+",\"type\":"+std::to_string(event.type)+",\"groove\":"+std::to_string(event.groove)+",\"range\":"+std::to_string(event.range)+",\"repeat\":"+std::to_string(event.repeat)+",\"actual\":{\"type\":"+std::to_string(command.type)+",\"groove\":"+std::to_string(command.groove)+",\"range\":"+std::to_string(command.range)+",\"repeat\":"+std::to_string(command.repeat)+"}}";
                        if(command.type!=event.type||command.groove!=event.groove||command.range!=event.range||command.repeat!=event.repeat)throw std::runtime_error("Loaded selected Command differs from source");}}
                    const auto meterList=track.find("LIST","TIMS");const auto meterData=track.find("tims")?track.find("tims"):(meterList?meterList->find("tims"):nullptr);
                    if(meterData){Timeline timeline;timeline.load_meter(meterData->encode());for(const auto& event:timeline.meters()){const auto time=timeline.clocks({event.measure,0,0});const auto meter=player.segment_meter(time,mask,index);if(!parameters.empty())parameters+=",";parameters+="{\"kind\":\"meter\",\"mask\":"+std::to_string(mask)+",\"index\":"+std::to_string(index)+",\"time\":"+std::to_string(time)+",\"beats\":"+std::to_string(meter.beats)+",\"denominator\":"+std::to_string(meter.denominator)+",\"grids\":"+std::to_string(meter.grids)+"}";if(meter.beats!=event.beats||meter.denominator!=event.denominator||meter.grids!=event.grids)throw std::runtime_error("Loaded selected meter differs from source");}}
                    if(!queries.empty())queries+=",";queries+="{\"mask\":"+std::to_string(mask)+",\"index\":"+std::to_string(index)+",\"expected\":"+std::to_string(expected)+",\"actual\":"+std::to_string(actual)+"}";++count;++index;
                    expectedGroups.push_back(expected);actualGroups.push_back(actual);orderMatched=orderMatched&&actual==expected;
                }
                std::sort(expectedGroups.begin(),expectedGroups.end());std::sort(actualGroups.begin(),actualGroups.end());
                if(actualGroups!=expectedGroups)throw std::runtime_error("Loaded type and mask group inventory differs from serialized Segment");
            }
        }
        for(unsigned i=0;i<20&&!started;++i){Sleep(50);started=player.position().playing;}
        player.stop();stopped=!player.position().playing;player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string calls;bool callsPassed=true;for(const auto& call:player.calls()){callsPassed=callsPassed&&SUCCEEDED(call.result);if(!calls.empty())calls+=",";calls+="{\"operation\":\""+call.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(call.result))+"}";}
    const bool passed=error.empty()&&exact&&started&&stopped&&count&&callsPassed;
    const auto json="{\"scope\":\"whole Segment snapshot, type-filtered runtime track groups; original Producer and audio unverified\",\"passed\":"+std::string(passed?"true":"false")+",\"snapshotExact\":"+(exact?"true":"false")+",\"sourceSnapshotExact\":"+(sourceExact?"true":"false")+",\"started\":"+(started?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"queryCount\":"+std::to_string(count)+",\"error\":\""+error+"\",\"queries\":["+queries+"],\"calls\":["+calls+"],\"modulePathsUtf16Hex\":["+modules+"],\"modulesAfterCleanupUtf16Hex\":["+module_report()+"]}\n";
    const auto finalJson=json.substr(0,json.size()-2)+",\"runtimeOrderMatchesRiff\":"+(orderMatched?"true":"false")+",\"parameters\":["+parameters+"]}\n";
    write_file_atomic((base/L"playback.json").wstring(),Bytes(finalJson.begin(),finalJson.end()));return passed?0:1;
}
int style_player_audio_run(const std::wstring& directory,const std::wstring& stylePath,const std::wstring& chordMapPath);
int style_player_audio(const std::wstring& directory,const std::wstring& stylePath,const std::wstring& chordMapPath){
    try{return style_player_audio_run(directory,stylePath,chordMapPath);}catch(const std::exception& e){const std::string error=e.what();write_file_atomic((std::filesystem::absolute(directory)/L"failure.txt").wstring(),Bytes(error.begin(),error.end()));return 1;}
}
int style_player_audio_run(const std::wstring& directory,const std::wstring& stylePath,const std::wstring& chordMapPath){
    // Bounded StylePlayer.txt contract run: Play, Re-Compose, live Band change, Shape change, Motif layer, Stop, Play again.
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto si=host.open_style(stylePath);std::optional<Bytes> map;
    if(chordMapPath!=L"-"){const auto mi=host.open_chordmap(chordMapPath);map=host.chordmap_document(mi).save_bytes();}
    Conductor player;player.enable_note_observation();std::string error,phases,modules;bool passed=false;
    StylePlayerSession session(player,GetDesktopWindow(),host.style_playback_snapshot(si),host.style_playback_collections(si),map);
    auto stamp=[&](const char* label){LARGE_INTEGER before,after,frequency;QueryPerformanceCounter(&before);const auto id=session.primary_id();const auto position=id?player.position(id):PlaybackPosition{};QueryPerformanceCounter(&after);QueryPerformanceFrequency(&frequency);const auto qpc=static_cast<unsigned long long>((static_cast<double>(before.QuadPart)+static_cast<double>(after.QuadPart))*5000000.0/static_cast<double>(frequency.QuadPart));if(!phases.empty())phases+=",";phases+="{\"phase\":\""+std::string(label)+"\",\"qpc100ns\":"+std::to_string(qpc)+",\"clocks\":"+std::to_string(position.clocks)+",\"start\":"+std::to_string(position.start)+",\"tempo\":"+std::to_string(position.tempo)+",\"primaryId\":"+std::to_string(id)+",\"playing\":"+(position.playing?"true":"false")+"}";};
    auto ready=[&](const char* label){const auto deadline=GetTickCount64()+5000;while(!session.playing()){if(GetTickCount64()>=deadline)throw std::runtime_error("StylePlayer scheduled playback did not start");Sleep(10);}stamp(label);};
    auto sounding=[&]{const auto deadline=GetTickCount64()+1500;for(;;){const auto p=player.position(session.primary_id());const auto offset=(p.clocks-p.start)%768;if(p.playing&&offset>=60&&offset<=100)return;if(GetTickCount64()>=deadline)throw std::runtime_error("StylePlayer sounding Stop clock not reached");Sleep(2);}};
    write_file_atomic((base/L"source-style.stp").wstring(),host.style_document(si).save_bytes());if(map)write_file_atomic((base/L"source-map.cdm").wstring(),*map);const auto ownedCollections=host.style_playback_collections(si);for(size_t i=0;i<ownedCollections.size();++i)write_file_atomic((base/(L"source-collection-"+std::to_wstring(i)+L".dls")).wstring(),ownedCollections[i].bytes);
    std::string counters;auto count=[&](const char* label){if(!counters.empty())counters+=",";const auto id=session.primary_id();const auto position=id?player.position(id):PlaybackPosition{};counters+="{\"after\":\""+std::string(label)+"\",\"compositions\":"+std::to_string(session.compositions())+",\"restarts\":"+std::to_string(session.restarts())+",\"bandChanges\":"+std::to_string(session.band_changes())+",\"motifs\":"+std::to_string(session.motif_requests())+",\"primaryId\":"+std::to_string(id)+",\"primaryStart\":"+std::to_string(position.start)+",\"primaryClocks\":"+std::to_string(position.clocks)+",\"mappedPChannel\":"+std::to_string(id?player.convert_pchannel(id,5):0)+",\"sessions\":"+std::to_string(player.playback_ids().size())+",\"playing\":"+(session.playing()?"true":"false")+"}";if(const auto* c=session.composition()){const auto name=std::wstring(label,label+std::strlen(label))+L".sgp";write_file_atomic((base/name).wstring(),c->segment);}};
    try{
        const auto bands=session.band_names();const auto motifs=session.motif_names();
        stamp("play-request");session.play();modules=module_report();stamp("play-returned");ready("play-ready");count("play");Sleep(10000);
        stamp("recompose-request");session.recompose();stamp("recompose-returned");ready("recompose-ready");count("recompose");Sleep(3000);
        if(session.restarts()!=1)throw std::runtime_error("Re-Compose did not restart exactly once");
        if(!bands.empty()){stamp("band-request");session.select_band(bands.back());count("band");stamp("band-returned");Sleep(3000);if(session.restarts()!=1)throw std::runtime_error("Band change restarted playback");}
        auto settings=session.settings();settings.shape=StyleShape::Quiet;stamp("shape-request");session.set_settings(settings);stamp("shape-returned");ready("shape-ready");count("shape");Sleep(3000);if(session.restarts()!=2)throw std::runtime_error("Shape change did not restart");
        if(!motifs.empty()){stamp("motif-request");session.play_motif(motifs.front());count("motif");stamp("motif-returned");Sleep(3000);if(session.restarts()!=2)throw std::runtime_error("Motif restarted playback");}
        sounding();stamp("stop-request");session.stop();count("stop");stamp("stop-returned");Sleep(2500);
        stamp("replay-request");session.set_settings(session.settings());stamp("replay-returned");ready("replay-ready");count("replay");Sleep(3000);if(session.restarts()!=3)throw std::runtime_error("Stopped parameter change did not restart");
        sounding();stamp("final-stop-request");session.stop();count("final-stop");stamp("final-stop-returned");Sleep(2000);passed=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();
    }catch(const std::exception& e){error=e.what();}
    const auto notes=player.observed_notes();player.shutdown();std::string noteJson;for(const auto& n:notes){if(!noteJson.empty())noteJson+=",";noteJson+="{\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+"}";}
    const auto* c=session.composition();if(c)write_file_atomic((base/L"composed.sgp").wstring(),c->segment);
    const auto report="{\"passed\":"+std::string(passed&&!notes.empty()?"true":"false")+",\"error\":\""+error+"\",\"phases\":["+phases+"],\"counters\":["+counters+"],\"notes\":["+noteJson+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"style-player-audio.json").wstring(),Bytes(report.begin(),report.end()));return passed&&!notes.empty()?0:1;
}
int motif_concurrent_audio(const std::wstring& directory,const std::wstring& primaryInput,const std::wstring& secondaryInput,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto first=host.open_style(primaryInput),second=host.open_style(secondaryInput);Conductor player;player.enable_note_observation();
    std::string error,phases,notificationEvents,modules;PlaybackId a=0,b=0,c=0;bool passed=false;
    auto stamp=[&](const char* label){
        LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);
        const auto qpc=static_cast<unsigned long long>(static_cast<double>(counter.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));
        std::string positions;for(const auto id:player.playback_ids()){
            const auto position=player.position(id);if(!positions.empty())positions+=",";
            positions+="{\"id\":"+std::to_string(id)+",\"playing\":"+(position.playing?"true":"false")+",\"clocks\":"+std::to_string(position.clocks)+",\"start\":"+std::to_string(position.start)+",\"tempoAvailable\":"+(position.tempoAvailable?"true":"false")+",\"tempo\":"+std::to_string(position.tempo)+"}";
        }
        if(!phases.empty())phases+=",";phases+="{\"phase\":\""+std::string(label)+"\",\"qpc100ns\":"+std::to_string(qpc)+",\"positions\":["+positions+"]}";
    };
    auto drain=[&](){for(const auto& n:player.notifications()){if(!notificationEvents.empty())notificationEvents+=",";notificationEvents+="{\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"clocks\":"+std::to_string(n.clocks)+"}";}};
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+5000;do{const auto p=player.position(id);drain();if(p.playing&&p.clocks>=p.start)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Concurrent audio start timed out");};
    auto snapshots=[&](size_t style,const wchar_t* prefix){const auto source=host.style_playback_snapshot(style);write_file_atomic((base/(std::wstring(prefix)+L"-source.stp")).wstring(),source.bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.stp")).wstring(),player.playback_styles().at(0).bytes);const auto collections=host.style_playback_collections(style);if(collections.size()!=1||player.playback_collections().size()!=1)throw std::runtime_error("Concurrent fixture needs exactly one owned collection");write_file_atomic((base/(std::wstring(prefix)+L"-source.dls")).wstring(),collections[0].bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.dls")).wstring(),player.playback_collections()[0].bytes);};
    try{
        stamp("primary-request");player.play_motif(host.style_playback_snapshot(first),name,GetDesktopWindow(),host.style_playback_collections(first));a=player.current_playback_id();wait(a);snapshots(first,L"primary");stamp("primary-ready");Sleep(2000);
        PlaybackOptions options;options.secondary=true;stamp("secondary-request");player.play_motif(host.style_playback_snapshot(second),name,GetDesktopWindow(),host.style_playback_collections(second),options);b=player.current_playback_id();wait(b);snapshots(second,L"secondary");if(!player.position(a).playing)throw std::runtime_error("Primary ended at secondary start");stamp("both-ready");modules=module_report();Sleep(2000);
        stamp("secondary-stop-request");player.stop(b);if(!player.position(a).playing)throw std::runtime_error("Secondary Stop disturbed primary");stamp("secondary-stop-return");Sleep(2000);drain();
        stamp("secondary-restart-request");player.play_motif(host.style_playback_snapshot(second),name,GetDesktopWindow(),host.style_playback_collections(second),options);c=player.current_playback_id();wait(c);if(!player.position(a).playing)throw std::runtime_error("Restart disturbed primary");stamp("both-restarted");Sleep(2000);
        stamp("primary-stop-request");player.stop(a);if(!player.position(c).playing)throw std::runtime_error("Primary Stop disturbed secondary");stamp("primary-stop-return");Sleep(2000);drain();
        stamp("all-stop-request");player.stop();if(!player.playback_ids().empty())throw std::runtime_error("All Stop retained instances");stamp("all-stop-return");Sleep(2000);drain();passed=true;
    }catch(const std::exception& e){error=e.what();}
    const auto notes=player.observed_notes();const auto observerGood=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.shutdown();stamp("shutdown-return");std::string noteJson;
    for(const auto& n:notes){if(!noteJson.empty())noteJson+=",";noteJson+="{\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+"}";}
    const auto report="{\"passed\":"+std::string(passed&&observerGood&&!notes.empty()?"true":"false")+",\"error\":\""+error+"\",\"primaryId\":"+std::to_string(a)+",\"secondaryId\":"+std::to_string(b)+",\"restartId\":"+std::to_string(c)+",\"observerGood\":"+(observerGood?"true":"false")+",\"phases\":["+phases+"],\"notes\":["+noteJson+"],\"notificationEvents\":["+notificationEvents+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";write_file_atomic((base/L"concurrent-audio.json").wstring(),Bytes(report.begin(),report.end()));return passed&&observerGood&&!notes.empty()?0:1;
}
int motif_primary_replacement_audio(const std::wstring& directory,const std::wstring& primaryInput,const std::wstring& secondaryInput,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto first=host.open_style(primaryInput),second=host.open_style(secondaryInput);Conductor player;player.enable_note_observation();
    std::string error,phases,notificationEvents,modules;PlaybackId a=0,b=0;bool passed=false;std::string samples;PlaybackRequest request{};LONG actualStart=0;
    auto stamp=[&](const char* label){LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);const auto qpc=static_cast<unsigned long long>(static_cast<double>(counter.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));if(!phases.empty())phases+=",";phases+="{\"phase\":\""+std::string(label)+"\",\"qpc100ns\":"+std::to_string(qpc)+"}";};
    auto drain=[&](){for(const auto& n:player.notifications()){if(!notificationEvents.empty())notificationEvents+=",";notificationEvents+="{\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"clocks\":"+std::to_string(n.clocks)+"}";}};
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+5000;do{const auto p=player.position(id);drain();if(p.playing&&p.clocks>=p.start)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Concurrent audio start timed out");};
    auto snapshots=[&](size_t style,const wchar_t* prefix){const auto source=host.style_playback_snapshot(style);write_file_atomic((base/(std::wstring(prefix)+L"-source.stp")).wstring(),source.bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.stp")).wstring(),player.playback_styles().at(0).bytes);const auto collections=host.style_playback_collections(style);if(collections.size()!=1||player.playback_collections().size()!=1)throw std::runtime_error("Concurrent fixture needs exactly one owned collection");write_file_atomic((base/(std::wstring(prefix)+L"-source.dls")).wstring(),collections[0].bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.dls")).wstring(),player.playback_collections()[0].bytes);};
    try{
        stamp("primary-request");player.play_motif(host.style_playback_snapshot(first),name,GetDesktopWindow(),host.style_playback_collections(first));a=player.current_playback_id();wait(a);snapshots(first,L"primary");stamp("primary-ready");Sleep(2000);
        PlaybackOptions options;options.delayClocks=6144;options.afterPrepareTime=false;
        stamp("replacement-request");player.play_motif(host.style_playback_snapshot(second),name,GetDesktopWindow(),host.style_playback_collections(second),options);b=player.current_playback_id();request=player.playback_request();snapshots(second,L"secondary");actualStart=player.position(b).start;stamp("replacement-scheduled");modules=module_report();
        const auto deadline=GetTickCount64()+10000;unsigned beforeCount=0;bool switched=false;
        do{
            const auto old=player.position(a),next=player.position(b);LARGE_INTEGER q,frequency;QueryPerformanceCounter(&q);QueryPerformanceFrequency(&frequency);const auto qpc=static_cast<unsigned long long>(static_cast<double>(q.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));
            if(!samples.empty())samples+=",";samples+="{\"qpc100ns\":"+std::to_string(qpc)+",\"clocks\":"+std::to_string(next.clocks)+",\"oldPlaying\":"+(old.playing?"true":"false")+",\"newPlaying\":"+(next.playing?"true":"false")+"}";
            drain();
            if(next.clocks<actualStart){if(!old.playing)throw std::runtime_error("Old primary stopped before replacement start");++beforeCount;}
            if(next.playing&&next.clocks>=actualStart&&!old.playing){switched=true;break;}
            Sleep(10);
        }while(GetTickCount64()<deadline);
        if(!switched||beforeCount<100)throw std::runtime_error("Primary replacement boundary not verified");
        stamp("replacement-ready");Sleep(2500);if(!player.position(b).playing)throw std::runtime_error("Replacement ended too early");
        stamp("all-stop-request");player.stop();if(!player.playback_ids().empty())throw std::runtime_error("All Stop retained instances");stamp("all-stop-return");Sleep(2000);drain();passed=true;
    }catch(const std::exception& e){error=e.what();}
    const auto notes=player.observed_notes();const auto observerGood=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.shutdown();stamp("shutdown-return");std::string noteJson;
    for(const auto& n:notes){if(!noteJson.empty())noteJson+=",";noteJson+="{\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+"}";}
    const auto report="{\"passed\":"+std::string(passed&&observerGood&&!notes.empty()?"true":"false")+",\"error\":\""+error+"\",\"primaryId\":"+std::to_string(a)+",\"secondaryId\":"+std::to_string(b)+",\"restartId\":"+std::to_string(0)+",\"observerGood\":"+(observerGood?"true":"false")+",\"phases\":["+phases+"],\"notes\":["+noteJson+"],\"notificationEvents\":["+notificationEvents+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";const auto finalReport=report.substr(0,report.size()-2)+",\"actualStart\":"+std::to_string(actualStart)+",\"submittedClocks\":"+std::to_string(request.submittedClocks)+",\"requestedClocks\":"+std::to_string(request.requestedClocks)+",\"flags\":"+std::to_string(request.flags)+",\"samples\":["+samples+"]}\n";write_file_atomic((base/L"primary-replacement.json").wstring(),Bytes(finalReport.begin(),finalReport.end()));return passed&&observerGood&&!notes.empty()?0:1;
}
int boundary_runtime_observe(const std::wstring& directory,const std::wstring& primaryInput,const std::wstring& secondaryInput,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto first=host.open_style(primaryInput),second=host.open_style(secondaryInput);Conductor player;
    std::string error,cases,modules;PlaybackId primary=0;bool passed=false;LONG primaryStart=0;
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+5000;do{if(player.position(id).playing)return;player.notifications();Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Boundary observation start timed out");};
    try{
        const auto source=host.style_playback_snapshot(first);write_file_atomic((base/L"primary-source.stp").wstring(),source.bytes);player.play_motif(source,name,GetDesktopWindow(),host.style_playback_collections(first));primary=player.current_playback_id();wait(primary);primaryStart=player.position(primary).start;
        auto saved=host.style_playback_snapshot(second);StyleDocument document;document.load(saved.bytes);size_t pi=document.patterns().size();for(size_t i=0;i<document.patterns().size();++i)if(document.patterns()[i].name==name)pi=i;
        auto settings=document.motif_settings(pi).value();settings.resolution=producer::runtime::playBeat;document.set_motif_settings(pi,settings);saved.bytes=document.save_bytes();write_file_atomic((base/L"secondary-source.stp").wstring(),saved.bytes);
        const PlaybackBoundary boundaries[]={PlaybackBoundary::Immediate,PlaybackBoundary::Grid,PlaybackBoundary::Beat,PlaybackBoundary::Measure,PlaybackBoundary::Stored};const LONG units[]={1,192,768,3072,768};
        for(unsigned i=0;i<5;++i){
            PlaybackOptions options;options.secondary=true;options.afterPrepareTime=false;options.delayClocks=1153;options.boundary=boundaries[i];player.play_motif(saved,name,GetDesktopWindow(),host.style_playback_collections(second),options);const auto id=player.current_playback_id();const auto request=player.playback_request();
            const auto delta=request.requestedClocks-primaryStart;const auto expected=primaryStart+((delta+units[i]-1)/units[i])*units[i];wait(id);const auto p=player.position(id);
            if(!cases.empty())cases+=",";cases+="{\"boundary\":"+std::to_string(i)+",\"flags\":"+std::to_string(request.flags)+",\"defaultResolution\":"+std::to_string(request.runtimeDefaultResolution)+",\"submitted\":"+std::to_string(request.submittedClocks)+",\"requested\":"+std::to_string(request.requestedClocks)+",\"actual\":"+std::to_string(request.actualStart)+",\"positionStart\":"+std::to_string(p.start)+",\"expected\":"+std::to_string(expected)+",\"unit\":"+std::to_string(units[i])+",\"secondaryId\":"+std::to_string(id)+",\"primaryPlaying\":"+(player.position(primary).playing?"true":"false")+"}";
            if(request.runtimeDefaultResolution!=producer::runtime::playBeat||request.actualStart!=p.start||p.start!=expected||!player.position(primary).playing)throw std::runtime_error("Runtime boundary does not match source or primary alignment");player.stop(id);
        }
        modules=module_report();player.stop();passed=player.playback_ids().empty();
    }catch(const std::exception& e){error=e.what();}player.shutdown();
    const auto report="{\"passed\":"+std::string(passed?"true":"false")+",\"error\":\""+error+"\",\"primaryStart\":"+std::to_string(primaryStart)+",\"cases\":["+cases+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";write_file_atomic((base/L"boundaries.json").wstring(),Bytes(report.begin(),report.end()));return passed?0:1;
}
int notification_identity_stress(const std::wstring& directory,const std::wstring& primaryInput,const std::wstring& secondaryInput,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto first=host.open_style(primaryInput),second=host.open_style(secondaryInput);Conductor player;std::vector<PlaybackId> retired;PlaybackId primary=0;std::string error,notificationJson,sampleJson,modules;bool passed=false;size_t identityCount=0,pendingCount=0;
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+3000;do{if(player.position(id).playing)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Identity stress start timed out");};
    try{
        player.play_motif(host.style_playback_snapshot(first),name,GetDesktopWindow(),host.style_playback_collections(first));primary=player.current_playback_id();wait(primary);LONG previous=player.position(primary).clocks;
        PlaybackOptions options;options.secondary=true;options.afterPrepareTime=false;
        for(unsigned i=0;i<32;++i){
            player.play_motif(host.style_playback_snapshot(second),name,GetDesktopWindow(),host.style_playback_collections(second),options);const auto id=player.current_playback_id();wait(id);Sleep(30);player.stop(id);retired.push_back(id);
            const auto p=player.position(primary);if(!p.playing||p.clocks<previous||player.playback_ids()!=std::vector<PlaybackId>{primary})throw std::runtime_error("Repeated secondary stop disturbed primary");previous=p.clocks;
            if(!sampleJson.empty())sampleJson+=",";sampleJson+="{\"iteration\":"+std::to_string(i)+",\"stoppedId\":"+std::to_string(id)+",\"primaryClocks\":"+std::to_string(p.clocks)+",\"primaryPlaying\":true}";
        }
        Sleep(200);pendingCount=player.pending_notification_count();const auto notifications=player.notifications();identityCount=player.notification_identity_count();
        for(const auto& n:notifications){if(!notificationJson.empty())notificationJson+=",";notificationJson+="{\"segment\":"+std::string(IsEqualGUID(n.type,producer::runtime::segmentNotification)?"true":"false")+",\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"currentSegment\":"+(n.currentSegment?"true":"false")+"}";}
        for(const auto id:retired){bool started=false,aborted=false;for(const auto& n:notifications)if(IsEqualGUID(n.type,producer::runtime::segmentNotification)&&n.playbackId==id){started=started||n.option==0;aborted=aborted||n.option==4;}if(!started||!aborted)throw std::runtime_error("Delayed retired identity attribution missing");}
        if(identityCount!=1||player.pending_notification_count())throw std::runtime_error("Terminal weak keys or public backlog retained");if(!player.position(primary).playing)throw std::runtime_error("Primary ended during delayed drain");modules=module_report();player.stop();passed=player.playback_ids().empty();
    }catch(const std::exception& e){error=e.what();}player.shutdown();
    const auto report="{\"identityCountAfterDrain\":"+std::to_string(identityCount)+",\"pendingBeforeDrain\":"+std::to_string(pendingCount)+",\"passed\":"+std::string(passed?"true":"false")+",\"error\":\""+error+"\",\"primaryId\":"+std::to_string(primary)+",\"iterations\":"+std::to_string(retired.size())+",\"samples\":["+sampleJson+"],\"notifications\":["+notificationJson+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";write_file_atomic((base/L"identity-stress.json").wstring(),Bytes(report.begin(),report.end()));return passed?0:1;
}
int short_playback_monitor(const std::wstring& directory){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);auto song=SegmentDocument::playback_test(60);
    while(!song.notes().empty())song.delete_note(0);song.select(1);if(!song.delete_selected())throw std::runtime_error("Short fixture tempo removal failed");if(!song.add_note({0,48,0,60,96}))throw std::runtime_error("Short fixture note failed");auto root=Chunk::parse(song.save_bytes());put32(root.find("segh")->data,4,96);song.load(root.encode());write_file_atomic((base/L"source.sgp").wstring(),song.save_bytes());
    Conductor player;player.enable_note_observation();PlaybackMonitor monitor;bool passed=false,started=false,ended=false;std::string error,notificationJson,modules;PlaybackId id=0;PlaybackPosition position{};size_t count=0;
    try{
        player.play(song.save_bytes(),base.wstring(),GetDesktopWindow());id=player.current_playback_id();write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());
        Sleep(2500);position=player.position(id);const auto owned=player.playback_ids();
        for(const auto& n:player.notifications()){if(!notificationJson.empty())notificationJson+=",";notificationJson+="{\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"segment\":"+(IsEqualGUID(n.type,producer::runtime::segmentNotification)?"true":"false")+"}";if(IsEqualGUID(n.type,producer::runtime::segmentNotification)&&n.playbackId==id){started=started||n.option==producer::runtime::segmentStarted;ended=ended||n.option==producer::runtime::segmentEnded;if((n.option==producer::runtime::segmentStarted||n.option==producer::runtime::segmentEnded)&&std::find(owned.begin(),owned.end(),n.playbackId)!=owned.end())monitor.observed_start(n.playbackId);}}
        const auto completed=monitor.update({{id,position.playing,position.clocks,position.start}},GetTickCount64());count=completed.size();modules=module_report();
        if(position.playing||position.clocks<position.start+96||!started||!ended||count!=1||completed[0].id!=id||completed[0].completion!=PlaybackCompletion::Ended)throw std::runtime_error("Unsampled short playback notification completion failed");
        player.stop(id);passed=player.playback_ids().empty();
    }catch(const std::exception& e){error=e.what();}
    const auto noteCount=player.observed_notes().size();const auto observerGood=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.shutdown();
    const auto report="{\"passed\":"+std::string(passed&&observerGood&&noteCount==1?"true":"false")+",\"error\":\""+error+"\",\"playbackId\":"+std::to_string(id)+",\"length\":96,\"playingSampleCount\":0,\"samplePlaying\":"+(position.playing?"true":"false")+",\"clocks\":"+std::to_string(position.clocks)+",\"start\":"+std::to_string(position.start)+",\"started\":"+(started?"true":"false")+",\"ended\":"+(ended?"true":"false")+",\"completionCount\":"+std::to_string(count)+",\"noteCount\":"+std::to_string(noteCount)+",\"observerGood\":"+(observerGood?"true":"false")+",\"notifications\":["+notificationJson+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"short-monitor.json").wstring(),Bytes(report.begin(),report.end()));return passed&&observerGood&&noteCount==1?0:1;
}
int motif_primary_cancel_audio(const std::wstring& directory,const std::wstring& primaryInput,const std::wstring& secondaryInput,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto first=host.open_style(primaryInput),second=host.open_style(secondaryInput);Conductor player;player.enable_note_observation();
    std::string error,phases,notificationEvents,modules;PlaybackId a=0,b=0;bool passed=false;std::string samples;PlaybackRequest request{};LONG actualStart=0;bool failedPreparationPreserved=false;unsigned long preparationFailure=0;
    auto stamp=[&](const char* label){LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);const auto qpc=static_cast<unsigned long long>(static_cast<double>(counter.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));if(!phases.empty())phases+=",";phases+="{\"phase\":\""+std::string(label)+"\",\"qpc100ns\":"+std::to_string(qpc)+"}";};
    auto drain=[&](){for(const auto& n:player.notifications()){if(!notificationEvents.empty())notificationEvents+=",";notificationEvents+="{\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"clocks\":"+std::to_string(n.clocks)+"}";}};
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+5000;do{const auto p=player.position(id);drain();if(p.playing&&p.clocks>=p.start)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Concurrent audio start timed out");};
    auto snapshots=[&](size_t style,const wchar_t* prefix){const auto source=host.style_playback_snapshot(style);write_file_atomic((base/(std::wstring(prefix)+L"-source.stp")).wstring(),source.bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.stp")).wstring(),player.playback_styles().at(0).bytes);const auto collections=host.style_playback_collections(style);if(collections.size()!=1||player.playback_collections().size()!=1)throw std::runtime_error("Concurrent fixture needs exactly one owned collection");write_file_atomic((base/(std::wstring(prefix)+L"-source.dls")).wstring(),collections[0].bytes);write_file_atomic((base/(std::wstring(prefix)+L"-runtime.dls")).wstring(),player.playback_collections()[0].bytes);};
    try{
        stamp("primary-request");player.play_motif(host.style_playback_snapshot(first),name,GetDesktopWindow(),host.style_playback_collections(first));a=player.current_playback_id();wait(a);snapshots(first,L"primary");stamp("primary-ready");Sleep(2000);
        PlaybackOptions options;options.delayClocks=6144;options.afterPrepareTime=false;
        stamp("replacement-request");player.play_motif(host.style_playback_snapshot(second),name,GetDesktopWindow(),host.style_playback_collections(second),options);b=player.current_playback_id();request=player.playback_request();snapshots(second,L"secondary");actualStart=player.position(b).start;stamp("replacement-scheduled");modules=module_report();
        Sleep(600);stamp("cancel-request");player.stop(b);stamp("cancel-return");
        if(player.current_playback_id()!=a||player.playback_ids()!=std::vector<PlaybackId>{a})throw std::runtime_error("Cancellation did not restore old ownership");
        const auto deadline=GetTickCount64()+10000;bool beyondBoundary=false;
        do{
            const auto old=player.position(a);LARGE_INTEGER q,frequency;QueryPerformanceCounter(&q);QueryPerformanceFrequency(&frequency);const auto qpc=static_cast<unsigned long long>(static_cast<double>(q.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));
            if(!samples.empty())samples+=",";samples+="{\"qpc100ns\":"+std::to_string(qpc)+",\"clocks\":"+std::to_string(old.clocks)+",\"oldPlaying\":"+(old.playing?"true":"false")+"}";
            drain();if(!old.playing)throw std::runtime_error("Old primary stopped after scheduled cancellation");if(old.clocks>=actualStart+3072){beyondBoundary=true;break;}Sleep(10);
        }while(GetTickCount64()<deadline);
        if(!beyondBoundary)throw std::runtime_error("Canceled start boundary not crossed");stamp("cancel-boundary-passed");
        auto invalid=host.style_playback_snapshot(second);StyleDocument invalidDocument;invalidDocument.load(invalid.bytes);if(!invalidDocument.set_motif_band_instrument(0,0,778,5,64,100))throw std::runtime_error("Bad instrument fixture unchanged");invalid.bytes=invalidDocument.save_bytes();write_file_atomic((base/L"invalid-preparation.stp").wstring(),invalid.bytes);
        const auto callBegin=player.calls().size();stamp("failure-request");bool rejected=false;try{player.play_motif(invalid,name,GetDesktopWindow(),host.style_playback_collections(second),options);}catch(const std::exception&){rejected=true;}
        for(size_t i=callBegin;i<player.calls().size();++i)if(player.calls()[i].operation=="Get assigned owned DLS instrument"&&FAILED(player.calls()[i].result))preparationFailure=static_cast<unsigned long>(player.calls()[i].result);
        failedPreparationPreserved=rejected&&preparationFailure&&player.current_playback_id()==a&&player.playback_ids()==std::vector<PlaybackId>{a}&&player.position(a).playing;if(!failedPreparationPreserved)throw std::runtime_error("Runtime preparation failure did not preserve old primary");stamp("failure-return");Sleep(2000);if(!player.position(a).playing)throw std::runtime_error("Old primary ended after failed preparation");
        stamp("all-stop-request");player.stop();if(!player.playback_ids().empty())throw std::runtime_error("All Stop retained instances");stamp("all-stop-return");Sleep(2000);drain();passed=true;
    }catch(const std::exception& e){error=e.what();}
    const auto notes=player.observed_notes();const auto observerGood=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.shutdown();stamp("shutdown-return");std::string noteJson;
    for(const auto& n:notes){if(!noteJson.empty())noteJson+=",";noteJson+="{\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+"}";}
    const auto report="{\"passed\":"+std::string(passed&&observerGood&&!notes.empty()?"true":"false")+",\"error\":\""+error+"\",\"primaryId\":"+std::to_string(a)+",\"secondaryId\":"+std::to_string(b)+",\"restartId\":"+std::to_string(0)+",\"observerGood\":"+(observerGood?"true":"false")+",\"phases\":["+phases+"],\"notes\":["+noteJson+"],\"notificationEvents\":["+notificationEvents+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";const auto finalReport=report.substr(0,report.size()-2)+",\"failedPreparationPreserved\":"+(failedPreparationPreserved?"true":"false")+",\"preparationFailure\":"+std::to_string(preparationFailure)+",\"actualStart\":"+std::to_string(actualStart)+",\"submittedClocks\":"+std::to_string(request.submittedClocks)+",\"requestedClocks\":"+std::to_string(request.requestedClocks)+",\"flags\":"+std::to_string(request.flags)+",\"samples\":["+samples+"]}\n";write_file_atomic((base/L"primary-cancel.json").wstring(),Bytes(finalReport.begin(),finalReport.end()));return passed&&observerGood&&!notes.empty()?0:1;
}
int motif_concurrent_observe(const std::wstring& directory,const std::wstring& input,const std::wstring& name){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework host;const auto index=host.open_style(input);auto primary=host.style_playback_snapshot(index);const auto secondary=primary;const auto collections=host.style_playback_collections(index);
    StyleDocument source;source.load(primary.bytes);size_t pattern=source.patterns().size();for(size_t i=0;i<source.patterns().size();++i)if(source.patterns()[i].name==name&&(source.patterns()[i].embellishment&16)){pattern=i;break;}
    auto settings=source.motif_settings(pattern).value();settings.repeats=8;source.set_motif_settings(pattern,settings);primary.bytes=source.save_bytes();
    write_file_atomic((base/L"primary.stp").wstring(),primary.bytes);write_file_atomic((base/L"secondary.stp").wstring(),secondary.bytes);
    Conductor player;player.enable_note_observation();std::string error,checkpoints,notificationEvents,modules;bool secondaryStopPreservesPrimary=false,primaryStopPreservesSecondary=false,finalEmpty=false;PlaybackId a=0,b=0,c=0;bool passed=false;
    auto checkpoint=[&](const char* label,PlaybackId first,PlaybackId second){const auto p=player.position(first),q=player.position(second);if(!checkpoints.empty())checkpoints+=",";checkpoints+="{\"label\":\""+std::string(label)+"\",\"primaryId\":"+std::to_string(first)+",\"secondaryId\":"+std::to_string(second)+",\"primaryPlaying\":"+(p.playing?"true":"false")+",\"secondaryPlaying\":"+(q.playing?"true":"false")+",\"primaryStart\":"+std::to_string(p.start)+",\"secondaryStart\":"+std::to_string(q.start)+",\"clocks\":"+std::to_string(p.clocks)+"}";if(!p.playing||!q.playing)throw std::runtime_error("Concurrent playback checkpoint failed");};
    auto wait=[&](PlaybackId id){const auto deadline=GetTickCount64()+5000;do{const auto p=player.position(id);if(p.playing&&p.clocks>=p.start)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Concurrent start timed out");};
    auto drain=[&](){for(const auto& n:player.notifications()){if(!notificationEvents.empty())notificationEvents+=",";notificationEvents+="{\"segment\":"+std::string(IsEqualGUID(n.type,producer::runtime::segmentNotification)?"true":"false")+",\"option\":"+std::to_string(n.option)+",\"playbackId\":"+std::to_string(n.playbackId)+",\"currentSegment\":"+(n.currentSegment?"true":"false")+",\"clocks\":"+std::to_string(n.clocks)+"}";}};
    try{
        player.play_motif(primary,name,GetDesktopWindow(),collections);a=player.current_playback_id();wait(a);
        PlaybackOptions options;options.secondary=true;player.play_motif(secondary,name,GetDesktopWindow(),collections,options);b=player.current_playback_id();wait(b);checkpoint("both-started",a,b);drain();modules=module_report();
        bool invalidRejected=false;try{player.play_motif(secondary,L"Missing owned Motif",GetDesktopWindow(),collections,options);}catch(const std::exception&){invalidRejected=true;}if(!invalidRejected)throw std::runtime_error("Invalid selection accepted");checkpoint("invalid-preserves-both",a,b);
        player.stop(b);Sleep(100);secondaryStopPreservesPrimary=player.position(a).playing&&player.playback_ids().size()==1;drain();if(!secondaryStopPreservesPrimary)throw std::runtime_error("Stopping secondary disturbed primary");
        player.play_motif(secondary,name,GetDesktopWindow(),collections,options);c=player.current_playback_id();wait(c);checkpoint("secondary-restarted",a,c);Sleep(100);drain();
        player.stop(a);Sleep(100);primaryStopPreservesSecondary=player.position(c).playing&&player.playback_ids().size()==1;drain();if(!primaryStopPreservesSecondary)throw std::runtime_error("Stopping primary disturbed secondary");
        player.stop(c);Sleep(100);drain();finalEmpty=player.playback_ids().empty()&&!player.position().playing;if(!finalEmpty)throw std::runtime_error("Final individual stop retained playback");
        if(a==b||a==c||b==c||!a||!b||!c)throw std::runtime_error("Playback ID reuse");passed=true;
    }catch(const std::exception& e){error=e.what();}
    const auto notes=player.observed_notes();const bool observerGood=!player.note_observation_overflow()&&!player.note_observation_forwarding_failed();player.shutdown();
    const auto report="{\"passed\":"+std::string(passed&&observerGood&&!notes.empty()?"true":"false")+",\"error\":\""+error+"\",\"primaryId\":"+std::to_string(a)+",\"secondaryId\":"+std::to_string(b)+",\"restartId\":"+std::to_string(c)+",\"observerGood\":"+(observerGood?"true":"false")+",\"secondaryStopPreservesPrimary\":"+(secondaryStopPreservesPrimary?"true":"false")+",\"primaryStopPreservesSecondary\":"+(primaryStopPreservesSecondary?"true":"false")+",\"finalEmpty\":"+(finalEmpty?"true":"false")+",\"noteCount\":"+std::to_string(notes.size())+",\"checkpoints\":["+checkpoints+"],\"notificationEvents\":["+notificationEvents+"],\"modulePathsUtf16Hex\":["+modules+"],\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"concurrent.json").wstring(),Bytes(report.begin(),report.end()));return passed&&observerGood&&!notes.empty()?0:1;
}
int note_observe(const std::wstring& directory,const std::wstring& input,const std::optional<MotifSelection>& motif={},bool standalone=false,const PlaybackOptions& options={},unsigned observationMs=12000) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework host;const auto index=standalone?host.open_style(input):host.open_segment(input);Conductor player;
    player.enable_note_observation();std::string error,modules,noteEvents,calls,channelMappings;bool started=false,ended=false,overflow=false,forwardingFailed=false;LONG start=0;
    std::vector<PlaybackNote> notes;
    const auto inputBytes=standalone?host.style_document(index).save_bytes():host.document(index).save_bytes();write_file_atomic((base/(standalone?L"input.stp":L"input.sgp")).wstring(),inputBytes);
    try {
        if(standalone){if(!motif)throw std::runtime_error("Standalone playback requires an explicit Motif");player.play_motif(host.style_playback_snapshot(index),motif->name,GetDesktopWindow(),host.style_playback_collections(index),options);}
        else player.play(inputBytes,std::filesystem::path(input).parent_path().wstring(),GetDesktopWindow(),host.document(index).styles(),host.playback_collections(index),motif,host.playback_waves(index),host.trigger_playback(index),host.playback_chordmaps(index));
        modules=module_report();if(!standalone)write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());else if(!player.playback_bytes().empty()||!host.documents().empty())throw std::runtime_error("Standalone Motif unexpectedly acquired a context Segment");
        const auto request=player.playback_request();const auto resolvedTempo=standalone&&!options.secondary?player.segment_tempo(0):0;const auto requestJson="{\"flags\":"+std::to_string(request.flags)+",\"submittedClocks\":"+std::to_string(request.submittedClocks)+",\"requestedClocks\":"+std::to_string(request.requestedClocks)+",\"delayClocks\":"+std::to_string(options.delayClocks)+",\"defaultResolution\":"+std::to_string(request.runtimeDefaultResolution)+",\"actualStart\":"+std::to_string(request.actualStart)+",\"standalonePrimaryTempo\":"+std::to_string(resolvedTempo)+"}\n";write_file_atomic((base/L"playback-request.json").wstring(),Bytes(requestJson.begin(),requestJson.end()));
        const auto sourceStyles=standalone?std::vector<StyleCatalogEntry>{host.style_playback_snapshot(index)}:std::vector<StyleCatalogEntry>{};
        const auto sourceCollections=standalone?host.style_playback_collections(index):host.playback_collections(index);
        if(sourceCollections.size()!=player.playback_collections().size())throw std::runtime_error("Collection evidence count mismatch");
        for(size_t i=0;i<sourceCollections.size();++i){write_file_atomic((base/(L"source-collection-"+std::to_wstring(i)+L".dls")).wstring(),sourceCollections[i].bytes);write_file_atomic((base/(L"runtime-collection-"+std::to_wstring(i)+L".dls")).wstring(),player.playback_collections()[i].bytes);}
        for(size_t i=0;i<player.playback_styles().size();++i){write_file_atomic((base/(L"source-style-"+std::to_wstring(i)+L".stp")).wstring(),standalone?sourceStyles.at(i).bytes:host.document(index).styles().at(i).bytes);write_file_atomic((base/(L"runtime-style-"+std::to_wstring(i)+L".stp")).wstring(),player.playback_styles()[i].bytes);}
        std::vector<DWORD> sourceChannels;if(!standalone)for(const auto& n:host.document(index).notes())sourceChannels.push_back(n.channel);
        for(const auto& snapshot:player.playback_styles()){StyleDocument source;source.load(snapshot.bytes);for(size_t i=0;i<source.patterns().size();++i)for(const auto& ref:source.part_references(i))if(ref.pchannel)sourceChannels.push_back(*ref.pchannel);}
        std::sort(sourceChannels.begin(),sourceChannels.end());sourceChannels.erase(std::unique(sourceChannels.begin(),sourceChannels.end()),sourceChannels.end());
        for(const auto local:sourceChannels){const auto mapped=player.performance_channel(local);if(!channelMappings.empty())channelMappings+=",";channelMappings+="{\"local\":"+std::to_string(local)+",\"performance\":"+(mapped?std::to_string(*mapped):"null")+"}";}
        for(unsigned i=0;i<(observationMs+99)/100;++i){Sleep(100);const auto p=player.position();if(p.playing){started=true;start=p.start;}
            for(const auto& n:player.notifications()){if(!noteEvents.empty())noteEvents+=",";noteEvents+="{\"command\":"+std::string(IsEqualGUID(n.type,producer::runtime::commandNotification)?"true":"false")+",\"option\":"+std::to_string(n.option)+",\"clocks\":"+std::to_string(n.clocks)+",\"currentSegment\":"+(n.currentSegment?"true":"false")+"}";}
            if(started&&!p.playing){ended=true;break;}
        }
        player.stop();notes=player.observed_notes();overflow=player.note_observation_overflow();forwardingFailed=player.note_observation_forwarding_failed();player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string noteJson;for(const auto& n:notes){if(!noteJson.empty())noteJson+=",";noteJson+="{\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"group\":"+std::to_string(n.group)+",\"musicValue\":"+std::to_string(n.musicValue)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+",\"flags\":"+std::to_string(n.flags)+",\"playMode\":"+std::to_string(n.playMode)+"}";}
    for(const auto& c:player.calls()){if(!calls.empty())calls+=",";calls+="{\"operation\":\""+c.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(c.result))+"}";}
    const bool passed=error.empty()&&started&&ended&&!overflow&&!forwardingFailed&&!notes.empty();
    const auto report="{\"scope\":\"Generated runtime note observation; Pattern attribution and audio require separate comparison\",\"passed\":"+std::string(passed?"true":"false")+",\"started\":"+(started?"true":"false")+",\"ended\":"+(ended?"true":"false")+",\"overflow\":"+(overflow?"true":"false")+",\"forwardingFailed\":"+(forwardingFailed?"true":"false")+",\"start\":"+std::to_string(start)+",\"error\":\""+error+"\",\"pchannelMappings\":["+channelMappings+"],\"notes\":["+noteJson+"],\"noteEvents\":["+noteEvents+"],\"calls\":["+calls+"],\"modulePathsUtf16Hex\":["+modules+"]}\n";
    write_file_atomic((base/L"notes.json").wstring(),Bytes(report.begin(),report.end()));return passed?0:1;
}
int audio_lifecycle(const std::wstring& directory,const std::wstring& input,const std::optional<MotifSelection>& motif={},unsigned playingMs=2000,bool ownedPath=false,bool transportDefault=false) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework host;size_t index=0;const auto extension=std::filesystem::path(input).extension().wstring();
    if(extension==L".pro"||extension==L".dmpj"){host.open_project(input);if(host.documents().size()!=1)throw std::runtime_error("Lifecycle Project requires exactly one Segment");write_file_atomic((base/L"input-project.pro").wstring(),read_file(input));}
    else index=host.open_segment(input);
    const auto& song=host.document(index);
    if(ownedPath&&(!motif||host.style_documents().size()!=1||host.audio_paths().size()!=1))throw std::runtime_error("Owned Motif lifecycle requires one Style and one AudioPath");
    const auto ownedCollections=ownedPath?host.style_playback_collections(0):host.playback_collections(index);
    const auto sourceStyles=ownedPath?std::vector<StyleCatalogEntry>{host.style_playback_snapshot(0)}:std::vector<StyleCatalogEntry>{};
    const auto root=Chunk::parse(song.save_bytes());const auto header=root.find("segh");
    if(!motif&&(!header||header->data.size()<8||read32(header->data,4)<49152))throw std::runtime_error("Audio lifecycle requires a long Segment (at least49152 clocks)");
    Conductor player;player.enable_note_observation();std::string error,phases,modules,calls,noteJson;bool first=false,firstStopped=false,restarted=false,finalStopped=false;LONG firstStart=0,restartStart=0;size_t firstCount=0;
    if(transportDefault){if(!ownedPath)throw std::runtime_error("Transport lifecycle requires owned input");player.set_default_audio_path(host.audio_path_document(0).save_bytes());}
    auto collect=[&](unsigned run,LONG start,size_t begin){const auto notes=player.observed_notes();for(size_t i=begin;i<notes.size();++i){const auto& n=notes[i];if(!noteJson.empty())noteJson+=",";noteJson+="{\"run\":"+std::to_string(run)+",\"start\":"+std::to_string(start)+",\"clocks\":"+std::to_string(n.clocks)+",\"duration\":"+std::to_string(n.duration)+",\"channel\":"+std::to_string(n.channel)+",\"midiValue\":"+std::to_string(n.midiValue)+",\"velocity\":"+std::to_string(n.velocity)+"}";}return notes.size();};
    auto stamp=[&](const char* label){LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);
        const auto qpc=static_cast<unsigned long long>(static_cast<double>(counter.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart));
        if(!phases.empty())phases+=",";phases+="{\"phase\":\""+std::string(label)+"\",\"qpc100ns\":"+std::to_string(qpc)+"}";};
    auto waitForStart=[&](){const auto deadline=GetTickCount64()+5000;do{const auto p=player.position();if(p.playing&&p.clocks>=p.start)return;Sleep(10);}while(GetTickCount64()<deadline);throw std::runtime_error("Prepared playback did not reach its start time");};
    write_file_atomic((base/L"input.sgp").wstring(),song.save_bytes());
    try {
        const auto inputDirectory=std::filesystem::path(input).parent_path().wstring();
        stamp("play-request");if(ownedPath)player.play_motif(sourceStyles.at(0),motif->name,GetDesktopWindow(),ownedCollections,{},transportDefault?Bytes{}:host.audio_path_document(0).save_bytes());else player.play(song.save_bytes(),inputDirectory,GetDesktopWindow(),song.styles(),host.playback_collections(index),motif,host.playback_waves(index),host.trigger_playback(index),host.playback_chordmaps(index));stamp("play-return");modules=module_report();
        write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());
        if(ownedPath){write_file_atomic((base/L"source-style-0.stp").wstring(),sourceStyles.at(0).bytes);write_file_atomic((base/L"source.aud").wstring(),host.audio_path_document(0).save_bytes());}
        else for(size_t i=0;i<song.styles().size();++i)write_file_atomic((base/(L"source-style-"+std::to_wstring(i)+L".stp")).wstring(),song.styles()[i].bytes);
        for(size_t i=0;i<player.playback_styles().size();++i)write_file_atomic((base/(L"runtime-style-"+std::to_wstring(i)+L".stp")).wstring(),player.playback_styles()[i].bytes);
        if(ownedCollections.size()!=player.playback_collections().size())throw std::runtime_error("Lifecycle collection snapshot count differs");
        for(size_t i=0;i<ownedCollections.size();++i){write_file_atomic((base/(L"source-collection-"+std::to_wstring(i)+L".dls")).wstring(),ownedCollections[i].bytes);write_file_atomic((base/(L"runtime-collection-"+std::to_wstring(i)+L".dls")).wstring(),player.playback_collections()[i].bytes);}
        waitForStart();firstStart=player.position().start;stamp("play-ready");Sleep(playingMs);first=player.position().playing;stamp("stop-request");player.stop();stamp("stop-return");firstStopped=!player.position().playing;
        Sleep(3000);firstCount=collect(1,firstStart,0);stamp("restart-request");if(ownedPath)player.play_motif(sourceStyles.at(0),motif->name,GetDesktopWindow(),ownedCollections,{},transportDefault?Bytes{}:host.audio_path_document(0).save_bytes());else player.play(song.save_bytes(),inputDirectory,GetDesktopWindow(),song.styles(),host.playback_collections(index),motif,host.playback_waves(index),host.trigger_playback(index),host.playback_chordmaps(index));stamp("restart-return");
        for(size_t i=0;i<player.playback_collections().size();++i)write_file_atomic((base/(L"restart-collection-"+std::to_wstring(i)+L".dls")).wstring(),player.playback_collections()[i].bytes);
        for(size_t i=0;i<player.playback_styles().size();++i)write_file_atomic((base/(L"restart-style-"+std::to_wstring(i)+L".stp")).wstring(),player.playback_styles()[i].bytes);
        waitForStart();restartStart=player.position().start;stamp("restart-ready");Sleep(playingMs);restarted=player.position().playing;stamp("final-stop-request");player.stop();stamp("final-stop-return");finalStopped=!player.position().playing;
        Sleep(2000);collect(2,restartStart,firstCount);if(player.note_observation_overflow()||player.note_observation_forwarding_failed())throw std::runtime_error("Lifecycle note observer failed");player.shutdown();stamp("shutdown-return");
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    const bool passed=error.empty()&&first&&firstStopped&&restarted&&finalStopped;
    for(const auto& c:player.calls()){if(!calls.empty())calls+=",";calls+="{\"operation\":\""+c.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(c.result))+"}";}
    const auto report="{\"scope\":\"Explicit early Stop, silent hold and replay; digital output requires independent recording\",\"motif\":"+std::string(motif?"true":"false")+",\"passed\":"+std::string(passed?"true":"false")+",\"firstPlaying\":"+(first?"true":"false")+",\"firstStopped\":"+(firstStopped?"true":"false")+",\"restarted\":"+(restarted?"true":"false")+",\"finalStopped\":"+(finalStopped?"true":"false")+",\"error\":\""+error+"\",\"phases\":["+phases+"],\"notes\":["+noteJson+"],\"calls\":["+calls+"],\"modulePathsUtf16Hex\":["+modules+"]}\n";
    write_file_atomic((base/L"lifecycle.json").wstring(),Bytes(report.begin(),report.end()));return passed?0:1;
}
int notification_smoke(const std::wstring& directory,const std::wstring& input) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework host;const auto index=host.open_segment(input);const auto& song=host.document(index);Conductor player;
    std::string notificationEvents,error,modules;unsigned starts=0,ends=0,commands=0,foreign=0;
    bool started=false,stopped=false,restarted=false;
    write_file_atomic((base/L"input.sgp").wstring(),song.save_bytes());
    auto drain=[&](unsigned run){for(const auto& n:player.notifications()){
        const bool segment=IsEqualGUID(n.type,producer::runtime::segmentNotification);
        const bool command=IsEqualGUID(n.type,producer::runtime::commandNotification);
        if(n.currentSegment){if(segment&&n.option==0)++starts;if(segment&&n.option==1)++ends;if(command)++commands;}else ++foreign;
        if(!notificationEvents.empty())notificationEvents+=",";
        notificationEvents+="{\"run\":"+std::to_string(run)+",\"kind\":\""+(segment?"segment":command?"command":"other")+"\",\"option\":"+std::to_string(n.option)+",\"field1\":"+std::to_string(n.field1)+",\"field2\":"+std::to_string(n.field2)+",\"group\":"+std::to_string(n.group)+",\"clocks\":"+std::to_string(n.clocks)+",\"currentSegment\":"+(n.currentSegment?"true":"false")+"}";
    }};
    try {
        const auto inputDirectory=std::filesystem::path(input).parent_path().wstring();
        player.play(song.save_bytes(),inputDirectory,GetDesktopWindow(),song.styles());modules=module_report();
        write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());
        for(size_t i=0;i<song.styles().size();++i){
            write_file_atomic((base/(L"source-style-"+std::to_wstring(i)+L".stp")).wstring(),song.styles()[i].bytes);
            write_file_atomic((base/(L"runtime-style-"+std::to_wstring(i)+L".stp")).wstring(),player.playback_styles().at(i).bytes);
        }
        // Startup preparation is outside the musical duration. Wait for the
        // actual terminal notification before releasing this run's identity.
        for(unsigned i=0;i<120;++i){Sleep(100);drain(1);const auto p=player.position();started=started||p.playing;if(started&&ends==1&&!p.playing)break;}
        drain(1);player.stop();stopped=!player.position().playing;
        player.play(song.save_bytes(),inputDirectory,GetDesktopWindow(),song.styles());
        for(unsigned i=0;i<10;++i){Sleep(100);drain(2);restarted=restarted||player.position().playing;}
        player.stop();player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string calls;for(const auto& c:player.calls()){if(!calls.empty())calls+=",";calls+="{\"operation\":\""+c.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(c.result))+"}";}
    const bool passed=error.empty()&&started&&stopped&&restarted&&starts==2&&ends==1&&commands>0&&foreign==0;
    const auto json="{\"scope\":\"Current-segment runtime notification ownership, end, early stop and restart; Pattern selection and audio unverified\",\"passed\":"+std::string(passed?"true":"false")+",\"started\":"+(started?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"restarted\":"+(restarted?"true":"false")+",\"starts\":"+std::to_string(starts)+",\"ends\":"+std::to_string(ends)+",\"commands\":"+std::to_string(commands)+",\"foreign\":"+std::to_string(foreign)+",\"error\":\""+error+"\",\"events\":["+notificationEvents+"],\"calls\":["+calls+"],\"modulePathsUtf16Hex\":["+modules+"]}\n";
    write_file_atomic((base/L"notifications.json").wstring(),Bytes(json.begin(),json.end()));return passed?0:1;
}
int command_observe(const std::wstring& directory,const std::wstring& input) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    const auto bytes=read_file(input);const auto root=Chunk::parse(bytes);const auto tracks=root.find("LIST","trkl");
    if(!tracks)throw std::runtime_error("Command observation requires tracks");
    write_file_atomic((base/L"input.sgp").wstring(),bytes);
    Conductor player;std::string samples,error,modules;bool exact=false,started=false,stopped=false;unsigned count=0;
    try {
        player.play(bytes,L"",GetDesktopWindow());exact=player.playback_bytes()==bytes;modules=module_report();
        write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());
        DWORD index=0;
        for(const auto& track:tracks->children)if(const auto data=track.find("cmnd")){
            std::vector<LONG> times{0,768,1536,2304,3072,3840,4608,5376};
            for(const auto& e:command_events(data->data)){times.push_back(e.time);if(e.time>0)times.push_back(e.time-1);if(e.time<INT32_MAX)times.push_back(e.time+1);}
            std::sort(times.begin(),times.end());times.erase(std::unique(times.begin(),times.end()),times.end());
            for(const auto time:times)for(const bool withTime:{false,true}){
                const auto s=player.command_parameter(time,0xffffffffu,index,withTime);const auto& e=s.event;
                if(!samples.empty())samples+=",";
                samples+="{\"index\":"+std::to_string(index)+",\"queryTime\":"+std::to_string(time)+",\"parameter\":"+std::to_string(withTime?2:1)+",\"hresult\":"+std::to_string(static_cast<unsigned long>(s.result))+",\"next\":"+std::to_string(s.next)+",\"eventTime\":"+std::to_string(e.time)+",\"type\":"+std::to_string(e.type)+",\"groove\":"+std::to_string(e.groove)+",\"range\":"+std::to_string(e.range)+",\"repeat\":"+std::to_string(e.repeat)+"}";++count;
            }++index;
        }
        for(unsigned i=0;i<20&&!started;++i){Sleep(50);started=player.position().playing;}
        player.stop();stopped=!player.position().playing;player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    const bool captured=error.empty()&&exact&&started&&stopped&&count;
    const auto report="{\"scope\":\"CommandParam/CommandParam2 observation only; captured does not imply semantic equality or product acceptance\",\"captured\":"+std::string(captured?"true":"false")+",\"snapshotExact\":"+(exact?"true":"false")+",\"started\":"+(started?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"error\":\""+error+"\",\"samples\":["+samples+"],\"modulePathsUtf16Hex\":["+modules+"]}\n";
    write_file_atomic((base/L"observation.json").wstring(),Bytes(report.begin(),report.end()));return captured?0:1;
}
int playback_smoke(const std::wstring& directory) {
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework owned;const auto si=owned.new_segment(),bi=owned.new_band();owned.document(si)=SegmentDocument::playback_test();owned.add_band_gm_instrument(bi,0,0,64,100);owned.assign_band(si,bi,0);owned.set_band_instrument(bi,0,40,0,64,100);if(!owned.assign_band(si,bi,3072))throw std::runtime_error("Timed owned Band assignment failed");owned.save_band(bi,(base/L"violin.bnp").wstring());owned.save_segment(si,(base/L"playback.sgp").wstring());owned.save_project((base/L"project.dmpj").wstring());Framework restored;restored.open_project((base/L"project.dmpj").wstring());auto song=restored.document(0);
    Conductor player;std::string samples,error,playingModules;bool started=false,stopped=false,tempo120=false,tempo180=false,editedTempo=false,earlyStop=false,restarted=false;ULONGLONG began=GetTickCount64();
    try {
        player.play(song.save_bytes(),L"",GetDesktopWindow());if(player.playback_bytes()!=song.save_bytes())throw std::runtime_error("Runtime Band snapshot bytes changed");write_file_atomic((base/L"runtime-band.sgp").wstring(),player.playback_bytes());playingModules=module_report();began=GetTickCount64();
        for(unsigned i=0;i<40;++i){Sleep(100);const auto p=player.position();started=started||p.playing;tempo120=tempo120||(p.playing&&p.tempo==120);tempo180=tempo180||(p.playing&&p.tempo==180);if(!samples.empty())samples+=",";samples+="{\"elapsedMs\":"+std::to_string(GetTickCount64()-began)+",\"playing\":"+(p.playing?"true":"false")+",\"clocks\":"+std::to_string(p.clocks)+",\"start\":"+std::to_string(p.start)+",\"tempo\":"+std::to_string(p.tempo)+"}";}
        player.stop();stopped=!player.position().playing;
        song.select(0);if(!song.change_selected(90)||!song.move_band(1,1536,1500))throw std::runtime_error("Playback fixture tempo/Band timing edit failed");song.save((base/L"edited-playback.sgp").wstring());
        player.play(song.save_bytes(),L"",GetDesktopWindow());if(player.playback_bytes()!=song.save_bytes())throw std::runtime_error("Moved Band runtime differs");write_file_atomic((base/L"edited-runtime-band.sgp").wstring(),player.playback_bytes());Sleep(700);const auto edited=player.position();editedTempo=edited.playing&&edited.tempo==90;
        player.stop();earlyStop=edited.playing&&!player.position().playing;
        if(!song.delete_band(1))throw std::runtime_error("Playback fixture Band delete failed");song.save((base/L"deleted-playback.sgp").wstring());player.play(song.save_bytes(),L"",GetDesktopWindow());if(player.playback_bytes()!=song.save_bytes())throw std::runtime_error("Deleted Band runtime differs");write_file_atomic((base/L"deleted-runtime-band.sgp").wstring(),player.playback_bytes());Sleep(400);restarted=player.position().playing;player.stop();player.shutdown();
    } catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string calls;bool callsPassed=true;for(const auto& call:player.calls()){callsPassed=callsPassed&&SUCCEEDED(call.result);if(!calls.empty())calls+=",";calls+="{\"operation\":\""+call.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(call.result))+"}";}
    const bool passed=error.empty()&&started&&stopped&&callsPassed&&tempo120&&tempo180&&editedTempo&&earlyStop&&restarted;
    const auto report="{\"scope\":\"native playback API, tempo, early stop and repeat; audible output not verified\",\"passed\":"+std::string(passed?"true":"false")+",\"started\":"+(started?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"tempo120\":"+(tempo120?"true":"false")+",\"tempo180\":"+(tempo180?"true":"false")+",\"editedTempo90\":"+(editedTempo?"true":"false")+",\"earlyStop\":"+(earlyStop?"true":"false")+",\"restarted\":"+(restarted?"true":"false")+",\"error\":\""+error+"\",\"calls\":["+calls+"],\"samples\":["+samples+"],\"modulePathsUtf16Hex\":["+playingModules+"],\"modulesAfterCleanupUtf16Hex\":["+module_report()+"]}\n";
    write_file_atomic((base/L"playback.json").wstring(),Bytes(report.begin(),report.end()));return passed?0:1;
}
int envelope_audio_fixture(const std::wstring& directory,const std::wstring& input){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);
    Framework host;const auto ci=host.new_collection();auto& d=host.collection_document(ci);
    if(!d.add_wave_pcm(read_file(input),"Envelope tone")||!d.create_instrument(2,7,"Envelope instrument",0))throw std::runtime_error("Envelope PCM/instrument creation failed");
    const auto frames=d.waves().at(0).frames;if(!d.set_region_loops(0,0,{{0,0,static_cast<std::uint32_t>(frames)}}))throw std::runtime_error("Envelope loop creation failed");
    DlsArticulation block{"lart","art1",{}};
    block.connections=envelope_connections(block,envelope_parameters.at(0),-12000*65536);
    block.connections=envelope_connections(block,envelope_parameters.at(3),-12000*65536);
    if(!d.add_articulation(0,0,block))throw std::runtime_error("Envelope Region articulation creation failed");
    const auto fast=d.save_bytes();host.save_collection(ci,(base/L"owned.dls").wstring());
    const auto si=host.new_segment(),bi=host.new_band();auto root=Chunk::parse(host.document(si).save_bytes());put32(root.find("segh")->data,4,49152);host.document(si).load(root.encode());
    for(int t=0;t<49152;t+=3072)if(!host.document(si).add_note({t,2304,0,60,96}))throw std::runtime_error("Envelope note insertion failed");
    if(!host.add_band_gm_instrument(bi,0,0,64,100))throw std::runtime_error("Envelope Band creation failed");host.save_band(bi,(base/L"Envelope.bnp").wstring());
    if(!host.set_band_collection_instrument(bi,0,ci,0)||!host.assign_band(si,bi,0))throw std::runtime_error("Envelope DLS assignment failed");
    host.save_band(bi,(base/L"Envelope.bnp").wstring());host.save_segment(si,(base/L"Envelope.sgp").wstring());host.save_project((base/L"Envelope.dmpj").wstring());
    const auto song=host.document(si).save_bytes(),band=host.band_document(bi).save_bytes();
    auto values=envelope_connections(d.articulations(0,0).at(0),envelope_parameters.at(0),0);
    if(!d.set_articulation_connections(0,0,0,values))throw std::runtime_error("Envelope slow Attack update failed");const auto slow=d.save_bytes();
    if(!d.undo()||d.save_bytes()!=fast||!d.redo()||d.save_bytes()!=slow)throw std::runtime_error("Envelope Attack history differs");
    write_file_atomic((base/L"Fast.dls").wstring(),fast);write_file_atomic((base/L"Slow.dls").wstring(),slow);
    host.save_collection(ci,(base/L"owned.dls").wstring());Framework restored;restored.open_project((base/L"Envelope.dmpj").wstring());
    if(restored.dirty()||restored.document(0).save_bytes()!=song||restored.band_document(0).save_bytes()!=band||restored.playback_collections(0).at(0).bytes!=slow)throw std::runtime_error("Envelope project restoration differs");
    const std::string report="{\"passed\":true,\"attackFastRaw\":-786432000,\"attackSlowRaw\":0,\"notes\":16,\"intervalClocks\":3072,\"durationClocks\":2304,\"pitch\":60,\"velocity\":96,\"scope\":\"Owned source-created Region EG1 Attack only edit, history and DMPJ restore; audio requires recording\",\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"fixture.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
int copied_envelope_fixture(const std::wstring& directory,const std::wstring& input){
    const auto base=std::filesystem::absolute(directory),source=base/L"Source",copied=base/L"Copied";
    envelope_audio_fixture(source.wstring(),input);
    Framework host;host.open_project((source/L"Envelope.dmpj").wstring());host.save_project((source/L"Source.pro").wstring());
    const auto projectBytes=read_file((source/L"Source.pro").wstring());
    host.copy_project((copied/L"Copied.pro").wstring());
    Framework restored;restored.open_project((copied/L"Copied.pro").wstring());
    const auto dependencies=restored.playback_collections(0);
    if(restored.dirty()||restored.documents().size()!=1||restored.band_documents().size()!=1||restored.collections().size()!=1||dependencies.size()!=1)throw std::runtime_error("Copied playable Project ownership differs");
    if(restored.document(0).save_bytes()!=host.document(0).save_bytes()||restored.band_document(0).save_bytes()!=host.band_document(0).save_bytes()||dependencies[0].bytes!=host.playback_collections(0)[0].bytes)throw std::runtime_error("Copied playable document bytes differ");
    for(const auto& name:{L"Envelope.sgp",L"Envelope.bnp",L"owned.dls"})if(read_file((source/name).wstring())!=read_file((copied/name).wstring()))throw std::runtime_error("Copied playable file differs");
    restored.save_project((copied/L"Copied.pro").wstring());
    if(read_file((copied/L"Copied.pro").wstring())!=projectBytes||read_file((source/L"Source.pro").wstring())!=projectBytes||host.dirty())throw std::runtime_error("Copied Project exact resave/source retention differs");
    const std::string report="{\"passed\":true,\"scope\":\"Native Project copy, separate Framework owned Segment/Band/DLS restore and exact resave; audio separate\",\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"fixture.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
int runtime_envelope_fixture(const std::wstring& directory,const std::wstring& input,const std::wstring& audioPath=L"",bool individual=false,bool defaults=false,bool updating=false,bool configured=false,bool multifolder=false){
    const auto base=std::filesystem::absolute(directory),source=base/L"Source",runtime=base/L"Runtime",held=base/L"SourceHeld";
    const auto songPath=runtime/(multifolder?L"Segments/Changed.sgt":L"Envelope.sgt"),bandPath=runtime/(multifolder?L"Bands/Changed.bnd":L"Envelope.bnd"),dlsPath=runtime/(multifolder?L"Collections/Changed.dls":L"owned.dls"),audioPathOutput=runtime/(multifolder?L"Paths/Changed.aud":L"Owned.aud");
    envelope_audio_fixture(source.wstring(),input);Framework host;host.open_project((source/L"Envelope.dmpj").wstring());
    auto root=Chunk::parse(host.document(0).save_bytes());Chunk layout;layout.id="LIST";layout.type="sgdl";Chunk state;state.id="segd";state.data=Bytes(128,0x39);layout.children.push_back(state);
    if(!audioPath.empty()){const auto ai=audioPath==L":new:"?host.new_audio_path():host.open_audio_path(audioPath);host.save_audio_path(ai,(source/L"Owned.aup").wstring());auto config=Chunk::parse(host.audio_path_document(ai).save_bytes());if(config.id!="RIFF"||config.type!="DMAP")throw std::runtime_error("AudioPath fixture requires DMAP");
        const auto ports=config.find("LIST","pcsl");const auto port=ports?ports->find("LIST","pcfl"):nullptr;const auto channels=port?port->find("LIST","pchl"):nullptr;const auto route=channels?channels->find("pchh"):nullptr;
        if(!route||route->data.size()<16||!read32(route->data,4))throw std::runtime_error("AudioPath fixture needs an explicit PChannel route");const auto channel=read32(route->data,0);
        const auto notes=host.document(0).notes();for(size_t i=0;i<notes.size();++i){auto note=notes[i];if(note.channel==channel)continue;note.channel=channel;if(!host.document(0).edit_note(i,note))throw std::runtime_error("AudioPath fixture note route unchanged");}
        const auto instrument=host.band_document(0).instruments().at(0);if(instrument.pchannel!=channel&&(!host.set_band_instrument(0,0,instrument.patch,channel,instrument.pan,instrument.volume)||!host.assign_band(0,0,0)))throw std::runtime_error("AudioPath fixture Band route unchanged");host.save_band(0,(source/L"Envelope.bnp").wstring());
        if(!host.assign_audio_path(0,ai))throw std::runtime_error("Owned AudioPath assignment failed");root=Chunk::parse(host.document(0).save_bytes());write_file_atomic((base/L"audio-path-input.aup").wstring(),config.encode());}
    root.children.push_back(layout);host.document(0).load(root.encode());host.save_segment(0,(source/L"Envelope.sgp").wstring());host.save_project((source/L"Source.pro").wstring());
    if(multifolder){
        const auto filename_only=[&](auto&& self,Chunk& node)->void{if(node.type=="DMRF"){if(auto file=node.find("file")){file->data=utf16(L"owned.dls");if(auto header=node.find("refh"))put32(header->data,16,(read32(header->data,16)|16u)&~1u);node.children.erase(std::remove_if(node.children.begin(),node.children.end(),[](const Chunk& c){return c.id=="guid";}),node.children.end());}}for(auto& c:node.children)if(c.container())self(self,c);};
        auto sourceBand=Chunk::parse(read_file((source/L"Envelope.bnp").wstring()));filename_only(filename_only,sourceBand);write_file_atomic((source/L"Envelope.bnp").wstring(),sourceBand.encode());
        auto sourceSong=Chunk::parse(read_file((source/L"Envelope.sgp").wstring()));const auto visit=[&](auto&& self,Chunk& c)->void{if(c.type=="DMBD")c=sourceBand;else for(auto& child:c.children)if(child.container())self(self,child);};visit(visit,sourceSong);write_file_atomic((source/L"Envelope.sgp").wstring(),sourceSong.encode());
        host.open_project((source/L"Source.pro").wstring());host.save_band(0,(source/L"Envelope.bnp").wstring());host.save_segment(0,(source/L"Envelope.sgp").wstring());root=Chunk::parse(host.document(0).save_bytes());
        host.set_runtime_project_folder(L"..\\Runtime\\");host.set_runtime_component_folder(RuntimeDocumentKind::Segment,L"..\\Runtime\\Segments\\");host.set_runtime_component_folder(RuntimeDocumentKind::Band,L"..\\Runtime\\Bands\\");host.set_runtime_component_folder(RuntimeDocumentKind::Collection,L"..\\Runtime\\Collections\\");host.set_runtime_component_folder(RuntimeDocumentKind::AudioPath,L"..\\Runtime\\Paths\\");host.set_runtime_filename(RuntimeDocumentKind::Segment,0,L"Changed.sgt");host.set_runtime_filename(RuntimeDocumentKind::Band,0,L"Changed.bnd");host.set_runtime_filename(RuntimeDocumentKind::Collection,0,L"Changed.dls");host.set_runtime_filename(RuntimeDocumentKind::AudioPath,0,L"Changed.aud");host.save_project((source/L"Source.pro").wstring());
    }
    if(defaults&&!multifolder){host.set_runtime_project_folder(L"..\\Runtime\\");host.set_runtime_component_folder(RuntimeDocumentKind::AudioPath,L"..\\Runtime\\");host.save_project((source/L"Source.pro").wstring());}
    const auto projectBytes=read_file((source/L"Source.pro").wstring()),band=host.band_document(0).save_bytes(),dls=host.collection_document(0).save_bytes();
    auto runtimeCollection=Chunk::parse(dls);
    const auto stripRegionEditor=[&](auto&& self,Chunk& c)->void{if(c.type=="rgn "||c.type=="rgn2")c.children.erase(std::remove_if(c.children.begin(),c.children.end(),[](const Chunk& x){return x.id=="dmpr";}),c.children.end());for(auto& child:c.children)if(child.container())self(self,child);};
    stripRegionEditor(stripRegionEditor,runtimeCollection);const auto runtimeDls=runtimeCollection.encode();
    if(updating){std::filesystem::create_directory(runtime);write_file_atomic((songPath).wstring(),{1,2,3});write_file_atomic((dlsPath).wstring(),{4,5,6});write_file_atomic((runtime/L"retained.bin").wstring(),{7,8,9});write_file_atomic((base/L"prior-segment.bin").wstring(),{1,2,3});write_file_atomic((base/L"prior-collection.bin").wstring(),{4,5,6});host.export_runtime(runtime.wstring());if(read_file((runtime/L"retained.bin").wstring())!=Bytes({7,8,9}))throw std::runtime_error("Runtime update changed unrelated file");std::filesystem::rename(runtime/L"retained.bin",base/L"retained-after.bin");}
    else if(configured){host.export_runtime_defaults();}
    else if(defaults){std::filesystem::create_directory(runtime);host.save_runtime_default(RuntimeDocumentKind::Segment,0);host.save_runtime_default(RuntimeDocumentKind::Band,0);host.save_runtime_default(RuntimeDocumentKind::Collection,0);host.save_runtime_default(RuntimeDocumentKind::AudioPath,0);}
    else if(individual){std::filesystem::create_directory(runtime);host.save_runtime(RuntimeDocumentKind::Segment,0,(songPath).wstring());host.save_runtime(RuntimeDocumentKind::Band,0,(bandPath).wstring());host.save_runtime(RuntimeDocumentKind::Collection,0,(dlsPath).wstring());if(!audioPath.empty())host.save_runtime(RuntimeDocumentKind::AudioPath,0,(audioPathOutput).wstring());}
    else host.export_runtime(runtime.wstring());
    auto expected=root;expected.children.erase(std::remove_if(expected.children.begin(),expected.children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="sgdl";}),expected.children.end());
    const auto stripAudioEditor=[&](auto&& self,Chunk& c)->void{c.children.erase(std::remove_if(c.children.begin(),c.children.end(),[&](const Chunk& x){return (c.type=="DMAP"&&x.type=="papd")||((c.type=="pcfl"||c.type=="pchl"||c.type=="DSFX")&&x.type=="UNFO")||(c.type=="DSFX"&&x.id=="pegd");}),c.children.end());for(auto& child:c.children)if(child.container())self(self,child);};stripAudioEditor(stripAudioEditor,expected);
    auto expectedBand=Chunk::parse(band);if(multifolder){const auto rewrite=[&](auto&& self,Chunk& c)->void{if(c.type=="DMRF")if(auto file=c.find("file"))file->data=utf16(L"..\\Collections\\Changed.dls");for(auto& child:c.children)if(child.container())self(self,child);};rewrite(rewrite,expected);rewrite(rewrite,expectedBand);}
    if(read_file((songPath).wstring())!=expected.encode()||read_file((bandPath).wstring())!=expectedBand.encode()||read_file((dlsPath).wstring())!=runtimeDls||host.collection_document(0).save_bytes()!=dls||host.dirty()||read_file((source/L"Source.pro").wstring())!=projectBytes)throw std::runtime_error("Playable runtime export changed unexpected bytes or source");
    std::filesystem::rename(source,held);
    Framework restored;if(!audioPath.empty())restored.open_audio_path((audioPathOutput).wstring());restored.open_collection((dlsPath).wstring());restored.open_band((bandPath).wstring());restored.open_segment((songPath).wstring());
    const auto dependencies=restored.playback_collections(0);
    if(dependencies.size()!=1||dependencies[0].bytes!=runtimeDls||std::filesystem::path(dependencies[0].path)!=dlsPath||restored.document(0).save_bytes()!=expected.encode())throw std::runtime_error("Runtime restoration uses unexpected dependency");
    // This auxiliary test catalog preloads exported GUID-only dependencies.
    // It is created after export, and is not a native runtime file format.
    restored.save_project((runtime/L"playback.dmpj").wstring());Framework check;check.open_project((runtime/L"playback.dmpj").wstring());
    if(check.dirty()||check.document(0).save_bytes()!=expected.encode()||check.playback_collections(0)[0].bytes!=runtimeDls||std::filesystem::exists(source))throw std::runtime_error("Runtime test catalog restoration differs");
    const std::string report="{\"passed\":true,\"sourceFolderAbsent\":true,\"scope\":\"Source-created real PCM/instrument runtime export, design layout removal, source retained under SourceHeld, independent exported-only catalog reload; DirectMusic/audio separate\",\"fullAcceptance\":false}\n";
    write_file_atomic((base/L"fixture.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
int runtime_style_fixture(const std::wstring& directory,const std::wstring& input,bool motif=false){
    const auto base=std::filesystem::absolute(directory),source=base/L"Source",runtime=base/L"Runtime";std::filesystem::create_directories(base);
    Framework original;original.open_project(input);if(original.documents().size()!=1||original.style_documents().size()!=1||original.collections().size()!=1||!original.document(0).notes().empty())throw std::runtime_error("Runtime Style input requires one generated-note Segment, Style and Collection");
    original.copy_project((source/L"project.dmpj").wstring());Framework host;host.open_project((source/L"project.dmpj").wstring());
    if(motif){const auto patterns=host.style_document(0).patterns();if(patterns.size()!=1||patterns[0].name!=L"Authored Motif"||!(patterns[0].embellishment&16)||!host.style_document(0).motif_band(0))throw std::runtime_error("Runtime Motif requires its explicit owned Band");
        if(!host.set_style_motif_settings(0,0,{63,0,768,2304,1}))throw std::runtime_error("Runtime Motif long finite repeat edit unchanged");host.save_style(0,host.style_documents()[0].path);
    }
    const auto ai=host.new_audio_path();host.save_audio_path(ai,(source/L"Owned.aup").wstring());if(!host.assign_audio_path(0,ai))throw std::runtime_error("Runtime Style AudioPath assignment failed");host.save_segment(0,host.documents()[0].path);host.save_project((source/L"Source.pro").wstring());
    host.set_runtime_project_folder(L"..\\Runtime\\");host.set_runtime_component_folder(RuntimeDocumentKind::Segment,L"..\\Runtime\\Segments\\");host.set_runtime_component_folder(RuntimeDocumentKind::Style,L"..\\Runtime\\Styles\\");host.set_runtime_component_folder(RuntimeDocumentKind::Collection,L"..\\Runtime\\Collections\\");host.set_runtime_component_folder(RuntimeDocumentKind::AudioPath,L"..\\Runtime\\Paths\\");host.set_runtime_filename(RuntimeDocumentKind::Segment,0,L"Normal.sgt");host.set_runtime_filename(RuntimeDocumentKind::Style,0,L"Normal.sty");host.set_runtime_filename(RuntimeDocumentKind::Collection,0,L"Tone.dls");host.set_runtime_filename(RuntimeDocumentKind::AudioPath,0,L"Owned.aud");host.save_project((source/L"Source.pro").wstring());
    const auto project=read_file((source/L"Source.pro").wstring()),segment=host.document(0).save_bytes(),style=host.style_document(0).save_bytes(),collection=host.collection_document(0).save_bytes();host.export_runtime_defaults();if(host.dirty()||read_file((source/L"Source.pro").wstring())!=project||host.document(0).save_bytes()!=segment||host.style_document(0).save_bytes()!=style||host.collection_document(0).save_bytes()!=collection)throw std::runtime_error("Runtime Style export changed source");
    std::filesystem::rename(source,base/L"SourceHeld");Framework restored;restored.open_collection((runtime/L"Collections"/L"Tone.dls").wstring());restored.open_style((runtime/L"Styles"/L"Normal.sty").wstring());restored.open_audio_path((runtime/L"Paths"/L"Owned.aud").wstring());restored.open_segment((runtime/L"Segments"/L"Normal.sgt").wstring());const auto dependencies=restored.playback_collections(0);
    if(!restored.document(0).notes().empty()||restored.document(0).styles().size()!=1||dependencies.size()!=(motif?1u:2u)||dependencies[0].path!=(runtime/L"Collections"/L"Tone.dls").wstring()||dependencies[0].bytes!=read_file(dependencies[0].path)||(!motif&&(dependencies[1].path!=dependencies[0].path||dependencies[1].bytes!=dependencies[0].bytes)))throw std::runtime_error("Runtime Style restoration dependency mismatch");
    restored.save_project((runtime/L"playback.dmpj").wstring());Framework check;check.open_project((runtime/L"playback.dmpj").wstring());if(check.dirty()||check.document(0).save_bytes()!=restored.document(0).save_bytes()||check.style_document(0).save_bytes()!=restored.style_document(0).save_bytes()||check.playback_collections(0).at(0).bytes!=dependencies[0].bytes||std::filesystem::exists(source))throw std::runtime_error("Runtime Style auxiliary catalog differs");
    const std::string report="{\"passed\":true,\"sourceFolderAbsent\":true,\"sequenceNotes\":0,\"styles\":1,\"motif\":"+std::string(motif?"true":"false")+",\"collectionSnapshots\":"+std::to_string(dependencies.size())+",\"scope\":\"Source-owned Style and Band through renamed configured runtime folders, auxiliary catalog reload; real playback/audio separate\",\"fullAcceptance\":false}\n";write_file_atomic((base/L"fixture.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
int authored_dls_fixture(const std::wstring& directory,const std::wstring& input){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);Framework host;const auto ci=host.open_collection(input);auto& d=host.collection_document(ci);const auto original=d.save_bytes();
    if(d.instruments().size()!=1||d.waves().size()!=1||d.instruments()[0].regions.size()!=1)throw std::runtime_error("Authored fixture requires one Instrument/Region/Wave");
    const auto frames=d.waves()[0].frames;if(frames<100||frames>UINT32_MAX)throw std::runtime_error("Fixture sample length invalid");
    if(!d.set_region_loops(0,0,{{0,0,static_cast<std::uint32_t>(frames)}}))throw std::runtime_error("Fixture Region loop unchanged");const auto looped=d.save_bytes();if(!d.undo()||d.save_bytes()!=original||!d.redo()||d.save_bytes()!=looped)throw std::runtime_error("Fixture loop history differs");
    host.save_collection(ci,(base/L"owned.dls").wstring());const auto si=host.new_segment(),bi=host.new_band();
    // Fixture length only; notes and ownership use actual editor APIs.
    auto root=Chunk::parse(host.document(si).save_bytes());put32(root.find("segh")->data,4,49152);host.document(si).load(root.encode());
    for(int time=0;time<49152;time+=768)if(!host.document(si).add_note({time,384,0,60,96}))throw std::runtime_error("Fixture note insertion failed");
    host.add_band_gm_instrument(bi,0,0,64,100);host.save_band(bi,(base/L"Authored.bnp").wstring());if(!host.set_band_collection_instrument(bi,0,ci,0))throw std::runtime_error("Fixture DLS assignment failed");host.save_band(bi,(base/L"Authored.bnp").wstring());if(!host.assign_band(si,bi,0))throw std::runtime_error("Fixture Segment Band copy failed");host.save_segment(si,(base/L"Authored.sgp").wstring());
    const auto project=base/(base.filename().wstring()+L".pro");host.save_project(project.wstring());const auto projectBytes=read_file(project.wstring());Framework reload;reload.open_project(project.wstring());if(reload.document(0).save_bytes()!=host.document(si).save_bytes()||reload.playback_collections(0).size()!=1||reload.playback_collections(0)[0].bytes!=looped||reload.dirty())throw std::runtime_error("Authored native Project reload differs");reload.save_project(project.wstring());if(read_file(project.wstring())!=projectBytes)throw std::runtime_error("Authored Project resave differs");write_file_atomic((base/L"input.dlp").wstring(),original);
    const std::string report="{\"passed\":true,\"notes\":64,\"pitch\":60,\"velocity\":96,\"loopFrames\":"+std::to_string(frames)+",\"scope\":\"GUI-authored input; typed Region loop history, owned Band and Segment, native Project reload/resave; no GUI restart/audio\"}\n";write_file_atomic((base/L"fixture.json").wstring(),Bytes(report.begin(),report.end()));return 0;
}
int dls_playback_smoke(const std::wstring& directory,const std::wstring& input,size_t selectedInstrument=0){
    const auto base=std::filesystem::absolute(directory);std::filesystem::create_directories(base);const auto empty=base/L"runtime-empty";std::filesystem::create_directories(empty);
    const auto original=read_file(input);write_file_atomic((base/L"owned.dls").wstring(),original);Framework host;const auto si=host.new_segment(),bi=host.new_band(),ci=host.open_collection((base/L"owned.dls").wstring());host.document(si)=SegmentDocument::playback_test(72);const auto instruments=host.collection_document(ci).instruments();const auto instrument=instruments.at(selectedInstrument);const auto& first=instruments.at(0);const auto expectedFirstPatch=(first.bank&0x80000000u)|((first.bank&0x7f7fu)<<8)|first.program;const auto expectedPatch=(instrument.bank&0x80000000u)|((instrument.bank&0x7f7fu)<<8)|instrument.program;for(const auto& note:host.document(si).notes())if(std::none_of(instrument.regions.begin(),instrument.regions.end(),[&](const DlsRegion& r){return note.pitch>=r.keyLow&&note.pitch<=r.keyHigh&&note.velocity>=r.velocityLow&&note.velocity<=r.velocityHigh;}))throw std::runtime_error("DLS test note outside assigned instrument regions");host.add_band_gm_instrument(bi,0,0,64,100);host.save_band(bi,(base/L"band.bnp").wstring());if(!host.set_band_collection_instrument(bi,0,ci,selectedInstrument))throw std::runtime_error("DLS locale assignment failed");host.save_segment(si,(base/L"playback.sgp").wstring());host.assign_band(si,bi,0);host.save_band(bi,(base/L"band.bnp").wstring());host.save_segment(si,(base/L"playback.sgp").wstring());host.save_project((base/L"project.dmpj").wstring());Framework restored;restored.open_project((base/L"project.dmpj").wstring());const auto& song=restored.document(0);const auto dependencies=restored.playback_collections(0);
    Conductor player;bool started=false,stopped=false,unresolvedRejected=false,invalidLoopRejected=false,restarted=false,generatedIdentity=false;DWORD firstPatch=0;std::string error,modules,samples;
    try{player.play(song.save_bytes(),empty.wstring(),GetDesktopWindow(),song.styles(),dependencies);modules=module_report();write_file_atomic((base/L"runtime.sgp").wstring(),player.playback_bytes());write_file_atomic((base/L"runtime.dls").wstring(),player.playback_collections()[0].bytes);firstPatch=player.playback_collection_first_patches().at(0);if(firstPatch!=expectedFirstPatch)throw std::runtime_error("Runtime patch differs from assigned DLS locale");
        for(unsigned i=0;i<16;++i){Sleep(100);const auto p=player.position();started=started||p.playing;if(!samples.empty())samples+=",";samples+="{\"playing\":"+std::string(p.playing?"true":"false")+",\"tempo\":"+std::to_string(p.tempo)+"}";}
        try{player.play(song.save_bytes(),empty.wstring(),GetDesktopWindow());}catch(const std::exception&){unresolvedRejected=player.position().playing;}// Corrupt only an owned test Region override. Preflight must refuse it
        // without replacing the active performance or emitting runtime calls.
        auto malformed=Chunk::parse(original);auto region=malformed.find("LIST","lins")->find("LIST","ins ")->find("LIST","lrgn");auto target=region->find("LIST","rgn ");if(!target)target=region->find("LIST","rgn2");auto sample=target->find("wsmp");if(!sample){Chunk c;c.id="wsmp";c.data=Bytes(36);put32(c.data,0,20);put32(c.data,16,1);put32(c.data,20,16);target->children.push_back(c);sample=&target->children.back();}else{sample->data.resize(36);put32(sample->data,0,20);put32(sample->data,16,1);put32(sample->data,20,16);}put32(sample->data,24,0);put32(sample->data,28,UINT32_MAX);put32(sample->data,32,1);
        auto invalid=dependencies;invalid[0].bytes=malformed.encode();write_file_atomic((base/L"invalid-loop.dls").wstring(),invalid[0].bytes);const auto callCount=player.calls().size();try{player.play(song.save_bytes(),empty.wstring(),GetDesktopWindow(),song.styles(),invalid);}catch(const std::exception&){const bool untouched=player.calls().size()==callCount;invalidLoopRejected=untouched&&player.playback_collections()[0].bytes==original&&player.position().playing;}player.stop();stopped=!player.position().playing;
        // Remove identity only from owned test copies; a relative filename
        // must map to a generated memory GUID without reopening the file.
        auto root=Chunk::parse(original);root.children.erase(std::remove_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id=="dlid";}),root.children.end());const auto noId=root.encode();write_file_atomic((base/L"filename-source.dls").wstring(),noId);BandDocument band;band.load(restored.band_document(0).save_bytes());band.set_collection_reference(0,{L"filename-source.dls",{}});auto copy=song;copy.set_band(0,band.save_bytes());write_file_atomic((base/L"filename-source.sgp").wstring(),copy.save_bytes());const auto resolved=resolve_collections(document_collection_references(copy.save_bytes()),base.wstring(),{{(base/L"filename-source.dls").wstring(),noId}});
        player.play(copy.save_bytes(),empty.wstring(),GetDesktopWindow(),{},resolved);write_file_atomic((base/L"generated-runtime.sgp").wstring(),player.playback_bytes());write_file_atomic((base/L"generated-runtime.dls").wstring(),player.playback_collections()[0].bytes);generatedIdentity=collection_identity(player.playback_collections()[0].bytes).has_value()&&player.playback_collection_first_patches()[0]==expectedFirstPatch&&!collection_identity(noId);Sleep(400);restarted=player.position().playing;player.stop();player.shutdown();
    }catch(const std::exception& e){error=e.what();player.shutdown();}
    std::string calls;bool callsPassed=true;for(const auto& c:player.calls()){callsPassed=callsPassed&&c.result==S_OK;if(!calls.empty())calls+=",";calls+="{\"operation\":\""+c.operation+"\",\"hresult\":"+std::to_string(static_cast<unsigned long>(c.result))+"}";}
    // COM initialization can legitimately return S_FALSE; other partial
    // load/download statuses are explicitly rejected by the production path.
    callsPassed=std::all_of(player.calls().begin(),player.calls().end(),[](const RuntimeCall& c){return c.result==S_OK||(c.result==S_FALSE&&(c.operation=="CoInitializeEx"||c.operation=="IsPlaying"||c.operation=="IsPlaying after StopEx"));});const bool passed=error.empty()&&started&&stopped&&unresolvedRejected&&invalidLoopRejected&&restarted&&generatedIdentity&&callsPassed;
    const auto json="{\"scope\":\"native owned DLS memory load, Segment download, Play/Stop/restart; actual audio unverified\",\"passed\":"+std::string(passed?"true":"false")+",\"firstPatch\":"+std::to_string(firstPatch)+",\"assignedPatch\":"+std::to_string(expectedPatch)+",\"selectedInstrument\":"+std::to_string(selectedInstrument)+",\"started\":"+(started?"true":"false")+",\"stopped\":"+(stopped?"true":"false")+",\"unresolvedRejectedBeforeStop\":"+(unresolvedRejected?"true":"false")+",\"invalidLoopRejectedBeforeStop\":"+(invalidLoopRejected?"true":"false")+",\"restarted\":"+(restarted?"true":"false")+",\"generatedIdentity\":"+(generatedIdentity?"true":"false")+",\"error\":\""+error+"\",\"calls\":["+calls+"],\"samples\":["+samples+"],\"modulePathsUtf16Hex\":["+modules+"],\"modulesAfterCleanupUtf16Hex\":["+module_report()+"]}\n";write_file_atomic((base/L"playback.json").wstring(),Bytes(json.begin(),json.end()));return passed?0:1;
}
}
int runtime_recovery_inspect(const std::wstring& project,const std::wstring& journal,const std::wstring& report,const std::wstring* restoreRoot=nullptr){
    Framework host;host.open_project(project);auto items=host.inspect_runtime_recovery(journal);
    const auto output=std::filesystem::absolute(report).lexically_normal();if(std::filesystem::exists(output))throw std::runtime_error("Recovery inspection report must be a new file");
    const auto equal=[&](const std::wstring& value){return CompareStringOrdinal(output.c_str(),-1,std::filesystem::absolute(value).lexically_normal().c_str(),-1,TRUE)==CSTR_EQUAL;};
    if(equal(project)||equal(journal))throw std::runtime_error("Recovery report aliases an input");for(const auto& item:items)if(equal(item.file.target))throw std::runtime_error("Recovery report aliases a target");
    const auto changed=std::count_if(items.begin(),items.end(),[](const RuntimeRecoveryTarget& s){return s.state==RuntimeRecoveryState::After;});
    if(restoreRoot){const bool configured=*restoreRoot==L":defaults:";host.recover_runtime_update(journal,configured?std::filesystem::absolute(project).parent_path().wstring():*restoreRoot,configured);items=host.inspect_runtime_recovery(journal);}
    const auto quote=[](const std::string& s){std::string out="\"";const char* digits="0123456789abcdef";for(unsigned char c:s){if(c=='"'||c=='\\'){out+='\\';out+=c;}else if(c<32){out+="\\u00";out+=digits[c>>4];out+=digits[c&15];}else out+=c;}return out+'"';};
    std::string entries;bool recoverable=true;for(const auto& item:items){if(!entries.empty())entries+=",";const auto state=item.state==RuntimeRecoveryState::Before?"before":item.state==RuntimeRecoveryState::After?"after":"conflict";recoverable=recoverable&&item.state!=RuntimeRecoveryState::Conflict;entries+="{\"target\":"+quote(std::filesystem::path(item.file.target).u8string())+",\"state\":"+quote(state)+",\"reason\":"+quote(item.reason)+"}";}
    const auto json="{\"schema\":1,\"inspectionPassed\":true,\"allTargetsRecognized\":"+std::string(recoverable?"true":"false")+",\"targets\":["+entries+"],\"writesToTargets\":"+(restoreRoot&&changed?"true":"false")+",\"recoveryExecuted\":"+(restoreRoot?"true":"false")+",\"restoredTargets\":"+std::to_string(restoreRoot?changed:0)+",\"fullAcceptance\":false}\n";write_file_atomic(output.wstring(),Bytes(json.begin(),json.end()));return 0;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
    int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);int result=1;
    try {
        if(!argv)throw std::runtime_error("Unable to parse arguments");
auto number=[](const wchar_t* text,unsigned long maximum){const std::wstring value=text;if(value.empty()||!std::all_of(value.begin(),value.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';}))throw std::runtime_error("Invalid playback option integer");size_t end=0;const auto n=std::stoul(value,&end);if(end!=value.size()||n>maximum)throw std::runtime_error("Playback option outside range");return n;};
        if(argc==3&&std::wstring(argv[1])==L"--smoke")result=smoke(argv[2]);
        else if(argc==5&&std::wstring(argv[1])==L"--inspect-runtime-recovery")result=runtime_recovery_inspect(argv[2],argv[3],argv[4]);
        else if(argc==6&&std::wstring(argv[1])==L"--recover-runtime-update"){const std::wstring root=argv[4];result=runtime_recovery_inspect(argv[2],argv[3],argv[5],&root);}
        else if(argc==3&&std::wstring(argv[1])==L"--playback-smoke")result=playback_smoke(argv[2]);
        else if(argc==4&&std::wstring(argv[1])==L"--group-playback-smoke")result=group_playback_smoke(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--command-observe")result=command_observe(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--notification-smoke")result=notification_smoke(argv[2],argv[3]);
        else if((argc==4||argc==5)&&std::wstring(argv[1])==L"--note-observe"){const auto observationMs=argc==5?number(argv[4],120000):12000;if(observationMs<1000)throw std::runtime_error("Note observation window must be1000..120000 ms");result=note_observe(argv[2],argv[3],{},false,{},observationMs);}
        else if(argc==5&&std::wstring(argv[1])==L"--motif-observe")result=note_observe(argv[2],argv[3],MotifSelection{0,argv[4]});
        else if(argc==3&&std::wstring(argv[1])==L"--short-playback-monitor")result=short_playback_monitor(argv[2]);
        else if(argc==6&&std::wstring(argv[1])==L"--boundary-runtime-observe")result=boundary_runtime_observe(argv[2],argv[3],argv[4],argv[5]);
        else if(argc==6&&std::wstring(argv[1])==L"--notification-identity-stress")result=notification_identity_stress(argv[2],argv[3],argv[4],argv[5]);
        else if(argc==6&&std::wstring(argv[1])==L"--motif-primary-cancel-audio")result=motif_primary_cancel_audio(argv[2],argv[3],argv[4],argv[5]);
        else if(argc==6&&std::wstring(argv[1])==L"--motif-primary-replacement-audio")result=motif_primary_replacement_audio(argv[2],argv[3],argv[4],argv[5]);
        else if(argc==5&&std::wstring(argv[1])==L"--style-player-audio")result=style_player_audio(argv[2],argv[3],argv[4]);
        else if(argc==6&&std::wstring(argv[1])==L"--motif-concurrent-audio")result=motif_concurrent_audio(argv[2],argv[3],argv[4],argv[5]);
        else if(argc==5&&std::wstring(argv[1])==L"--motif-concurrent-observe")result=motif_concurrent_observe(argv[2],argv[3],argv[4]);
        else if(argc==5&&std::wstring(argv[1])==L"--style-motif-observe")result=note_observe(argv[2],argv[3],MotifSelection{0,argv[4]},true);
        else if(argc==9&&std::wstring(argv[1])==L"--style-motif-scheduled-observe"){
            PlaybackOptions options{static_cast<PlaybackBoundary>(number(argv[6],4)),number(argv[7],1)!=0,number(argv[8],1)!=0,static_cast<LONG>(number(argv[5],INT32_MAX))};result=note_observe(argv[2],argv[3],MotifSelection{0,argv[4]},true,options);
        }
        else if(argc==4&&std::wstring(argv[1])==L"--audio-lifecycle")result=audio_lifecycle(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--normal-style-lifecycle")result=audio_lifecycle(argv[2],argv[3],{},6000);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-authored-dls")result=authored_dls_fixture(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-envelope-audio")result=envelope_audio_fixture(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-copied-envelope")result=copied_envelope_fixture(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-runtime-envelope")result=runtime_envelope_fixture(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-runtime-style")result=runtime_style_fixture(argv[2],argv[3]);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-runtime-motif")result=runtime_style_fixture(argv[2],argv[3],true);
        else if(argc==4&&std::wstring(argv[1])==L"--prepare-runtime-default-audiopath")result=runtime_envelope_fixture(argv[2],argv[3],L":new:");
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-multifolder")result=runtime_envelope_fixture(argv[2],argv[3],argv[4],false,true,false,true,true);
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-defaults")result=runtime_envelope_fixture(argv[2],argv[3],argv[4],false,true,false,true);
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-update")result=runtime_envelope_fixture(argv[2],argv[3],argv[4],false,false,true);
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-settings")result=runtime_envelope_fixture(argv[2],argv[3],argv[4],false,true);
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-save-as")result=runtime_envelope_fixture(argv[2],argv[3],argv[4],true);
        else if(argc==5&&std::wstring(argv[1])==L"--prepare-runtime-audiopath")result=runtime_envelope_fixture(argv[2],argv[3],argv[4]);
        else if(argc==5&&std::wstring(argv[1])==L"--owned-motif-path-lifecycle")result=audio_lifecycle(argv[2],argv[3],MotifSelection{0,argv[4]},2000,true);
        else if(argc==5&&std::wstring(argv[1])==L"--transport-motif-lifecycle")result=audio_lifecycle(argv[2],argv[3],MotifSelection{0,argv[4]},2000,true,true);
        else if(argc==5&&std::wstring(argv[1])==L"--motif-lifecycle")result=audio_lifecycle(argv[2],argv[3],MotifSelection{0,argv[4]});
        else if((argc==4||argc==5)&&std::wstring(argv[1])==L"--dls-playback-smoke"){size_t index=0;if(argc==5){const std::wstring value=argv[4];size_t end=0;index=std::stoul(value,&end);if(end!=value.size()||value.empty()||value[0]==L'-')throw std::runtime_error("Invalid DLS instrument index");}result=dls_playback_smoke(argv[2],argv[3],index);}
        else if(argc==5&&std::wstring(argv[1])==L"--style-playback-smoke")result=style_playback_smoke(argv[2],argv[3],argv[4]);
        else {
            if(argc==3&&std::wstring(argv[1])==L"--open-project"){
                framework.open_project(argv[2]);
                active=activeStyle=activeBand=0;
                styleMode=framework.documents().empty()&&!framework.style_documents().empty();
                bandMode=framework.documents().empty()&&framework.style_documents().empty()&&!framework.band_documents().empty();
            }else if(argc!=1)throw std::runtime_error("Use --open-project <project> to open a project at startup");
            clipboardFormat=RegisterClipboardFormatW(L"Producer.Source.Tempo.v1");patternClipboardFormat=RegisterClipboardFormatW(L"Producer.Source.Pattern.v1");if(!clipboardFormat||!patternClipboardFormat)throw std::runtime_error("Clipboard format registration failed");
            WNDCLASSW c{};c.lpfnWndProc=window_proc;c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.lpszClassName=L"SourceProducer";
            if(!RegisterClassW(&c))throw std::runtime_error("Window class registration failed");
            HMENU menu=CreateMenu(),file=CreatePopupMenu();
            transportMenu=CreatePopupMenu();AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(transportMenu),L"Transport AudioPath");
            AppendMenuW(file,MF_STRING,NewProject,L"New Project");AppendMenuW(file,MF_STRING,NewSegment,L"New Segment");AppendMenuW(file,MF_STRING,Open,L"Open...");AppendMenuW(file,MF_STRING,ImportMidi,L"Import MIDI as Segment...");AppendMenuW(file,MF_STRING,SaveSegment,L"Save Document");AppendMenuW(file,MF_STRING,SaveDocumentAs,L"Save Document As...");AppendMenuW(file,MF_STRING,SaveProject,L"Save Project As...");AppendMenuW(file,MF_STRING,CopyProject,L"Copy Project...");AppendMenuW(file,MF_STRING,Exit,L"Exit");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(file),L"File");
            HMENU addInsMenu=CreatePopupMenu();AppendMenuW(addInsMenu,MF_STRING,MessageWindowCommand,L"Message Window");AppendMenuW(addInsMenu,MF_STRING,StylePlayerCommand,L"StylePlayer...");AppendMenuW(addInsMenu,MF_STRING,FarmPlayerCommand,L"Farm Score Player...");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(addInsMenu),L"Add-Ins");
            HMENU recordingMenu=CreatePopupMenu();AppendMenuW(recordingMenu,MF_STRING,StartFileOutput,L"Start Buffer Recording...");AppendMenuW(recordingMenu,MF_STRING,StopFileOutput,L"Stop Buffer Recording");AppendMenuW(recordingMenu,MF_STRING,WavesReverbCommand,L"Playing Waves Reverb Parameters...");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(recordingMenu),L"Recording");
            HMENU patternMenu=CreatePopupMenu();AppendMenuW(patternMenu,MF_STRING,PatternNew,L"New Pattern");AppendMenuW(patternMenu,MF_STRING,MotifNew,L"New Motif");AppendMenuW(patternMenu,MF_STRING,MotifSettings,L"Motif Playback Settings...");AppendMenuW(patternMenu,MF_STRING,MotifPlay,L"Play Selected Motif");AppendMenuW(patternMenu,MF_STRING,StopCurrentPlayback,L"Stop Most Recent Playback");AppendMenuW(patternMenu,MF_STRING,PlaybackSessions,L"Playback Sessions...");AppendMenuW(patternMenu,MF_STRING,MotifBandAssign,L"Assign Selected Style Band to Motif");AppendMenuW(patternMenu,MF_STRING,MotifBandEdit,L"Edit Motif Band Instruments...");AppendMenuW(patternMenu,MF_STRING,PatternDelete,L"Delete Pattern...");AppendMenuW(patternMenu,MF_STRING,PatternDuplicate,L"Duplicate Pattern");AppendMenuW(patternMenu,MF_STRING,PatternUnshare,L"Make Selected Part Independent");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(patternMenu),L"Pattern");
            InsertMenuW(file,2,MF_BYPOSITION|MF_STRING,NewPlaybackTest,L"New Playback Test");
            AppendMenuW(file,MF_STRING,AudioPathEditor,L"AudioPath Documents...");
            AppendMenuW(file,MF_STRING,StyleReferenceEditor,L"Segment Style References...");
            AppendMenuW(file,MF_STRING,ChordMapReferenceEditor,L"Segment ChordMap References...");
            AppendMenuW(file,MF_STRING,SegmentTriggerEditor,L"Segment Triggers...");
            AppendMenuW(file,MF_STRING,ScriptTrackEditor,L"Script Track...");
            AppendMenuW(file,MF_STRING,ChordMapEditor,L"Chordmap Documents...");
            AppendMenuW(file,MF_STRING,MarkerEditor,L"Segment Markers / Enter SwitchPoints...");
            AppendMenuW(file,MF_STRING,LyricEditor,L"Segment Lyrics...");
            AppendMenuW(file,MF_STRING,MuteEditor,L"Segment Mute / PChannel Remap...");
            AppendMenuW(file,MF_STRING,ParamControlEditor,L"Segment Parameter Control...");
            AppendMenuW(file,MF_STRING,WaveEditor,L"Segment Wave Placement / Trim...");
            AppendMenuW(file,MF_STRING,WaveDocuments,L"Wave Documents...");
            AppendMenuW(file,MF_STRING,ScriptDocuments,L"Script Documents...");
            AppendMenuW(file,MF_STRING,ToolGraphDocuments,L"ToolGraph Documents...");
            AppendMenuW(file,MF_STRING,ContainerDocuments,L"Container Documents...");
            AppendMenuW(file,MF_STRING,TimelineRangeEditor,L"Timeline Range...");
            AppendMenuW(file,MF_STRING,RuntimeSettings,L"Runtime Properties...");
            AppendMenuW(file,MF_STRING,RuntimeSaveAs,L"Runtime Save As...");
            AppendMenuW(file,MF_STRING,RuntimeSaveAll,L"Runtime Save All Files To Folder...");
            AppendMenuW(file,MF_STRING,RuntimeSaveDefaults,L"Runtime Save All To Defaults");
            AppendMenuW(file,MF_STRING,RuntimeRecovery,L"Runtime Recovery...");
            InsertMenuW(file,3,MF_BYPOSITION|MF_STRING,NewStyle,L"New Style");
            InsertMenuW(file,4,MF_BYPOSITION|MF_STRING,NewBand,L"New Band");
            InsertMenuW(file,5,MF_BYPOSITION|MF_STRING,NewDls,L"New DLS Collection");
            const auto window=CreateWindowW(c.lpszClassName,L"DirectMusic Producer — source reconstruction",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1000,800,nullptr,menu,instance,nullptr);
            if(!window){DestroyMenu(menu);throw std::runtime_error("Producer window creation failed");}ShowWindow(window,show);UpdateWindow(window);
            
            MSG message{};BOOL state;while((state=GetMessageW(&message,nullptr,0,0))>0){if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
            result=state<0?1:static_cast<int>(message.wParam);
        }
    } catch(const std::exception& e) {
        OutputDebugStringA(e.what());result=1;
        if(argv&&((argc==4&&(std::wstring(argv[1])==L"--prepare-runtime-envelope"||std::wstring(argv[1])==L"--prepare-runtime-default-audiopath"||std::wstring(argv[1])==L"--prepare-runtime-style"||std::wstring(argv[1])==L"--prepare-runtime-motif"))||(argc==5&&(std::wstring(argv[1])==L"--prepare-runtime-audiopath"||std::wstring(argv[1])==L"--prepare-runtime-save-as"||std::wstring(argv[1])==L"--prepare-runtime-settings"||std::wstring(argv[1])==L"--prepare-runtime-update"||std::wstring(argv[1])==L"--prepare-runtime-defaults"||std::wstring(argv[1])==L"--prepare-runtime-multifolder")))){
            try{const std::string error=e.what();write_file_atomic((std::filesystem::absolute(argv[2])/L"failure.txt").wstring(),Bytes(error.begin(),error.end()));}catch(...){/* Preserve the original failure result. */}
        }
    }
    if(argv)LocalFree(argv);return result;
}





