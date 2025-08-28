#pragma once

// Unified ARCH4 header combining common utilities, sandbox, module manager,
// router, aggregator, and test runner APIs into a single include.

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <random>
#include <filesystem>
#include <cmath>

namespace arch4 {

// ===== common (inline utils) =====
inline std::string read_file_to_string(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline bool write_string_to_file(const std::string &path, const std::string &data) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << data;
    return true;
}

inline std::string trim_copy(const std::string &s) {
    size_t start = 0; while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size(); while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) end--;
    return s.substr(start, end - start);
}

inline std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out; std::string item; std::istringstream iss(s);
    while (std::getline(iss, item, delim)) out.push_back(item);
    return out;
}

inline std::vector<std::string> tokenize_words(const std::string &s) {
    std::vector<std::string> tokens; tokens.reserve(64);
    std::string cur;
    for (char ch : s) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch=='_' ) {
            cur.push_back(std::tolower(static_cast<unsigned char>(ch)));
        } else {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

inline std::string now_iso8601() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

inline std::string join(const std::vector<std::string> &v, const std::string &sep) {
    std::ostringstream oss; for (size_t i=0;i<v.size();++i){ if(i) oss<<sep; oss<<v[i]; } return oss.str();
}

// ===== sandbox =====
struct SandboxLimits {
    int time_limit_ms = 1500;
    size_t memory_limit_bytes = 256 * 1024 * 1024; // 256MB
    bool disable_network = true;
};

std::optional<std::string> sandbox_exec(const std::vector<std::string> &cmd,
                                        const SandboxLimits &limits,
                                        int &exit_code,
                                        std::string &err);

// ===== module manager =====
struct PatchMeta {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::vector<std::string> tags;
    double trust_score = 0.5;
    std::string entry_point;
    std::string tests_path;
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
    std::vector<PatchMeta> discover(const std::string &dir);
    bool install(const std::string &patch_dir, std::string &err);
    bool enable(const std::string &patch_id);
    bool disable(const std::string &patch_id);
    std::vector<PatchRecord> list() const;
    std::optional<PatchRecord> get(const std::string &patch_id) const;
    std::optional<std::string> run(const std::string &patch_id, const std::string &input, int time_limit_ms = 1500);
    void load_registry();
    void save_registry() const;
private:
    std::string state_dir_;
    std::unordered_map<std::string, PatchRecord> registry_;
    std::optional<PatchMeta> read_manifest(const std::string &patch_dir);
    bool run_tests(const PatchMeta &meta, const std::string &install_path, double &pass_rate, std::string &err);
};

// ===== router =====
struct RouteCandidate {
    PatchRecord record;
    double meta_score = 0.0;
    double graph_score = 0.0;
    double ann_score = 0.0;
    double selector_score = 0.0;
    double total_score = 0.0;
};

struct RouterConfig {
    size_t ann_top_k = 50;
    size_t final_top_m = 2;
    double alpha = 0.4, beta = 0.2, gamma = 0.2, zeta = 0.2;
};

class SimpleANN {
public:
    static std::unordered_map<std::string, double> embed(const std::string &text);
    static double cosine(const std::unordered_map<std::string,double> &a,
                         const std::unordered_map<std::string,double> &b);
};

class Router {
public:
    Router(const RouterConfig &cfg, ModuleManager &mm);
    std::vector<RouteCandidate> route(const std::string &query_text);
private:
    RouterConfig cfg_;
    ModuleManager &mm_;
    std::unordered_map<std::string, std::unordered_set<std::string>> seed_cache_;
    const std::unordered_set<std::string>& seeds_for(const PatchRecord &rec);
};

// ===== aggregator =====
struct AggregatedAnswer {
    std::string text;
    std::vector<std::pair<std::string,double>> provenance;
};

class Aggregator {
public:
    AggregatedAnswer aggregate(const std::vector<std::pair<PatchRecord,std::string>> &outputs) const;
};

// ===== tests =====
struct TestResult { size_t total=0; size_t passed=0; };
class TestRunner {
public:
    explicit TestRunner(ModuleManager &mm);
    bool run_tests(const PatchMeta &meta, const std::string &install_path, TestResult &result, std::string &err);
private:
    ModuleManager &mm_;
};

}

