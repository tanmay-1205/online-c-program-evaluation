#include "ocpe/sandbox.hpp"
#include "ocpe/language.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

namespace ocpe {

namespace {

int run_compile_process(const char* executable, char* const argv[], int timeout_sec,
                        std::string& compile_output) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        compile_output = "Failed to create compile pipe";
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        compile_output = "fork() failed for compilation";
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(executable, argv);
        _exit(127);
    }

    close(pipefd[1]);
    compile_output.clear();
    char buf[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        compile_output.append(buf, n);
    }
    close(pipefd[0]);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    int status = 0;
    while (true) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) break;
        if (std::chrono::steady_clock::now() > deadline) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            compile_output = "Compilation timed out\n" + compile_output;
            return -2;
        }
        usleep(50000);
    }

    return status;
}

CompileResult validate_python_syntax(const std::string& source_path, int timeout_sec) {
    CompileResult result;
    result.runnable_path = source_path;
    result.interpreter = "python3";

    char* argv[] = {
        const_cast<char*>("python3"),
        const_cast<char*>("-m"),
        const_cast<char*>("py_compile"),
        const_cast<char*>(source_path.c_str()),
        nullptr
    };

    int status = run_compile_process("python3", argv, timeout_sec, result.output);
    if (status == -1 || status == -2) {
        result.success = false;
        return result;
    }

    result.success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    return result;
}

}  // namespace

CompileResult compile_source(const std::string& language,
                             const std::string& source_path,
                             const std::string& output_path,
                             int timeout_sec) {
    CompileResult result;
    std::string lang = normalize_language(language);

    if (lang == "python") {
        return validate_python_syntax(source_path, timeout_sec);
    }

    result.runnable_path = output_path;
    result.interpreter.clear();

    int status = 0;
    if (lang == "cpp") {
        char* argv[] = {
            const_cast<char*>("g++"),
            const_cast<char*>("-O2"),
            const_cast<char*>("-std=c++17"),
            const_cast<char*>("-Wall"),
            const_cast<char*>(source_path.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(output_path.c_str()),
            nullptr
        };
        status = run_compile_process("g++", argv, timeout_sec, result.output);
    } else {
        char* argv[] = {
            const_cast<char*>("gcc"),
            const_cast<char*>("-O2"),
            const_cast<char*>("-std=c11"),
            const_cast<char*>("-Wall"),
            const_cast<char*>(source_path.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(output_path.c_str()),
            const_cast<char*>("-lm"),
            nullptr
        };
        status = run_compile_process("gcc", argv, timeout_sec, result.output);
    }

    if (status == -1 || status == -2) {
        result.success = false;
        return result;
    }

    result.success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    return result;
}

}  // namespace ocpe
