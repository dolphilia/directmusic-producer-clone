#pragma once
#include "conductor.h"
#include "style_player.h"
#include <optional>
namespace producer::app {
// Audition session for one owned Style and optional ChordMap, composed through
// the OS Composer (ComposeSegmentFromShape). Contract from StylePlayer.txt:
// Style/ChordMap/Shape/intro/end changes and Re-Compose start playback over;
// Band changes never restart; Motifs layer on the running performance.
class StylePlayerSession {
    Conductor& conductor_;HWND owner_;StyleCatalogEntry style_;std::vector<ResolvedCollection> collections_;std::vector<ChordMapCatalogEntry> maps_;std::optional<Bytes> chordMap_;StylePlayerSettings settings_;
    std::wstring band_;bool playing_=false;unsigned compositions_=0,restarts_=0,bandChanges_=0,motifs_=0;
    std::optional<StylePlayerComposition> composition_;
    PlaybackId primary_=0;std::vector<PlaybackId> layers_;
    void start();
    void restart_from_beginning();
public:
    StylePlayerSession(Conductor&,HWND owner,StyleCatalogEntry style,std::vector<ResolvedCollection> collections,std::optional<Bytes> chordMap={},StylePlayerSettings settings={},std::vector<ChordMapCatalogEntry> maps={});
    // Parameter edits start a new performance, including after Stop/natural end.
    void set_style(StyleCatalogEntry style,std::vector<ResolvedCollection> collections,std::optional<Bytes> chordMap,std::vector<ChordMapCatalogEntry> maps={});
    void set_chordmap(std::optional<Bytes>);
    void set_settings(const StylePlayerSettings&);
    void recompose();
    // Band: applied as an additional Band-only Segment; playback is not restarted.
    void select_band(const std::wstring& name);
    void play();void stop();
    void play_motif(const std::wstring& name);
    bool playing() const;
    PlaybackId primary_id() const {return primary_;}
    const std::wstring& band() const {return band_;}
    const StylePlayerSettings& settings() const {return settings_;}
    const StylePlayerComposition* composition() const {return composition_?&*composition_:nullptr;}
    std::vector<std::wstring> motif_names() const;std::vector<std::wstring> band_names() const;
    unsigned compositions() const {return compositions_;}unsigned restarts() const {return restarts_;}unsigned band_changes() const {return bandChanges_;}unsigned motif_requests() const {return motifs_;}
};
}
