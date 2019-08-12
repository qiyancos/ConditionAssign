#ifndef EXECUTORJOBFUNC_H
#define EXECUTORJOBFUNC_H

#include "ResourcePool.h"
#include "MifType.h"
#include "SyntaxBase.h"

#include <string>
#include <vector>
#include <utility>

namespace condition_assign {

namespace job {

// 每个工作项最多负责解析的config行数
#define MAX_LINE_PER_JOB 128
// 每个工作项最多处理的item个数
#define MAX_ITEM_PER_JOB 256
// 每个工作项最多负责处理的Node分数最大和
#define MAX_SCORE_SUM_PER_JOB 1024

// 加载目标Layer的工作项
class LoadLayerJob : public ExecutorJob {
public:
    // 构造函数
    LoadLayerJob(const int sharedID, ResourcePool* resourcePool) :
            sharedID_(sharedID), resourcePool_(resourcePool) {}
    // 析构函数
    ~LoadLayerJob() = default;
    // 执行函数
    virtual int process(const int executorID);

private:
    // 目标层的ID
    const int sharedID_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 关闭保存目标Layer
class SaveLayerJob : public ExecutorJob {
public:
    // 构造函数
    SaveLayerJob(const int sharedID, const std::string& savePath,
            ResourcePool* resourcePool) : sharedID_(sharedID),
            savePath_(savePath), resourcePool_(resourcePool) {}
    // 析构函数
    ~SaveLayerJob() = default;
    // 关闭和保存Layer的函数
    virtual int process(const int executorID);

private:
    // 目标层的路径
    const int sharedID_;
    // 保存的目标路径
    const std::string& savePath_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 配置文件工作项生成函数参数
class ParseConfigFileJob : public ExecutorJob {
public:
    // 构造函数
    ParseConfigFileJob(const std::vector<int>& configIndexes,
            const std::string& filePath, ResourcePool* resourcePool) :
            configIndexes_(configIndexes),
            filePath_(filePath), resourcePool_(resourcePool) {}
    // 析构函数
    ~ParseConfigFileJob() = default;
    // 为一个配置文件生成工作项的函数
    virtual int process(const int executorID);

private:
    // 目标层的ID
    const std::vector<int> configIndexes_;
    // 配置文件的路径
    const std::string& filePath_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 配置文件语法解析函数参数
class ParseConfigLinesJob : public ExecutorJob {
public:
    using FullContent = std::vector<std::pair<std::string, int>>;
    // 构造函数
    ParseConfigLinesJob(const std::string& filePath, FullContent* fullContent,
            const int startIndex, const int lineCount,
            std::vector<ConfigSubGroup*>* subGroups,
            std::vector<MifLayer*>* srcLayers,
            std::vector<MifLayer*>* targetLayers,
            ResourcePool* resourcePool) : filePath_(filePath),
            fullContent_(fullContent), startIndex_(startIndex),
            lineCount_(lineCount), subGroups_(subGroups),
            srcLayers_(srcLayers), targetLayers_(targetLayers),
            resourcePool_(resourcePool) {}
    // 析构函数
    ~ParseConfigLinesJob() = default;
    // 对多行配置文件的内容进行语法解析的函数
    virtual int process(const int executorID);

private:
    // 配置文件的路径
    const std::string& filePath_;
    // 需要处理的多行Config信息的内容
    FullContent* fullContent_;
    // 分配的ConfigItem对应的起始索引
    const int startIndex_;
    // 分配的ConfigItem对应的起始索引
    const int lineCount_;
    // 处理所在的子config组
    std::vector<ConfigSubGroup*>* subGroups_;
    // 目标Layer的指针
    std::vector<MifLayer*>* srcLayers_;
    // 目标Layer的指针
    std::vector<MifLayer*>* targetLayers_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 建立数据组函数参数
class ParseGroupJob : public ExecutorJob {
public:
    // 构造函数
    ParseGroupJob(std::pair<std::string, Group**>* groupInfo,
            ResourcePool* resourcePool) : groupInfo_(groupInfo),
            resourcePool_(resourcePool) {}
    // 析构函数
    ~ParseGroupJob() = default;
    // 建立给定类型的Group类型
    virtual int process(const int executorID);

private:
    // 需要构建的目标组原始字符串和对应节点组指针
    std::pair<std::string, Group**>* groupInfo_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 建立数据组函数
class BuildGroupJob : public ExecutorJob {
public:
    // 构造函数
    BuildGroupJob(MifLayer* pluginLayer, Group* itemGroup, Group* typeGroup,
            const int startIndex, const int itemCount,
            ResourcePool* resourcePool) : pluginLayer_(pluginLayer),
            itemGroup_(itemGroup), typeGroup_(typeGroup),
            startIndex_(startIndex), itemCount_(itemCount),
            resourcePool_(resourcePool) {}
    // 析构函数
    ~BuildGroupJob() = default;
    // 建立给定类型的Group类型
    virtual int process(const int executorID);

private:
    // 处理的Layer
    MifLayer* pluginLayer_;
    // 需要构建的目标组指针
    Group* itemGroup_;
    // 需要构建的目标组指针
    Group* typeGroup_;
    // 处理MifItem索引的起始值
    const int startIndex_;
    // 需要处理的MifItem个数
    const int itemCount_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

// 对Mif进行条件赋值的参数
class ProcessMifItemsJob : public ExecutorJob {
public:
    // 构造函数
    ProcessMifItemsJob(MifLayer* srcLayer, MifLayer* targetLayer,
            ConfigSubGroup* subGroup, const int startIndex,
            const int itemCount, ResourcePool* resourcePool) :
            srcLayer_(srcLayer), targetLayer_(targetLayer),
            subGroup_(subGroup), startIndex_(startIndex),
            itemCount_(itemCount), resourcePool_(resourcePool) {}
    // 析构函数
    ~ProcessMifItemsJob() = default;
    // 对多个Mif元素执行条件赋值操作
    virtual int process(const int executorID);

private:
    // 当前处理的输入Layer
    MifLayer* srcLayer_;
    // 当前处理的目标Layer
    MifLayer* targetLayer_;
    // 处理的Config所在组指针
    ConfigSubGroup* subGroup_;
    // 匹配的MifItem在子组中起始索引
    const int startIndex_;
    // 当前工作需要匹配的item数量
    const int itemCount_;
    // 资源池指针
    ResourcePool* resourcePool_;
};

} // namespace job

} // namespace condition_assign

#endif // EXECUTORJOBFUNC_H
