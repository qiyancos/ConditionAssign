#ifndef EXECUTORJOBFUNC_H
#define EXECUTORJOBFUNC_H

namespace condition_assign {

namespace job_func {

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
struct ParseConfigLineParams {
    // 需要处理的多行Config信息的内容
    std::vector<std::string>* fullContent;
    // 分配的ConfigItem对应的起始索引
    const int startIndex;
    // 分配的ConfigItem对应的起始索引
    const int lineCount;
    // 当前解析的内容所属的subGroup
    ConfigSubGroup* subGroup;
    // 资源池指针
    ResourcePool* resourcePool;
};

// 对多行配置文件的内容进行语法解析的函数
int parseConfigLine(void* param);

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
struct BuildGroupParams {
    // 需要构建的目标组指针
    Group* group;
    // 资源池指针
    ResourcePool* resourcePool;
    // 处理MifItem索引的起始值
    const int startIndex;
    // 需要处理的MifItem个数
    const int itemCount;
};

// 建立给定类型的Group类型的函数
int buildGroup(void* param);

// 对Mif进行条件赋值的参数
struct ProcessMifItemParams {
    // 当前处理的输入Layer
    MifLayer* inputLayer;
    // 当前处理的目标Layer
    MifLayer* outputLayer;
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
