#include "band_mixer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace producer::app { namespace {
int span(int first,int last,int size){return std::max(1,last-first-size);}
int pixel(unsigned value,int length){return static_cast<int>((std::uint64_t(value)*length+63)/127);}
unsigned value(std::int64_t position,int length){return static_cast<unsigned>(std::clamp<std::int64_t>((position*127+length/2)/length,0,127));}
}
bool band_mixer_active(const BandInstrument& i){return (i.flags&0x60u)!=0;}
BandMixerPoint band_mixer_position(const BandInstrument& i,size_t ordinal,const BandMixerArea& a){
    const auto x=span(a.left,a.right,a.width),y=span(a.top,a.bottom,a.height);
    if(band_mixer_active(i))return {a.left+pixel(std::min(i.pan,127u),x),a.top+pixel(127-std::min(i.volume,127u),y)};
    // Inactive channels retain the original eight-column rack until edited.
    const auto rows=ordinal/8;return {a.left+int(ordinal%8)*(a.width+8),a.top+y/2-a.height*2+int(rows)*(a.height+4)};
}
BandMixValue band_mixer_value(std::uint32_t channel,BandMixerPoint p,const BandMixerArea& a){
    if(channel>=0xfffffffcu)throw std::runtime_error("Reserved mixer PChannel");
    return {channel,value(std::int64_t(p.x)-a.left,span(a.left,a.right,a.width)),127-value(std::int64_t(p.y)-a.top,span(a.top,a.bottom,a.height))};
}
std::vector<BandMixValue> band_mixer_drag(const std::vector<BandInstrument>& instruments,const std::vector<std::uint32_t>& selected,const BandMixerArea& a,int dx,int dy){
    std::vector<BandMixValue> result;for(const auto channel:selected){
        if(channel>=0xfffffffcu||std::any_of(result.begin(),result.end(),[&](const auto& v){return v.pchannel==channel;}))throw std::runtime_error("Invalid mixer selection");
        const auto found=std::find_if(instruments.begin(),instruments.end(),[&](const auto& i){return i.pchannel==channel;});if(found==instruments.end())throw std::runtime_error("Unknown mixer PChannel");
        auto p=band_mixer_position(*found,static_cast<size_t>(found-instruments.begin()),a);
        p.x=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(p.x)+dx,INT32_MIN,INT32_MAX));p.y=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(p.y)+dy,INT32_MIN,INT32_MAX));
        result.push_back(band_mixer_value(channel,p,a));
    }return result;
}
std::wstring band_mixer_pan_label(unsigned pan){if(pan>127)throw std::runtime_error("Invalid mixer pan");return pan<63?L"L"+std::to_wstring(63-pan):pan>63?L"R"+std::to_wstring(pan-63):L"Center";}
std::optional<std::vector<BandMixValue>> band_mixer_explicit_values(const std::vector<BandInstrument>& instruments,const std::vector<std::uint32_t>& selected){
    if(selected.empty())return {};
    std::vector<BandMixValue> result;
    for(const auto channel:selected){
        const auto found=std::find_if(instruments.begin(),instruments.end(),[&](const auto& i){return i.pchannel==channel;});
        if(channel>=0xfffffffcu||found==instruments.end()||(found->flags&0x60u)!=0x60u||found->pan>127||found->volume>127||
            std::any_of(result.begin(),result.end(),[&](const auto& v){return v.pchannel==channel;}))return {};
        result.push_back({channel,found->pan,found->volume});
    }
    return result;
}
std::vector<BandMixValue> band_mixer_changed_values(const std::vector<BandInstrument>& before,const std::vector<BandInstrument>& after){
    std::vector<BandMixValue> result;
    for(const auto& next:after){
        const auto old=std::find_if(before.begin(),before.end(),[&](const auto& i){return i.pchannel==next.pchannel;});
        if(old==before.end()||next.pchannel>=0xfffffffcu||(next.flags&0x60u)!=0x60u||next.pan>127||next.volume>127)continue;
        // A patch/DLS/structural history action needs a fresh playback snapshot.
        if(old->patch!=next.patch||old->collectionReference!=next.collectionReference||((old->flags^next.flags)&~0x60u))continue;
        if(old->pan!=next.pan||old->volume!=next.volume||((old->flags^next.flags)&0x60u))result.push_back({next.pchannel,next.pan,next.volume});
    }
    return result;
}
}
