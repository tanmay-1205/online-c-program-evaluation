#include "ocpe/worker.hpp"
#include "ocpe/evaluator.hpp"

#include <chrono>
#include <iostream>

namespace ocpe {

WorkerPool::WorkerPool(Config config, ProblemLoader loader)
    : config_(std::move(config)), loader_(std::move(loader)) {}

void WorkerPool::start() {
    running_ = true;
    threads_.reserve(config_.worker_count);
    for (int i = 0; i < config_.worker_count; ++i) {
        threads_.emplace_back(&WorkerPool::worker_loop, this, i);
    }
    std::cout << "Started " << config_.worker_count << " worker threads" << std::endl;
}

void WorkerPool::stop() {
    running_ = false;
}

void WorkerPool::join() {
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
}

void WorkerPool::worker_loop(int worker_id) {
    RedisClient redis(config_);
    while (!redis.connect()) {
        if (!running_) return;
        std::cerr << "Worker " << worker_id << ": waiting for Redis..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "Worker " << worker_id << " ready" << std::endl;

    while (running_) {
        auto job = redis.dequeue_job(5);
        if (!job) continue;

        std::cout << "Worker " << worker_id << " processing " << job->submission_id << std::endl;

        redis.set_status(job->submission_id, Verdict::COMPILING);

        redis.set_status(job->submission_id, Verdict::RUNNING);
        SubmissionResult result = evaluate_submission(*job, config_.submissions_dir, loader_);

        redis.set_result(result);
        redis.set_status(job->submission_id, result.verdict);

        std::cout << "Worker " << worker_id << " finished " << job->submission_id
                  << " -> " << verdict_to_string(result.verdict)
                  << " (" << result.passed << "/" << result.total << ")" << std::endl;
    }
}

}  // namespace ocpe
