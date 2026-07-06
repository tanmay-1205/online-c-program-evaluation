#pragma once

#include <hiredis/hiredis.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ocpe/config.hpp"
#include "ocpe/submission.hpp"

namespace ocpe {

class RedisClient {
public:
    static constexpr const char* QUEUE_KEY = "ocpe:submission_queue";
    static constexpr const char* RESULT_PREFIX = "ocpe:result:";
    static constexpr const char* STATUS_PREFIX = "ocpe:status:";

    explicit RedisClient(const Config& config);
    ~RedisClient();

    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    bool connect();
    void disconnect();
    bool is_connected() const { return ctx_ != nullptr; }

    bool enqueue_job(const SubmissionJob& job);
    std::optional<SubmissionJob> dequeue_job(int timeout_sec = 5);

    bool set_status(const std::string& submission_id, Verdict status);
    std::optional<Verdict> get_status(const std::string& submission_id);

    bool set_result(const SubmissionResult& result);
    std::optional<SubmissionResult> get_result(const std::string& submission_id);

    long long queue_length();

    // Low-level helpers for auth module
    redisReply* raw_get(const std::string& key);
    bool raw_set(const std::string& key, const std::string& value);
    bool raw_set_ex(const std::string& key, const std::string& value, int ttl_sec);
    redisReply* raw_lpush(const std::string& key, const std::string& value);
    bool raw_ltrim(const std::string& key, int start, int stop);
    redisReply* raw_lrange(const std::string& key, int start, int stop);

private:
    Config config_;
    redisContext* ctx_ = nullptr;

    bool reconnect();
    redisReply* exec_command(const char* format, ...);
};

}  // namespace ocpe
