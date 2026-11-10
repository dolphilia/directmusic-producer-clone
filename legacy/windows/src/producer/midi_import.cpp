#include "midi_import.h"
#include "band.h"
#include <algorithm>
#include <deque>
#include <cstring>
#include <limits>
#include <map>
#include <tuple>
#include <stdexcept>

namespace producer::app {
namespace {
[[noreturn]] void fail(const char* why){throw std::runtime_error(std::string("MIDI import: ")+why);}
struct Reader {
    const Bytes& b;size_t at,end;
    unsigned byte(){if(at==end)fail("truncated event");return b.at(at++);}
    unsigned be(unsigned n){unsigned v=0;while(n--)v=(v<<8)|byte();return v;}
    unsigned vlq(){unsigned v=0;for(unsigned i=0;i<4;++i){const auto c=byte();v=(v<<7)|(c&127);if(!(c&128))return v;}fail("VLQ exceeds four bytes");}
    Bytes take(size_t n){if(n>end-at)fail("event exceeds track boundary");Bytes v(b.begin()+at,b.begin()+at+n);at+=n;return v;}
};
struct Event {std::uint64_t tick;unsigned track,order,status,a,b;Bytes meta;};
Chunk leaf(const char* id,Bytes data){Chunk c;c.id=id;c.data=std::move(data);return c;}
Chunk list(const char* id,const char* type,std::vector<Chunk> children){Chunk c;c.id=id;c.type=type;c.children=std::move(children);return c;}
std::int32_t clocks(std::uint64_t tick,unsigned division){
    if(tick>(std::uint64_t(INT32_MAX)*division-division/2)/768)fail("timeline exceeds MUSIC_TIME");
    return static_cast<std::int32_t>((tick*768+division/2)/division);
}
Bytes curve_data(const std::vector<Bytes>& records){Bytes b(4);put32(b,0,32);for(const auto& r:records)b.insert(b.end(),r.begin(),r.end());return leaf("curl",std::move(b)).encode();}
Bytes curve(std::int32_t time,unsigned channel,unsigned type,unsigned value,unsigned cc){
    Bytes b(32);put32(b,0,time);put32(b,12,channel);b[18]=b[20]=static_cast<std::uint8_t>(value);b[19]=b[21]=static_cast<std::uint8_t>(value>>8);
    b[24]=static_cast<std::uint8_t>(type);b[25]=1;b[26]=static_cast<std::uint8_t>(cc);return b;
}
struct Channel {unsigned msb=0,lsb=0,program=0,pan=64,volume=100;};
}
MidiImport import_midi(const Bytes& bytes){
    if(bytes.size()>64*1024*1024)fail("file exceeds bounded import size");
    Reader file{bytes,0,bytes.size()};if(file.take(4)!=Bytes{'M','T','h','d'})fail("missing MThd");
    if(file.be(4)!=6)fail("unsupported header extension");
    MidiImport result;result.format=file.be(2);result.sourceTracks=file.be(2);result.division=file.be(2);
    if(result.format>1)fail("format 2 requires independent pattern import");
    if(!result.sourceTracks||result.sourceTracks>1024||(result.format==0&&result.sourceTracks!=1))fail("invalid track count");
    if(!result.division||(result.division&0x8000))fail("SMPTE timing requires its own conversion contract");
    std::vector<Event> events;std::uint64_t lastTick=0;unsigned descriptive=0;
    for(unsigned track=0;track<result.sourceTracks;++track){
        if(file.take(4)!=Bytes{'M','T','r','k'})fail("missing MTrk");const auto n=file.be(4);if(n>file.end-file.at)fail("track exceeds file boundary");
        Reader r{bytes,file.at,file.at+n};file.at+=n;std::uint64_t tick=0;unsigned running=0,order=0;bool ended=false;
        while(r.at<r.end){tick+=r.vlq();(void)clocks(tick,result.division);unsigned status=r.byte(),a=0,b=0;
            if(status<128){if(!running)fail("running status without channel status");a=status;status=running;}else if(status<240){running=status;a=r.byte();}
            else if(status==255){running=0;const auto kind=r.byte();const auto size=r.vlq();auto data=r.take(size);
                if(kind==0x2f){if(size||r.at!=r.end)fail("invalid End of Track");ended=true;break;}
                if(kind==0x51){if(size!=3)fail("invalid tempo length");events.push_back({tick,track,order++,255,kind,0,std::move(data)});}
                else if(kind==0x58){if(size!=4)fail("invalid meter length");events.push_back({tick,track,order++,255,kind,0,std::move(data)});}
                else if(kind==0x21){if(size!=1||data[0]!=0)fail("multiple MIDI ports require PChannel mapping");}
                else if(kind==0x59){if(size!=2||tick||data[0]!=0||data[1]!=0)fail("key signature requires explicit editor mapping");}
                else if(kind==0x00||kind==0x03||kind==0x04||kind==0x02||kind==0x01){++descriptive;}
                else fail("unsupported meta event (lyrics/key/signatures/markers require explicit import mapping)");
                continue;
            }else fail("SysEx or system message requires an explicit import mapping");
            const unsigned type=status>>4;if(type!=12&&type!=13)b=r.byte();if(a>127||b>127)fail("channel data must be seven-bit");
            events.push_back({tick,track,order++,status,a,b,{}});if(events.size()>1000000)fail("event count exceeds bounded import size");
        }
        if(!ended)fail("End of Track missing");lastTick=std::max(lastTick,tick);
    }
    if(file.at!=file.end)fail("trailing bytes or undeclared tracks");
    std::stable_sort(events.begin(),events.end(),[](const Event& a,const Event& b){return std::tie(a.tick,a.track,a.order)<std::tie(b.tick,b.track,b.order);});
    std::array<std::vector<Note>,16> notes;std::array<std::vector<Bytes>,16> curves;
    struct Start {std::uint64_t tick;unsigned velocity;};std::array<std::array<std::deque<Start>,128>,16> pending;
    std::array<Channel,16> states;std::array<bool,16> used{};
    std::map<std::int32_t,double> tempos{{0,120.0}};std::map<std::int32_t,Bytes> meters{{0,{4,2,24,8}}};
    struct Patch {std::int32_t time;unsigned channel;Channel state;};std::vector<Patch> patches;
    auto finish=[&](unsigned ch,unsigned key,std::uint64_t tick){auto& q=pending[ch][key];if(q.empty())fail("note-off without matching note-on");const auto start=q.front();q.pop_front();const auto t=clocks(start.tick,result.division),end=clocks(tick,result.division);if(end<=t)fail("note duration collapses at DirectMusic clock resolution");notes[ch].push_back({t,end-t,ch,static_cast<std::uint8_t>(key),static_cast<std::uint8_t>(start.velocity)});++result.notes;};
    for(const auto& e:events){const auto time=clocks(e.tick,result.division);
        if(e.status==255){if(e.a==0x51){const auto us=(e.meta[0]<<16)|(e.meta[1]<<8)|e.meta[2];if(!us)fail("zero microseconds per quarter");const double bpm=60000000.0/us;if(bpm<1||bpm>1000)fail("tempo outside editor contract");tempos[time]=bpm;}else{if(!e.meta[0]||e.meta[1]>5||e.meta[2]!=24||e.meta[3]!=8)fail("unsupported meter or metronome notation");meters[time]=e.meta;}continue;}
        const auto ch=e.status&15,type=e.status>>4;used[ch]=true;auto& state=states[ch];
        switch(type){
        case 8:finish(ch,e.a,e.tick);break;
        case 9:if(e.b)pending[ch][e.a].push_back({e.tick,e.b});else finish(ch,e.a,e.tick);break;
        case 11:
            if(e.a==0)state.msb=e.b;else if(e.a==32)state.lsb=e.b;
            else {if(e.a==7)state.volume=e.b;if(e.a==10)state.pan=e.b;curves[ch].push_back(curve(time,ch,4,e.b,e.a));++result.curves;}
            if(e.a==120||e.a==123){for(unsigned key=0;key<128;++key)while(!pending[ch][key].empty())finish(ch,key,e.tick);}break;
        case 12:state.program=e.a;patches.push_back({time,ch,state});break;
        case 13:curves[ch].push_back(curve(time,ch,5,e.a,0));++result.curves;break;
        case 14:curves[ch].push_back(curve(time,ch,3,e.a|(e.b<<7),0));++result.curves;break;
        case 10:fail("polyphonic aftertouch requires key-specific native mapping");
        default:fail("unsupported channel event");
        }
    }
    for(const auto& channel:pending)for(const auto& key:channel)if(!key.empty())fail("unterminated note-on");
    auto length=clocks(lastTick,result.division);if(length==INT32_MAX)fail("no room for final event");length=std::max(1,length+1);
    auto root=Chunk::parse(result.segment.save_bytes());put32(root.find("segh")->data,4,length);put32(root.find("segh")->data,20,0);
    auto& tracks=root.find("LIST","trkl")->children;auto& tempoData=tracks[0].find("tetr")->data;tempoData.assign(4,0);put32(tempoData,0,16);
    for(const auto& e:tempos){Bytes r(16);put32(r,0,e.first);std::memcpy(r.data()+8,&e.second,8);tempoData.insert(tempoData.end(),r.begin(),r.end());}
    Bytes mh(32);const GUID meterId={0xd2ac2888,0xb39b,0x11d1,{0x87,4,0,0x60,8,0x93,0xb1,0xbd}};std::memcpy(mh.data(),&meterId,16);put32(mh,20,1);std::memcpy(mh.data()+28,"TIMS",4);
    Bytes meterData(4);put32(meterData,0,8);int previous=0;unsigned beats=4,den=4;
    for(const auto& e:meters){const auto span=3072/den*beats;if((e.first-previous)%span)fail("mid-measure time signature requires editor contract extension");Bytes r(8);put32(r,0,e.first);r[4]=e.second[0];r[5]=static_cast<std::uint8_t>(1u<<e.second[1]);r[6]=4;meterData.insert(meterData.end(),r.begin(),r.end());previous=e.first;beats=r[4];den=r[5];}
    tracks.push_back(list("RIFF","DMTK",{leaf("trkh",mh),list("LIST","TIMS",{leaf("tims",meterData)})}));
    for(unsigned ch=0;ch<16;++ch)if(used[ch]){result.channels.push_back(ch);auto track=sequence_track();auto& payload=track.find("seqt")->data;
        for(const auto& n:notes[ch])payload=sequence_insert(payload,n);
        // The generated factory contains only evtl then curl. Preserve the
        // complete evtl (including its RIFF padding), replacing empty curves.
        const auto eventSize=8+read32(payload,4)+(read32(payload,4)&1);payload.resize(eventSize);const auto c=curve_data(curves[ch]);payload.insert(payload.end(),c.begin(),c.end());tracks.push_back(std::move(track));
    }
    auto bandTrack=make_band_track();std::array<Channel,16> initial;
    for(const auto& p:patches)if(p.time==0)initial[p.channel]=p.state;
    // Volume/pan at zero are also represented as CCs; default Band values
    // remain neutral until the corresponding Sequence controller is played.
    auto band=[&](const std::array<Channel,16>& s){BandDocument b;for(const auto ch:result.channels){const auto& c=s[ch];b.add_gm_instrument(c.program|(c.lsb<<8)|(c.msb<<16)|(ch==9?0x80000000u:0),ch,c.pan,c.volume);}return b.save_bytes();};
    if(!result.channels.empty()){set_band_track_event(bandTrack,0,band(initial));
        // Merge simultaneous patch changes, but do not reassign unrelated
        // channels and reset their controllers at another channel's change.
        std::map<std::int32_t,std::map<unsigned,Channel>> changed;
        for(const auto& p:patches)if(p.time)changed[p.time][p.channel]=p.state;
        for(const auto& e:changed){BandDocument b;for(const auto& p:e.second){const auto& c=p.second;b.add_gm_instrument(c.program|(c.lsb<<8)|(c.msb<<16)|(p.first==9?0x80000000u:0),p.first,c.pan,c.volume);}set_band_track_event(bandTrack,e.first,b.save_bytes());}
        tracks.push_back(std::move(bandTrack));}
    auto chord=chord_track();set_chord_event(*chord.find("LIST","cord"),ChordEvent{});tracks.push_back(std::move(chord));
    for(size_t i=0;i<tracks.size();++i)put32(tracks[i].find("trkh")->data,16,static_cast<std::uint32_t>(i));
    result.segment.load(root.encode());if(descriptive)result.notices.push_back("Descriptive MIDI text/name/copyright metadata is not imported into editor names");
    return result;
}
}
