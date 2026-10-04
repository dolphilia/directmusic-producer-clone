#pragma once
#include "riff.h"
#include <array>
namespace producer::app {
using AudioBufferId=std::array<std::uint8_t,16>;
struct AudioPathRoute {std::uint32_t base,count,flags;std::vector<AudioBufferId> buffers;};
struct AudioPathPort {std::uint32_t base,count,flags;std::vector<AudioPathRoute> routes;};
class AudioPathDocument {
    Chunk root_;Bytes saved_;std::vector<Bytes> undo_,redo_;
    bool commit(Chunk next);
public:
    AudioPathDocument();
    void load(const Bytes&);
    Bytes save_bytes() const {return root_.encode();}
    void save(const std::wstring&);
    bool dirty() const {return save_bytes()!=saved_;}
    std::wstring name() const;
    bool set_name(const std::wstring&);
    std::vector<AudioBufferId> buffers() const;
    std::vector<AudioPathPort> ports() const;
    bool set_route_buffers(size_t port,size_t route,const std::vector<AudioBufferId>&);
    bool set_port_range(size_t port,std::uint32_t base,std::uint32_t count);
    bool set_route_range(size_t port,size_t route,std::uint32_t base,std::uint32_t count);
    bool undo();bool redo();
};
}
