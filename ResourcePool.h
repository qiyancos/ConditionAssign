#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H

#include "MifType.h"
#include "Config.h"

#include <string>
#include <mutex>
#include <function>

namespace condition_assign {

// 准备好工作项的队列的最大长s
#define MAX_READY_QUEUE_SIZE 2

#ifndef EXECUTORPOOL_H
class ExecutorJob;
#endif

class ResourcePool {
public:
    enum LayerType {Input, Output, Plugin};
    // 构造函数
    ResourcePool();
    // 初始化
    int init(const int targetCnt, const int pluginCnt,
            const int executorCnt, const int candidateQueueCnt,
            const JobSelectAlgo jobSelectAlgo);
    // 根据目标层ID获取对应的ConfigSubGroup
    int getConfigSubGroup(int targetID, ConfigSubGroup** subGroupPtr);
    // 生成新的Group
    int newGroup(Group** newGroupPtr, Group* newGroup);
    // 插入生成好的Group到映射中去
    int insertGroup(const std::string& key, Group* newGroup);
    // 根据group的key获取
    int findGroup(const std::string& key, Group** groupPtr);
    // 打开指定的Layer
    int openLayer(const std::string& layerPath, const LayerType layerType);
    // 根据路径名或者部分路径名查找对应Layer的索引
    int getLayerByName(MifLayer** layerPtr, const LayerType layerType,
            const std::string& layerPath, int* targetID = nullptr);
    // 获取某一个Layer的指针
    int getLayerByIndex(MifLayer** layerPtr, const LayerType layerType,
            const int targetID = -1);
    // 获取一个可以运行的工作项
    int getReadyJob(const int threadID, ExecutorJob** jobConsumerPtr);
    // 获取一个备选工作项空位，插入新的工作
    int getCandidateJob(const int candidateIndex, ExecutorJob** jobProducer);
    // 从备选工作队列中选择准备好的工作项放到准备队列
    int selectReadyJob();
    // 释放所有的资源
    int releaseResource();

private:
    // 当前工作的目标层数目
    int targetCnt_;
    // 当前工作的外挂表层数目
    int pluginCnt_;
    // 当前的执行器数量
    int executorCnt_;
    // 实际的候选队列的数量
    int candidateQueueCnt_;
    // 当前任务选择的算法类型
    JobSelectAlgo jobSelectAlgo_;
    
    // 对配置文件解析完毕后的语法信息, 使用指针避免加锁的需要
    std::vector<ConfigSubGroup*> configGroup_;
    
    // 所有Group数据缓存的锁
    std::mutex groupsLock_;
    // 缓存构建过的Group指针
    std::vector<Group*> groups_;
    // 缓存Group映射的锁
    std::mutex groupMapLock_;
    // 用于缓存Group查找的映射
    std::map<std::string, Group*> groupMap_;

    // 输入层数据
    MifLayer* inputLayer_;
    // 外挂表锁
    std::mutex pluginLayersLock_;
    // 外挂表数据
    std::map<std::string, MifLayer*> pluginLayersMap_;
    // 输出层数据路径到索引的映射锁
    std::mutex outputLayersLock_;
    // 输出层数据路径到索引的映射
    std::map<std::string, int> outputLayersMap_;
    // 输出层数据
    std::vector<MifLayer*> outputLayers_;

    // 准备就绪队列的写入操作互斥锁
    std::mutex readyQueueLock_;
    // 准备就绪的工作项队列
    std::vector<std::queue<ExecutorJob*>> readyQueue_;
    // 候选工作项队列的写入互斥锁
    std::mutex candidateQueueLock_;
    // 候选工作项的队列
    std::vector<std::queue<ExecutorJob*>> candidateQueue_;
};

} // namespace condition_assign

#endif // RESOURCEPOOL_H
