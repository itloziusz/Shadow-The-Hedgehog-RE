#include "shadowpc/streaming/WorldStreaming.hpp"
#include <cmath>
namespace shadowpc {
float WorldStreamer::distance_to_aabb(Vec3 p,const Aabb& b){auto d=[](float v,float lo,float hi){return v<lo?lo-v:v>hi?v-hi:0.f;};float x=d(p.x,b.min.x,b.max.x),y=d(p.y,b.min.y,b.max.y),z=d(p.z,b.min.z,b.max.z);return std::sqrt(x*x+y*y+z*z);}
void WorldStreamer::update(const StreamingView& view){
    for(const auto& c:cells_){
        const float dist=distance_to_aabb(view.position,c.bounds); const bool loaded=resident_.contains(c.id); const auto cc=center(c.bounds);
        StreamingObservation obs{}; obs.camera_to_asset={cc.x-view.position.x,cc.y-view.position.y,cc.z-view.position.z}; obs.camera_forward={view.forward.x,view.forward.y,view.forward.z}; obs.player_velocity={view.forward.x*view.speed,view.forward.y*view.speed,view.forward.z*view.speed}; obs.bounding_radius=0.0f; obs.inside_frustum=false; obs.mission_critical=c.mission_critical;
        const auto score=priority_.score(obs);
        const float predictive_radius=c.preload_radius+std::min(view.speed*1.5f,120.0f);
        if(!loaded&&(dist<=predictive_radius||score.priority==ResourcePriority::Critical)){
            auto& list=resident_[c.id];list.reserve(c.assets.size());for(auto id:c.assets){auto h=resources_.request(id,score.priority);if(h)list.push_back(h);}
        }else if(loaded&&dist>c.unload_radius+std::min(view.speed*0.5f,60.0f)&&!c.mission_critical){
            for(auto h:resident_[c.id]) resources_.release(h);
            resident_.erase(c.id);
        }
    }
}
}
