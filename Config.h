#ifndef CONFIG_H_
#define CONFIG_H

#include "SyntaxBase.h"
#include "NormalOperators.h"
#include "SpecialOperator.h"
#include "ResourcePool.h"

#include <map>
#include <string>
#include <vector>
#include <stack>

namespace condition_assign {

// 单行ConditionAssign对应的结构
class ConfigItem {
public:
    ConfigItem();
    // 获取当前ConfigItem的分数
    int score() {return score_;}
    // 添加一个节点到节点库中
    int newNode(syntax::Node**);
    // 添加一个新的运算符到运算符库中
    int newOperator(syntax::Operator**);
    // 添加一个核心条件操作对应的节点指针
    int addCondition(syntax::Node*);
    // 添加一个核心赋值操作对应的节点指针
    int addAssign(syntax::Node*);
    // 根据当前的内容对制定的MifItem进行操作
    int matchMifItem(const Layer& srclayer, const int index, Layer& targetLayer);

public:
    // 核心条件操作对应的主节点
    std::vector<syntax::Node*> conditions_;
    // 核心赋值操作对应的主节点
    std::vector<syntax::Node*> assigns_;

private:
    // 当前ConfigItem的运算评分
    int score_;
    // 所有的节点实体库
    std::vector<syntax::Node*> nodes_;
    // 所有的运算符实体库
    std::vector<syntax::Operator*> operators_;
};

struct ConfigSubGroup {
    // 当前子组完成解析的ConfigItem数量锁
    std::mutex readyCntLock;
    // 当前子组完成解析的ConfigItem数量
    int readyCnt;
    // 多有ConfigItem的组
    std::vector<ConfigItem> group;
}

// 解析单行配置文件内容并生成对应的ConfigItem
int parseConfigLine(const std::string& line, ConfigSubGroup* subGroup,
        ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups);

// 解析当前组的基本信息
int parseGroupInfo(const std::string& content, ResourcePool* resourcePool,
        std::pair<int, Group*>* itemGroup, std::pair<int, Group*>* typeGroup);

} // namespace condition_assign

#endif // CONFIG_H
