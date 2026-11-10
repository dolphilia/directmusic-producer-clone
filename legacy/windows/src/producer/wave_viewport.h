#pragma once
#include <algorithm>
#include <cstdint>

namespace producer::app {
// View coordinates never change a Wave document or its edit history.
struct WaveViewport {
    std::uint64_t total=0,first=0,count=0;
    static std::uint64_t proportion(std::uint64_t value,unsigned part,unsigned whole) {
        if(!whole)return 0;
        part=std::min(part,whole);
        return (value/whole)*part+(value%whole)*part/whole;
    }
    void fit(std::uint64_t frames){total=count=frames;first=0;}
    void synchronize(std::uint64_t frames) {
        const bool fitted=first==0&&count==total;
        total=frames;
        if(fitted||!count){fit(frames);return;}
        count=std::min(count,total);first=std::min(first,total-count);
    }
    bool zoom(bool inward,std::uint64_t cursor) {
        if(!total)return false;
        const auto next=inward?std::max<std::uint64_t>(1,count/2+count%2):count>=total-count?total:count*2;
        if(next==count)return false;
        const auto anchor=cursor>=first&&cursor<=first+count?cursor:first+count/2;
        const auto offset=anchor-first;
        // Keep the anchor at the same relative location without multiplying
        // large absolute frame values; expansion saturates at the Wave ends.
        const auto relative=inward?offset/2:offset>UINT64_MAX/2?next:offset*2;
        first=anchor>=relative?anchor-relative:0;count=next;first=std::min(first,total-count);
        return true;
    }
    bool scroll_to(std::uint64_t frame) {
        const auto next=std::min(frame,total-count);
        if(next==first)return false;
        first=next;return true;
    }
    bool scroll(bool forward,bool page) {
        const auto step=std::max<std::uint64_t>(1,page?count-count/10:count/10);
        return scroll_to(forward?first+std::min(step,total-count-first):first-std::min(first,step));
    }
    std::uint64_t frame_at(int pixel,int width)const {
        const auto w=static_cast<unsigned>(std::max(1,width));
        return first+proportion(count,static_cast<unsigned>(std::clamp(pixel,0,static_cast<int>(w))),w);
    }
    int pixel_at(std::uint64_t frame,int width)const {
        if(!count)return 0;
        frame=std::clamp(frame,first,first+count)-first;
        const auto w=static_cast<unsigned>(std::max(1,width));
        unsigned low=0,high=w;
        while(low<high){
            const auto mid=low+(high-low+1)/2;
            const auto threshold=proportion(count,mid,w)+((count%w)*mid%w?1:0);
            if(threshold<=frame)low=mid;else high=mid-1;
        }
        return static_cast<int>(low);
    }
};
}
