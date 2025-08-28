#pragma once

#include "common.hpp"

namespace arch4 {

struct PatchMeta {
    std::string id;                // unique id
    std::string name;              // human readable
    std::string version;           // semver-like
    std::string author;
    std::vector<std::string> tags; // language, domain
    double trust_score = 0.5;      // [0,1]
    std::string entry_point;       // path to executable/script inside patch dir
    std::string tests_path;        // path to tests file
};

struct PatchRecord {
    PatchMeta meta;
    std::string install_path;
    bool enabled = false;
    double test_pass_rate = 0.0;
    std::string last_verified_at;
};

class ModuleManager {
public:
    explicit ModuleManager(const std::string &state_dir);

    // Discover patches within a directory (each subdir has manifest.json)
    std::vector<PatchMeta> discover(const std::string &dir);

    // Install: verify signature (dev placeholder), run sandboxed tests, register
    bool install(const std::string &patch_dir, std::string &err);

    // Enable/Disable registered patch
    bool enable(const std::string &patch_id);
    bool disable(const std::string &patch_id);

    // Get registry snapshot
    std::vector<PatchRecord> list() const;
    std::optional<PatchRecord> get(const std::string &patch_id) const;

    // Execute a patch with input; returns output text (sandboxed)
    std::optional<std::string> run(const std::string &patch_id, const std::string &input, int time_limit_ms = 1500);

    // Persistence
    void load_registry();
    void save_registry() const;

private:
    std::string state_dir_;
    std::unordered_map<std::string, PatchRecord> registry_;

    std::optional<PatchMeta> read_manifest(const std::string &patch_dir);
    bool run_tests(const PatchMeta &meta, const std::string &install_path, double &pass_rate, std::string &err);
};

}

