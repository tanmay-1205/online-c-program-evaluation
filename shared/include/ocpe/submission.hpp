#pragma once

#include <string>
#include <vector>

namespace ocpe {

enum class Verdict {
    PENDING,
    COMPILING,
    RUNNING,
    ACCEPTED,
    WRONG_ANSWER,
    TIME_LIMIT_EXCEEDED,
    MEMORY_LIMIT_EXCEEDED,
    RUNTIME_ERROR,
    COMPILATION_ERROR,
    INTERNAL_ERROR
};

std::string verdict_to_string(Verdict v);
Verdict verdict_from_string(const std::string& s);

struct TestCaseResult {
    int index = 0;
    Verdict verdict = Verdict::PENDING;
    int time_ms = 0;
    int memory_kb = 0;
    std::string expected_output;
    std::string actual_output;
    std::string error_message;
};

struct SubmissionResult {
    std::string submission_id;
    std::string problem_id;
    std::string username;
    std::string language = "c";
    Verdict verdict = Verdict::PENDING;
    int passed = 0;
    int total = 0;
    int max_time_ms = 0;
    int max_memory_kb = 0;
    std::string compile_output;
    std::vector<TestCaseResult> test_results;
};

struct SubmissionJob {
    std::string submission_id;
    std::string problem_id;
    std::string source_code;
    std::string language = "c";
    std::string username;
};

struct ProblemInfo {
    std::string id;
    std::string title;
    std::string description;
    int time_limit_ms = 2000;
    int memory_limit_mb = 256;
    int test_case_count = 0;
};

}  // namespace ocpe
