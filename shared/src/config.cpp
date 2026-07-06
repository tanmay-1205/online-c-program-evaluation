#include "ocpe/config.hpp"

#include <cstdlib>
#include <cstring>

namespace ocpe {

namespace {

std::string env_or(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : fallback;
}

int env_int_or(const char* key, int fallback) {
    const char* val = std::getenv(key);
    if (!val || std::strlen(val) == 0) return fallback;
    return std::atoi(val);
}

}  // namespace

Config load_config_from_env() {
    Config cfg;
    cfg.redis_host = env_or("REDIS_HOST", cfg.redis_host);
    cfg.redis_port = env_int_or("REDIS_PORT", cfg.redis_port);
    cfg.problems_dir = env_or("PROBLEMS_DIR", cfg.problems_dir);
    cfg.submissions_dir = env_or("SUBMISSIONS_DIR", cfg.submissions_dir);
    cfg.api_port = env_int_or("API_PORT", cfg.api_port);
    cfg.worker_count = env_int_or("WORKER_COUNT", cfg.worker_count);
    return cfg;
}

}  // namespace ocpe
