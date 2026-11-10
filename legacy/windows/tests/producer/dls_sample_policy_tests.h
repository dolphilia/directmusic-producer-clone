void dls_sample_policy_tests(const std::filesystem::path& dir) {
    Chunk wav;wav.id="RIFF";wav.type="WAVE";
    Chunk fmt;fmt.id="fmt ";fmt.data=Bytes(16);fmt.data[0]=1;fmt.data[2]=1;put32(fmt.data,4,16000);put32(fmt.data,8,32000);fmt.data[12]=2;fmt.data[14]=16;
    Chunk pcm;pcm.id="data";pcm.data.resize(32000);
    for(size_t i=0;i<16000;++i){const auto x=static_cast<std::int16_t>(std::lround(6000*std::sin(2*3.14159265358979323846*261.6255653005986*i/16000)));pcm.data[i*2]=static_cast<std::uint8_t>(x);pcm.data[i*2+1]=static_cast<std::uint8_t>(x>>8);}
    wav.children={fmt,pcm};DlsDocument d=DlsDocument::create();
    require(d.add_wave_pcm(wav.encode(),"Policy tone")&&d.create_instrument(0,0,"Policy instrument",0),"Sample policy portable tone and one Region");
    require(d.set_wave_loops(0,{{0,320,11987}}),"Wave default loop is authored independently");
    (void)d.set_region_loops(0,0,{}); // Creation already installs an explicit one-shot WSMP.
    const auto source=d.save_bytes();d.load(source);
    require(!d.region_inherits_wave_sample(0,0)&&d.effective_region_loops(0,0).empty()&&d.wave_loops(0).size()==1,"No manufacture of effective loops across explicit one-shot override");
    auto expected=Chunk::parse(source);auto& region=expected.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0];
    region.children.erase(std::remove_if(region.children.begin(),region.children.end(),[](const Chunk& c){return c.id=="wsmp";}),region.children.end());
    require(d.inherit_wave_sample(0,0)&&d.save_bytes()==expected.encode()&&d.region_inherits_wave_sample(0,0),"Inheritance removes only entire Region WSMP, retaining all other bytes");
    require(d.effective_region_loops(0,0).size()==1&&d.effective_region_loops(0,0)[0].start==320&&d.effective_region_loops(0,0)[0].length==11987,"Inherited effective loop follows linked Wave");
    require(d.undo()&&d.save_bytes()==source&&!d.dirty()&&d.redo()&&d.save_bytes()==expected.encode(),"Inheritance is one exact checkpoint-aware UndoRedo transaction");
    const auto inherited=d.save_bytes();require(!d.inherit_wave_sample(0,0)&&d.save_bytes()==inherited,"Already inherited settings are a byte-preserving no-op");
    auto runtimeExpected=Chunk::parse(inherited);auto& runtimeRegion=runtimeExpected.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0];
    runtimeRegion.children.push_back(*runtimeExpected.find("LIST","wvpl")->children[0].find("wsmp"));
    require(d.playback_sample_bytes()==runtimeExpected.encode()&&d.save_bytes()==inherited&&d.dirty()&&!d.region_loops(0,0).size(),"Private runtime copy resolves entire inherited Wave WSMP without publishing an editor override");
    require(d.undo()&&d.save_bytes()==source&&d.redo()&&d.save_bytes()==inherited,"Read-only runtime resolution retains editor UndoRedo history");
    require(d.set_region_loops(0,0,{})&&d.effective_region_loops(0,0).empty()&&!d.region_inherits_wave_sample(0,0),"Explicit empty loop list restores one-shot without changing Wave defaults");
    const auto oneShot=d.save_bytes();const auto explicitRoot=Chunk::parse(oneShot);const auto& explicitSample=explicitRoot.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0].find("wsmp")->data;
    require(d.playback_sample_bytes()==oneShot,"Explicit Region one shot remains explicit in the private runtime copy");
    const auto sourceRoot=Chunk::parse(source);require(explicitSample==sourceRoot.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0].find("wsmp")->data,"One-shot override retains the Wave sample header");
    require(d.undo()&&d.save_bytes()==inherited&&d.redo()&&d.save_bytes()==oneShot,"New explicit WSMP placement has exact UndoRedo history");
    for(const unsigned note:{0u,127u}){auto valid=Chunk::parse(source);auto& w=valid.find("LIST","wvpl")->children[0].find("wsmp")->data;w[4]=static_cast<std::uint8_t>(note);w[5]=0;d.load(valid.encode());d.validate_playback_samples();require(d.inherit_wave_sample(0,0),"Unity-note boundary is allowed for inherited playback");}
    auto invalid=Chunk::parse(source);auto& bad=invalid.find("LIST","wvpl")->children[0].find("wsmp")->data;bad[4]=128;bad[5]=0;
    const auto invalidBytes=invalid.encode();d.load(invalidBytes);
    require(d.set_instrument(0,0,1)&&d.undo()&&d.save_bytes()==invalidBytes&&!d.dirty(),"Invalid default fixture keeps a pending Redo without normalization");
    rejected([&]{d.inherit_wave_sample(0,0);},"Invalid inherited Wave unity note rejected before Region override removal");
    require(d.save_bytes()==invalidBytes&&!d.dirty()&&d.redo()&&d.instruments()[0].program==1,"Rejected inheritance preserves exact bytes, dirty checkpoint and pending Redo");
    d.load(invalidBytes);rejected([&]{d.validate_playback_samples();},"Malformed Wave unity note refused by read-only playback preflight");require(d.save_bytes()==invalidBytes&&!d.dirty()&&!d.undo(),"Rejected preflight does not mutate collection or history");
    invalid=Chunk::parse(source);auto& badRegion=invalid.find("LIST","lins")->children[0].find("LIST","lrgn")->children[0].find("wsmp")->data;badRegion[4]=128;badRegion[5]=0;d.load(invalid.encode());rejected([&]{d.validate_playback_samples();},"Invalid explicit Region unity note is refused even with no loops");
    CollectionReference ref{L"LoopSource.dls",{}};BandDocument band;band.add_gm_instrument(0,0,64,100);band.set_collection_reference(0,ref);SegmentDocument song;song.set_band(0,band.save_bytes());
    const auto songBytes=song.save_bytes();const std::vector<ResolvedCollection> dependencies{{ref,L"C:\\owned\\LoopSource.dls",invalidBytes}};
    const auto runtimePlayback=prepare_collection_playback({songBytes},{{ref,L"C:\\owned\\LoopSource.dls",inherited}});
    require(runtimePlayback.collections[0].bytes==runtimeExpected.encode()&&runtimePlayback.collections[0].reference.objectId==collection_identity(inherited)&&song.save_bytes()==songBytes,"Band playback maps inherited samples in a private collection while retaining owned GUID and caller document");
    const std::vector<ResolvedCollection> colliding{{ref,L"C:\\owned\\LoopSource.dls",inherited},{ref,L"C:\\other\\LoopSource.dls",runtimeExpected.encode()}};
    rejected([&]{prepare_collection_playback({songBytes,songBytes},colliding);},"Runtime sample resolution must not hide conflicting owned snapshots sharing a GUID");
    require(colliding[0].bytes==inherited&&colliding[1].bytes==runtimeExpected.encode(),"GUID conflict rejection retains both caller snapshots including inheritance representation");
    rejected([&]{prepare_collection_playback({songBytes},dependencies);},"Invalid sample is rejected before a runtime playback snapshot is published");require(song.save_bytes()==songBytes&&dependencies[0].bytes==invalidBytes,"Rejected runtime preparation retains caller snapshots");
    // Independent fresh carrier for main/save/reload/PCM. No original executable.
    const auto base=dir/L"SampleInheritance";std::filesystem::create_directories(base);
    write_file_atomic((base/L"LoopSource.dls").wstring(),source);
    Framework host;const auto ci=host.open_collection((base/L"LoopSource.dls").wstring()),bi=host.new_band(),si=host.new_segment();host.add_band_gm_instrument(bi,0,0,64,100);host.save_band(bi,(base/L"LoopBand.bnp").wstring());require(host.set_band_collection(bi,0,ci),"Inheritance carrier Band owns DLS reference");host.save_band(bi,(base/L"LoopBand.bnp").wstring());
    auto& segment=host.document(si);segment.add_tempo(0,12);segment.add_note({0,1152,0,60,96});segment.add_note({1536,1152,0,60,96});segment.set_band(0,host.band_document(bi).save_bytes());
    host.save_segment(si,(base/L"LoopSong.sgp").wstring());host.save_project((base/L"SampleInheritance.pro").wstring());
    Framework reload;reload.open_project((base/L"SampleInheritance.pro").wstring());require(reload.collection_document(0).save_bytes()==source&&reload.document(0).notes().size()==2&&!reload.collection_document(0).region_inherits_wave_sample(0,0),"Native carrier restores explicit one-shot override over looping Wave default");
}
