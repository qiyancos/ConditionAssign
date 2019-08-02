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
    enum Type {Normal, OnceForAll};
    struct Param {
        // 当前信号量的类型
        Type type;
        // 等待线程计数
        int count;
        // 唤醒状态
        int wakeupCnt;
        // 互斥锁
        std::mutex lock;
        // 信号量用于通知
        std::condition_variable condition;

        // 等待信号的到来
        std::atomic<void(*)(Param*)> waitFunc {waitOrigin};
        // 发出信号，开启等待的线程
        std::atomic<void(*)(Param*)> signalFunc {signalOrigin};
        // 发出信号，开启等待的线程
        std::atomic<void(*)(Param*)> signalAllFunc {signalOrigin};
    };

    // 初始化函数
    Semaphore(const int count = 0, const Type type = Normal);
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
    // 等待信号的到来
    static void waitOrigin(Param* param);
    // 发出信号，开启等待的线程
    static void signalOrigin(Param* param);
    // 发出信号，开启等待的线程
    static void signalAllOrigin(Param* param);

    // 空等待函数
    static void emptyFunc(Param* param) {}

private:
    // 信号量的参数
    Param param_;
};

} // namespace condition_assign

#endif // SEMAPHORE_H
