#include "shadowpc/streaming/PriorityEngine.hpp"
#include <algorithm>
#include <cmath>
namespace shadowpc {
namespace { float dot(const std::array<float,3>& a,const std::array<float,3>& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];} float len(const std::array<float,3>& a){return std::sqrt(dot(a,a));} }
StreamingScore StreamingPriorityEngine::score(const StreamingObservation& o) const noexcept {
    const float distance=std::max(0.01f,len(o.camera_to_asset)-o.bounding_radius);
    const float inv_distance=1.0f/(1.0f+distance);
    const float to_len=std::max(0.001f,len(o.camera_to_asset));
    const std::array<float,3> dir{o.camera_to_asset[0]/to_len,o.camera_to_asset[1]/to_len,o.camera_to_asset[2]/to_len};
    const float forward=std::clamp(dot(dir,o.camera_forward),-1.0f,1.0f);
    const float velocity_toward=std::max(0.0f,dot(dir,o.player_velocity));
    float s=inv_distance*80.0f + std::max(0.0f,forward)*18.0f + std::min(velocity_toward,60.0f)*0.25f;
    if(o.inside_frustum) s+=35.0f;
    if(o.mission_critical) s+=1000.0f;
    ResourcePriority p=ResourcePriority::Low;
    if(s>=80.0f) p=ResourcePriority::Critical; else if(s>=45.0f) p=ResourcePriority::High; else if(s>=20.0f) p=ResourcePriority::Normal;
    return {s,p};
}
}
