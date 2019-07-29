#include "Semaphore.h"

namespace condition_assign {

Semaphore::Semaphore(const int count = 0, const Type type = Normal) {
    param_.type = type;
    param_.count = count;
    param_.wakeupCnt = 0;
}

int Semaphore::init(const int count = 0, const Type type = Normal) {
    param_.type = type;
    param_.count = count;
    param_.wakeupCnt = 0;
    return 0;
}

void Semaphore::wait() {
    param_.waitFunc(&param_);
}

void Semaphore::signal() {
    param_.signalFunc(&param_);
}

void Semaphore::signalAll() {
    param_.signalAllFunc(&param_);
}

void Semaphore::waitOrigin(Param* param) {
    std::unique_lock<std::mutex> lock(param->lock);
    if (--(param->count) < 0) {
        param->condition.wait(lock,
                [&]()->bool{return param->wakeupCnt > 0;});
        --(param->wakeupCnt);
    }
}

void Semaphore::signalOrigin(Param* param) {
    std::lock_guard<std::mutex> lock(param->lock);
    if (++(param->count) <= 0) {
        ++(param->wakeupCnt);
        param->condition.notify_one();
    }
    if (param->type == OnceForAll) {
        param->waitFunc = emptyFunc;
        param->signalFunc = emptyFunc;
        param->signalAllFunc = emptyFunc;
    }
}

void Semaphore::signalAllOrigin(Param* param) {
    std::lock_guard<std::mutex> lock {param->lock};
    while (++(param->count) <= 0) {
        ++(param->wakeupCnt);
        param->condition.notify_one();
    }
    if (param->type == OnceForAll) {
        param->waitFunc = emptyFunc;
        param->signalFunc = emptyFunc;
        param->signalAllFunc = emptyFunc;
    }
}

} // namesapce condition_assign
