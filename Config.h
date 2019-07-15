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
    // 添加一个节点到节点库中
    int newNode(syntax::Node*);
    // 添加一个核心条件操作对应的节点指针
    int addCondition(syntax::Node*);
    // 添加一个核心赋值操作对应的节点指针
    int addAssign(syntax::Node*);
    // 根据当前的内容对制定的MifItem进行操作
    int matchMifItem(const Layer& srclayer, const int index, Layer& targetLayer);
private:
    // 当前ConfigItem的运算评分
    int score;
    // 所有的节点实体库
    std::vector<syntax::Node> nodes_;
    // 核心条件操作对应的主节点
    std::vector<syntax::Node*> conditions_;
    // 核心赋值操作对应的主节点
    std::vector<syntax::Node*> assigns_;
};

using ConfigSubGroup = std::stack<ConfigItem>;

// 解析单行配置文件内容并生成对应的ConfigItem
int parseConfigLine(const std::string& line, ConfigSubGroup& subGroup);

} // namespace condition_assign

#endif // CONFIG_H
