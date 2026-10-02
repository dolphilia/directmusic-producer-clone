#include "tempo/tempo_track.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

static std::vector<std::uint8_t> read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Missing reference stream");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        const std::filesystem::path directory(argv[1]);
        producer::tempo::Track track;
        bool ok = track.save() == read(directory / "initial-stream.bin");
        for (const auto* name : {"single", "fractional", "multiple", "unsorted", "duplicate", "replace"}) {
            const bool loaded = track.load(read(directory / (std::string(name) + "-input.bin"))) == producer::tempo::LoadResult::ok;
            const bool saved = loaded && track.save() == read(directory / (std::string(name) + "-output.bin"));
            ok = ok && saved;
            std::cout << "{\"case\":\"" << name << "\",\"saved_bytes_equal\":" << (saved ? "true" : "false") << "}\n";
            // This is compared with independent original editor GetParam logs,
            // not a second implementation of the expected lookup algorithm.
            for (int at : {0, 1, 768, 769, 1536, 1537, 3072}) {
                const auto value = track.query(at);
                std::cout << "{\"case\":\"" << name << "\",\"at\":" << at << ",\"found\":" << (value.found ? "true" : "false")
                    << ",\"time\":" << value.eventTime << ",\"tempo\":" << value.bpm << ",\"next\":" << value.next << "}\n";
            }
        }
        for (const auto* name : {"property-edit", "delete"}) {
            const auto reference = read(directory / (std::string(name) + "-output.bin"));
            const bool same = track.load(reference) == producer::tempo::LoadResult::ok && track.save() == reference;
            ok = ok && same;
            std::cout << "{\"case\":\"" << name << "\",\"saved_bytes_equal\":" << (same ? "true" : "false") << "}\n";
        }
        const auto empty = track.query(0);
        ok = ok && !empty.found && empty.bpm == 120.0;
        // Robustness checks describe our parser contract, not reference behavior.
        const auto before = track.save();
        auto truncated = before; truncated.pop_back();
        ok = track.load(truncated) == producer::tempo::LoadResult::malformed && track.save() == before && ok;
        auto unknown = before; unknown[8] = 0;
        ok = track.load(unknown) == producer::tempo::LoadResult::unsupported && track.save() == before && ok;
        std::cout << "{\"passed\":" << (ok ? "true" : "false") << "}\n";
        return ok ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
