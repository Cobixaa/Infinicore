#pragma once

#include "common.hpp"
#include "module_manager.hpp"

namespace arch4 {

struct TestResult { size_t total=0; size_t passed=0; };

// Tests are small JSONL: {"input":"...","expected":"..."}
class TestRunner {
public:
    explicit TestRunner(ModuleManager &mm);
    bool run_tests(const PatchMeta &meta, const std::string &install_path, TestResult &result, std::string &err);
private:
    ModuleManager &mm_;
};

}

