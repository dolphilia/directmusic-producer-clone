#include "sequence.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace producer::app {
namespace {
struct Part {std::string id;Bytes bytes;};
std::vector<Part> parts(const Bytes& payload) {
    std::vector<Part> result;size_t at=0;
    while(at<payload.size()){
        if(payload.size()-at<8)throw std::runtime_error("Truncated Sequence subchunk");
        const auto n=read32(payload,at+4);if(n>payload.size()-at-8)throw std::runtime_error("Sequence subchunk boundary");
        const auto end=at+8+n+(n&1);if(end>payload.size())throw std::runtime_error("Sequence padding missing");
        result.push_back({std::string(reinterpret_cast<const char*>(payload.data()+at),4),Bytes(payload.begin()+at,payload.begin()+end)});at=end;
    }
    return result;
}
size_t events_part(const std::vector<Part>& p) {
    size_t index=SIZE_MAX;for(size_t i=0;i<p.size();++i)if(p[i].id=="evtl"){if(index!=SIZE_MAX)throw std::runtime_error("Ambiguous Sequence events");index=i;}
    if(index==SIZE_MAX)throw std::runtime_error("Sequence event chunk missing");return index;
}
DWORD stride(const Part& part) {const auto count=read32(part.bytes,4);if(count<4)throw std::runtime_error("Sequence record size missing");const auto size=read32(part.bytes,8);if(size<20||size>4096||size%4||(count-4)%size)throw std::runtime_error("Unsupported Sequence record size");return size;}
size_t note_record(const Part& part,DWORD size,size_t index){
    for(size_t at=12;at<8+read32(part.bytes,4);at+=size)if((part.bytes[at+14]&0xf0)==0x90&&part.bytes[at+16]){if(!index)return at;--index;}
    throw std::out_of_range("Sequence note index");
}
Bytes join(const std::vector<Part>& p){Bytes result;for(const auto& item:p)result.insert(result.end(),item.bytes.begin(),item.bytes.end());return result;}
Chunk leaf(const char* id,Bytes payload){Chunk c;c.id=id;c.data=std::move(payload);return c;}
Chunk container(const char* id,const char* type,std::vector<Chunk> children){Chunk c;c.id=id;c.type=type;c.children=std::move(children);return c;}
Bytes track_header(DWORD clsid,const char* chunk,const char* form=nullptr){Bytes b(32);const GUID guid={clsid,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};std::memcpy(b.data(),&guid,16);put32(b,20,1);if(chunk)std::memcpy(b.data()+24,chunk,4);if(form)std::memcpy(b.data()+28,form,4);return b;}
}
bool is_sequence_track(const Chunk& track) {
    const auto h=track.find("trkh");const auto expected=track_header(0xd2ac2886,"seqt");return track.id=="RIFF"&&track.type=="DMTK"&&h&&h->data.size()>=32&&std::equal(expected.begin(),expected.begin()+16,h->data.begin());
}
std::vector<Note> sequence_notes(const Bytes& payload) {
    const auto p=parts(payload);const auto& part=p.at(events_part(p));const auto size=stride(part);std::vector<Note> notes;
    for(size_t at=12;at<8+read32(part.bytes,4);at+=size)if((part.bytes[at+14]&0xf0)==0x90&&part.bytes[at+16]){
        const auto offset=static_cast<short>(part.bytes[at+12]|(part.bytes[at+13]<<8));
        const auto time=std::int64_t(static_cast<std::int32_t>(read32(part.bytes,at)))+offset;
        if(time<INT32_MIN||time>INT32_MAX)throw std::runtime_error("Sequence time overflow");
        notes.push_back({static_cast<std::int32_t>(time),static_cast<std::int32_t>(read32(part.bytes,at+4)),read32(part.bytes,at+8),part.bytes[at+15],part.bytes[at+16]});
    }
    return notes;
}
Bytes sequence_insert(const Bytes& payload,Note note,size_t* resultingIndex) {
    if(note.time<0||note.duration<=0||note.pitch>127||!note.velocity||note.velocity>127||note.channel>=0xfffffffcu)throw std::runtime_error("Invalid Sequence note");
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);Bytes record(size);
    put32(record,0,note.time);put32(record,4,note.duration);put32(record,8,note.channel);record[14]=0x90;record[15]=note.pitch;record[16]=note.velocity;
    size_t insert=12;const auto end=8+read32(part.bytes,4);
    for(;insert<end;insert+=size)if(static_cast<std::int32_t>(read32(part.bytes,insert))>note.time)break;
    size_t selected=0;for(size_t at=12;at<insert;at+=size)if((part.bytes[at+14]&0xf0)==0x90&&part.bytes[at+16])++selected;
    part.bytes.insert(part.bytes.begin()+insert,record.begin(),record.end());put32(part.bytes,4,read32(part.bytes,4)+size);
    Bytes result;for(const auto& item:p)result.insert(result.end(),item.bytes.begin(),item.bytes.end());if(resultingIndex)*resultingIndex=selected;return result;
}
Bytes sequence_change(const Bytes& payload,size_t index,Note note,size_t* resultingIndex){
    if(note.time<0||note.duration<=0||note.pitch>127||!note.velocity||note.velocity>127||note.channel>=0xfffffffcu)throw std::runtime_error("Invalid Sequence note");
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);const auto at=note_record(part,size,index);
    Bytes record(part.bytes.begin()+at,part.bytes.begin()+at+size);
    const auto oldTime=static_cast<std::int32_t>(read32(record,0));const auto offset=static_cast<short>(record[12]|(record[13]<<8));
    const auto rawTime=std::int64_t(note.time)-offset;if(rawTime<INT32_MIN||rawTime>INT32_MAX)throw std::runtime_error("Sequence raw time overflow");
    put32(record,0,static_cast<std::uint32_t>(rawTime));put32(record,4,note.duration);put32(record,8,note.channel);record[15]=note.pitch;record[16]=note.velocity;
    size_t resultAt=at;if(rawTime==oldTime)std::copy(record.begin(),record.end(),part.bytes.begin()+at);
    else{part.bytes.erase(part.bytes.begin()+at,part.bytes.begin()+at+size);size_t insert=12;const auto end=8+read32(part.bytes,4)-size;for(;insert<end;insert+=size)if(static_cast<std::int32_t>(read32(part.bytes,insert))>rawTime)break;part.bytes.insert(part.bytes.begin()+insert,record.begin(),record.end());resultAt=insert;}
    if(resultingIndex){size_t count=0;for(size_t pos=12;pos<resultAt;pos+=size)if((part.bytes[pos+14]&0xf0)==0x90&&part.bytes[pos+16])++count;*resultingIndex=count;}
    return join(p);
}
Bytes sequence_change_timings(const Bytes& payload,const std::vector<SequenceTimingChange>& changes,std::vector<size_t>* resultingIndices){
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);
    struct Record {Bytes bytes;size_t note;};std::vector<Record> records;size_t noteIndex=0;
    for(size_t at=12;at<8+read32(part.bytes,4);at+=size){const bool sounding=(part.bytes[at+14]&0xf0)==0x90&&part.bytes[at+16];records.push_back({Bytes(part.bytes.begin()+at,part.bytes.begin()+at+size),sounding?noteIndex++:SIZE_MAX});}
    bool moved=false;
    for(size_t i=0;i<changes.size();++i){const auto& e=changes[i];
        if(e.note>=noteIndex||e.time<0||e.duration<=0)throw std::runtime_error("Invalid Sequence timing target");
        for(size_t j=0;j<i;++j)if(changes[j].note==e.note)throw std::runtime_error("Duplicate Sequence timing target");
        auto& record=std::find_if(records.begin(),records.end(),[&](const Record& r){return r.note==e.note;})->bytes;
        const auto offset=static_cast<short>(record[12]|(record[13]<<8));const auto raw=std::int64_t(e.time)-offset;
        if(raw<INT32_MIN||raw>INT32_MAX)throw std::runtime_error("Sequence raw time overflow");
        moved|=static_cast<std::int32_t>(read32(record,0))!=raw;
        put32(record,0,static_cast<std::uint32_t>(raw));put32(record,4,e.duration);
    }
    if(moved)std::stable_sort(records.begin(),records.end(),[](const Record& a,const Record& b){return static_cast<std::int32_t>(read32(a.bytes,0))<static_cast<std::int32_t>(read32(b.bytes,0));});
    std::vector<size_t> mapping(noteIndex);size_t after=0;part.bytes.resize(12);
    for(const auto& r:records){if(r.note!=SIZE_MAX)mapping[r.note]=after++;part.bytes.insert(part.bytes.end(),r.bytes.begin(),r.bytes.end());}
    auto result=join(p);if(resultingIndices){std::vector<size_t> indices;for(const auto& e:changes)indices.push_back(mapping[e.note]);*resultingIndices=std::move(indices);}return result;
}
Bytes sequence_delete(const Bytes& payload,size_t index){
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);const auto at=note_record(part,size,index);
    part.bytes.erase(part.bytes.begin()+at,part.bytes.begin()+at+size);put32(part.bytes,4,read32(part.bytes,4)-size);return join(p);
}
namespace {
size_t curves_part(const std::vector<Part>& parts){
    size_t found=SIZE_MAX;for(size_t i=0;i<parts.size();++i)if(parts[i].id=="curl"){if(found!=SIZE_MAX)throw std::runtime_error("Ambiguous Curve data");found=i;}return found;
}
DWORD curve_stride(const Part& part){const auto count=read32(part.bytes,4);if(count<4)throw std::runtime_error("Curve record size missing");const auto size=read32(part.bytes,8);if(size<28||size>4096||size%4||(count-4)%size)throw std::runtime_error("Unsupported Curve record size");return size;}
short curve_short(const Bytes& b,size_t at){return static_cast<short>(b.at(at)|(b.at(at+1)<<8));}
void curve_short(Bytes& b,size_t at,int value){b.at(at)=static_cast<BYTE>(value);b.at(at+1)=static_cast<BYTE>(static_cast<unsigned>(value)>>8);}
void write_curve(Bytes& record,const SequenceCurve& curve){
    if(!valid_sequence_curve(curve))throw std::runtime_error("Invalid Sequence curve");
    const auto raw=std::int64_t(curve.time)-curve_short(record,16);if(raw<INT32_MIN||raw>INT32_MAX)throw std::runtime_error("Curve raw time overflow");
    if(record.size()<32&&(curve.controller.parameter||curve.merge))throw std::runtime_error("Legacy Curve record cannot hold DX8 parameters");
    put32(record,0,static_cast<std::uint32_t>(raw));put32(record,4,curve.duration);put32(record,8,curve.resetDuration);put32(record,12,curve.channel);
    const auto bias=curve.controller.type==3?8192:0;
    curve_short(record,18,curve.start+bias);curve_short(record,20,curve.end+bias);curve_short(record,22,curve.reset+bias);
    record[24]=curve.controller.type;record[25]=curve.shape;record[26]=curve.controller.data;record[27]=curve.flags;
    if(record.size()>=32){curve_short(record,28,curve.controller.parameter);curve_short(record,30,curve.merge);}
}
size_t place_curve(Part& part,const Bytes& record,DWORD size){
    size_t at=12;const auto time=static_cast<std::int32_t>(read32(record,0));for(;at<part.bytes.size();at+=size)if(static_cast<std::int32_t>(read32(part.bytes,at))>time)break;
    part.bytes.insert(part.bytes.begin()+at,record.begin(),record.end());put32(part.bytes,4,static_cast<std::uint32_t>(part.bytes.size()-8));return (at-12)/size;
}
}
int sequence_curve_minimum(SequenceController controller){return controller.type==3?-8192:0;}
int sequence_curve_maximum(SequenceController controller){return controller.type==3?8191:controller.type==7||controller.type==8?16383:127;}
bool valid_sequence_curve(const SequenceCurve& c){
    if(c.time<0||c.duration<0||(!c.duration&&c.shape!=1)||c.resetDuration<0||c.channel>=0xfffffffcu||c.shape>4||c.controller.type<3||c.controller.type>8||c.controller.data>127||(c.controller.parameter&0x8080))return false;
    if(c.controller.type!=7&&c.controller.type!=8&&c.controller.parameter)return false;
    if(c.controller.type!=4&&c.controller.data)return false;
    const auto low=sequence_curve_minimum(c.controller),high=sequence_curve_maximum(c.controller);return c.start>=low&&c.start<=high&&c.end>=low&&c.end<=high&&c.reset>=low&&c.reset<=high;
}
double sequence_curve_value(const SequenceCurve& c,double fraction){
    // Display approximation; native playback evaluates the stored DX shape.
    const auto t=std::clamp(fraction,0.,1.);double f=t;
    switch(c.shape){case 1:f=1;break;case 2:f=t*t*t*t;break;case 3:f=1-std::pow(1-t,4);break;case 4:f=(1-std::cos(t*3.14159265358979323846))/2;break;default:break;}
    return c.start+(c.end-c.start)*f;
}
std::vector<SequenceCurve> sequence_curves(const Bytes& payload){
    const auto p=parts(payload);(void)events_part(p);const auto found=curves_part(p);if(found==SIZE_MAX)return {};
    const auto& part=p[found];const auto size=curve_stride(part);std::vector<SequenceCurve> out;
    for(size_t at=12;at<part.bytes.size();at+=size){const auto time=std::int64_t(static_cast<std::int32_t>(read32(part.bytes,at)))+curve_short(part.bytes,at+16);if(time<INT32_MIN||time>INT32_MAX)throw std::runtime_error("Curve time overflow");
        SequenceCurve c;c.time=static_cast<std::int32_t>(time);c.duration=static_cast<std::int32_t>(read32(part.bytes,at+4));c.resetDuration=static_cast<std::int32_t>(read32(part.bytes,at+8));c.channel=read32(part.bytes,at+12);
        const int bias=part.bytes[at+24]==3?8192:0;c.start=static_cast<short>(curve_short(part.bytes,at+18)-bias);c.end=static_cast<short>(curve_short(part.bytes,at+20)-bias);c.reset=static_cast<short>(curve_short(part.bytes,at+22)-bias);c.controller={part.bytes[at+24],part.bytes[at+26],size>=32?static_cast<std::uint16_t>(curve_short(part.bytes,at+28)):std::uint16_t(0)};c.shape=part.bytes[at+25];c.flags=part.bytes[at+27];if(size>=32)c.merge=static_cast<std::uint16_t>(curve_short(part.bytes,at+30));out.push_back(c);
    }return out;
}
Bytes sequence_insert_curve(const Bytes& payload,const SequenceCurve& curve,size_t* resultingIndex){
    if(!valid_sequence_curve(curve))throw std::runtime_error("Invalid Sequence curve");auto p=parts(payload);(void)events_part(p);auto found=curves_part(p);
    if(found==SIZE_MAX){Bytes bytes(12);std::memcpy(bytes.data(),"curl",4);put32(bytes,4,4);put32(bytes,8,32);p.push_back({"curl",bytes});found=p.size()-1;}
    auto& part=p[found];const auto size=curve_stride(part);Bytes record(size);write_curve(record,curve);const auto index=place_curve(part,record,size);if(resultingIndex)*resultingIndex=index;return join(p);
}
Bytes sequence_change_curve(const Bytes& payload,size_t index,const SequenceCurve& curve,size_t* resultingIndex){
    auto p=parts(payload);const auto found=curves_part(p);if(found==SIZE_MAX)throw std::out_of_range("Sequence curve index");auto& part=p[found];const auto size=curve_stride(part);if(index>=(part.bytes.size()-12)/size)throw std::out_of_range("Sequence curve index");
    const auto at=12+index*size;Bytes record(part.bytes.begin()+at,part.bytes.begin()+at+size);write_curve(record,curve);size_t after=index;
    if(read32(record,0)==read32(part.bytes,at))std::copy(record.begin(),record.end(),part.bytes.begin()+at);
    else{part.bytes.erase(part.bytes.begin()+at,part.bytes.begin()+at+size);after=place_curve(part,record,size);}
    if(resultingIndex)*resultingIndex=after;return join(p);
}
Bytes sequence_delete_curve(const Bytes& payload,size_t index){
    auto p=parts(payload);const auto found=curves_part(p);if(found==SIZE_MAX)throw std::out_of_range("Sequence curve index");auto& part=p[found];const auto size=curve_stride(part);if(index>=(part.bytes.size()-12)/size)throw std::out_of_range("Sequence curve index");
    const auto at=12+index*size;part.bytes.erase(part.bytes.begin()+at,part.bytes.begin()+at+size);put32(part.bytes,4,static_cast<std::uint32_t>(part.bytes.size()-8));return join(p);
}
namespace {
bool timed(const Part& p){return p.id=="evtl"||p.id=="curl";}
DWORD range_stride(const Part& p){
    if(p.id=="evtl")return stride(p);
    const auto count=read32(p.bytes,4);if(count<4)throw std::runtime_error("Curve record size missing");
    const auto size=read32(p.bytes,8);if(size<28||size>4096||size%4||(count-4)%size)throw std::runtime_error("Unsupported Curve record size");return size;
}
std::int64_t effective_time(const Bytes& record,const std::string& id){
    const size_t offset=id=="evtl"?12:16;
    return std::int64_t(static_cast<std::int32_t>(read32(record,0)))+static_cast<short>(record[offset]|(record[offset+1]<<8));
}
void validate_range_parts(const std::vector<Part>& p,bool clipboard){
    (void)events_part(p);bool curve=false;
    for(const auto& item:p){if(clipboard&&!timed(item))throw std::runtime_error("Unknown range clipboard chunk");if(item.id=="curl"){if(curve)throw std::runtime_error("Ambiguous Curve data");curve=true;}if(timed(item))(void)range_stride(item);}
}
Part range_part(const Part& p,std::int32_t begin,std::int32_t end,bool keepInside,bool relative){
    const auto size=range_stride(p);Part result{p.id,Bytes(p.bytes.begin(),p.bytes.begin()+12)};
    for(size_t pos=12;pos<8+read32(p.bytes,4);pos+=size){Bytes record(p.bytes.begin()+pos,p.bytes.begin()+pos+size);const auto time=effective_time(record,p.id);
        if((time>=begin&&time<end)!=keepInside)continue;
        if(relative){const auto raw=std::int64_t(static_cast<std::int32_t>(read32(record,0)))-begin;if(raw<INT32_MIN||raw>INT32_MAX)throw std::runtime_error("Range timestamp overflow");put32(record,0,static_cast<std::uint32_t>(raw));}
        result.bytes.insert(result.bytes.end(),record.begin(),record.end());
    }put32(result.bytes,4,static_cast<std::uint32_t>(result.bytes.size()-8));return result;
}
void check_range(std::int32_t begin,std::int32_t end){if(begin<0||end<=begin)throw std::runtime_error("Invalid Sequence range");}
}
Bytes sequence_copy_range(const Bytes& payload,std::int32_t begin,std::int32_t end){
    check_range(begin,end);const auto p=parts(payload);validate_range_parts(p,false);std::vector<Part> out;
    for(const auto& item:p)if(timed(item))out.push_back(range_part(item,begin,end,true,true));return join(out);
}
Bytes sequence_delete_range(const Bytes& payload,std::int32_t begin,std::int32_t end){
    check_range(begin,end);auto p=parts(payload);validate_range_parts(p,false);
    for(auto& item:p)if(timed(item))item=range_part(item,begin,end,false,false);return join(p);
}
bool sequence_range_empty(const Bytes& bytes){const auto p=parts(bytes);validate_range_parts(p,true);return std::all_of(p.begin(),p.end(),[](const Part& item){return read32(item.bytes,4)==4;});}
Bytes sequence_paste_range(const Bytes& destination,const Bytes& clipboard,std::int32_t at,std::int32_t span,bool overwrite,std::int32_t length){
    if(at<0||span<=0||std::int64_t(at)+span>length)throw std::runtime_error("Invalid Sequence paste range");
    auto p=parts(overwrite?sequence_delete_range(destination,at,at+span):destination);const auto incoming=parts(clipboard);validate_range_parts(p,false);validate_range_parts(incoming,true);
    for(const auto& source:incoming){auto target=std::find_if(p.begin(),p.end(),[&](const Part& item){return item.id==source.id;});
        if(read32(source.bytes,4)==4)continue;
        if(target==p.end()){p.push_back({source.id,Bytes(source.bytes.begin(),source.bytes.begin()+12)});put32(p.back().bytes,4,4);target=p.end()-1;}
        const auto size=range_stride(source);if(read32(target->bytes,4)==4)put32(target->bytes,8,size);if(size!=range_stride(*target))throw std::runtime_error("Range record sizes differ; extensions cannot be discarded");
        std::vector<Bytes> records;for(size_t pos=12;pos<8+read32(target->bytes,4);pos+=size)records.emplace_back(target->bytes.begin()+pos,target->bytes.begin()+pos+size);
        for(size_t pos=12;pos<8+read32(source.bytes,4);pos+=size){Bytes record(source.bytes.begin()+pos,source.bytes.begin()+pos+size);const auto relative=effective_time(record,source.id);if(relative<0||relative>=span)throw std::runtime_error("Clipboard record outside range");if(source.id=="curl"||((record[14]&0xf0)==0x90&&record[16])){const auto duration=static_cast<std::int32_t>(read32(record,4));if(duration<0||std::int64_t(at)+relative+duration>length)throw std::runtime_error("Pasted duration exceeds Segment");}const auto raw=std::int64_t(static_cast<std::int32_t>(read32(record,0)))+at;if(raw<INT32_MIN||raw>INT32_MAX)throw std::runtime_error("Paste timestamp overflow");put32(record,0,static_cast<std::uint32_t>(raw));records.push_back(std::move(record));}
        std::stable_sort(records.begin(),records.end(),[](const Bytes& a,const Bytes& b){return static_cast<std::int32_t>(read32(a,0))<static_cast<std::int32_t>(read32(b,0));});
        target->bytes.resize(12);for(const auto& record:records)target->bytes.insert(target->bytes.end(),record.begin(),record.end());put32(target->bytes,4,static_cast<std::uint32_t>(target->bytes.size()-8));
    }return join(p);
}
Chunk sequence_track() {
    Bytes records(4);put32(records,0,20);auto sequence=leaf("evtl",records).encode();Bytes curves(4);put32(curves,0,32);const auto curve=leaf("curl",curves).encode();sequence.insert(sequence.end(),curve.begin(),curve.end());
    return container("RIFF","DMTK",{leaf("trkh",track_header(0xd2ac2886,"seqt")),leaf("seqt",sequence)});
}
Chunk gm_piano_band_track() {
    Bytes instrument(44);put32(instrument,28,0x1161);instrument[32]=64;instrument[33]=100; // patch0, PChannel0, GM/default GM + patch/pan/volume.
    const auto band=container("RIFF","DMBD",{container("LIST","lbil",{container("LIST","lbin",{leaf("bins",instrument)})})});
    Bytes autoDownload(4);put32(autoDownload,0,1);
    const auto track=container("RIFF","DMBT",{leaf("bdth",autoDownload),container("LIST","lbdl",{container("LIST","lbnd",{leaf("bd2h",Bytes(8)),band})})});
    return container("RIFF","DMTK",{leaf("trkh",track_header(0xd2ac2894,nullptr,"DMBT")),track});
}
}
