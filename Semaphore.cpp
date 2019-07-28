#include "Semaphore.h"

namespace condition_assign {

Semaphore::Semaphore(const int count = 0, const Type type = Normal) { 
    param.type = type;
    param.count = count;
    param.wakeupCnt = 0; 
}

int Semaphore::init(const int count = 0, const Type type = Normal) { 
    param.type = type;
    param.count = count;
    param.wakeupCnt = 0; 
    return 0;
}

void Semaphore::wait() {
    waitFunc(&param_);
}

void Semaphore::signal() {
    signalFunc(&param_);
}

void Semaphore::signalAll() {
    signalAllFunc(&param_);
}

void Semaphore::waitOrigin(Parami* param) {
    std::unique_lock<std::mutex> lock(lock_);
    if (--count_ < 0) {
        condition_.wait(lock, [&]()->bool{return wakeupCnt_ > 0;});
        --wakeupCnt_;
    }
}

void Semaphore::signalOrigin() {
    std::lock_guard<std::mutex> lock(lock_);
    if (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        waitFunc = emptyFunc;
        signalFunc = emptyFunc;
        signalAllFunc = emptyFunc;
    }
}

void Semaphore::signalAllOrigin() {
    std::lock_guard<std::mutex> lock {lock_};
    while (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        waitFunc = emptyFunc;
        signalFunc = emptyFunc;
        signalAllFunc = emptyFunc;
    }
}

} // namesapce condition_assign
