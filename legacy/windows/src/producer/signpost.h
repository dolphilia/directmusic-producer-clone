#pragma once
#include "riff.h"
namespace producer::app {
struct SignpostEvent {std::int32_t time=0;std::uint32_t chords=0x100;std::uint16_t measure=0;};
bool valid_signpost(SignpostEvent,const SignpostEvent* previous=nullptr);
std::vector<SignpostEvent> signpost_events(const Bytes&);
bool set_signpost_event(Bytes&,SignpostEvent,size_t* resultingIndex=nullptr);
bool change_signpost_event(Bytes&,size_t,SignpostEvent,size_t* resultingIndex=nullptr);
bool delete_signpost_event(Bytes&,size_t);
Chunk signpost_track();
}
