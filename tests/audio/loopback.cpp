// Default render endpoint capture. Does not change mixer, endpoint or policy settings.
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <iomanip>
using Microsoft::WRL::ComPtr;
static void check(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error("WASAPI HRESULT " + std::to_string(static_cast<unsigned long>(hr))); }
static void u32(std::ofstream& out, unsigned long v) { out.write(reinterpret_cast<const char*>(&v),4); }
static unsigned long long utc_filetime() {
  FILETIME time; GetSystemTimePreciseAsFileTime(&time);
  return (static_cast<unsigned long long>(time.dwHighDateTime)<<32)|time.dwLowDateTime;
}
int wmain(int argc,wchar_t** argv) {
  if(argc!=3) return 2;
  WAVEFORMATEX* fmt=nullptr;
  bool com=false;
  try {
    const auto dir=std::filesystem::path(argv[1]);
    const int seconds=std::stoi(argv[2]);
    if(seconds<3 || seconds>300 || !std::filesystem::is_directory(dir)) {
      std::cerr << "Capture requires an existing output directory and 3..300 seconds";
      return 2;
    }
    check(CoInitializeEx(nullptr,COINIT_MULTITHREADED)); com=true;
    ComPtr<IMMDeviceEnumerator> enumerator;
    check(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&enumerator)));
    ComPtr<IMMDevice> endpoint; check(enumerator->GetDefaultAudioEndpoint(eRender,eConsole,&endpoint));
    LPWSTR id=nullptr; check(endpoint->GetId(&id));
    { std::wofstream out(dir/L"endpoint.txt"); out<<id; } CoTaskMemFree(id);
    ComPtr<IAudioClient> client; check(endpoint->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(client.GetAddressOf())));
    check(client->GetMixFormat(&fmt));
    check(client->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,10000000,0,fmt,nullptr));
    ComPtr<IAudioCaptureClient> capture; check(client->GetService(IID_PPV_ARGS(&capture)));
    const unsigned long fmtSize=sizeof(WAVEFORMATEX)+fmt->cbSize;
    const size_t frameCount=static_cast<size_t>(seconds)*fmt->nSamplesPerSec;
    std::vector<unsigned char> data(frameCount*fmt->nBlockAlign,0);
    LARGE_INTEGER frequency,start; check(QueryPerformanceFrequency(&frequency)?S_OK:E_FAIL);
    const auto utcBefore=utc_filetime();
    check(QueryPerformanceCounter(&start)?S_OK:E_FAIL);
    const auto utcAfter=utc_filetime();
    const double start100ns=static_cast<double>(start.QuadPart)*10000000.0/static_cast<double>(frequency.QuadPart);
    check(client->Start());
    { std::ofstream ready(dir/L"ready.json"); ready<<std::setprecision(17)
      <<"{\"schema\":2,\"sampleRate\":"<<fmt->nSamplesPerSec<<",\"channels\":"<<fmt->nChannels<<",\"seconds\":"<<seconds
      <<",\"startQpcTicks\":\""<<start.QuadPart<<"\",\"qpcFrequency\":\""<<frequency.QuadPart
      <<"\",\"startQpc100ns\":"<<start100ns<<",\"utcBeforeFileTime\":\""<<utcBefore
      <<"\",\"utcAfterFileTime\":\""<<utcAfter<<"\"}"; }
    std::ofstream packets(dir/L"packets.csv"); packets<<"frame,frames,flags,devicePosition,qpc100ns\n";
    unsigned packetsCount=0, discontinuities=0, timestampErrors=0; size_t endFrame=0;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    while(std::chrono::steady_clock::now()<deadline) {
      UINT32 available=0; check(capture->GetNextPacketSize(&available));
      while(available) {
        BYTE* bytes=nullptr; UINT32 frames=0; DWORD flags=0; UINT64 position=0,qpc=0;
        check(capture->GetBuffer(&bytes,&frames,&flags,&position,&qpc));
        const long long at=static_cast<long long>((static_cast<double>(qpc)-start100ns)*fmt->nSamplesPerSec/10000000.0);
        packets<<at<<','<<frames<<','<<flags<<','<<position<<','<<qpc<<'\n';
        ++packetsCount; if(flags&AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) ++discontinuities;
        if(flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) ++timestampErrors;
        const size_t offset=static_cast<size_t>(std::max(0LL,at));
        const size_t skip=static_cast<size_t>(std::max(0LL,-at));
        if(offset<frameCount && skip<frames && !(flags&AUDCLNT_BUFFERFLAGS_SILENT)) {
          const auto count=std::min(static_cast<size_t>(frames)-skip,frameCount-offset);
          std::copy_n(bytes+skip*fmt->nBlockAlign,count*fmt->nBlockAlign,data.data()+offset*fmt->nBlockAlign);
          endFrame=std::max(endFrame,offset+count);
        }
        check(capture->ReleaseBuffer(frames)); check(capture->GetNextPacketSize(&available));
      }
      Sleep(5);
    }
    check(client->Stop());
    LARGE_INTEGER endQpc;
    const auto endUtcBefore=utc_filetime();
    check(QueryPerformanceCounter(&endQpc)?S_OK:E_FAIL);
    const auto endUtcAfter=utc_filetime();
    std::ofstream wav(dir/L"output.wav",std::ios::binary);
    wav.write("RIFF",4); u32(wav,20+fmtSize+static_cast<unsigned long>(data.size())); wav.write("WAVEfmt ",8); u32(wav,fmtSize);
    wav.write(reinterpret_cast<const char*>(fmt),fmtSize); wav.write("data",4);u32(wav,static_cast<unsigned long>(data.size()));wav.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size())); wav.close();
    std::ofstream meta(dir/L"capture.json"); meta<<"{\"schema\":2,\"passed\":true,\"mode\":\"default-render-endpoint-loopback\",\"seconds\":"<<seconds<<",\"sampleRate\":"<<fmt->nSamplesPerSec<<",\"channels\":"<<fmt->nChannels<<",\"bits\":"<<fmt->wBitsPerSample<<",\"packets\":"<<packetsCount<<",\"discontinuities\":"<<discontinuities<<",\"timestampErrors\":"<<timestampErrors<<",\"lastNonSilentPacketEndFrame\":"<<endFrame<<",\"endQpcTicks\":\""<<endQpc.QuadPart<<"\",\"endUtcBeforeFileTime\":\""<<endUtcBefore<<"\",\"endUtcAfterFileTime\":\""<<endUtcAfter<<"\",\"physicalSpeakerVerified\":false}";
    CoTaskMemFree(fmt); CoUninitialize(); return 0;
  } catch(const std::exception& e) { if(fmt) CoTaskMemFree(fmt); if(com) CoUninitialize(); std::cerr<<e.what(); return 1; }
}
