#include "threading_utils.hpp"
#include <atomic>

std::atomic<unsigned> nextId = 0;

unsigned CurrentThreadId() {

    thread_local unsigned id = nextId.fetch_add(1);
    return id;
}

ThreadManager& ThreadManager::Get() {
    static ThreadManager m;
    return m;
}

void ThreadManager::RunJobs(std::vector<std::function<void()>> jobs) {
    Assert(remainingJobs.empty());
    std::unique_lock lock(remainingJobsMutex);
    remainingJobs = jobs;
    notifyWorkers.notify_all();    
    notifyMain.wait(lock, [this]() {return remainingJobs.empty() && numIdleWorkers == threads.size(); });
    lock.unlock();
}


ThreadManager::ThreadManager() {
    WORKER_THREAD_ID = -1;

    for (unsigned i = 0; i < MAX_WORKER_THREADS; i++) {
        threads.emplace_back(ThreadMain, i);
        numIdleWorkers++;
    }
}

ThreadManager::~ThreadManager() {
    remainingJobsMutex.lock();
    killWorkers = true;
    notifyWorkers.notify_all();
    remainingJobsMutex.unlock();

    for (auto& t : threads) t.join();
}

void ThreadManager::ThreadMain(int id) {
    WORKER_THREAD_ID = id;
    auto& m = ThreadManager::Get();
    Assert(id < MAX_WORKER_THREADS);

    while (true) {
        std::unique_lock lock(m.remainingJobsMutex);
        m.notifyWorkers.wait(lock, [&m]() {return m.killWorkers || !m.remainingJobs.empty(); });

        if (m.killWorkers) {
            return;
        }
        else if (!m.remainingJobs.empty()) {
            Assert(m.numIdleWorkers != 0);
            m.numIdleWorkers--;
            auto job = m.remainingJobs.back();
            m.remainingJobs.pop_back();
            lock.unlock();
            //m.notifyWorkers.notify_one();
            job();
            lock.lock();
            m.numIdleWorkers++;
            m.notifyMain.notify_one();
            lock.unlock();
        }   

    }
}
