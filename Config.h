#ifndef CONFIG_H
#define CONFIG_H

#include "SyntaxBase.h"
#include "Semaphore.h"

#include <atomic>
#include <mutex>
#include <map>
#include <string>
#include <vector>

namespace condition_assign {

class ResourcePool;
class MifLayer;

// 单行ConditionAssign对应的结构
class ConfigItem {
public:
    // 构造函数
    ConfigItem() = default;
    // 析构函数
    ~ConfigItem();

    // 获取当前ConfigItem的分数
    // int score();
    // 添加一个新的运算符到运算符库中
    int addOperator(syntax::Operator* newOperator,
            syntax::Operator** newOperatorPtr = nullptr);
    // 添加一个核心条件操作对应的节点指针
    int addCondition(syntax::Node* newNode,
            syntax::Node** newNodePtr = nullptr);
    // 添加一个核心赋值操作对应的节点指针
    int addAssign(syntax::Node* newNode, syntax::Node** newNodePtr = nullptr);

public:
    // 条件主节点
    syntax::Node* conditionMainNode_ = nullptr;
    // 赋值主节点
    syntax::Node* assignMainNode_ = nullptr;

private:
    // 当前ConfigItem的运算评分
    int score_ = -1;
    // 核心条件操作对应的主节点
    std::vector<syntax::Node*> conditions_;
    // 核心赋值操作对应的主节点
    std::vector<syntax::Node*> assigns_;
    // 所有的运算符实体库
    std::vector<syntax::Operator*> operators_;
};

class ConfigSubGroup {
public:
    // 当前子组完成解析的ConfigItem数量
    std::atomic<int> readyCount_ {0};
    // 当前子组完成处理的MifItem数量
    std::atomic<int> processedCount_ {0};
    // 多有ConfigItem的组
    std::vector<std::pair<int, ConfigItem*>>* group_;
    // 对应的对应的文件路径
    const std::string* filePath_;
    // 对应的源LayerID
    int srcLayerID_ = -1;
    // 对应的目标LayerID
    int targetLayerID_ = -1;
    
    // 当前的目标Layer的保存位置
    int savePoint_ = -1;
    // 对应的目标Layer存储路径
    std::string* savePath_;
    // 解析文件数
    std::atomic<int>* finishedFileCount_;
    // 当前子组的ID
    int id_;

public:
    // 构造函数
    ConfigSubGroup() = default;
    // 析构函数
    ~ConfigSubGroup() = default;
};

class ConfigGroup {
public:
    // 解析文件数
    std::atomic<int> finishedFileCount_ {0};
    // 需要解析的配置文件个数
    int totalCount_;
    // 配置文件对应的Vector
    std::vector<ConfigSubGroup*> group_;

public:
    // 初始化函数
    int init(const int totalCount, const std::vector<int>& subGroupMap,
            const std::vector<int>& savePoints, ResourcePool* resourcePool);
    // 构造函数
    ConfigGroup() = default;
    // 析构函数
    ~ConfigGroup();
};

namespace parser {

// 双目逻辑运算符类型
enum DelimType {Null, Not, And, Or, Semicolon, LeftBracket, RightBracket};
// 双目逻辑运算符类型和位置信息
using Delimiter = std::pair<int, DelimType>;
// 代解析的简单表达式形式
using Expression = std::pair<std::string, syntax::Node*>;

// 解析单行配置文件内容并生成对应的ConfigItem
int parseConfigLine(const std::string& line, ConfigSubGroup* subGroup,
        std::vector<MifLayer*>* srcLayers,
        std::vector<MifLayer*>* targetLayers,
        const int index, ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups);

// 解析当前组的基本信息
int parseGroupInfo(const std::string& content,
        const std::vector<MifLayer*>& srcLayers, ResourcePool* resourcePool,
        std::pair<int64_t, Group*>* itemGroup,
        std::pair<int64_t, Group*>* typeGroup);

} // namespace parser

} // namespace condition_assign

#endif // CONFIG_H
