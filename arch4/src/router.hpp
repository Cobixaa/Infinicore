#pragma once

#include "common.hpp"
#include "module_manager.hpp"

namespace arch4 {

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
    // Extremely small bag-of-words cosine similarity as placeholder ANN
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

}

