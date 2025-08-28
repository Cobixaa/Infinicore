#include "arch4.hpp"

namespace arch4 {

AggregatedAnswer Aggregator::aggregate(const std::vector<std::pair<PatchRecord,std::string>> &outputs) const {
    // Normalize: lowercase trim
    std::unordered_map<std::string, double> votes;
    std::unordered_map<std::string, std::vector<std::pair<std::string,double>>> prov;
    bool has_nontrivial = false;
    for (const auto &pr : outputs) {
        auto norm = to_lower_copy(trim_copy(pr.second));
        bool trivial = (norm == "unknown" || norm == "error" || norm.empty());
        if (!trivial) has_nontrivial = true;
        double w = 0.5 * pr.first.meta.trust_score + 0.5 * pr.first.test_pass_rate;
        if (trivial) w *= 0.5; // initial penalty for trivial answers
        votes[norm] += w;
        prov[norm].push_back({pr.first.meta.id, w});
    }
    if (votes.empty()) return {"", {}};
    if (has_nontrivial) {
        // further penalize trivial buckets if any nontrivial exists
        for (auto &kv : votes) {
            if (kv.first == "unknown" || kv.first == "error" || kv.first.empty()) {
                kv.second *= 0.3;
            }
        }
    }
    auto best = std::max_element(votes.begin(), votes.end(), [](auto &a, auto &b){ return a.second < b.second; });
    std::string text = best->first;
    auto sources = prov[text];
    // If ambiguous, include a short provenance trailer
    if (votes.size() > 1) {
        std::ostringstream oss;
        oss << text << "\n\n" << "Provenance: ";
        for (size_t i=0;i<sources.size();++i) {
            if (i) oss << ", ";
            oss << sources[i].first << "(" << sources[i].second << ")";
        }
        text = oss.str();
    }
    return {text, sources};
}

}

