#include "ocpe/server.hpp"

#include <iostream>
#include <unistd.h>

int main() {
    ocpe::Config config = ocpe::load_config_from_env();
    ocpe::RedisClient redis(config);

    std::cout << "Connecting to Redis at " << config.redis_host << ":" << config.redis_port << std::endl;
    int retries = 30;
    while (!redis.connect() && retries-- > 0) {
        std::cerr << "Waiting for Redis..." << std::endl;
        sleep(2);
    }
    if (!redis.is_connected()) {
        std::cerr << "Failed to connect to Redis" << std::endl;
        return 1;
    }

    ocpe::ProblemLoader loader(config.problems_dir);
    ocpe::ApiServer server(std::move(config), std::move(loader), std::move(redis));
    server.run();
    return 0;
}
