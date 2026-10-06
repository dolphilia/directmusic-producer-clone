#pragma once
#include "riff.h"
namespace producer::app {
struct LyricEvent {
    std::int32_t physical=0,logical=0;
    std::uint32_t timing=16;
    std::wstring text;
};
bool valid_lyric(const LyricEvent&);
std::vector<LyricEvent> lyric_events(const Chunk&);
bool add_lyric_event(Chunk&,const LyricEvent&,size_t* =nullptr);
bool change_lyric_event(Chunk&,size_t,const LyricEvent&,size_t* =nullptr);
bool delete_lyric_event(Chunk&,size_t);
Bytes copy_lyric_event(const Chunk&,size_t);
bool paste_lyric_event(Chunk&,const Bytes&,std::int32_t physical,std::int32_t logical,size_t* =nullptr);
Chunk lyric_track();
Bytes copy_lyric_range(const Chunk&,std::int32_t begin,std::int32_t end);
bool delete_lyric_range(Chunk&,std::int32_t begin,std::int32_t end);
bool lyric_range_empty(const Bytes&,std::int32_t at,std::int32_t span,std::int32_t length);
bool paste_lyric_range(Chunk&,const Bytes&,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length);
}
