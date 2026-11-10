#include "file_output_writer.h"
#include "riff.h"
#include <limits>
namespace producer::app {
namespace { HRESULT failure(DWORD error){return HRESULT_FROM_WIN32(error?error:ERROR_WRITE_FAULT);} }
bool FileOutputWriter::valid_format(const WAVEFORMATEX& f){
    if(!f.nChannels||f.nChannels>32||f.nSamplesPerSec<8000||f.nSamplesPerSec>192000||f.cbSize)return false;
    if(f.wFormatTag==WAVE_FORMAT_PCM){if(f.wBitsPerSample!=8&&f.wBitsPerSample!=16&&f.wBitsPerSample!=24&&f.wBitsPerSample!=32)return false;}
    else if(f.wFormatTag!=WAVE_FORMAT_IEEE_FLOAT||f.wBitsPerSample!=32)return false;
    const auto alignment=std::uint64_t(f.nChannels)*f.wBitsPerSample/8;
    return alignment==f.nBlockAlign&&alignment*f.nSamplesPerSec==f.nAvgBytesPerSec;
}
HRESULT FileOutputWriter::write_locked(const void* bytes,DWORD count){
    DWORD written=0;if(!WriteFile(file_,bytes,count,&written,nullptr))return failure(GetLastError());
    return written==count?S_OK:failure(ERROR_WRITE_FAULT);
}
HRESULT FileOutputWriter::start(const std::wstring& path,const WAVEFORMATEX& format){
    std::lock_guard<std::mutex> lock(mutex_);
    if(status_.active)return E_UNEXPECTED;
    if(path.empty()||path.find(L'\0')!=std::wstring::npos||!valid_format(format))return E_INVALIDARG;
    const auto opened=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(opened==INVALID_HANDLE_VALUE)return failure(GetLastError());
    file_=opened;format_=format;status_={true,S_OK,0,0,path};
    Bytes header;auto text=[&](const char* s){header.insert(header.end(),s,s+4);};
    auto dword=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)header.push_back(static_cast<std::uint8_t>(n>>(i*8)));};
    auto word=[&](std::uint16_t n){header.push_back(static_cast<std::uint8_t>(n));header.push_back(static_cast<std::uint8_t>(n>>8));};
    const bool floating=format.wFormatTag==WAVE_FORMAT_IEEE_FLOAT;
    text("RIFF");dword(0);text("WAVE");text("fmt ");dword(floating?18:16);
    word(format.wFormatTag);word(format.nChannels);dword(format.nSamplesPerSec);dword(format.nAvgBytesPerSec);word(format.nBlockAlign);word(format.wBitsPerSample);
    factFramesAt_=0;if(floating){word(0);text("fact");dword(4);factFramesAt_=static_cast<DWORD>(header.size());dword(0);}
    text("data");dataLengthAt_=static_cast<DWORD>(header.size());dword(0);headerBytes_=static_cast<DWORD>(header.size());
    put32(header,4,headerBytes_-8);
    const auto result=write_locked(header.data(),headerBytes_);
    if(FAILED(result)){status_.result=result;status_.active=false;CloseHandle(file_);file_=INVALID_HANDLE_VALUE;}
    return result;
}
HRESULT FileOutputWriter::append(const void* bytes,DWORD count){
    std::lock_guard<std::mutex> lock(mutex_);if(!status_.active)return S_FALSE;
    if((count&&!bytes)||count%format_.nBlockAlign)return E_INVALIDARG;
    if(status_.bytes+count+headerBytes_+1>std::numeric_limits<DWORD>::max())return status_.result=failure(ERROR_FILE_TOO_LARGE);
    if(FAILED(status_.result))return status_.result;
    DWORD written=0;const auto success=WriteFile(file_,bytes,count,&written,nullptr);
    status_.bytes+=written;status_.frames=status_.bytes/format_.nBlockAlign;
    if(!success||written!=count)return status_.result=failure(success?ERROR_WRITE_FAULT:GetLastError());
    return S_OK;
}
HRESULT FileOutputWriter::stop_locked(){
    if(file_==INVALID_HANDLE_VALUE)return S_FALSE;
    HRESULT result=status_.result;
    const auto at=[&](DWORD offset,DWORD value){LARGE_INTEGER pos{};pos.QuadPart=offset;
        if(!SetFilePointerEx(file_,pos,nullptr,FILE_BEGIN))return failure(GetLastError());return write_locked(&value,sizeof(value));};
    if(status_.bytes%format_.nBlockAlign)result=failure(ERROR_WRITE_FAULT);
    const auto pad=static_cast<DWORD>(status_.bytes&1);if(pad){const std::uint8_t zero=0;const auto hr=write_locked(&zero,1);if(FAILED(hr)&&SUCCEEDED(result))result=hr;}
    for(const auto hr:{at(4,static_cast<DWORD>(headerBytes_+status_.bytes+pad-8)),at(dataLengthAt_,static_cast<DWORD>(status_.bytes)),factFramesAt_?at(factFramesAt_,static_cast<DWORD>(status_.frames)):S_OK})if(FAILED(hr)&&SUCCEEDED(result))result=hr;
    if(!FlushFileBuffers(file_)&&SUCCEEDED(result))result=failure(GetLastError());
    CloseHandle(file_);file_=INVALID_HANDLE_VALUE;status_.active=false;status_.result=result;return result;
}
HRESULT FileOutputWriter::stop(){std::lock_guard<std::mutex> lock(mutex_);return stop_locked();}
FileOutputStatus FileOutputWriter::status() const {std::lock_guard<std::mutex> lock(mutex_);return status_;}
FileOutputWriter::~FileOutputWriter(){std::lock_guard<std::mutex> lock(mutex_);stop_locked();}
}
