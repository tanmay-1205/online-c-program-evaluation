#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "ocpe/config.hpp"
#include "ocpe/problem_loader.hpp"
#include "ocpe/redis_client.hpp"

namespace ocpe {

class WorkerPool {
public:
    WorkerPool(Config config, ProblemLoader loader);

    void start();
    void stop();
    void join();

private:
    Config config_;
    ProblemLoader loader_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> threads_;

    void worker_loop(int worker_id);
};

}  // namespace ocpe
