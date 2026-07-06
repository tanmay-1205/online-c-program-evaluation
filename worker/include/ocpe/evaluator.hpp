#pragma once

#include "ocpe/submission.hpp"

#include <string>

namespace ocpe {

class ProblemLoader;

SubmissionResult evaluate_submission(
    const SubmissionJob& job,
    const std::string& submissions_dir,
    ProblemLoader& loader);

}  // namespace ocpe
