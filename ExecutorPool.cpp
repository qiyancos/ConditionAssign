#include "ExecutorPool.h"
#include "ConditionAssign.h"
#include "ExecutorJobFunc.h"

namespace condition_assign {

Semaphore::Semaphore(const int count = 1, const Type type = Normal) : 
        type_(type), count_(count), wakeupCnt_(0) {}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lock {lock_};
    if (--count_ < 0) {
        condition_.wait(lock, [&]()->bool{return wakeupCnt_ > 0;});
        --wakeupCnt_;
    }
}

void Semaphore::signal() {
    std::lock_guard<std::mutex> lock {lock_};
    if (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        count_ = INT_MAX;
    }
}

void Semaphore::signalAll() {
    std::lock_guard<std::mutex> lock {lock_};
    while (++count_ <= 0) {
        ++wakeupCnt_;
        condition_.notify_one();
    }
    if (type_ == OnceForAll) {
        count_ = INT_MAX;
    }
}

Executor::Executor(const int id, ExecutorPool* pool) : id_(id), pool_(pool) {
    status_ = &(pool_->executorStatus_[id_]);
    haveReadyJob_ = &(pool_->executorWakeup_[id_]);
    needReadyJob_ = &(pool_->needReadyJob_);
}

static int Executor::mainRunner(const Executor& executor) {
    ExecutorJob** workingJobPtr = &(executor.pool_->workingJob_[executor.id_]);
    ExecutorPool::Status poolStatus;
    while (1) {
        poolStatus = executor.pool_->getStatus();
        if (poolStatus == ExecutorPool::Error) {
            return -1;
        } else if (poolStatus == Executor::Finished) {
            return 0;
        }
        if (executor.pool_->resourcePool.getReadyJob(executor.id_,
                workingJobPtr)) {
            std::function<int(void*)> jobFunc =
                    (*workingJobPtr)->getJobFunc();
            if (jobFunc((*workingJobPtr)->param_) < 0) {
                sys_log_println(_ERROR,
                        "Error occourred while running executor job.");
                executor.status_ = Executor::Error;
                executor.needStatusCheck_->signal();
                return -1;
            } else {
                delete (*workingJobPtr);
                *workingJobPtr = nullptr;
            }
        } else {
            CHECK_RET(executor.startWaiting(), "Failed to start wait mode.");
        }
    }
}

int Executor::startWaiting() {
    CHECK_ARGS(*status_ == Busy, "Executor status should be Busy.");
    *status_ = Idle;
    needReadyJob_->signal();
    haveReadyJob_->wait();
    *status_ = Busy;
    return 0;
}

ExecutorJob::ExecutorJob(const JobType type, const int targetID,
        const int mifItemIndex, void* param) : type_(type), targetID_(targetID),
        mifItemIndex_(mifItemIndex), param_(param) {}

std::function<int(void*)> ExecutorJob::getJobFunc(){
    switch (type_) {
    case LoadLayer: return std::function<int(void*)>(job_func::loadLayer);
    case SaveLayer: return std::function<int(void*)>(job_func::saveLayer);
    case ConfigLineParse:
        return std::function<int(void*)>(job_func::parseConfigLines);
    case ConfigFileParse:
        return std::function<int(void*)>(job_func::parseConfigFile);
    case ParseGroup: return std::function<int(void*)>(job_func::parseGroup);
    case BuildGroup: return std::function<int(void*)>(job_func::buildGroup);
    case MifItemProcess:
        return std::function<int(void*)>(job_func::processMifItem);
    }
}

ExecutorJob::~ExecutorJob() {delete param_;}

ExecutorPool::ExecutorPool(const Params& params) : params_(params) {}

int ExecutorPool::init() {
    CHECK_ARGS(params_.outputs.size() == params_.configs.size(),
            "Config-file's count[%d] %s [%d]."
            params_.outputs.size(),
            "does not match the count of output layers",
            params_.configs.size());
    int totalMifLayerCount = params_.outputs.size() +
            params_.plugins.size() + 1;
    CHECK_ARGS(totalMifLayerCount <= MAX_MIFLAYERS,
            "Too many mif layers[%d] to be opened.", totalMifLayerCount);
    int executorCnt = params_.executorNum;
    if (executorNum > thread::hardware_concurrency()) {
        std::cerr << "Warning: thread number is greater than the";
        std::cerr << " cpu cores in this computer." << std::endl;
    }
    CHECK_RET(resourcePool_.init(params_.outputs.size(),
            params_.plugins.size(), executorCnt, executorCnt >
            MAX_CANDIDATE_SIZE ? MAX_CANDIDATE_SIZE : executorCnt),
            "ResourcePool failed to init.");
    workingJob_.resize(executorCnt, nullptr);
    executorWakeup_.resize(executorCnt, Semaphore(0));
    executorStatus_.resize(executorCnt, Executor::Idle);
    for (int id = 0; id < executorCnt; id++) {
        executors_.push_back(Executor(id, this));
    }
    status_ = Idle;
}

int ExecutorPool::execute() {
    // 初始化工作内容
    std::vector<ExecutoJob*> initJobs;
    initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
            new job_func::LoadLayerParams {ReourcePool::Input,
            &(params_.input), &resourcePool_}));
    for (int i = 0; i < params_.outputs.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
                new job_func::LoadLayerParams {ReourcePool::Output,
                &(params_.outputs[i]), &resourcePool_}));
    }
    for (int i = 0; i < params_.plugins.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
                new job_func::LoadLayerParams {ReourcePool::Plugin,
                &(params_.plugins[i]), &resourcePool_}));
    }
    for (int i = 0; i < params_.configs.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::parseConfigFile,
                new job_func::ParseConfigFileParams {
                &(params_.configs[i]), i, &resourcePool_}));
    }
    resourcePool_.candidateQueueLock_.lock();
    for (int i = 0; i < initJobs.size(); i++) {
        resourcePool_.candidateQueue_.push(initJobs[i]);
    }
    // 插入初始化的工作内容
    resourcePool_.candidateQueueLock_.unlock();
    resourceConsole_ = new std::thread(resourceController);
    for (int id = 0; id < params_.executorNum; id++) {
        executors_[id].thread = new std::thread(Executor::mainRunner,
                executors_[id]);
    }
    executorConsole_ = new std::thread(executorController);
    resourcePool_.newCandidateJob.signal();
    // 等待所有子线程结束
    executorConsole_->join();
    resourceConsole_->join();
    for (int id = 0; id < params_.executorNum; id++) {
        executors_[id].thread->join();
    }
    CHECK_ARGS(status_ == Finished, "Main procedure exit with bad status.");
    return 0;
}

int ExecutorPool::resourceController() {
    std::set<int> wakeupExecutorID;
    bool wakeupByNewCandidateJob = false;
    while (1) {
        wakeupExecutorID.clear();
        if (!wakeupByNewCandidateJob) {
            needReadyJob_.wait();
        } else {
            wakeupByNewCandidateJob = false;
        }
        CHECK_RET(resourcePool_.selectReadyJob(&wakeupExecutorID),
                "Failed to select ready job from candidate queue.");
        if (wakeupExecutorID.empty()) {
            needStatusCheck_.signal();
            statusCheckOver_.wait();
            if (status_ == Finished || status_ == Error) {
                return 0;
            }
            resourcePool_.newCandidateJob.wait();
            wakeupByNewCandidateJob = true;
        } else {
            for (auto id : wakeupExecutorID) {
                executorWakeup_[id].signal();
            }
        }
    }
}

int ExecutorPool::executorController() {
    status_ = Running;
    while (status_ != Error) {
        // 判断是否发生了错误
        for (auto status : executorStatus_) {
            if (status == Executor::Error) {
                status_ = Error;
                statusCheckOver().signal();
                return -1;
            }
        }
        {
            // 判断是否可以结束
            std::lock_guard<std::mutex> candidateLockGuard(
                    resourcePool_.candidateQueueLock_);
            std::lock_guard<std::mutex> readyLockGuard(
                    resourcePool_.readyQueueLock_);
            if (resourcePool_.candidateQueue_.empty() &&
                    resourcePool_.readyJobCnt_ == 0) {
                bool noWorkingJob = true;
                for (auto job : workingJob_) {
                    if (job != nullptr) {
                        noWorkingJob = false;
                        break;
                    }
                }
                if (noWorkingJob == true) {
                    status_ = Finished;
                    statusCheckOver().signal();
                    return 0;
                }
            }
        }
        statusCheckOver().signal();
        needStatusCheck_.wait();
    }
    return 0;
}

} // namespace condition_assign
