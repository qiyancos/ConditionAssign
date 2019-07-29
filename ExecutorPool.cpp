#include "ExecutorPool.h"
#include "ConditionAssign.h"
#include "ExecutorJobFunc.h"
#include "Group.h"

namespace condition_assign {

Executor::Executor(const int id, ExecutorPool* pool) : id_(id), pool_(pool) {
    status_ = &(pool_->executorStatus_[id_]);
    haveReadyJob_ = pool_->executorWakeup_[id_];
    needReadyJob_ = &(pool_->needReadyJob_);
}

Executor::~Executor() {
    if (thread_ != nullptr) {
        delete thread_;
    }
}

static int Executor::mainRunner(Executor* executor) {
    ExecutorJob** workingJobPtr =
            &(executor->pool_->workingJob_[executor->id_]);
    ExecutorPool::Status poolStatus;
    while (1) {
        poolStatus = executor->pool_->status_;
        if (poolStatus == ExecutorPool::Error) {
            return -1;
        } else if (poolStatus == ExecutorPool::Finished) {
            return 0;
        }
        if (executor->pool_->resourcePool_->getReadyJob(executor->id_,
                workingJobPtr)) {
            std::function<int(void*)> jobFunc =
                    (*workingJobPtr)->getJobFunc();
            if (jobFunc((*workingJobPtr)->param_) < 0) {
                sys_log_println(_ERROR,
                        "Error occourred while running executor job %s[%d].",
                        "in executor", executor->id_);
                *(executor->status_) = Executor::Error;
                executor->pool_->needStatusCheck_.signal();
                return -1;
            } else {
                delete *workingJobPtr;
                *workingJobPtr = nullptr;
            }
        } else {
            CHECK_RET(executor->startWaiting(), "Failed to start wait mode.");
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

ExecutorJob::ExecutorJob(const JobType type, void* param) :
        type_(type), param_(param) {}

std::function<int(void*)> ExecutorJob::getJobFunc(){
    switch (type_) {
    case LoadLayer: return std::function<int(void*)>(job_func::loadLayer);
    case SaveLayer: return std::function<int(void*)>(job_func::saveLayer);
    case ParseConfigLines:
        return std::function<int(void*)>(job_func::parseConfigLines);
    case ParseConfigFile:
        return std::function<int(void*)>(job_func::parseConfigFile);
    case ParseGroup: return std::function<int(void*)>(job_func::parseGroup);
    case BuildGroup: return std::function<int(void*)>(job_func::buildGroup);
    case ProcessMifItem:
        return std::function<int(void*)>(job_func::processMifItem);
    }
}

ExecutorJob::~ExecutorJob() {
    switch (type_) {
    case LoadLayer:
        delete reinterpret_cast<job_func::LoadLayerParam*>(param_);
        break;
    case SaveLayer:
        delete reinterpret_cast<job_func::SaveLayerParam*>(param_);
        break;
    case ParseConfigLines:
        delete reinterpret_cast<job_func::ParseConfigLinesParam*>(param_);
        break;
    case ParseConfigFile:
        delete reinterpret_cast<job_func::ParseConfigFileParam*>(param_);
        break;
    case ParseGroup:
        delete reinterpret_cast<job_func::ParseGroupParam*>(param_);
        break;
    case BuildGroup:
        delete reinterpret_cast<job_func::BuildGroupParam*>(param_);
        break;
    case ProcessMifItem:
        delete reinterpret_cast<job_func::ProcessMifItemParam*>(param_);
        break;
    }
}

ExecutorPool::ExecutorPool(const Params& params) : params_(params) {}

ExecutorPool::~ExecutorPool() {
    for (Semaphore* sema : executorWakeup_) {
        delete sema;
    }
    for (ExecutorJob* job : workingJob_) {
        if (job != nullptr) {
            delete job;
        }
    }
    if (executorConsole_ != nullptr) {
        delete executorConsole_;
    }
    if (resourceConsole_ != nullptr) {
        delete resourceConsole_;
    }
}

int ExecutorPool::init() {
    CHECK_ARGS(params_.outputs.size() == params_.configs.size(),
            "Config-file's count[%d] %s [%d].", params_.outputs.size(),
            "does not match the count of output layers",
            params_.configs.size());
    CHECK_RET(Group::setInputType(params_.geoType),
            "Failed to set input layer's geometry type.");
    int totalMifLayerCount = params_.outputs.size() +
            params_.plugins.size() + 1;
    CHECK_ARGS(totalMifLayerCount <= MAX_MIFLAYERS,
            "Too many mif layers[%d] to be opened.", totalMifLayerCount);
    int executorCnt = params_.executorNum;
    if (executorCnt > std::thread::hardware_concurrency()) {
        std::cerr << "Warning: thread number is greater than the";
        std::cerr << " cpu cores in this computer." << std::endl;
    }
    resourcePool_ = new ResourcePool();
    CHECK_RET(resourcePool_->init(params_), "ResourcePool failed to init.");
    workingJob_.resize(executorCnt, nullptr);
    executorStatus_.resize(executorCnt, Executor::Idle);
    for (int id = 0; id < executorCnt; id++) {
        executors_.push_back(Executor(id, this));
        executorWakeup_.push_back(new Semaphore(0));
    }
    status_ = Idle;
    // 初始化信号量
    needReadyJob_.init(1);
    return 0;
}

int ExecutorPool::execute() {
    // 初始化工作内容
    std::vector<ExecutorJob*> initJobs;
    initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
            new job_func::LoadLayerParam {ResourcePool::Input,
            &(params_.input), -1, resourcePool_}));
    for (int i = 0; i < params_.outputs.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
                new job_func::LoadLayerParam {ResourcePool::Output,
                &(params_.outputs[i]), i, resourcePool_}));
    }
    for (int i = 0; i < params_.plugins.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::LoadLayer,
                new job_func::LoadLayerParam {ResourcePool::Plugin,
                &(params_.plugins[i]), i, resourcePool_}));
    }
    for (int i = 0; i < params_.configs.size(); i++) {
        initJobs.push_back(new ExecutorJob(ExecutorJob::ParseConfigFile,
                new job_func::ParseConfigFileParam {
                &(params_.configs[i]), i, resourcePool_}));
    }
    resourcePool_->candidateQueueLock_.lock();
    for (int i = 0; i < initJobs.size(); i++) {
        resourcePool_->candidateQueue_.push(initJobs[i]);
    }
    // 插入初始化的工作内容
    resourcePool_->candidateQueueLock_.unlock();
    resourceConsole_ = new std::thread(resourceController, this);
    for (int id = 0; id < params_.executorNum; id++) {
        executors_[id].thread_ = new std::thread(Executor::mainRunner,
                &(executors_[id]));
    }
    executorConsole_ = new std::thread(executorController, this);
    resourcePool_->newCandidateJob_.signal();
    // 等待所有子线程结束
    executorConsole_->join();
    resourceConsole_->join();
    for (int id = 0; id < params_.executorNum; id++) {
        executors_[id].thread_->join();
    }
    CHECK_ARGS(status_ == Finished, "Main procedure exit with bad status.");
    return 0;
}

int ExecutorPool::resourceController(ExecutorPool* mainPool) {
    std::set<int> wakeupExecutorID;
    bool wakeupByNewCandidateJob = false;
    while (1) {
        wakeupExecutorID.clear();
        if (!wakeupByNewCandidateJob) {
            mainPool->needReadyJob_.wait();
        } else {
            wakeupByNewCandidateJob = false;
        }
        CHECK_RET(mainPool->resourcePool_->selectReadyJob(&wakeupExecutorID),
                "Failed to select ready job from candidate queue.");
        if (wakeupExecutorID.empty()) {
            mainPool->needStatusCheck_.signal();
            mainPool->statusCheckOver_.wait();
            if (mainPool->status_ == Finished || mainPool->status_ == Error) {
                return 0;
            }
            mainPool->resourcePool_->newCandidateJob_.wait();
            wakeupByNewCandidateJob = true;
        } else {
            for (auto id : wakeupExecutorID) {
                mainPool->executorWakeup_[id]->signal();
            }
        }
    }
}

int ExecutorPool::executorController(ExecutorPool* mainPool) {
    mainPool->status_ = Running;
    while (mainPool->status_ != Error) {
        // 判断是否发生了错误
        for (auto status : mainPool->executorStatus_) {
            if (status == Executor::Error) {
                mainPool->status_ = Error;
                mainPool->statusCheckOver_.signal();
                return -1;
            }
        }
        {
            // 判断是否可以结束
            std::lock_guard<std::mutex> candidateLockGuard(
                    mainPool->resourcePool_->candidateQueueLock_);
            if (mainPool->resourcePool_->candidateQueue_.empty() &&
                    mainPool->resourcePool_->readyJobCnt_ == 0) {
                bool noWorkingJob = true;
                for (auto job : mainPool->workingJob_) {
                    if (job != nullptr) {
                        noWorkingJob = false;
                        break;
                    }
                }
                if (noWorkingJob == true) {
                    mainPool->status_ = Finished;
                    mainPool->statusCheckOver_.signal();
                    return 0;
                }
            }
        }
        mainPool->statusCheckOver_.signal();
        mainPool->needStatusCheck_.wait();
    }
    return 0;
}

} // namespace condition_assign
