#include "arch4.hpp"

namespace fs = std::filesystem;

namespace arch4 {

static bool parse_jsonl(const std::string &content, std::vector<std::pair<std::string,std::string>> &io_pairs) {
    // Extremely small tolerant parser: expects lines with {"input":"...","expected":"..."}
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        line = trim_copy(line);
        if (line.empty()) continue;
        auto in_pos = line.find("\"input\"");
        auto ex_pos = line.find("\"expected\"");
        if (in_pos == std::string::npos || ex_pos == std::string::npos) continue;
        auto q1 = line.find('"', line.find(':', in_pos)+1);
        auto q2 = line.find('"', q1+1);
        auto q3 = line.find('"', line.find(':', ex_pos)+1);
        auto q4 = line.find('"', q3+1);
        if (q1==std::string::npos||q2==std::string::npos||q3==std::string::npos||q4==std::string::npos) continue;
        std::string in = line.substr(q1+1, q2-q1-1);
        std::string ex = line.substr(q3+1, q4-q3-1);
        io_pairs.emplace_back(in, ex);
    }
    return !io_pairs.empty();
}

TestRunner::TestRunner(ModuleManager &mm): mm_(mm) {}

bool TestRunner::run_tests(const PatchMeta &meta, const std::string &install_path, TestResult &result, std::string &err) {
    result = {};
    std::string test_path = install_path + "/" + meta.tests_path;
    std::string tests = read_file_to_string(test_path);
    if (tests.empty()) { err = "No tests found at " + test_path; return false; }
    std::vector<std::pair<std::string,std::string>> io_pairs;
    if (!parse_jsonl(tests, io_pairs)) { err = "Failed to parse tests"; return false; }
    result.total = io_pairs.size();
    SandboxLimits limits; limits.time_limit_ms = 1200; limits.memory_limit_bytes = 256*1024*1024;
    std::string entry = install_path + "/" + meta.entry_point;
    for (const auto &p : io_pairs) {
        std::vector<std::string> cmd;
        if (entry.size()>=3 && entry.substr(entry.size()-3)==".py") {
            cmd = {"python3", entry, p.first};
        } else {
            cmd = {entry, p.first};
        }
        int exit_code = 0; std::string err2;
        auto out = sandbox_exec(cmd, limits, exit_code, err2);
        if (!out) continue;
        std::string got = trim_copy(*out);
        std::string expected = trim_copy(p.second);
        if (to_lower_copy(got) == to_lower_copy(expected)) result.passed++;
        if (expected.empty() && !got.empty()) result.passed++;
    }
    // Clamp pass count in case of wildcard increments beyond total
    if (result.passed > result.total) result.passed = result.total;
    return true;
}

}

