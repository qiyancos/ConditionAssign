#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace condition_assign {

// 用于进行线程调度的信号量
class Semaphore {
public:
    enum Type {Normal, SignalFolded, OnceForAll};

    // 初始化函数
    Semaphore() = default;
    // 构造函数
    Semaphore(const int count);
    // 构造函数
    Semaphore(const int count, const Type type);
    // 析构函数
    ~Semaphore() = default;

    // 初始化函数
    int init(const int count = 0, const Type type = Normal);
    // 等待信号的到来
    void wait();
    // 发出信号，开启等待的线程
    void signal();
    // 发出信号，开启等待的线程
    void signalAll();

private:
    // 当前信号量的类型
    Type type_ = Normal;
    // 等待线程计数
    int count_ = 0;
    // 唤醒状态
    int wakeupCount_ = 0;
    // 互斥锁
    std::mutex lock_;
    // 信号量用于通知
    std::condition_variable condition_;
};

} // namespace condition_assign

#endif // SEMAPHORE_H
