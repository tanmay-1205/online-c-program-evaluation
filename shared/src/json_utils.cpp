#include "ocpe/json_utils.hpp"

namespace ocpe {

std::string submission_job_to_json(const SubmissionJob& job) {
    nlohmann::json j;
    j["submission_id"] = job.submission_id;
    j["problem_id"] = job.problem_id;
    j["source_code"] = job.source_code;
    j["language"] = job.language;
    j["username"] = job.username;
    return j.dump();
}

SubmissionJob submission_job_from_json(const std::string& json) {
    auto j = nlohmann::json::parse(json);
    SubmissionJob job;
    job.submission_id = j.value("submission_id", "");
    job.problem_id = j.value("problem_id", "");
    job.source_code = j.value("source_code", "");
    job.language = j.value("language", "c");
    job.username = j.value("username", "");
    return job;
}

nlohmann::json test_case_result_to_json(const TestCaseResult& tc) {
    nlohmann::json j;
    j["index"] = tc.index;
    j["verdict"] = verdict_to_string(tc.verdict);
    j["time_ms"] = tc.time_ms;
    j["memory_kb"] = tc.memory_kb;
    j["expected_output"] = tc.expected_output;
    j["actual_output"] = tc.actual_output;
    j["error_message"] = tc.error_message;
    return j;
}

TestCaseResult test_case_result_from_json(const nlohmann::json& j) {
    TestCaseResult tc;
    tc.index = j.value("index", 0);
    tc.verdict = verdict_from_string(j.value("verdict", "PENDING"));
    tc.time_ms = j.value("time_ms", 0);
    tc.memory_kb = j.value("memory_kb", 0);
    tc.expected_output = j.value("expected_output", "");
    tc.actual_output = j.value("actual_output", "");
    tc.error_message = j.value("error_message", "");
    return tc;
}

std::string submission_result_to_json(const SubmissionResult& result) {
    nlohmann::json j;
    j["submission_id"] = result.submission_id;
    j["problem_id"] = result.problem_id;
    j["username"] = result.username;
    j["language"] = result.language;
    j["verdict"] = verdict_to_string(result.verdict);
    j["passed"] = result.passed;
    j["total"] = result.total;
    j["max_time_ms"] = result.max_time_ms;
    j["max_memory_kb"] = result.max_memory_kb;
    j["compile_output"] = result.compile_output;
    j["test_results"] = nlohmann::json::array();
    for (const auto& tc : result.test_results) {
        j["test_results"].push_back(test_case_result_to_json(tc));
    }
    return j.dump();
}

SubmissionResult submission_result_from_json(const std::string& json) {
    auto j = nlohmann::json::parse(json);
    SubmissionResult result;
    result.submission_id = j.value("submission_id", "");
    result.problem_id = j.value("problem_id", "");
    result.username = j.value("username", "");
    result.language = j.value("language", "c");
    result.verdict = verdict_from_string(j.value("verdict", "PENDING"));
    result.passed = j.value("passed", 0);
    result.total = j.value("total", 0);
    result.max_time_ms = j.value("max_time_ms", 0);
    result.max_memory_kb = j.value("max_memory_kb", 0);
    result.compile_output = j.value("compile_output", "");
    if (j.contains("test_results") && j["test_results"].is_array()) {
        for (const auto& tc : j["test_results"]) {
            result.test_results.push_back(test_case_result_from_json(tc));
        }
    }
    return result;
}

std::string problem_info_to_json(const ProblemInfo& problem) {
    nlohmann::json j;
    j["id"] = problem.id;
    j["title"] = problem.title;
    j["description"] = problem.description;
    j["time_limit_ms"] = problem.time_limit_ms;
    j["memory_limit_mb"] = problem.memory_limit_mb;
    j["test_case_count"] = problem.test_case_count;
    return j.dump();
}

ProblemInfo problem_info_from_json(const std::string& json) {
    auto j = nlohmann::json::parse(json);
    ProblemInfo p;
    p.id = j.value("id", "");
    p.title = j.value("title", "");
    p.description = j.value("description", "");
    p.time_limit_ms = j.value("time_limit_ms", 2000);
    p.memory_limit_mb = j.value("memory_limit_mb", 256);
    p.test_case_count = j.value("test_case_count", 0);
    return p;
}

}  // namespace ocpe
