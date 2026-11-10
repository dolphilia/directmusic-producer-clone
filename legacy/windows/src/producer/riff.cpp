#include "riff.h"
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace producer::app {
namespace {
constexpr size_t maxFile = 64 * 1024 * 1024;
Chunk parse_at(const Bytes& b, size_t& at, size_t end, unsigned depth, size_t& count) {
    if (depth > 64 || ++count > 100000 || at > end || end - at < 8) throw std::runtime_error("Invalid RIFF header or nesting");
    Chunk c; c.id.assign(reinterpret_cast<const char*>(b.data() + at), 4);
    const size_t n = read32(b, at + 4), start = at + 8;
    if (n > end - start) throw std::runtime_error("RIFF chunk exceeds container");
    const size_t stop = start + n;
    if (c.container()) {
        if (n < 4) throw std::runtime_error("RIFF container type missing");
        c.type.assign(reinterpret_cast<const char*>(b.data() + start), 4);
        size_t child = start + 4;
        while (child < stop) c.children.push_back(parse_at(b, child, stop, depth + 1, count));
    } else c.data.assign(b.begin() + start, b.begin() + stop);
    at = stop;
    if (n & 1) { if (at == end) throw std::runtime_error("RIFF padding missing"); c.padding = b[at++]; }
    return c;
}
}
std::uint32_t read32(const Bytes& b, size_t at) {
    if (at > b.size() || b.size() - at < 4) throw std::runtime_error("Missing DWORD");
    return std::uint32_t(b[at]) | (std::uint32_t(b[at+1]) << 8) | (std::uint32_t(b[at+2]) << 16) | (std::uint32_t(b[at+3]) << 24);
}
void put32(Bytes& b, size_t at, std::uint32_t v) {
    if (at > b.size() || b.size() - at < 4) throw std::runtime_error("Missing DWORD destination");
    for (unsigned i=0; i<4; ++i) b[at+i] = static_cast<std::uint8_t>(v >> (8*i));
}
Chunk* Chunk::find(const std::string& childId, const std::string& childType) {
    for (auto& c : children) if (c.id == childId && (childType.empty() || c.type == childType)) return &c;
    return nullptr;
}
const Chunk* Chunk::find(const std::string& childId, const std::string& childType) const {
    for (const auto& c : children) if (c.id == childId && (childType.empty() || c.type == childType)) return &c;
    return nullptr;
}
Chunk Chunk::parse(const Bytes& b) {
    if (b.size() > maxFile) throw std::runtime_error("RIFF file exceeds 64 MiB limit");
    size_t at=0, count=0; auto result=parse_at(b, at, b.size(), 0, count);
    if (at != b.size() || result.id != "RIFF") throw std::runtime_error("Expected one RIFF root");
    return result;
}
Bytes Chunk::encode() const {
    if (id.size()!=4 || (container() && type.size()!=4)) throw std::runtime_error("Invalid chunk identifier");
    Bytes payload=data;
    if (container()) {
        payload.assign(type.begin(), type.end());
        for (const auto& c:children) { auto b=c.encode(); if (b.size()>maxFile-payload.size()) throw std::runtime_error("RIFF size limit"); payload.insert(payload.end(),b.begin(),b.end()); }
    }
    if (payload.size()>maxFile-9) throw std::runtime_error("RIFF size limit");
    Bytes out(id.begin(),id.end()); out.resize(8); put32(out,4,static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(),payload.begin(),payload.end()); if (payload.size()&1) out.push_back(padding); return out;
}
Chunk Chunk::parse_list(const Bytes& b) {
    if(b.size()>maxFile)throw std::runtime_error("RIFF list exceeds size limit");size_t at=0,count=0;auto result=parse_at(b,at,b.size(),0,count);if(at!=b.size()||result.id!="LIST")throw std::runtime_error("Expected one RIFF LIST");return result;
}
Bytes read_file(const std::wstring& path) {
    std::ifstream f(std::filesystem::path(path),std::ios::binary|std::ios::ate);
    if (!f) throw InputFileReadError("Unable to open input file");
    const auto n=f.tellg(); if (n<0 || n>static_cast<std::streamoff>(maxFile)) throw std::runtime_error("Input size limit");
    Bytes b(static_cast<size_t>(n)); f.seekg(0); if (!b.empty() && !f.read(reinterpret_cast<char*>(b.data()),n)) throw InputFileReadError("Input read failed"); return b;
}
void write_file_atomic(const std::wstring& path, const Bytes& bytes) {
    const auto full=std::filesystem::absolute(path);
    // CREATE_NEW avoids clobbering another writer's temporary file.
    std::wstring temporary; HANDLE file=INVALID_HANDLE_VALUE;
    for (unsigned i=0;i<100 && file==INVALID_HANDLE_VALUE;++i) {
        // Keep the temporary name independent of the destination filename.
        // Appending it can exceed MAX_PATH even for a valid target path.
        temporary=(full.parent_path()/(L".producer-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(i)+L".tmp")).wstring();
        file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (file==INVALID_HANDLE_VALUE){const auto error=GetLastError();if(error!=ERROR_FILE_EXISTS)throw std::runtime_error("Unable to create save file; Windows error "+std::to_string(error)+"; destination: "+full.u8string());}
    }
    if (file==INVALID_HANDLE_VALUE) throw std::runtime_error("Save temporary names exhausted");
    DWORD written=0;
    DWORD error=ERROR_SUCCESS;const char* stage="write";
    if(bytes.size()>std::numeric_limits<DWORD>::max())error=ERROR_FILE_TOO_LARGE;
    else if(!WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr))error=GetLastError();
    else if(written!=bytes.size())error=ERROR_WRITE_FAULT;
    else if(!FlushFileBuffers(file)){stage="flush";error=GetLastError();}
    CloseHandle(file);
    if(!error&&!MoveFileExW(temporary.c_str(),full.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){stage="replace";error=GetLastError();}
    if(error){DeleteFileW(temporary.c_str());throw std::runtime_error("Atomic save failed at "+std::string(stage)+"; Windows error "+std::to_string(error)+"; destination retained: "+full.u8string());}
}
Bytes utf16(const std::wstring& s) {
    Bytes b; for (wchar_t c:s) { b.push_back(static_cast<std::uint8_t>(c)); b.push_back(static_cast<std::uint8_t>(c>>8)); } b.push_back(0); b.push_back(0); return b;
}
std::wstring decode_utf16(const Bytes& b) {
    if (b.size()%2 || b.size()<2 || b[b.size()-1] || b[b.size()-2]) throw std::runtime_error("Invalid UTF-16 string");
    std::wstring s; for (size_t i=0;i+2<b.size();i+=2) { const wchar_t c=static_cast<wchar_t>(b[i]|(b[i+1]<<8)); if (!c) throw std::runtime_error("Embedded NUL in path"); s.push_back(c); } return s;
}
}
