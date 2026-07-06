#pragma once

#include "ocpe/submission.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace ocpe {

std::string submission_job_to_json(const SubmissionJob& job);
SubmissionJob submission_job_from_json(const std::string& json);

std::string submission_result_to_json(const SubmissionResult& result);
SubmissionResult submission_result_from_json(const std::string& json);

std::string problem_info_to_json(const ProblemInfo& problem);
ProblemInfo problem_info_from_json(const std::string& json);

nlohmann::json test_case_result_to_json(const TestCaseResult& tc);
TestCaseResult test_case_result_from_json(const nlohmann::json& j);

}  // namespace ocpe
