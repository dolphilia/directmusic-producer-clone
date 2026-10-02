// Run one original module's registration against a new, process-private app hive.
// The predefined-key overrides affect this process only; no global registration.
#include <windows.h>
#include <bcrypt.h>
#include <oleauto.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

static std::string quote(const wchar_t* value){
    const int count=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);
    std::string utf8(static_cast<size_t>(count),0);
    if(count)WideCharToMultiByte(CP_UTF8,0,value,-1,utf8.data(),count,nullptr,nullptr);
    std::string result="\"";
    for(unsigned char c:utf8){if(!c)break;switch(c){case '\\':result+="\\\\";break;case '"':result+="\\\"";break;case '\n':result+="\\n";break;case '\r':result+="\\r";break;case '\t':result+="\\t";break;default:if(c<32){char code[7];std::snprintf(code,sizeof(code),"\\u%04x",c);result+=code;}else result+=static_cast<char>(c);}}
    return result+'"';
}
static void status(const char* operation,LSTATUS error){std::printf("{\"operation\":\"%s\",\"error\":%ld}\n",operation,error);std::fflush(stdout);}
static std::string sha256(const wchar_t* path){
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return {};
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectSize=0,returned=0;std::vector<unsigned char> object;
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&returned,0)>=0;
    if(ok){object.resize(objectSize);ok=BCryptCreateHash(algorithm,&hash,object.data(),objectSize,nullptr,0,0)>=0;}
    unsigned char block[65536],digest[32];DWORD read=0;
    while(ok){if(!ReadFile(file,block,sizeof(block),&read,nullptr)){ok=false;break;}if(!read)break;ok=BCryptHashData(hash,block,read,0)>=0;}
    if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);
    if(!ok)return {};std::string result;for(auto byte:digest){char digits[3];std::snprintf(digits,sizeof(digits),"%02x",byte);result+=digits;}return result;
}
struct PrivateRegistry {
    HKEY hive=nullptr;HKEY roots[3]{};unsigned mapped=0;
    const HKEY predefined[3]={HKEY_CLASSES_ROOT,HKEY_LOCAL_MACHINE,HKEY_CURRENT_USER};
    const wchar_t* names[3]={L"HKCR",L"HKLM",L"HKCU"};
    bool open(const wchar_t* file){
        if(GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES){status("hive_must_be_new",ERROR_FILE_EXISTS);return false;}
        auto error=RegLoadAppKeyW(file,&hive,KEY_ALL_ACCESS,REG_PROCESS_APPKEY,0);status("private_hive_open",error);if(error)return false;
        for(unsigned i=0;i<3;i++){error=RegCreateKeyExW(hive,names[i],0,nullptr,0,KEY_ALL_ACCESS,nullptr,&roots[i],nullptr);status("private_root_create",error);if(error)return false;}
        for(unsigned i=0;i<3;i++){error=RegOverridePredefKey(predefined[i],roots[i]);status("predefined_key_override",error);if(error)return false;++mapped;}
        return true;
    }
    bool restore(){bool ok=true;while(mapped){--mapped;const auto error=RegOverridePredefKey(predefined[mapped],nullptr);status("predefined_key_restore",error);ok=error==ERROR_SUCCESS&&ok;}return ok;}
    ~PrivateRegistry(){restore();for(HKEY root:roots)if(root)RegCloseKey(root);if(hive)RegCloseKey(hive);}
};
static bool enumerate(HKEY key,const std::wstring& path,unsigned depth=0){
    if(depth>64){status("registry_depth_limit",ERROR_STACK_OVERFLOW);return false;}
    std::printf("{\"operation\":\"registry_key\",\"path\":%s}\n",quote(path.c_str()).c_str());
    DWORD values=0,maxValueName=0,maxData=0,subkeys=0,maxSubkey=0;
    auto error=RegQueryInfoKeyW(key,nullptr,nullptr,nullptr,&subkeys,&maxSubkey,nullptr,&values,&maxValueName,&maxData,nullptr,nullptr);
    if(error){status("registry_query_info",error);return false;}
    std::vector<wchar_t> name(static_cast<size_t>(maxValueName)+2);
    std::vector<unsigned char> bytes(static_cast<size_t>(maxData)+2);
    for(DWORD i=0;i<values;i++){
        DWORD chars=static_cast<DWORD>(name.size()),size=static_cast<DWORD>(bytes.size()),type=0;
        error=RegEnumValueW(key,i,name.data(),&chars,nullptr,&type,bytes.data(),&size);if(error){status("registry_enum_value",error);return false;}
        std::string hex;for(DWORD j=0;j<size;j++){char digits[3];std::snprintf(digits,sizeof(digits),"%02x",bytes[j]);hex+=digits;}
        std::printf("{\"operation\":\"registry_value\",\"path\":%s,\"name\":%s,\"type\":%lu,\"bytes\":%lu,\"hex\":\"%s\"}\n",quote(path.c_str()).c_str(),quote(name.data()).c_str(),type,size,hex.c_str());
    }
    std::vector<wchar_t> subkey(static_cast<size_t>(maxSubkey)+2);
    for(DWORD i=0;i<subkeys;i++){
        DWORD chars=static_cast<DWORD>(subkey.size());error=RegEnumKeyExW(key,i,subkey.data(),&chars,nullptr,nullptr,nullptr,nullptr);
        if(error){status("registry_enum_key",error);return false;}
        HKEY child=nullptr;error=RegOpenKeyExW(key,subkey.data(),0,KEY_READ,&child);if(error){status("registry_open_child",error);return false;}
        const bool ok=enumerate(child,path+L"\\"+subkey.data(),depth+1);RegCloseKey(child);if(!ok)return false;
    }
    return true;
}
using Register=HRESULT (STDAPICALLTYPE*)();
static HRESULT register_guarded(Register entry,DWORD& exception){
    __try{return entry();}__except(EXCEPTION_EXECUTE_HANDLER){exception=GetExceptionCode();return E_UNEXPECTED;}
}
int wmain(int count,wchar_t** args){
    if(count!=4){std::fputs("Usage: producer_registration_probe absolute-module-path expected-sha256 new-hive-path\n",stderr);return 2;}
    const auto actual=sha256(args[1]);std::wstring hash(actual.begin(),actual.end());
    const bool identity=!actual.empty()&&_wcsicmp(hash.c_str(),args[2])==0;
    std::printf("{\"operation\":\"registration_module_identity\",\"path\":%s,\"sha256\":\"%s\",\"same\":%s}\n",quote(args[1]).c_str(),actual.c_str(),identity?"true":"false");
    if(!identity)return 1;
    const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    std::printf("{\"operation\":\"registration_com_initialize\",\"hresult\":\"0x%08lx\"}\n",static_cast<unsigned long>(initialized));
    if(FAILED(initialized))return 1;
    struct ComApartment {~ComApartment(){CoUninitialize();}} apartment;
    // REGKIND_NONE only reads the embedded typelib. It does not register it.
    ITypeLib* library=nullptr;const HRESULT typeResult=LoadTypeLibEx(args[1],REGKIND_NONE,&library);
    std::printf("{\"operation\":\"registration_typelib_read\",\"hresult\":\"0x%08lx\"}\n",static_cast<unsigned long>(typeResult));
    if(library){TLIBATTR* attributes=nullptr;
        if(SUCCEEDED(library->GetLibAttr(&attributes))&&attributes){wchar_t id[40]{};StringFromGUID2(attributes->guid,id,40);
            std::printf("{\"operation\":\"registration_typelib_attributes\",\"guid\":%s,\"lcid\":%lu,\"major\":%u,\"minor\":%u}\n",quote(id).c_str(),attributes->lcid,attributes->wMajorVerNum,attributes->wMinorVerNum);
            library->ReleaseTLibAttr(attributes);}
        library->Release();}
    PrivateRegistry registry;if(!registry.open(args[3]))return 1;
    HMODULE module=LoadLibraryExW(args[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    status("registration_module_load",module?ERROR_SUCCESS:static_cast<LSTATUS>(GetLastError()));
    bool registered=false;DWORD exception=0;
    if(module){
        auto entry=reinterpret_cast<Register>(GetProcAddress(module,"DllRegisterServer"));
        if(entry){const HRESULT hr=register_guarded(entry,exception);registered=SUCCEEDED(hr)&&!exception;
            std::printf("{\"operation\":\"private_registration\",\"hresult\":\"0x%08lx\",\"exception\":%lu}\n",static_cast<unsigned long>(hr),exception);}
        else status("registration_export_missing",ERROR_PROC_NOT_FOUND);
        // Detach may also access the registry; keep the overrides active until it finishes.
        FreeLibrary(module);
    }
    const bool exported=enumerate(registry.hive,L"");
    const bool restored=registry.restore();
    const auto flush=RegFlushKey(registry.hive);status("private_hive_flush",flush);
    std::printf("{\"operation\":\"end_registration_capture\",\"registered\":%s,\"exported\":%s,\"restored\":%s}\n",registered?"true":"false",exported?"true":"false",restored?"true":"false");
    return registered&&exported&&restored&&flush==ERROR_SUCCESS?0:1;
}
