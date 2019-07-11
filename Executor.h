#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include <mutex>
#include <function>
#include "Config.h"
#include "Layer.h"

namespace condition_assign {

// 候选工作队列的数量
#define MAX_CANDIDATE_SIZE 4
// 单个Executor处理的WorkItem数量上限
#define MAX_WORKITEM_CNT 1000

// Executor类对应线程，只使用其中的函数
class Executor {
public:
    // Executor的运行状态(空闲/繁忙)
    enum Status {Idle, Busy};
    // 每个线程分配的主函数
    static int mainRunner();
    // Executor进入等待状态的处理函数
    static int startWaiting();
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
        // 建立给定类型的Group类型
        BuildGroup,
        // 对多个Mif元素执行条件赋值操作
        MifItemProcess
    };
    
    // 构造函数，参数是一个结构体
    ExecutorJob(JobTypes type, void* params);
    // 根据工作类型获取对应的执行函数
    std::function<int(void*)> getJobFunc(JobTypes);

public:
    // 加载目标Layer的函数
    int loadLayer(void* params);
    // 关闭和保存Layer的函数
    int saveLayer(void* params);
    // 对多行配置文件的内容进行语法解析的函数
    int parseConfigLine(void* params);
    // 为一个配置文件生成工作项的函数
    int parseConfigFile(void* params);
    // 建立给定类型的Group类型的函数
    int buildGroup(void* params);
    // 对多个Mif元素执行条件赋值操作的函数
    int mifItemProcess(void* params);

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
    const void* params_;
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

/*
    // 加载输入层和外挂数据
    int loadInput();
    // 打开或者创建输出层数据
    int openOutput();
    // 加载配置文件并进行处理
    int loadConfig();
*/
    // 执行所有操作
    int execute();
/*
    // 保存输出层数据
    int saveOutput();
*/

    // 构造函数，需要给出对应参数
    ExecutorPool(const Params& params);
    // 析构函数
    ~ExecutorPool();

public:
    // 对配置文件解析完毕后的语法信息
    ConfigGroup configGroup_;
    // 缓存构建过的Group数据
    GroupCache groupMap;

    // 输入层数据
    Layer inputLayer_;
    // 外挂数据
    std::vector<Layer> pluginLayers_;
    // 输出层数据
    std::vector<Layer> outputLayers_;
    // 输出层写入操作的互斥锁
    std::vector<std::mutex> outputLayersLock_;

    // 实际的候选队列的数量
    const int candidateQueueCnt_;
    // 准备就绪的工作项队列
    std::vector<ExecutorJob> readyQueue_;
    // 准备就绪队列的写入操作互斥锁
    std::mutex readyQueueLock_;
    // 候选工作项的队列
    std::vector<std::vector<ExecutorJob>> candidateQueue_;
    // 候选工作项队列的写入互斥锁
    std::vector<std::mutex> candidateQueueLock_;

    // Executor工作状态
    std::vector<Executor::Status> executorStatus;
    // Executor工作状态的互斥锁
    std::mutex executorStatusLock_;

private:
    // ExecutorPool的参数信息
    const Params params_;
    // 工作项和Executor管理函数
    int executorController();
};

} // namespace condition_assign

#endif // EXECUTOR_H
