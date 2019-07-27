#ifndef SEMAPHORE_H
#define SEMAPHORE_H

namespace condition_assign {

// 用于进行线程调度的信号量
class Semaphore {
public:
    enum Type {Normal, OnceForAll};
    
    // 构造函数
    Semaphore(const int count = 1, const Type type = Normall);
    // 析构函数
    ~Semaphore() = default;
    
    // 等待信号的到来
    std::function<void()> wait = waitOrigin;
    // 发出信号，开启等待的线程
    std::function<void()> signal = signalOrigin;
    // 发出信号，开启等待的线程
    std::function<void()> signalAll = signalOrigin;

private:
    // 等待信号的到来
    void waitOrigin();
    // 发出信号，开启等待的线程
    void signalOrigin();
    // 发出信号，开启等待的线程
    void signalAllOrigin();
    
    // 空等待函数
    void emptyFunc() {}

private:
    // 当前信号量的类型
    Type type_;
    // 等待线程计数
    int count_;
    // 唤醒状态
    int wakeupCnt_;
    // 互斥锁
    std::mutex lock_;
    // 信号量用于通知
    std::condition_variable condition_;
}

} // namespace condition_assign

#endif // SEMAPHORE_H
