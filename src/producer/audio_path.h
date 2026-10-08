#pragma once
#include "riff.h"
#include <array>
#include <optional>
namespace producer::app {
using AudioBufferId=std::array<std::uint8_t,16>;
struct AudioPathRoute {std::uint32_t base,count,flags;std::vector<AudioBufferId> buffers;};
struct AudioPathPort {std::uint32_t base,count,flags;std::vector<AudioPathRoute> routes;};
struct AudioPathEffect {size_t buffer,index;AudioBufferId classId;std::uint32_t flags;AudioBufferId sendBuffer;};
struct AudioPathBuffer {AudioBufferId id;std::uint32_t flags;std::uint16_t channels;size_t synthBuses;bool routed;};
// Class declaration only. Callers must verify the registered system dsdmo.dll
// server before loading serialized effects; this never activates a factory.
bool is_declared_os_audio_effect(const AudioBufferId& classId);
class AudioPathDocument {
    Chunk root_;Bytes saved_;std::vector<Bytes> undo_,redo_;
    bool commit(Chunk next);
    bool add_default_effect(size_t buffer,const AudioBufferId& classId);
    bool insert_effect(size_t buffer,const AudioBufferId& classId,const AudioBufferId& sendBuffer,std::optional<std::int32_t> attenuation,size_t before,bool allowDuplicate);
public:
    AudioPathDocument();
    void load(const Bytes&);
    Bytes save_bytes() const {return root_.encode();}
    void save(const std::wstring&);
    bool dirty() const {return save_bytes()!=saved_;}
    std::wstring name() const;
    bool set_name(const std::wstring&);
    std::vector<AudioBufferId> buffers() const;
    std::vector<AudioPathBuffer> buffer_details() const;
    std::vector<AudioPathPort> ports() const;
    std::vector<AudioPathEffect> effects() const;
    bool add_file_output(size_t buffer);
    bool add_waves_reverb(size_t buffer);
    bool add_mixin_buffer(std::uint32_t channels);
    bool add_environmental_reverb_buffer();
    std::optional<size_t> environmental_reverb_buffer() const;
    std::optional<size_t> default_send_destination(size_t buffer) const;
    std::vector<size_t> available_send_destinations(size_t buffer) const;
    bool add_send(size_t buffer,size_t destination,size_t before=static_cast<size_t>(-1),std::int32_t attenuation=0);
    std::optional<std::int32_t> send_attenuation(size_t buffer,size_t effect) const;
    bool set_send_attenuation(size_t buffer,size_t effect,std::int32_t attenuation);
    std::vector<size_t> send_destinations(size_t buffer,size_t effect) const;
    bool set_send_destination(size_t buffer,size_t effect,size_t destination);
    bool set_route_buffers(size_t port,size_t route,const std::vector<AudioBufferId>&);
    bool set_port_range(size_t port,std::uint32_t base,std::uint32_t count);
    bool set_route_range(size_t port,size_t route,std::uint32_t base,std::uint32_t count);
    Bytes tool_graph() const;
    bool set_tool_graph(const Bytes&);
    bool remove_tool_graph();
    bool undo();bool redo();
};
// Download preparation only: retain source/Sequence/Style events, but omit
// disconnected instrument downloads in the private runtime Band copy.
Bytes prepare_audio_path_band_downloads(const Bytes& document,const Bytes& audioPath);
// Private runtime copy only: local Send destinations must be created first.
Bytes prepare_audio_path_send_runtime(const Bytes& audioPath);
struct AudioPathRecordingTarget {size_t buffer;std::uint32_t pchannel,stage,index;};
// Original document determines recording filenames; runtime copy determines
// global mix-in ordinals. No source bytes or routes are changed.
std::vector<AudioPathRecordingTarget> audio_path_file_output_targets(const Bytes& source,const Bytes& runtimeCopy);
// Owned Waves buffers use the same route/global lookup after Send reordering.
std::vector<AudioPathRecordingTarget> audio_path_waves_reverb_targets(const Bytes& source,const Bytes& runtimeCopy);
// The predefined native GUID owns the source DMO in the private global mix-in.
std::vector<AudioPathRecordingTarget> audio_path_environmental_reverb_targets(const Bytes& source,const Bytes& runtimeCopy);
}
