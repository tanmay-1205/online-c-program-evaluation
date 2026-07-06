#include "ocpe/redis_client.hpp"
#include "ocpe/json_utils.hpp"

#include <cstdarg>
#include <cstdio>
#include <iostream>

namespace ocpe {

RedisClient::RedisClient(const Config& config) : config_(config) {}

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect() {
    disconnect();
    struct timeval timeout = {2, 0};
    ctx_ = redisConnectWithTimeout(config_.redis_host.c_str(), config_.redis_port, timeout);
    if (!ctx_ || ctx_->err) {
        if (ctx_) {
            std::cerr << "Redis connection error: " << ctx_->errstr << std::endl;
            redisFree(ctx_);
            ctx_ = nullptr;
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        return false;
    }
    return true;
}

void RedisClient::disconnect() {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

bool RedisClient::reconnect() {
    disconnect();
    return connect();
}

redisReply* RedisClient::exec_command(const char* format, ...) {
    if (!ctx_) return nullptr;

    va_list ap;
    va_start(ap, format);
    redisReply* reply = static_cast<redisReply*>(redisvCommand(ctx_, format, ap));
    va_end(ap);

    if (!reply && ctx_->err == REDIS_ERR_IO) {
        if (reconnect()) {
            va_start(ap, format);
            reply = static_cast<redisReply*>(redisvCommand(ctx_, format, ap));
            va_end(ap);
        }
    }
    return reply;
}

bool RedisClient::enqueue_job(const SubmissionJob& job) {
    std::string payload = submission_job_to_json(job);
    redisReply* reply = exec_command("LPUSH %s %b", QUEUE_KEY, payload.data(), payload.size());
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

std::optional<SubmissionJob> RedisClient::dequeue_job(int timeout_sec) {
    redisReply* reply = exec_command("BRPOP %s %d", QUEUE_KEY, timeout_sec);
    if (!reply || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }
    if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
        freeReplyObject(reply);
        return std::nullopt;
    }
    std::string payload(reply->element[1]->str, reply->element[1]->len);
    freeReplyObject(reply);
    return submission_job_from_json(payload);
}

bool RedisClient::set_status(const std::string& submission_id, Verdict status) {
    std::string key = std::string(STATUS_PREFIX) + submission_id;
    std::string val = verdict_to_string(status);
    redisReply* reply = exec_command("SET %s %s EX 86400", key.c_str(), val.c_str());
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

std::optional<Verdict> RedisClient::get_status(const std::string& submission_id) {
    std::string key = std::string(STATUS_PREFIX) + submission_id;
    redisReply* reply = exec_command("GET %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }
    Verdict v = verdict_from_string(std::string(reply->str, reply->len));
    freeReplyObject(reply);
    return v;
}

bool RedisClient::set_result(const SubmissionResult& result) {
    std::string key = std::string(RESULT_PREFIX) + result.submission_id;
    std::string payload = submission_result_to_json(result);
    redisReply* reply = exec_command("SET %s %b EX 86400", key.c_str(), payload.data(), payload.size());
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

std::optional<SubmissionResult> RedisClient::get_result(const std::string& submission_id) {
    std::string key = std::string(RESULT_PREFIX) + submission_id;
    redisReply* reply = exec_command("GET %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }
    std::string payload(reply->str, reply->len);
    freeReplyObject(reply);
    return submission_result_from_json(payload);
}

long long RedisClient::queue_length() {
    redisReply* reply = exec_command("LLEN %s", QUEUE_KEY);
    if (!reply || reply->type != REDIS_REPLY_INTEGER) {
        if (reply) freeReplyObject(reply);
        return -1;
    }
    long long len = reply->integer;
    freeReplyObject(reply);
    return len;
}

redisReply* RedisClient::raw_get(const std::string& key) {
    return exec_command("GET %s", key.c_str());
}

bool RedisClient::raw_set(const std::string& key, const std::string& value) {
    redisReply* reply = exec_command("SET %s %b", key.c_str(), value.data(), value.size());
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

bool RedisClient::raw_set_ex(const std::string& key, const std::string& value, int ttl_sec) {
    redisReply* reply = exec_command("SET %s %b EX %d", key.c_str(), value.data(), value.size(), ttl_sec);
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

redisReply* RedisClient::raw_lpush(const std::string& key, const std::string& value) {
    return exec_command("LPUSH %s %b", key.c_str(), value.data(), value.size());
}

bool RedisClient::raw_ltrim(const std::string& key, int start, int stop) {
    redisReply* reply = exec_command("LTRIM %s %d %d", key.c_str(), start, stop);
    if (!reply) return false;
    bool ok = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

redisReply* RedisClient::raw_lrange(const std::string& key, int start, int stop) {
    return exec_command("LRANGE %s %d %d", key.c_str(), start, stop);
}

}  // namespace ocpe
