#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace producer::app {
using Bytes = std::vector<std::uint8_t>;
// An owned RIFF tree retains unknown payloads, order, and original padding.
struct Chunk {
    std::string id, type;
    Bytes data;
    std::vector<Chunk> children;
    std::uint8_t padding = 0;
    bool container() const { return id == "RIFF" || id == "LIST"; }
    Chunk* find(const std::string& childId, const std::string& childType = "");
    const Chunk* find(const std::string& childId, const std::string& childType = "") const;
    Bytes encode() const;
    static Chunk parse(const Bytes& bytes);
    static Chunk parse_list(const Bytes& bytes);
};
std::uint32_t read32(const Bytes& bytes, size_t offset);
void put32(Bytes& bytes, size_t offset, std::uint32_t value);
Bytes read_file(const std::wstring& path);
void write_file_atomic(const std::wstring& path, const Bytes& bytes);
Bytes utf16(const std::wstring& text);
std::wstring decode_utf16(const Bytes& bytes);
}
