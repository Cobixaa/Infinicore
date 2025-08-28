#include "arch4.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>

namespace arch4 {

static void apply_rlimits(const SandboxLimits &limits) {
    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max = limits.memory_limit_bytes;
    setrlimit(RLIMIT_AS, &rl);

    // CPU time seconds: convert ms to seconds rounded up
    rlim_t secs = (limits.time_limit_ms + 999) / 1000;
    if (secs == 0) secs = 1;
    rl.rlim_cur = rl.rlim_max = secs;
    setrlimit(RLIMIT_CPU, &rl);
}

std::optional<std::string> sandbox_exec(const std::vector<std::string> &cmd,
                                        const SandboxLimits &limits,
                                        int &exit_code,
                                        std::string &err) {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        err = "pipe failed";
        return std::nullopt;
    }

    pid_t pid = fork();
    if (pid < 0) {
        err = "fork failed";
        return std::nullopt;
    }

    if (pid == 0) {
        // Child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        // Apply limits
        apply_rlimits(limits);

        // Wall-clock safety: alarm as a backstop (seconds)
        unsigned int secs = (unsigned int)((limits.time_limit_ms + 999) / 1000);
        if (secs == 0) secs = 1;
        alarm(secs);

        // Best-effort: drop network by unsetting env and using no net syscalls (Termux limited)
        unsetenv("http_proxy"); unsetenv("https_proxy"); unsetenv("no_proxy");

        // Build argv
        std::vector<char*> argv;
        argv.reserve(cmd.size()+1);
        for (const auto &s : cmd) argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    // Parent
    close(pipefd[1]);
    std::string output;
    char buf[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) output.append(buf, buf + n);
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
    } else {
        exit_code = -1;
    }
    return output;
}

}

