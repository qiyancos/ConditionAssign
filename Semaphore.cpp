#include "Semaphore.h"

namespace condition_assign {

Semaphore::Semaphore(const int count = 1, const Type type = Normal) : 
        type_(type), count_(count), wakeupCnt_(0) {}

void Semaphore::waitOrigin() {
    std::unique_lock<std::mutex> lock {lock_};
    if (--count_ < 0) {
        condition_.wait(lock, [&]()->bool{return wakeupCnt_ > 0;});
        --wakeupCnt_;
    }
}

void Semaphore::signalOrigin() {
    std::lock_guard<std::mutex> lock {lock_};
    if (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        wait = emptyFunc;
        signal = emptyFunc;
        signalAll = emptyFunc;
    }
}

void Semaphore::signalAllOrigin() {
    std::lock_guard<std::mutex> lock {lock_};
    while (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        wait = emptyFunc;
        signal = emptyFunc;
        signalAll = emptyFunc;
    }
}

} // namesapce condition_assign
