#ifndef EXECUTORJOBFUNC_H
#define EXECUTORJOBFUNC_H

namespace condition_assign {

namespace job_func {

// 每个工作项最多负责解析
#define MAX_LINE_PER_JOB 100
// 每个工作项最多负责处理的Node分数最大和
#define MAX_SCORE_SUM_PER_JOB 1000
// 每个工作项最少的ConfigItem数量
#define MIN_CONFIGITEM_PER_JOB 50

// 加载目标Layer函数参数
struct LoadLayerParams {
    // 需要打开的layer类型
    ResourcePool::LayerType layerType;
    // 目标层的路径
    std::string* layerPath;
    // 目标层的ID
    int layerID;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 加载目标Layer的函数
int loadLayer(void* param);

// 关闭保存目标Layer函数参数
struct SaveLayerParams {
    // 目标层的路径
    std::string* layerPath;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 关闭和保存Layer的函数
int saveLayer(void* param);

// 配置文件语法解析函数参数
struct ParseConfigLinesParams {
    // 需要处理的多行Config信息的内容
    std::vector<std::string>* fullContent;
    // 分配的ConfigItem对应的起始索引
    const int startIndex;
    // 分配的ConfigItem对应的起始索引
    const int lineCount;
    // 目标层的ID
    int layerID;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 对多行配置文件的内容进行语法解析的函数
int parseConfigLines(void* param);

// 配置文件工作项生成函数参数
struct ParseConfigFileParams {
    // 配置文件的路径
    const std::string* filePath; 
    // 目标层的ID
    int layerID;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 为一个配置文件生成工作项的函数
int parseConfigFile(void* param);

// 建立数据组函数参数
struct ParseGroupParams {
    // 需要构建的目标组原始字符串和对应节点组指针
    std::pair<std::string, Group**>* groupInfo;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 建立给定类型的Group类型的函数
int parseGroup(void* param);

// 建立数据组函数参数
struct BuildGroupParams {
    // 处理的Layer
    MifLyaer* pluginLayer;
    // 需要构建的目标组指针
    Group* itemGroup;
    // 需要构建的目标组指针
    Group* typeGroup;
    // 处理MifItem索引的起始值
    const int startIndex;
    // 需要处理的MifItem个数
    const int itemCount;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 建立给定类型的Group类型的函数
int buildGroup(void* param);

// 对Mif进行条件赋值的参数
struct ProcessMifItemParams {
    // 需要处理的MifItem索引
    int mifItemIndex
    // 当前处理的输入Layer
    MifLayer* srcLayer;
    // 当前处理的目标Layer
    MifLayer* targetLayer;
    // 处理的Config所在组指针
    ConfigSubGroup* subGroup;
    // 匹配的ConfigItem在子组中起始索引
    const int startIndex;
    // 当前工作需要匹配的item数量
    const int itemCount;
};

// 对多个Mif元素执行条件赋值操作的函数
int processMifItem(void* param);

} // namespace job_func

} // namespace condition_assign

#endif // EXECUTORJOBFUNC_H
