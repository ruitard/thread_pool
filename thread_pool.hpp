#include <future>
#include <queue>
#include <atomic>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <condition_variable>

namespace tardis {

class thread_pool final {
private:
    using task_type = std::function<void()>;
    size_t pool_size;
    std::mutex tq_mtx;
    std::atomic_bool running = false;
    std::condition_variable tardis_cv;
    std::queue<task_type> task_queue;
    std::unordered_map<std::thread::id, std::thread> threads;
    void make_worker();
    void work();
public:
    template<typename Fn, typename... Args>
    auto async(Fn&&, Args&&...) -> std::future<std::invoke_result_t<Fn, Args...>>;
    thread_pool(size_t sz = std::thread::hardware_concurrency() * 2 + 1);
    ~thread_pool();
    thread_pool(thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator = (thread_pool&) = delete;
    thread_pool& operator = (thread_pool&&) = delete;
    void add_task(task_type&&);
    void add_worker(size_t n);
    size_t unsafe_size();
    size_t tq_size();
    
};

thread_pool::thread_pool(size_t sz) : pool_size{sz}, running{true} {
    for (size_t i = 0; i < pool_size; i++) {
        make_worker();
    }
}

thread_pool::~thread_pool() {
    tq_mtx.lock();
    running = false;
    tq_mtx.unlock();
    tardis_cv.notify_all();
    for (auto& it : threads) {
        if (it.second.joinable()) {
            it.second.join();
        }
    }
}

inline void thread_pool::work() {
    while (running) {
        task_type task;
        {
            std::unique_lock<std::mutex> ulck{tq_mtx};
            if (!task_queue.empty()) {
                task = std::move(task_queue.front());
                task_queue.pop();
            } else {
                tardis_cv.wait(ulck, [&]() -> bool {
                    return !task_queue.empty() or !running;
                });
            }
        }
        if (task) {
            task();
        }
    }
}

inline void thread_pool::make_worker() {
    std::thread t(&thread_pool::work, this);
    threads[t.get_id()] = move(t);
}

inline void thread_pool::add_worker(size_t n = 1) {
    for (size_t i = 0; i < n; i++) {
        make_worker();
    }
    pool_size += n;
}

inline void thread_pool::add_task(task_type&& task) {
    {
        std::lock_guard<std::mutex> lg{tq_mtx};
        task_queue.emplace(move(task));
    }
    tardis_cv.notify_one();
}
    
template<typename Fn, typename... Args>
inline auto 
thread_pool::async(Fn&& fn, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>> {
    using async_result_t = std::invoke_result_t<Fn, Args...>;
    auto wrap_task_ptr = std::make_shared<std::packaged_task<async_result_t()>>(
        std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)
    );
    auto result = wrap_task_ptr->get_future();
    {
        std::lock_guard<std::mutex> lg{tq_mtx};
        task_queue.emplace([wrap_task_ptr](){
            (*wrap_task_ptr)();
        }); 
    }
    tardis_cv.notify_one();
    return result;
}

inline size_t thread_pool::unsafe_size() {
    return pool_size;
}

inline size_t thread_pool::tq_size() {
    std::lock_guard<std::mutex> tq_lck{tq_mtx};
    return task_queue.size();
}

}