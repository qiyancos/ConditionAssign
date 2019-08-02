#ifndef EXECUTORPOOL_H
#define EXECUTORPOOL_H

#include "Config.h"
#include "Semaphore.h"

#include <string>
#include <mutex>
#include <functional>
#include <thread>

namespace condition_assign {

// 最大的MifLayer数目，防止爆内存
#define MAX_MIFLAYERS 32

class ResourcePool;
class ExecutorPool;

// Executor类对应线程，只使用其中的函数
class Executor {
public:
    // Executor的运行状态(空闲/繁忙/出错)
    enum Status {Idle, Busy, Error};
    
    // 构造函数
    Executor(const int id, ExecutorPool* pool);
    // 析构函数
    ~Executor();
    
    // 每个线程分配的主函数
    static int mainRunner(Executor* executor);
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
    std::thread* thread_ = nullptr;
    // 所属的执行器池指针
    ExecutorPool* pool_;
    // 执行器唯一的标识ID
    const int id_;
};

// Executor工作项
class ExecutorJob {
public:
    // 工作类型
    enum JobType {
        // 加载给定的Layer数据
        LoadLayer,
        // 关闭并保存Layer
        SaveLayer,
        // 对多行配置文件的内容进行语法解析
        ParseConfigLines,
        // 为一个配置文件生成工作项
        ParseConfigFile,
        // 解析给定类型的Group
        ParseGroup,
        // 建立给定类型的Group类型
        BuildGroup,
        // 对多个Mif元素执行条件赋值操作
        ProcessMifItem
    };
    
    // 根据工作类型获取对应的执行函数
    std::function<int(void*, const int)> getJobFunc();
    
    // 构造函数，参数是一个结构体
    ExecutorJob(const JobType type, void* param);
    // 析构函数
    ~ExecutorJob();

public:
    // 当前工作项的参数
    void* param_;

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
    };

    // 执行初始化操作，然后交给executorController
    int execute();
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
    std::vector<Semaphore*> executorWakeup_;
    // 判断当前是否有执行器需要获取新的工作
    Semaphore needReadyJob_;
    // 确定是否需要进行状态检查
    Semaphore needStatusCheck_;
    // 状态检查结束
    Semaphore statusCheckOver_;
    // 运行资源的统一管控结构
    ResourcePool* resourcePool_;

    // 执行池状态锁
    std::mutex statusLock_;
    // 当前执行器池的主状态
    Status status_;

private:
    // 状态管理线程
    std::thread* executorConsole_ = nullptr;
    // 资源管理线程
    std::thread* resourceConsole_ = nullptr;
    // ExecutorPool的参数信息
    const Params params_;
    // Executor状态管理函数
    void executorController();
    // 工作项和资源管理函数
    static int resourceController(ExecutorPool* mainPool);
};

} // namespace condition_assign

#endif // EXECUTORPOOL_H
