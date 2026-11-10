#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <vector>
#include <algorithm>

namespace producer::app {
struct PlaybackMonitorSample {std::uint64_t id;bool playing;std::int32_t clocks,start;};
enum class PlaybackCompletion {Ended,StartTimedOut};
struct PlaybackMonitorEvent {std::uint64_t id;PlaybackCompletion completion;};
// UI-thread samples of every owned instance. Start timeout runs only after
// the runtime's actual scheduled music clock, not an estimated tempo delay.
class PlaybackMonitor {
    struct History {bool started=false;std::optional<std::uint64_t> dueSince;};
    std::map<std::uint64_t,History> histories_;
public:
    void clear(){histories_.clear();}
    void forget(std::uint64_t id){histories_.erase(id);}
    // Segment notifications can arrive after a short instance has already
    // ended between UI samples. They still prove that it started.
    void observed_start(std::uint64_t id){if(id){auto& history=histories_[id];history.started=true;history.dueSince.reset();}}
    std::vector<PlaybackMonitorEvent> update(const std::vector<PlaybackMonitorSample>& samples,std::uint64_t now){
        for(auto i=histories_.begin();i!=histories_.end();){
            if(std::none_of(samples.begin(),samples.end(),[&](const auto& s){return s.id==i->first;}))i=histories_.erase(i);else ++i;
        }
        std::vector<PlaybackMonitorEvent> events;
        for(const auto& sample:samples){
            if(!sample.id)continue;
            auto& history=histories_[sample.id];
            if(sample.playing){history.started=true;history.dueSince.reset();continue;}
            if(history.started){events.push_back({sample.id,PlaybackCompletion::Ended});continue;}
            if(sample.clocks<sample.start){history.dueSince.reset();continue;}
            if(!history.dueSince)history.dueSince=now;
            if(now>=*history.dueSince&&now-*history.dueSince>=5000)events.push_back({sample.id,PlaybackCompletion::StartTimedOut});
        }
        return events;
    }
};
}
