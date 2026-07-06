#include "ocpe/sandbox.hpp"

namespace ocpe {

std::vector<TestCaseResult> run_all_test_cases(
    const std::string& runnable_path,
    const std::string& interpreter,
    const std::vector<std::pair<std::string, std::string>>& test_cases,
    int time_limit_ms,
    int memory_limit_mb) {

    SandboxLimits limits;
    limits.time_limit_ms = time_limit_ms;
    limits.memory_limit_mb = memory_limit_mb;

    Sandbox sandbox(limits);
    std::vector<TestCaseResult> results;
    results.reserve(test_cases.size());

    for (size_t i = 0; i < test_cases.size(); ++i) {
        TestCaseResult tc;
        tc.index = static_cast<int>(i);

        SandboxResult sr = sandbox.run(
            runnable_path, interpreter, test_cases[i].first, test_cases[i].second);

        tc.verdict = sr.verdict;
        tc.time_ms = sr.time_ms;
        tc.memory_kb = sr.memory_kb;
        tc.expected_output = test_cases[i].second;
        tc.actual_output = sr.stdout_output;
        tc.error_message = sr.error_message;

        if (!sr.stderr_output.empty() && tc.error_message.empty()) {
            tc.error_message = sr.stderr_output;
        }

        results.push_back(tc);
    }

    return results;
}

}  // namespace ocpe
