namespace script_dependency_tests {
Chunk reference(REFGUID cls,const Bytes& id,const std::wstring& file){
    Chunk r;r.id="LIST";r.type="DMRF";Chunk h;h.id="refh";h.data.resize(20);std::memcpy(h.data.data(),&cls,16);put32(h.data,16,0x13);r.children.push_back(h);
    Chunk g;g.id="guid";g.data=id;r.children.push_back(g);Chunk f;f.id="file";f.data=utf16(file);r.children.push_back(f);return r;
}
Bytes contained(const Chunk& payload,REFGUID cls){
    ScriptDocument doc;doc.set_source(L"Sub LoadContent\r\nContent.Load\r\nEnd Sub\r\n");auto root=Chunk::parse(doc.save_bytes());auto c=root.find("RIFF","DMCN");put32(c->find("conh")->data,0,2);
    Chunk entry;entry.id="LIST";entry.type="cobl";Chunk a;a.id="coba";a.data=utf16(L"Content");entry.children.push_back(a);Chunk h;h.id="cobh";h.data.resize(28);std::memcpy(h.data.data(),&cls,16);put32(h.data,16,1);std::copy_n(payload.id.data(),4,h.data.begin()+20);std::copy_n(payload.type.data(),4,h.data.begin()+24);entry.children.push_back(h);entry.children.push_back(payload);c->find("LIST","cosl")->children.push_back(entry);return root.encode();
}
std::vector<const Chunk*> refs(const Chunk& root){std::vector<const Chunk*> out;std::function<void(const Chunk&)> walk=[&](const Chunk& c){if(c.id=="LIST"&&c.type=="DMRF")out.push_back(&c);for(const auto& child:c.children)walk(child);};walk(root);return out;}
void descriptors(const std::filesystem::path& dir){
    const auto base=dir/L"descriptors";std::filesystem::create_directories(base);
    auto field=[](Chunk& root,const char* id,const Bytes& bytes){if(auto c=root.find(id))c->data=bytes;else{Chunk added;added.id=id;added.data=bytes;root.children.push_back(added);}};
    StyleDocument a,b;require(a.set_name(L"Owned \u97f3\u8272")&&b.set_name(L"Other Style"),"Owned Unicode descriptor inputs named");
    auto first=Chunk::parse(a.save_bytes()),second=Chunk::parse(b.save_bytes());field(first,"catg",utf16(L"Category A"));field(second,"catg",utf16(L"Category B"));
    const auto firstBytes=first.encode(),secondBytes=second.encode();write_file_atomic((base/L"First.sty").wstring(),firstBytes);write_file_atomic((base/L"Second.sty").wstring(),secondBytes);
    auto named=reference(producer::runtime::styleClass,first.find("guid")->data,L"Second.sty");put32(named.find("refh")->data,16,6);field(named,"name",utf16(L"Owned \u97f3\u8272"));field(named,"catg",utf16(L"Category A"));
    named.find("refh")->data.insert(named.find("refh")->data.end(),{0xa1,0xb2});Chunk opaque;opaque.id="keep";opaque.data={7,8,9};opaque.padding=0x6d;named.children.push_back(opaque);
    auto entry=[&](const Chunk& payload,const std::wstring& alias,REFGUID cls=producer::runtime::styleClass){auto script=Chunk::parse(contained(payload,cls));auto result=script.find("RIFF","DMCN")->find("LIST","cosl")->children.front();result.find("coba")->data=utf16(alias);return result;};
    auto scriptFor=[&](const Chunk& ref,const std::vector<Chunk>& owners){auto script=Chunk::parse(contained(ref,producer::runtime::styleClass));auto& entries=script.find("RIFF","DMCN")->find("LIST","cosl")->children;entries.insert(entries.end(),owners.begin(),owners.end());return script;};
    const std::vector<Chunk> owners={entry(first,L"FirstOwner"),entry(second,L"SecondOwner")};
    auto check=[&](const Chunk& script,const Chunk& target,const std::wstring& directory){const auto bytes=script.encode();const auto result=prepare_script_runtime(bytes,directory);const auto prepared=Chunk::parse(result.script);require(prepared.find("RIFF","DMCN")->find("LIST","cosl")->children.front().children.back().encode()==target.encode(),"Descriptor selects independently specified native target");ScriptDocument native;native.load(bytes);require(native.save_bytes()==bytes&&script.encode()==bytes&&!native.dirty(),"Descriptor selection leaves caller/native document bytes and dirty state unchanged");require(read32(prepared.find("RIFF","DMCN")->find("conh")->data,0)==2&&read32(prepared.find("RIFF","DMCN")->find("LIST","cosl")->children.front().find("cobh")->data,16)==1,"Descriptor keeps native NOLOADS and KEEP flags in private copy");return result;};
    const auto nameOnly=check(scriptFor(named,owners),first,L"");require(nameOnly.selections.front().requestedName&&nameOnly.selections.front().selectedByName&&!nameOnly.selections.front().requestedFile&&!nameOnly.selections.front().selectedByGuid&&!nameOnly.selections.front().selectedByCategory,"Forward owned name-only selection ignores inactive filename/GUID/category without directory search");
    auto byCategory=named;put32(byCategory.find("refh")->data,16,14);const auto categorized=check(scriptFor(byCategory,owners),first,L"");require(categorized.selections.front().selectedByCategory,"Active NAME and CATEGORY selects owned metadata");
    field(byCategory,"catg",utf16(L"Absent Category"));const auto fallbackName=check(scriptFor(byCategory,owners),first,L"");require(fallbackName.selections.front().selectedByName&&!fallbackName.selections.front().selectedByCategory,"SDK category miss falls back to name rather than requiring every valid field to match");
    field(byCategory,"catg",utf16(L""));const auto emptyCategory=check(scriptFor(byCategory,owners),first,L"");require(emptyCategory.selections.front().requestedCategory&&emptyCategory.selections.front().category.empty()&&emptyCategory.selections.front().selectedByName,"Empty active category is valid and permits name fallback");
    auto stale=named;put32(stale.find("refh")->data,16,7);stale.find("guid")->data[0]^=0x65;const auto staleResult=check(scriptFor(stale,owners),first,L"");require(staleResult.selections.front().requestedGuid&&!staleResult.selections.front().selectedByGuid&&staleResult.selections.front().selectedByName,"Unowned stale GUID falls back to owned internal name");
    auto guidWins=named;put32(guidWins.find("refh")->data,16,0x17);guidWins.find("guid")->data=second.find("guid")->data;const auto guidResult=check(scriptFor(guidWins,owners),second,base.wstring());require(guidResult.selections.front().selectedByGuid&&!guidResult.selections.front().selectedByName,"Owned GUID retains priority over conflicting internal name and local filename");
    auto local=named;put32(local.find("refh")->data,16,0x16);const auto localResult=check(scriptFor(local,owners),first,base.wstring());require(localResult.selections.front().requestedFile&&localResult.selections.front().selectedByName&&!localResult.selections.front().selectedByFullPath,"Internal name outranks a different existing local filename");
    auto full=local;put32(full.find("refh")->data,16,0x36);full.find("file")->data=utf16((base/L"Second.sty").wstring());const auto fullResult=check(scriptFor(full,owners),second,L"");require(fullResult.selections.front().selectedByFullPath&&!fullResult.selections.front().selectedByName,"Explicit full path outranks conflicting name");
    auto missing=local;missing.find("file")->data=utf16(L"Missing.sty");const auto missingResult=check(scriptFor(missing,owners),first,base.wstring());require(missingResult.selections.front().selectedByName&&missingResult.selections.front().fallbackReadError=="Unable to open input file","Owned name retains original IO observation for unavailable lower-priority file");
    const auto noDirectory=check(scriptFor(missing,owners),first,L"");require(noDirectory.selections.front().fallbackPath.empty()&&noDirectory.selections.front().fallbackReadError.empty(),"Unused relative filename does not invent an IO observation when name is owned");
    auto fileFallback=local;field(fileFallback,"name",utf16(L"Unowned name"));const auto fileResult=check(scriptFor(fileFallback,owners),second,base.wstring());require(!fileResult.selections.front().selectedByName&&!fileResult.selections.front().selectedByFullPath,"Name miss uses declared local filename without registry or directory enumeration");
    auto duplicate=second;duplicate.find("LIST","UNFO")->find("UNAM")->data=utf16(L"Owned \u97f3\u8272");const std::vector<Chunk> duplicates={entry(first,L"FirstOwner"),entry(duplicate,L"OtherOwner")};
    auto disambiguated=named;put32(disambiguated.find("refh")->data,16,14);field(disambiguated,"catg",utf16(L"Category B"));const auto categoryResult=check(scriptFor(disambiguated,duplicates),duplicate,L"");require(categoryResult.selections.front().selectedByCategory,"Category resolves equal names with different owned GUIDs");
    auto reversed=duplicates;std::reverse(reversed.begin(),reversed.end());check(scriptFor(disambiguated,reversed),duplicate,L"");
    rejected([&]{prepare_script_runtime(scriptFor(named,duplicates).encode(),L"");},"Unverified native same-name collision policy rejects rather than silently depending on alias order");
    auto duplicateBytes=owners;duplicateBytes.push_back(entry(first,L"SameObject"));check(scriptFor(named,duplicateBytes),first,L"");
    SegmentDocument segment;auto song=Chunk::parse(segment.save_bytes());Chunk unfo;unfo.id="LIST";unfo.type="UNFO";Chunk unam;unam.id="UNAM";unam.data=utf16(L"Owned \u97f3\u8272");unfo.children.push_back(unam);song.children.push_back(unfo);
    const std::vector<Chunk> classOwners={entry(song,L"SameNameSegment",producer::runtime::segmentClass),entry(first,L"StyleOwner")};check(scriptFor(named,classOwners),first,L"");
    rejected([&]{prepare_script_runtime(scriptFor(named,{classOwners.front()}).encode(),L"");},"Name selection is scoped to requested native class");
    song.children.push_back(named);auto nested=Chunk::parse(contained(song,producer::runtime::segmentClass));nested.find("RIFF","DMCN")->find("LIST","cosl")->children.push_back(entry(first,L"NestedOwner"));const auto nestedBytes=nested.encode();const auto nestedResult=prepare_script_runtime(nestedBytes,L"");
    auto expectedSong=song;put32(expectedSong.children.back().find("refh")->data,16,3);expectedSong.children.back().find("guid")->data=first.find("guid")->data;const auto nestedPrepared=Chunk::parse(nestedResult.script);
    require(nestedResult.dependencies.size()==1&&nestedResult.dependencies.front().sourceBytes==firstBytes&&nestedResult.selections.front().selectedByName&&nestedPrepared.find("RIFF","DMCN")->find("LIST","cosl")->children.front().children.back().encode()==expectedSong.encode()&&nested.encode()==nestedBytes,"Nested name resolves forward owner and changes only private GUID flags while preserving tail, padding, inactive file and opaque bytes");
    auto invalid=named;invalid.children.erase(std::remove_if(invalid.children.begin(),invalid.children.end(),[](const auto& c){return c.id=="name";}),invalid.children.end());rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Active NAME missing chunk rejects");
    invalid=named;field(invalid,"name",utf16(L""));rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Empty active NAME rejects");
    invalid=named;field(invalid,"name",Bytes{0,0xd8,0,0});rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Invalid surrogate NAME rejects");
    invalid=named;field(invalid,"name",Bytes{65,0,66,0});rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Unterminated active NAME rejects");
    invalid=named;field(invalid,"name",utf16(std::wstring(L"Owned\0Other",11)));rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Embedded NUL active NAME rejects");
    invalid=named;invalid.children.push_back(*invalid.find("name"));rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Duplicate active NAME field rejects");
    invalid=named;put32(invalid.find("refh")->data,16,14);invalid.children.erase(std::remove_if(invalid.children.begin(),invalid.children.end(),[](const auto& c){return c.id=="catg";}),invalid.children.end());rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),L"");},"Active CATEGORY requires a chunk");
    invalid=full;invalid.find("file")->data=utf16(L"Second.sty");rejected([&]{prepare_script_runtime(scriptFor(invalid,owners).encode(),base.wstring());},"FULLPATH flag rejects relative path");
    auto longName=first;longName.find("LIST","UNFO")->find("UNAM")->data=utf16(std::wstring(63,L'a'));auto boundary=named;field(boundary,"name",utf16(std::wstring(63,L'a')));check(scriptFor(boundary,{entry(longName,L"Boundary")}),longName,L"");field(boundary,"name",utf16(std::wstring(64,L'a')));rejected([&]{prepare_script_runtime(scriptFor(boundary,owners).encode(),L"");},"SDK descriptor capacity includes terminating NUL");
    auto ambiguousMetadata=first;ambiguousMetadata.children.push_back(*ambiguousMetadata.find("LIST","UNFO"));rejected([&]{prepare_script_runtime(scriptFor(named,{entry(ambiguousMetadata,L"BadMetadata")}).encode(),L"");},"Ambiguous owned Unicode metadata rejects name selection");
    write_file_atomic((base/L"NamedScript.spp").wstring(),scriptFor(byCategory,owners).encode());require(read_file((base/L"First.sty").wstring())==firstBytes&&read_file((base/L"Second.sty").wstring())==secondBytes,"All descriptor selection controls leave original files unchanged");
}
void owned(const std::filesystem::path& dir,const std::filesystem::path& farm){
    const auto original=read_file(farm.wstring());const auto snapshot=prepare_script_runtime(original,farm.parent_path().wstring());
    const auto before=Chunk::parse(original),after=Chunk::parse(snapshot.script);ScriptDocument native;native.load(original);
    require(snapshot.dependencies.size()==8,"Farm input resolves one Style one DLS and six Waves");
    require(native.save_bytes()==original&&read_file(farm.wstring())==original,"Script snapshot leaves input and native document bytes unchanged");
    auto expected=before;*expected.find("RIFF","DMCN")=*after.find("RIFF","DMCN");require(expected.encode()==snapshot.script,"Farm snapshot changes only contained runtime payloads");
    const auto originalObjects=native.container().objects();ContainerGraph mapped(after.find("RIFF","DMCN")->encode());const auto runtimeObjects=mapped.objects();
    require(originalObjects.size()==10&&runtimeObjects.size()==10&&mapped.flags()==native.container().flags(),"Ten Farm aliases and NOLOADS are retained");
    for(size_t i=0;i<runtimeObjects.size();++i)require(runtimeObjects[i].alias==originalObjects[i].alias&&runtimeObjects[i].flags==originalObjects[i].flags&&runtimeObjects[i].payload.type=="DMSG"&&!runtimeObjects[i].reference,"Farm alias/KEEP preserved with owned embedded Segment");
    auto verify=[&](const Chunk& c){for(const auto r:refs(c)){const auto h=r->find("refh"),g=r->find("guid");require(h&&g&&read32(h->data,16)==3&&g->data.size()==16,"Nested reference uses GUID+CLASS with no filename fallback");require(std::any_of(snapshot.dependencies.begin(),snapshot.dependencies.end(),[&](const auto& d){return std::equal(d.classId.begin(),d.classId.end(),h->data.begin())&&Bytes(d.objectId.begin(),d.objectId.end())==g->data;}),"Every nested reference matches an owned dependency");}};
    verify(after);
    for(const auto& d:snapshot.dependencies){require(read_file(d.path)==d.sourceBytes,"Farm dependency source remains exact on disk");const auto root=Chunk::parse(d.runtimeBytes);const auto guid=root.find(root.type=="DLS "?"dlid":"guid");require(guid&&guid->data==Bytes(d.objectId.begin(),d.objectId.end()),"Owned dependency retains native GUID including DLS dlid");verify(root);}
    require(std::count_if(snapshot.selections.begin(),snapshot.selections.end(),[](const auto& s){return s.requestedGuid&&!s.selectedByGuid&&s.requestedId!=s.selectedId;})==10,"Farm stale top-level reference GUIDs select only explicit files");
    size_t embeddedPaths=0;
    for(size_t i=0;i<originalObjects.size();++i){
        const auto file=originalObjects[i].payload.find("file");require(file!=nullptr,"Supplied Farm alias has an explicit native input");
        const auto nativeSegment=Chunk::parse(read_file((farm.parent_path()/std::filesystem::path(decode_utf16(file->data))).wstring()));
        if(const auto config=nativeSegment.find("RIFF","DMAP")){++embeddedPaths;const auto prepared=runtimeObjects[i].payload.find("RIFF","DMAP");require(prepared&&prepared->encode()==config->encode(),"Farm embedded AudioPath routes effect parameters identity and padding remain byte exact");}
    }
    require(embeddedPaths==7,"Independent supplied Farm input has seven embedded AudioPath configurations");
    const GUID configClass={0xee0b9ca0,0xa81e,0x11d3,{0x9b,0xd1,0,0x80,0xc7,0x15,0x0a,0x74}};
    require(std::any_of(snapshot.requirements.begin(),snapshot.requirements.end(),[&](const auto& r){return IsEqualGUID(r.classId,configClass)&&std::wstring(r.server)==L"dmime.dll";}),"Farm embedded config requires its declared Windows class before Loader activation");
    require(std::any_of(snapshot.requirements.begin(),snapshot.requirements.end(),[](const auto& r){return std::wstring(r.server)==L"dsdmo.dll";}),"Farm standard effect requires declared system dsdmo provenance before Loader activation");
    // Native test output is evidence only; no substitute for Producer save/reload.
    write_file_atomic((dir/L"FarmRuntimeSnapshot.spt").wstring(),snapshot.script);
    const auto base=dir/L"GraphControls";std::filesystem::create_directories(base/L"nested");
    auto song=Chunk::parse(SegmentDocument::playback_test().save_bytes());Chunk opaque;opaque.id="zzzz";opaque.data={2,4,7};opaque.padding=0xbd;song.children.push_back(opaque);
    AudioPathDocument plainPath;auto configured=song;configured.children.push_back(Chunk::parse(plainPath.save_bytes()));
    const auto configSource=contained(configured,producer::runtime::segmentClass);const auto configSnapshot=prepare_script_runtime(configSource,L"");ScriptDocument configNative;configNative.load(configSource);
    const auto configuredRoot=Chunk::parse(configSnapshot.script);const auto configuredObjects=configuredRoot.find("RIFF","DMCN")->find("LIST","cosl");
    require(configuredObjects->children[0].children.back().encode()==configured.encode()&&configNative.save_bytes()==configSource,"Embedded plain AudioPath validates without changing private payload bytes or native source");
    auto invalidConfig=configured;invalidConfig.find("RIFF","DMAP")->find("LIST","pcsl")->children[0].find("LIST","pchl")->children[0].data[16]^=1;
    rejected([&]{prepare_script_runtime(contained(invalidConfig,producer::runtime::segmentClass),L"");},"Malformed embedded AudioPath missing route buffer rejects before activation");
    AudioPathDocument effectPath;require(effectPath.add_waves_reverb(0),"Embedded standard effect control prepared");auto unownedEffect=Chunk::parse(effectPath.save_bytes());
    auto effectHeader=unownedEffect.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls")->children[0].find("fxhr");std::fill_n(effectHeader->data.begin()+4,16,0);
    auto unknownEffectSong=song;unknownEffectSong.children.push_back(unownedEffect);
    rejected([&]{prepare_script_runtime(contained(unknownEffectSong,producer::runtime::segmentClass),L"");},"Unknown embedded effect class never falls back to a registered Producer factory");
    auto sendConfig=Chunk::parse(effectPath.save_bytes());sendConfig.find("LIST","dbfl")->find("RIFF","DSBC")->find("LIST","fxls")->children[0].find("fxhr")->data[36]=1;
    auto sendSong=song;sendSong.children.push_back(sendConfig);rejected([&]{prepare_script_runtime(contained(sendSong,producer::runtime::segmentClass),L"");},"Embedded Send destination cannot be ignored as a standard effect");
    plainPath.set_tool_graph(ToolGraphDocument().save_bytes());auto graphSong=song;graphSong.children.push_back(Chunk::parse(plainPath.save_bytes()));
    rejected([&]{prepare_script_runtime(contained(graphSong,producer::runtime::segmentClass),L"");},"Embedded AudioPath graph retains explicit unsupported factory boundary");
    for(const auto type:{"DMTG","DMCN","DMSC"}){auto unsupportedSong=song;Chunk nested;nested.id="RIFF";nested.type=type;unsupportedSong.children.push_back(nested);rejected([&]{prepare_script_runtime(contained(unsupportedSong,producer::runtime::segmentClass),L"");},"Other nested graph/container/script remains rejected rather than skipped");}
    auto style=StyleDocument().save_bytes();const auto styleTree=Chunk::parse(style);const auto styleGuid=styleTree.find("guid")->data;
    write_file_atomic((base/L"nested"/L"Owned.sty").wstring(),style);
    auto styleRef=reference(producer::runtime::styleClass,styleGuid,L"Owned.sty");styleRef.find("refh")->data.push_back(0xa7);styleRef.find("refh")->padding=0xcb;styleRef.children.push_back(opaque);song.children.push_back(styleRef);
    write_file_atomic((base/L"nested"/L"Song.sgt").wstring(),song.encode());
    const auto linked=contained(reference(producer::runtime::segmentClass,song.find("guid")->data,L"nested\\Song.sgt"),producer::runtime::segmentClass);
    const auto linkedBefore=linked;const auto relative=prepare_script_runtime(linked,base.wstring());require(relative.dependencies.size()==1&&relative.dependencies[0].sourceBytes==style,"Nested relative path is based on owning Segment folder");
    const auto relativeRoot=Chunk::parse(relative.script);const auto& runtimeSong=relativeRoot.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back();auto expectedSong=song;put32(expectedSong.children.back().find("refh")->data,16,3);require(runtimeSong.encode()==expectedSong.encode(),"Only private DMRF valid flags change; header tail/order/padding/opaque remain exact");
    require(linked==linkedBefore&&read_file((base/L"nested"/L"Song.sgt").wstring())==song.encode(),"Relative resolver preserves caller bytes and owned input files");
    // GUID-only references resolve solely against explicitly owned data. The
    // reference precedes its embedded target, and inactive file text must not
    // cause a search or a dependency on the caller's current directory.
    auto guidRef=reference(producer::runtime::styleClass,styleGuid,L"InactiveMissing.sty");put32(guidRef.find("refh")->data,16,3);
    auto guidScript=Chunk::parse(contained(styleTree,producer::runtime::styleClass));auto& guidEntries=guidScript.find("RIFF","DMCN")->find("LIST","cosl")->children;
    const auto guidEntrySource=Chunk::parse(contained(guidRef,producer::runtime::styleClass));auto guidEntry=guidEntrySource.find("RIFF","DMCN")->find("LIST","cosl")->children[0];guidEntry.find("coba")->data=utf16(L"ByIdentity");guidEntries.insert(guidEntries.begin(),guidEntry);
    const auto guidSource=guidScript.encode();const auto guidOnly=prepare_script_runtime(guidSource,L"");const auto guidPrepared=Chunk::parse(guidOnly.script);
    require(guidOnly.selections.size()==1&&guidOnly.selections[0].requestedGuid&&guidOnly.selections[0].selectedByGuid&&!guidOnly.selections[0].requestedFile&&guidOnly.selections[0].filename.empty(),"Forward top-level GUID-only reference selects owned embedded target without a filename");
    require(guidPrepared.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back().encode()==styleTree.encode()&&guidScript.encode()==guidSource,"GUID-only top alias uses original embedded payload and caller source remains unchanged");
    auto guidSong=song;guidSong.children.back()=guidRef;auto nestedGuidScript=Chunk::parse(contained(guidSong,producer::runtime::segmentClass));
    auto embeddedStyle=guidEntries.back();embeddedStyle.find("coba")->data=utf16(L"OwnedStyle");nestedGuidScript.find("RIFF","DMCN")->find("LIST","cosl")->children.push_back(embeddedStyle);
    const auto nestedGuidBytes=nestedGuidScript.encode();const auto nestedGuid=prepare_script_runtime(nestedGuidBytes,L"");
    require(nestedGuid.dependencies.size()==1&&nestedGuid.dependencies[0].sourceBytes==style&&nestedGuid.selections[0].selectedByGuid&&!nestedGuid.selections[0].requestedFile,"Nested GUID-only edge finds later embedded owner and registers its native source");
    const auto nestedPrepared=Chunk::parse(nestedGuid.script);const auto& preparedGuidSong=nestedPrepared.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back();
    require(preparedGuidSong.encode()==guidSong.encode()&&nestedGuidScript.encode()==nestedGuidBytes,"Already GUID-only nested flags and inactive file/opaque bytes are preserved exactly");
    auto absentGuid=nestedGuidScript;absentGuid.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back().children.back().find("guid")->data[0]^=1;
    rejected([&]{prepare_script_runtime(absentGuid.encode(),L"");},"Unowned GUID-only target rejects without filename or registry search");
    auto classMismatch=nestedGuidScript;classMismatch.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back().children.back().find("guid")->data=song.find("guid")->data;
    rejected([&]{prepare_script_runtime(classMismatch.encode(),L"");},"GUID-only selection cannot bind matching GUID under another native class");
    auto malformedGuid=nestedGuidScript;malformedGuid.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back().children.back().find("guid")->data.pop_back();
    rejected([&]{prepare_script_runtime(malformedGuid.encode(),L"");},"Truncated nested GUID-only identity rejected");
    auto nameOnly=nestedGuidScript;put32(nameOnly.find("RIFF","DMCN")->find("LIST","cosl")->children[0].children.back().children.back().find("refh")->data,16,6);
    rejected([&]{prepare_script_runtime(nameOnly.encode(),L"");},"Active NAME without a name chunk rejects malformed descriptor");
    descriptors(dir);
    // Public Loader selection gives owned class/GUID priority. Missing or
    // sharing-denied fallback files cannot defeat it, but a needed file must
    // still return its original typed IO failure. These are isolated controls.
    auto missingPriority=guidScript;
    auto& missingEntries=missingPriority.find("RIFF","DMCN")->find("LIST","cosl")->children;
    auto& missingRef=*missingEntries.front().find("LIST","DMRF");
    put32(missingRef.find("refh")->data,16,0x13);
    missingRef.find("file")->data=utf16(L"MissingPriority.sty");
    const auto missingSource=missingPriority.encode();
    const auto missingOwned=prepare_script_runtime(missingSource,base.wstring());
    require(missingOwned.selections.size()==1&&missingOwned.selections[0].selectedByGuid&&missingOwned.selections[0].requestedFile&&missingOwned.selections[0].fallbackReadError=="Unable to open input file"&&!missingOwned.selections[0].fallbackPath.empty(),"Forward owned GUID wins over missing explicit filename and retains IO observation");
    const auto missingPrepared=Chunk::parse(missingOwned.script);
    const auto missingObjects=missingPrepared.find("RIFF","DMCN")->find("LIST","cosl");
    require(missingObjects->children.front().children.back().encode()==styleTree.encode()&&read32(missingObjects->children.front().find("cobh")->data,16)==read32(missingEntries.front().find("cobh")->data,16)&&missingPriority.encode()==missingSource,"Missing fallback selection preserves embedded owner, KEEP flags and caller bytes");
    auto reversedPriority=missingPriority;auto& reversedEntries=reversedPriority.find("RIFF","DMCN")->find("LIST","cosl")->children;std::reverse(reversedEntries.begin(),reversedEntries.end());
    const auto reversedOwned=prepare_script_runtime(reversedPriority.encode(),base.wstring());
    require(reversedOwned.selections.size()==1&&reversedOwned.selections[0].selectedByGuid&&reversedOwned.selections[0].selectedId==missingOwned.selections[0].selectedId,"Missing fallback and owned GUID selection are independent of alias order");
    const auto withoutDirectory=prepare_script_runtime(missingSource,L"");
    require(withoutDirectory.selections[0].selectedByGuid&&withoutDirectory.selections[0].fallbackReadError.empty()&&withoutDirectory.selections[0].fallbackPath.empty(),"Owned GUID with relative unused fallback does not need a directory or invent an IO observation");
    auto missingNested=nestedGuidScript;auto& missingSong=missingNested.find("RIFF","DMCN")->find("LIST","cosl")->children.front().children.back();
    missingSong.children.back()=styleRef;missingSong.children.back().find("file")->data=utf16(L"MissingPriority.sty");
    const auto missingNestedSource=missingNested.encode();const auto missingNestedOwned=prepare_script_runtime(missingNestedSource,base.wstring());
    const auto missingNestedPrepared=Chunk::parse(missingNestedOwned.script);auto expectedMissingSong=missingSong;put32(expectedMissingSong.children.back().find("refh")->data,16,3);
    require(missingNestedOwned.dependencies.size()==1&&missingNestedOwned.dependencies[0].sourceBytes==style&&missingNestedOwned.selections[0].selectedByGuid&&!missingNestedOwned.selections[0].fallbackReadError.empty(),"Nested reference uses later owned GUID despite unavailable file");
    require(missingNestedPrepared.find("RIFF","DMCN")->find("LIST","cosl")->children.front().children.back().encode()==expectedMissingSong.encode()&&missingNested.encode()==missingNestedSource,"Nested missing-file binding changes only private flags and preserves header tail, padding and opaque bytes");
    auto neededIo=[&](const Bytes& input,const std::wstring& directory){bool observed=false;try{(void)prepare_script_runtime(input,directory);}catch(const InputFileReadError& e){observed=std::string(e.what())=="Unable to open input file";}require(observed,"Needed filename rethrows the original IO failure rather than a generic binding error");};
    auto noMissingOwner=missingPriority;noMissingOwner.find("RIFF","DMCN")->find("LIST","cosl")->children.pop_back();neededIo(noMissingOwner.encode(),base.wstring());
    rejected([&]{prepare_script_runtime(noMissingOwner.encode(),L"");},"Unowned GUID with needed relative filename still rejects unavailable directory");
    auto fileOnlyPriority=missingPriority;put32(fileOnlyPriority.find("RIFF","DMCN")->find("LIST","cosl")->children.front().find("LIST","DMRF")->find("refh")->data,16,0x12);neededIo(fileOnlyPriority.encode(),base.wstring());
    auto mismatchedPriority=missingPriority;mismatchedPriority.find("RIFF","DMCN")->find("LIST","cosl")->children.front().find("LIST","DMRF")->find("guid")->data[0]^=1;neededIo(mismatchedPriority.encode(),base.wstring());
    write_file_atomic((base/L"InvalidPriority.sty").wstring(),Bytes{1,2,3});
    auto invalidPriority=missingPriority;invalidPriority.find("RIFF","DMCN")->find("LIST","cosl")->children.front().find("LIST","DMRF")->find("file")->data=utf16(L"InvalidPriority.sty");
    rejected([&]{prepare_script_runtime(invalidPriority.encode(),base.wstring());},"Accessible malformed fallback is not swallowed as an IO failure");
    write_file_atomic((base/L"LockedPriority.sty").wstring(),style);
    {
        struct ExclusiveRead {HANDLE handle=INVALID_HANDLE_VALUE;~ExclusiveRead(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}} lock;
        lock.handle=CreateFileW((base/L"LockedPriority.sty").c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(lock.handle!=INVALID_HANDLE_VALUE,"Isolated sharing-denied fallback control opens exclusively");
        auto lockedPriority=missingPriority;auto lockedRef=lockedPriority.find("RIFF","DMCN")->find("LIST","cosl")->children.front().find("LIST","DMRF");lockedRef->find("file")->data=utf16(L"LockedPriority.sty");
        const auto lockedSource=lockedPriority.encode();const auto lockedOwned=prepare_script_runtime(lockedSource,base.wstring());
        require(lockedOwned.selections[0].selectedByGuid&&lockedOwned.selections[0].fallbackReadError=="Unable to open input file"&&lockedPriority.encode()==lockedSource,"Owned GUID wins over sharing-denied fallback without changing input or reader");
        put32(lockedRef->find("refh")->data,16,0x12);neededIo(lockedPriority.encode(),base.wstring());
    }
    require(read_file((base/L"LockedPriority.sty").wstring())==style,"Sharing-denied control releases handle and preserves input bytes");
    auto wrong=styleRef;wrong.find("guid")->data[0]^=1;auto bad=song;bad.children.back()=wrong;const auto fallback=prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());require(fallback.dependencies.size()==1&&fallback.dependencies[0].objectId==relative.dependencies[0].objectId&&fallback.selections.back().requestedGuid&&!fallback.selections.back().selectedByGuid,"Absent owned GUID uses declared filename without changing actual GUID");
    auto otherSong=song;otherSong.find("guid")->data[0]^=0x25;write_file_atomic((base/L"nested"/L"Other.sgt").wstring(),otherSong.encode());auto priority=Chunk::parse(linked);auto& priorityList=priority.find("RIFF","DMCN")->find("LIST","cosl")->children;auto otherEntry=priorityList.front();otherEntry.find("coba")->data=utf16(L"Other");otherEntry.find("LIST","DMRF")->find("file")->data=utf16(L"nested\\Other.sgt");priorityList.push_back(otherEntry);const auto preferred=prepare_script_runtime(priority.encode(),base.wstring());const auto preferredRoot=Chunk::parse(preferred.script);require(preferredRoot.find("RIFF","DMCN")->find("LIST","cosl")->children[1].children.back().find("guid")->data==song.find("guid")->data,"Owned GUID has priority over different declared filename");
    bad=song;bad.children.back().find("file")->data=utf16(L"Missing.sty");rejected([&]{prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());},"Missing nested input rejected without fallback");
    bad=song;bad.children.back().find("refh")->data[0]^=1;rejected([&]{prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());},"Unsupported nested class rejected before activation");
    bad=song;bad.children.back().find("file")->data=utf16(std::wstring(L"Owned.sty\0Other.sty",19));rejected([&]{prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());},"Embedded NUL filename rejected");
    bad=song;bad.children.back().find("guid")->data=Bytes(16,0);put32(bad.children.back().find("refh")->data,16,0x12);auto noGuid=styleTree;noGuid.children.erase(std::remove_if(noGuid.children.begin(),noGuid.children.end(),[](const auto& c){return c.id=="guid";}),noGuid.children.end());write_file_atomic((base/L"nested"/L"NoGuid.sty").wstring(),noGuid.encode());bad.children.back().find("file")->data=utf16(L"NoGuid.sty");rejected([&]{prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());},"Nested identity-less input remains explicit unsupported responsibility");
    auto changed=StyleDocument();changed.load(style);changed.set_tempo(changed.tempo()+1);write_file_atomic((base/L"nested"/L"Conflicting.sty").wstring(),changed.save_bytes());bad=song;bad.children.push_back(reference(producer::runtime::styleClass,styleGuid,L"Conflicting.sty"));rejected([&]{prepare_script_runtime(contained(bad,producer::runtime::segmentClass),(base/L"nested").wstring());},"Same class/GUID with different contents rejected");
    auto cycle=song;cycle.children.back()=reference(producer::runtime::segmentClass,song.find("guid")->data,L"Cycle.sgt");write_file_atomic((base/L"nested"/L"Cycle.sgt").wstring(),cycle.encode());rejected([&]{prepare_script_runtime(contained(cycle,producer::runtime::segmentClass),(base/L"nested").wstring());},"Cyclic dependency graph rejected with retained sources");
    const GUID styleTrack={0xd2ac288d,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};
    require(std::wstring(declared_script_runtime_server(styleTrack))==L"dmstyle.dll","Style track explicitly declared to dmstyle");
    auto muteTrack=styleTrack;muteTrack.Data1=0xd2ac2898;require(std::wstring(declared_script_runtime_server(muteTrack))==L"dmstyle.dll","Mute track module follows recorded read-only Windows class mapping");
    GUID unknown{};rejected([&]{declared_script_runtime_server(unknown);},"Unknown runtime class never uses generic dmime fallback");
}
void runtime(const std::filesystem::path& farm){
    const auto bytes=read_file(farm.wstring());Conductor c;require(c.load_script(bytes,farm.parent_path().wstring(),nullptr).passed(),"Actual supplied Farm Script initializes with nested owned dependencies");auto& session=c.script_session();
    const std::vector<std::wstring> calls={L"dmSfxCougar",L"dmSfxCow",L"dmSfxRooster",L"dmSfxSheep",L"dmSfxWolf",L"dmSfxAlarm",L"dmBGNight",L"dmBGPredawn",L"dmBGDawn",L"dmEnding",L"dmSSBird",L"dmAllStop"};const auto routines=session.routines();
    for(const auto& name:calls){require(std::any_of(routines.begin(),routines.end(),[&](const auto& r){return CompareStringOrdinal(r.c_str(),-1,name.c_str(),-1,TRUE)==CSTR_EQUAL;}),"Farm routine is publicly enumerated");const auto result=session.call(name);std::wcerr<<L"Farm routine="<<name<<L" result=0x"<<std::hex<<static_cast<unsigned long>(result.result)<<L" error=0x"<<static_cast<unsigned long>(result.error.result)<<L" description="<<result.error.description<<L" source="<<result.error.sourceLine<<std::dec<<L"\n";require(result.passed(),"Actual Farm routine completes without partial/failure result");if(name==L"dmBGNight"||name==L"dmBGPredawn"||name==L"dmBGDawn"){LONG flag=-1;const LONG expected=name==L"dmBGNight"?1:name==L"dmBGPredawn"?2:3;require(session.get_number(L"PlayFlag",flag).passed()&&flag==expected,"Farm background routine updates source-defined PlayFlag");}}
    LONG flag=-1;require(session.set_number(L"PlayFlag",2).passed()&&session.get_number(L"PlayFlag",flag).passed()&&flag==2,"Application writes and reads Farm numeric variable");require(session.call(L"dmAllStop").passed(),"Farm background-selected stop callable after variable write");
    require(read_file(farm.wstring())==bytes,"Farm calls leave native source unchanged");c.shutdown();require(!c.initialized(),"Farm private runtime normally releases without main session");
}
}
