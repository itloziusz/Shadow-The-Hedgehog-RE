#pragma once

#include "shadow/gc/Motion.hpp"
#include "shadow/gc/RenderWare.hpp"
#include "shadow/pc/NativeAssets.hpp"

namespace shadow::pc {

NativeModel to_native_model(const gc::Clump& clump, std::string name);
NativeTextureDictionary to_native_textures(const gc::TexDictionary& dictionary, std::string name);
NativeWorld to_native_world(const gc::World& world, std::string name);
NativeBon to_native_bon(const gc::BonFile& bon, std::string name);
NativeMotion to_native_motion(const gc::Motion& motion, std::string name);
NativeMotionPack to_native_motion_pack(const gc::MotionPack& pack, std::string name);

}  // namespace shadow::pc
