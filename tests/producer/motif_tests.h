void motif_tests(const std::filesystem::path& dir,const std::wstring& segmentPath,const std::wstring& stylePath){
    const auto base=dir/L"motif";std::filesystem::create_directories(base);
    StyleDocument source;source.load(read_file(stylePath));const auto sourceBytes=source.save_bytes();
    const auto pi=source.patterns().size();require(source.new_motif(L"Owned Motif",5),"new owned Motif");
    auto created=source.save_bytes();const auto settings=source.motif_settings(pi);
    require(source.patterns()[pi].embellishment==16&&settings&&settings->repeats==0&&settings->playStart==0&&settings->loopEnd==0&&settings->resolution==1,"Motif default settings and kind");
    require(source.part_references(pi).size()==1&&source.part_references(pi)[0].pchannel==5u&&source.part_notes(source.part_references(pi)[0].partIndex).empty(),"owned fresh Part empty notes");
    require(!source.new_motif(L"Owned Motif",5)&&!source.new_motif(L"",5)&&!source.new_motif(L"Bad",16)&&source.save_bytes()==created,"invalid duplicate new Motif atomic");
    require(source.undo()&&source.save_bytes()==sourceBytes&&!source.dirty()&&!source.undo(),"creation single history transaction");
    require(source.redo()&&source.save_bytes()==created,"creation redo exact");
    auto enriched=Chunk::parse(created);Chunk* motif=nullptr;size_t at=0;for(auto& c:enriched.children)if(c.id=="LIST"&&c.type=="pttn"&&at++==pi)motif=&c;require(motif!=nullptr,"Motif chunk");
    motif->find("mtfs")->data.push_back(0xad);motif->find("mtfs")->padding=0xb7;Chunk unknown;unknown.id="munk";unknown.data={4,5,6};unknown.padding=0xc8;motif->children.push_back(unknown);
    const auto original=enriched.encode();source.load(original);StyleMotifSettings edited{0xffffffffu,384,768,2304,0x81234567u};
    require(source.set_motif_settings(pi,edited),"full repeats/resolution opaque DWORD and bounds edit");
    auto expected=enriched;at=0;for(auto& c:expected.children)if(c.id=="LIST"&&c.type=="pttn"&&at++==pi){auto& b=c.find("mtfs")->data;put32(b,0,0xffffffffu);put32(b,4,384);put32(b,8,768);put32(b,12,2304);put32(b,16,0x81234567u);}
    const auto changed=source.save_bytes();require(changed==expected.encode(),"only twenty settings bytes change tails padding unknowns intact");
    require(!source.set_motif_settings(pi,edited)&&!source.set_motif_settings(999,edited)&&!source.set_motif_settings(0,edited),"unchanged invalid and normal Pattern reject");
    for(const auto invalid:{StyleMotifSettings{1,-1,0,0,1},StyleMotifSettings{1,3072,0,0,1},StyleMotifSettings{1,0,-1,0,1},StyleMotifSettings{1,0,3072,0,1},StyleMotifSettings{1,0,768,768,1},StyleMotifSettings{1,0,0,3073,1}})require(!source.set_motif_settings(pi,invalid)&&source.save_bytes()==changed,"invalid Motif bounds atomic");
    require(source.undo()&&source.save_bytes()==original&&!source.dirty()&&!source.undo()&&source.redo()&&source.save_bytes()==changed,"settings exact single UndoRedo");
    auto truncated=enriched;at=0;for(auto& c:truncated.children)if(c.id=="LIST"&&c.type=="pttn"&&at++==pi)c.find("mtfs")->data.resize(19);StyleDocument bad;bad.load(truncated.encode());rejected([&]{bad.set_motif_settings(pi,edited);},"truncated Motif settings reject");require(!bad.dirty()&&bad.save_bytes()==truncated.encode(),"truncated retains bytes");
    auto duplicated=enriched;at=0;for(auto& c:duplicated.children)if(c.id=="LIST"&&c.type=="pttn"&&at++==pi){auto copy=*c.find("mtfs");c.children.push_back(copy);}bad.load(duplicated.encode());rejected([&]{bad.set_motif_settings(pi,edited);},"duplicate settings reject");
    StyleDocument fresh;fresh.load(sourceBytes);require(fresh.new_motif(L"Whole Loop",0)&&fresh.set_motif_settings(pi,StyleMotifSettings{2,0,0,0,1}),"whole Motif loop sentinel");
    write_file_atomic((base/L"original.stp").wstring(),original);write_file_atomic((base/L"edited.stp").wstring(),changed);write_file_atomic((base/L"Heartlnd.stp").wstring(),original);write_file_atomic((base/L"selection.sgp").wstring(),read_file(segmentPath));
    Framework host;const auto si=host.open_style((base/L"Heartlnd.stp").wstring()),di=host.open_segment((base/L"selection.sgp").wstring());const auto song=host.document(di).save_bytes();
    require(host.set_style_motif_settings(si,pi,edited)&&host.style_document(si).save_bytes()==changed&&host.document(di).styles()[0].bytes==changed,"Framework publishes Motif snapshot");
    require(host.document(di).save_bytes()==song&&!host.document(di).dirty(),"Segment bytes dirty unchanged");
    require(host.undo_style(si)&&host.document(di).styles()[0].bytes==original&&host.redo_style(si)&&host.document(di).styles()[0].bytes==changed,"Framework UndoRedo cache");
    host.save_style(si,(base/L"Heartlnd.stp").wstring());host.save_project((base/L"project.dmpj").wstring());Framework reload;reload.open_project((base/L"project.dmpj").wstring());
    require(reload.style_document(0).save_bytes()==changed&&reload.style_document(0).motif_settings(pi)->loopEnd==2304&&reload.document(0).styles()[0].bytes==changed,"separate Framework reload Motif");reload.save_style(0,(base/L"resaved.stp").wstring());require(read_file((base/L"resaved.stp").wstring())==changed,"Motif resave exact");
    require(reload.new_style_motif(0,L"Framework Motif",5)&&reload.document(0).styles()[0].bytes==reload.style_document(0).save_bytes()&&reload.undo_style(0)&&reload.style_document(0).save_bytes()==changed,"Framework new Motif one transaction");
}
