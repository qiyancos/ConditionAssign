#ifndef EXECUTORPOOL_H
#define EXECUTORPOOL_H

#include <string>
#include <mutex>
#include <function>
#include "ResourcePool.h"
#include "Config.h"

namespace condition_assign {

// 最大的MifLayer数目，防止爆内存
#define MAX_MIFLAYERS 32
// 候选工作队列的数量
#define MAX_CANDIDATE_SIZE 4

#ifndef RESOURCEPOOL_H
class ResourcePool;
#endif
class ExecutorPool;

// 用于进行线程调度的信号量
class Semaphore {
public:
    enum Type {Normal, OnceForAll};
    // 构造函数
    Semaphore(const int count = 1, const Type type = Normall);
    // 等待信号的到来
    void wait();
    // 发出信号，开启等待的线程
    void signal();
    // 发出信号，开启等待的线程
    void signalAll();

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

// Executor类对应线程，只使用其中的函数
class Executor {
public:
    // Executor的运行状态(空闲/繁忙/出错)
    enum Status {Idle, Busy, Error};
    // 构造函数
    Executor(const int id, ExecutorPool* pool);
    // 每个线程分配的主函数
    static int mainRunner(const Executor& executor);
    // Executor进入等待状态的处理函数
    int startWaiting();

public:
    // 当前执行对应status状态指针
    Status* status_;
    // 用于通知执行器池需要分配新的工作项
    Semaphore* needReadyJob_;
    // 执行器会通过该信号量通知本线程有新的工作项
    Semaphore* haveReadyJob_;
    // 当前执行器对应线程的指针
    std::thread* thread_;
    // 所属的执行器池指针
    ExecutorPool* pool_;
    // 执行器唯一的标识ID
    const int id_;
}

// Executor工作项
class ExecutorJob {
public:
    // 工作类型
    enum JobTypes {
        // 加载给定的Layer数据
        LoadLayer,
        // 关闭并保存Layer
        SaveLayer,
        // 对多行配置文件的内容进行语法解析
        ConfigLineParse,
        // 为一个配置文件生成工作项
        ConfigFileParse,
        // 解析给定类型的Group
        ParseGroup,
        // 建立给定类型的Group类型
        BuildGroup,
        // 对多个Mif元素执行条件赋值操作
        MifItemProcess
    };
    
    // 构造函数，参数是一个结构体
    ExecutorJob(const JobTypes type, const int targetID,
            const int mifItemIndex, void* param);
    // 根据工作类型获取对应的执行函数
    std::function<int(void*)> getJobFunc();
    // 析构函数
    ~ExecutorJob();

public:
    // 当前工作项的参数
    const void* param_;
    // 目标层的通用ID
    const int targetID_;
    // 处理的MifItem对应的Index，没有则为-1
    const int mifItemIndex_;

private:
    // 当前工作项的类型
    const JobType type_;
};

class ExecutorPool {
public:
    enum Status {Running, Idle, Finished, Error};
    // ExecutorPool的参数结构体
    struct Params {
        // ExecutorPool中Executor的数目上限
        int executorNum;
        // 输入Layer的完整路径
        std::string input;
        // 输入Layer的地理结构类型
        std::string geoType;
        // 输出Layer的完整路径
        std::vector<std::string> outputs;
        // 外挂Layer的完整路径
        std::vector<std::string> plugins;
        // 使用的所有配置文件的完整路径
        std::vector<std::string> configs;
    }

    // 执行初始化操作，然后交给executorController
    int execute();
    // 获取当前执行器池的状态
    Status getStatus();
    // 初始化函数
    int init();
    // 构造函数，需要给出对应参数
    ExecutorPool(const Params& params);
    // 析构函数
    ~ExecutorPool();

public:
    // 执行器队列
    std::vector<Executor> executors_;
    // Executor工作状态
    std::vector<Executor::Status> executorStatus_;
    // 当前所有Executor正在执行的工作
    std::vector<ExecutorJob*> workingJob_;
    // 当前所有Executor和执行器池交互的信号量
    std::vector<Semaphore> executorWakeup_(0, Semaphore::WithCount);
    // 判断当前是否有执行器需要获取新的工作
    Semaphore needReadyJob_(1);
    // 确定是否需要进行状态检查
    Semaphore needStatusCheck_(0);
    // 状态检查结束
    Semaphore statusCheckOver_(0);
    // 运行资源的统一管控结构
    ResourcePool resourcePool_;

private:
    // 执行池状态锁
    std::mutex statusLock_;
    // 当前执行器池的主状态
    Status status_;

    // 状态管理线程
    std::thread* executorConsole_;
    // 资源管理线程
    std::thread* resourceConsole_;
    // ExecutorPool的参数信息
    const Params params_;
    // Executor状态管理函数
    int executorController();
    // 工作项和资源管理函数
    int resourceController();
};

} // namespace condition_assign

#endif // EXECUTOR_H
