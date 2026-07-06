#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ocpe/redis_client.hpp"

namespace ocpe {

struct User {
    std::string username;
};

struct AuthSession {
    std::string token;
    std::string username;
};

class AuthService {
public:
    explicit AuthService(RedisClient& redis);

    bool register_user(const std::string& username, const std::string& password, std::string& error);
    std::optional<AuthSession> login(const std::string& username, const std::string& password);
    std::optional<std::string> validate_token(const std::string& token);
    bool record_submission(const std::string& username, const std::string& submission_id);
    std::vector<std::string> get_user_submissions(const std::string& username, int limit = 50);

    static std::string extract_bearer_token(const std::string& auth_header);

private:
    RedisClient& redis_;

    static std::string hash_password(const std::string& password, const std::string& salt);
    static std::string generate_salt();
    static std::string generate_token();
};

}  // namespace ocpe
