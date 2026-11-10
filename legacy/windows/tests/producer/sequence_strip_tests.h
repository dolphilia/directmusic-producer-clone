void segment_extended_boundary_tests(){
    auto d=SegmentDocument::create_empty({false,4,8000},L"Extended boundary ownership");d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence});d.add_note({192,768,0,72,96});d.set_info_text(SegmentInfoField::subject,L"Retain Ω");
    auto root=Chunk::parse(d.save_bytes());Chunk opaque;opaque.id="xb??";opaque.data={9,7,5};opaque.padding=0xb9;root.children.push_back(opaque);
    const SegmentPerformanceBoundary choices[]={SegmentPerformanceBoundary::immediate,SegmentPerformanceBoundary::grid,SegmentPerformanceBoundary::beat,SegmentPerformanceBoundary::barline,SegmentPerformanceBoundary::align_beat,SegmentPerformanceBoundary::align_barline,SegmentPerformanceBoundary::align_segment,SegmentPerformanceBoundary::segment_end,SegmentPerformanceBoundary::queue_end};
    // Formal handler's nine values, with retained Primary-only invalidation,
    // AfterPrepare/unknown bits and the Marker clear at the first Align choice.
    const std::uint32_t trail[]={0xb5000440,0xb5000c40,0xb5001440,0xb5002440,0xb4011440,0xb4012440,0xb4810440,0xb4800440,0xb4000540};
    const std::uint32_t aligned[]={0x11000,0x12000,0x810000};
    for(unsigned variant=0;variant<4;++variant){auto source=root;auto& h=source.find("segh")->data;h.resize(variant==0?24:variant==1?40:65,0xa3);source.find("segh")->padding=0xd7;if(variant==3)put32(h,32,3);put32(h,20,0xb5001440);const auto variantHeader=h;
        for(unsigned i=0;i<9;++i){const auto input=source.encode();d.load(input);d.select_range({192,3840,TimelineSequence});d.select_all();auto expected=source;put32(expected.find("segh")->data,20,trail[i]);
            require(d.set_performance_boundary(choices[i])&&d.save_bytes()==expected.encode(),"Nine original boundary choices retain whole native image except declared boundary and coupled Marker fields");
            require(d.performance_boundary()==choices[i]&&d.alignment().has_value()==(i>=4&&i<=6)&&d.invalidation()==SegmentInvalidation::cut_primary,"Extended boundary getter keeps Invalidation independent and enables only the three aligned choices");
            require(!d.set_performance_boundary(choices[i])&&d.can_undo(),"Repeated supported boundary choice creates no extra history");
            require(d.selected_range().begin==192&&d.selected_range().end==3840&&d.selected_range().strips==TimelineSequence&&d.tempos()[0].selected&&d.name()==L"Extended boundary ownership","Extended boundary retains selected events range groups identity and Unicode metadata");
            require(d.undo()&&d.save_bytes()==input&&!d.dirty()&&d.redo()&&d.save_bytes()==expected.encode(),"Coupled Marker clear and boundary selection undo as one complete transaction");
            SegmentDocument restored;restored.load(expected.encode());require(restored.performance_boundary()==choices[i]&&restored.save_bytes()==expected.encode()&&!restored.dirty()&&!restored.can_undo(),"Separate clean restoration retains each of the nine native boundaries and opaque padding");source=expected;
        }
        for(unsigned i=0;i<3;++i){auto entered=root;entered.find("segh")->data=variantHeader;put32(entered.find("segh")->data,20,0xb5000440|aligned[i]);const auto marked=entered.encode();d.load(marked);auto cleared=entered;put32(cleared.find("segh")->data,20,0xb4000440|aligned[i]);
            require(d.alignment()==SegmentAlignment::time&&d.set_performance_boundary(choices[i+4])&&d.save_bytes()==cleared.encode(),"Explicit same Align choice clears imported dormant Marker atomically and retains zero valid-start Time representation");
            require(d.undo()&&d.save_bytes()==marked&&d.redo()&&d.save_bytes()==cleared.encode(),"Undo restores even imported Marker plus Align bytes without normalizing during load");d.load(cleared.encode());
            require(!d.set_alignment(SegmentAlignment::time)&&d.save_bytes()==cleared.encode()&&!d.dirty()&&!d.can_undo(),"Legacy zero valid-start displays Time and repeated Time preserves exact bytes as a no-op");
            const SegmentAlignment alignChoices[]={SegmentAlignment::grid,SegmentAlignment::beat,SegmentAlignment::barline,SegmentAlignment::time};const std::uint32_t alignBits[]={0x40000,0x20000,0x8000000,0x80000};
            for(unsigned j=0;j<4;++j){auto previous=d.save_bytes();auto expected=cleared;put32(expected.find("segh")->data,20,0xb4000440|aligned[i]|alignBits[j]);
                require(d.set_alignment(alignChoices[j])&&d.save_bytes()==expected.encode()&&d.alignment()==alignChoices[j],"Four original valid-start choices retain exact boundary timing invalidation and unknown source bytes");
                require(!d.set_alignment(alignChoices[j])&&d.undo()&&d.save_bytes()==previous&&d.redo()&&d.save_bytes()==expected.encode(),"Alignment repeated value is a no-op and one UndoRedo restores exact prior native bytes");
                SegmentDocument restored;restored.load(d.save_bytes());require(restored.alignment()==alignChoices[j]&&restored.save_bytes()==expected.encode()&&!restored.can_undo(),"Each explicit alignment bit survives independent clean load");
            }
            auto current=d.save_bytes();auto expected=cleared;put32(expected.find("segh")->data,20,0xb4001440);require(d.set_performance_boundary(SegmentPerformanceBoundary::beat)&&d.save_bytes()==expected.encode()&&!d.alignment()&&d.undo()&&d.save_bytes()==current,"Leaving Align clears its valid-start field in the same undoable transaction");
            d.load(cleared.encode());rejected([&]{d.set_boundary_marker(true);},"Marker enable is rejected for every aligned boundary");require(d.save_bytes()==cleared.encode()&&!d.dirty()&&!d.can_undo(),"Refused Marker enable leaves aligned source and history untouched");
            auto conflicting=cleared;put32(conflicting.find("segh")->data,20,0xb40c0440|aligned[i]);d.load(conflicting.encode());require(!d.alignment()&&d.performance_boundary()==choices[i+4],"Multiple valid-start bits retain the supported boundary but have no invented alignment selection");
            require(d.set_boundary_after_prepare(false)&&d.undo()&&d.can_redo(),"Prepare a pending timing redo on conflicting alignment input");for(auto value:alignChoices)rejected([&]{d.set_alignment(value);},"Conflicting imported alignment refuses every radio choice atomically");
            rejected([&]{d.set_alignment(static_cast<SegmentAlignment>(99));},"Invalid alignment enum cannot write arbitrary flags");require(d.save_bytes()==conflicting.encode()&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"Alignment refusals preserve unknown bytes clean state and pending redo");
        }
        auto queue=root;queue.find("segh")->data=variantHeader;put32(queue.find("segh")->data,20,0xb5001440);d.load(queue.encode());auto expected=queue;put32(expected.find("segh")->data,20,0xb4000540);
        require(d.set_performance_boundary(SegmentPerformanceBoundary::queue_end)&&d.save_bytes()==expected.encode()&&d.undo()&&d.save_bytes()==queue.encode(),"Queue selection and forced Marker clear are a single reversible edit");d.load(expected.encode());rejected([&]{d.set_boundary_marker(true);},"Queue refuses Marker enable");
        rejected([&]{d.set_alignment(SegmentAlignment::grid);},"Nonaligned Queue refuses alignment editing");require(d.save_bytes()==expected.encode()&&!d.dirty()&&!d.can_undo(),"Queue field refusals retain exact bytes and clean history");
    }
}
void segment_invalidation_settings_tests(){
    auto d=SegmentDocument::create_empty({false,4,8000},L"Invalidation ownership");d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence});d.add_note({192,768,0,72,96});d.set_info_text(SegmentInfoField::subject,L"Retain Ω");
    auto root=Chunk::parse(d.save_bytes());Chunk opaque;opaque.id="iv??";opaque.data={3,5,7};opaque.padding=0xbd;root.children.push_back(opaque);
    const std::uint32_t retained=0xa5001440;const SegmentInvalidation choices[]={SegmentInvalidation::dont_cut_off,SegmentInvalidation::cut_primary,SegmentInvalidation::cut_all};const std::uint32_t bits[]={0x8000,0x10000000,0};
    for(unsigned variant=0;variant<4;++variant){auto source=root;auto& h=source.find("segh")->data;h.resize(variant==0?24:variant==1?40:65,0xa9);source.find("segh")->padding=0xc7;if(variant==3)put32(h,32,3);
        for(unsigned from=0;from<3;++from)for(unsigned to=0;to<3;++to){put32(h,20,retained|bits[from]);const auto input=source.encode();d.load(input);d.select_range({192,3840,TimelineSequence});d.select_all();auto expected=source;put32(expected.find("segh")->data,20,retained|bits[to]);
            require(d.invalidation()==choices[from]&&d.default_resolution()==(retained|bits[from]),"All original invalidation choices decode independently of header length and clock flags");
            require(d.set_invalidation(choices[to])==(from!=to)&&d.save_bytes()==expected.encode()&&d.selected_range().begin==192&&d.selected_range().end==3840&&d.selected_range().strips==TimelineSequence&&d.tempos()[0].selected&&d.name()==L"Invalidation ownership","Invalidation transaction changes only its exact two native bits and retains tracks identity Info selection opaque bytes and padding");
            require(!d.set_invalidation(choices[to])&&d.can_undo()==(from!=to),"Repeated invalidation radio choice has no extra dirty or history change");
            if(from!=to)require(d.undo()&&d.save_bytes()==input&&!d.dirty()&&d.redo()&&d.save_bytes()==expected.encode(),"Invalidation UndoRedo restores whole native images as one transaction");
            SegmentDocument restored;restored.load(d.save_bytes());require(restored.invalidation()==choices[to]&&restored.save_bytes()==expected.encode()&&!restored.dirty()&&!restored.can_undo(),"Independent clean load restores invalidation and all unknown native bytes");
        }
        put32(h,20,retained);d.load(source.encode());require(d.set_invalidation(choices[0])&&d.undo()&&d.can_redo(),"Prepare a pending invalidation redo before rejecting an invalid enum");const auto clean=d.save_bytes();
        rejected([&]{d.set_invalidation(static_cast<SegmentInvalidation>(99));},"Invalid invalidation enum cannot introduce arbitrary native bits");
        require(d.save_bytes()==clean&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"Rejected invalidation enum retains the clean checkpoint and pending redo");
        put32(h,20,retained|0x10008000);const auto conflict=source.encode();d.load(conflict);require(!d.invalidation(),"Conflicting imported invalidation flags have no invented radio selection");
        for(auto choice:choices)rejected([&]{d.set_invalidation(choice);},"Every invalidation choice refuses both-bit imported conflict without normalizing it");
        require(d.save_bytes()==conflict&&!d.dirty()&&!d.can_undo(),"All refused conflict edits retain complete bytes and history");
        auto markerOff=source;put32(markerOff.find("segh")->data,20,(retained|0x10008000)&~0x1000000u);
        require(d.set_boundary_marker(false)&&d.save_bytes()==markerOff.encode()&&!d.invalidation()&&d.undo()&&d.save_bytes()==conflict,"Independent marker field and Undo retain an imported invalidation conflict");
    }
    for(const auto advanced:{0x100u,0x4000u,0x11000u,0x12000u,0x810000u,0x800000u}){auto source=root;put32(source.find("segh")->data,20,0xa4000040|advanced);d.load(source.encode());auto expected=source;put32(expected.find("segh")->data,20,0xb4000040|advanced);
        require(d.invalidation()==SegmentInvalidation::cut_all&&d.set_invalidation(SegmentInvalidation::cut_primary)&&d.save_bytes()==expected.encode(),"Invalidation remains independent of imported Queue Default Align and SegmentEnd boundary bits");
    }
}
void segment_boundary_settings_tests(){
    auto d=SegmentDocument::create_empty({false,4,8000},L"Boundary ownership");d.add_editor_tracks({SegmentTrackKind::sequence});d.add_note({192,768,0,72,96});d.set_info_text(SegmentInfoField::subject,L"Retain Ω");
    auto root=Chunk::parse(d.save_bytes());Chunk opaque;opaque.id="b???";opaque.data={0x32,0x54,0x76};opaque.padding=0xad;root.children.push_back(opaque);
    const std::uint32_t retained=0xa4008040;const SegmentPerformanceBoundary values[]={SegmentPerformanceBoundary::immediate,SegmentPerformanceBoundary::grid,SegmentPerformanceBoundary::beat,SegmentPerformanceBoundary::barline};const std::uint32_t bits[]={0,0x800,0x1000,0x2000};
    for(unsigned variant=0;variant<4;++variant){auto source=root;auto& h=source.find("segh")->data;h.resize(variant==0?24:variant==1?40:65,0xa5);source.find("segh")->padding=0xb7;put32(h,20,retained|0x800);if(variant==3)put32(h,32,3);const auto input=source.encode();
        for(unsigned i=0;i<4;++i){d.load(input);d.select_range({192,3840,TimelineSequence});auto expected=source;put32(expected.find("segh")->data,20,retained|bits[i]);
            require(d.default_resolution()==(retained|0x800)&&d.performance_boundary()==SegmentPerformanceBoundary::grid,"Native resolution is read independently of header length and clock length/loop flags");
            require(d.set_performance_boundary(values[i])==(i!=1)&&d.save_bytes()==expected.encode(),"Basic boundary changes only its three resolution bits, retaining unknown header tails, odd padding and every other chunk");
            require(d.selected_range().begin==192&&d.selected_range().end==3840&&d.selected_range().strips==TimelineSequence&&d.name()==L"Boundary ownership"&&d.info_text(SegmentInfoField::subject)==L"Retain Ω","Boundary editing retains selection component name Unicode Info and note ownership");
            require(!d.set_performance_boundary(values[i])&&d.can_undo()==(i!=1),"Same boundary is a no-op with no extra history");
            if(i!=1)require(d.undo()&&d.save_bytes()==input&&!d.dirty()&&d.redo()&&d.save_bytes()==expected.encode(),"Boundary UndoRedo is one complete native transaction across legacy and extended headers");
            SegmentDocument restored;restored.load(d.save_bytes());require(restored.performance_boundary()==values[i]&&restored.save_bytes()==expected.encode()&&!restored.dirty()&&!restored.can_undo(),"Separate model restoration preserves boundary and opaque native bytes exactly");
        }
        d.load(input);auto marker=source;put32(marker.find("segh")->data,20,retained|0x800|0x1000000);
        require(d.set_boundary_marker(true)&&d.save_bytes()==marker.encode()&&!d.set_boundary_marker(true),"Marker toggle changes only DMUS_SEGF_MARKER and repeats are a no-op");
        auto prepare=marker;put32(prepare.find("segh")->data,20,retained|0x800|0x1000000|0x400);
        require(d.set_boundary_after_prepare(true)&&d.save_bytes()==prepare.encode()&&!d.set_boundary_after_prepare(true),"After Prepare Time independently changes only its own native flag");
        require(d.undo()&&d.save_bytes()==marker.encode()&&d.undo()&&d.save_bytes()==input&&d.redo()&&d.save_bytes()==marker.encode()&&d.redo()&&d.save_bytes()==prepare.encode(),"Independent marker and timing commits retain two UndoRedo entries");
        require(d.set_boundary_marker(false)&&d.set_boundary_after_prepare(false)&&d.save_bytes()==input,"Clearing marker and prepare flags restores the complete original native image");
    }
    for(const auto unsupported:{0x1800u,0x3000u,0x4000u,0x10000u}){auto source=root;put32(source.find("segh")->data,20,retained|unsupported);const auto input=source.encode();d.load(input);
        require(!d.performance_boundary(),"Conflicting boundaries imported DEFAULT and bare ALIGN have no invented boundary selection");
        require(d.set_boundary_after_prepare(true)&&d.undo()&&d.can_redo(),"Prepare pending timing redo on an imported unsupported boundary");
        rejected([&]{d.set_performance_boundary(SegmentPerformanceBoundary::beat);},"Boundary field refuses conflicting or DEFAULT import without silently normalizing it");
        rejected([&]{d.set_performance_boundary(static_cast<SegmentPerformanceBoundary>(99));},"Invalid boundary enum cannot write arbitrary flags");
        require(d.save_bytes()==input&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"Rejected boundary edits retain bytes clean checkpoint and pending redo");
        auto expected=source;put32(expected.find("segh")->data,20,retained|unsupported|0x400|((unsupported&0x10000)?0:0x1000000));
        if(unsupported&0x10000)rejected([&]{d.set_boundary_marker(true);},"Bare imported Align retains the original Marker disable gate");else require(d.set_boundary_marker(true),"Unsupported nonaligned boundary retains independent Marker edit");
        require(d.set_boundary_after_prepare(true)&&d.save_bytes()==expected.encode(),"Timing remains independently editable while all unsupported native boundary bits are retained");
    }
}
void segment_info_tests(){
    auto d=SegmentDocument::create_empty({false,4,8000},L"Identity stays separate");d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence});d.add_note({192,768,0,72,96});
    auto root=Chunk::parse(d.save_bytes());auto info=root.find("LIST","UNFO");
    const auto field=[](const char* id,const std::wstring& value){Chunk c;c.id=id;c.data=utf16(value);return c;};
    Chunk opaque;opaque.id="i???";opaque.data={0x12,0x45,0x78};opaque.padding=0xa7;
    info->children.push_back(opaque);for(const auto id:{"USBJ","UART","UCOP","UCMT"})info->children.push_back(field(id,L"Initial"));
    root.children.push_back(opaque);auto version=root.find("vers");if(!version){Chunk c;c.id="vers";root.children.push_back(c);version=&root.children.back();}
    version->data=Bytes(11,0xa3);version->padding=0xbd;put32(version->data,0,0x00010002);put32(version->data,4,0x00030004);
    const auto input=root.encode();d.load(input);
    const SegmentInfoField kinds[]={SegmentInfoField::subject,SegmentInfoField::author,SegmentInfoField::copyright,SegmentInfoField::information};
    const char* ids[]={"USBJ","UART","UCOP","UCMT"};
    for(unsigned i=0;i<4;++i){
        d.select_range({192,3840,TimelineSequence});
        const auto value=i==3?L" First line\r\n第二行 Ω\r\n":L" Unicode Ω 検証 ";auto expected=root;expected.find("LIST","UNFO")->find(ids[i])->data=utf16(value);
        require(d.info_text(kinds[i])==L"Initial"&&!d.set_info_text(kinds[i],L"Initial")&&!d.dirty()&&!d.can_undo(),"Reading/same-value Info does not create history or dirty the document");
        require(d.set_info_text(kinds[i],value)&&d.info_text(kinds[i])==value&&d.save_bytes()==expected.encode(),"Each Unicode Info field changes only its own chunk including exact whitespace and multiline CRLF");
        require(d.name()==L"Identity stays separate"&&d.selected_range().begin==192&&d.selected_range().end==3840&&d.selected_range().strips==TimelineSequence,"Info edit preserves component name and selected timeline range");
        require(d.undo()&&d.save_bytes()==input&&!d.dirty()&&d.redo()&&d.save_bytes()==expected.encode(),"Info is one complete atomic UndoRedo transaction preserving all opaque padding and version tail");
        SegmentDocument restored;restored.load(d.save_bytes());require(restored.info_text(kinds[i])==value&&restored.save_bytes()==expected.encode()&&!restored.dirty()&&!restored.can_undo(),"Independent document restoration preserves authored Unicode metadata exactly");
        d.load(input);
    }
    const std::array<std::uint16_t,4> beforeVersion{1,2,3,4},afterVersion{9999,0,65535,17};auto expected=root;put32(expected.find("vers")->data,0,0x270f0000);put32(expected.find("vers")->data,4,0xffff0011);
    require(d.info_version()==beforeVersion&&!d.set_info_version(beforeVersion)&&!d.can_undo(),"Version exposes high/low WORDs in each native DWORD and unchanged read is a no-op");
    require(d.set_info_version(afterVersion)&&d.info_version()==afterVersion&&d.save_bytes()==expected.encode(),"Version changes only first8 bytes of extended native vers while retaining all WORD values");
    require(d.undo()&&d.save_bytes()==input&&d.redo()&&d.save_bytes()==expected.encode(),"Version UndoRedo retains every extension and odd padding byte");
    require(!d.set_info_text(SegmentInfoField::subject,L"Initial")&&d.undo()&&d.save_bytes()==input,"Unchanged unrelated Info field does not intercept pending version Undo");
    require(d.set_info_text(SegmentInfoField::author,L"")&&d.info_text(SegmentInfoField::author).empty(),"Clearing existing Author retains an explicit empty UTF16 field");auto empty=root;empty.find("LIST","UNFO")->find("UART")->data=utf16(L"");require(d.save_bytes()==empty.encode(),"Clearing does not remove unrelated metadata or normalize container order");
    auto missing=root;missing.children.erase(std::remove_if(missing.children.begin(),missing.children.end(),[](const Chunk& c){return c.id=="vers"||(c.id=="LIST"&&c.type=="UNFO");}),missing.children.end());const auto without=missing.encode();d.load(without);
    require(d.info_text(SegmentInfoField::information).empty()&&d.info_version()==std::array<std::uint16_t,4>{}&&!d.set_info_text(SegmentInfoField::information,L"")&&!d.set_info_version({})&&d.save_bytes()==without&&!d.can_undo(),"Missing Info/version defaults remain absent on reads and empty/zero no-ops");
    require(d.set_info_text(SegmentInfoField::subject,L"Added subject")&&d.info_text(SegmentInfoField::subject)==L"Added subject"&&d.name().empty(),"Missing UNFO is created only for an actual Subject change without inventing UNAM");
    require(d.undo()&&d.save_bytes()==without&&d.set_info_version({1,0,0,0})&&d.info_version()==std::array<std::uint16_t,4>{1,0,0,0},"Missing version is created on an actual change with no unrelated Info data");
    d.load(input);require(d.set_info_version(afterVersion)&&d.undo()&&d.can_redo(),"Prepare pending metadata redo for atomic-refusal tests");
    for(const auto& invalid:std::vector<std::wstring>{std::wstring(L"a\0b",3),std::wstring(32768,L'x')}){rejected([&]{d.set_info_text(SegmentInfoField::author,invalid);},"Embedded NUL and excessive text refuse before serialization");require(d.save_bytes()==input&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"Invalid Info text preserves bytes checkpoint and pending redo");}
    rejected([&]{d.set_info_text(static_cast<SegmentInfoField>(99),L"Unknown");},"Unknown field identifier refuses instead of modifying arbitrary chunks");require(d.save_bytes()==input&&d.can_redo(),"Invalid field identity preserves history");
    for(unsigned variant=0;variant<5;++variant){auto bad=root;
        if(variant==0)bad.children.push_back(*bad.find("LIST","UNFO"));
        else if(variant==1)bad.find("LIST","UNFO")->children.push_back(*bad.find("LIST","UNFO")->find("UART"));
        else if(variant==2)bad.find("LIST","UNFO")->find("UART")->data={0x41};
        else if(variant==3)bad.find("LIST","UNFO")->find("UART")->data={0x41,0};
        else bad.find("LIST","UNFO")->find("UART")->data=utf16(std::wstring(L"a\0b",3));
        const auto bytes=bad.encode();d.load(bytes);rejected([&]{d.info_text(SegmentInfoField::author);},"Ambiguous/truncated/nonterminated Info text is unavailable");rejected([&]{d.set_info_text(SegmentInfoField::author,L"Other");},"Malformed targeted Info metadata cannot be adopted by an edit");require(d.save_bytes()==bytes&&!d.dirty()&&!d.can_undo(),"Malformed Info refusal retains original bytes with no history");
    }
    for(unsigned variant=0;variant<2;++variant){auto bad=root;if(variant)bad.children.push_back(*bad.find("vers"));else bad.find("vers")->data.resize(7);const auto bytes=bad.encode();d.load(bytes);rejected([&]{d.set_info_version(afterVersion);},"Ambiguous/short native version refuses atomically");require(d.save_bytes()==bytes&&!d.dirty()&&!d.can_undo(),"Invalid version retains original metadata and history");}
    auto unrelated=root;unrelated.find("LIST","UNFO")->find("UCMT")->data={0x31};const auto malformed=unrelated.encode();d.load(malformed);auto desired=unrelated;desired.find("LIST","UNFO")->find("UART")->data=utf16(L"Author only");require(d.set_info_text(SegmentInfoField::author,L"Author only")&&d.save_bytes()==desired.encode(),"Editing Author preserves malformed unrelated Information bytes without broad normalization");
}
void segment_music_start_tests(){
    auto d=SegmentDocument::create_empty({false,4,8000},L"Music playback start");d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence});d.add_note({0,768,0,72,96});d.add_note({2304,1536,0,72,96});d.edit_note_logical_time(1,3072);
    auto root=Chunk::parse(d.save_bytes());auto& header=root.find("segh")->data;header.resize(73);header[72]=0xad;put32(header,0,UINT32_MAX);put32(header,12,3072);put32(header,16,6912);put32(header,32,0x20);put32(header,36,0x12345678);put32(header,40,0x99887766);put32(header,48,0x55443322);put32(header,56,0x11223344);
    Chunk opaque;opaque.id="st??";opaque.data={9,4,7};opaque.padding=0xbd;root.children.push_back(opaque);const auto input=root.encode();d.load(input);d.select_range({192,3840,TimelineSequence});d.select_all();
    const auto expected=[&](std::int32_t start){auto x=root;put32(x.find("segh")->data,8,static_cast<std::uint32_t>(start));return x.encode();};
    require(d.play_start()==0&&!d.set_music_start(0)&&d.save_bytes()==input&&!d.dirty()&&!d.can_undo(),"Default playback start is a byte-exact clean no-op independent of the custom Infinite loop");
    for(const auto start:{1,192,768,1920,3072,6911})require(d.set_music_start(start)&&d.play_start()==start&&d.save_bytes()==expected(start),"Music start changes only its signed native DWORD across tick grid beat and loop-start boundaries");
    const auto selection=d.selected_range();require(selection.begin==192&&selection.end==3840&&selection.strips==TimelineSequence&&d.selected_groups()==1&&d.selected_sequence_index()==0,"Start edits preserve the editor range selected group and selected native Sequence track");
    d.load(input);require(d.set_music_start(1920)&&d.set_music_start(3072),"Independent playback-start edits create distinct document transactions");
    require(d.undo()&&d.save_bytes()==expected(1920)&&d.undo()&&d.save_bytes()==input,"Two Undo operations restore all native start states without changing loop or opaque bytes");
    require(d.redo()&&d.save_bytes()==expected(1920)&&d.redo()&&d.save_bytes()==expected(3072)&&d.undo()&&d.save_bytes()==expected(1920),"Redo and final Undo retain complete start-point states");
    const auto edited=d.save_bytes();SegmentDocument clean;clean.load(edited);
    require(clean.play_start()==1920&&clean.save_bytes()==edited&&!clean.dirty()&&!clean.can_undo()&&!clean.set_music_start(1920),"Independent load restores configured start without normalization history or dirty no-op");
    for(const auto start:{-1,6912,6913,12288,INT32_MAX}){rejected([&]{clean.set_music_start(start);},"Playback start outside length or at/after loop end is refused");require(clean.save_bytes()==edited&&!clean.dirty()&&!clean.can_undo(),"Invalid start preserves bytes dirty state and history atomically");}
    require(clean.set_music_start(0)&&clean.save_bytes()==input,"Restoring initial start leaves custom loop and extended reference data exact");
    for(const auto flags:{1u,2u,3u}){auto x=root;put32(x.find("segh")->data,32,flags|0x20);const auto bytes=x.encode();SegmentDocument clock;clock.load(bytes);rejected([&]{clock.set_music_start(1920);},"Clock/reference-length source cannot be rewritten by the music-time start editor");require(clock.save_bytes()==bytes&&!clock.dirty()&&!clock.can_undo(),"Clock start refusal retains every reference field extension flag and track");}
    for(const auto bounds:std::vector<std::pair<std::int32_t,std::int32_t>>{{-1,6912},{3072,-1},{6912,3072},{0,12289}}){auto x=root;put32(x.find("segh")->data,12,static_cast<std::uint32_t>(bounds.first));put32(x.find("segh")->data,16,static_cast<std::uint32_t>(bounds.second));const auto bytes=x.encode();SegmentDocument invalid;invalid.load(bytes);rejected([&]{invalid.set_music_start(1920);},"Imported invalid loop bounds cannot be silently normalized by a start edit");require(invalid.save_bytes()==bytes&&!invalid.dirty()&&!invalid.can_undo(),"Invalid imported loop refusal remains wholly atomic");}
    auto legacy=root;legacy.find("segh")->data.resize(40);const auto oldBytes=legacy.encode();SegmentDocument old;old.load(oldBytes);require(old.set_music_start(1920)&&Chunk::parse(old.save_bytes()).find("segh")->data.size()==40&&old.undo()&&old.save_bytes()==oldBytes,"Legacy40-byte start edits never expand the header and undo exactly");
    auto endZero=root;put32(endZero.find("segh")->data,12,0);put32(endZero.find("segh")->data,16,0);SegmentDocument whole;whole.load(endZero.encode());require(whole.set_music_start(12287)&&whole.play_start()==12287&&whole.loop_settings().end==0,"Native end0 sentinel allows a start up to the last source clock without rewriting loop bounds");
    const auto audition=sequence_audition_snapshot(d,d.notes()[1]);SegmentDocument preview;preview.load(audition);require(preview.play_start()==0&&preview.loop_settings().repeats==0&&d.save_bytes()==edited,"Private audition creates start0 and no repeat independently of the source configured start");
    require(d.redo()&&d.save_bytes()==expected(3072),"Reading start and constructing preview preserve the pending document redo");
}
void segment_playback_clock_tests(){
    const SegmentLoopSettings infinite{UINT32_MAX,3072,6912,0x20};
    for(const auto& pair:std::vector<std::pair<std::int64_t,std::int32_t>>{{0,0},{2304,2304},{6911,6911},{6912,3072},{10751,6911},{10752,3072},{12288,4608},{38400000006912LL,3072},{INT64_MAX,5887}})
        require(segment_playback_clock(12288,infinite,0,pair.first)==pair.second,"Infinite source clock wraps at exact boundaries without clamping to source length or overflowing long elapsed time");
    for(const auto& pair:std::vector<std::pair<std::int64_t,std::int32_t>>{{0,2304},{4607,6911},{4608,3072},{8448,3072}})
        require(segment_playback_clock(12288,infinite,2304,pair.first)==pair.second,"Explicit seek changes only the initial pass length before normal loop wrapping");
    const SegmentLoopSettings finite{2,768,1536,0};
    for(const auto& pair:std::vector<std::pair<std::int64_t,std::int32_t>>{{0,0},{1535,1535},{1536,768},{2303,1535},{2304,768},{3071,1535},{3072,1536},{4607,3071},{4608,3071},{INT64_MAX,3071}})
        require(segment_playback_clock(3072,finite,0,pair.first)==pair.second,"Finite Repeat2 traverses intro exactly two added loop passes then outro and clamps only at the final source end");
    require(segment_playback_clock(3072,{0,-9,-5,0},576,1536)==2112&&segment_playback_clock(3072,{0,768,1536,0},576,INT64_MAX)==3071,"Repeat0 stays linear even with unused imported loop bounds and very large elapsed time");
    require(segment_playback_clock(3072,{UINT32_MAX,0,0,0},0,6144)==0&&segment_playback_clock(3072,{1,0,0,0},0,6144)==3071,"Native end0 resolves to source length for Infinite and finite whole-Segment loops");
    require(segment_playback_clock(INT32_MAX,{UINT32_MAX-1,0,INT32_MAX,0},0,INT64_MAX)==INT32_MAX-1,"Largest finite repeat product and elapsed clock cannot overflow signed64 arithmetic");
    for(const auto flags:{1u,2u,3u})require(!segment_playback_clock(3072,{2,768,1536,flags},0,2000),"Clock/reference-time inputs do not claim a music-time cursor mapping");
    for(const auto& loop:std::vector<SegmentLoopSettings>{{1,-1,1536,0},{1,1536,1536,0},{1,768,3073,0},{1,768,-1,0}})
        require(!segment_playback_clock(3072,loop,0,2000),"Invalid repeating loop bounds return unavailable without mutating or normalizing native input");
    require(!segment_playback_clock(3072,finite,-1,0)&&!segment_playback_clock(3072,finite,3072,0)&&!segment_playback_clock(3072,finite,0,-1)&&!segment_playback_clock(0,finite,0,0)&&!segment_playback_clock(3072,finite,1536,0),"Invalid elapsed length seek and seek at loop end preserve the existing caller fallback");
}
void segment_music_loop_settings_tests(){
    segment_playback_clock_tests();
    auto d=SegmentDocument::create_empty({false,4,8000},L"Music loop settings");d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence});d.add_note({0,768,0,72,96});d.add_note({2304,1536,0,72,96});d.edit_note_logical_time(1,3072);
    auto root=Chunk::parse(d.save_bytes());auto& header=root.find("segh")->data;header.resize(73);header[72]=0xad;put32(header,32,0x20);put32(header,36,0x12345678);put32(header,40,0x99887766);put32(header,48,0x55443322);
    Chunk opaque;opaque.id="lp??";opaque.data={1,7,9};opaque.padding=0xbe;root.children.push_back(opaque);const auto input=root.encode();d.load(input);d.select_range({0,3072,TimelineSequence});d.select_all();
    const auto expected=[&](std::uint32_t repeats,std::int32_t start,std::int32_t end){auto x=root;auto& h=x.find("segh")->data;put32(h,0,repeats);put32(h,12,start);put32(h,16,end);return x.encode();};
    const auto initial=d.loop_settings();require(initial.repeats==0&&initial.start==0&&initial.end==0&&initial.flags==0x20&&initial.music_time(),"Loop getter preserves native0end sentinel and unrelated flags without normalizing the source");
    require(!d.set_music_loop(0,0,0)&&d.save_bytes()==input&&!d.dirty()&&!d.can_undo(),"Default whole-Segment loop is a byte-exact no-op with no history");
    require(d.set_music_loop(0,3072,0)&&d.save_bytes()==expected(0,3072,0),"Start-only loop edit keeps native end0 sentinel and every other header track opaque byte");
    require(d.set_music_loop(0,3072,6144)&&d.save_bytes()==expected(0,3072,6144),"Loop end measure edit is an independent complete transaction");
    require(d.set_music_loop(0,3072,6912)&&d.save_bytes()==expected(0,3072,6912),"Loop end beat edit preserves every sounding and logical note field");
    require(d.set_music_loop(1,3072,6912)&&d.save_bytes()==expected(1,3072,6912),"Finite Repeat1 changes only its native DWORD");
    const auto finite=d.save_bytes();require(d.set_music_loop(UINT32_MAX,3072,6912)&&d.save_bytes()==expected(UINT32_MAX,3072,6912),"Infinite is the full unsigned native repeat value");const auto custom=d.save_bytes();
    require(d.notes().size()==2&&d.notes()[1].time==2304&&d.note_positions()[1].logicalTime==3072&&d.note_positions()[1].offset==-768&&d.selected_groups()==1&&d.selected_range().end==3072&&d.selected_range().strips==TimelineSequence&&d.tempos()[0].selected,"Loop edits preserve selected group Timeline range Tempo selection and negative-offset notes");
    require(d.set_music_loop(UINT32_MAX,0,0)&&d.save_bytes()==expected(UINT32_MAX,0,0),"Reset bounds retains Infinite and restores only native0/0 whole-Segment fields");const auto reset=d.save_bytes();
    require(d.undo()&&d.save_bytes()==custom&&d.loop_settings().end==6912,"Undo Reset restores custom loop bounds and Infinite");
    require(d.undo()&&d.save_bytes()==finite&&d.loop_settings().repeats==1,"Second Undo restores the previous finite repeat count");
    require(d.redo()&&d.save_bytes()==custom&&d.redo()&&d.save_bytes()==reset&&d.undo()&&d.save_bytes()==custom,"Two Redo and final Undo preserve complete custom/reset native states");
    SegmentDocument reload;reload.load(custom);require(reload.save_bytes()==custom&&!reload.dirty()&&!reload.can_undo()&&reload.loop_settings().start==3072&&reload.loop_settings().end==6912&&reload.loop_settings().repeats==UINT32_MAX,"Independent clean load restores custom Infinite loop without editor history");
    for(const auto bounds:std::vector<std::pair<std::int32_t,std::int32_t>>{{-1,6912},{3072,-1},{12288,0},{3072,3072},{3073,3072},{0,12289},{INT32_MAX,INT32_MAX}}){rejected([&]{reload.set_music_loop(2,bounds.first,bounds.second);},"Invalid loop interval is rejected atomically");require(reload.save_bytes()==custom&&!reload.dirty()&&!reload.can_undo(),"Rejected interval leaves all bytes dirty state and history unchanged");}
    require(!reload.set_music_loop(UINT32_MAX,3072,6912)&&reload.save_bytes()==custom&&!reload.dirty(),"Repeated committed loop values are a byte-exact clean no-op");
    require(prepare_command_playback(prepare_style_playback(custom,{}).segment)==custom&&NativeScriptTrackRuntime::loader_bytes(custom)==custom,"Native runtime preparations retain the exact edited loop and unknown extended header bytes");
    auto audition=sequence_audition_snapshot(reload,reload.notes()[1]);SegmentDocument privateNote;privateNote.load(audition);require(privateNote.loop_settings().repeats==0&&privateNote.loop_settings().start==0&&privateNote.loop_settings().end==0&&source_playback_prepare_milliseconds(audition,true)==300,"Private note audition cannot inherit Infinite or custom music-time loop settings");
    for(const auto flags:{1u,2u,3u}){auto x=root;put32(x.find("segh")->data,32,flags|0x20);const auto bytes=x.encode();SegmentDocument clock;clock.load(bytes);require(!clock.loop_settings().music_time(),"Clock or reference-length input is identified as outside this music-time editor");rejected([&]{clock.set_music_loop(2,0,0);},"Music loop editor does not rewrite incoming clock-time settings");require(clock.save_bytes()==bytes&&!clock.dirty()&&!clock.can_undo(),"Clock-time refusal preserves flags reference times header tail and every track");}
    auto legacy=root;legacy.find("segh")->data.resize(40);SegmentDocument old;old.load(legacy.encode());require(old.set_music_loop(4,0,0)&&Chunk::parse(old.save_bytes()).find("segh")->data.size()==40,"Legacy40byte loop edits never expand the Segment header");
    auto seek=root;put32(seek.find("segh")->data,8,7680);SegmentDocument lateStart;lateStart.load(seek.encode());rejected([&]{lateStart.set_music_loop(1,3072,6912);},"Loop end must remain after the retained playback start");
    require(d.redo()&&d.save_bytes()==reset&&d.undo()&&d.save_bytes()==custom,"Loop getter and private audition leave the existing pending Reset redo/undo history usable");
}
void sequence_loop_logical_tests(){
    SegmentCreationLength length;length.measures=4;auto d=SegmentDocument::create_empty(length,L"Logical loop");
    require(d.add_editor_tracks({SegmentTrackKind::tempo,SegmentTrackKind::sequence})&&d.add_note({0,768,0,72,96})&&d.add_note({2304,1536,0,72,96}),"Loop fixture has an initial pilot and a physically pre-loop C6 note");
    auto root=Chunk::parse(d.save_bytes());auto& h=root.find("segh")->data;h.resize(73);h[72]=0x91;put32(h,0,UINT32_MAX);put32(h,12,3072);put32(h,16,6912);put32(h,32,0x20);
    auto& track=root.find("LIST","trkl")->children.back();track.find("trkh")->data.resize(36,0xb7);auto& raw=track.find("seqt")->data;
    require(raw.size()>=52&&std::memcmp(raw.data(),"evtl",4)==0&&read32(raw,8)==20,"Loop expected-byte oracle uses the declared native20-byte note record layout");
    raw[49]=0xa1;raw[50]=0xb2;raw[51]=0xc3;Chunk opaque;opaque.id="lp??";opaque.data={9,7,3};opaque.padding=0xbc;const auto hidden=opaque.encode();raw.insert(raw.end(),hidden.begin(),hidden.end());track.children.push_back(opaque);root.children.push_back(opaque);
    const auto input=root.encode();d.load(input);d.select_range({0,3072,TimelineSequence});
    const auto inLoop=[](const SequenceNotePosition& p){return p.logicalTime>=3072&&p.logicalTime<6912;};
    require(d.note_positions().size()==2&&!inLoop(d.note_positions()[0])&&!inLoop(d.note_positions()[1])&&d.note_positions()[1].logicalTime==2304&&d.note_positions()[1].offset==0&&d.notes()[1].time==2304,"Default ownership keeps both pilot and physicalbeat4 outside the native loop interval");
    require(NativeScriptTrackRuntime::loader_bytes(input)==input,"Native loader preparation preserves loop repeat points logical note bytes unknown headers and odd padding without Script tracks");
    require(prepare_command_playback(prepare_style_playback(input,{}).segment)==input,"Style and Command preparation do not rewrite the loop fixture or its native note ownership");
    auto preview=sequence_audition_snapshot(d,d.notes()[1]);auto previewTree=Chunk::parse(preview);auto ph=previewTree.find("segh")->data;SegmentDocument audition;audition.load(preview);
    require(read32(ph,0)==0&&read32(ph,12)==0&&read32(ph,16)==0&&audition.length()==3072&&audition.notes().size()==1&&audition.notes()[0].time==0&&audition.notes()[0].duration==384&&d.save_bytes()==input,"Private audition of a pre-loop note creates a short nonloop Segment and leaves the infinite source loop unchanged");
    const auto expected=[&](std::int32_t logical){auto e=root;auto& payload=e.find("LIST","trkl")->children.back().find("seqt")->data;put32(payload,32,logical);const auto offset=static_cast<std::uint16_t>(2304-logical);payload[44]=static_cast<BYTE>(offset);payload[45]=static_cast<BYTE>(offset>>8);return e.encode();};
    size_t selected=99;require(d.edit_note_logical_time(1,5376,&selected)&&selected==1,"The measure field can move ownership to2:4 while retaining the explicit sounding occurrence");
    const auto intermediate=d.save_bytes();require(intermediate==expected(5376),"Intermediate measure edit changes only the target rawLONG and signedSHORT preserving all loop and opaque bytes");
    const auto range=d.selected_range();require(d.notes()[1].time==2304&&d.notes()[1].duration==1536&&d.notes()[1].pitch==72&&d.notes()[1].velocity==96&&d.selected_groups()==1&&range.begin==0&&range.end==3072&&range.strips==TimelineSequence,"Ownership edit preserves sounding position duration pitch velocity group and Timeline range");
    require(d.edit_note_logical_time(1,3072,&selected)&&selected==1,"The beat field moves the same note's ownership to the inclusive loop start2:1");
    const auto member=d.save_bytes();require(member==expected(3072),"Final membership edit retains loop repetition count points header tail every other record reserved bytes and opaque padding");
    require(inLoop(d.note_positions()[1])&&d.note_positions()[1].logicalTime==3072&&d.note_positions()[1].offset==-768&&d.notes()[1].time==2304&&d.notes()[1].time<3072,"A physically earlier note can belong to the loop via an exact negative one-beat offset");
    require(NativeScriptTrackRuntime::loader_bytes(member)==member,"The native loader copy preserves the negative-offset loop member instead of normalizing it to physical time");
    require(prepare_command_playback(prepare_style_playback(member,{}).segment)==member,"Style and Command snapshots preserve final logical loop membership exactly");
    preview=sequence_audition_snapshot(d,d.notes()[1]);previewTree=Chunk::parse(preview);ph=previewTree.find("segh")->data;audition.load(preview);
    require(read32(ph,0)==0&&read32(ph,12)==0&&read32(ph,16)==0&&audition.note_positions()[0].logicalTime==0&&audition.note_positions()[0].offset==0&&audition.notes()[0].duration==384&&d.save_bytes()==member,"Private audition of a loop member does not inherit infinite repeats negative offset or source logical ownership");
    require(d.undo()&&d.save_bytes()==intermediate,"One beat Undo restores the preceding measure edit exactly");
    require(d.undo()&&d.save_bytes()==input&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"A second Undo restores the complete clean original loop fixture");
    for(const auto invalid:{-1,d.length(),INT32_MAX,2304})require(!d.edit_note_logical_time(1,invalid)&&d.save_bytes()==input&&!d.dirty()&&!d.can_undo()&&d.can_redo(),"Invalid or unchanged loop ownership preserves every byte and both pending Redo steps");
    require(!d.edit_note_logical_time(99,3072)&&d.save_bytes()==input&&d.can_redo(),"A stale sounding index cannot move an unrelated note into the loop");
    require(d.redo()&&d.save_bytes()==intermediate,"First Redo restores only the measure ownership edit");
    require(d.redo()&&d.save_bytes()==member&&!d.can_redo(),"Second Redo restores the exact final loop membership bytes");
    SegmentDocument reload;reload.load(member);require(reload.save_bytes()==member&&!reload.dirty()&&!reload.can_undo()&&inLoop(reload.note_positions()[1])&&reload.notes()[1].time==2304&&reload.notes()[1].duration==1536,"Independent native load retains loop membership and physical note values without session history");
    require(reload.edit_note_logical_time(1,6912)&&!inLoop(reload.note_positions()[1])&&reload.save_bytes()==expected(6912),"Ownership at the exclusive loop end remains representable without moving the sounding note");
    require(reload.undo()&&reload.save_bytes()==member,"Undo from the loop-end boundary restores the exact inclusive-start member");
    require(source_playback_prepare_milliseconds({},true)==300&&source_playback_prepare_milliseconds({},false)==1000,"Empty responsive and complex carriers retain their existing preparation policy");
    require(source_playback_prepare_milliseconds(input,true)==300&&d.save_bytes()==member,"A loop with zero offsets keeps short preparation without editing the source");
    require(source_playback_prepare_milliseconds(intermediate,true)==1000,"Negative intermediate ownership requests full native preparation; long-offset audibility remains unqualified");
    require(source_playback_prepare_milliseconds(member,true)==1000,"A one-beat negative loop offset has full lookahead on the responsive primary path");
    require(source_playback_prepare_milliseconds(member,false)==1000,"Complex playback retains full preparation regardless of negative-note inspection");
    require(source_playback_prepare_milliseconds(preview,true)==300,"Independent zero-offset nonloop private audition keeps responsive preparation");
    auto positive=root;auto& positiveRaw=positive.find("LIST","trkl")->children.back().find("seqt")->data;positiveRaw[44]=0;positiveRaw[45]=3;
    require(source_playback_prepare_milliseconds(positive.encode(),true)==300,"A physically later positive note offset does not require early lookahead");
    auto silent=Chunk::parse(member);auto& silentRaw=silent.find("LIST","trkl")->children.back().find("seqt")->data;silentRaw[48]=0;
    require(source_playback_prepare_milliseconds(silent.encode(),true)==300,"A velocity-zero note-off with negative storage offset does not lengthen sounding-note preparation");
    silentRaw[48]=96;silentRaw[46]=0xb0;
    require(source_playback_prepare_milliseconds(silent.encode(),true)==300,"A non-note native record cannot be mistaken for an early sounding note");
    auto hiddenGroup=root;auto hiddenTrack=Chunk::parse(member).find("LIST","trkl")->children.back();put32(hiddenTrack.find("trkh")->data,20,2);hiddenGroup.find("LIST","trkl")->children.push_back(hiddenTrack);const auto hiddenBytes=hiddenGroup.encode();
    require(source_playback_prepare_milliseconds(hiddenBytes,true)==1000&&hiddenGroup.encode()==hiddenBytes,"An unselected group2 negative Sequence requests full lookahead without rewriting masks tracks or bytes");
    put32(hiddenTrack.find("trkh")->data,0,0x12345678);hiddenGroup.find("LIST","trkl")->children.back()=hiddenTrack;
    require(source_playback_prepare_milliseconds(hiddenGroup.encode(),true)==300,"Unknown track classes with Sequence-shaped opaque bytes do not affect native note preparation");
    auto malformed=root;malformed.find("LIST","trkl")->children.back().find("seqt")->data={1};bool prepareRejected=false;try{(void)source_playback_prepare_milliseconds(malformed.encode(),true);}catch(const std::exception&){prepareRejected=true;}
    require(prepareRejected,"Malformed native Sequence events fail validation instead of silently selecting short preparation");
}
void band_logical_collision_tests(){
    auto d=SegmentDocument::create_empty({},L"Band collisions");d.add_tracks({SegmentTrackKind::band});
    require(d.insert_default_band(0)&&d.insert_default_band(768)&&d.insert_default_band(1536),"Collision fixture has three independent Band instances");
    const auto original=d.band_events();size_t at=99;
    require(d.update_band(2,0,191,original[2].band,&at)&&at==1,"Moving the selected third Band onto the first logical time returns its stable occurrence index");
    require(d.band_events()[0].band==original[0].band&&d.band_events()[1].band==original[2].band&&d.band_events()[2].band==original[1].band,"Logical collision sorts only the selected occurrence without replacing its sibling");
    require(d.update_band(2,0,96,original[1].band,&at)&&at==2,"A third same-logical Band remains a separate explicit occurrence");
    const auto collisions=d.save_bytes();d.load(collisions);
    require(d.band_events().size()==3&&d.band_events()[0].logicalTime==0&&d.band_events()[1].logicalTime==0&&d.band_events()[2].logicalTime==0,"Native duplicate logical times reload without collapsing records");
    rejected([&]{d.set_band(0,original[1].band);},"A timed assignment cannot guess among duplicate logical positions");
    require(d.save_bytes()==collisions&&!d.dirty()&&!d.can_undo(),"Ambiguous timed assignment leaves complete native bytes and history intact");
    BandDocument changed;changed.load(original[2].band);changed.set_name(L"Selected C");
    require(d.update_band(1,0,191,changed.save_bytes(),&at)&&at==1,"Indexed content edit retains the selected same-time instance");
    require(d.band_events()[0].band==original[0].band&&d.band_events()[2].band==original[1].band&&d.band_events()[1].band==changed.save_bytes(),"Renaming one colliding Band preserves both sibling payloads");
    const auto renamed=d.save_bytes();require(d.undo()&&d.save_bytes()==collisions&&!d.can_undo()&&d.redo()&&d.save_bytes()==renamed,"Same-time content edit is one complete UndoRedo transaction");
    require(assign_sequence_gm_patch(d,200,0,16)&&sequence_patch(d,200,0)->patch==16,"Sequence instrument assignment reaches the most recent physical Band among duplicate logical times");
    require(d.band_events()[0].band==original[0].band&&d.band_events()[2].band==original[1].band,"Sequence patch assignment does not change a same-logical sibling");
    require(d.undo()&&d.save_bytes()==renamed,"Duplicate-time Sequence instrument assignment undoes to exact bytes");
    auto root=Chunk::parse(collisions);auto track=std::find_if(root.find("LIST","trkl")->children.begin(),root.find("LIST","trkl")->children.end(),is_band_track);
    auto& slots=track->find("RIFF","DMBT")->find("LIST","lbdl")->children;
    Chunk opaque;opaque.id="dup?";opaque.data={1,3,7};opaque.padding=0xa5;slots.insert(slots.begin()+1,opaque);auto header=slots[2].find("bd2h");header->data.push_back(0xbc);header->padding=0x93;
    const auto extended=root.encode();d.load(extended);require(d.update_band(1,768,191,original[2].band,&at)&&at==2,"Selected colliding occurrence can move out of the stack with its new index");
    const auto moved=Chunk::parse(d.save_bytes());const Chunk* movedTrack=nullptr;for(const auto& t:moved.find("LIST","trkl")->children)if(is_band_track(t))movedTrack=&t;
    const auto& retained=movedTrack->find("RIFF","DMBT")->find("LIST","lbdl")->children;
    require(retained[1].encode()==opaque.encode()&&retained[3].find("bd2h")->data.back()==0xbc&&retained[3].find("bd2h")->padding==0x93,"Duplicate sorting preserves opaque sibling slots and selected extended header padding");
    require(d.undo()&&d.save_bytes()==extended,"Moving out of a logical stack undoes all native bytes");
    Framework host;const auto si=host.new_segment();auto& owner=host.document(si);owner=SegmentDocument::create_empty({},L"Copied Band identities");owner.add_tracks({SegmentTrackKind::band});owner.insert_default_band(0);
    auto copies=Chunk::parse(owner.save_bytes());Chunk* copyTrack=nullptr;for(auto& t:copies.find("LIST","trkl")->children)if(is_band_track(t))copyTrack=&t;
    auto& copySlots=copyTrack->find("RIFF","DMBT")->find("LIST","lbdl")->children;put32(copySlots[0].find("bd2h")->data,4,0);copySlots.push_back(copySlots[0]);put32(copySlots[1].find("bd2h")->data,4,96);owner.load(copies.encode());
    auto first=make_embedded_band_target(host,si,1,0,0),second=make_embedded_band_target(host,si,1,0,1);const auto copied=owner.save_bytes();
    require(read_embedded_band(host,second)&&read_embedded_band(host,second)->target.event==1,"Explicit selection distinguishes same-GUID same-logical copied native Bands");
    require(embedded_band_is_playing(copied,first,0)&&!embedded_band_is_playing(copied,second,0)&&embedded_band_is_playing(copied,second,96),"Live guard distinguishes physical intervals even when Band identity and logical time are copied");
    require(owner.add_tempo(0,120)&&!read_embedded_band(host,second),"An independent owner edit disables an ambiguous stale copied-identity target");
    const auto independent=owner.save_bytes();BandDocument replacement;replacement.load(second.lastBytes);replacement.set_name(L"Only second");
    require(!update_embedded_band(host,second,0,96,replacement.save_bytes())&&owner.save_bytes()==independent,"An unavailable copied target cannot mutate a guessed Band");
    require(owner.undo()&&owner.save_bytes()==copied&&read_embedded_band(host,second)->target.event==1,"Exact owner snapshot restoration reestablishes the explicit occurrence");
    require(update_embedded_band(host,second,0,96,replacement.save_bytes())&&owner.band_events()[0].band==first.lastBytes&&owner.band_events()[1].band==replacement.save_bytes(),"Explicit copied occurrence update changes only the selected payload");
    require(owner.undo()&&owner.save_bytes()==copied&&!read_embedded_band(host,second),"Undo preserves copies exactly and never guesses a stale ambiguous identity");
    second=make_embedded_band_target(host,si,1,0,1);require(read_embedded_band(host,second)->target.event==1,"Explicit reselection restores the intended copied Band after Undo");
}
void sequence_logical_time_tests(){
    const auto record=[](int time,BYTE status,BYTE pitch,BYTE velocity,short offset){Bytes b(24,0xa7);put32(b,0,time);put32(b,4,192);put32(b,8,17);const auto encoded=static_cast<std::uint16_t>(offset);b[12]=static_cast<BYTE>(encoded);b[13]=static_cast<BYTE>(encoded>>8);b[14]=status;b[15]=pitch;b[16]=velocity;return b;};
    const auto first=record(1000,0x93,61,80,-12),control=record(1100,0xb3,7,100,0),off=record(1200,0x83,61,0,0),second=record(1400,0x93,64,99,7);
    Chunk opaque;opaque.id="log?";opaque.data={3,1,4};opaque.padding=0xce;Chunk curve;curve.id="curl";curve.data={32,0,0,0};
    const auto payload=[&](const std::vector<Bytes>& records){Chunk events;events.id="evtl";events.data.resize(4);put32(events.data,0,24);for(const auto& b:records)events.data.insert(events.data.end(),b.begin(),b.end());auto result=events.encode();const auto cb=curve.encode(),ob=opaque.encode();result.insert(result.end(),cb.begin(),cb.end());result.insert(result.end(),ob.begin(),ob.end());return result;};
    const auto input=payload({first,control,off,second});const auto positions=sequence_note_positions(input);
    require(positions.size()==2&&positions[0].logicalTime==1000&&positions[0].offset==-12&&positions[0].note.time==988&&positions[1].logicalTime==1400&&positions[1].offset==7&&positions[1].note.time==1407,"Logical note reads retain signed offsets while excluding controllers and note-offs from sounding indexes");
    require(sequence_notes(input)[0].time==988&&sequence_notes(input)[1].time==1407,"Existing Note API continues to expose physical onset rather than logical ownership");
    size_t selected=99;const auto edited=sequence_change_logical_time(input,0,1536,&selected);auto expectedFirst=first;put32(expectedFirst,0,1536);expectedFirst[12]=0xdc;expectedFirst[13]=0xfd;
    require(selected==1&&edited==payload({control,off,second,expectedFirst}),"Logical move changes only raw LONG and signed SHORT of the explicit record while retaining other MIDI order status extensions curve and odd opaque padding");
    const auto after=sequence_note_positions(edited);
    require(after[1].note.time==988&&after[1].note.duration==192&&after[1].note.channel==17&&after[1].note.pitch==61&&after[1].note.velocity==80&&after[1].logicalTime==1536&&after[1].offset==-548,"Logical edit preserves every physical note value and publishes the moved sounding index");
    require(sequence_change_logical_time(edited,1,1536,&selected)==edited&&selected==1,"Unchanged logical time preserves complete native ordering and bytes");
    const auto lower=sequence_note_positions(sequence_change_logical_time(input,0,33756));
    require(lower[1].note.time==988&&lower[1].offset==INT16_MIN&&lower[1].logicalTime==33756,"Negative signed SHORT endpoint retains the physical onset");
    rejected([&]{sequence_change_logical_time(input,0,33757);},"A negative offset below signed SHORT is rejected rather than truncated");
    auto plain=sequence_track().find("seqt")->data;plain=sequence_insert(plain,{40000,192,0xfffffffbu,72,96});
    const auto upper=sequence_note_positions(sequence_change_logical_time(plain,0,7233));
    require(upper[0].offset==INT16_MAX&&upper[0].note.time==40000&&upper[0].note.channel==0xfffffffbu,"Positive signed SHORT endpoint retains the highest nonreserved DWORD PChannel");
    rejected([&]{sequence_change_logical_time(plain,0,7232);},"A positive offset above signed SHORT is rejected rather than truncated");
    rejected([&]{sequence_change_logical_time(input,0,-1);},"Negative logical ownership is not silently wrapped into a DWORD");
    rejected([&]{sequence_change_logical_time(input,9,768);},"Logical mutation rejects a stale sounding-note index");
    auto duplicate=sequence_track().find("seqt")->data;duplicate=sequence_insert(duplicate,{1000,192,17,61,80});duplicate=sequence_insert(duplicate,{1000,192,17,61,80});
    const auto duplicateEdited=sequence_change_logical_time(duplicate,0,1536,&selected);const auto duplicatePositions=sequence_note_positions(duplicateEdited);
    require(selected==1&&duplicatePositions[0].logicalTime==1000&&duplicatePositions[1].logicalTime==1536&&duplicatePositions[0].note.time==1000&&duplicatePositions[1].note.time==1000,"Explicit occurrence editing does not collapse physical-identical sounding notes");
    auto owner=SegmentDocument::create_empty({false,20,40000},L"Logical owner");owner.add_tracks({SegmentTrackKind::sequence});owner.select_track_group(2);owner.add_tracks({SegmentTrackKind::sequence});owner.add_note({2000,192,19,67,96});owner.select_track_group(1);
    auto root=Chunk::parse(owner.save_bytes());auto& tracks=root.find("LIST","trkl")->children;auto target=std::find_if(tracks.begin(),tracks.end(),is_sequence_track);target->find("seqt")->data=input;target->find("trkh")->data.resize(36,0xb9);target->children.push_back(opaque);const auto native=root.encode();owner.load(native);owner.select_track_group(1);
    selected=99;require(owner.edit_note_logical_time(0,1536,&selected)&&selected==1,"Document logical mutation addresses the selected group and returns its owned occurrence");
    auto expectedRoot=root;auto expectedTrack=std::find_if(expectedRoot.find("LIST","trkl")->children.begin(),expectedRoot.find("LIST","trkl")->children.end(),is_sequence_track);expectedTrack->find("seqt")->data=edited;
    require(owner.save_bytes()==expectedRoot.encode(),"Logical document edit preserves the other group track headers opaque chunks GUID and all unrelated native bytes");
    const auto nativeEdited=owner.save_bytes();require(owner.undo()&&owner.save_bytes()==native&&!owner.dirty()&&!owner.can_undo()&&owner.can_redo(),"One logical ownership edit has a complete clean native-byte Undo");
    for(const auto invalid:std::vector<std::pair<size_t,int>>{{0,-1},{0,owner.length()},{0,33757},{99,768},{0,1000}}){selected=99;require(!owner.edit_note_logical_time(invalid.first,invalid.second,&selected)&&selected==99&&owner.save_bytes()==native&&!owner.dirty()&&!owner.can_undo()&&owner.can_redo(),"Invalid or unchanged logical ownership preserves bytes dirty history Redo and caller selection");}
    require(owner.redo()&&owner.save_bytes()==nativeEdited&&!owner.can_redo(),"Logical Redo restores the exact selected-record mutation");
    owner.select_track_group(2);require(owner.note_positions().size()==1&&owner.note_positions()[0].logicalTime==2000&&owner.note_positions()[0].note.channel==19,"Logical view and mutation leave another selected group independent");
    const Timeline axis;const SequenceNotePosition logical{positions[0]};
    require(sequence_property_logical_time(axis,61440,logical,1,2)==1000,"An unchanged displayed logical beat retains its hidden native tick");
    require(sequence_property_logical_time(axis,61440,logical,1,3)==1768&&sequence_property_logical_time(axis,61440,logical,0,2)==4072,"Logical measure and beat Properties preserve the hidden tick and physical onset");
    for(const auto invalid:std::vector<std::pair<unsigned,std::int64_t>>{{2,1},{0,0},{1,5},{0,21},{0,INT64_MAX}})rejected([&]{sequence_property_logical_time(axis,61440,logical,invalid.first,invalid.second);},"Logical Properties reject invalid field division Segment endpoint and wide values");
    auto meterOwner=SegmentDocument::create_empty({false,20,40000},L"Logical meter");meterOwner.set_meter(1,3,8,3);const auto meterAxis=meterOwner.timeline();
    require(sequence_property_logical_time(meterAxis,meterOwner.length(),{{3168,192,17,61,80},3172,-4},1,3)==3940,"Logical beat Properties use the selected changed eighth-note meter without changing onset");
    rejected([&]{sequence_property_logical_time(meterAxis,meterOwner.length(),{{600,192,17,61,80},600,0},0,2);},"A hidden tick outside a destination meter beat is rejected instead of normalized");
    const Note samePhysical{1000,192,17,61,80};const std::vector<SequenceNotePosition> historyBefore{{samePhysical,1000,0},{samePhysical,768,232}},historyAfter{{samePhysical,768,232},{samePhysical,1536,-536}};
    require(sequence_property_position_history_selection(historyBefore,historyAfter,0)==std::optional<size_t>(1)&&sequence_property_position_history_selection(historyAfter,historyBefore,1)==std::optional<size_t>(0),"Logical-only history follows its unique occurrence even among physical-identical notes");
    require(sequence_property_position_history_selection(historyBefore,historyAfter,1)==std::optional<size_t>(0),"History keeps an unchanged logical sibling across another record's reorder");
    require(!sequence_property_position_history_selection({historyBefore[0],historyBefore[0]},historyAfter,0)&&!sequence_property_position_history_selection(historyBefore,{historyBefore[1],historyBefore[1]},0),"Ambiguous native logical duplicates clear Properties instead of guessing a replacement");
    auto bulk=historyAfter;bulk[0].note.velocity=72;require(!sequence_property_position_history_selection(historyBefore,bulk,0)&&!sequence_property_position_history_selection(historyBefore,historyAfter,99),"Bulk and stale logical history clear the selected Properties target safely");
}
void sequence_strip_tests(const std::filesystem::path& dir){
    segment_extended_boundary_tests();segment_invalidation_settings_tests();segment_boundary_settings_tests();segment_info_tests();segment_music_start_tests();segment_music_loop_settings_tests();sequence_loop_logical_tests();
    band_logical_collision_tests();
    sequence_logical_time_tests();
    const BandMixerArea mixArea{100,200,251,349,24,22};
    const auto grayChannels=BandDocument::inactive_default().instruments();bool overlap=grayChannels.size()==16;
    for(size_t i=0;i<grayChannels.size();++i){const auto p=band_mixer_position(grayChannels[i],i,mixArea);overlap=overlap&&p.x==164&&p.y==227;}
    require(overlap,"All sixteen inactive default channels overlap at stored pan64 volume100 rather than an ordinal rack");
    auto gray=grayChannels[0];gray.pchannel=0xfffffffbu;gray.pan=0;gray.volume=127;auto point=band_mixer_position(gray,15,mixArea);
    require(point.x==100&&point.y==200,"An inactive high DWORD channel reaches hard left and maximum volume");
    gray.pan=127;gray.volume=0;point=band_mixer_position(gray,0,mixArea);
    require(point.x==227&&point.y==327,"An inactive channel reaches hard right and zero volume independently of validity");
    gray.pan=UINT32_MAX;gray.volume=UINT32_MAX;point=band_mixer_position(gray,0,mixArea);
    require(point.x==227&&point.y==200,"Out of range stored inactive bytes cannot position a button beyond either MIDI edge");
    const auto unchangedDrag=band_mixer_drag(grayChannels,{0,15},mixArea,0,0);
    require(unchangedDrag.size()==2&&unchangedDrag[0].pan==64&&unchangedDrag[0].volume==100&&unchangedDrag[1].pan==64&&unchangedDrag[1].volume==100,"Starting a grouped drag on inactive overlapping values introduces no rack displacement");
    const auto shiftedDrag=band_mixer_drag(grayChannels,{0,15},mixArea,4,-7);
    require(shiftedDrag.size()==2&&shiftedDrag[0].pan==68&&shiftedDrag[0].volume==107&&shiftedDrag[1].pan==68&&shiftedDrag[1].volume==107,"Inactive grouped drag uses stored pan volume with the same relative delta for both selected channels");
    const BandMixerArea smallMixArea{1,2,42,47,24,22};bool quantizedMix=true;
    for(unsigned n=0;n<128;++n){gray.pan=n;gray.volume=127-n;const auto p=band_mixer_position(gray,n,smallMixArea);const auto v=band_mixer_value(gray.pchannel,p,smallMixArea);quantizedMix=quantizedMix&&std::abs(int(v.pan)-int(n))<=4&&std::abs(int(v.volume)-int(127-n))<=3;}
    require(quantizedMix,"Resized inactive graph inverse mapping stays within half-pixel MIDI quantization on both axes");
    SequencePitchView pitchView,otherPitchView;
    require(pitchView.visible_rows()==18&&pitchView.pitch_at(0)==70&&pitchView.pitch_at(215)==53,"Default Part viewport maps both visible endpoints to MIDI keys");
    require(!pitchView.pitch_at(-1)&&!pitchView.pitch_at(216),"Part header and CC boundary cannot select invisible notes");
    pitchView.scroll(1);require(pitchView.topPitch==71&&pitchView.pitch_at(12)==70,"Keyboard arrow advances one semitone without changing pitch-row height");
    pitchView.zoom(1);require(pitchView.rowHeight==13&&pitchView.visible_rows()==17&&pitchView.pitch_at(13)==70&&SequencePitchView::areaHeight==216,"Part vertical zoom changes key spacing while retaining the CC boundary");
    require(otherPitchView.rowHeight==12&&otherPitchView.topPitch==70,"Zoom and pitch scroll belong only to the operated Part");
    const auto noteBounds=pitchView.note_bounds(70,96);
    require(noteBounds.first==17&&noteBounds.second==25&&pitchView.pitch_at(noteBounds.first)==70&&pitchView.pitch_at(noteBounds.second-1)==70,"Zoomed velocity-height note rectangle and pitch hit map identify the same MIDI note");
    require(pitchView.drag_pitch(13)==1&&pitchView.drag_pitch(-13)==-1&&pitchView.drag_pitch(6)==0&&pitchView.drag_pitch(7)==1,"Zoomed drag maps a row to one semitone and rounds at its midpoint");
    require(pitchView.drag_velocity(13)==125&&pitchView.drag_velocity(-13)==-125,"Velocity drag uses the operated Part's vertical zoom scale");
    pitchView.scroll(INT32_MAX);require(pitchView.topPitch==127&&pitchView.scroll_position()==0&&pitchView.pitch_at(0)==127,"Keyboard scrollbar reaches the highest MIDI key without overflow");
    pitchView.set_scroll_position(INT32_MAX);require(pitchView.topPitch==16&&pitchView.pitch_at(215)==0,"Keyboard scrollbar reaches the lowest MIDI key with a partial final row");
    pitchView.zoom(INT32_MIN);require(pitchView.rowHeight==2&&pitchView.visible_rows()==108&&pitchView.topPitch==107&&pitchView.pitch_at(215)==0,"Minimum vertical zoom clamps range to valid MIDI keys");
    pitchView.zoom(INT32_MAX);require(pitchView.rowHeight==32&&pitchView.visible_rows()==7,"Maximum vertical zoom retains a finite visible pitch count");
    pitchView.follow(0);require(pitchView.topPitch==6&&pitchView.pitch_at(215)==0,"Following a low cursor preserves valid bottom keys at maximum zoom");
    pitchView.follow(127);require(pitchView.topPitch==127,"Following a high cursor makes MIDI127 visible");
    const auto base=dir/L"sequence-strip";std::filesystem::create_directories(base);
    auto doc=SegmentDocument::create_empty({false,20,40000},L"Sequence strip");doc.add_tracks({SegmentTrackKind::band,SegmentTrackKind::sequence});doc.insert_default_band(0);
    const auto input=doc.save_bytes();doc.load(input);const auto axis=doc.timeline();
    auto pitchOwner=SegmentDocument::create_empty({},L"Zoomed note owner");pitchOwner.add_tracks({SegmentTrackKind::sequence});pitchOwner.add_note({1000,192,17,70,96});const auto pitchBefore=pitchOwner.save_bytes();pitchOwner.load(pitchBefore);
    SequencePitchView editedView;editedView.zoom(1);const auto zoomedMove=sequence_drag_note(pitchOwner.timeline(),pitchOwner.length(),pitchOwner.notes()[0],SequenceNoteHandle::move,0,editedView.drag_pitch(13),0,SequenceSnap::none);
    require(pitchOwner.edit_note(0,zoomedMove)&&pitchOwner.notes()[0].pitch==71&&pitchOwner.notes()[0].time==1000&&pitchOwner.notes()[0].duration==192&&pitchOwner.notes()[0].channel==17&&pitchOwner.notes()[0].velocity==96,"One zoomed row of drag changes only the owned note pitch by one semitone");
    require(pitchOwner.undo()&&pitchOwner.save_bytes()==pitchBefore,"Zoom-derived native note edit undoes to complete original bytes");
    const Note quantizeSource{1000,500,0xfffffffbu,60,96};SequenceQuantizeOptions quantize;
    auto quantized=sequence_quantize_note(axis,doc.length(),quantizeSource,quantize);
    require(quantized.time==1152&&quantized.duration==500&&quantized.channel==0xfffffffbu&&quantized.pitch==60&&quantized.velocity==96,"Quantize Start preserves duration and the highest nonreserved DWORD PChannel");
    quantize.start=false;quantize.duration=true;quantized=sequence_quantize_note(axis,doc.length(),quantizeSource,quantize);
    require(quantized.time==1000&&quantized.duration==384,"Duration-only quantization retains the unsnapped start");
    quantize.start=true;quantize.strength=50;quantized=sequence_quantize_note(axis,doc.length(),quantizeSource,quantize);
    require(quantized.time==1076&&quantized.duration==442,"Strength50 moves both timing values halfway to their beat divisions");
    quantized=sequence_quantize_note(axis,doc.length(),quantized,quantize);
    require(quantized.time==1114&&quantized.duration==413,"Repeated Strength50 moves half of the remaining distance");
    const SequenceQuantizeOptions observedStrength33{true,false,2,33},observedStrength1{true,false,2,1};
    require(sequence_quantize_note(axis,doc.length(),{2784,576,0,61,80},observedStrength33).time==2753,"Original selected Strength33 truncates a negative fractional timing delta towards zero");
    require(sequence_quantize_note(axis,doc.length(),{1728,192,0,61,96},observedStrength1).time==1729,"Original Part Strength1 truncates a positive fractional timing delta towards zero");
    require(sequence_quantize_note(axis,doc.length(),{2784,576,0,61,80},observedStrength1).time==2784,"Original Part Strength1 retains a note whose negative delta is less than one clock");
    quantize.strength=0;quantized=sequence_quantize_note(axis,doc.length(),quantizeSource,quantize);
    require(quantized.time==1000&&quantized.duration==500,"Zero Strength preserves exact timing");
    quantize.strength=100;quantize.divisions=4;quantized=sequence_quantize_note(axis,doc.length(),quantizeSource,quantize);
    require(quantized.time==960&&quantized.duration==576,"Four divisions differ from the default two");
    quantize.divisions=24;quantized=sequence_quantize_note(axis,doc.length(),{15,1,17,60,96},quantize);
    require(quantized.time==0&&quantized.duration==1&&quantized.channel==17,"Fine divisions retain a minimum sounding duration and PChannel17");
    quantize.divisions=2;quantize.duration=false;
    require(sequence_quantize_note(axis,doc.length(),{192,100,0,60,96},quantize).time==384,"Integer halfway boundary uses the documented source forward-tie policy");
    auto quantizeMeter=doc;quantizeMeter.set_meter(1,3,8,3);quantize.divisions=5;
    require(sequence_quantize_note(quantizeMeter.timeline(),quantizeMeter.length(),{3152,100,0,60,96},quantize).time==3149,"Quantize follows the changed eighth-note beat with fractional division clocks");
    for(const auto invalid:std::vector<SequenceQuantizeOptions>{{true,false,0,100},{true,false,25,100},{true,false,2,101}}){bool failed=false;try{sequence_quantize_note(axis,doc.length(),quantizeSource,invalid);}catch(const std::runtime_error&){failed=true;}require(failed,"Quantize rejects invalid divisions or Strength");}
    bool boundaryRejected=false;try{sequence_quantize_note(axis,1000,{980,20,0,60,96},{true,false,2,100});}catch(const std::runtime_error&){boundaryRejected=true;}
    require(boundaryRejected,"Out-of-Segment quantization is rejected rather than silently clipping another field");
    auto quantizeDoc=SegmentDocument::create_empty({false,4,8000},L"Quantize atomic");quantizeDoc.add_tracks({SegmentTrackKind::sequence});quantizeDoc.add_note({100,50,17,60,90});quantizeDoc.add_note({200,60,18,64,100});quantizeDoc.add_tracks({SegmentTrackKind::sequence});quantizeDoc.select_track_group(1,0,0,1);quantizeDoc.add_note({300,70,0xfffffffbu,67,110});
    const auto quantizeInput=quantizeDoc.save_bytes();quantizeDoc.load(quantizeInput);std::vector<size_t> quantizeIndices{99};
    require(quantizeDoc.edit_note_timings({{1,0,0,300,80},{1,0,1,20,90},{1,1,0,400,100}},&quantizeIndices)&&quantizeIndices==std::vector<size_t>({1,0,0}),"Bulk timing uses original indexes across reorder and physical parts");
    const auto quantizeEdited=quantizeDoc.save_bytes();quantizeDoc.select_track_group(1,0,0,0);
    require(quantizeDoc.notes()[0].channel==18&&quantizeDoc.notes()[0].time==20&&quantizeDoc.notes()[1].channel==17&&quantizeDoc.notes()[1].time==300,"Reordered targets retain their pitch/channel identities");
    require(quantizeDoc.undo()&&quantizeDoc.save_bytes()==quantizeInput&&!quantizeDoc.dirty()&&!quantizeDoc.can_undo()&&quantizeDoc.can_redo(),"Multi-part quantization is one complete native-byte Undo");
    quantizeIndices={99};
    require(!quantizeDoc.edit_note_timings({{1,0,0,384,80},{1,1,0,-1,90}},&quantizeIndices)&&quantizeDoc.save_bytes()==quantizeInput&&!quantizeDoc.dirty()&&!quantizeDoc.can_undo()&&quantizeDoc.can_redo()&&quantizeIndices==std::vector<size_t>{99},"Late invalid timing preserves bytes dirty UndoRedo and caller selection mapping");
    require(!quantizeDoc.edit_note_timings({{1,0,0,384,80},{3,0,0,768,90}})&&quantizeDoc.save_bytes()==quantizeInput&&quantizeDoc.can_redo(),"Overlapping group masks cannot edit the same physical note twice");
    require(!quantizeDoc.edit_note_timings({{1,0,0,100,50}})&&quantizeDoc.save_bytes()==quantizeInput&&quantizeDoc.can_redo(),"Unchanged quantization preserves Redo instead of adding a history entry");
    require(quantizeDoc.redo()&&quantizeDoc.save_bytes()==quantizeEdited&&!quantizeDoc.can_redo(),"Bulk timing Redo restores exact bytes");
    Chunk quantizeEvents;quantizeEvents.id="evtl";quantizeEvents.data.resize(4+3*24);put32(quantizeEvents.data,0,24);
    for(size_t i=0;i<3;++i){const size_t at=4+i*24;put32(quantizeEvents.data,at,100+static_cast<unsigned>(i)*50);put32(quantizeEvents.data,at+4,50);put32(quantizeEvents.data,at+8,17);quantizeEvents.data[at+14]=i==1?0xb0:0x90;quantizeEvents.data[at+15]=static_cast<BYTE>(60+i);quantizeEvents.data[at+16]=90;quantizeEvents.data[at+20]=static_cast<BYTE>(0xa1+i);}
    quantizeEvents.data[16]=0xfc;quantizeEvents.data[17]=0xff;
    Chunk quantizeOpaque;quantizeOpaque.id="qopa";quantizeOpaque.data={3,1,4};quantizeOpaque.padding=0xa7;
    auto quantizePayload=quantizeEvents.encode();const auto opaqueEncoded=quantizeOpaque.encode();quantizePayload.insert(quantizePayload.end(),opaqueEncoded.begin(),opaqueEncoded.end());
    const auto quantizeRaw=sequence_change_timings(quantizePayload,{{0,300,90},{1,20,40}},&quantizeIndices);
    const auto quantizeNotes=sequence_notes(quantizeRaw);
    require(quantizeIndices==std::vector<size_t>({1,0})&&quantizeNotes[0].time==20&&quantizeNotes[1].time==300,"Low-level timing mapping counts sounding notes rather than intervening controllers");
    auto quantizeExpected=quantizeEvents;quantizeExpected.data.resize(4);const auto appendRecord=[&](size_t i,int time,int duration){Bytes record(quantizeEvents.data.begin()+4+i*24,quantizeEvents.data.begin()+4+(i+1)*24);put32(record,0,time);put32(record,4,duration);quantizeExpected.data.insert(quantizeExpected.data.end(),record.begin(),record.end());};
    appendRecord(2,20,40);appendRecord(1,150,50);appendRecord(0,304,90);auto quantizeExpectedBytes=quantizeExpected.encode();quantizeExpectedBytes.insert(quantizeExpectedBytes.end(),opaqueEncoded.begin(),opaqueEncoded.end());
    require(quantizeRaw==quantizeExpectedBytes,"Bulk timing retains signed offsets controller bytes record extensions opaque chunks and odd padding exactly");
    const Note propertyNote{2784,576,17,61,80};
    const auto start=sequence_property_position(axis,2784),end=sequence_property_position(axis,3360),duration=sequence_property_duration(axis,propertyNote);
    require(start.measure==0&&start.beat==3&&start.grid==3&&start.tick==-96&&end.measure==1&&end.beat==0&&end.grid==2&&end.tick==-96,"Sequence Properties matches observed original signed-tick Start and End");
    require(sequence_property_clocks(axis,start)==2784&&sequence_property_clocks(axis,end)==3360&&duration.measure==0&&duration.beat==0&&duration.grid==3&&duration.tick==0,"Signed position roundtrip and original three-grid duration retain exact clocks");
    const auto movedProperty=sequence_property_edit(axis,doc.length(),propertyNote,6,-48);
    require(movedProperty.time==2832&&movedProperty.duration==576&&movedProperty.channel==17,"Start tick edit retains duration and DWORD PChannel");
    require(sequence_property_edit(axis,doc.length(),propertyNote,10,0).duration==672,"End tick edit changes duration from the unchanged start");
    require(sequence_property_edit(axis,doc.length(),propertyNote,13,2).duration==384,"Duration grid edit changes length without snapping start");
    require(sequence_property_edit(axis,doc.length(),propertyNote,2,1).velocity==1&&sequence_property_edit(axis,doc.length(),propertyNote,2,127).velocity==127,"Properties accepts both sounding velocity endpoints");
    require(sequence_property_edit(axis,doc.length(),propertyNote,1,10).pitch==121,"Properties octave10 retains pitch class and allows valid high notes");
    auto propertyDoc=SegmentDocument::create_empty({false,4,8000},L"Properties");propertyDoc.add_tracks({SegmentTrackKind::sequence});propertyDoc.add_note(propertyNote);propertyDoc.add_note({3264,192,19,62,96});const auto propertyInput=propertyDoc.save_bytes();propertyDoc.load(propertyInput);
    size_t propertySelected=SIZE_MAX;
    require(propertyDoc.edit_note(0,sequence_property_edit(propertyDoc.timeline(),propertyDoc.length(),propertyNote,3,2),&propertySelected)&&propertySelected==1&&propertyDoc.notes()[1].channel==17,"Properties time reorder publishes the same owned note index on PChannel17");
    const auto propertyEdited=propertyDoc.save_bytes();require(propertyDoc.undo()&&propertyDoc.save_bytes()==propertyInput&&!propertyDoc.can_undo()&&propertyDoc.can_redo(),"Properties field commit has one atomic native-byte Undo");
    for(const auto invalid:std::vector<std::pair<unsigned,std::int64_t>>{{0,12},{1,11},{2,0},{2,128},{3,0},{4,5},{5,5},{6,INT64_MAX},{7,1},{13,0}}){
        bool rejected=false;try{const auto value=sequence_property_edit(propertyDoc.timeline(),propertyDoc.length(),propertyNote,invalid.first,invalid.second);propertyDoc.edit_note(0,value);}catch(const std::runtime_error&){rejected=true;}
        require(rejected&&propertyDoc.save_bytes()==propertyInput&&!propertyDoc.dirty()&&!propertyDoc.can_undo()&&propertyDoc.can_redo(),"Invalid Properties field preserves bytes dirty state and UndoRedo");
    }
    require(propertyDoc.redo()&&propertyDoc.save_bytes()==propertyEdited&&!propertyDoc.can_redo(),"Properties Redo restores the same reordered native bytes");
    const Note propertyOther{3264,192,19,62,96};const auto propertyMoved=sequence_property_edit(axis,doc.length(),propertyNote,3,2);
    const std::vector<Note> historyBefore{propertyNote,propertyOther},historyAfter{propertyOther,propertyMoved};
    require(sequence_property_history_selection(historyBefore,historyAfter,0)==std::optional<size_t>(1)&&sequence_property_history_selection(historyAfter,historyBefore,1)==std::optional<size_t>(0),"Properties selection follows the same note across time reorder Redo and Undo");
    auto velocityChanged=historyBefore;velocityChanged[0].velocity=72;
    require(sequence_property_history_selection(historyBefore,velocityChanged,0)==std::optional<size_t>(0),"Single-note velocity history retains its selected record");
    auto otherChanged=historyBefore;otherChanged[1].pitch=70;
    require(sequence_property_history_selection(historyBefore,otherChanged,0)==std::optional<size_t>(0),"Unrelated history keeps the unique unchanged selected note");
    auto bulkChanged=velocityChanged;bulkChanged[1].velocity=72;
    require(!sequence_property_history_selection(historyBefore,bulkChanged,0),"Bulk replacement cannot silently reassign Properties");
    require(!sequence_property_history_selection({propertyNote,propertyNote},{propertyNote,propertyMoved},0),"Identical pre-edit notes require reselection instead of an arbitrary mapping");
    require(!sequence_property_history_selection(historyBefore,{propertyOther,propertyOther},0),"Replacement identical to another note cannot pick either duplicate");
    require(!sequence_property_history_selection(historyBefore,{propertyOther},0)&&!sequence_property_history_selection(historyBefore,historyAfter,99),"Deleted or stale selected notes clear safely");
    require(sequence_snap_clock(axis,233,doc.length(),SequenceSnap::none)==233&&sequence_snap_clock(axis,233,doc.length(),SequenceSnap::grid)==192,"None preserves exact insertion time; Grid floors to an actual grid");
    require(sequence_snap_clock(axis,1555,doc.length(),SequenceSnap::beat)==1536&&sequence_snap_clock(axis,1555,doc.length(),SequenceSnap::bar)==0,"Beat and Bar insertion snap are distinct");
    SequenceCursor marker{192,60,SequenceSnap::grid};require(move_sequence_cursor(axis,doc.length(),marker,1,1)&&marker.clock==384&&marker.pitch==61,"Arrow keys move the view marker by one grid and semitone");
    marker.snap=SequenceSnap::none;require(!move_sequence_cursor(axis,doc.length(),marker,1,0)&&marker.clock==384,"None disables horizontal arrow movement");
    require(move_sequence_cursor(axis,doc.length(),marker,0,-1)&&marker.pitch==60,"None retains vertical arrow movement");
    marker={4327,60,SequenceSnap::grid};
    require(move_sequence_cursor(axis,doc.length(),marker,1,0)&&marker.clock==4519&&move_sequence_cursor(axis,doc.length(),marker,-1,0)&&marker.clock==4327,"Grid arrows preserve a non-grid marker offset after switching from None, as observed in original");
    marker={0,0,SequenceSnap::grid};require(!move_sequence_cursor(axis,doc.length(),marker,-1,-1)&&marker.clock==0&&marker.pitch==0,"Marker respects timeline and MIDI lower limits");
    marker={doc.length()-192,127,SequenceSnap::grid};require(!move_sequence_cursor(axis,doc.length(),marker,1,1)&&marker.clock==doc.length()-192&&marker.pitch==127,"Marker does not enter the excluded Segment endpoint");
    require(sequence_default_duration(axis,0,SequenceSnap::grid)==192&&sequence_default_duration(axis,0,SequenceSnap::beat)==768&&sequence_default_duration(axis,0,SequenceSnap::bar)==3072,"Default insertion duration follows snap division");
    doc.set_meter(1,3,8,3);require(sequence_grid_span(doc.timeline(),3072)==128&&sequence_default_duration(doc.timeline(),3072,SequenceSnap::bar)==1152,"Sequence grid and bar duration follow meter changes");
    doc.load(input);const auto part=sequence_part_info(doc,0);require(part.name==L"Sequence"&&part.channels==std::vector<std::uint32_t>{0},"Empty initial Sequence shows part on PChannel1");
    require(doc.save_bytes()==input&&!doc.dirty()&&!doc.can_undo(),"Reading part and marker information has no document edit");
    const auto current=current_sequence_band(doc,0);require(current&&current->track==0&&current->event==0&&current->value.physicalTime==-1,"Current Band includes its physical beat-minus-one event");
    require(!sequence_patch(doc,0,0),"An inactive default PChannel has no valid instrument assignment");
    require(assign_sequence_gm_patch(doc,0,0,0),"Insert New Instrument enables Piano1 in the owned Band");
    auto activeBand=doc.band_events().front();BandDocument active;active.load(activeBand.band);const auto channels=active.instruments();
    require(channels[0].patch==0&&(channels[0].flags&3)==3&&!(channels[0].flags&96),"Patch assignment leaves pan/volume validity disabled");
    BandDocument beforeBand;beforeBand.load(current->value.band);require(channels[1].patch==beforeBand.instruments()[1].patch&&channels[1].flags==beforeBand.instruments()[1].flags,"Patch assignment leaves other PChannels unchanged");
    const auto piano=doc.save_bytes();require(!assign_sequence_gm_patch(doc,0,0,0)&&doc.save_bytes()==piano,"Unchanged instrument assignment has no additional history entry");
    require(doc.undo()&&doc.save_bytes()==input&&!doc.can_undo()&&doc.redo()&&doc.save_bytes()==piano,"Insert instrument is exactly one atomic Segment UndoRedo transaction");
    require(assign_sequence_gm_patch(doc,0,0,16)&&sequence_patch(doc,0,0)->patch==16,"Organ1 updates the owned Band patch rather than just a label");
    auto tree=Chunk::parse(doc.save_bytes());Chunk opaque;opaque.id="zseq";opaque.data={1,2,3};opaque.padding=0xa5;tree.children.push_back(opaque);const auto beforeNotes=tree.encode();doc.load(beforeNotes);
    size_t selected=SIZE_MAX;require(doc.add_note({192,192,0,60,96},&selected)&&selected==0,"Insertion publishes the actual sounding-note index");
    require(doc.add_note({0,192,0,62,88},&selected)&&selected==0,"Insertion index follows raw order before existing notes");
    require(doc.add_note({192,192,0,60,96},&selected)&&selected==2,"Duplicate-time insertion selects the newly appended note");
    selected=88;require(!doc.add_note({doc.length()-1,192,0,60,96},&selected)&&selected==88,"Rejected insertion leaves the selection result unpublished");
    require(Chunk::parse(doc.save_bytes()).find("zseq")->encode()==opaque.encode(),"Sequence edits preserve unrelated odd chunk and padding");
    const auto source=doc.save_bytes();const auto previewBytes=sequence_audition_snapshot(doc,{192,192,0,60,43});SegmentDocument preview;preview.load(previewBytes);BandDocument previewBand;previewBand.load(preview.band_events().front().band);
    require(preview.notes().size()==1&&preview.notes()[0].time==0&&preview.notes()[0].duration==384&&preview.notes()[0].velocity==43&&previewBand.instruments()[0].patch==16,"Audition contains only requested pitch/velocity and effective Organ patch");
    require(doc.save_bytes()==source,"Audition snapshot does not edit source notes or history");
    require(preview.tracks().size()==3&&preview.tempos().empty(),"Audition excludes all unrelated source tracks and tempo events");
    BandDocument earlier;earlier.add_gm_instrument(16,0,31,77);BandDocument later=BandDocument::inactive_default();doc.set_band(0,earlier.save_bytes());doc.set_band(1536,later.save_bytes());const auto inheritedSource=doc.save_bytes();
    require(sequence_patch(doc,2000,0)->patch==16,"Disabled later instrument change retains previous patch");
    preview.load(sequence_audition_snapshot(doc,{2000,48,0,67,100}));previewBand.load(preview.band_events().front().band);const auto inherited=previewBand.instruments().front();
    require(inherited.patch==16&&inherited.pan==31&&inherited.volume==77&&doc.save_bytes()==inheritedSource,"Audition retains independently valid earlier patch pan and volume");
    require(earlier.set_collection_reference(0,{L"Tone.dls",{}}),"Create declared DLS preview dependency");doc.set_band(0,earlier.save_bytes());preview.load(sequence_audition_snapshot(doc,{0,48,0,67,100}));previewBand.load(preview.band_events().front().band);
    require(collection_reference(previewBand.instruments().front().collectionReference).filename==L"Tone.dls","Audition retains the effective DLS descriptor");
    require(assign_sequence_gm_patch(doc,0,0,16)&&sequence_patch(doc,0,0)->collectionReference.empty(),"Selecting GM explicitly replaces only the selected PChannel DLS assignment");
    const auto& catalog=gm_instruments();require(std::count_if(catalog.begin(),catalog.end(),[](const auto& x){return !x.percussion&&(x.patch&0x7f7f00u)==0;})==128,"GM catalog contains all128 base programs");
    require(gm_instrument_name(0)==L"Piano 1"&&gm_instrument_name(16)==L"Organ 1"&&gm_instrument_name(0x80000)==L"Piano 1w","GM and GS bank-program names are distinct");
    std::vector<std::uint32_t> patches;for(const auto& x:catalog)patches.push_back(x.patch);std::sort(patches.begin(),patches.end());require(std::adjacent_find(patches.begin(),patches.end())==patches.end(),"Catalog does not alias GS or drum patches");
    write_file_atomic((base/L"owned.sgp").wstring(),doc.save_bytes());SegmentDocument restored;restored.load(read_file((base/L"owned.sgp").wstring()));require(restored.save_bytes()==doc.save_bytes()&&restored.notes().size()==3&&sequence_patch(restored,0,0)->patch==16,"Separate document reload retains notes and actual Band assignment");
    auto multiple=SegmentDocument::create_empty({false,4,8000},L"Parts");multiple.add_tracks({SegmentTrackKind::sequence});multiple.add_note({0,192,3,60,96});multiple.add_tracks({SegmentTrackKind::sequence});multiple.select_track_group(1,0,0,1);multiple.add_note({0,192,7,64,96});
    require(sequence_part_info(multiple,0).channels==std::vector<std::uint32_t>{3}&&sequence_part_info(multiple,1).channels==std::vector<std::uint32_t>{7},"Physical Sequence parts retain their distinct PChannels");
    const Note origin{1000,500,17,60,96};
    auto drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::start,-100,0,0,SequenceSnap::grid);
    require(drag.time==768&&drag.duration==732&&drag.channel==17&&drag.pitch==60,"Start edge snaps but preserves the original end and PChannel17");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::start,-100,0,0,SequenceSnap::none);
    require(drag.time==900&&drag.duration==600,"None start edge retains exact ticks");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::end,37,0,0,SequenceSnap::grid);
    require(drag.time==1000&&drag.duration==537,"End edge is free even under Grid, per user observation");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::end,-9999,0,0,SequenceSnap::grid);
    require(drag.duration==1,"End edge cannot invert or erase the note");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::end,INT64_MAX,0,0,SequenceSnap::none);
    require(drag.duration==2072,"End edge safely clamps extreme deltas to the Segment end");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::move,250,3,0,SequenceSnap::grid);
    require(drag.time==1152&&drag.pitch==63&&drag.duration==500&&drag.velocity==96,"Body move snaps time, transposes pitch and preserves length/velocity");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::move,250,3,0,SequenceSnap::none);
    require(drag.time==1250&&drag.pitch==63,"None body move has exact time and semitone pitch");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::move,-99999,-500,0,SequenceSnap::grid);
    require(drag.time==0&&drag.pitch==0&&drag.channel==17,"Body lower boundaries retain DWORD PChannel");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::move,99999,500,0,SequenceSnap::none);
    require(drag.time==2572&&drag.pitch==127&&drag.duration==500,"Body upper boundaries preserve duration");
    drag=sequence_drag_note(axis,3072,origin,SequenceNoteHandle::velocity,0,0,5,SequenceSnap::grid);
    require(drag.velocity==101&&drag.time==1000&&drag.pitch==60&&drag.duration==500,"Velocity drag changes only velocity in five-unit increments");
    require(sequence_drag_note(axis,3072,origin,SequenceNoteHandle::velocity,0,0,1000,SequenceSnap::grid).velocity==127&&sequence_drag_note(axis,3072,origin,SequenceNoteHandle::velocity,0,0,-1000,SequenceSnap::grid).velocity==1,"Velocity endpoints stay sounding1 through127");
    SequenceVelocityOptions linear;linear.percent=false;linear.start=10;linear.end=-20;
    require(sequence_velocity_value(origin,0,2000,linear)==91,"Linear velocity ramp interpolates at event time");
    SequenceVelocityOptions percent;percent.start=percent.end=-50;
    require(sequence_velocity_value(origin,0,2000,percent)==48,"Percent offset scales source velocity");
    SequenceVelocityOptions limit;limit.limit=true;limit.minimum=limit.maximum=80;
    require(sequence_velocity_value(origin,0,2000,limit)==80,"Equal Min and Max supplies an exact numeric velocity");
    require(sequence_velocity_value(origin,1000,1000,linear)==106,"Single-time target uses Start without division by zero");
    limit.minimum=100;limit.maximum=20;bool badLimit=false;try{sequence_velocity_value(origin,0,2000,limit);}catch(const std::runtime_error&){badLimit=true;}require(badLimit,"Invalid range is rejected rather than reordered");
    const auto multiInput=multiple.save_bytes();multiple.load(multiInput);
    require(multiple.edit_note_velocities({{1,0,0,1},{1,1,0,127}}),"One velocity command updates two physical parts");
    const auto multiEdited=multiple.save_bytes();multiple.select_track_group(1,0,0,0);require(multiple.notes()[0].velocity==1&&multiple.notes()[0].channel==3,"First part retains its PChannel and low velocity");
    multiple.select_track_group(1,0,0,1);require(multiple.notes()[0].velocity==127&&multiple.notes()[0].channel==7,"Second part retains its PChannel and high velocity");
    require(multiple.undo()&&multiple.save_bytes()==multiInput&&!multiple.can_undo()&&multiple.can_redo(),"Multi-part velocity command is one atomic Undo");
    require(!multiple.edit_note_velocities({{1,0,0,120},{1,1,0,0}})&&multiple.save_bytes()==multiInput&&!multiple.dirty()&&!multiple.can_undo()&&multiple.can_redo(),"Late invalid velocity leaves bytes/dirty/Undo/Redo unchanged");
    require(!multiple.edit_note_velocities({{1,0,0,120},{1,0,0,100}})&&multiple.save_bytes()==multiInput&&multiple.can_redo(),"Duplicate target cannot partially apply a velocity command");
    require(multiple.redo()&&multiple.save_bytes()==multiEdited&&!multiple.can_redo(),"Multi-part velocity Redo restores exact native bytes");
    require(!multiple.edit_note_velocities({{1,0,0,1},{1,1,0,127}})&&multiple.save_bytes()==multiEdited,"Unchanged numeric velocity adds no edit");
    multiple.undo();const auto gestureInput=multiple.save_bytes();multiple.select_track_group(1,0,0,0);const auto oldNote=multiple.notes()[0];
    const auto interim=sequence_drag_note(multiple.timeline(),multiple.length(),oldNote,SequenceNoteHandle::move,96,1,0,SequenceSnap::none);
    const auto finalNote=sequence_drag_note(multiple.timeline(),multiple.length(),oldNote,SequenceNoteHandle::move,384,2,0,SequenceSnap::grid);
    require(multiple.save_bytes()==gestureInput&&!multiple.dirty()&&!multiple.can_undo(),"Drag previews and cancellation never mutate the owner history");
    size_t resultIndex=SIZE_MAX;require(multiple.edit_note(0,finalNote,&resultIndex)&&resultIndex==0&&multiple.undo()&&multiple.save_bytes()==gestureInput&&!multiple.can_undo(),"Final drag commits as one Undo despite multiple preview positions");
    require(interim.time==96&&interim.pitch==61,"Preview remains available for rendering/audition before commit");
}
