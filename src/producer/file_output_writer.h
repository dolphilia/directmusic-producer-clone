#pragma once
#include <windows.h>
#include <mmreg.h>
#include <cstdint>
#include <mutex>
#include <string>
namespace producer::app {
struct FileOutputStatus { bool active=false; HRESULT result=S_OK; std::uint64_t bytes=0,frames=0; std::wstring path; };
// Owns only an explicitly selected output. The audio input is never altered.
class FileOutputWriter {
    mutable std::mutex mutex_; HANDLE file_=INVALID_HANDLE_VALUE; WAVEFORMATEX format_{};
    FileOutputStatus status_; DWORD headerBytes_=0,dataLengthAt_=0,factFramesAt_=0;
    HRESULT write_locked(const void*,DWORD); HRESULT stop_locked();
public:
    FileOutputWriter()=default; ~FileOutputWriter();
    FileOutputWriter(const FileOutputWriter&)=delete; FileOutputWriter& operator=(const FileOutputWriter&)=delete;
    static bool valid_format(const WAVEFORMATEX&);
    HRESULT start(const std::wstring&,const WAVEFORMATEX&);
    HRESULT append(const void*,DWORD); HRESULT stop(); FileOutputStatus status() const;
};
}
