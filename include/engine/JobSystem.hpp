#pragma once
// Typengine — JobSystem.hpp
// Fixed thread pool for data-parallel game systems.
//
// The frame stays deterministic: parallelFor() blocks until every chunk is
// done, so systems run in order — only their inner loops fan out. Rules for
// job code: no structural ECS changes (create/destroy/emplace/remove), write
// only to components owned by the current item, accumulate shared results
// into per-chunk storage or atomics and apply them after the call returns.
//
//   engine.jobs().parallelFor(entities.size(), 64, [&](std::size_t i) {
//       auto& t = registry.get<Transform>(entities[i]);
//       ...
//   });

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ty {

class JobSystem {
public:
    JobSystem() = default;
    ~JobSystem() { shutdown(); }
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // 0 = one worker per hardware thread minus one (the main thread also
    // participates in parallelFor), at least 1.
    void initialize(unsigned workerCount = 0);
    void shutdown();

    bool isInitialized() const { return !workers_.empty(); }
    unsigned workerCount() const { return static_cast<unsigned>(workers_.size()); }

    // Fire-and-forget task; wait() blocks until all of these have finished.
    void run(std::function<void()> job);
    void wait();

    // Runs fn(index) for every index in [0, count), split into chunks of
    // `grain` indices. Blocks until complete; the calling thread works too.
    // Falls back to a plain loop when the pool is not initialized or the
    // range is too small to be worth dispatching.
    void parallelFor(std::size_t count, std::size_t grain,
                     const std::function<void(std::size_t)>& fn);

    // Chunk variant — fn(begin, end) — for loops with per-chunk setup
    // (scratch buffers, local accumulators).
    void parallelForChunks(std::size_t count, std::size_t grain,
                           const std::function<void(std::size_t, std::size_t)>& fn);

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> queue_;
    std::mutex mutex_;
    std::condition_variable wakeWorkers_;
    std::condition_variable allDone_;
    std::size_t pending_ = 0;   // queued + currently running jobs
    bool stop_ = false;
};

} // namespace ty
