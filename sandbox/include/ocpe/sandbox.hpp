#pragma once

#include <string>
#include <vector>

#include "ocpe/submission.hpp"

namespace ocpe {

struct SandboxLimits {
    int time_limit_ms = 2000;
    int memory_limit_mb = 256;
    int output_limit_kb = 1024;
};

struct SandboxResult {
    Verdict verdict = Verdict::PENDING;
    int exit_code = 0;
    int time_ms = 0;
    int memory_kb = 0;
    std::string stdout_output;
    std::string stderr_output;
    std::string error_message;
};

class Sandbox {
public:
    explicit Sandbox(SandboxLimits limits);

    SandboxResult run(const std::string& runnable_path,
                      const std::string& interpreter,
                      const std::string& input_data,
                      const std::string& expected_output);

    static std::string trim_output(const std::string& s);
    static bool outputs_match(const std::string& actual, const std::string& expected);

private:
    SandboxLimits limits_;
};

struct CompileResult {
    bool success = false;
    std::string runnable_path;
    std::string interpreter;
    std::string output;
};

CompileResult compile_source(const std::string& language,
                             const std::string& source_path,
                             const std::string& output_path,
                             int timeout_sec = 10);

std::vector<TestCaseResult> run_all_test_cases(
    const std::string& runnable_path,
    const std::string& interpreter,
    const std::vector<std::pair<std::string, std::string>>& test_cases,
    int time_limit_ms,
    int memory_limit_mb);

}  // namespace ocpe
