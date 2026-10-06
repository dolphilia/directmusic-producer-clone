struct SourceVelocityTool final:producer::runtime::Tool {
    LONG refs=1;int amount;bool multiply;unsigned* count;
    SourceVelocityTool(int value,bool product,unsigned* calls):amount(value),multiply(product),count(calls){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==producer::runtime::toolId){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}
    ULONG STDMETHODCALLTYPE Release()override{const auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Init(producer::runtime::Graph*)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMsgDeliveryType(DWORD* out)override{if(!out)return E_POINTER;*out=8;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypeArraySize(DWORD* out)override{if(!out)return E_POINTER;*out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMediaTypes(DWORD** out,DWORD n)override{if(!out||!*out)return E_POINTER;if(n!=1)return E_INVALIDARG;**out=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE ProcessPMsg(producer::runtime::Performance*,producer::runtime::Message* p)override{
        if(!p)return E_POINTER;if(p->type==1&&p->size>=sizeof(producer::runtime::NoteMessage)){
            auto& note=*reinterpret_cast<producer::runtime::NoteMessage*>(p);InterlockedIncrement(reinterpret_cast<LONG*>(count));
            const auto value=multiply?note.velocity/amount:note.velocity+amount;note.velocity=static_cast<BYTE>((std::min)(127,value));
        }
        return p->graph&&SUCCEEDED(p->graph->StampPMsg(p))?DMUS_S_REQUEUE:DMUS_S_FREE;
    }
    HRESULT STDMETHODCALLTYPE Flush(producer::runtime::Performance*,producer::runtime::Message*,LONGLONG)override{return DMUS_S_FREE;}
};
void tool_graph_runtime_tests(const std::filesystem::path& dir){
    std::array<std::uint8_t,16> add{},half{},unused{};add[0]=0x91;half[0]=0x92;unused[0]=0x93;
    unsigned a=0,h=0,u=0;std::vector<ToolFactory> factories={
        {add,[&](const GraphTool&){return new SourceVelocityTool(4,false,&a);}},
        {half,[&](const GraphTool&){return new SourceVelocityTool(2,true,&h);}},
        {unused,[&](const GraphTool&){return new SourceVelocityTool(1,false,&u);}}};
    ToolGraphDocument graph;graph.add_tool(add,{0});graph.add_tool(half,{});graph.add_tool(unused,{16});
    rejected([&]{validate_tool_factories(graph.save_bytes(),{});},"Undeclared tool cannot activate registry fallback");
    auto duplicate=factories;duplicate.push_back(factories[0]);rejected([&]{validate_tool_factories(graph.save_bytes(),duplicate);},"Ambiguous tool factory rejected");
    auto segment=Chunk::parse(SegmentDocument::playback_test().save_bytes());segment.children.push_back(Chunk::parse(graph.save_bytes()));
    require(segment_tool_graph(segment.encode())==graph.save_bytes(),"Embedded Segment graph extracted exactly");
    auto bad=segment;bad.children.push_back(Chunk::parse(graph.save_bytes()));rejected([&]{segment_tool_graph(bad.encode());},"Duplicate embedded graph refused");
    Conductor conductor;conductor.set_tool_factories(factories);conductor.enable_note_observation();
    conductor.play(segment.encode(),L"",nullptr);const auto id=conductor.current_playback_id();
    rejected([&]{conductor.set_tool_factories({});},"Cannot mutate factories during runtime");
    auto unknown=segment;auto gh=unknown.find("RIFF","DMTG");gh->find("LIST","toll")->children[0].find("tolh")->data[0]=0x94;
    rejected([&]{conductor.play(unknown.encode(),L"",nullptr);},"Undeclared graph rejected before Stop");
    require(conductor.current_playback_id()==id,"Rejected graph preserves current playback session");
    const auto finish=[&]{const auto deadline=GetTickCount64()+15000;bool started=false;while(GetTickCount64()<deadline){const auto p=conductor.position();started=started||p.playing;if(started&&!p.playing)return true;Sleep(20);}return false;};
    require(finish(),"Scheduled playback starts and naturally completes before Stop");
    conductor.stop();auto first=conductor.observed_notes();std::cerr<<"runtime notes="<<first.size()<<" add="<<a<<" half="<<h<<" excluded="<<u<<"\n";for(const auto& n:first)std::cerr<<"note channel="<<n.channel<<" velocity="<<static_cast<unsigned>(n.velocity)<<" clock="<<n.clocks<<"\n";require(first.size()==8&&a==8&&h==8&&u==0,"Actual generated messages visit selected and all-channel tools, skip PChannel16");
    require(std::all_of(first.begin(),first.end(),[](const PlaybackNote& n){return n.velocity==50;}),"Tool order add then half transforms generated velocity");
    graph.move_tool(0,1);segment.children.back()=Chunk::parse(graph.save_bytes());
    write_file_atomic((dir/L"SourceGraph.sgp").wstring(),segment.encode());
    conductor.play(read_file((dir/L"SourceGraph.sgp").wstring()),L"",nullptr);
    require(finish(),"Reordered saved graph replay starts and naturally completes");
    conductor.stop();const auto second=conductor.observed_notes();
    require(second.size()==16&&std::all_of(second.begin()+8,second.end(),[](const PlaybackNote& n){return n.velocity==52;}),"Saved reordered graph changes actual runtime message effects");
    require(!conductor.note_observation_forwarding_failed()&&!conductor.note_observation_overflow(),"Graph forwards without observer overflow or failed stamping");
    AudioPathDocument path;const auto pathBefore=path.save_bytes();ToolGraphDocument pathGraph;pathGraph.add_tool(half,{0});pathGraph.add_tool(unused,{16});
    require(path.set_tool_graph(pathGraph.save_bytes())&&path.tool_graph()==pathGraph.save_bytes(),"AudioPath owns typed embedded graph");
    const auto pathWith=path.save_bytes();require(path.undo()&&path.save_bytes()==pathBefore&&path.redo()&&path.save_bytes()==pathWith,"AudioPath graph history retains routing and identity");
    auto duplicatePath=Chunk::parse(pathWith);duplicatePath.children.push_back(Chunk::parse(pathGraph.save_bytes()));rejected([&]{path.load(duplicatePath.encode());},"Duplicate AudioPath graph load rejected");require(path.save_bytes()==pathWith,"Invalid AudioPath load leaves document unchanged");
    rejected([&]{path.set_tool_graph(Bytes{1,2,3});},"Invalid AudioPath graph edit rejected");require(path.save_bytes()==pathWith,"Invalid graph edit retains document");
    require(path.remove_tool_graph()&&path.tool_graph().empty()&&path.undo()&&path.save_bytes()==pathWith,"Remove embedded AudioPath graph is undoable");
    ToolGraphDocument segmentGraph;segmentGraph.add_tool(add,{0});SegmentDocument routed=SegmentDocument::playback_test();routed.set_tool_graph(segmentGraph.save_bytes());routed.set_audio_path(path.save_bytes());
    const auto routedSource=routed.save_bytes();write_file_atomic((dir/L"AudioPathGraph.sgp").wstring(),routedSource);
    const auto previousA=a,previousH=h;conductor.play(read_file((dir/L"AudioPathGraph.sgp").wstring()),L"",nullptr);const auto pathId=conductor.current_playback_id();
    auto unknownPath=Chunk::parse(routedSource);unknownPath.find("RIFF","DMAP")->find("RIFF","DMTG")->find("LIST","toll")->children[0].find("tolh")->data[0]=0x94;
    rejected([&]{conductor.play(unknownPath.encode(),L"",nullptr);},"Undeclared AudioPath factory rejected before runtime activation");require(conductor.current_playback_id()==pathId,"Rejected AudioPath graph preserves existing playback session");
    require(finish(),"Saved AudioPath graph starts and completes actual playback");conductor.stop();const auto third=conductor.observed_notes();
    std::cerr<<"AudioPath notes="<<third.size()<<" add="<<a-previousA<<" half="<<h-previousH<<" excluded="<<u<<"\n";
    require(third.size()==24&&a-previousA==8&&h-previousH==8&&u==0,"Actual note messages traverse Segment and AudioPath graphs with local channel filtering");
    require(std::all_of(third.begin()+16,third.end(),[](const PlaybackNote& n){return n.velocity==50;}),"Segment add precedes AudioPath half before performance observer");
    require(routed.save_bytes()==routedSource&&read_file((dir/L"AudioPathGraph.sgp").wstring())==routedSource,"Runtime graph preparation does not mutate owned or saved source");
    require(!conductor.note_observation_forwarding_failed()&&!conductor.note_observation_overflow(),"AudioPath graph forwards without observation failure");
    conductor.shutdown();
}
