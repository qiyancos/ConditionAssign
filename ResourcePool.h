#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H

#include "MifType.h"
#include "Config.h"

#include <string>
#include <mutex>
#include <function>

namespace condition_assign {

#ifndef EXECUTORPOOL_H
class ExecutorJob;
#endif

class ResourcePool {
public:
    enum LayerType {Input, Output, Plugin};
    enum AccessType {Read, Write};
    // 构造函数
    ResourcePool();
    // 初始化
    init(const int targetCnt, const int candidateQueCnt);
    // 根据目标层ID获取对应的ConfigSubGroup
    int getConfigSubGroup(int targetID, ConfigSubGroup** subGroupPtr);
    // 生成新的Group
    int newGroup(Group** newGroupPtr);
    // 插入生成好的Group到映射中去
    int insertGroup(const std::string& key, const Group* newGroupPtr);
    // 根据group的key获取
    int findGroup(const std::string& key, Group** groupPtr);
    // 打开指定的Layer
    int openLayer(const std::string& layerPath, const LayerType layerType);
    // 根据路径名或者部分路径名查找对应Layer的索引
    int findLayer(const std:;string& layerPath, int* targetID,
            const LayerType layerType)
    // 获取输入层数据
    int getLayer(MifLayer** layerPtr, const LayerType layerType,
            const AccessType accessType);
    // 获取输出或者外挂层数据
    int getLayer(MifLayer** layerPtr, const int targetID,
            const LayerType layerType, const AccessType accessType);
    // 获取一个可以运行的工作项
    int getReadyJob(ExecutorJob* jobConsumer);
    // 获取一个备选工作项空位，插入新的工作
    int getCandidateJob(const int candidateIndex, ExecutorJob** jobProducer);
    // 从备选工作队列中选择准备好的工作项放到准备队列
    int selectReadyJob();
private:
    // 当前工作的目标层数目
    const int targetCnt_;
    // 对配置文件解析完毕后的语法信息
    std::vector<ConfigSubGroup> configGroup_;
    // 缓存构建过的Group数据
    std::vector<Group> groups_;
    // 用于缓存Group查找的映射
    std::map<std::string, Group*> groupMap_;

    // 输入层数据
    MifLayer inputLayer_;
    // 外挂数据路径到索引的映射
    std::map<std::string, int> pluginLayersMapping_;
    // 外挂数据
    std::vector<MifLayer> pluginLayers_;
    // 输出层数据路径到索引的映射
    std::map<std::string, int> outputLayersMapping_;
    // 输出层数据
    std::vector<MifLayer> outputLayers_;
    // 输出层写入操作的互斥锁
    std::vector<std::mutex> outputLayersLock_;

    // 实际的候选队列的数量
    const int candidateQueueCnt_;
    // 准备就绪的工作项队列
    std::queue<ExecutorJob> readyQueue_;
    // 准备就绪队列的写入操作互斥锁
    std::mutex readyQueueLock_;
    // 候选工作项的队列
    std::vector<std::queue<ExecutorJob>> candidateQueue_;
    // 候选工作项队列的写入互斥锁
    std::vector<std::mutex> candidateQueueLock_;
};

} // namespace condition_assign

#endif // RESOURCEPOOL_H
