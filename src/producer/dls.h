#pragma once
#include "riff.h"
#include <optional>

namespace producer::app {
struct DlsRegion {unsigned keyLow,keyHigh,velocityLow,velocityHigh,keyGroup;std::uint32_t tableIndex;};
struct DlsInstrument {std::uint32_t bank,program;std::vector<DlsRegion> regions;};
struct DlsWave {unsigned format,channels,bits,blockAlign;std::uint32_t sampleRate;size_t frames;};
struct DlsLoop {std::uint32_t type,start,length;};
// Native CONNECTION fields. Scale is a signed raw DLS value; its units depend
// on the destination. Do not apply a universal seconds or decibels conversion.
struct DlsConnection {std::uint16_t source,control,destination,transform;std::int32_t scale;};
struct DlsArticulation {std::string listType,chunkId;std::vector<DlsConnection> connections;};
// Preserve every RIFF chunk. Typed operations reject ambiguous or unsupported
// layouts before changing the owned document. Pool edits relocate ptbl offsets
// while retaining existing Region cue indices.
class DlsDocument {
    Chunk root_;Bytes saved_;std::vector<Bytes> undo_,redo_;
    bool adopt(Chunk next);
public:
    static DlsDocument create();
    void load(const Bytes& bytes);
    Bytes save_bytes() const {return root_.encode();}
    void save(const std::wstring& path);
    bool dirty() const {return save_bytes()!=saved_;}
    std::vector<DlsInstrument> instruments() const;
    std::vector<DlsWave> waves() const;
    // A missing Region selects Instrument ownership. Blocks retain their
    // explicit list/chunk level and order; this does not flatten inheritance
    // or evaluate Level 2 conditional chunks.
    std::vector<DlsArticulation> articulations(size_t instrument,std::optional<size_t> region={}) const;
    bool set_articulation_connections(size_t instrument,std::optional<size_t> region,size_t block,const std::vector<DlsConnection>& values);
    bool add_articulation(size_t instrument,std::optional<size_t> region,const DlsArticulation& value);
    // Playback preflight is read-only; validate all Wave/Region loop bounds
    // against the linked sample before replacing a live performance.
    void validate_playback_samples() const;
    // WSMP start/length use sample frames, with an exclusive end. Region
    // queries expose only explicit overrides, never manufacture defaults.
    std::vector<DlsLoop> wave_loops(size_t wave) const;
    std::vector<DlsLoop> region_loops(size_t instrument,size_t region) const;
    bool set_wave_loop(size_t wave,size_t loop,const DlsLoop& value);
    bool set_region_loop(size_t instrument,size_t region,size_t loop,const DlsLoop& value);
    // Replace active loops, preserving retained record extensions and opaque
    // header/tail bytes. Region creation copies the Wave sample defaults.
    bool set_wave_loops(size_t wave,const std::vector<DlsLoop>& loops);
    bool set_region_loops(size_t instrument,size_t region,const std::vector<DlsLoop>& loops);
    bool region_inherits_wave_sample(size_t instrument,size_t region) const;
    std::vector<DlsLoop> effective_region_loops(size_t instrument,size_t region) const;
    // Removes the entire explicit WSMP override (root/tuning/attenuation and
    // loops). Undo restores it; this is not just a loop-only inheritance flag.
    bool inherit_wave_sample(size_t instrument,size_t region);
    bool set_instrument(size_t index,std::uint32_t bank,std::uint32_t program);
    // Explicit locale/cue; one full-range mono PCM Region with observed
    // Producer defaults. An empty name retains locale-derived naming.
    bool create_instrument(std::uint32_t bank,std::uint32_t program,const std::string& name,std::uint32_t cue);
    bool set_region(size_t instrument,size_t region,const DlsRegion& value);
    bool create_region(size_t instrument,const DlsRegion& value);
    bool duplicate_region(size_t instrument,size_t region);
    bool remove_region(size_t instrument,size_t region);
    bool duplicate_wave(size_t index);
    // Removing a referenced Wave requires an explicit surviving pool cue.
    // Preserve Region sample overrides; reject incompatible PCM/loop bounds.
    bool remove_wave(size_t index,std::optional<std::uint32_t> replacementCue={});
    bool scale_wave(size_t index,unsigned percent);
    Bytes export_wave_pcm(size_t index) const;
    bool import_wave_pcm(size_t index,const Bytes& wav);
    // Append a portable PCM Wave and stable new pool cue with an ASCII name.
    bool add_wave_pcm(const Bytes& wav,const std::string& name);
    bool undo();bool redo();
};
}
