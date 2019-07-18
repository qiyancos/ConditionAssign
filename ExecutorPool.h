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
// 单个Executor处理的WorkItem数量上限
#define MAX_WORKITEM_CNT 1000

#ifndef RESOURCEPOOL_H
class ResourcePool;
#endif

// Executor类对应线程，只使用其中的函数
class Executor {
public:
    // Executor的运行状态(空闲/繁忙)
    enum Status {Idle, Busy};
    // 构造函数
    Executor(const int id);
    // 每个线程分配的主函数
    static int mainRunner(const Executor& executor);
    // Executor进入等待状态的处理函数
    static int startWaiting();

public:
    // 所属执行器池的指针
    static ExecutorPool* pool_;
    // 执行器唯一的标识ID
    const int id_;
}

ExecutorPool* Executor::pool_ = nullptr;

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
        // 建立给定类型的Group类型
        BuildGroup,
        // 对多个Mif元素执行条件赋值操作
        MifItemProcess
    };
    
    // 构造函数，参数是一个结构体
    ExecutorJob(const JobTypes type, const int targetID,
            const int mifItemIndex, void* param);
    // 根据工作类型获取对应的执行函数
    std::function<int(void*)> getJobFunc(JobTypes);

public:
    // 加载目标Layer的函数
    int loadLayer(void* param);
    // 关闭和保存Layer的函数
    int saveLayer(void* param);
    // 对多行配置文件的内容进行语法解析的函数
    int parseConfigLine(void* param);
    // 为一个配置文件生成工作项的函数
    int parseConfigFile(void* param);
    // 建立给定类型的Group类型的函数
    int buildGroup(void* param);
    // 对多个Mif元素执行条件赋值操作的函数
    int mifItemProcess(void* param);

public:
    // 加载目标Layer函数参数
    struct LoadLayerParams {
    };
    
    // 关闭保存目标Layer函数参数
    struct SaveLayerParams {
    };
    
    // 配置文件语法解析函数参数
    struct ConfigLineParseParams {
    };

    // 配置文件工作项生成函数参数
    struct ConfigFileParseParams {
    };

    // 建立数据组函数参数
    struct BuildGroupParams {
    };

    // 对Mif进行条件赋值的参数
    struct MifItemProcess {
    };

private:
    // 当前工作项的类型
    const JobTypes type_;
    // 当前工作项的参数
    const void* param_;
    // 目标层的通用ID
    const int targetID_;
    // 处理的MifItem对应的Index，没有则为-1
    const int mifItemIndex_;
};

class ExecutorPool {
public:
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
    // 构造函数，需要给出对应参数
    ExecutorPool(const Params& params);
    // 析构函数
    ~ExecutorPool();

public:
    // Executor工作状态
    std::vector<Executor::Status> executorStatus;
    // Executor工作状态的互斥锁
    std::mutex executorStatusLock_;
    // 运行资源的统一管控结构
    ResourcePool resource_;

private:
    // ExecutorPool的参数信息
    const Params params_;
    // 工作项和Executor管理函数
    int executorController();
};

} // namespace condition_assign

#endif // EXECUTOR_H
