#include "ocpe/auth.hpp"

#include <openssl/sha.h>

#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>

namespace ocpe {

namespace {
constexpr const char* USER_PREFIX = "ocpe:user:";
constexpr const char* TOKEN_PREFIX = "ocpe:token:";
constexpr const char* USER_SUBS_PREFIX = "ocpe:user_subs:";
constexpr int TOKEN_TTL_SEC = 7 * 24 * 3600;
}  // namespace

AuthService::AuthService(RedisClient& redis) : redis_(redis) {}

std::string AuthService::hash_password(const std::string& password, const std::string& salt) {
    std::string data = salt + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string AuthService::generate_salt() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";
    std::string salt;
    salt.reserve(32);
    for (int i = 0; i < 32; ++i) salt += hex[dis(gen)];
    return salt;
}

std::string AuthService::generate_token() {
    return generate_salt() + generate_salt();
}

std::string AuthService::extract_bearer_token(const std::string& auth_header) {
    const std::string prefix = "Bearer ";
    if (auth_header.size() > prefix.size() &&
        auth_header.compare(0, prefix.size(), prefix) == 0) {
        return auth_header.substr(prefix.size());
    }
    return "";
}

bool AuthService::register_user(const std::string& username, const std::string& password,
                                std::string& error) {
    if (username.size() < 3 || username.size() > 32) {
        error = "Username must be 3-32 characters";
        return false;
    }
    if (password.size() < 6) {
        error = "Password must be at least 6 characters";
        return false;
    }
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            error = "Username may only contain letters, numbers, and underscores";
            return false;
        }
    }

    std::string user_key = std::string(USER_PREFIX) + username;
    redisReply* exists = redis_.raw_get(user_key);
    if (exists && exists->type != REDIS_REPLY_NIL) {
        if (exists) freeReplyObject(exists);
        error = "Username already exists";
        return false;
    }
    if (exists) freeReplyObject(exists);

    std::string salt = generate_salt();
    std::string hash = hash_password(password, salt);
    std::string payload = salt + ":" + hash;

    bool ok = redis_.raw_set(user_key, payload);
    if (!ok) error = "Failed to create user";
    return ok;
}

std::optional<AuthSession> AuthService::login(const std::string& username,
                                              const std::string& password) {
    std::string user_key = std::string(USER_PREFIX) + username;
    redisReply* reply = redis_.raw_get(user_key);
    if (!reply || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }

    std::string stored(reply->str, reply->len);
    freeReplyObject(reply);

    auto colon = stored.find(':');
    if (colon == std::string::npos) return std::nullopt;

    std::string salt = stored.substr(0, colon);
    std::string hash = stored.substr(colon + 1);
    if (hash_password(password, salt) != hash) return std::nullopt;

    AuthSession session;
    session.token = generate_token();
    session.username = username;

    std::string token_key = std::string(TOKEN_PREFIX) + session.token;
    if (!redis_.raw_set_ex(token_key, username, TOKEN_TTL_SEC)) return std::nullopt;

    return session;
}

std::optional<std::string> AuthService::validate_token(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string token_key = std::string(TOKEN_PREFIX) + token;
    redisReply* reply = redis_.raw_get(token_key);
    if (!reply || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }
    std::string username(reply->str, reply->len);
    freeReplyObject(reply);
    return username;
}

bool AuthService::record_submission(const std::string& username, const std::string& submission_id) {
    std::string key = std::string(USER_SUBS_PREFIX) + username;
    redisReply* reply = redis_.raw_lpush(key, submission_id);
    if (!reply) return false;
    freeReplyObject(reply);
    redis_.raw_ltrim(key, 0, 99);
    return true;
}

std::vector<std::string> AuthService::get_user_submissions(const std::string& username, int limit) {
    std::vector<std::string> subs;
    std::string key = std::string(USER_SUBS_PREFIX) + username;
    redisReply* reply = redis_.raw_lrange(key, 0, limit - 1);
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) freeReplyObject(reply);
        return subs;
    }
    for (size_t i = 0; i < reply->elements; ++i) {
        subs.emplace_back(reply->element[i]->str, reply->element[i]->len);
    }
    freeReplyObject(reply);
    return subs;
}

}  // namespace ocpe
