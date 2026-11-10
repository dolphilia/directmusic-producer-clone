#pragma once
#include <windows.h>
#include <array>
#include <optional>
#include <commctrl.h>
#include <string>
#include <vector>

namespace producer::app {
struct StandardToolbarCommands {
    UINT newFile,openProject,insertFiles,saveProject,saveFile,projectTree,tileHorizontal,tileVertical;
};
// Commands are owned by the frame, shared with its menus and accelerators.
// Toolbar visibility is view state, never document history.
class StandardToolbar {
    HWND window_=nullptr;
    StandardToolbarCommands commands_{};
    bool visible_=true;
public:
    void create(HWND parent,UINT controlId,const StandardToolbarCommands&);
    int height()const{return window_&&visible_?30:0;}
    bool visible()const{return visible_;}
    bool owns(HWND child)const{return window_&&(child==window_||IsChild(window_,child));}
    void show(bool);
    void layout(int width);
    void refresh(bool canSaveFile,bool projectTreeVisible,bool canTile);
    POINT new_menu_position()const;
};
struct TransportToolbarCommands {UINT play,stop,tempoModifier,fromStart,boundaryStop,tempoEdit,tempoSpin,tempoCommit,tempoCancel,primaryList;};
class TransportToolbar {
    HWND window_=nullptr,tempo_=nullptr,tempoSpin_=nullptr,primaryList_=nullptr;
    std::vector<std::wstring> primaryLabels_;
    std::optional<double> displayedTempo_;
    bool noPrimary_=true;
    int top_=0;
    int playImage_=0,stopImage_=0;
    TransportToolbarCommands commands_{};
    bool visible_=true;
    void layout_tempo();
public:
    void create(HWND parent,UINT controlId,const TransportToolbarCommands&);
    int height()const{return window_&&visible_?30:0;}
    bool visible()const{return visible_;}
    bool owns(HWND child)const{return child&&(child==tempo_||child==tempoSpin_||child==primaryList_||(window_&&(child==window_||IsChild(window_,child))));}
    void show(bool);
    void layout(int width,int top);
    void refresh(bool canPlay,bool playing,bool modifierEnabled,unsigned percent,std::optional<double> tempo,bool restorePending=false);
    void refresh_primary_list(const std::vector<std::wstring>&,std::optional<size_t> selected,bool enabled);
    std::optional<size_t> primary_selection()const;
    bool tempo_focused()const{return tempo_&&GetFocus()==tempo_;}
    bool modifier_context(const NMHDR&)const;
    bool translate(const MSG&)const;
    bool spin(const NMHDR&);
    std::optional<double> tempo_value()const;
    void restore_tempo();
};
std::optional<unsigned> show_alternate_tempo(HWND owner,unsigned percent);
struct TransportOptionsCommands {UINT audioPath,midiOptions,metronome,playCursor,midiEcho;};
class TransportOptionsToolbar {
    HWND window_=nullptr,paths_=nullptr;bool visible_=true;
    TransportOptionsCommands commands_{};
    std::vector<std::wstring> labels_;
public:
    void create(HWND parent,UINT controlId,const TransportOptionsCommands&);
    int height()const{return window_&&visible_?30:0;}
    bool visible()const{return visible_;}
    bool owns(HWND child)const{return child==paths_||(window_&&(child==window_||IsChild(window_,child)));}
    void show(bool);
    void layout(int width,int top);
    void refresh(const std::vector<std::wstring>&,size_t selected,bool metronome=false,bool playCursor=true,bool midiEcho=false);
    bool metronome_context(const NMHDR&)const;
    size_t selection()const;
};
}
