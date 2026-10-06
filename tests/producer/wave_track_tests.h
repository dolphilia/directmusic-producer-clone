void wave_track_tests(const std::filesystem::path& dir,const std::wstring& sample){
    const auto input=read_file(sample);SegmentDocument doc;doc.load(input);
    require(doc.save_bytes()==input&&!doc.dirty(),"Original Wave Segment lossless owned load");
    const auto items=doc.waves();require(items.size()==1&&items[0].pchannel==11&&items[0].filename==L"SfxCow.wvp"&&items[0].clockTime,"Original Wave part reference and clock-time configuration decoded");
    const auto initial=items[0].placement;require(initial.time==0&&initial.startOffset==0&&initial.duration==14483039&&initial.volume==0&&initial.pitch==0,"Original Wave 64-bit header fields match independently observed bytes");
    auto value=initial;value.time=5000000;value.startOffset=1000000;value.duration=8000000;value.volume=-600;value.pitch=125;
    require(doc.edit_wave(0,0,value)&&doc.dirty(),"Wave placement trim gain pitch edit");
    auto expected=Chunk::parse(input);auto& h=expected.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi")->children[0].find("waih")->data;
    put32(h,0,static_cast<std::uint32_t>(-600));put32(h,4,125);put32(h,16,5000000);put32(h,24,1000000);put32(h,40,8000000);
    require(doc.save_bytes()==expected.encode(),"Only five Wave fields changed; references flags loops reserved padding extensions retained");
    const auto edited=doc.save_bytes();require(!doc.edit_wave(0,0,value)&&doc.save_bytes()==edited,"Wave no-op retains document and history");
    for(unsigned i=0;i<5;++i){auto bad=value;if(i==0)bad.time=-1;if(i==1)bad.startOffset=-1;if(i==2)bad.duration=0;if(i==3)bad.volume=1;if(i==4)bad.time=INT64_MAX;require(!doc.edit_wave(0,0,bad)&&doc.save_bytes()==edited,"Invalid Wave input retains complete document");}
    rejected([&]{doc.edit_wave(0,1,value);},"Invalid Wave selection rejected");require(doc.save_bytes()==edited,"Invalid Wave selection retains document");
    require(doc.undo()&&doc.save_bytes()==input&&!doc.dirty(),"Wave Undo restores exact imported tree and saved state");
    require(doc.redo()&&doc.save_bytes()==edited,"Wave Redo restores exact edited tree");
    const auto base=dir/L"wave-track";std::filesystem::create_directories(base);doc.save((base/L"edited.sgp").wstring());SegmentDocument reload;reload.load(read_file((base/L"edited.sgp").wstring()));require(reload.save_bytes()==edited&&reload.waves()[0].placement.startOffset==1000000,"Wave disk reload restores trim");
    auto tree=Chunk::parse(input);auto& tracks=*tree.find("LIST","trkl");auto second=tracks.children[0];put32(second.find("trkh")->data,20,2);tracks.children.push_back(second);doc.load(tree.encode());doc.select_track_group(2);require(doc.edit_wave(0,0,value),"Wave selected group edit");const auto groups=Chunk::parse(doc.save_bytes());require(groups.find("LIST","trkl")->children[0].encode()==tracks.children[0].encode(),"Wave other group bytes invariant");
    auto malformed=tree;malformed.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi")->children[0].find("waih")->data.resize(63);doc.load(malformed.encode());auto before=doc.save_bytes();rejected([&]{doc.edit_wave(0,0,value);},"Truncated Wave header rejected");require(doc.save_bytes()==before&&!doc.dirty(),"Malformed Wave edit retains complete source document");
    doc.load(input);auto large=initial;large.time=static_cast<std::int64_t>(UINT32_MAX)+1;large.duration=100000;require(doc.edit_wave(0,0,large)&&doc.waves()[0].placement.time==large.time,"Wave clock times retain high DWORD");
    write_file_atomic((base/L"high-time.sgp").wstring(),doc.save_bytes());
    const auto projectDir=base/L"Scenario";std::filesystem::create_directories(projectDir);
    write_file_atomic((projectDir/L"SfxCow.sgp").wstring(),input);
    std::filesystem::copy_file(std::filesystem::path(sample).parent_path()/L"SfxCow.wvp",projectDir/L"SfxCow.wvp",std::filesystem::copy_options::overwrite_existing);
    Framework host;host.new_project();const auto segment=host.open_segment((projectDir/L"SfxCow.sgp").wstring());
    host.save_project((projectDir/L"Scenario.pro").wstring());Framework restored;restored.open_project((projectDir/L"Scenario.pro").wstring());
    require(restored.document(0).waves()[0].filename==L"SfxCow.wvp"&&restored.document(0).save_bytes()==host.document(segment).save_bytes(),"Native Project restores owned Wave reference and payload");
    auto music=Chunk::parse(input);put32(music.find("LIST","trkl")->children[0].find("trkx")->data,0,0x10);doc.load(music.encode());before=doc.save_bytes();require(!doc.waves()[0].clockTime,"Wave clock domain derives from configuration, not variations flags");rejected([&]{doc.edit_wave(0,0,value);},"Music-time Wave edit explicitly unsupported until logical time contract");require(doc.save_bytes()==before,"Unsupported clock domain retains document");
    // CRUD starts from an observed original event; only the duplicate's time
    // changes, while producer data and opaque siblings remain byte exact.
    auto opaqueTree=Chunk::parse(input);auto& opaqueList=*opaqueTree.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi");
    Chunk opaque;opaque.id="xWav";opaque.data={5,6,7};opaque.padding=0x7f;opaqueList.children.insert(opaqueList.children.begin(),opaque);
    auto& event=opaqueList.children[1];Chunk extra;extra.id="xEvt";extra.data={1,9,3};extra.padding=0x23;event.children.push_back(extra);
    const auto originalOpaque=opaqueTree.encode();doc.load(originalOpaque);const auto clip=doc.copy_wave(0,0);
    require(doc.save_bytes()==originalOpaque&&!doc.dirty()&&!doc.undo(),"Wave copy retains exact source and creates no history");
    size_t after=99;require(doc.paste_wave(clip,0,20000000,0,&after)&&after==1&&doc.waves().size()==2,"Wave clipboard creates new event and returns typed index excluding unknown siblings");
    auto expectedPaste=opaqueTree;auto duplicate=event;put32(duplicate.find("waih")->data,16,20000000);expectedPaste.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi")->children.push_back(duplicate);
    const auto pasted=doc.save_bytes();require(pasted==expectedPaste.encode(),"Wave paste changes only copied time and preserves reference loops flags extensions and unknown siblings");
    require(doc.undo()&&doc.save_bytes()==originalOpaque&&!doc.dirty()&&doc.redo()&&doc.save_bytes()==pasted,"Wave paste shared UndoRedo exact source and duplicate");
    require(!doc.paste_wave(clip,0,-1)&&doc.save_bytes()==pasted&&!doc.paste_wave(clip,0,INT64_MAX)&&doc.save_bytes()==pasted,"Invalid Wave paste time and end overflow retain full document");
    auto badClip=Chunk::parse(clip);badClip.find("LIST","wave")->find("waih")->data.resize(63);rejected([&]{doc.paste_wave(badClip.encode(),0,0);},"Malformed Wave clipboard rejected");require(doc.save_bytes()==pasted,"Malformed Wave clipboard retains document");
    badClip=Chunk::parse(clip);put32(badClip.find("waph")->data,8,12);rejected([&]{doc.paste_wave(badClip.encode(),0,0);},"Different clipboard PChannel rejected instead of silently rerouting");require(doc.save_bytes()==pasted,"Part mismatch retains document");
    badClip=Chunk::parse(clip);put32(badClip.find("mode")->data,0,0);rejected([&]{doc.paste_wave(badClip.encode(),0,0);},"Music-time clipboard explicitly rejected");require(doc.save_bytes()==pasted,"Clock mismatch retains document");
    require(!doc.delete_wave(0,9)&&doc.save_bytes()==pasted,"Missing Wave delete retains full document");
    require(doc.delete_wave(0,0)&&doc.waves().size()==1&&doc.waves()[0].placement.time==20000000,"Wave delete selects typed index without deleting opaque sibling");
    auto expectedDeleted=expectedPaste;expectedDeleted.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi")->children.erase(expectedDeleted.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi")->children.begin()+1);
    require(doc.save_bytes()==expectedDeleted.encode(),"Wave deletion preserves surviving event and all unrelated bytes");
    require(doc.delete_wave(0,0)&&doc.waves().empty(),"Last Wave deletion retains empty authored part and track");const auto emptyWave=doc.save_bytes();
    require(doc.paste_wave(clip,0,30000000)&&doc.waves().size()==1&&doc.waves()[0].placement.time==30000000,"Wave clipboard inserts into empty existing part");
    require(doc.undo()&&doc.save_bytes()==emptyWave&&doc.redo()&&doc.waves().size()==1,"Empty part paste UndoRedo retained");
    doc.save((base/L"crud.sgp").wstring());SegmentDocument crudReload;crudReload.load(read_file((base/L"crud.sgp").wstring()));require(crudReload.save_bytes()==doc.save_bytes()&&crudReload.waves().size()==1,"Wave CRUD disk reload exact reference and timing");
    auto multi=opaqueTree;auto other=multi.find("LIST","trkl")->children[0];put32(other.find("trkh")->data,20,2);multi.find("LIST","trkl")->children.push_back(other);doc.load(multi.encode());doc.select_track_group(2);require(doc.paste_wave(clip,0,40000000)&&doc.waves().size()==2,"Wave paste into selected group");
    require(Chunk::parse(doc.save_bytes()).find("LIST","trkl")->children[0].encode()==multi.find("LIST","trkl")->children[0].encode(),"Wave clipboard leaves nonselected group byte exact");
    auto musicPaste=opaqueTree;put32(musicPaste.find("LIST","trkl")->children[0].find("trkx")->data,0,0x10);doc.load(musicPaste.encode());before=doc.save_bytes();rejected([&]{doc.paste_wave(clip,0,0);},"Clock-time clipboard cannot paste into music-time track");require(doc.save_bytes()==before&&!doc.dirty()&&!doc.undo(),"Failed clipboard paste creates no document or history changes");

    // Build a fresh reference from an owned WVP, without copying an event.
    doc.load(originalOpaque);const WaveReference freshRef{L"SfxCow.wvp",items[0].objectId};size_t inserted=99;
    require(doc.insert_wave(0,freshRef,{20000000,0,14483039,0,0},1,0,&inserted)&&inserted==1,"Fresh Wave reference insertion without event clipboard");
    const auto freshBytes=doc.save_bytes();const auto freshTree=Chunk::parse(freshBytes);const auto& freshList=*freshTree.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("LIST","wavi");
    require(freshList.children[0].encode()==opaqueList.children[0].encode()&&freshList.children[1].encode()==event.encode(),"Fresh insertion keeps original event extensions and opaque siblings exact");
    const auto& newItem=freshList.children.back();const auto& refh= newItem.find("LIST","DMRF")->find("refh")->data;
    require(newItem.find("waih")->data.size()==64&&read32(refh,16)==(items[0].objectId?0x13u:0x12u)&&doc.waves()[1].variations==1,"Fresh 64-byte header and explicit Wave class/object/filename flags");
    require(doc.undo()&&doc.save_bytes()==originalOpaque&&doc.redo()&&doc.save_bytes()==freshBytes,"Fresh reference has one exact shared UndoRedo edit");
    doc.load(originalOpaque);before=doc.save_bytes();
    require(!doc.insert_wave(0,freshRef,{0,0,1,0,0},0)&&!doc.insert_wave(0,freshRef,{-1,0,1,0,0},1)&&doc.save_bytes()==before&&!doc.undo(),"Invalid variation/start creates no history or mutation");
    rejected([&]{doc.insert_wave(0,{L"../outside.wvp",{}},{0,0,1,0,0},1);},"Fresh reference traversal rejected");require(doc.save_bytes()==before&&!doc.undo(),"Traversal rejection preserves document/history");
    rejected([&]{doc.insert_wave(9,freshRef,{0,0,1,0,0},1);},"Missing Wave part rejected");require(doc.save_bytes()==before&&!doc.undo(),"Missing part retains document/history");
    auto oneVariation=opaqueTree;put32(oneVariation.find("LIST","trkl")->children[0].find("LIST","wavt")->find("LIST","wavp")->find("waph")->data,4,1);doc.load(oneVariation.encode());before=doc.save_bytes();
    rejected([&]{doc.insert_wave(0,freshRef,{0,0,1,0,0},2);},"Variation outside part mask rejected");require(doc.save_bytes()==before&&!doc.undo(),"Variation rejection preserves full tree/history");
    doc.load(musicPaste.encode());before=doc.save_bytes();rejected([&]{doc.insert_wave(0,freshRef,{0,0,1,0,0},1);},"Music-time fresh Wave insertion explicitly rejected");require(doc.save_bytes()==before&&!doc.undo(),"Clock-domain rejection is atomic");
    doc.load(input);require(doc.delete_wave(0,0)&&doc.waves().empty(),"Prepare existing empty Wave part");const auto emptyInsertion=doc.save_bytes();
    require(doc.insert_wave(0,freshRef,{30000000,0,14483039,0,0},1)&&doc.waves().size()==1&&doc.waves()[0].filename==L"SfxCow.wvp","Fresh WVP reference fills empty retained part");
    require(doc.undo()&&doc.save_bytes()==emptyInsertion&&doc.redo()&&doc.waves().size()==1,"Empty part fresh insertion history");
    const auto referenceDir=base/L"Reference";std::filesystem::create_directories(referenceDir);write_file_atomic((referenceDir/L"Input.sgp").wstring(),input);std::filesystem::copy_file(projectDir/L"SfxCow.wvp",referenceDir/L"SfxCow.wvp");Framework insertHost;insertHost.new_project();const auto insertSegment=insertHost.open_segment((referenceDir/L"Input.sgp").wstring());const auto wvpPath=referenceDir/L"SfxCow.wvp";const auto sourceWave=read_file(wvpPath.wstring());const auto ownedWave=insertHost.open_wave(wvpPath.wstring());const auto priorSegment=insertHost.document(insertSegment).save_bytes();
    require(insertHost.insert_segment_wave(insertSegment,ownedWave,0,40000000,1,&inserted),"Framework inserts its owned Project WVP and full duration");const auto format=insertHost.wave_document(ownedWave).format();
    require(insertHost.document(insertSegment).waves().back().placement.duration==static_cast<std::int64_t>((format.frames*10000000u+format.sampleRate-1)/format.sampleRate)&&insertHost.playback_waves(insertSegment).back().bytes==sourceWave,"Fresh reference resolves owned PCM and exact full frame duration");
    require(read_file(wvpPath.wstring())==sourceWave&&!insertHost.wave_document(ownedWave).dirty(),"Wave insertion never changes source audio");
    require(insertHost.document(insertSegment).undo()&&insertHost.document(insertSegment).save_bytes()==priorSegment&&insertHost.document(insertSegment).redo(),"Framework insertion uses Segment shared history");
    insertHost.save_segment(insertSegment,(referenceDir/L"Inserted.sgp").wstring());insertHost.save_project((referenceDir/L"Reference.pro").wstring());Framework insertedReload;insertedReload.open_project((referenceDir/L"Reference.pro").wstring());
    require(insertedReload.document(0).waves().size()==2&&insertedReload.playback_waves(0).back().bytes==sourceWave,"Native Project separately restores new Wave reference and owned audio");
    write_file_atomic((base/L"fresh-reference.sgp").wstring(),insertHost.document(insertSegment).save_bytes());

}
