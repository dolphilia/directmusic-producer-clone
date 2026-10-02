#pragma once
#include <windows.h>
#include <objidl.h>

namespace producer::host {
// Only recovered slots are callable here. These adapters do not invent
// signatures for the rest of Framework or PersistInfo.
struct StreamInfo { DWORD fileType; GUID format; IUnknown* directoryNode; };
static_assert(sizeof(StreamInfo) == 24);
inline HRESULT alloc_memory_stream(IUnknown* framework, DWORD type, GUID format, IStream** out) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, GUID, IStream**);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(framework))[19])(framework, type, format, out);
}
inline HRESULT get_stream_info(IUnknown* metadata, StreamInfo* out) {
    using Call = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, StreamInfo*);
    return reinterpret_cast<Call>((*reinterpret_cast<void***>(metadata))[4])(metadata, out);
}
}
