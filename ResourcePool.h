#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H

#include "ExecutorPool.h"
#include "MifType.h"
#include "Config.h"

#include <string>
#include <mutex>
#include <functional>
#include <queue>
#include <cmath>

namespace condition_assign {

// 是否使用复杂依赖关系
#define USE_COMPLICATE_DEPENDENCY
// 准备好工作项的队列的最大长度
#define MAX_READY_QUEUE_SIZE 32
// 最大的MifLayer数目，防止爆内存
#define MAX_MIFLAYERS 32

class ConfigSubGroup;

class ResourcePool {
public:
    enum LayerType {Input, Output, Plugin};
    // 构造函数
    ResourcePool() = default;
    // 析构函数
    ~ResourcePool();

    // 初始化
    int init(ExecutorPool::Params params, Semaphore* newCandidateJob);
    // 根据目标层ID获取对应的ConfigSubGroup
    int getConfigSubGroup(int targetID, ConfigSubGroup** subGroupPtr);
    // 插入生成好的Group到映射中去
    int insertGroup(const int key, Group* newGroup);
    // 根据group的key获取
    int findGroup(const int key, Group** groupPtr);
    // 根据group的key获取并插入Group(与解析匹配的功能)
    int findInsertGroup(const int itemGroupKey, Group** itemGroupPtr,
            const int typeGroupKey = -1, Group** typeGroupPtr = nullptr);
    // 打开指定的Layer
    int openLayer(const std::string& layerPath, const LayerType layerType,
            const inputLayerID, const int layerID = -1);
    // 根据路径名或者部分路径名查找对应Layer的索引
    int getLayerByName(MifLayer** layerPtr, const LayerType layerType,
            const std::string& layerPath, int* layerID = nullptr);
    // 获取某一个Layer的指针
    int getLayerByIndex(MifLayer** layerPtr, const LayerType layerType,
            const int layerID = -1);
    // 获取外挂表的文件路径全名
    int getPluginFullPath(const std::string& layerName, std::string* fullPath);
    // 获取一个可以运行的工作项
    int getReadyJob(const int threadID, ExecutorJob** jobConsumerPtr);
    // 从备选工作队列中选择准备好的工作项放到准备队列
    int selectReadyJob(std::set<int>* wakeupExecutorID);

private:
    // 根据输入模型设置依赖关系和Layer属性
    int initRunningModel(const ExecutorPool::Params& params,
            std::set<std::string>* needCachelayers,
            std::vector<int>* layerDependency);
    // 初始化ConfigGroup
    int initConfigGroup(const ExecutorPool::Params& params,
            const std::map<std::string, ExecutorPool::LayerInfo>& layerList);

private:
    // 当前工作的目标层数目
    int targetCount_;
    // 当前的执行器数量
    int executorCount_;
    
    // 对配置文件解析完毕后的语法信息, 使用指针避免加锁的需要
    ConfigGroup configGroup_;
    
    // 所有Group数据缓存的锁
    std::mutex groupMapLock_;
    // 用于缓存Group查找的映射
    std::map<int, Group*> groupMap_;

    // 外挂表数据
    std::map<std::string, int> pluginLayersMap_;
    // 输入输出层数据路径到索引的映射
    std::map<std::string, int> portLayersMap_;
    // 所有MifLayer数据
    std::vector<MifLayer*> layers_;

public:
    // 当前预备队列中任务的数目
    std::atomic<int> readyJobCount_ {0};
    // 预备队列的写入操作互斥锁
    std::vector<std::mutex*> readyQueueLock_;
    // 准备就绪的工作项队列
    std::vector<std::deque<ExecutorJob*>> readyQueue_;

    // 是否有新的备选工作
    Semaphore* newCandidateJob_;
    // 候选工作项队列的写入互斥锁
    std::mutex candidateQueueLock_;
    // 候选工作项的队列
    std::queue<ExecutorJob*> candidateQueue_;

    // 用于缓存ProcessMifItem工作项以避免死锁
    std::vector<std::vector<ExecutorJob*>> jobCache_;
    // 当前ParseGroup工作项的数目
    std::atomic<int> parseGroupJobCount_ {0};
    // 信号量组，和依赖检查有直接关系
    std::vector<Semaphore*> dependencySignals_;
};

} // namespace condition_assign

#endif // RESOURCEPOOL_H
