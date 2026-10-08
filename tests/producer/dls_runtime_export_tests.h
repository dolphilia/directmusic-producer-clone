void dls_runtime_export_tests(const std::filesystem::path& dir) {
    // Reuse the established public-contract fixture, not an old test result.
    dls_sample_policy_tests(dir);
    const auto base=dir/L"SampleInheritance";
    Framework host;host.open_project((base/L"SampleInheritance.pro").wstring());
    auto& document=host.collection_document(0);
    auto authored=Chunk::parse(document.save_bytes());
    auto& wave=*authored.find("LIST","wvpl")->children[0].find("wsmp");
    // Retain header/record extensions, a tail, unknown padding and independent
    // sampler metadata; expected output below is built without the exporter.
    wave.data.insert(wave.data.begin()+20,{0x51,0x52});put32(wave.data,0,22);
    wave.data.insert(wave.data.end(),{0x61,0x62,0x63,0x64});put32(wave.data,22,20);
    wave.data.push_back(0x71);
    Chunk sampler;sampler.id="smpl";sampler.data=Bytes(36,0);
    authored.find("LIST","wvpl")->children[0].children.push_back(sampler);
    Chunk design;design.id="dmpr";design.data={1,2,3};design.padding=0x9a;
    authored.children.push_back(design);
    auto& region=authored.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0];
    region.children.push_back(design);
    Chunk opaque;opaque.id="zzzz";opaque.data={9,8,7};opaque.padding=0xad;region.children.push_back(opaque);
    document.load(authored.encode());
    const auto explicitBytes=document.save_bytes();
    require(document.inherit_wave_sample(0,0),"Runtime fixture inherits extended Wave sample settings");
    const auto inherited=document.save_bytes();
    auto expected=Chunk::parse(inherited);
    auto& expectedRegion=expected.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0];
    expectedRegion.children.push_back(*expected.find("LIST","wvpl")->children[0].find("wsmp"));
    const auto removeDesign=[&](auto&& self,Chunk& chunk)->void {
        if(chunk.type=="DLS "||chunk.type=="rgn "||chunk.type=="rgn2")
            chunk.children.erase(std::remove_if(chunk.children.begin(),chunk.children.end(),[](const Chunk& c){return c.id=="dmpr";}),chunk.children.end());
        for(auto& child:chunk.children)if(child.container())self(self,child);
    };
    removeDesign(removeDesign,expected);const auto expectedBytes=expected.encode();
    const auto standalone=dir/L"Standalone.dls";
    host.save_runtime(RuntimeDocumentKind::Collection,0,standalone.wstring());
    require(read_file(standalone.wstring())==expectedBytes,"Standalone runtime DLS materializes entire inherited WSMP and strips only observed design metadata");
    require(document.save_bytes()==inherited&&document.dirty()&&document.undo()&&document.save_bytes()==explicitBytes&&document.redo()&&document.save_bytes()==inherited,"Standalone runtime save preserves native absent WSMP, dirty checkpoint and exact UndoRedo");
    host.save_collection(0,(base/L"LoopSource.dls").wstring());host.save_project((base/L"SampleInheritance.pro").wstring());
    const auto projectBytes=read_file((base/L"SampleInheritance.pro").wstring());
    host.export_runtime((dir/L"Bulk").wstring());
    require(read_file((dir/L"Bulk"/L"LoopSource.dls").wstring())==expectedBytes,"Whole Project export uses the same inherited sample conversion");
    require(read_file((base/L"LoopSource.dls").wstring())==inherited&&read_file((base/L"SampleInheritance.pro").wstring())==projectBytes&&!host.dirty(),"Whole export retains saved native DLS and Project exactly");
    Framework runtime;runtime.open_collection((dir/L"Bulk"/L"LoopSource.dls").wstring());runtime.open_segment((dir/L"Bulk"/L"LoopSong.sgt").wstring());
    const auto resolved=runtime.playback_collections(0);
    require(resolved.size()==1&&resolved[0].bytes==expectedBytes&&collection_identity(expectedBytes)==collection_identity(inherited),"Fresh runtime Segment resolves only exported collection with unchanged identity");
    host.set_runtime_component_folder(RuntimeDocumentKind::Collection,L"..\\Configured\\samples\\");
    host.set_runtime_filename(RuntimeDocumentKind::Collection,0,L"Inherited.dls");
    host.save_project((base/L"SampleInheritance.pro").wstring());
    host.export_runtime_defaults();
    require(read_file((dir/L"Configured"/L"samples"/L"Inherited.dls").wstring())==expectedBytes,"Configured runtime collection publication resolves inheritance after dependency rewriting");
    require(document.save_bytes()==inherited&&!document.dirty()&&document.undo()&&document.save_bytes()==explicitBytes&&document.redo()&&document.save_bytes()==inherited&&!document.dirty(),"Configured publication retains saved checkpoint and document history");
    (void)document.set_region_loops(0,0,{});const auto oneShot=document.save_bytes();auto explicitExpected=Chunk::parse(oneShot);removeDesign(removeDesign,explicitExpected);
    host.save_runtime(RuntimeDocumentKind::Collection,0,(dir/L"OneShot.dls").wstring());
    require(read_file((dir/L"OneShot.dls").wstring())==explicitExpected.encode()&&document.save_bytes()==oneShot,"Explicit zero-loop override remains one shot in runtime output");
    document.load(inherited);
    auto invalid=Chunk::parse(inherited);auto& invalidSample=invalid.find("LIST","wvpl")->children[0].find("wsmp")->data;invalidSample[4]=128;
    const auto invalidBytes=invalid.encode();document.load(invalidBytes);require(document.set_instrument(0,0,1)&&document.undo(),"Invalid export fixture has a pending Redo");
    const auto prior=read_file(standalone.wstring());
    rejected([&]{host.save_runtime(RuntimeDocumentKind::Collection,0,standalone.wstring());},"Invalid inherited Unity Note refuses standalone output before publication");
    require(read_file(standalone.wstring())==prior&&document.save_bytes()==invalidBytes&&!document.dirty()&&document.redo()&&document.instruments()[0].program==1,"Invalid runtime refusal retains old destination, source checkpoint and pending Redo");
    document.load(invalidBytes);host.save_collection(0,(base/L"LoopSource.dls").wstring());host.save_project((base/L"SampleInheritance.pro").wstring());
    rejected([&]{host.export_runtime((dir/L"InvalidBulk").wstring());},"Invalid inherited sample refuses whole runtime Project before publication");
    require(!std::filesystem::exists(dir/L"InvalidBulk")&&document.save_bytes()==invalidBytes&&!host.dirty(),"Rejected whole export leaves no published output or native mutation");
    const auto configuredPrior=read_file((dir/L"Configured"/L"samples"/L"Inherited.dls").wstring());
    rejected([&]{host.export_runtime_defaults();},"Invalid sample refuses configured runtime update");
    require(read_file((dir/L"Configured"/L"samples"/L"Inherited.dls").wstring())==configuredPrior&&document.save_bytes()==invalidBytes&&!host.dirty(),"Rejected configured update retains prior runtime and native bytes");
    // Keep a valid, explicit source carrier for subsequent real GUI authoring.
    document.load(explicitBytes);host.save_collection(0,(base/L"LoopSource.dls").wstring());host.save_project((base/L"SampleInheritance.pro").wstring());
}
