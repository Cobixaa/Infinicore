#include "common.hpp"
#include "module_manager.hpp"
#include "router.hpp"
#include "aggregator.hpp"

using namespace arch4;

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::string query = argv[1];
    ModuleManager mm("arch4/state");
    // Ensure examples are installed on first run
    std::string examples_dir = "arch4/examples/patches";
    for (auto &meta : mm.discover(examples_dir)) {
        if (mm.get(meta.id)) continue;
        std::string err;
        mm.install(examples_dir + "/" + meta.id, err);
    }
    Router router({.ann_top_k=50, .final_top_m=3}, mm);
    Aggregator agg;
    auto cands = router.route(query);
    std::vector<std::pair<PatchRecord,std::string>> outs;
    for (const auto &c : cands) {
        auto out = mm.run(c.record.meta.id, query, 1200);
        if (out) outs.push_back({c.record, *out});
    }
    auto ans = agg.aggregate(outs);
    std::cout << (ans.text.empty() ? "" : ans.text) << std::endl;
    return 0;
}

