#include "style_player_session.h"
#include "document.h"
#include "chordmap.h"
#include "style_player_band.h"
#include <stdexcept>
#include <algorithm>
#include <filesystem>
namespace producer::app {
namespace {
void validate_settings(const StylePlayerSettings& s){if(s.activity>3||s.measures==0||static_cast<WORD>(s.shape)>8)throw std::runtime_error("StylePlayer settings invalid");}
std::wstring band_name(const BandDocument& band){const auto root=Chunk::parse(band.save_bytes());if(const auto info=root.find("LIST","UNFO"))if(const auto n=info->find("UNAM"))return decode_utf16(n->data);return L"";}
BandDocument selected_band(const StyleCatalogEntry& style,const std::wstring& name){StyleDocument d;d.load(style.bytes);const auto bands=d.bands();size_t index=bands.size();for(size_t i=0;i<bands.size();++i)if(band_name(bands[i])==name){if(index!=bands.size())throw std::runtime_error("StylePlayer Band name ambiguous");index=i;}if(index==bands.size())throw std::runtime_error("StylePlayer Band not in selected Style");return bands[index];}
void add_band(StylePlayerComposition& composition,const BandDocument& band){SegmentDocument d;d.load(composition.segment);d.set_band(0,band.save_bytes());composition.segment=d.save_bytes();}
std::vector<ResolvedCollection> audition_collections(const std::vector<Bytes>& documents,const StyleCatalogEntry& style,const std::vector<ResolvedCollection>& sources){
    std::vector<CollectionEntry> catalog;for(const auto& c:sources){const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const CollectionEntry& e){return CompareStringOrdinal(e.path.c_str(),-1,c.path.c_str(),-1,TRUE)==CSTR_EQUAL;});if(found!=catalog.end()){if(found->bytes!=c.bytes)throw std::runtime_error("StylePlayer collection snapshots conflict");}else catalog.push_back({c.path,c.bytes});}
    std::vector<CollectionReference> references;for(const auto& bytes:documents){const auto refs=document_collection_references(bytes);references.insert(references.end(),refs.begin(),refs.end());}
    return resolve_collections(references,std::filesystem::path(style.path).parent_path().wstring(),catalog);
}
}
StylePlayerSession::StylePlayerSession(Conductor& conductor,HWND owner,StyleCatalogEntry style,std::vector<ResolvedCollection> collections,std::optional<Bytes> chordMap,StylePlayerSettings settings,std::vector<ChordMapCatalogEntry> maps):conductor_(conductor),owner_(owner),style_(std::move(style)),collections_(std::move(collections)),maps_(std::move(maps)),chordMap_(std::move(chordMap)),settings_(settings){StyleDocument d;d.load(style_.bytes);validate_settings(settings);}
bool StylePlayerSession::playing() const {return playing_&&primary_&&conductor_.position(primary_).playing;}
void StylePlayerSession::start(){
    auto next=compose_style_player_segment(style_,chordMap_,settings_,collections_,maps_);
    if(!band_.empty())add_band(next,selected_band(style_,band_));
    const auto dependencies=audition_collections({next.segment,next.style.bytes},style_,collections_);
    const auto nextBand=band_.empty()?initial_style_player_band_name(next.segment):band_;stop();
    conductor_.play(next.segment,L"",owner_,{next.style},dependencies,{},{},{},next.maps);
    primary_=conductor_.current_playback_id();composition_=std::move(next);band_=nextBand;playing_=true;++compositions_;
}
void StylePlayerSession::restart_from_beginning(){start();++restarts_;}
void StylePlayerSession::set_style(StyleCatalogEntry style,std::vector<ResolvedCollection> collections,std::optional<Bytes> chordMap,std::vector<ChordMapCatalogEntry> maps){StyleDocument d;d.load(style.bytes);auto oldStyle=style_;auto oldCollections=collections_;auto oldMaps=maps_;auto oldMap=chordMap_;auto oldBand=band_;style_=std::move(style);collections_=std::move(collections);maps_=std::move(maps);chordMap_=std::move(chordMap);band_.clear();try{restart_from_beginning();}catch(...){style_=std::move(oldStyle);collections_=std::move(oldCollections);maps_=std::move(oldMaps);chordMap_=std::move(oldMap);band_=std::move(oldBand);throw;}}
void StylePlayerSession::set_chordmap(std::optional<Bytes> map){if(map){ChordMapDocument d;d.load(*map);}auto previous=chordMap_;chordMap_=std::move(map);try{restart_from_beginning();}catch(...){chordMap_=std::move(previous);throw;}}
void StylePlayerSession::set_settings(const StylePlayerSettings& s){validate_settings(s);const auto previous=settings_;settings_=s;try{restart_from_beginning();}catch(...){settings_=previous;throw;}}
void StylePlayerSession::recompose(){restart_from_beginning();}
void StylePlayerSession::play(){start();}
void StylePlayerSession::stop(){for(auto id:layers_)conductor_.stop(id);layers_.clear();if(primary_)conductor_.stop(primary_);primary_=0;playing_=false;}
std::vector<std::wstring> StylePlayerSession::motif_names() const{StyleDocument d;d.load(style_.bytes);std::vector<std::wstring> r;for(const auto& p:d.patterns())if(p.embellishment&16)r.push_back(p.name);return r;}
std::vector<std::wstring> StylePlayerSession::band_names() const{StyleDocument d;d.load(style_.bytes);std::vector<std::wstring> r;for(const auto& b:d.bands())r.push_back(band_name(b));return r;}
void StylePlayerSession::select_band(const std::wstring& name){
    const auto band=selected_band(style_,name);
    auto next=composition_;if(next)add_band(*next,band);
    if(playing()){
        SegmentDocument carrier;auto root=Chunk::parse(carrier.save_bytes());root.find("LIST","trkl")->children.clear();carrier.load(root.encode());carrier.set_band(0,band.save_bytes());
        const auto dependencies=audition_collections({carrier.save_bytes()},style_,collections_);conductor_.play_extra(carrier.save_bytes(),owner_,dependencies,{PlaybackBoundary::Immediate,true,true,0},primary_);layers_.push_back(conductor_.current_playback_id());
    }
    band_=name;composition_=std::move(next);++bandChanges_;
}
void StylePlayerSession::play_motif(const std::wstring& name){
    if(!playing())throw std::runtime_error("Motifs layer on a running performance");
    PlaybackOptions options;options.secondary=true;options.boundary=PlaybackBoundary::Immediate;conductor_.play_motif(style_,name,owner_,collections_,options,{},primary_);layers_.push_back(conductor_.current_playback_id());++motifs_;
}
}
