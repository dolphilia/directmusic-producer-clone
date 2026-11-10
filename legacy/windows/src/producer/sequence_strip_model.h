#pragma once
#include "document.h"
#include "sequence_note_properties.h"
#include <optional>
#include <cmath>
#include <utility>
namespace producer::app {
// View state only. The fixed note area leaves CC lanes in place when zooming.
struct SequencePitchView {
    static constexpr int areaHeight=216;
    int rowHeight=12,topPitch=70;
    int visible_rows()const{return (areaHeight+rowHeight-1)/rowHeight;}
    void normalize(){rowHeight=std::clamp(rowHeight,2,32);topPitch=std::clamp(topPitch,visible_rows()-1,127);}
    void scroll(int semitones){topPitch=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(topPitch)+semitones,visible_rows()-1,127));}
    void zoom(int pixels){rowHeight=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(rowHeight)+pixels,2,32));normalize();}
    int scroll_position()const{return 127-topPitch;}
    void set_scroll_position(int position){topPitch=127-std::clamp(position,0,128-visible_rows());}
    void follow(unsigned pitch){const int p=static_cast<int>(std::min(pitch,127u));if(p>topPitch)topPitch=p;else if(p<=topPitch-visible_rows())topPitch=p+visible_rows()-1;normalize();}
    std::optional<unsigned> pitch_at(int y)const{if(y<0||y>=areaHeight)return {};return static_cast<unsigned>(topPitch-y/rowHeight);}
    std::pair<int,int> note_bounds(unsigned pitch,unsigned velocity)const{const int bottom=(topPitch-static_cast<int>(pitch)+1)*rowHeight;return {bottom-std::max(2,static_cast<int>(velocity)*rowHeight/127),bottom-1};}
    int drag_pitch(int pixels)const{return static_cast<int>(std::lround(double(pixels)/rowHeight));}
    int drag_velocity(int pixels)const{return static_cast<int>(std::lround(pixels*127./rowHeight/5))*5;}
};

enum class SequenceSnap {none,grid,beat,bar};
struct SequenceCursor {std::int32_t clock=0;unsigned pitch=60;SequenceSnap snap=SequenceSnap::grid;};
std::int32_t sequence_snap_clock(const Timeline&,std::int32_t raw,std::int32_t length,SequenceSnap);
std::int32_t sequence_grid_span(const Timeline&,std::int32_t clock);
std::int32_t sequence_default_duration(const Timeline&,std::int32_t clock,SequenceSnap);
bool move_sequence_cursor(const Timeline&,std::int32_t length,SequenceCursor&,int horizontal,int vertical);
enum class SequenceNoteHandle {move,start,end,velocity};
// Deltas are measured from the original note, never from intermediate previews.
Note sequence_drag_note(const Timeline&,std::int32_t length,Note original,SequenceNoteHandle,
    std::int64_t clockDelta,int pitchDelta,int velocityDelta,SequenceSnap);
struct SequenceVelocityOptions {bool limit=false,percent=true;int start=0,end=0;unsigned minimum=1,maximum=127;};
unsigned sequence_velocity_value(Note,std::int32_t first,std::int32_t last,const SequenceVelocityOptions&);
struct SequenceQuantizeOptions {bool start=true,duration=false;unsigned divisions=2,strength=100;};
// Nearest beat division; integer ties move forward. Strength truncates the signed
// delta towards zero, matching observed original division2 timing samples.
// Other divisions and boundary behavior remain unqualified original comparisons.
// An out-of-Segment result is rejected atomically.
Note sequence_quantize_note(const Timeline&,std::int32_t length,Note,const SequenceQuantizeOptions&);
struct SequencePartInfo {std::wstring name=L"Sequence";std::vector<std::uint32_t> channels;};
// Reads runtime channels and the track name. Opaque Producer UI chunks stay
// untouched; clone-created empty parts use versioned source-owned metadata.
SequencePartInfo sequence_part_info(const SegmentDocument&,size_t track);
struct SequenceBandContext {size_t track=0,event=0;std::uint32_t groups=1;BandEvent value;};
std::optional<SequenceBandContext> current_sequence_band(const SegmentDocument&,std::int32_t clock);
std::optional<BandInstrument> sequence_patch(const SegmentDocument&,std::int32_t clock,std::uint32_t channel);
// Updates just PATCH/BANKSELECT in the current owned Band; pan/volume validity
// and unrelated PChannels remain unchanged. One Segment undo transaction.
bool assign_sequence_gm_patch(SegmentDocument&,std::int32_t clock,std::uint32_t channel,std::uint32_t patch);
bool assign_sequence_dls_patch(SegmentDocument&,std::int32_t clock,std::uint32_t channel,const CollectionReference&,std::uint32_t bank,std::uint32_t program,bool emptyCollection=false);
// A private, short note and its effective Band. Never plays other source tracks
// or edits the source. Caller resolves only this snapshot's DLS dependencies.
Bytes sequence_audition_snapshot(const SegmentDocument&,Note);
}
