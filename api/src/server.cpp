#include "ocpe/server.hpp"

#include "ocpe/json_utils.hpp"
#include "ocpe/language.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace ocpe {

ApiServer::ApiServer(Config config, ProblemLoader loader, RedisClient redis)
    : config_(std::move(config)),
      loader_(std::move(loader)),
      redis_(std::move(redis)),
      auth_(redis_) {
    setup_routes();
}

std::optional<std::string> ApiServer::require_auth(const httplib::Request& req,
                                                   httplib::Response& res) {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) {
        res.status = 401;
        res.set_content(R"({"error":"authentication required"})", "application/json");
        return std::nullopt;
    }
    std::string token = AuthService::extract_bearer_token(it->second);
    auto username = auth_.validate_token(token);
    if (!username) {
        res.status = 401;
        res.set_content(R"({"error":"invalid or expired token"})", "application/json");
        return std::nullopt;
    }
    return username;
}

void ApiServer::setup_routes() {
    server_.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
    });

    server_.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    server_.Get("/languages", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& lang : supported_languages()) {
            arr.push_back({{"id", lang.id}, {"name", lang.name}, {"extension", lang.extension}});
        }
        res.set_content(arr.dump(), "application/json");
    });

    server_.Post("/auth/register", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        std::string error;
        if (!auth_.register_user(username, password, error)) {
            res.status = 400;
            nlohmann::json j;
            j["error"] = error;
            res.set_content(j.dump(), "application/json");
            return;
        }

        auto session = auth_.login(username, password);
        if (!session) {
            res.status = 500;
            res.set_content(R"({"error":"registration succeeded but login failed"})", "application/json");
            return;
        }

        nlohmann::json j;
        j["token"] = session->token;
        j["username"] = session->username;
        res.status = 201;
        res.set_content(j.dump(), "application/json");
    });

    server_.Post("/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }

        auto session = auth_.login(body.value("username", ""), body.value("password", ""));
        if (!session) {
            res.status = 401;
            res.set_content(R"({"error":"invalid credentials"})", "application/json");
            return;
        }

        nlohmann::json j;
        j["token"] = session->token;
        j["username"] = session->username;
        res.set_content(j.dump(), "application/json");
    });

    server_.Get("/auth/me", [this](const httplib::Request& req, httplib::Response& res) {
        auto username = require_auth(req, res);
        if (!username) return;
        nlohmann::json j;
        j["username"] = *username;
        res.set_content(j.dump(), "application/json");
    });

    server_.Get("/auth/submissions", [this](const httplib::Request& req, httplib::Response& res) {
        auto username = require_auth(req, res);
        if (!username) return;

        auto ids = auth_.get_user_submissions(*username);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& id : ids) {
            nlohmann::json entry;
            entry["submission_id"] = id;
            auto result = redis_.get_result(id);
            if (result) {
                entry["problem_id"] = result->problem_id;
                entry["verdict"] = verdict_to_string(result->verdict);
                entry["language"] = result->language;
                entry["passed"] = result->passed;
                entry["total"] = result->total;
            } else {
                auto status = redis_.get_status(id);
                entry["verdict"] = status ? verdict_to_string(*status) : "PENDING";
            }
            arr.push_back(entry);
        }
        res.set_content(arr.dump(), "application/json");
    });

    server_.Get("/problems", [this](const httplib::Request&, httplib::Response& res) {
        auto problems = loader_.list_problems();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : problems) {
            arr.push_back(nlohmann::json::parse(problem_info_to_json(p)));
        }
        res.set_content(arr.dump(), "application/json");
    });

    server_.Get(R"(/problems/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto problem = loader_.get_problem(req.matches[1]);
        if (!problem) {
            res.status = 404;
            res.set_content(R"({"error":"problem not found"})", "application/json");
            return;
        }
        res.set_content(problem_info_to_json(*problem), "application/json");
    });

    server_.Post("/submit", [this](const httplib::Request& req, httplib::Response& res) {
        auto username = require_auth(req, res);
        if (!username) return;

        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }

        std::string problem_id = body.value("problem_id", "");
        std::string source_code = body.value("source_code", "");
        std::string language = body.value("language", "c");

        if (problem_id.empty() || source_code.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"problem_id and source_code required"})", "application/json");
            return;
        }

        if (!is_supported_language(language)) {
            res.status = 400;
            res.set_content(R"({"error":"unsupported language"})", "application/json");
            return;
        }

        auto problem = loader_.get_problem(problem_id);
        if (!problem) {
            res.status = 404;
            res.set_content(R"({"error":"problem not found"})", "application/json");
            return;
        }

        SubmissionJob job;
        job.submission_id = generate_submission_id();
        job.problem_id = problem_id;
        job.source_code = source_code;
        job.language = normalize_language(language);
        job.username = *username;

        fs::create_directories(config_.submissions_dir);

        if (!redis_.enqueue_job(job)) {
            res.status = 503;
            res.set_content(R"({"error":"queue unavailable"})", "application/json");
            return;
        }

        redis_.set_status(job.submission_id, Verdict::PENDING);
        auth_.record_submission(*username, job.submission_id);

        nlohmann::json response;
        response["submission_id"] = job.submission_id;
        response["status"] = "PENDING";
        response["language"] = job.language;
        response["queue_position"] = redis_.queue_length();
        res.status = 202;
        res.set_content(response.dump(), "application/json");
    });

    server_.Get(R"(/submissions/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto username = require_auth(req, res);
        if (!username) return;

        std::string id = req.matches[1];
        auto result = redis_.get_result(id);
        if (result) {
            res.set_content(submission_result_to_json(*result), "application/json");
            return;
        }
        auto status = redis_.get_status(id);
        if (status) {
            nlohmann::json j;
            j["submission_id"] = id;
            j["verdict"] = verdict_to_string(*status);
            res.set_content(j.dump(), "application/json");
            return;
        }
        res.status = 404;
        res.set_content(R"({"error":"submission not found"})", "application/json");
    });

    server_.Get("/stats", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j;
        j["queue_length"] = redis_.queue_length();
        j["worker_count"] = config_.worker_count;
        j["languages"] = nlohmann::json::array();
        for (const auto& lang : supported_languages()) {
            j["languages"].push_back(lang.id);
        }
        res.set_content(j.dump(), "application/json");
    });
}

void ApiServer::run() {
    std::cout << "OCPE API listening on port " << config_.api_port << std::endl;
    server_.listen("0.0.0.0", config_.api_port);
}

}  // namespace ocpe
