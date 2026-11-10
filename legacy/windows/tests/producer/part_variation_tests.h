void part_variation_tests(const std::filesystem::path& dir,const std::wstring& segmentPath,const std::wstring& stylePath){
    const auto base=dir/L"part-variation";std::filesystem::create_directories(base);
    auto root=Chunk::parse(read_file(stylePath));auto* part=root.find("LIST","part");require(part!=nullptr,"variation input Part");
    part->find("prth")->data.push_back(0xed);part->find("prth")->padding=0xa7;Chunk unknown;unknown.id="vunk";unknown.data={1,2,3};unknown.padding=0xb6;part->children.push_back(unknown);
    const auto original=root.encode();StyleDocument doc;doc.load(original);const auto old=doc.parts()[0].variationChoices;
    require(!doc.set_part_variation_choice(0,32,1)&&!doc.set_part_variation_choice(999,0,1)&&!doc.set_part_variation_choice(0,0,old[0])&&!doc.undo()&&!doc.dirty(),"invalid unchanged variation creates no history");
    auto expected=root;auto& choices=expected.find("LIST","part")->find("prth")->data;put32(choices,4,0);put32(choices,128,0x91234567u);
    require(doc.set_part_variation_choice(0,0,0)&&doc.set_part_variation_choice(0,31,0x91234567u),"first last variation including high bits");
    const auto edited=doc.save_bytes();require(edited==expected.encode(),"only two eligibility DWORDs change; notes GUID tails padding remain");
    require(doc.parts()[0].variationChoices[0]==0&&doc.parts()[0].variationChoices[31]==0x91234567u,"zero eligibility and opaque mode bits read exactly");
    require(doc.undo()&&doc.parts()[0].variationChoices[31]==old[31]&&doc.undo()&&doc.save_bytes()==original&&!doc.dirty(),"variation Undo exact clean baseline");
    require(doc.redo()&&doc.redo()&&doc.save_bytes()==edited,"variation Redo exact");
    require(doc.duplicate_pattern(0,L"Shared Variations"),"shared Pattern setup");const auto refs=doc.part_references(0),shared=doc.part_references(doc.patterns().size()-1);
    require(refs[0].partIndex==shared[0].partIndex&&doc.set_part_variation_choice(shared[0].partIndex,15,0x40000000u)&&doc.parts()[refs[0].partIndex].variationChoices[15]==0x40000000u,"shared Part eligibility common to both Patterns");
    const auto packet=doc.copy_pattern(doc.patterns().size()-1);require(doc.paste_pattern(packet,L"Independent Variations"),"copy eligible Part");const auto copied=doc.part_references(doc.patterns().size()-1)[0].partIndex;
    require(copied!=refs[0].partIndex&&doc.parts()[copied].variationChoices==doc.parts()[refs[0].partIndex].variationChoices&&doc.set_part_variation_choice(copied,15,0)&&doc.parts()[refs[0].partIndex].variationChoices[15]==0x40000000u,"pasted eligibility independent after fresh identity");
    auto malformed=root;malformed.find("LIST","part")->find("prth")->data.resize(131);StyleDocument bad;bad.load(malformed.encode());rejected([&]{bad.set_part_variation_choice(0,0,1);},"truncated header rejects edit");require(bad.save_bytes()==malformed.encode()&&!bad.dirty(),"malformed data retained");
    write_file_atomic((base/L"original.stp").wstring(),original);write_file_atomic((base/L"edited.stp").wstring(),edited);
    write_file_atomic((base/L"Heartlnd.stp").wstring(),original);write_file_atomic((base/L"selection.sgp").wstring(),read_file(segmentPath));Framework host;const auto si=host.open_style((base/L"Heartlnd.stp").wstring()),di=host.open_segment((base/L"selection.sgp").wstring());const auto song=host.document(di).save_bytes();
    require(host.set_style_part_variation_choice(si,0,0,0)&&host.set_style_part_variation_choice(si,0,31,0x91234567u)&&host.style_document(si).save_bytes()==edited&&host.document(di).styles()[0].bytes==edited,"Framework eligible Part publishes owned snapshot");
    require(host.document(di).save_bytes()==song&&!host.document(di).dirty(),"dependent Segment bytes dirty unchanged");
    require(host.undo_style(si)&&host.undo_style(si)&&host.document(di).styles()[0].bytes==original&&host.redo_style(si)&&host.redo_style(si)&&host.document(di).styles()[0].bytes==edited,"Framework UndoRedo eligibility snapshots");
    host.save_style(si,(base/L"Heartlnd.stp").wstring());host.save_project((base/L"project.dmpj").wstring());Framework reload;reload.open_project((base/L"project.dmpj").wstring());
    require(reload.style_document(0).save_bytes()==edited&&reload.style_document(0).parts()[0].variationChoices[31]==0x91234567u&&reload.document(0).styles()[0].bytes==edited,"separate owner eligibility restore");
    reload.save_style(0,(base/L"resaved.stp").wstring());require(read_file((base/L"resaved.stp").wstring())==edited,"exact eligibility resave");
}
