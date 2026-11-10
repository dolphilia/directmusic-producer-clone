// Identity-pinned, read-only extraction of TimeSigStripMgr's COM tables and IDs.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.join(repo,'work/analysis/pe/app__TimeSigStripMgr.dll');
const pe=JSON.parse(fs.readFileSync(path.join(directory,'pe.json'),'utf8'));
const bytes=fs.readFileSync(path.join(repo,pe.input));
const sha256=crypto.createHash('sha256').update(bytes).digest('hex');
if(sha256!=='898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7'||pe.sha256!==sha256||pe.imageBase!=='0x400000')throw Error('TimeSig original identity mismatch');
const sdkPath='work/analysis/sources/dmusici.h';
const sdkBytes=fs.readFileSync(path.join(repo,sdkPath));
const sdkSha256=crypto.createHash('sha256').update(sdkBytes).digest('hex');
if(sdkSha256!=='4801df72813ff1aa5a7609c0efb78b888b2f064c962051eb900198cad974a796')throw Error('SDK reference identity mismatch');
const sdk=sdkBytes.toString('utf8');
if(!/DEFINE_GUID\(GUID_IDirectMusicStyle,\s*0xd2ac28a1/i.test(sdk))throw Error('Style parameter GUID mismatch');
const styleInterface=sdk.match(/DECLARE_INTERFACE_\(IDirectMusicStyle, IUnknown\)([\s\S]*?)\n};/);
const styleMethods=styleInterface?.[1].match(/STDMETHOD(?:_\([^,]+,|\()\s*\w+/g);
if(styleMethods?.findIndex(method=>method.includes('GetTimeSignature'))!==11)throw Error('Style signature slot mismatch');
const offset=rva=>{const s=pe.sections.find(s=>rva>=s.rva&&rva<s.rva+s.rawSize);if(!s)throw Error('Unmapped RVA');return s.rawOffset+rva-s.rva;};
const hex=(value,n)=>value.toString(16).padStart(n,'0');
const guid=rva=>{const o=offset(rva);return `${hex(bytes.readUInt32LE(o),8)}-${hex(bytes.readUInt16LE(o+4),4)}-${hex(bytes.readUInt16LE(o+6),4)}-${bytes.subarray(o+8,o+10).toString('hex')}-${bytes.subarray(o+10,o+16).toString('hex')}`;};
const tables=[
  {name:'StripManager',rva:0x24a4,targets:[0xc619,0xc623,0xd077,0x66da,0x64e6,0x6688,0x7575,0x673e,0x7620]},
  {name:'IPersistStream',rva:0x2480,targets:[0xc62d,0xc637,0xd081,0x6881,0x68be,0x7cc6,0x68e9,0xf7fd]},
  {name:'TimelineEdit',rva:0x20b4,targets:[0xab4f,0x8dd6,0xab59,0x9939,0x99a5,0x9be9,0x9e55,0x9fe6,0xa095,0xa0fe,0xa1c3,0xa279,0xa397,0xa3f9,0xa449]},
  {name:'Strip',rva:0x210c,targets:[0x8e8d,0x887c,0x8fa2,0x8fba,0x9593,0x9715,0xaf98]},
];
for(const table of tables)for(const [slot,target] of table.targets.entries())if(bytes.readUInt32LE(offset(table.rva+slot*4))!==0x400000+target)throw Error(`${table.name} slot ${slot} changed`);
const instructions=[
  {role:'borrowed_store_without_addref',rva:0x66be,hex:'8b4d0889415033c0'},
  {role:'borrowed_get_addref',rva:0x65f6,hex:'8b0353ff5004'},
  {role:'track_header_group_input',rva:0x76c6,hex:'8b40148b4d08894140'},
  {role:'track_flags_mask',rva:0x769c,hex:'2538000300'},
  {role:'track_header_list_type',rva:0x680f,hex:'c7401c54494d53'},
  {role:'initial_group_bits',rva:0x7478,hex:'c7464401000000'},
  {role:'initial_track_flags',rva:0x747f,hex:'c7464838000000'},
  {role:'initial_borrowed_slot',rva:0x746f,hex:'897e54'},
  {role:'initial_undo_resource',rva:0x8e75,hex:'894864'},
  {role:'framework_release_and_clear',rva:0x76e7,hex:'8b0850ff5108891e'},
  {role:'framework_query_interface',rva:0x76fa,hex:'8b085668e429400050ff11'},
  {role:'runtime_release',rva:0x771d,hex:'8b0850ff5108'},
  {role:'runtime_query_interface',rva:0x772a,hex:'8b0856689421400050ff11'},
  {role:'runtime_query_result_ignored',rva:0x7735,hex:'e92e010000'},
  {role:'service_get_canonical_unknown',rva:0x6828,hex:'8b1183c0085068942a400051ff12'},
  {role:'framework_destructor_release',rva:0x750d,hex:'ff5108'},
  {role:'runtime_destructor_release',rva:0x752b,hex:'ff5108'},
  {role:'timeline_remove_manager_page',rva:0x7775,hex:'ff504c'},
  {role:'timeline_remove_meter_notification',rva:0x778c,hex:'ff91a4000000'},
  {role:'timeline_remove_style_notification',rva:0x77a0,hex:'ff91a4000000'},
  {role:'timeline_remove_position_notification',rva:0x77b4,hex:'ff91a4000000'},
  {role:'timeline_remove_strip',rva:0x77d5,hex:'ff9180000000'},
  {role:'timeline_release',rva:0x77e0,hex:'ff5108'},
  {role:'timeline_query_interface',rva:0x77f0,hex:'68a429400050ff1185c0'},
  {role:'timeline_insert_strip',rva:0x7815,hex:'ff9190000000'},
  {role:'timeline_add_position_notification',rva:0x7832,hex:'ff91a0000000'},
  {role:'timeline_add_style_notification',rva:0x7846,hex:'ff91a0000000'},
  {role:'timeline_add_meter_notification',rva:0x785a,hex:'ff90a0000000'},
  {role:'timeline_find_style_manager',rva:0x723c,hex:'ff918c000000'},
  {role:'timeline_get_direct_addref',rva:0x6845,hex:'8b4d0866c7000d008b49103bcb740d8948088b0151ff5004'},
  {role:'strip_resizable_bool_one',rva:0x9608,hex:'66c7060b0066c746080100'},
  {role:'strip_height_twenty',rva:0x9702,hex:'66c7061600c7460814000000'},
];
for(const instruction of instructions){const at=offset(instruction.rva);if(bytes.subarray(at,at+instruction.hex.length/2).toString('hex')!==instruction.hex)throw Error('Instruction changed: '+instruction.role);}
const ids=Object.fromEntries(Object.entries({timeSignatureParam:0x2174,styleParam:0x2184,refreshPositions:0x2a64,runtimeTrackClass:0x21a4,runtimeTrackInterface:0x2194,frameworkInterface:0x29e4,timelineInterface:0x29a4,canonicalUnknown:0x2a94,defaultStreamFormat:0x2a14,alternateStreamFormat:0x2a24,undoLabelParam:0x2a44,borrowedObjectParam:0x2a54,persistClass:0x2a84}).map(([name,rva])=>[name,{rva:'0x'+rva.toString(16),guid:guid(rva)}]));
const evidence={schema:5,input:pe.input,sha256,sdkReference:{path:sdkPath,sha256:sdkSha256,styleTimeSignatureSlot:11},tables:tables.map(t=>({...t,rva:'0x'+t.rva.toString(16),targets:t.targets.map(r=>'0x'+r.toString(16))})),ids,
  instructions:instructions.map(i=>({...i,rva:'0x'+i.rva.toString(16),verified:true})),
  observedStatically:['GetParam null output returns E_POINTER; empty meter map returns 0x88781161.',
    'Load reads tims records containing LONG clock, BYTE beats, BYTE denominator, WORD grids.',
    'Load replaces zero beats/denominator/grids with 4/2/2 before insertion.',
    'Save emits LIST:TIMS with tims record size 8; nonempty map uses a measure-to-clock conversion.',
    'GetSizeMax returns E_NOTIMPL.',
    'GetStripMgrProperty(3) writes 32-byte runtime track header; Set property 3 consumes group bits only.',
    'Set property 4 masks bits with 0x30038; property 5 stores unmasked producer flags.',
    'Borrowed-object SetParam stores input without AddRef, GetParam adds a caller reference. Destructor has no release for this slot.',
    'GUID at 0x2a44 selects an undo resource label, not the strip name.',
    'Property 1 replaces an owned DirectMusicTrack interface, ignores failed QI and returns S_OK.',
    'Property 2 replaces an owned Framework interface and returns its QI HRESULT.',
    'Both service getters query canonical IUnknown; destructor releases the stored service pointers.',
    'Timeline getter returns the stored Timeline interface with AddRef, without canonical-IUnknown QI.',
    'Timeline connection inserts the strip, registers position/Style/meter notifications, and looks up a Style manager. SDK identifies d2ac28a1 as GUID_IDirectMusicStyle.',
    'Timeline disconnect removes manager page, meter/Style/position notifications, strip page and strip, then releases Timeline.',
    'Strip getters provide group-prefixed TimeSig title, BOOL 1, fixed height 20, and canonical manager identity.'],
  limitations:['Static ABI only; dynamic inputs and output comparison required','Style time-signature import, actual Timeline, UI, editing and playback not established for a replacement TimeSig module']};
fs.writeFileSync(path.join(directory,'time-signature-abi.json'),JSON.stringify(evidence,null,2)+'\n');
console.log(JSON.stringify(evidence));
