void marker_boundary_tests(const std::filesystem::path& dir){
    SegmentDocument d;const auto empty=d.save_bytes();
    require(!d.mark_boundaries(MarkerKind::play,0,1,3072,true)&&d.save_bytes()==empty&&!d.dirty(),"Marker range with no matching boundaries leaves document clean");
    require(!d.mark_boundaries(MarkerKind::enter,1,0,6144,false)&&d.save_bytes()==empty,"Unmark absent track does not materialize a track");
    require(d.add_marker({3072,MarkerKind::play})&&d.add_marker({3072,MarkerKind::play})&&d.add_marker({769,MarkerKind::play})&&d.add_marker({3072,MarkerKind::enter}),"Boundary fixture retains overlapping kinds and off-boundary event");
    const auto before=d.save_bytes();
    require(d.mark_boundaries(MarkerKind::play,0,0,6144,true)&&d.markers().size()==5,"Mark measures fills missing zero boundary without duplicating existing overlaps");const auto marked=d.save_bytes();
    require(!d.mark_boundaries(MarkerKind::play,0,0,6144,true)&&d.save_bytes()==marked,"Repeated Mark measures is read-only");
    require(d.undo()&&d.save_bytes()==before&&d.redo()&&d.save_bytes()==marked,"Bulk Mark is one whole-document Undo Redo transaction");
    require(d.mark_boundaries(MarkerKind::play,0,0,6144,false)&&d.markers().size()==2&&d.markers()[0].time==769&&d.markers()[1].kind==MarkerKind::enter,"Unmark removes all matching same-kind overlaps; off-boundary and Enter retained");const auto unmarked=d.save_bytes();
    require(d.undo()&&d.save_bytes()==marked&&!d.mark_boundaries(MarkerKind::play,0,0,6144,true)&&d.redo()&&d.save_bytes()==unmarked,"No-op bulk operation preserves Redo");
    require(d.undo(),"Prepare invalid-input Redo checkpoint");const auto invalidBefore=d.save_bytes();
    require(!d.mark_boundaries(MarkerKind::play,3,0,6144,true)&&!d.mark_boundaries(static_cast<MarkerKind>(7),0,0,6144,true)&&!d.mark_boundaries(MarkerKind::play,0,-1,6144,true)&&!d.mark_boundaries(MarkerKind::play,0,6144,6144,true)&&!d.mark_boundaries(MarkerKind::play,0,0,d.length()+1,true),"Invalid Marker bulk kind division and clock ranges refused");
    require(d.save_bytes()==invalidBefore&&d.redo()&&d.save_bytes()==unmarked,"Invalid bulk operations preserve bytes and Redo");
    SegmentDocument meterDoc;require(meterDoc.set_meter(1,3,8,3),"Mixed-meter boundary fixture");
    require(meterDoc.mark_boundaries(MarkerKind::enter,2,3000,4300,true),"Grid boundaries follow selected meter transition");const auto grid=meterDoc.markers();
    require(grid.size()==10&&grid.front().time==3072&&grid[1].time==3200&&grid.back().time==4224,"Mixed-meter grids are exact inside half-open range");
    const auto gridBefore=meterDoc.save_bytes();meterDoc.select_track_group(2);require(meterDoc.mark_boundaries(MarkerKind::play,1,0,1536,true)&&meterDoc.markers().size()==2,"Bulk boundary ownership selected group");meterDoc.select_track_group(1);require(meterDoc.markers().size()==10,"Bulk boundary leaves peer group unchanged");
    SegmentDocument fractional;require(fractional.set_meter(0,4,4,5),"Fractional grid fixture");const auto fraction=fractional.save_bytes();rejected([&]{fractional.mark_boundaries(MarkerKind::play,2,0,1000,true);},"Fractional-clock grids refused without invented rounding");require(fractional.save_bytes()==fraction&&!fractional.markers().size(),"Fractional grid rejection leaves bytes and no Marker track");
    auto extended=marker_track();add_marker_event(extended,{3072,MarkerKind::play});auto mark=extended.find("LIST","MARK");put32(mark->find("play")->data,0,8);mark->find("play")->data.insert(mark->find("play")->data.end(),{0xa1,0xb2,0xc3,0xd4});Chunk opaque;opaque.id="zzzz";opaque.data={9,8,7};opaque.padding=0xad;mark->children.push_back(opaque);extended.children.push_back(*mark);const auto extendedBefore=extended.encode();
    require(!mark_marker_boundaries(extended,MarkerKind::play,{3072},true)&&extended.encode()==extendedBefore,"Existing extended boundary Mark is a no-op");
    require(mark_marker_boundaries(extended,MarkerKind::play,{0,3072},true),"Bulk Mark preserves extended records and mirrored containers");auto expected=Chunk::parse(extendedBefore);for(auto& c:expected.children)if(c.id=="LIST"&&c.type=="MARK")c.find("play")->data.insert(c.find("play")->data.begin()+4,8,0);require(extended.encode()==expected.encode(),"Only new zero-initialized extended record inserted; all other bytes retained");
    auto inconsistent=extended;put32(inconsistent.children.back().find("play")->data,4,1);const auto inconsistentBefore=inconsistent.encode();rejected([&]{mark_marker_boundaries(inconsistent,MarkerKind::enter,{0},true);},"Bulk rejects disagreeing duplicate MARK containers");require(inconsistent.encode()==inconsistentBefore,"Malformed duplicate bulk operation is atomic");
    const auto base=dir/L"MarkerBoundaries";std::filesystem::create_directories(base);write_file_atomic((base/L"SourceMarkers.sgp").wstring(),gridBefore);Framework host;host.open_segment((base/L"SourceMarkers.sgp").wstring());host.save_project((base/L"MarkerBoundaries.pro").wstring());Framework reload;reload.open_project((base/L"MarkerBoundaries.pro").wstring());require(reload.document(0).save_bytes()==gridBefore&&reload.document(0).markers().size()==10,"Bulk Marker native Project separate Framework restores mixed-meter boundary records");
}
void marker_document_tests(const std::filesystem::path& dir,const std::wstring& original=L""){
    marker_boundary_tests(dir);
    SegmentDocument doc;const auto empty=doc.save_bytes();size_t selected=999;
    require(doc.add_marker({3073,MarkerKind::play},0,&selected)&&selected==0,"Marker precise grid/tick time insert");const auto one=doc.save_bytes();
    require(doc.add_marker({3073,MarkerKind::enter},0,&selected)&&selected==1,"Marker and Enter may overlap");
    require(doc.add_marker({3073,MarkerKind::play},0,&selected)&&selected==1&&doc.markers().size()==3,"same-type overlapping Markers retain independent selection");
    const auto three=doc.save_bytes();const auto clip=doc.copy_marker(selected);
    require(doc.save_bytes()==three&&doc.paste_marker(clip,6145,0,&selected)&&selected==3,"Marker copy is read-only and paste keeps fine time");const auto pasted=doc.save_bytes();
    require(doc.undo()&&doc.save_bytes()==three&&doc.redo()&&doc.save_bytes()==pasted,"Marker paste whole-document Undo Redo");
    require(doc.edit_marker(3,{769,MarkerKind::enter},0,&selected)&&selected==0,"Marker type and time edit returns sorted selection");const auto edited=doc.save_bytes();
    require(doc.delete_marker(0)&&doc.markers().size()==3&&doc.undo()&&doc.save_bytes()==edited,"Marker deletion and history exact");doc.undo();const auto before=doc.save_bytes();selected=987;
    require(!doc.add_marker({-1,MarkerKind::play},0,&selected)&&!doc.add_marker({doc.length(),MarkerKind::enter})&&!doc.add_marker({0,static_cast<MarkerKind>(9)}),"invalid Marker positions and kinds rejected");
    require(!doc.edit_marker(999,{0,MarkerKind::play},0,&selected)&&!doc.delete_marker(999)&&!doc.paste_marker(clip,doc.length()),"invalid selection and paste destination rejected");
    rejected([&]{doc.paste_marker(Bytes{1,2,3},0);},"malformed Marker clipboard rejected");
    require(doc.save_bytes()==before&&selected==987&&doc.redo()&&doc.save_bytes()==edited,"failed Marker edits preserve document output index and redo");
    SegmentDocument restored;restored.load(edited);require(restored.save_bytes()==edited&&restored.markers().size()==4&&!restored.dirty(),"Marker saved bytes reload exact");
    doc.select_track_group(2);require(doc.markers().empty()&&doc.add_marker({100,MarkerKind::enter}),"Marker ownership in independently selected group");doc.select_track_group(1);require(doc.markers().size()==4,"other group Marker records retained");
    auto root=Chunk::parse(doc.save_bytes());auto track=marker_track();add_marker_event(track,{256,MarkerKind::play});root.find("LIST","trkl")->children.push_back(track);restored.load(root.encode());
    require(restored.markers(1).size()==1&&restored.edit_marker(0,{257,MarkerKind::play},1)&&restored.markers(0).size()==4&&restored.markers(1)[0].time==257,"nth Marker track edit leaves peer track unchanged");
    const auto base=dir/L"marker-document";std::filesystem::create_directories(base);write_file_atomic((base/L"Created.sgp").wstring(),restored.save_bytes());Framework host;host.open_segment((base/L"Created.sgp").wstring());host.save_project((base/L"marker-document.pro").wstring());Framework reload;reload.open_project((base/L"marker-document.pro").wstring());require(reload.document(0).save_bytes()==restored.save_bytes()&&!reload.dirty(),"Marker native Project independent Framework reload");
    auto extended=marker_track();add_marker_event(extended,{12,MarkerKind::play});auto mark=extended.find("LIST","MARK");auto& data=mark->find("play")->data;put32(data,0,8);data.insert(data.end(),{0xa1,0xb2,0xc3,0xd4});Chunk opaque;opaque.id="zzzz";opaque.data={9,8,7};opaque.padding=0xad;mark->children.push_back(opaque);extended.children.push_back(*mark);const auto originalExtended=extended.encode();
    require(marker_events(extended).size()==1,"identical duplicate MARK containers represent one event");
    require(change_marker_event(extended,0,{13,MarkerKind::play}),"extended duplicate Marker edit");auto expected=Chunk::parse(originalExtended);for(auto& c:expected.children)if(c.id=="LIST"&&c.type=="MARK")put32(c.find("play")->data,4,13);
    require(extended.encode()==expected.encode(),"Marker edit mirrors duplicates retains unknown padding and record extensions");
    const auto extendedClip=copy_marker_event(extended,0);auto destination=marker_track();require(paste_marker_event(destination,extendedClip,14)&&destination.find("LIST","MARK")->find("play")->data==Bytes({8,0,0,0,14,0,0,0,0xa1,0xb2,0xc3,0xd4}),"clipboard preserves complete extended Marker record");
    auto shortTarget=marker_track();add_marker_event(shortTarget,{1,MarkerKind::play});const auto shortBytes=shortTarget.encode();require(!paste_marker_event(shortTarget,extendedClip,14)&&shortTarget.encode()==shortBytes,"paste refuses extension truncation atomically");
    const auto beforeType=extended.encode();require(!change_marker_event(extended,0,{13,MarkerKind::enter})&&extended.encode()==beforeType,"kind change refuses unrepresentable extension");
    auto inconsistent=extended;put32(inconsistent.children.back().find("play")->data,4,99);const auto inconsistentBytes=inconsistent.encode();rejected([&]{change_marker_event(inconsistent,0,{14,MarkerKind::play});},"inconsistent duplicate MARK containers refused");require(inconsistent.encode()==inconsistentBytes,"duplicate disagreement preserves input bytes");
    auto malformed=marker_track();add_marker_event(malformed,{1,MarkerKind::play});put32(malformed.find("LIST","MARK")->find("play")->data,0,3);rejected([&]{marker_events(malformed);},"short Marker stride refused");
    if(!original.empty()){
        const auto input=std::filesystem::path(original);
        for(const auto& name:{L"OriginalEmptyMarker.sgp",L"OriginalPlayMarker.sgp",L"OriginalMarkerAndEnter.sgp",L"OriginalUndoEnter.sgp"}){
            const auto bytes=read_file((input/name).wstring());SegmentDocument observed;observed.load(bytes);const auto events=observed.markers();const auto both=std::wstring(name)==L"OriginalMarkerAndEnter.sgp",none=std::wstring(name)==L"OriginalEmptyMarker.sgp";
            require(events.size()==(none?0:both?2:1)&&observed.save_bytes()==bytes,"original Marker no-edit reload exact");
            if(!none)require(events[0].time==3072&&events[0].kind==MarkerKind::play&&(!both||(events[1].time==6144&&events[1].kind==MarkerKind::enter)),"original Marker and Enter positions match dynamic observation");
            require(observed.add_marker({9217,MarkerKind::enter}),"edit original empty single or duplicate Marker container");const auto changed=observed.save_bytes();require(observed.undo()&&observed.save_bytes()==bytes&&observed.redo()&&observed.save_bytes()==changed,"original Marker edit whole-byte history");
            observed.save((base/name).wstring());SegmentDocument reopened;reopened.load(read_file((base/name).wstring()));require(reopened.markers().size()==events.size()+1&&reopened.save_bytes()==changed,"original Marker edited save reload");
        }
    }
    require(one!=empty,"Marker track materializes in owned Segment");
}
