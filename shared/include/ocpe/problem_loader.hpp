#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ocpe/submission.hpp"

namespace ocpe {

class ProblemLoader {
public:
    explicit ProblemLoader(std::string problems_dir);

    std::vector<ProblemInfo> list_problems();
    std::optional<ProblemInfo> get_problem(const std::string& problem_id);
    std::vector<std::pair<std::string, std::string>> load_test_cases(const std::string& problem_id);

private:
    std::string problems_dir_;
};

std::string generate_submission_id();

}  // namespace ocpe
