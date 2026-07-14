#include "engine/JobSystem.hpp"

#include <algorithm>

namespace ty {

void JobSystem::initialize(unsigned workerCount) {
    shutdown();
    if (workerCount == 0) {
        unsigned hw = std::thread::hardware_concurrency();
        workerCount = hw > 1 ? hw - 1 : 1;
    }
    stop_ = false;
    workers_.reserve(workerCount);
    for (unsigned i = 0; i < workerCount; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

void JobSystem::shutdown() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    wakeWorkers_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
    workers_.clear();
    queue_.clear();
    pending_ = 0;
}

void JobSystem::run(std::function<void()> job) {
    if (workers_.empty()) {
        job();
        return;
    }
    {
        std::lock_guard lock(mutex_);
        queue_.push_back(std::move(job));
        ++pending_;
    }
    wakeWorkers_.notify_one();
}

void JobSystem::wait() {
    std::unique_lock lock(mutex_);
    allDone_.wait(lock, [this] { return pending_ == 0; });
}

void JobSystem::workerLoop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock lock(mutex_);
            wakeWorkers_.wait(lock, [this] { return stop_ || !queue_.empty(); });
            if (stop_ && queue_.empty()) return;
            job = std::move(queue_.front());
            queue_.pop_front();
        }
        job();
        {
            std::lock_guard lock(mutex_);
            --pending_;
            if (pending_ == 0) allDone_.notify_all();
        }
    }
}

void JobSystem::parallelFor(std::size_t count, std::size_t grain,
                            const std::function<void(std::size_t)>& fn) {
    parallelForChunks(count, grain, [&fn](std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; ++i) fn(i);
    });
}

void JobSystem::parallelForChunks(std::size_t count, std::size_t grain,
                                  const std::function<void(std::size_t, std::size_t)>& fn) {
    if (count == 0) return;
    if (grain == 0) grain = 1;

    // Not worth a dispatch: pool missing or everything fits in one chunk.
    if (workers_.empty() || count <= grain) {
        fn(0, count);
        return;
    }

    const std::size_t chunks = (count + grain - 1) / grain;

    // Per-call completion state, shared_ptr so helper jobs that outlive this
    // call (a worker still spinning on the chunk counter) keep it alive.
    struct Batch {
        std::function<void(std::size_t, std::size_t)> fn;
        std::size_t count, grain, chunks;
        std::atomic<std::size_t> next{0};
        std::atomic<std::size_t> remaining;
        std::mutex m;
        std::condition_variable cv;
    };
    auto batch = std::make_shared<Batch>();
    batch->fn = fn;
    batch->count = count;
    batch->grain = grain;
    batch->chunks = chunks;
    batch->remaining.store(chunks, std::memory_order_relaxed);

    auto runChunks = [batch] {
        for (;;) {
            std::size_t c = batch->next.fetch_add(1, std::memory_order_relaxed);
            if (c >= batch->chunks) return;
            std::size_t begin = c * batch->grain;
            std::size_t end = std::min(begin + batch->grain, batch->count);
            batch->fn(begin, end);
            if (batch->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard lock(batch->m);
                batch->cv.notify_all();
            }
        }
    };

    // One pool job per worker; each pulls chunks from the shared counter, so
    // uneven chunk costs balance out. The calling thread joins in as well.
    const std::size_t helpers = std::min<std::size_t>(workers_.size(), chunks - 1);
    {
        std::lock_guard lock(mutex_);
        for (std::size_t i = 0; i < helpers; ++i) {
            queue_.emplace_back(runChunks);
        }
        pending_ += helpers;
    }
    wakeWorkers_.notify_all();

    runChunks();

    std::unique_lock lock(batch->m);
    batch->cv.wait(lock, [&batch] {
        return batch->remaining.load(std::memory_order_acquire) == 0;
    });
}

} // namespace ty
