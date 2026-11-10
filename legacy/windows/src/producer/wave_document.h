#pragma once
#include "riff.h"
#include <optional>
#include <array>
namespace producer::app {
struct WaveFormat {unsigned tag,channels,sampleRate,blockAlign,bits;std::uint64_t frames;};
struct ResolvedWave {std::wstring path;Bytes bytes;WaveFormat format;};
struct WaveSampleLoop {
    std::uint32_t type=0,start=0,length=1;
    bool operator==(const WaveSampleLoop& other)const{return type==other.type&&start==other.start&&length==other.length;}
};
class WaveDocument {
    struct CloneOrigin {size_t index;WaveSampleLoop loop;};
    struct History {Bytes bytes;std::optional<CloneOrigin> clone;};
    Chunk root_;Bytes saved_;std::vector<History> undo_,redo_;
    std::optional<CloneOrigin> cloneOrigin_;
    bool commit(Chunk,std::optional<CloneOrigin> = {});
public:
    void load(const Bytes&);
    Bytes save_bytes()const{return root_.encode();}
    void save(const std::wstring&);
    bool dirty()const{return save_bytes()!=saved_;}
    WaveFormat format()const;
    std::optional<std::array<std::uint8_t,16>> identity()const;
    std::wstring name()const;
    unsigned root_note()const;
    bool set_root_note(unsigned);
    bool set_name(const std::wstring&);
    bool set_properties(const std::wstring&,unsigned);
    std::vector<WaveSampleLoop> sample_loops()const;
    bool set_sample_loops(const std::vector<WaveSampleLoop>&);
    bool remove_sample_loop(size_t index);
    // Frame ranges are half-open; clipboard bytes are a minimal RIFF WAVE.
    Bytes copy_pcm(std::uint64_t begin,std::uint64_t end)const;
    bool erase_pcm(std::uint64_t begin,std::uint64_t end);
    bool paste_pcm(std::uint64_t insertion,const Bytes& wave);
    std::uint64_t nearest_zero_frame(std::uint64_t frame)const;
    bool find_best_sample_loop_end(size_t index,std::uint64_t begin,std::uint64_t end);
    bool clone_sample_loop(size_t index);
    bool undo();bool redo();
    bool can_undo()const{return !undo_.empty();}
    bool can_redo()const{return !redo_.empty();}
};
}
