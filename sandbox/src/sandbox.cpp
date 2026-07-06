#include "ocpe/sandbox.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

namespace ocpe {

Sandbox::Sandbox(SandboxLimits limits) : limits_(limits) {}

std::string Sandbox::trim_output(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        --end;
    }
    return s.substr(start, end - start);
}

bool Sandbox::outputs_match(const std::string& actual, const std::string& expected) {
    return trim_output(actual) == trim_output(expected);
}

SandboxResult Sandbox::run(const std::string& runnable_path,
                           const std::string& interpreter,
                           const std::string& input_data,
                           const std::string& expected_output) {
    SandboxResult result;

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        result.verdict = Verdict::INTERNAL_ERROR;
        result.error_message = "Failed to create pipes";
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        result.verdict = Verdict::INTERNAL_ERROR;
        result.error_message = "fork() failed";
        return result;
    }

    if (pid == 0) {
        // Child process — isolated sandbox
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Memory limit: 256MB default via RLIMIT_AS
        struct rlimit mem_limit;
        mem_limit.rlim_cur = static_cast<rlim_t>(limits_.memory_limit_mb) * 1024 * 1024;
        mem_limit.rlim_max = mem_limit.rlim_cur;
        setrlimit(RLIMIT_AS, &mem_limit);

        // CPU time limit (seconds, rounded up)
        struct rlimit cpu_limit;
        int cpu_sec = (limits_.time_limit_ms + 999) / 1000;
        if (cpu_sec < 1) cpu_sec = 1;
        cpu_limit.rlim_cur = cpu_sec;
        cpu_limit.rlim_max = cpu_sec + 1;
        setrlimit(RLIMIT_CPU, &cpu_limit);

        // Output size limit
        struct rlimit fsize_limit;
        fsize_limit.rlim_cur = static_cast<rlim_t>(limits_.output_limit_kb) * 1024;
        fsize_limit.rlim_max = fsize_limit.rlim_cur;
        setrlimit(RLIMIT_FSIZE, &fsize_limit);

        // Process isolation: new process group
        setpgid(0, 0);

        // Drop privileges — run as nobody if possible
        setgid(65534);
        setuid(65534);

        // Write input to stdin via pipe from parent
        // Parent will write to child's stdin through a separate mechanism
        // For simplicity, use a temp file for input in child
        char input_template[] = "/tmp/ocpe_input_XXXXXX";
        int input_fd = mkstemp(input_template);
        if (input_fd >= 0) {
            write(input_fd, input_data.data(), input_data.size());
            close(input_fd);
            int in_fd = open(input_template, O_RDONLY);
            if (in_fd >= 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            unlink(input_template);
        }

        char* argv[3];
        if (interpreter.empty()) {
            argv[0] = const_cast<char*>(runnable_path.c_str());
            argv[1] = nullptr;
            execv(runnable_path.c_str(), argv);
        } else {
            argv[0] = const_cast<char*>(interpreter.c_str());
            argv[1] = const_cast<char*>(runnable_path.c_str());
            argv[2] = nullptr;
            execv(interpreter.c_str(), argv);
        }

        _exit(127);
    }

    // Parent process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Read stdout and stderr with wall-clock timeout
    std::string stdout_data, stderr_data;
    char buf[4096];
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(limits_.time_limit_ms + 500);

    bool timed_out = false;
    fd_set readfds;

    while (true) {
        if (std::chrono::steady_clock::now() > deadline) {
            timed_out = true;
            kill(-pid, SIGKILL);
            break;
        }

        FD_ZERO(&readfds);
        FD_SET(stdout_pipe[0], &readfds);
        FD_SET(stderr_pipe[0], &readfds);
        int maxfd = std::max(stdout_pipe[0], stderr_pipe[0]);

        struct timeval tv = {0, 100000};  // 100ms poll
        int ready = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);

        if (ready > 0) {
            if (FD_ISSET(stdout_pipe[0], &readfds)) {
                ssize_t n = read(stdout_pipe[0], buf, sizeof(buf));
                if (n > 0) stdout_data.append(buf, n);
                else if (n == 0) FD_CLR(stdout_pipe[0], &readfds);
            }
            if (FD_ISSET(stderr_pipe[0], &readfds)) {
                ssize_t n = read(stderr_pipe[0], buf, sizeof(buf));
                if (n > 0) stderr_data.append(buf, n);
                else if (n == 0) FD_CLR(stderr_pipe[0], &readfds);
            }
        }

        int status_check;
        pid_t w = waitpid(pid, &status_check, WNOHANG);
        if (w == pid) {
            // Drain remaining output
            while (true) {
                ssize_t n = read(stdout_pipe[0], buf, sizeof(buf));
                if (n <= 0) break;
                stdout_data.append(buf, n);
            }
            while (true) {
                ssize_t n = read(stderr_pipe[0], buf, sizeof(buf));
                if (n <= 0) break;
                stderr_data.append(buf, n);
            }
            break;
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    struct rusage usage{};
    wait4(pid, &status, 0, &usage);

    result.stdout_output = stdout_data;
    result.stderr_output = stderr_data;
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    // CPU time from wait4 rusage (user + system time in microseconds)
    long cpu_us = usage.ru_utime.tv_sec * 1000000L + usage.ru_utime.tv_usec +
                  usage.ru_stime.tv_sec * 1000000L + usage.ru_stime.tv_usec;
    result.time_ms = static_cast<int>(cpu_us / 1000);

    // Peak memory from wait4 rusage (maxrss in KB on Linux)
    result.memory_kb = static_cast<int>(usage.ru_maxrss);

    if (timed_out) {
        result.verdict = Verdict::TIME_LIMIT_EXCEEDED;
        result.error_message = "Time limit exceeded";
        return result;
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGXCPU || sig == SIGKILL) {
            result.verdict = Verdict::TIME_LIMIT_EXCEEDED;
            result.error_message = "Time limit exceeded (signal)";
        } else if (sig == SIGSEGV || sig == SIGABRT) {
            if (result.memory_kb >= limits_.memory_limit_mb * 1024 * 0.9) {
                result.verdict = Verdict::MEMORY_LIMIT_EXCEEDED;
                result.error_message = "Memory limit exceeded";
            } else {
                result.verdict = Verdict::RUNTIME_ERROR;
                result.error_message = "Runtime error (signal " + std::to_string(sig) + ")";
            }
        } else {
            result.verdict = Verdict::RUNTIME_ERROR;
            result.error_message = "Killed by signal " + std::to_string(sig);
        }
        return result;
    }

    if (result.exit_code != 0) {
        result.verdict = Verdict::RUNTIME_ERROR;
        result.error_message = "Non-zero exit code: " + std::to_string(result.exit_code);
        return result;
    }

    if (result.memory_kb > limits_.memory_limit_mb * 1024) {
        result.verdict = Verdict::MEMORY_LIMIT_EXCEEDED;
        result.error_message = "Memory limit exceeded";
        return result;
    }

    if (result.time_ms > limits_.time_limit_ms) {
        result.verdict = Verdict::TIME_LIMIT_EXCEEDED;
        result.error_message = "Time limit exceeded";
        return result;
    }

    if (outputs_match(result.stdout_output, expected_output)) {
        result.verdict = Verdict::ACCEPTED;
    } else {
        result.verdict = Verdict::WRONG_ANSWER;
    }

    return result;
}

}  // namespace ocpe
