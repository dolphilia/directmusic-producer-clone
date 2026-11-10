struct LyricStampGraph final:producer::runtime::Graph {
    unsigned stamps=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void** out)override{if(out)*out=nullptr;return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return 1;}
    ULONG STDMETHODCALLTYPE Release()override{return 1;}
    HRESULT STDMETHODCALLTYPE StampPMsg(producer::runtime::Message*)override{++stamps;return S_OK;}
    HRESULT STDMETHODCALLTYPE InsertTool(producer::runtime::Tool*,DWORD*,DWORD,LONG)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetTool(DWORD,producer::runtime::Tool**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE RemoveTool(producer::runtime::Tool*)override{return E_NOTIMPL;}
};
void lyric_runtime_tests(const std::filesystem::path& dir){
    struct Payload {producer::runtime::Message header{};wchar_t text[8]{};};
    static_assert(offsetof(Payload,text)==sizeof(producer::runtime::Message));
    LyricStampGraph graph;Payload payload;payload.header.size=sizeof(payload);payload.header.type=13;payload.header.graph=&graph;payload.header.pchannel=16;payload.header.musicTime=123;payload.header.group=2;std::copy_n(L"\u6b4c\xd83c\xdfb5",4,payload.text);
    auto* observer=new LyricObserver;observer->include_channel(16);const auto untouched=payload;
    require(observer->ProcessPMsg(nullptr,&payload.header)==DMUS_S_REQUEUE&&std::memcmp(&payload,&untouched,sizeof(payload))==0&&graph.stamps==1,"Lyric callback stamps and forwards without mutating message or retaining references");
    payload.text[0]=L'X';const auto copies=observer->snapshot();require(copies.size()==1&&copies[0].text==L"\u6b4c\xd83c\xdfb5"&&copies[0].clocks==123&&copies[0].group==2&&copies[0].visible,"Lyric callback owns Unicode value copy and public mapped channel eligibility");
    payload.header.flags=0x20;require(observer->ProcessPMsg(nullptr,&payload.header)==DMUS_S_FREE&&observer->snapshot().size()==1&&graph.stamps==1,"Flushed Lyric is freed without display or requeue");require(!observer->failed(),"Valid bounded callback remains healthy");observer->Release();
    for(unsigned kind=0;kind<5;++kind){observer=new LyricObserver;payload={};payload.header.type=13;payload.header.graph=&graph;payload.header.size=sizeof(payload);if(kind==0)payload.header.size=sizeof(producer::runtime::Message)-1;if(kind==1)payload.header.size=sizeof(producer::runtime::Message)+3;if(kind==2)std::fill(std::begin(payload.text),std::end(payload.text),L'x');if(kind==3)payload.text[0]=0xd800;if(kind==4)payload.text[0]=0xdc00;require(observer->ProcessPMsg(nullptr,&payload.header)==DMUS_S_REQUEUE&&observer->snapshot().empty()&&observer->failed(),"Malformed Lyric payload is bounded rejected and still forwarded");observer->Release();}
    SegmentDocument doc=SegmentDocument::playback_test();
    require(doc.add_lyric({768,768,4,L"Quick \u6b4c\u8a5e"})&&doc.add_lyric({1536,2304,8,L"Queue \u03a9"})&&doc.add_lyric({2304,1536,16,L"At time \xd83c\xdfb5"}),"Lyric runtime fixture owns three delivery settings and independent clocks");
    const auto source=doc.save_bytes();const auto file=dir/L"LyricDelivery.sgp";write_file_atomic(file.wstring(),source);
    SegmentDocument restored;restored.load(read_file(file.wstring()));require(restored.save_bytes()==source,"Lyric runtime saved input reload exact");
    Conductor player;player.enable_note_observation();player.enable_lyric_observation();
    auto run=[&]{player.play(source,L"",nullptr);const auto start=player.playback_request().actualStart;const auto channel=player.performance_channel(0);require(channel.has_value(),"Owned runtime AudioPath includes Message Window PChannel 1");Sleep(4000);player.stop();return std::pair{start,*channel};};
    const auto first=run();const auto a=player.observed_lyrics();
    for(const auto& e:a)std::cerr<<"lyric clocks="<<e.clocks<<" start="<<first.first<<" channel="<<e.channel<<" group="<<e.group<<" visible="<<e.visible<<" chars="<<e.text.size()<<"\n";
    require(a.size()==3,"Actual OS Lyric Track delivers exactly three messages to source Tool");
    const auto expected=restored.lyrics();
    for(size_t i=0;i<a.size();++i){const auto& event=expected[i];require(a[i].text==event.text&&(a[i].group&restored.selected_groups())==restored.selected_groups(),"Actual Lyric Unicode and runtime group mask cover selected track group");require(a[i].clocks==first.first+event.physical,"Actual at-time Tool lyric clocks equal saved physical clock plus public playback start");require(a[i].visible&&a[i].channel==first.second,"Actual Lyric message uses publicly converted Message Window channel");}
    Sleep(500);require(player.observed_lyrics().size()==a.size(),"Stopped playback does not deliver future lyric messages");
    const auto second=run();const auto b=player.observed_lyrics();require(b.size()==6,"Same saved snapshot replay delivers each Lyric exactly once");for(size_t i=0;i<3;++i)require(b[i+3].text==a[i].text&&b[i+3].clocks==second.first+expected[i].physical&&b[i+3].group==a[i].group&&b[i+3].visible&&b[i+3].channel==second.second,"Replay Unicode and owned channel mapping retained");
    const auto notes=player.observed_notes();require(!notes.empty()&&std::all_of(notes.begin(),notes.end(),[](const auto& n){return n.velocity==96;}),"Lyric observer leaves actual note delivery and velocity intact");
    require(!player.lyric_observation_failed()&&!player.note_observation_overflow()&&!player.note_observation_forwarding_failed(),"Both observers forward without malformed text overflow or failed graph stamping");
    require(doc.save_bytes()==source&&read_file(file.wstring())==source,"Runtime observation never edits document or saved source");player.shutdown();
}
