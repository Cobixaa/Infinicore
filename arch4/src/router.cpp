#include "router.hpp"

namespace fs = std::filesystem;

namespace arch4 {

std::unordered_map<std::string,double> SimpleANN::embed(const std::string &text) {
    std::unordered_map<std::string,double> v;
    for (const auto &w : tokenize_words(text)) v[w] += 1.0;
    double norm = 0.0; for (auto &kv : v) norm += kv.second * kv.second; norm = std::max(1e-9, std::sqrt(norm));
    for (auto &kv : v) kv.second /= norm;
    return v;
}

double SimpleANN::cosine(const std::unordered_map<std::string,double> &a,
                         const std::unordered_map<std::string,double> &b) {
    const auto *pa = &a; const auto *pb = &b;
    if (pa->size() > pb->size()) std::swap(pa, pb);
    double dot = 0.0;
    for (const auto &kv : *pa) {
        auto it = pb->find(kv.first);
        if (it != pb->end()) dot += kv.second * it->second;
    }
    return dot;
}

Router::Router(const RouterConfig &cfg, ModuleManager &mm): cfg_(cfg), mm_(mm) {}

std::vector<RouteCandidate> Router::route(const std::string &query_text) {
    auto qemb = SimpleANN::embed(query_text);
    auto qtokens = tokenize_words(query_text);
    std::unordered_set<std::string> qset(qtokens.begin(), qtokens.end());

    std::vector<RouteCandidate> eligible;
    std::vector<RouteCandidate> others;

    for (const auto &rec : mm_.list()) {
        if (!rec.enabled) continue;
        double meta = 0.0;
        for (const auto &tag : rec.meta.tags) {
            auto t = to_lower_copy(tag);
            if (qset.count(t)) meta += 0.6; // stronger boost for direct tag hits
        }

        // ANN similarity on id+name+tags
        auto pemb = SimpleANN::embed(rec.meta.name + " " + rec.meta.id + " " + join(rec.meta.tags, " "));
        double ann = SimpleANN::cosine(qemb, pemb);

        // Graph score: id/name hits and concept seeds from tests
        double graph = 0.0;
        std::string low_q = to_lower_copy(query_text);
        if (low_q.find(to_lower_copy(rec.meta.name)) != std::string::npos) graph += 0.4;
        if (low_q.find(to_lower_copy(rec.meta.id)) != std::string::npos) graph += 0.4;
        // Concept seeds from tests.jsonl
        std::string tests_path = rec.install_path + "/" + rec.meta.tests_path;
        std::string tests = read_file_to_string(tests_path);
        if (!tests.empty()) {
            std::istringstream iss(tests);
            std::string line;
            std::unordered_set<std::string> seeds;
            while (std::getline(iss, line)) {
                for (auto &tok : tokenize_words(line)) {
                    if (tok.size() >= 3) seeds.insert(tok);
                }
            }
            int hits = 0;
            for (const auto &t : seeds) if (qset.count(t)) hits++;
            graph += std::min(1, hits) * 0.6; // single-hit strong boost
        }

        double selector = 0.5 * ann + 0.5 * meta; // cheap MLP placeholder

        double total = cfg_.alpha*ann + cfg_.beta*graph + cfg_.gamma*rec.meta.trust_score + cfg_.zeta*selector;
        RouteCandidate cand{rec, meta, graph, ann, selector, total};

        bool is_eligible = (meta > 0.0) || (graph > 0.0) || (ann > 0.05);
        if (is_eligible) eligible.push_back(cand); else others.push_back(cand);
    }

    auto sorter = [](const RouteCandidate &a, const RouteCandidate &b){ return a.total_score > b.total_score; };
    std::sort(eligible.begin(), eligible.end(), sorter);
    std::sort(others.begin(), others.end(), sorter);

    std::vector<RouteCandidate> out;
    for (auto &c : eligible) { if (out.size() < cfg_.final_top_m) out.push_back(c); }
    for (auto &c : others) { if (out.size() < cfg_.final_top_m) out.push_back(c); }
    return out;
}

}

