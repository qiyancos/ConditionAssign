#include "Config.h"
#include "ConditionAssign.h"

namespace condition_assign {

int ConfigItem::score() {
    if (score_ == -1)
        int tempScore = 0;
        CHECK_RET(tempScore = calculateScore(conditions_),
                "Failed to calculate score of conditions.");
        score_ += tempScore;
        CHECK_RET(tempScore = calculateScore(assigns_),
                "Failed to calculate score of conditions.");
        score_ += tempScore;
    }
    return score_;
}

int ConfigItem::addOperator(syntax::Operator* op) {
    operators_.push_back(op);
    return 0;
}

int ConfigItem::addCondition(syntax::Node* newCondition) {
    conditions_.push_back(newCondition);
    return 0;
}

int ConfigItem::addAssign(syntax::Node* newAssign) {
    assigns_.push_back(newAssign);
    return 0;
}

ConfigItem::~ConfigItem() {
    for (syntax::Node* node : conditions_) {
        delete node;
    }
    for (syntax::Node* node : assigns_) {
        delete node;
    }
    for (syntax::Operator* op : operators_) {
        delete op;
    }
}

namespace parser {

int findDelimeter(const std::string& content,
        std::vector<Delimeter>* delimeters, std::string* result) {
    size_t index = 0, newIndex = 0;
    const char* contentStr = content.c_str();
    int intoString = 0;
    while (index < content.size()) {
        if (intoString) {
            switch(contentStr[index]) {
            case '-':
                if (intoString = 3 && contentStr[index + 1] == '>') {
                    result += "->";
                    newIndex += 2;
                    index += 2;
                    intoString = 0;
                }
            case ')':
                if (intoString = 2 && contentStr[index + 1] == '$') {
                    result += ")$";
                    index += 2;
                    newIndex += 2;
                    intoString = 0;
                } else {
                    result.append(')', 1);
                    newIndex++;
                    index++;
                }
                break;
            case '\"':
                if (intoString = 1) {
                    intoString = 0;
                }
            default:
                result.append(contentStr[index++], 1);
                newIndex++;
                break;
            }
        } else {
            char temp;
            switch (contentStr[index]) {
            case ' ': index++; break;
            case ';':
                delimeters.push_back(Delimeter(newIndex++, Semicolon));
                result.append(';', 1);
                index++;
                break;
            case '&':
                delimeters.push_back(Delimeter(newIndex++, And));
                CHECK_ARGS(contentStr[index++] == '&',
                            "Operator \"&\" not supported.");
                index++;
                newIndex++;
                result.append('&', 2);
                break;
            case '|':
                delimeters.push_back(Delimeter(newIndex++, Or));
                CHECK_ARGS(contentStr[index++] == '|',
                        "Operator \"|\" not supported.");
                index++;
                newIndex++;
                result.append('|', 2);
                break;
            case '!':
                delimeters.push_back(Delimeter(newIndex++, Not));
                result.append('!', 1);
                index++;
                break;
            case '(':
                temp = newIndex == 0 ? -1 : result[newIndex - 1];
                if (temp == -1 || temp == '(' || temp == '|' ||
                        temp == '&' || temp == '!') {
                    intoString = 3;
                } else {
                    delimeters.push_back(Delimeter(newIndex++, LeftBracket));
                }
                result.append('(', 1);
                index++;
                break;
            case ')':
                delimeters.push_back(Delimeter(newIndex++, RightBracket));
                result.append(')', 1);
                index++;
                break;
            case '^':
                result.append('^', 1);
                CHECK_ARGS(contentStr[index] == '(',
                        "Operator \"^\" not supported.");
                newIndex++;
                intoString = 2;
            case '\"':
                intoString = 1;
            default:
                result.append(contentStr[index++], 1);
                newIndex++;
                break;
            }
        }
    }
    return 0;
}

int parseExpr(const std::string& content, syntax::Node* node,
        ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    for ()
}

int reduceExpr(std::vector<Node*>* nodeStack, const int reduceDepth) {
}

int linkExpr(const std::string& content, ConfigItem* configItem,
        const std::vector<Delimeter>& delimeters,
        std::vector<Expression>* exprs) {
    int reduceDepth = 0, index = 0;
    syntax::Node* newNode;
    std::vector<Node*> nodeStack;
    Delimeter* lastDelim = nullptr;
    for (Delimeter delim : delimeters) {
        // 表明中间存在需要解析的简单表达式
        if (delim.first > index) {
            CHECK_ARGS(delim.second != Not,
                    "Can not connect two conditions with \"!\".");
            CHECK_ARGS(delim.second != LeftBracket,
                    "Lack of logic operator between expressions..");
            newNode = new syntax::Node();
            newNode->reduceDepth = reduceDepth;
            configItem->conditions_.push_back(newNode);
            exprs->push_back(Expression(content.substr(index,
                    delim.second - index), newNode));
            nodeStack.push_back(newNode);
            CHECK_RET(reduceExpr(&nodeStack, reduceDepth),
                    "Failed to shift-reduce in depth[%d].", reduceDepth);
            if (delim.second == RightBracket) {
            } else {
                newNode = new syntax::Node();
                configItem->conditions_.push_back(newNode);
                newNode->leftType = syntax::Expr;
                newNode->rightType = syntax::Expr;
                newNode->leftNode = nodeStack.front();
                switch (delim.second) {
                case Or: newNode->op = new syntax::opOr(); break;
                case And:
                case Semicolon:
                    newNode->op = new syntax::opAnd(); break;
                }
                nodeStack.pop();
                nodeStack.push(newNode);
            }
        } else {
        }
        lastDelim = &delim;
        if (delim.second == Or || delim.second == And) {
            index = delim.first + 2;
        } else {
            index = delim.first + 1;
        }
    }
    return 0;
}

int parseConditions(const std::string& content, ConfigItem* configItem,
        ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::vector<Delimeter> delimeters;
    std::string newContent;
    CHECK_RET(findDelimeter(content, &delimeters, &newContent),
            "Failed to split with delimeters.");
    content = newContent;
    if (content.length() == 0) {
        configItem->conditions_.push_back(nullptr);
        return 0;
    }
    std::vector<Expression> exprs;
    if (delimeters.size() != 0) {
        CHECK_RET(linkExpr(content, configItem, delimeters, &exprs),
                "Failed split config line into expressions.");
    } else {
        configItem->conditions_.push_back(new syntax::Node());
        exprs.push_back(Expression(content, configItem->conditions_.back()));
    }
    for (auto expr : exprs) {
        CHECK_RET(parseExpr(expr.first, expr.second, resourcePool, newGroups),
                "Failed to parse expression \"%s\".", expr.first.c_str());
    }
    return 0;
}

int parseAssigns(const std::string& content, ConfigItem* configItem,
        ResourcePool* resourcePool){
}

int parseConfigLine(const std::string& line, ConfigSubGroup* subGroup,
        ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::vector<std::string> partitions = htk::split(line, "\t");
    CHECK_ARGS(partitions.size() >= 2,
            "Lack of assign expressions for a single line \"%s\".",
            line.c_str());
    ConfigItem* configItem = new ConfigItem();
    CHECK_RET(parseConditions(partitions[0], configItem, resourcePool,
            newGroups), "Failed to parse condition expressions \"%s\".",
            partitions[0].c_str());
    CHECK_RET(parseAssigns(partitions[1], configItem, resourcePool),
            "Failed to parse assign expressions \"%s\".",
            partitions[1].c_str());
    std::lock_guard<std::mutex> groupGuard(subGroup->groupLock);
    subGroup->group.push_back(configItem);
    return 0;
}

int parseGroupInfo(const std::string& content, ResourcePool* resourcePool,
        std::pair<int, Group*>* itemGroup, std::pair<int, Group*>* typeGroup) {
}

} // namesapce parser

} // namespace consition_assisgn
