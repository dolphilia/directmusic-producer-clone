#include "signpost.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace producer::app {
namespace {
struct Record {SignpostEvent event;Bytes bytes;};
size_t stride(const Bytes& b){if(b.size()<4)throw std::runtime_error("Truncated SignPost header");const auto s=read32(b,0);if(s<10||(b.size()-4)%s)throw std::runtime_error("Invalid SignPost stride or records");return s;}
std::vector<Record> records(const Bytes& b){const auto s=stride(b);std::vector<Record> r;for(size_t p=4;p<b.size();p+=s){SignpostEvent e{static_cast<std::int32_t>(read32(b,p)),read32(b,p+4),static_cast<std::uint16_t>(b[p+8]|(b[p+9]<<8))};if(e.time<0)throw std::runtime_error("Negative SignPost position");r.push_back({e,Bytes(b.begin()+p,b.begin()+p+s)});}return r;}
Bytes patch(Bytes b,SignpostEvent e){put32(b,0,static_cast<std::uint32_t>(e.time));put32(b,4,e.chords);b[8]=static_cast<std::uint8_t>(e.measure);b[9]=static_cast<std::uint8_t>(e.measure>>8);return b;}
size_t commit(Bytes& b,std::vector<Record> r,SignpostEvent e){std::stable_sort(r.begin(),r.end(),[](const Record& a,const Record& c){return a.event.time<c.event.time;});Bytes out(b.begin(),b.begin()+4);size_t index=0;for(size_t i=0;i<r.size();++i){out.insert(out.end(),r[i].bytes.begin(),r[i].bytes.end());if(r[i].event.measure==e.measure)index=i;}b=std::move(out);return index;}
}
bool valid_signpost(SignpostEvent e,const SignpostEvent* old){const auto group=e.chords&~0x8000u;return e.time>=0&&((group&&!(group&(group-1))&&!(group&~0x7f3fu))||(old&&group==(old->chords&~0x8000u)));}
std::vector<SignpostEvent> signpost_events(const Bytes& b){std::vector<SignpostEvent> out;for(const auto& r:records(b))out.push_back(r.event);return out;}
bool set_signpost_event(Bytes& b,SignpostEvent e,size_t* result){auto r=records(b);auto found=std::find_if(r.begin(),r.end(),[&](const Record& v){return v.event.measure==e.measure;});if(!valid_signpost(e,found==r.end()?nullptr:&found->event))return false;auto bytes=patch(found==r.end()?Bytes(stride(b)):found->bytes,e);if(found!=r.end()){if(found->bytes==bytes)return false;*found={e,bytes};}else r.push_back({e,bytes});const auto i=commit(b,std::move(r),e);if(result)*result=i;return true;}
bool change_signpost_event(Bytes& b,size_t index,SignpostEvent e,size_t* result){auto r=records(b);if(index>=r.size()||!valid_signpost(e,&r[index].event))return false;const auto bytes=patch(r[index].bytes,e);if(bytes==r[index].bytes)return false;r.erase(r.begin()+index);r.erase(std::remove_if(r.begin(),r.end(),[&](const Record& v){return v.event.measure==e.measure;}),r.end());r.push_back({e,bytes});const auto i=commit(b,std::move(r),e);if(result)*result=i;return true;}
bool delete_signpost_event(Bytes& b,size_t index){auto r=records(b);if(index>=r.size())return false;r.erase(r.begin()+index);(void)commit(b,std::move(r),SignpostEvent{});return true;}
Chunk signpost_track(){Chunk root;root.id="RIFF";root.type="DMTK";Chunk h;h.id="trkh";h.data=Bytes(32);const Bytes id={0x72,0x86,0x7e,0xf1,0xb4,0xc3,0xd1,0x11,0x87,0x0b,0x00,0x60,0x08,0x93,0xb1,0xbd};std::copy(id.begin(),id.end(),h.data.begin());put32(h.data,20,1);std::memcpy(h.data.data()+24,"sgnp",4);Chunk data;data.id="sgnp";data.data=Bytes(4);put32(data.data,0,12);root.children={h,data};return root;}
}
