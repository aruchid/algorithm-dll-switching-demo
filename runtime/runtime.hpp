#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>
// Serializes calls and switching; instance is destroyed before its library.
// A failed candidate never replaces the active plugin.
class Runtime {
public:
    Runtime();
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    void load(const std::filesystem::path& path);
    std::vector<float> process(const std::vector<float>& input);
    void release();
private:
    struct Instance;
    std::unique_ptr<Instance> active_;
    std::mutex mutex_;
};
