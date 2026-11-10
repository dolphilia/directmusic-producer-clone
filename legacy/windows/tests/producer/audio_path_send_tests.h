#pragma once
// The fabricated effect class below tests document preservation only. Its
// bytes are NOT an observed/native Send CLSID and it is never instantiated.
void audio_path_send_tests(const std::filesystem::path& dir){
    const auto leaf=[](const char* id,Bytes bytes){Chunk c;c.id=id;c.data=std::move(bytes);return c;};
    const auto buffer=[](Chunk& root,size_t at)->Chunk&{size_t index=0;for(auto& c:root.children)if(c.id=="LIST"&&c.type=="dbfl"&&index++==at)return c;throw std::runtime_error("Test buffer absent");};
    const auto send=[&](const AudioBufferId& target){
        Bytes header(60);header[4]=0xa5;put32(header,0,0x11);std::copy(target.begin(),target.end(),header.begin()+36);put32(header,56,0x76543210);
        Chunk fx;fx.id="RIFF";fx.type="DSFX";fx.children={leaf("fxhr",header),leaf("data",{1,2,3,4,5,6,7,8}),leaf("xSnd",{0x45,0x67,0x89})};fx.children.back().padding=0x77;return fx;
    };
    const auto append=[&](Chunk& root,size_t at,Chunk effect){auto descriptor=buffer(root,at).find("RIFF","DSBC");if(!descriptor->find("LIST","fxls")){Chunk list;list.id="LIST";list.type="fxls";descriptor->children.push_back(list);}descriptor->find("LIST","fxls")->children.push_back(std::move(effect));};
    AudioPathDocument seed;require(seed.add_file_output(0),"Owned stereo source materialized without original DMO");
    require(seed.add_mixin_buffer(2)&&seed.add_mixin_buffer(1)&&seed.add_mixin_buffer(6),"Mix-in creation supports stereo mono and multichannel documents");
    auto initial=Chunk::parse(seed.save_bytes());AudioBufferId external{};external.fill(0x7e);append(initial,0,send(external));
    initial.children.push_back(leaf("xTop",{1,3,5}));initial.children.back().padding=0x91;
    AudioPathDocument document;document.load(initial.encode());const auto source=document.save_bytes();
    const auto details=document.buffer_details();require(details.size()==4&&details[0].channels==2&&details[0].routed&&details[0].synthBuses==2,"Source details distinguish audio channels from PChannels and synth buses");
    require(details[1].flags==8&&!details[1].routed&&!details[1].synthBuses&&details[1].channels==2,"Owned mix-in has one destination buffer no PChannels and no synth bus");
    require(document.effects().size()==2&&document.effects()[1].sendBuffer==external,"Imported external Send identity is retained without inventing a factory");
    require(document.send_destinations(0,1)==std::vector<size_t>{1},"Stereo Send offers only stereo compatible mix-in");
    require(document.send_destinations(0,0).empty()&&document.send_destinations(99,0).empty()&&document.send_destinations(0,99).empty(),"FileOutput and absent effect cannot be reinterpreted as Send");
    require(document.set_send_destination(0,1,1),"Explicit Send destination edit changes native GUID");
    auto expected=initial;auto& header=buffer(expected,0).find("RIFF","DSBC")->find("LIST","fxls")->children[1].find("fxhr")->data;
    std::copy(details[1].id.begin(),details[1].id.end(),header.begin()+36);
    const auto edited=document.save_bytes();require(edited==expected.encode(),"Send edit changes only destination GUID preserving class flags data tail padding and order");
    require(document.dirty()&&document.undo()&&document.save_bytes()==source&&!document.dirty()&&document.redo()&&document.save_bytes()==edited,"Send edit one complete document UndoRedo transaction");
    require(!document.set_send_destination(0,1,1)&&document.save_bytes()==edited,"Repeated Send destination is a no-op");
    const auto invalid=[&](const std::function<bool(AudioPathDocument&)>& change,const char* message){
        AudioPathDocument rejectedDoc;rejectedDoc.load(source);require(rejectedDoc.set_name(L"History marker")&&rejectedDoc.undo(),"Seed redo without changing baseline bytes");
        require(!change(rejectedDoc)&&rejectedDoc.save_bytes()==source&&!rejectedDoc.dirty(),message);
        require(rejectedDoc.redo()&&rejectedDoc.name()==L"History marker"&&rejectedDoc.undo()&&rejectedDoc.save_bytes()==source,"Rejected edit preserves UndoRedo and dirty state");
    };
    invalid([](auto& d){return d.set_send_destination(0,1,0);},"Self Send rejects without mutation");
    invalid([](auto& d){return d.set_send_destination(0,1,2);},"Stereo to mono Send rejects without mutation");
    invalid([](auto& d){return d.set_send_destination(0,1,3);},"Stereo to six-channel Send rejects without mutation");
    invalid([](auto& d){return d.set_send_destination(0,1,99);},"Absent Send destination rejects without mutation");
    invalid([](auto& d){return d.set_route_buffers(0,0,{d.buffers()[1]});},"PChannel connection to mix-in rejects without mutation");
    invalid([](auto& d){return d.add_mixin_buffer(0);},"Zero audio channels reject without mutation");
    invalid([](auto& d){return d.add_mixin_buffer(65536);},"Channel count beyond serialized WORD rejects without mutation");
    auto mono=initial;buffer(mono,0).find("RIFF","DSBC")->find("dsbd")->data[4]=1;AudioPathDocument monoDoc;monoDoc.load(mono.encode());
    require(monoDoc.send_destinations(0,1)==std::vector<size_t>({1,2}),"Mono Send permits mono and stereo destinations only");
    auto six=initial;buffer(six,0).find("RIFF","DSBC")->find("dsbd")->data[4]=6;AudioPathDocument sixDoc;sixDoc.load(six.encode());
    require(sixDoc.send_destinations(0,1)==std::vector<size_t>{3},"Multichannel Send requires exact destination channel count");
    auto bus=initial;buffer(bus,1).find("RIFF","DSBC")->children.push_back(leaf("bsid",Bytes(4)));AudioPathDocument busDoc;busDoc.load(bus.encode());
    require(busDoc.send_destinations(0,1).empty()&&!busDoc.set_send_destination(0,1,1)&&busDoc.save_bytes()==bus.encode(),"Mix-in with synth bus is preserved on import but not offered as new Send destination");
    auto ordinary=initial;put32(buffer(ordinary,1).find("ddah")->data,16,0);AudioPathDocument ordinaryDoc;ordinaryDoc.load(ordinary.encode());
    require(ordinaryDoc.send_destinations(0,1).empty(),"Disconnected ordinary buffer is not implicitly a mix-in destination");
    auto routed=initial;auto& route=routed.find("LIST","pcsl")->children[0].find("LIST","pchl")->children[0].data;std::copy(details[1].id.begin(),details[1].id.end(),route.begin()+16);AudioPathDocument routedDoc;routedDoc.load(routed.encode());
    require(routedDoc.send_destinations(0,1).empty(),"Mix-in with imported PChannel connection not eligible for new Send");
    auto cycle=initial;append(cycle,1,send(details[0].id));AudioPathDocument cycleDoc;cycleDoc.load(cycle.encode());
    require(cycleDoc.send_destinations(0,1).empty()&&!cycleDoc.set_send_destination(0,1,1)&&cycleDoc.save_bytes()==cycle.encode(),"Destination reaching source rejects local Send cycle without rewriting input");
    auto indirect=initial;buffer(indirect,2).find("RIFF","DSBC")->find("dsbd")->data[4]=2;append(indirect,1,send(details[2].id));append(indirect,2,send(details[0].id));AudioPathDocument indirectDoc;indirectDoc.load(indirect.encode());
    require(indirectDoc.send_destinations(0,1).empty(),"Cycle search traverses multiple mix-in Send edges");
    // A predefined target is replaced by add_file_output. Its incoming Send
    // and routing references must both follow the new identity; MIXIN survives.
    const auto materialize=[&](bool mixin){
        AudioPathDocument predefined;auto root=Chunk::parse(predefined.save_bytes());const auto originalId=predefined.buffers()[0];
        if(mixin){put32(buffer(root,0).find("ddah")->data,16,10);root.children.erase(std::remove_if(root.children.begin(),root.children.end(),[](const Chunk& c){return c.id=="LIST"&&c.type=="pcsl";}),root.children.end());}
        auto owner=buffer(initial,0);owner.find("RIFF","DSBC")->find("LIST","fxls")->children.clear();root.children.push_back(owner);append(root,1,send(originalId));
        AudioPathDocument d;d.load(root.encode());const auto before=d.save_bytes();const auto incoming=d.effects()[0];require(incoming.buffer==1&&incoming.sendBuffer==originalId,"Materialization fixture incoming Send owns old predefined GUID");
        require(d.add_file_output(0),"FileOutput materializes predefined destination");const auto after=d.save_bytes();const auto b=d.buffer_details();
        require(b[0].id!=originalId&&b[0].flags==(mixin?8u:0u)&&b[0].synthBuses==(mixin?0u:2u)&&b[0].routed==!mixin,"Materialization preserves mix-in role and only direct buffers get buses");
        const auto fx=d.effects();require(fx.size()==2&&fx[1].classId==incoming.classId&&fx[1].flags==incoming.flags&&fx[1].sendBuffer==b[0].id,"Incoming Send identity follows materialized target with class and options intact");
        if(!mixin)require(d.ports()[0].routes[0].buffers[0]==b[0].id,"PChannel identity follows materialized direct buffer");
        auto expectedOwner=owner;auto& sendHeader=expectedOwner.find("RIFF","DSBC")->find("LIST","fxls")->children;
        sendHeader.push_back(send(b[0].id));const auto actual=Chunk::parse(after);auto actualOwner=actual;require(buffer(actualOwner,1).encode()==expectedOwner.encode(),"Materialization preserves all incoming Send owner metadata except target identity");
        require(d.undo()&&d.save_bytes()==before&&!d.dirty()&&d.redo()&&d.save_bytes()==after,"Materialization and all incoming references are one UndoRedo transaction");
    };
    materialize(false);materialize(true);
    const auto file=dir/L"SendAuthoring.aup";document.save(file.wstring());AudioPathDocument restored;restored.load(read_file(file.wstring()));
    require(restored.save_bytes()==edited&&!restored.dirty()&&restored.effects()[1].sendBuffer==details[1].id,"Fresh native document roundtrip restores Send destination and opaque metadata");
    require(!restored.undo()&&!restored.redo(),"Native reload does not depend on in-memory edit history");
}
