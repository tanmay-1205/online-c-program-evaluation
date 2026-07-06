#include "ocpe/problem_loader.hpp"

#include <dirent.h>
#include <fstream>
#include <random>
#include <sstream>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

namespace ocpe {

ProblemLoader::ProblemLoader(std::string problems_dir)
    : problems_dir_(std::move(problems_dir)) {}

std::vector<ProblemInfo> ProblemLoader::list_problems() {
    std::vector<ProblemInfo> problems;
    DIR* dir = opendir(problems_dir_.c_str());
    if (!dir) return problems;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        std::string path = problems_dir_ + "/" + entry->d_name + "/meta.json";
        struct stat st;
        if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;

        std::ifstream f(path);
        if (!f) continue;
        std::stringstream ss;
        ss << f.rdbuf();
        auto p = problem_info_from_json(ss.str());
        p.id = entry->d_name;

        std::string tests_dir = problems_dir_ + "/" + entry->d_name + "/tests";
        int count = 0;
        while (true) {
            std::string in_path = tests_dir + "/" + std::to_string(count) + ".in";
            struct stat tst;
            if (stat(in_path.c_str(), &tst) != 0) break;
            ++count;
        }
        p.test_case_count = count;
        problems.push_back(p);
    }
    closedir(dir);
    return problems;
}

std::optional<ProblemInfo> ProblemLoader::get_problem(const std::string& problem_id) {
    std::string path = problems_dir_ + "/" + problem_id + "/meta.json";
    std::ifstream f(path);
    if (!f) return std::nullopt;

    std::stringstream ss;
    ss << f.rdbuf();
    auto p = problem_info_from_json(ss.str());
    p.id = problem_id;

    auto tests = load_test_cases(problem_id);
    p.test_case_count = static_cast<int>(tests.size());
    return p;
}

std::vector<std::pair<std::string, std::string>> ProblemLoader::load_test_cases(
    const std::string& problem_id) {
    std::vector<std::pair<std::string, std::string>> cases;
    std::string tests_dir = problems_dir_ + "/" + problem_id + "/tests";

    for (int i = 0; i < 10000; ++i) {
        std::string in_path = tests_dir + "/" + std::to_string(i) + ".in";
        std::string out_path = tests_dir + "/" + std::to_string(i) + ".out";

        struct stat st;
        if (stat(in_path.c_str(), &st) != 0) break;

        std::ifstream in_file(in_path);
        std::ifstream out_file(out_path);
        if (!in_file || !out_file) break;

        std::stringstream in_ss, out_ss;
        in_ss << in_file.rdbuf();
        out_ss << out_file.rdbuf();
        cases.emplace_back(in_ss.str(), out_ss.str());
    }
    return cases;
}

std::string generate_submission_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";

    std::string id;
    id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

}  // namespace ocpe
