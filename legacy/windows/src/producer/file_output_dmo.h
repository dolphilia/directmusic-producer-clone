#pragma once
#include "file_output_writer.h"
#include <mediaobj.h>
#include <memory>
namespace producer::app {
inline constexpr GUID fileOutputClass={0x2d6d1411,0xdcd7,0x45e7,{0xad,0xde,0xac,0xac,0x85,0xa2,0x42,0x5d}};
inline constexpr GUID fileOutputRuntimeClass={0xc1cb99dc,0x6c09,0x4643,{0x9a,0x95,0x6f,0x4b,0xf4,0x9c,0x74,0xf1}};
inline constexpr GUID fileOutputControlId={0xe13dce68,0x47d1,0x4b77,{0xae,0xba,0xbf,0xac,0x78,0xf7,0xaa,0x17}};
// The first six control slots follow the statically identified original ABI.
// GetFilename is a legacy caller-sized buffer; source callers use status().
struct FileOutputControl: IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetFilename(const WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetFilename(WCHAR*)=0;
    virtual HRESULT STDMETHODCALLTYPE GetOption(DWORD*)=0;
    virtual HRESULT STDMETHODCALLTYPE SetOption(DWORD)=0;
    virtual HRESULT STDMETHODCALLTYPE Start()=0;
    virtual HRESULT STDMETHODCALLTYPE Stop()=0;
};
IMediaObject* create_file_output_dmo(); // transfers one reference
// Process-local registration, no registry writes or original activation.
class FileOutputRegistration {
    DWORD cookie_=0;
public:
    FileOutputRegistration();~FileOutputRegistration();
    FileOutputRegistration(const FileOutputRegistration&)=delete;
    FileOutputRegistration& operator=(const FileOutputRegistration&)=delete;
};
}
