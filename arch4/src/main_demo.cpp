#include "arch4.hpp"

using namespace arch4;

static void println(const std::string &s){ std::cout << s << std::endl; }

int main() {
    std::string state_dir = "arch4/state";
    ModuleManager mm(state_dir);

    // Auto-install example patches from examples/patches if not already present
    std::string examples_dir = "arch4/examples/patches";
    for (auto &meta : mm.discover(examples_dir)) {
        if (mm.get(meta.id)) continue;
        std::string err;
        if (!mm.install(examples_dir + "/" + meta.id, err)) {
            println("Install failed for " + meta.id + ": " + err);
        } else {
            println("Installed patch: " + meta.id);
        }
    }

    RouterConfig cfg; cfg.final_top_m = 3; cfg.ann_top_k = 50;
    Router router(cfg, mm);
    Aggregator agg;

    println("ARCH4 demo chatbot. Type 'exit' to quit.");
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        line = trim_copy(line);
        if (line=="exit" || line=="quit") break;
        if (line.empty()) continue;

        auto cands = router.route(line);
        std::vector<std::pair<PatchRecord,std::string>> outs;
        for (const auto &c : cands) {
            auto out = mm.run(c.record.meta.id, line, 1200);
            if (out) outs.push_back({c.record, *out});
        }
        auto answer = agg.aggregate(outs);
        println(answer.text.empty() ? "(no answer)" : answer.text);
    }
    return 0;
}

