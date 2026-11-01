#include "runtime.hpp"
#include "algorithm_plugin.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif
struct Runtime::Instance {
#ifdef _WIN32
    HMODULE library = nullptr;
#else
    void* library = nullptr;
#endif
    AlgorithmApi table{};
    const AlgorithmApi* api = nullptr;
    void* object = nullptr;
    ~Instance() {
        if (object && api) api->destroy(object);
#ifdef _WIN32
        if (library) FreeLibrary(library);
#else
        if (library) dlclose(library);
#endif
    }
};
Runtime::Runtime() = default;
Runtime::~Runtime() = default;
void Runtime::load(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto candidate = std::make_unique<Instance>();
    const auto absolute = std::filesystem::absolute(path);
#ifdef _WIN32
    candidate->library = LoadLibraryExW(absolute.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!candidate->library)
        throw std::runtime_error("LoadLibraryExW failed: " + std::to_string(GetLastError()));
    auto entry = reinterpret_cast<AlgorithmGetApi>(GetProcAddress(candidate->library, "algorithm_get_api"));
#else
    candidate->library = dlopen(absolute.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!candidate->library) throw std::runtime_error(std::string("dlopen: ") + dlerror());
    auto entry = reinterpret_cast<AlgorithmGetApi>(dlsym(candidate->library, "algorithm_get_api"));
#endif
    if (!entry) throw std::runtime_error("Missing symbol: algorithm_get_api");
    if (entry(ALGO_ABI_V1, &candidate->table, sizeof(AlgorithmApi)) != 0)
        throw std::runtime_error("Plugin API query failed");
    const auto* api = &candidate->table;
    if ( api->abi_version != ALGO_ABI_V1 || api->struct_size < sizeof(AlgorithmApi))
        throw std::runtime_error("Incompatible plugin ABI");
    if (!api->create || !api->destroy || !api->process)
        throw std::runtime_error("Incomplete plugin function table");
    candidate->api = api;
    if (api->create(&candidate->object) != 0 || !candidate->object)
        throw std::runtime_error("Plugin create failed");
    const float input = 1;
    float output = std::numeric_limits<float>::quiet_NaN();
    if (api->process(candidate->object, &input, 1, &output, 1) != 0 || !std::isfinite(output))
        throw std::runtime_error("Plugin preflight failed");
    active_.swap(candidate);
}
std::vector<float> Runtime::process(const std::vector<float>& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) throw std::runtime_error("No active plugin");
    if (input.empty() || input.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("Invalid input length");
    for (float v : input) if (!std::isfinite(v)) throw std::runtime_error("Non-finite input");
    std::vector<float> output(input.size(), std::numeric_limits<float>::quiet_NaN());
    if (active_->api->process(active_->object, input.data(), static_cast<uint32_t>(input.size()),
                             output.data(), static_cast<uint32_t>(output.size())) != 0)
        throw std::runtime_error("Plugin process failed");
    for (float v : output) if (!std::isfinite(v)) throw std::runtime_error("Non-finite plugin output");
    return output;
}
void Runtime::release() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_.reset();
}
