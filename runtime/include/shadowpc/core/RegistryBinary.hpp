#pragma once
#include "shadowpc/core/AssetRegistry.hpp"
#include <filesystem>
#include <string>
namespace shadowpc {
struct RegistryIoResult { bool ok{}; std::string error; };
RegistryIoResult save_registry_binary(const AssetRegistry&, const std::filesystem::path&);
RegistryIoResult load_registry_binary(AssetRegistry&, const std::filesystem::path&);
}
