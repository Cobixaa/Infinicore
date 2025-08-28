#include "arch4.hpp"

namespace fs = std::filesystem;

namespace arch4 {

static std::optional<PatchMeta> parse_manifest(const std::string &json_text) {
    PatchMeta m;
    auto get_val = [&](const std::string &key)->std::optional<std::string>{
        auto p = json_text.find("\""+key+"\""); if (p==std::string::npos) return std::nullopt;
        auto c = json_text.find(':', p); if (c==std::string::npos) return std::nullopt;
        auto q1 = json_text.find('"', c+1); if (q1==std::string::npos) return std::nullopt;
        auto q2 = json_text.find('"', q1+1); if (q2==std::string::npos) return std::nullopt;
        return json_text.substr(q1+1, q2-q1-1);
    };
    auto id = get_val("id"); if (!id) return std::nullopt; m.id = *id;
    if (auto v=get_val("name")) m.name=*v; else m.name=m.id;
    if (auto v=get_val("version")) m.version=*v; else m.version="0.0.0";
    if (auto v=get_val("author")) m.author=*v; else m.author="unknown";
    if (auto v=get_val("entry_point")) m.entry_point=*v; else return std::nullopt;
    if (auto v=get_val("tests")) m.tests_path=*v; else m.tests_path="tests.jsonl";
    // Parse tags as comma-separated for simplicity
    auto tpos = json_text.find("\"tags\"");
    if (tpos != std::string::npos) {
        auto b = json_text.find('[', tpos); auto e = json_text.find(']', b);
        if (b!=std::string::npos && e!=std::string::npos) {
            std::string arr = json_text.substr(b+1, e-b-1);
            for (auto &x : split(arr, ',')) {
                std::string s = trim_copy(x);
                if (!s.empty() && s.front()=='"' && s.back()=='"') s = s.substr(1, s.size()-2);
                if (!s.empty()) m.tags.push_back(s);
            }
        }
    }
    auto trust = get_val("trust_score");
    if (trust) { try { m.trust_score = std::stod(*trust); } catch(...) {} }
    return m;
}

ModuleManager::ModuleManager(const std::string &state_dir): state_dir_(state_dir) {
    std::filesystem::create_directories(state_dir_ + "/registry");
    load_registry();
}

std::vector<PatchMeta> ModuleManager::discover(const std::string &dir) {
    std::vector<PatchMeta> metas;
    std::error_code ec;
    if (!fs::exists(dir, ec)) return metas;
    for (auto &p : fs::directory_iterator(dir)) {
        if (!p.is_directory()) continue;
        auto manifest = p.path().string() + "/manifest.json";
        if (fs::exists(manifest)) {
            auto meta = read_manifest(p.path().string());
            if (meta) metas.push_back(*meta);
        }
    }
    return metas;
}

std::optional<PatchMeta> ModuleManager::read_manifest(const std::string &patch_dir) {
    auto text = read_file_to_string(patch_dir + "/manifest.json");
    if (text.empty()) return std::nullopt;
    return parse_manifest(text);
}

bool ModuleManager::run_tests(const PatchMeta &meta, const std::string &install_path, double &pass_rate, std::string &err) {
    TestRunner tr(*this);
    TestResult r;
    if (!tr.run_tests(meta, install_path, r, err)) return false;
    pass_rate = r.total ? (double)r.passed / (double)r.total : 0.0;
    return true;
}

bool ModuleManager::install(const std::string &patch_dir, std::string &err) {
    auto meta_opt = read_manifest(patch_dir);
    if (!meta_opt) { err = "manifest parse failed"; return false; }
    auto meta = *meta_opt;

    // Placeholder signature verification (dev mode)
    const char *require_sig = std::getenv("ARCH4_REQUIRE_SIG");
    if (require_sig && std::string(require_sig)=="1") {
        auto sig = read_file_to_string(patch_dir + "/signature.txt");
        if (sig.empty()) { err = "Missing signature.txt"; return false; }
    }

    // Copy to state/registry/<id>@<version>
    std::string dst = state_dir_ + "/registry/" + meta.id + "@" + meta.version;
    std::error_code ec;
    fs::create_directories(dst, ec);
    for (auto &p : fs::recursive_directory_iterator(patch_dir)) {
        auto rel = fs::relative(p.path(), patch_dir, ec);
        auto target = fs::path(dst) / rel;
        if (p.is_directory()) { fs::create_directories(target, ec); continue; }
        fs::create_directories(target.parent_path(), ec);
        fs::copy_file(p.path(), target, fs::copy_options::overwrite_existing, ec);
    }

    // Run tests sandboxed
    double pass_rate = 0.0;
    if (!run_tests(meta, dst, pass_rate, err)) return false;
    if (pass_rate < 0.95) { err = "Test pass rate below threshold: " + std::to_string(pass_rate); return false; }

    PatchRecord rec; rec.meta = meta; rec.install_path = dst; rec.enabled = true; rec.test_pass_rate = pass_rate; rec.last_verified_at = now_iso8601();
    registry_[meta.id] = rec;
    save_registry();
    return true;
}

bool ModuleManager::enable(const std::string &patch_id) {
    auto it = registry_.find(patch_id); if (it==registry_.end()) return false; it->second.enabled = true; save_registry(); return true;
}

bool ModuleManager::disable(const std::string &patch_id) {
    auto it = registry_.find(patch_id); if (it==registry_.end()) return false; it->second.enabled = false; save_registry(); return true;
}

std::vector<PatchRecord> ModuleManager::list() const {
    std::vector<PatchRecord> v; v.reserve(registry_.size()); for (auto &kv : registry_) v.push_back(kv.second); return v;
}

std::optional<PatchRecord> ModuleManager::get(const std::string &patch_id) const {
    auto it = registry_.find(patch_id); if (it==registry_.end()) return std::nullopt; return it->second;
}

std::optional<std::string> ModuleManager::run(const std::string &patch_id, const std::string &input, int time_limit_ms) {
    auto it = registry_.find(patch_id); if (it==registry_.end()) return std::nullopt;
    auto &rec = it->second;
    std::string entry = rec.install_path + "/" + rec.meta.entry_point;
    // Support Python and native executables
    std::vector<std::string> cmd;
    if (entry.size()>=3 && entry.substr(entry.size()-3)==".py") {
        cmd = {"python3", entry, input};
    } else {
        cmd = {entry, input};
    }
    SandboxLimits limits; limits.time_limit_ms = time_limit_ms;
    int exit_code = 0; std::string err;
    auto out = sandbox_exec(cmd, limits, exit_code, err);
    if (!out) return std::nullopt;
    return *out;
}

void ModuleManager::load_registry() {
    registry_.clear();
    auto reg_dir = state_dir_ + "/registry";
    std::error_code ec;
    if (!fs::exists(reg_dir)) return;
    for (auto &p : fs::directory_iterator(reg_dir)) {
        if (!p.is_directory()) continue;
        auto manifest = p.path().string() + "/manifest.json";
        if (!fs::exists(manifest)) continue;
        auto meta_opt = read_manifest(p.path().string());
        if (!meta_opt) continue;
        PatchRecord rec; rec.meta = *meta_opt; rec.install_path = p.path().string(); rec.enabled = true; rec.test_pass_rate = 1.0; rec.last_verified_at = now_iso8601();
        registry_[rec.meta.id] = rec;
    }
}

void ModuleManager::save_registry() const {
    // Lightweight persistence as a single text file
    std::ostringstream oss;
    for (const auto &kv : registry_) {
        const auto &r = kv.second;
        oss << r.meta.id << "\t" << r.meta.version << "\t" << r.enabled << "\t" << r.test_pass_rate << "\t" << r.install_path << "\n";
    }
    write_string_to_file(state_dir_ + "/registry_index.tsv", oss.str());
}

}

