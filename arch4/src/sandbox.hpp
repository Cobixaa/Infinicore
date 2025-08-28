#pragma once

#include "common.hpp"

namespace arch4 {

struct SandboxLimits {
    int time_limit_ms = 1500;
    size_t memory_limit_bytes = 256 * 1024 * 1024; // 256MB
    bool disable_network = true;
};

// Executes a command with arguments in a constrained subprocess. Captures stdout.
// On Termux/Android where namespaces may be limited, we best-effort apply rlimits only.
std::optional<std::string> sandbox_exec(const std::vector<std::string> &cmd,
                                        const SandboxLimits &limits,
                                        int &exit_code,
                                        std::string &err);

}

