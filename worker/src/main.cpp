#include "ocpe/worker.hpp"

#include <csignal>
#include <iostream>

static ocpe::WorkerPool* g_pool = nullptr;

void signal_handler(int) {
    if (g_pool) g_pool->stop();
}

int main() {
    ocpe::Config config = ocpe::load_config_from_env();
    ocpe::ProblemLoader loader(config.problems_dir);
    ocpe::WorkerPool pool(config, std::move(loader));

    g_pool = &pool;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    pool.start();
    pool.join();
    return 0;
}
