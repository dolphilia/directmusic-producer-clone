namespace script_ansi_tests {
template<class T>struct Release{void operator()(T* p)const{if(p)p->Release();}};
template<class T>using Owner=std::unique_ptr<T,Release<T>>;
void declared(REFGUID cls,const wchar_t* dll){
    wchar_t id[40]{},server[32768]{},expanded[32768]{},system[MAX_PATH]{},wow[MAX_PATH]{};require(StringFromGUID2(cls,id,40)!=0,"ANSI probe class formats");DWORD n=sizeof(server);
    const auto key=std::wstring(L"CLSID\\")+id+L"\\InprocServer32";
    require(RegGetValueW(HKEY_CLASSES_ROOT,key.c_str(),nullptr,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ,nullptr,server,&n)==ERROR_SUCCESS,"ANSI probe declared class registered");
    const auto length=ExpandEnvironmentStringsW(server,expanded,32768);require(length>0&&length<=32768,"ANSI probe server expands");GetSystemDirectoryW(system,MAX_PATH);GetSystemWow64DirectoryW(wow,MAX_PATH);const std::filesystem::path p(expanded);
    const auto same=[](const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;};
    require(same(p.filename().wstring(),dll)&&(same(p.parent_path().wstring(),system)||same(p.parent_path().wstring(),wow)),"ANSI probe uses only declared Windows server");
}
std::pair<DWORD,std::wstring> native_name(const Bytes& bytes,REFGUID cls){
    namespace rt=producer::runtime;rt::Loader* raw=nullptr;require(CoCreateInstance(rt::loaderClass,nullptr,CLSCTX_INPROC_SERVER,rt::loader8Id,reinterpret_cast<void**>(&raw))==S_OK&&raw,"Fresh independent Loader per ANSI case");Owner<rt::Loader> loader(raw);
    rt::ObjectDesc input{};input.size=sizeof(input);input.valid=2|1024;input.classId=cls;input.memory=const_cast<BYTE*>(bytes.data());input.memoryLength=bytes.size();rt::MusicObject* p=nullptr;
    require(raw->GetObject(&input,rt::musicObjectId,reinterpret_cast<void**>(&p))==S_OK&&p,"Independent OS loads full controlled ANSI object");Owner<rt::MusicObject> object(p);rt::ObjectDesc d{};d.size=sizeof(d);
    require(p->GetDescriptor(&d)==S_OK&&std::wmemchr(d.name,0,64),"Independent OS descriptor supplies bounded name");return {d.valid,d.name};
}
Chunk script_for(const Chunk& source,REFGUID cls,const std::wstring& name){
    using namespace script_dependency_tests;const auto id=source.find(source.type=="DLS "?"dlid":"guid");require(id&&id->data.size()==16,"ANSI source retains explicit identity");
    auto ref=reference(cls,id->data,L"Unused lower-priority filename");put32(ref.find("refh")->data,16,6);Chunk field;field.id="name";field.data=utf16(name);ref.children.push_back(field);ref.find("refh")->data.insert(ref.find("refh")->data.end(),{0xa1,0xb2});
    auto script=Chunk::parse(contained(ref,cls)),owner=Chunk::parse(contained(source,cls));auto entry=owner.find("RIFF","DMCN")->find("LIST","cosl")->children.front();entry.find("coba")->data=utf16(L"Native owner");
    script.find("RIFF","DMCN")->find("LIST","cosl")->children.push_back(entry);return script;
}
void run(const std::filesystem::path& dir,const std::filesystem::path& input){
    namespace rt=producer::runtime;const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);require(SUCCEEDED(initialized),"ANSI descriptor COM apartment");struct Apartment{~Apartment(){CoUninitialize();}} apartment;
    const GUID waveClass={0x8a667154,0xf9cb,0x11d2,{0xad,0x8a,0,0x60,0xb0,0x57,0x5a,0xbc}};declared(rt::loaderClass,L"dmloader.dll");declared(rt::collectionClass,L"dmusic.dll");declared(waveClass,L"dswave.dll");
    for(unsigned i=0;i<12;++i){const bool wave=i>=6;const unsigned mode=i%6;const auto cls=wave?waveClass:rt::collectionClass;const auto file=input/(std::to_wstring(i)+(wave?L".wav":L".dls"));const auto bytes=read_file(file.wstring());const auto source=Chunk::parse(bytes);const auto descriptor=native_name(bytes,cls);
        if(mode==0)require((descriptor.first&4)&&descriptor.second==L"Owned ANSI","Independent ASCII INFO name matches fixed control");
        if(mode==1)require((descriptor.first&4)&&!descriptor.second.empty()&&descriptor.second!=L"Owned ANSI","Independent nonASCII INFO name is not prior memory-cache object");
        if(mode==2||mode==3)require((descriptor.first&4)&&descriptor.second==L"Owned ANSI","Declared DLS/Wave descriptors use ANSI when Unicode appears before or after");
        if(mode==4||mode==5){require(descriptor.second.empty()&&((descriptor.first&4)!=0)==(mode==5),"Unicode-only absent NAME and empty ANSI NAME remain distinct");const auto script=script_for(source,cls,L"Unicode winner");const auto before=script.encode();rejected([&]{prepare_script_runtime(before,L"");},"DLS/Wave UNFO does not invent a runtime NAME fallback");require(script.encode()==before&&read_file(file.wstring())==bytes,"Absent runtime name keeps all caller/source bytes");continue;}
        const auto script=script_for(source,cls,descriptor.second);const auto before=script.encode();ScriptDocument native;native.load(before);const auto result=prepare_script_runtime(before,L"");const auto prepared=Chunk::parse(result.script);const auto entries=prepared.find("RIFF","DMCN")->find("LIST","cosl");
        require(result.selections.size()==1&&result.selections.front().selectedByName&&!result.selections.front().selectedByGuid&&!result.selections.front().selectedByFullPath,"ANSI owned name binds without GUID priority or filesystem discovery");
        require(entries->children.front().children.back().encode()==bytes,"Selected ANSI object retains complete independent source bytes");
        require(read32(entries->children.front().find("cobh")->data,16)==1&&read32(prepared.find("RIFF","DMCN")->find("conh")->data,0)==2,"ANSI binding retains KEEP and NOLOADS flags");
        require(native.save_bytes()==before&&!native.dirty()&&read_file(file.wstring())==bytes,"ANSI resolution leaves native Script dirty/history and source input unchanged");
        if(mode==0){const auto saved=dir/(wave?L"AnsiWave.spp":L"AnsiDls.spp");native.save(saved.wstring());ScriptDocument restored;restored.load(read_file(saved.wstring()));require(restored.save_bytes()==before&&!restored.dirty()&&prepare_script_runtime(restored.save_bytes(),L"").script==result.script,"Native ANSI Script save and fresh document restoration keep exact runtime binding");
            auto duplicate=source;duplicate.find(wave?"guid":"dlid")->data[0]^=0x61;auto ambiguous=script;auto owner=Chunk::parse(script_dependency_tests::contained(duplicate,cls)).find("RIFF","DMCN")->find("LIST","cosl")->children.front();owner.find("coba")->data=utf16(L"Other owner");ambiguous.find("RIFF","DMCN")->find("LIST","cosl")->children.push_back(owner);rejected([&]{prepare_script_runtime(ambiguous.encode(),L"");},"Different identities with same ANSI name reject ambiguity");
            for(unsigned bad=0;bad<4;++bad){auto invalid=source;auto info=invalid.find("LIST","INFO");if(bad==0)info->find("INAM")->data={65};if(bad==1)info->find("INAM")->data={65,0,66,0};if(bad==2)info->children.push_back(*info->find("INAM"));if(bad==3)invalid.children.push_back(*info);const auto badScript=script_for(invalid,cls,descriptor.second);const auto badBytes=badScript.encode();rejected([&]{prepare_script_runtime(badBytes,L"");},"Malformed or duplicate ANSI metadata rejects before private selection");require(badScript.encode()==badBytes,"Rejected ANSI metadata preserves caller bytes");}
            if(GetACP()==932){auto invalid=source;invalid.find("LIST","INFO")->find("INAM")->data={0x81,0};rejected([&]{prepare_script_runtime(script_for(invalid,cls,descriptor.second).encode(),L"");},"Incomplete Shift-JIS lead byte cannot become a replacement-name match");}
        }
    }
}
}
