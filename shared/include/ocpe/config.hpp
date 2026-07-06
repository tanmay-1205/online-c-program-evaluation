#pragma once

#include <cstdint>
#include <string>

namespace ocpe {

struct Config {
    std::string redis_host = "127.0.0.1";
    int redis_port = 6379;
    std::string problems_dir = "./problems";
    std::string submissions_dir = "./data/submissions";
    int api_port = 8080;
    int worker_count = 8;
    int compile_timeout_sec = 10;
    int default_time_limit_ms = 2000;
    int default_memory_limit_mb = 256;
};

Config load_config_from_env();

}  // namespace ocpe
