#include "ocpe/evaluator.hpp"
#include "ocpe/problem_loader.hpp"
#include "ocpe/sandbox.hpp"
#include "ocpe/language.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace ocpe {

SubmissionResult evaluate_submission(
    const SubmissionJob& job,
    const std::string& submissions_dir,
    ProblemLoader& loader) {

    SubmissionResult result;
    result.submission_id = job.submission_id;
    result.problem_id = job.problem_id;
    result.username = job.username;
    result.language = normalize_language(job.language);

    auto problem = loader.get_problem(job.problem_id);
    if (!problem) {
        result.verdict = Verdict::INTERNAL_ERROR;
        result.compile_output = "Problem not found";
        return result;
    }

    if (!is_supported_language(result.language)) {
        result.verdict = Verdict::COMPILATION_ERROR;
        result.compile_output = "Unsupported language: " + job.language;
        return result;
    }

    fs::path work_dir = fs::path(submissions_dir) / job.submission_id;
    fs::create_directories(work_dir);

    std::string ext = source_extension(result.language);
    std::string source_path = (work_dir / ("main" + ext)).string();
    std::string binary_path = (work_dir / "main").string();

    std::ofstream src(source_path);
    src << job.source_code;
    src.close();

    auto compile = compile_source(result.language, source_path, binary_path, 10);
    result.compile_output = compile.output;

    if (!compile.success) {
        result.verdict = Verdict::COMPILATION_ERROR;
        return result;
    }

    auto test_cases = loader.load_test_cases(job.problem_id);
    result.total = static_cast<int>(test_cases.size());

    auto test_results = run_all_test_cases(
        compile.runnable_path,
        compile.interpreter,
        test_cases,
        problem->time_limit_ms,
        problem->memory_limit_mb);

    result.test_results = test_results;
    result.passed = 0;
    result.max_time_ms = 0;
    result.max_memory_kb = 0;

    for (const auto& tc : test_results) {
        if (tc.verdict == Verdict::ACCEPTED) ++result.passed;
        result.max_time_ms = std::max(result.max_time_ms, tc.time_ms);
        result.max_memory_kb = std::max(result.max_memory_kb, tc.memory_kb);
    }

    if (test_results.empty()) {
        result.verdict = Verdict::INTERNAL_ERROR;
    } else if (result.passed == result.total) {
        result.verdict = Verdict::ACCEPTED;
    } else {
        result.verdict = test_results.back().verdict;
        if (result.verdict == Verdict::PENDING) {
            result.verdict = Verdict::WRONG_ANSWER;
        }
    }

    if (result.language != "python") {
        fs::remove(binary_path);
    }
    return result;
}

}  // namespace ocpe
