#pragma once

#include <httplib.h>
#include <optional>

#include "ocpe/auth.hpp"
#include "ocpe/config.hpp"
#include "ocpe/problem_loader.hpp"
#include "ocpe/redis_client.hpp"

namespace ocpe {

class ApiServer {
public:
    ApiServer(Config config, ProblemLoader loader, RedisClient redis);

    void run();

private:
    Config config_;
    ProblemLoader loader_;
    RedisClient redis_;
    AuthService auth_;
    httplib::Server server_;

    void setup_routes();
    std::optional<std::string> require_auth(const httplib::Request& req, httplib::Response& res);
};

}  // namespace ocpe
