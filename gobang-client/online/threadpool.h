/*

*/

#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

template <typename T> class safequeue {
  private:
    std::queue<T> m_queue; // 队列
    std::mutex m_mutex;    // 互斥锁

  public:
    safequeue() {}
    ~safequeue() {}

    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex); // 加锁，防止队列改变
        return m_queue.empty();
    }
    int size() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    void enqueue(T &t) { // 队列加入元素
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(t);
    }
    bool dequeue(T &t) { // 队列取出元素
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        t = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }
};

class threadpool {
  private:
    class threadworker {
      private:
        int m_id;           // id
        threadpool *m_pool; // 所属线程池
      public:
        threadworker(threadpool *pool, const int id) : m_id(id), m_pool(pool) {}
        void operator()() {
            std::function<void()> func; // 定义基础函数类func
            bool dequeued;              // 是否正在取出队列元素
            while (!m_pool->m_shutdown) {
                {
                    std::unique_lock<std::mutex> lock(
                        m_pool->m_condition_mutex); // 加锁
                    if (m_pool->m_queue
                            .empty()) { // //如果任务列队为空，阻塞当前线程
                        m_pool->m_condition_lock.wait(lock); // 等待条件变量通知
                    }
                    dequeued =
                        m_pool->m_queue.dequeue(func); // 取出任务队列的元素
                }
                if (dequeued) // 取出成功则执行工作函数
                    func();
            }
        }
    };
    bool m_shutdown;
    safequeue<std::function<void()>> m_queue; // 任务队列
    std::vector<std::thread> m_threads;       // 工作线程队列
    std::mutex m_condition_mutex;             // 线程休眠互斥锁
    std::condition_variable
        m_condition_lock; // 线程环境锁，可以让线程处于休眠或唤醒状态

  public:
    threadpool(const int threads = 4)
        : m_threads(std::vector<std::thread>(threads)), m_shutdown(false){};
    threadpool(const threadpool &) = delete;
    threadpool(threadpool &&) = delete;
    threadpool &operator=(const threadpool &) = delete;
    threadpool &operator=(threadpool &&) = delete;
    ~threadpool(){};

    void init() {
        for (int i = 0; i < m_threads.size(); i++) {
            m_threads.at(i) = std::thread(threadworker(this, i));
        }
    }
    void shutdown() {
        m_shutdown = true;
        m_condition_lock.notify_all(); // 唤醒所有工作线程
        for (int i = 0; i < m_threads.size(); i++) {
            if (m_threads.at(i).joinable())
                m_threads.at(i).join();
        }
    }

    // 提交函数
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        // 此处需在看看

        // 连接函数与参数
        std::function<decltype(f(args...))()> func =
            std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        // Encapsulate it into a shared pointer in order to be able to copy
        // construct
        auto task_ptr =
            std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // Warp packaged task into void function
        std::function<void()> warpper_func = [task_ptr]() { (*task_ptr)(); };

        // 队列通用安全封包函数，并压入安全队列
        m_queue.enqueue(warpper_func);

        // 唤醒一个等待中的线程
        m_condition_lock.notify_one();

        // 返回先前注册的任务指针
        return task_ptr->get_future();
    }
};
#endif /*THREADPOOL*/