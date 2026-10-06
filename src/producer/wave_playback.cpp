#include "wave_playback.h"
#include "wave_track.h"
#include <objbase.h>
#include <cstring>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
namespace producer::app {
WavePlaybackSnapshot prepare_wave_playback(const Bytes& source,const std::vector<ResolvedWave>& dependencies){
    WavePlaybackSnapshot result{source,dependencies};if(source.empty()){if(!dependencies.empty())throw std::runtime_error("Wave context without Segment");return result;}
    auto root=Chunk::parse(source);std::vector<Chunk*> references;std::vector<WaveItem> items;
    if(auto tracks=root.find("LIST","trkl"))for(auto& track:tracks->children){const auto h=track.find("trkh");const auto type=wave_track_identity();if(!h||h->data.size()<16||!std::equal(type.begin(),type.end(),h->data.begin()))continue;const auto values=wave_items(track);items.insert(items.end(),values.begin(),values.end());std::function<void(Chunk&)> visit=[&](Chunk& c){if(c.id=="LIST"&&c.type=="wave"){auto ref=c.find("LIST","DMRF");if(!ref)throw std::runtime_error("Wave reference missing");references.push_back(ref);return;}for(auto& child:c.children)visit(child);};visit(track);}
    if(references.size()!=dependencies.size()||items.size()!=dependencies.size())throw std::runtime_error("Unresolved playback Wave dependencies");
    for(size_t i=0;i<dependencies.size();++i){
        const auto header=references[i]->find("refh");if(!header||header->data.size()<20||std::memcmp(header->data.data(),&wave_runtime_class,16))throw std::runtime_error("Wave runtime reference class mismatch");const auto flags=read32(header->data,16);if(!(flags&2)||flags&(32u|64u|1024u|2048u))throw std::runtime_error("Wave reference requires project identity or relative filename");
        WaveDocument wave;wave.load(dependencies[i].bytes);const auto identity=wave.identity();if(items[i].objectId&&identity!=items[i].objectId)throw std::runtime_error("Wave playback identity mismatch");
        if(!items[i].objectId&&std::filesystem::path(items[i].filename).filename()!=std::filesystem::path(dependencies[i].path).filename())throw std::runtime_error("Wave filename-only context mismatch");
        const auto format=wave.format();if(format.tag==1){const auto offset=items[i].placement.startOffset;if(offset<0||static_cast<long double>(offset)*format.sampleRate>static_cast<long double>(format.frames)*10000000.0L)throw std::runtime_error("Wave start offset exceeds owned PCM");}
        result.waves[i].format=format;
        for(size_t j=0;j<i;++j){WaveDocument prior;prior.load(dependencies[j].bytes);if(identity&&identity==prior.identity()&&dependencies[i].bytes!=dependencies[j].bytes)throw std::runtime_error("Conflicting Wave snapshots share an identity");}
    }
    for(size_t i=0;i<result.waves.size();++i){
        auto& mapped=result.waves[i];WaveDocument wave;wave.load(mapped.bytes);auto identity=wave.identity();
        if(!identity){GUID guid{};if(FAILED(CoCreateGuid(&guid)))throw std::runtime_error("Wave playback identity creation failed");std::array<std::uint8_t,16> value{};std::memcpy(value.data(),&guid,16);identity=value;auto tree=Chunk::parse(mapped.bytes);Chunk c;c.id="guid";c.data.assign(value.begin(),value.end());tree.children.push_back(c);mapped.bytes=tree.encode();}
        auto& ref=*references[i];put32(ref.find("refh")->data,16,3u);auto id=ref.find("guid");if(!id){Chunk c;c.id="guid";ref.children.push_back(c);id=&ref.children.back();}id->data.assign(identity->begin(),identity->end());
    }
    result.segment=root.encode();return result;
}
}
