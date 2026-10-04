#include "sequence.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
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
Bytes sequence_insert(const Bytes& payload,Note note) {
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);Bytes record(size);
    put32(record,0,note.time);put32(record,4,note.duration);put32(record,8,note.channel);record[14]=0x90;record[15]=note.pitch;record[16]=note.velocity;
    size_t insert=12;const auto end=8+read32(part.bytes,4);
    for(;insert<end;insert+=size)if(static_cast<std::int32_t>(read32(part.bytes,insert))>note.time)break;
    part.bytes.insert(part.bytes.begin()+insert,record.begin(),record.end());put32(part.bytes,4,read32(part.bytes,4)+size);
    Bytes result;for(const auto& item:p)result.insert(result.end(),item.bytes.begin(),item.bytes.end());return result;
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
Bytes sequence_delete(const Bytes& payload,size_t index){
    auto p=parts(payload);auto& part=p.at(events_part(p));const auto size=stride(part);const auto at=note_record(part,size,index);
    part.bytes.erase(part.bytes.begin()+at,part.bytes.begin()+at+size);put32(part.bytes,4,read32(part.bytes,4)-size);return join(p);
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
