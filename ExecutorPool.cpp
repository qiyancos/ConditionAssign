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
        int newJob = executor->pool_->resourcePool_->getReadyJob(executor->id_,
                workingJobPtr);
        if (newJob > 0) {
            TEST(executor->id_);
            std::function<int(void*, const int)> jobFunc =
                    (*workingJobPtr)->getJobFunc();
            if (jobFunc((*workingJobPtr)->param_, executor->id_) < 0) {
                sys_log_println(_ERROR,
                        "Error occourred while running executor job %s[%d].\n",
                        "in executor", executor->id_);
                *(executor->status_) = Executor::Error;
                executor->pool_->needStatusCheck_.signal();
                return -1;
            } else {
                delete *workingJobPtr;
                *workingJobPtr = nullptr;
            }
        } else if (newJob == 0) {
            CHECK_RET(executor->startWaiting(), "Failed to start wait mode.");
        } else {
            sys_log_println(_ERROR,
                    "Error occourred while get ready job in executor[%d].\n",
                    executor->id_);
            *(executor->status_) = Executor::Error;
            executor->pool_->needStatusCheck_.signal();
            return -1;
        }
    }
}

int Executor::startWaiting() {
    TEST(id_);
    CHECK_ARGS(*status_ == Busy, "Executor status should be Busy.");
    *status_ = Idle;
    needReadyJob_->signal();
    haveReadyJob_->wait();
    *status_ = Busy;
    return 0;
}

ExecutorJob::ExecutorJob(const JobType type, void* param) :
        type_(type), param_(param) {}

std::function<int(void*, const int)> ExecutorJob::getJobFunc(){
    switch (type_) {
    case LoadLayer:
            return std::function<int(void*, const int)>(job_func::loadLayer);
    case SaveLayer:
            return std::function<int(void*, const int)>(job_func::saveLayer);
    case ParseConfigLines:
        return std::function<int(void*, const int)>(
                job_func::parseConfigLines);
    case ParseConfigFile:
        return std::function<int(void*, const int)>(job_func::parseConfigFile);
    case ParseGroup:
        return std::function<int(void*, const int)>(job_func::parseGroup);
    case BuildGroup:
        return std::function<int(void*, const int)>(job_func::buildGroup);
    case ProcessMifItems:
        return std::function<int(void*, const int)>(job_func::processMifItems);
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
    case ProcessMifItems:
        delete reinterpret_cast<job_func::ProcessMifItemsParam*>(param_);
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
    CHECK_RET(resourcePool_->init(params_, &(needReadyJob_)),
            "ResourcePool failed to init.");
    workingJob_.resize(executorCnt, nullptr);
    executorStatus_.resize(executorCnt, Executor::Busy);
#ifdef DEBUG
    debugStream.push_back(std::ofstream((debugLogDir + "/main.log").c_str(),
            std::ofstream::out));
    debugStream.push_back(std::ofstream((debugLogDir + "/rc.log").c_str(),
            std::ofstream::out));
#endif
    for (int id = 0; id < executorCnt; id++) {
        executorWakeup_.push_back(new Semaphore(0));
#ifdef DEBUG
        debugStream.push_back(std::ofstream((debugLogDir + "/" +
                std::to_string(id) + ".log").c_str(), std::ofstream::out));
#endif
    }
    for (int id = 0; id < executorCnt; id++) {
        executors_.push_back(Executor(id, this));
    }
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
    for (int i = 0; i < initJobs.size(); i++) {
        resourcePool_->candidateQueue_.push(initJobs[i]);
    }
    // 插入初始化的工作内容
    status_ = Running;
    resourceConsole_ = new std::thread(resourceController, this);
    for (int id = 0; id < params_.executorNum; id++) {
        executors_[id].thread_ = new std::thread(Executor::mainRunner,
                &(executors_[id]));
    }
    // 等待所有子线程结束
    executorController();
    needReadyJob_.signal();
    resourceConsole_->join();
    for (int id = 0; id < params_.executorNum; id++) {
        executorWakeup_[id]->signal();
        executors_[id].thread_->join();
    }
    CHECK_ARGS(status_ == Finished, "Main procedure exit with bad status.");
    return 0;
}

int ExecutorPool::resourceController(ExecutorPool* mainPool) {
    TEST("rc")
    std::set<int> wakeupExecutorID;
    while (1) {
        wakeupExecutorID.clear();
        TEST("rc")
        mainPool->needReadyJob_.wait();
        if (mainPool->status_ == Error || mainPool->status_ == Finished) {
            return 0;
        }
        TEST("rc")
        CHECK_RET(mainPool->resourcePool_->selectReadyJob(&wakeupExecutorID),
                "Failed to select ready job from candidate queue.");
        if (wakeupExecutorID.empty()) {
            TEST("rc")
            mainPool->needStatusCheck_.signal();
            mainPool->statusCheckOver_.wait();
            if (mainPool->status_ == Finished || mainPool->status_ == Error) {
                return 0;
            }
            TEST("rc")
        } else {
            TEST("rc")
            for (auto id : wakeupExecutorID) {
                mainPool->executorWakeup_[id]->signal();
            }
        }
    }
}

void ExecutorPool::executorController() {
    TEST("main");
    bool alreadySaved = false;
    while (status_ != Error) {
        TEST("main");
        needStatusCheck_.wait();
        TEST("main")
        // 判断是否发生了错误
        for (auto status : executorStatus_) {
            if (status == Executor::Error) {
                status_ = Error;
                statusCheckOver_.signal();
                return -1;
            }
        }
        // 判断是否可以结束
        std::lock_guard<std::mutex> candidateLockGuard(
                resourcePool_->candidateQueueLock_);
        if (resourcePool_->candidateQueue_.empty() &&
                resourcePool_->readyJobCnt_ == 0) {
            TEST("main")
            bool noWorkingJob = true;
            for (auto job : workingJob_) {
                if (job != nullptr) {
                    noWorkingJob = false;
                    break;
                }
            }
            if (noWorkingJob == true && alreadySaved == false) {
                TEST("main");
                for (int i = 0; i < params_.outputs.size(); i++) {
                    resourcePool_->candidateQueue_.push(
                            new ExecutorJob(ExecutorJob::SaveLayer,
                            new job_func::SaveLayerParam {i, resourcePool_}));
                }
                alreadySaved = true;
                needReadyJob_.signal();
            } else if (noWorkingJob == true && alreadySaved == true) {
                status_ = Finished;
                statusCheckOver_.signal();
                return 0;
            }
        }
        TEST("main")
        statusCheckOver_.signal();
    }
    return 0;
}

} // namespace condition_assign
