#include "arch4.hpp"
// C headers for ABI helpers
#include <cstring>
#include <cstdlib>
extern "C" {

// Minimal C ABI for Python ctypes
// Opaque handles
typedef void* arch4_manager_t;
typedef void* arch4_router_t;
typedef void* arch4_aggregator_t;

arch4_manager_t arch4_manager_create(const char* state_dir) {
    try { return new arch4::ModuleManager(state_dir ? state_dir : "arch4/state"); }
    catch (...) { return nullptr; }
}

void arch4_manager_destroy(arch4_manager_t h) {
    if (!h) return; delete reinterpret_cast<arch4::ModuleManager*>(h);
}

int arch4_manager_install(arch4_manager_t h, const char* patch_dir, char* errbuf, int errbuf_sz) {
    if (!h || !patch_dir) return -1;
    std::string err;
    bool ok = reinterpret_cast<arch4::ModuleManager*>(h)->install(patch_dir, err);
    if (!ok && errbuf && errbuf_sz>0) {
        std::snprintf(errbuf, (size_t)errbuf_sz, "%s", err.c_str());
    }
    return ok ? 0 : -2;
}

// Execute by patch id; returns newly allocated C string (caller frees via free())
char* arch4_manager_run(arch4_manager_t h, const char* patch_id, const char* input, int time_limit_ms) {
    if (!h || !patch_id || !input) return nullptr;
    auto out = reinterpret_cast<arch4::ModuleManager*>(h)->run(patch_id, input, time_limit_ms);
    if (!out) return nullptr;
    char* c = (char*)malloc(out->size()+1);
    if (!c) return nullptr;
    std::memcpy(c, out->c_str(), out->size()+1);
    return c;
}

arch4_router_t arch4_router_create(arch4_manager_t mgr) {
    if (!mgr) return nullptr;
    try { return new arch4::Router(arch4::RouterConfig{}, *reinterpret_cast<arch4::ModuleManager*>(mgr)); }
    catch (...) { return nullptr; }
}

void arch4_router_destroy(arch4_router_t h) { if (!h) return; delete reinterpret_cast<arch4::Router*>(h); }

// Route and run aggregate; returns allocated C string answer
char* arch4_route_answer(arch4_router_t r, arch4_manager_t m, const char* query) {
    if (!r || !m || !query) return nullptr;
    auto &router = *reinterpret_cast<arch4::Router*>(r);
    auto &mm = *reinterpret_cast<arch4::ModuleManager*>(m);
    auto cands = router.route(query);
    std::vector<std::pair<arch4::PatchRecord,std::string>> outs;
    for (const auto &c : cands) {
        auto out = mm.run(c.record.meta.id, query, 1200);
        if (out) outs.push_back({c.record, *out});
    }
    arch4::Aggregator agg;
    auto ans = agg.aggregate(outs);
    char* c = (char*)malloc(ans.text.size()+1);
    if (!c) return nullptr;
    std::memcpy(c, ans.text.c_str(), ans.text.size()+1);
    return c;
}

}

