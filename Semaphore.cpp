#include "Semaphore.h"

#include <iostream>
#include <limits.h>

namespace condition_assign {

Semaphore::Semaphore(const int count) : count_(count) {}

Semaphore::Semaphore(const int count, const Type type) :
        type_(type), count_(count){}

int Semaphore::init(const int count, const Type type) {
    type_ = type;
    count_ = count;
    wakeupCount_ = 0;
    return 0;
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lock(lock_);
    if (--count_ < 0) {
        condition_.wait(lock,
                [&]()->bool{return wakeupCount_ > 0;});
        --wakeupCount_;
    }
}

void Semaphore::signal() {
    std::lock_guard<std::mutex> lock(lock_);
    if (count_ != INT_MAX) {
        if (++count_ <= 0) {
            ++wakeupCount_;
            condition_.notify_one();
        }
        if (type_ == SignalFolded) {
            count_ = 1;
        } else if (type_ == OnceForAll) {
            count_ = INT_MAX;
        }
    }
}

void Semaphore::signalAll() {
    std::lock_guard<std::mutex> lock(lock_);
    if (count_ != INT_MAX) {
        while (++count_ <= 0) {
            ++wakeupCount_;
            condition_.notify_one();
        }
        if (type_ == SignalFolded) {
            count_ = 1;
        } else if (type_ == OnceForAll) {
            count_ = INT_MAX;
        }
    }
}

} // namesapce condition_assign
