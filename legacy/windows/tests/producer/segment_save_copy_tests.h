void segment_save_copy_tests(const std::filesystem::path& dir){
    const auto base=dir/L"CopyVersion";std::filesystem::create_directories(base);
    Framework host;const auto source=host.new_segment();
    auto root=Chunk::parse(SegmentDocument::playback_test().save_bytes());
    Chunk opaque;opaque.id="qCpY";opaque.data={0x51,0,0x92,0x31,0x77};root.children.push_back(opaque);
    host.document(source).load(root.encode());
    const auto sourcePath=(base/L"Source.sgp").wstring(),copyPath=(base/L"Version.sgp").wstring();
    host.save_segment(source,sourcePath);const auto checkpoint=read_file(sourcePath);
    require(host.document(source).change_current_tempo(0,137),"Copy fixture has an unsaved source edit");
    const auto edited=host.document(source).save_bytes();
    const auto notes=host.document(source).notes();
    const auto owner=host.documents()[source].document.get();
    const auto copied=host.save_segment_copy(source,copyPath);
    require(copied!=source&&host.documents().size()==2,"Save As adds another owned Segment");
    require(host.documents()[source].document.get()==owner&&host.documents()[source].path==sourcePath,"Copy retains source owner and path");
    require(read_file(sourcePath)==checkpoint&&host.document(source).save_bytes()==edited&&host.document(source).dirty()&&host.document(source).can_undo(),"Copy preserves source disk bytes and unsaved history");
    auto expected=Chunk::parse(edited);const auto copy=host.document(copied).save_bytes();const auto copiedRoot=Chunk::parse(copy);
    require(expected.find("guid")->data!=copiedRoot.find("guid")->data,"Copy has a new native GUID");
    expected.find("guid")->data=copiedRoot.find("guid")->data;
    require(expected.encode()==copy&&read_file(copyPath)==copy,"Only GUID changes in self-contained copy; opaque bytes preserved");
    require(!host.document(copied).dirty()&&!host.document(copied).can_undo()&&!host.document(copied).can_redo()&&!host.documents()[copied].projectReference,"New copy owns a clean checkpoint and fresh project identity");
    require(host.document(copied).notes().size()==notes.size()&&host.document(copied).tempo_at(0)==137,"Copy retains source music and unsaved Tempo snapshot");
    const auto count=host.documents().size();
    for(const auto& target:{sourcePath,copyPath,(base/L"NotSegment.txt").wstring()}){
        rejected([&]{host.save_segment_copy(source,target);},"Owned or wrong-type copy destination rejected");
        require(host.documents().size()==count&&host.document(source).save_bytes()==edited&&read_file(sourcePath)==checkpoint&&read_file(copyPath)==copy,"Rejected copy preserves both documents and files");
    }
    const auto blocker=(base/L"not-a-directory").wstring();write_file_atomic(blocker,Bytes{'k','e','e','p'});
    rejected([&]{host.save_segment_copy(source,(std::filesystem::path(blocker)/L"Failure.sgp").wstring());},"Invalid copy parent fails without publishing ownership");
    require(host.documents().size()==count&&read_file(blocker)==Bytes({'k','e','e','p'})&&host.document(source).save_bytes()==edited,"Failed file publication preserves source and catalog");
    require(host.document(source).undo()&&host.document(source).save_bytes()==checkpoint&&!host.document(source).dirty(),"Source Undo remains independent after copy");
    require(host.document(copied).save_bytes()==copy&&!host.document(copied).can_undo(),"Source Undo cannot alter copy or its history");
    host.save_project((base/L"CopyVersion.pro").wstring());
    Framework reload;reload.open_project((base/L"CopyVersion.pro").wstring());
    require(reload.documents().size()==2&&reload.document(0).save_bytes()==checkpoint&&reload.document(1).save_bytes()==copy,"Native Project reload keeps both distinct GUID versions");
    require(reload.document(0).tempo_at(0)!=reload.document(1).tempo_at(0),"Source and copied Tempo remain distinct on reload");
    reload.save_segment(1,copyPath);require(read_file(copyPath)==copy&&read_file(sourcePath)==checkpoint,"Ordinary save of copy preserves source and copy bytes");
    auto bare=copiedRoot;bare.children.erase(std::remove_if(bare.children.begin(),bare.children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="UNFO";}),bare.children.end());
    auto emptyInfo=bare;Chunk info;info.id="LIST";info.type="UNFO";emptyInfo.children.push_back(info);const auto authoredEmpty=emptyInfo.encode();
    require(NativeScriptTrackRuntime::loader_bytes(authoredEmpty)==bare.encode(),"Private loader omits empty optional UNFO; GUID/music/opaque chunks unchanged");
    require(emptyInfo.encode()==authoredEmpty&&read_file(copyPath)==copy,"Private loader does not alter authored tree or saved copy");
    Chunk author;author.id="UART";author.data=utf16(L"Keep unknown metadata");emptyInfo.find("LIST","UNFO")->children.push_back(author);const auto populated=emptyInfo.encode();
    require(NativeScriptTrackRuntime::loader_bytes(populated)==populated,"Nonempty optional UNFO without UNAM remains byte-identical");
    auto other=bare;info.type="INFO";other.children.push_back(info);const auto otherBytes=other.encode();
    require(NativeScriptTrackRuntime::loader_bytes(otherBytes)==otherBytes,"Other empty optional lists are preserved in private runtime");
}
