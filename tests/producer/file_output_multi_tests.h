#pragma once
// A first serialized buffer belongs to the SECOND route. The first output
// must follow route order and carry note69, not buffer storage order/note60.
void file_output_multi_tests(const std::filesystem::path& dir){
    AudioPathDocument seed;require(seed.add_file_output(0),"Multi output first source tap prepared");
    auto root=Chunk::parse(seed.save_bytes());auto first=*root.find("LIST","dbfl"),second=first;
    GUID fresh{};require(SUCCEEDED(CoCreateGuid(&fresh)),"Multi output second unique buffer identity");
    Bytes id(16);std::memcpy(id.data(),&fresh,16);std::copy(id.begin(),id.end(),second.find("ddah")->data.begin());
    auto desc=second.find("RIFF","DSBC");desc->find("guid")->data=id;
    desc->children.erase(std::remove_if(desc->children.begin(),desc->children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="fxls";}),desc->children.end());
    root.children.push_back(second);auto channels=root.find("LIST","pcsl")->children[0].find("LIST","pchl");
    auto low=channels->children[0],high=low;put32(low.data,4,8);std::copy(id.begin(),id.end(),low.data.begin()+16);
    put32(high.data,0,8);put32(high.data,4,8);channels->children={low,high};
    AudioPathDocument path;path.load(root.encode());const auto initial=path.save_bytes();
    require(path.ports()[0].routes.size()==2&&path.ports()[0].routes[0].buffers[0]==path.buffers()[1]&&path.effects().size()==1,"Two disjoint routes deliberately reverse physical buffer order");
    require(path.add_file_output(1),"Multi output second independently owned tap");const auto source=path.save_bytes();
    require(path.undo()&&path.save_bytes()==initial&&!path.dirty()&&path.redo()&&path.save_bytes()==source,"Second tap one complete UndoRedo transaction");
    require(!path.add_file_output(1)&&path.save_bytes()==source,"Repeated buffer tap retains source");

    Chunk wav;wav.id="RIFF";wav.type="WAVE";Chunk fmt;fmt.id="fmt ";fmt.data=Bytes(16);fmt.data[0]=1;fmt.data[2]=1;put32(fmt.data,4,16000);put32(fmt.data,8,32000);fmt.data[12]=2;fmt.data[14]=16;
    Chunk data;data.id="data";data.data.resize(32000);for(size_t i=0;i<16000;++i){const auto x=static_cast<std::int16_t>(std::lround(6000*std::sin(2*3.14159265358979323846*261.6255653005986*i/16000)));data.data[i*2]=static_cast<std::uint8_t>(x);data.data[i*2+1]=static_cast<std::uint8_t>(x>>8);}wav.children={fmt,data};
    auto dls=DlsDocument::create();require(dls.add_wave_pcm(wav.encode(),"Route sine")&&dls.create_instrument(0,0,"Route instrument",0)&&dls.set_wave_loops(0,{{0,320,11987}})&&dls.inherit_wave_sample(0,0),"Multi output known owned sinusoid with sustained inherited sample");
    const auto base=dir/L"MultiCapture";std::filesystem::create_directories(base);dls.save((base/L"RouteSource.dls").wstring());
    Framework host;const auto ci=host.open_collection((base/L"RouteSource.dls").wstring()),bi=host.new_band(),si=host.new_segment();
    require(host.add_band_gm_instrument(bi,0,0,64,100)&&host.add_band_gm_instrument(bi,0,8,64,100),"Route Band owns instruments on both channels");host.save_band(bi,(base/L"RouteBand.bnp").wstring());
    require(host.set_band_collection(bi,0,ci)&&host.set_band_collection(bi,1,ci),"Both routes resolve identical owned DLS source");host.save_band(bi,(base/L"RouteBand.bnp").wstring());
    auto& song=host.document(si);require(song.add_tempo(0,60)&&song.add_note({0,3072,0,69,96})&&song.add_note({0,3072,8,60,96})&&song.set_band(0,host.band_document(bi).save_bytes()),"Multi output channel0 note69 and channel8 note60 fixture");
    write_file_atomic((base/L"RoutePath.aup").wstring(),initial);const auto ai=host.open_audio_path((base/L"RoutePath.aup").wstring());
    require(host.assign_audio_path(si,ai),"Main fixture initially owns second buffer without tap");host.save_segment(si,(base/L"RouteSong.sgp").wstring());host.save_project((base/L"MultiCapture.pro").wstring());
    Framework restored;restored.open_project((base/L"MultiCapture.pro").wstring());require(restored.audio_path_document(0).save_bytes()==initial&&restored.document(0).notes().size()==2&&restored.collections().size()==1&&restored.band_documents().size()==1,"Native four-document carrier restores two routes and missing second tap");
    require(host.audio_path_document(ai).add_file_output(1)&&host.assign_audio_path(si,ai),"Final runtime source has both taps");
    host.save_runtime(RuntimeDocumentKind::AudioPath,ai,(dir/L"two-buffer.aud").wstring());AudioPathDocument exported;exported.load(read_file((dir/L"two-buffer.aud").wstring()));
    require(exported.effects().empty()&&exported.buffers().size()==2&&host.audio_path_document(ai).save_bytes()==source,"Runtime export removes both Producer-only effects and retains buffers and source");
    const auto songBytes=song.save_bytes();const auto deps=host.playback_collections(si);
    write_file_atomic((dir/L"runtime-input.sgp").wstring(),songBytes);write_file_atomic((dir/L"runtime-input.aup").wstring(),source);

    Conductor player;const auto collision=dir/L"Collision1.wav";const Bytes marker={'k','e','e','p'};write_file_atomic(collision.wstring(),marker);
    rejected([&]{player.start_file_output(source,(dir/L"Collision.wav").wstring(),GetDesktopWindow());},"Numbered output collision rejected before any capture publication");
    require(!player.file_output_active()&&!std::filesystem::exists(dir/L"Collision.wav")&&read_file(collision.wstring())==marker,"Collision preserves every existing output and creates no first output");
    auto orphan=path;require(orphan.set_route_buffers(0,1,{orphan.buffers()[1]}),"Prepare unreachable tap without deleting its bytes");
    rejected([&]{player.start_file_output(orphan.save_bytes(),(dir/L"Orphan.wav").wstring(),GetDesktopWindow());},"Unrouted FileOutput refuses implicit Send interpretation");
    require(!player.file_output_active()&&!std::filesystem::exists(dir/L"Orphan.wav"),"Unrouted rejection creates no output");
    auto duplicate=Chunk::parse(source);auto fx=duplicate.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls");fx->children.push_back(fx->children[0]);
    rejected([&]{player.start_file_output(duplicate.encode(),(dir/L"Duplicate.wav").wstring(),GetDesktopWindow());},"Duplicate tap in one buffer rejected before output creation");
    require(!player.file_output_active()&&!std::filesystem::exists(dir/L"Duplicate.wav"),"Duplicate rejection retains source and recorder state");

    const auto firstFile=dir/L"Record.wav",secondFile=dir/L"Record1.wav";
    player.start_file_output(source,firstFile.wstring(),GetDesktopWindow());require(player.file_output_active()&&std::filesystem::exists(firstFile)&&std::filesystem::exists(secondFile)&&!std::filesystem::exists(dir/L"Record2.wav"),"One Start opens exactly two numbered buffer outputs");
    rejected([&]{player.start_file_output(source,(dir/L"Other.wav").wstring(),GetDesktopWindow());},"Active capture cannot start another recording");
    Sleep(300);player.play(songBytes,base.wstring(),GetDesktopWindow(),{},deps);Sleep(2000);
    require(player.position().playing&&player.file_output_active(),"Two routed buffer capture shares musical playback path");const auto playback=player.current_playback_id();
    auto changed=host.document(si);AudioPathDocument renamed;renamed.load(source);renamed.set_name(L"Conflicting recording routes");changed.set_audio_path(renamed.save_bytes());
    rejected([&]{player.play(changed.save_bytes(),base.wstring(),GetDesktopWindow(),{},deps);},"Different active recording AudioPath rejected before musical Stop");
    require(player.current_playback_id()==playback&&player.position().playing&&player.file_output_active()&&song.save_bytes()==songBytes,"Rejected path change retains musical session recorder and source bytes");
    player.stop();const auto size0=std::filesystem::file_size(firstFile),size1=std::filesystem::file_size(secondFile);Sleep(400);
    require(player.file_output_active()&&std::filesystem::file_size(firstFile)>size0&&std::filesystem::file_size(secondFile)>size1,"Musical Stop leaves both independent buffer captures running");
    player.play(songBytes,base.wstring(),GetDesktopWindow(),{},deps);Sleep(2000);player.stop_file_output();
    require(!player.file_output_active()&&player.position().playing,"Explicit recording Stop finalizes both without musical Stop");const auto finalized0=std::filesystem::file_size(firstFile),finalized1=std::filesystem::file_size(secondFile);Sleep(300);
    require(std::filesystem::file_size(firstFile)==finalized0&&std::filesystem::file_size(secondFile)==finalized1,"Both finalized files stop growing during ongoing music");player.stop();
    for(const auto& file:{firstFile,secondFile}){const auto b=read_file(file.wstring());const auto c=Chunk::parse(b);require(read32(b,4)+8==b.size()&&c.find("fmt ")&&c.find("data")&&std::any_of(c.find("data")->data.begin(),c.find("data")->data.end(),[](auto n){return n!=0;}),"Numbered recording has valid finalized RIFF lengths and audible PCM");}
    player.start_file_output(source,(dir/L"Shutdown.wav").wstring(),GetDesktopWindow());Sleep(150);player.shutdown();
    for(const auto& name:{L"Shutdown.wav",L"Shutdown1.wav"}){const auto b=read_file((dir/name).wstring());require(read32(b,4)+8==b.size()&&Chunk::parse(b).find("data"),"Shutdown finalizes every pending buffer control");}
    require(!player.file_output_active()&&path.save_bytes()==source&&song.save_bytes()==songBytes,"Complete multi-buffer lifecycle leaves authoring source unchanged");
}
