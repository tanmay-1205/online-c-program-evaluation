#include "ocpe/submission.hpp"

namespace ocpe {

std::string verdict_to_string(Verdict v) {
    switch (v) {
        case Verdict::PENDING: return "PENDING";
        case Verdict::COMPILING: return "COMPILING";
        case Verdict::RUNNING: return "RUNNING";
        case Verdict::ACCEPTED: return "ACCEPTED";
        case Verdict::WRONG_ANSWER: return "WRONG_ANSWER";
        case Verdict::TIME_LIMIT_EXCEEDED: return "TIME_LIMIT_EXCEEDED";
        case Verdict::MEMORY_LIMIT_EXCEEDED: return "MEMORY_LIMIT_EXCEEDED";
        case Verdict::RUNTIME_ERROR: return "RUNTIME_ERROR";
        case Verdict::COMPILATION_ERROR: return "COMPILATION_ERROR";
        case Verdict::INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    return "UNKNOWN";
}

Verdict verdict_from_string(const std::string& s) {
    if (s == "PENDING") return Verdict::PENDING;
    if (s == "COMPILING") return Verdict::COMPILING;
    if (s == "RUNNING") return Verdict::RUNNING;
    if (s == "ACCEPTED") return Verdict::ACCEPTED;
    if (s == "WRONG_ANSWER") return Verdict::WRONG_ANSWER;
    if (s == "TIME_LIMIT_EXCEEDED") return Verdict::TIME_LIMIT_EXCEEDED;
    if (s == "MEMORY_LIMIT_EXCEEDED") return Verdict::MEMORY_LIMIT_EXCEEDED;
    if (s == "RUNTIME_ERROR") return Verdict::RUNTIME_ERROR;
    if (s == "COMPILATION_ERROR") return Verdict::COMPILATION_ERROR;
    return Verdict::INTERNAL_ERROR;
}

}  // namespace ocpe
