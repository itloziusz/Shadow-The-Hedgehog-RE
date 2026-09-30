#include "shadowpc/core/RegistryBinary.hpp"
#include <fstream>
#include <limits>
namespace shadowpc {
namespace {
template<class T> bool write_pod(std::ofstream& f,const T& v){f.write(reinterpret_cast<const char*>(&v),sizeof(v));return bool(f);}
template<class T> bool read_pod(std::ifstream& f,T& v){f.read(reinterpret_cast<char*>(&v),sizeof(v));return bool(f);}
bool write_string(std::ofstream& f,const std::string& s){std::uint32_t n=static_cast<std::uint32_t>(s.size());return write_pod(f,n)&&bool(f.write(s.data(),n));}
bool read_string(std::ifstream& f,std::string& s){std::uint32_t n{};if(!read_pod(f,n)||n>(1u<<24))return false;s.resize(n);return n==0||bool(f.read(s.data(),n));}
}
RegistryIoResult save_registry_binary(const AssetRegistry& r,const std::filesystem::path& p){
 try{std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("open failed");const std::uint32_t magic=0x47455253u;const std::uint32_t version=1;const std::uint64_t count=r.size();write_pod(f,magic);write_pod(f,version);write_pod(f,count);for(const auto& a:r.records()){write_pod(f,a.id);write_pod(f,a.guid);write_pod(f,a.type);write_pod(f,a.codec);write_pod(f,a.container_offset);write_pod(f,a.stored_size);write_pod(f,a.uncompressed_size);write_pod(f,a.content_hash);write_string(f,a.canonical_path);write_string(f,a.container_path);std::uint32_t n=static_cast<std::uint32_t>(a.dependencies.size());write_pod(f,n);for(auto d:a.dependencies)write_pod(f,d);}if(!f)throw std::runtime_error("write failed");return{true,{}};}catch(const std::exception&e){return{false,e.what()};}
}
RegistryIoResult load_registry_binary(AssetRegistry& r,const std::filesystem::path& p){
 try{std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("open failed");std::uint32_t magic{},version{};std::uint64_t count{};if(!read_pod(f,magic)||!read_pod(f,version)||!read_pod(f,count)||magic!=0x47455253u||version!=1||count>(1ull<<30))throw std::runtime_error("invalid registry header");for(std::uint64_t i=0;i<count;++i){AssetRecord a;read_pod(f,a.id);read_pod(f,a.guid);read_pod(f,a.type);read_pod(f,a.codec);read_pod(f,a.container_offset);read_pod(f,a.stored_size);read_pod(f,a.uncompressed_size);read_pod(f,a.content_hash);if(!read_string(f,a.canonical_path)||!read_string(f,a.container_path))throw std::runtime_error("invalid string");std::uint32_t n{};if(!read_pod(f,n)||n>(1u<<20))throw std::runtime_error("invalid dependency count");a.dependencies.resize(n);for(auto& d:a.dependencies)if(!read_pod(f,d))throw std::runtime_error("truncated dependencies");if(!r.add(std::move(a)))throw std::runtime_error("duplicate asset");}return{true,{}};}catch(const std::exception&e){return{false,e.what()};}
}
}
