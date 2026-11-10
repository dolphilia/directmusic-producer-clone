#pragma once
#include "framework.h"
#include "chord_keyboard.h"
#include "embedded_band_target.h"
#include <functional>
#include <map>

namespace producer::app {
struct LyricPropertiesTarget {
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
struct MarkerPropertiesTarget {
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
struct MeterPropertiesTarget {
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
struct ChordPropertiesTarget {
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
struct SequenceNotePropertiesTarget {
    size_t document=0,track=0,event=0;
    std::uint32_t groups=1;
};
// One shared modeless view. Targets are owned indexes, not editor pointers.
// Segment/Style names and the selected Tempo marker share the same window.
class PropertiesWindow {
    HWND owner_=nullptr,window_=nullptr,tabs_=nullptr,name_=nullptr,label_=nullptr;
    Framework* framework_=nullptr;
    std::function<void()> changed_;
    std::optional<OpenedDocument> target_;
    std::optional<size_t> tempoDocument_;
    std::optional<SequenceNotePropertiesTarget> sequenceTarget_;
    std::function<void(size_t)> sequenceSelectionChanged_;
    HWND sequenceFields_[17]{},sequenceLabels_[11]{},sequenceSpins_[17]{};
    std::optional<LyricPropertiesTarget> lyricTarget_;
    std::function<void(size_t)> lyricSelectionChanged_;
    std::optional<MarkerPropertiesTarget> markerTarget_;
    std::function<void(size_t)> markerSelectionChanged_;
    std::optional<MeterPropertiesTarget> meterTarget_;
    std::function<void(size_t)> meterSelectionChanged_;
    std::optional<EmbeddedBandTarget> bandTarget_;
    std::function<void(const EmbeddedBandTarget&)> bandSelectionChanged_;
    HWND bandFields_[6]{},bandLabels_[6]{},bandSpins_[5]{};
    std::optional<ChordPropertiesTarget> chordTarget_;
    std::function<void(size_t)> chordSelectionChanged_;
    HWND chordType_=nullptr,chordLabel_=nullptr;
    HWND chordRoot_=nullptr,chordSpin_=nullptr,chordNotation_[2]{},chordLevels_[4]{};
    ChordKeyboard chordKeys_[4],scaleKeys_[4];
    bool chordFlats_=false;
    HWND segmentLengthLabel_=nullptr,segmentLengthButton_=nullptr;
    HWND segmentStartFields_[4]{},segmentStartSpins_[4]{};
    HFONT segmentStartFont_=nullptr;
    std::vector<HWND> segmentStartControls_;
    HWND segmentInfoFields_[8]{};
    std::vector<HWND> segmentInfoControls_;
    HWND segmentBoundaryName_=nullptr,segmentBoundaryMarker_=nullptr,segmentBoundaryFallback_=nullptr,segmentBoundaryDefault_=nullptr,segmentBoundaryList_=nullptr,segmentBoundaryTiming_[2]{};
    HWND segmentInvalidation_[3]{};
    HWND segmentAlignmentDescription_=nullptr,segmentAlignment_[4]{};
    std::vector<HWND> segmentBoundaryControls_;
    HWND loopFields_[9]{},loopSpins_[9]{},loopInfinite_=nullptr,loopReset_=nullptr,loopClock_=nullptr,loopClockFields_[2]{};
    std::vector<HWND> loopControls_;
    std::map<size_t,std::uint32_t> finiteLoopRepeats_;
    int tabFamily_=0;
    HWND meterButton_=nullptr,meterMeasure_=nullptr,meterLabel_=nullptr,meterSpin_=nullptr;
    HWND tempoFields_[4]{},tempoLabels_[4]{},tempoSpins_[4]{};
    HWND lyricFields_[6]{},lyricLabels_[6]{},lyricSpins_[5]{},lyricTiming_[3]{},lyricTimingGroup_=nullptr;
    HWND markerFields_[5]{},markerLabels_[5]{},markerSpins_[4]{};
    HWND waveFields_[8]{},waveLabels_[8]{},waveChecks_[4]{},waveSpins_[4]{};
    std::map<size_t,std::vector<WaveSampleLoop>> disabledWaveLoops_;
    bool waveLoopLocked_=false;
    bool finalizing_=false;
    bool updating_=false;
    static LRESULT CALLBACK proc(HWND,UINT,WPARAM,LPARAM);
    void synchronize(bool selectionChanged=false);
    void clear_page_background();
    void ensure_window();
    void edit_tempo(UINT field,bool final,int delta=0);
    void edit_sequence(UINT field,int delta=0);
    void edit_lyric(UINT field,int delta=0);
    void edit_band(UINT field,int delta=0);
    void edit_marker(UINT field,int delta=0);
    void edit_meter_measure(int delta=0);
    void edit_meter_signature();
    void edit_segment_length();
    void edit_segment_start(UINT field,int delta=0);
    void edit_segment_info(UINT field);
    void edit_segment_boundary(UINT field);
    void edit_segment_loop(UINT field,int delta=0);
    void edit_chord_type();
    void edit_chord_structure(unsigned level,unsigned note,bool scale);
    void transpose_chord_root(int delta);
    void edit_wave(UINT field,int delta=0);
    std::wstring target_name()const;
public:
    void initialize(HWND owner,Framework& host,std::function<void()> changed);
    bool supports(const std::optional<OpenedDocument>&)const;
    void show(std::optional<OpenedDocument>);
    void follow(std::optional<OpenedDocument>);
    bool supports_tempo(size_t)const;
    void show_tempo(size_t);
    void follow_tempo(size_t);
    bool supports_sequence(const SequenceNotePropertiesTarget&)const;
    void show_sequence(SequenceNotePropertiesTarget,std::function<void(size_t)>);
    void follow_sequence(SequenceNotePropertiesTarget,std::function<void(size_t)>);
    bool supports_lyric(const LyricPropertiesTarget&)const;
    void show_lyric(LyricPropertiesTarget,std::function<void(size_t)> selectionChanged);
    void follow_lyric(LyricPropertiesTarget,std::function<void(size_t)> selectionChanged);
    bool supports_marker(const MarkerPropertiesTarget&)const;
    void show_marker(MarkerPropertiesTarget,std::function<void(size_t)> selectionChanged);
    void follow_marker(MarkerPropertiesTarget,std::function<void(size_t)> selectionChanged);
    bool supports_meter(const MeterPropertiesTarget&)const;
    void show_meter(MeterPropertiesTarget,std::function<void(size_t)> selectionChanged);
    void follow_meter(MeterPropertiesTarget,std::function<void(size_t)> selectionChanged);
    bool supports_chord(const ChordPropertiesTarget&)const;
    void show_chord(ChordPropertiesTarget,std::function<void(size_t)> selectionChanged);
    void follow_chord(ChordPropertiesTarget,std::function<void(size_t)> selectionChanged);
    bool supports_band(const EmbeddedBandTarget&)const;
    void show_band(EmbeddedBandTarget,std::function<void(const EmbeddedBandTarget&)>);
    void follow_band(EmbeddedBandTarget,std::function<void(const EmbeddedBandTarget&)>);
    bool translate(MSG&);
    bool focus_existing()const;
    void release();
    bool owns(HWND child)const{return window_&&(child==window_||IsChild(window_,child));}
};
}
