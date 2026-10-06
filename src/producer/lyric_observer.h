#pragma once
#include "compat/playback_runtime.h"
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <dmerror.h>

namespace producer::app {
struct PlaybackLyric { LONG clocks; DWORD channel,group; std::wstring text; bool visible;DWORD payloadBytes=0,flags=0;std::array<BYTE,64> payloadPrefix{}; };
// Realtime callback storage owns values only. No allocation, UI calls or COM
// references escape ProcessPMsg; the UI thread copies completed entries.
class LyricObserver final:public runtime::Tool {
    struct Entry {LONG clocks;DWORD channel,group;size_t length;bool visible;DWORD payloadBytes,flags;std::array<BYTE,64> payloadPrefix;std::array<wchar_t,1024> text;};
    DWORD mediaType_=13,delivery_=16;bool allVisible_=false;
    volatile LONG references_=1,forwardingFailed_=0;
    mutable SRWLOCK lock_=SRWLOCK_INIT;
    std::array<Entry,1024> entries_{};
    std::array<DWORD,128> channels_{};
    size_t count_=0,channelCount_=0;bool overflow_=false,malformed_=false;
public:
    explicit LyricObserver(DWORD mediaType=13,DWORD delivery=16,bool visible=false):mediaType_(mediaType),delivery_(delivery),allVisible_(visible){}
    void set_visible(bool visible){AcquireSRWLockExclusive(&lock_);allVisible_=visible;ReleaseSRWLockExclusive(&lock_);}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==runtime::toolId){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&references_);}
    ULONG STDMETHODCALLTYPE Release()override{const auto n=InterlockedDecrement(&references_);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(runtime::Graph*)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* out)override{if(!out)return E_POINTER;*out=delivery_;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* out)override{if(!out)return E_POINTER;*out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** out,DWORD n)override{if(!out||!*out)return E_POINTER;if(n!=1)return E_INVALIDARG;**out=mediaType_;return S_OK;}
    void include_channel(DWORD channel){AcquireSRWLockExclusive(&lock_);bool found=false;for(size_t i=0;i<channelCount_;++i)found=found||channels_[i]==channel;if(!found){if(channelCount_<channels_.size())channels_[channelCount_++]=channel;else overflow_=true;}ReleaseSRWLockExclusive(&lock_);}
    HRESULT STDMETHODCALLTYPE ProcessPMsg(runtime::Performance*,runtime::Message* p)override{
        if(!p)return E_POINTER;if(p->flags&0x20)return DMUS_S_FREE;
        if(p->type==mediaType_){
            const size_t bytes=p->size>=sizeof(runtime::Message)?p->size-sizeof(runtime::Message):0;
            const auto* text=reinterpret_cast<const wchar_t*>(reinterpret_cast<const BYTE*>(p)+sizeof(runtime::Message));
            size_t length=0;bool valid=bytes>=2&&bytes%2==0;
            if(valid){const auto limit=(std::min)(bytes/2,size_t{1024});while(length<limit&&text[length])++length;valid=length<limit;
                for(size_t i=0;valid&&i<length;++i){const auto c=text[i];if(c>=0xd800&&c<=0xdbff){valid=i+1<length&&text[i+1]>=0xdc00&&text[i+1]<=0xdfff;++i;}else if(c>=0xdc00&&c<=0xdfff)valid=false;}}
            AcquireSRWLockExclusive(&lock_);
            if(!valid)malformed_=true;else if(count_==entries_.size())overflow_=true;else{auto& e=entries_[count_++];e.clocks=p->musicTime;e.channel=p->pchannel;e.group=p->group;e.length=length;e.payloadBytes=static_cast<DWORD>(bytes);e.flags=p->flags;e.payloadPrefix.fill(0);std::copy_n(reinterpret_cast<const BYTE*>(text),(std::min)(bytes,e.payloadPrefix.size()),e.payloadPrefix.begin());e.visible=allVisible_;for(size_t i=0;i<channelCount_;++i)e.visible=e.visible||channels_[i]==p->pchannel;std::copy_n(text,length,e.text.begin());}
            ReleaseSRWLockExclusive(&lock_);
        }
        if(p->graph&&SUCCEEDED(p->graph->StampPMsg(p)))return DMUS_S_REQUEUE;
        InterlockedExchange(&forwardingFailed_,1);return DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(runtime::Performance*,runtime::Message* p,LONGLONG)override{return p?DMUS_S_FREE:S_OK;}
    std::vector<PlaybackLyric> snapshot()const{std::vector<PlaybackLyric> out;AcquireSRWLockShared(&lock_);try{out.reserve(count_);for(size_t i=0;i<count_;++i){const auto& e=entries_[i];out.push_back({e.clocks,e.channel,e.group,std::wstring(e.text.data(),e.length),e.visible,e.payloadBytes,e.flags,e.payloadPrefix});}}catch(...){ReleaseSRWLockShared(&lock_);throw;}ReleaseSRWLockShared(&lock_);return out;}
    bool failed()const{AcquireSRWLockShared(&lock_);const bool result=overflow_||malformed_;ReleaseSRWLockShared(&lock_);return result||InterlockedCompareExchange(const_cast<volatile LONG*>(&forwardingFailed_),0,0)!=0;}
};
}
