#include "Config.h"
#include "MifType.h"
#include "ConditionAssign.h"
#include "NormalOperators.h"
#include "SpecialOperators.h"
#include "ResourcePool.h"
#include "Group.h"

namespace condition_assign {

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

int ConfigItem::score() {
    if (score_ == -1) {
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

int ConfigItem::addOperator(syntax::Operator* newOperator,
        syntax::Operator** newOperatorPtr = nullptr) {
    operators_.push_back(newOperator);
    if (newOperatorPtr != nullptr) {
        *newOperatorPtr = newOperator;
    }
    return 0;
}

int ConfigItem::addCondition(syntax::Node* newNode,
        syntax::Node** newNodePtr = nullptr) {
    conditions_.push_back(newNode);
    if (newNodePtr != nullptr) {
        *newNodePtr = newNode;
    }
    return 0;
}

int ConfigItem::addAssign(syntax::Node* newNode,
        syntax::Node** newNodePtr = nullptr) {
    assigns_.push_back(newNode);
    if (newNodePtr != nullptr) {
        *newNodePtr = newNode;
    }
    return 0;
}

ConfigSubGroup::~ConfigSubGroup() {
    for (auto itemPair : group_) {
        delete itemPair.second;
    }
}

namespace parser {

int findDelimeter(const syntax::Operator::OperatorType opType,
        const std::string& content, std::vector<Delimeter>* delimeters,
        std::string* result) {
    size_t index = 0, newIndex = 0;
    const char* contentStr = content.c_str();
    int intoString = 0;
    // intoString 0-无 1-普通双引号字符串中 3-Group内容
    while (index < content.size()) {
        if (intoString) {
            switch(contentStr[index]) {
            case '-':
                if (intoString == 3 && contentStr[index + 1] == '>') {
                    *result += "->";
                    newIndex += 2;
                    index += 2;
                    intoString = 0;
                } else {
                    result->append(1, contentStr[index++]);
                    newIndex++;
                }
                break;
            case '\"':
                if (intoString = 1) {
                    intoString = 0;
                }
            default:
                result->append(1, contentStr[index++]);
                newIndex++;
                break;
            }
        } else {
            char temp;
            switch (contentStr[index]) {
            case ' ': index++; break;
            case ';':
                delimeters->push_back(Delimeter(newIndex++, Semicolon));
                result->append(1, ';');
                index++;
                break;
            case '&':
                temp = contentStr[index++];
                CHECK_ARGS(temp == '&' || temp == '<' || temp == '[',
                            "Operator \"&\" not supported.");
                delimeters->push_back(Delimeter(newIndex++, And));
                result->append(1, '&');
                if (temp == '&') {
                    index++;
                    newIndex++;
                    result->append(1, '&');
                }
                break;
            case '|':
                CHECK_ARGS(contentStr[index++] == '|',
                        "Operator \"|\" not supported.");
                CHECK_ARGS(opType == syntax::Operator::Condition,
                        "Operator \"||\" not supported in %s",
                        "assign expressions.");
                delimeters->push_back(Delimeter(newIndex++, Or));
                index++;
                newIndex++;
                result->append(2, '|');
                break;
            case '!':
                if (contentStr[++index] != '=') {
                    CHECK_ARGS(opType == syntax::Operator::Condition,
                            "Operator \"!\" not supported in %s",
                            "assign expressions.");
                    delimeters->push_back(Delimeter(newIndex, Not));
                }
                newIndex++;
                result->append(1, '!');
                break;
            case '(':
                temp = newIndex == 0 ? -1 : (*result)[newIndex - 1];
                if (temp == -1 || temp == '(' || temp == '|' ||
                        temp == '&' || temp == '!') {
                    delimeters->push_back(Delimeter(newIndex++, LeftBracket));
                } else {
                    intoString = 3;
                }
                result->append(1, '(');
                index++;
                break;
            case ')':
                delimeters->push_back(Delimeter(newIndex++, RightBracket));
                result->append(1, ')');
                index++;
                break;
            case '\"':
                intoString = 1;
            default:
                result->append(1, contentStr[index++]);
                newIndex++;
                break;
            }
        }
    }
    return 0;
}

int parseExpr(const syntax::Operator::OperatorType opType,
        const std::string& content, syntax::Node* node, ConfigItem* configItem,
        ResourcePool* resourcePool, MifLayer* srcLayer,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::pair<size_t, size_t> range;
    std::string opName;
    syntax::Operator* newOperator;
    for (syntax::Operator* op : syntax::operatorList) {
        if (op->find(&newOperator, content, &range, &opName) > -1) {
#ifdef DEBUG_OP
            std::cout << opName << ": Operator matched." << std::endl;
#endif
            CHECK_ARGS(op->type() == opType,
                    "[%s] Found operator type[Condition/Assign] not matched.",
                    opName.c_str());
            CHECK_ARGS(range.first > 0,
                    "[%s] No left value provided in expression \"%s\".",
                    opName.c_str(), content.c_str());
            node->tagName = content.substr(0, range.first);
            CHECK_RET(srcLayer->getTagType(node->tagName,
                    &(node->leftType)), "Failed to get data type of tag \"%s\".",
                    node->tagName.c_str());
            configItem->addOperator(newOperator, &(node->op));
            if (op->isSupported(syntax::GroupType)) {
                CHECK_ARGS(node->value.stringValue.find(")->") !=
                        std::string::npos, "Bad group format.");
                node->rightType = syntax::GroupType;
                newGroups->push_back(new std::pair<std::string, Group**>(
                        node->value.stringValue, &(node->value.groupPtr)));
            } else if (range.second >= content.size()) {
                node->rightType = syntax::Number;
                node->value.stringValue = "";
                node->value.numberValue = 0.0f;
            } else {
                node->value.stringValue = content.substr(range.second,
                        content.size() - range.second);
                node->rightType = syntax::getDataType(node->value.stringValue,
                        &(node->value.stringValue),
                        &(node->value.numberValue));
            }
            return 0;
        }
    }
    CHECK_ARGS(false, "Failed to find any operators in expression \"%s\".",
            content.c_str());
}

int reduceExpr(std::vector<syntax::Node*>* nodeStack, const int reduceDepth) {
    int lastNodeIndex = nodeStack->size() - 1;
    CHECK_ARGS(nodeStack->back()->rightNode != nullptr ||
            (nodeStack->back()->rightNode == nullptr &&
            nodeStack->back()->leftNode == nullptr),
            "Front node in node stack format not good.");
    while (lastNodeIndex > 0 &&
            (*nodeStack)[lastNodeIndex - 1]->reduceDepth >= reduceDepth) {
        CHECK_ARGS(nodeStack->back()->reduceDepth >= reduceDepth,
                "Expression nodes' reduce-depth out of order.");
        if ((*nodeStack)[lastNodeIndex - 1]->reduceDepth >= reduceDepth) {
            CHECK_ARGS((*nodeStack)[lastNodeIndex - 1]->rightNode == nullptr,
                    "Node stack end with two completed node.");
            (*nodeStack)[--lastNodeIndex]->rightNode = nodeStack->back();
            nodeStack->pop_back();
        }
    }
    return 0;
}

int linkExpr(const syntax::Operator::OperatorType opType,
        const std::string& content, ConfigItem* configItem,
        const std::vector<Delimeter>& delimeters,
        std::vector<Expression>* exprs) {
    int reduceDepth = 0, index = 0;
    syntax::Node* newNode;
    std::vector<syntax::Node*> nodeStack;
    Delimeter* lastDelim = nullptr;
    for (int i = 0; i < delimeters.size(); i++) {
        const Delimeter& delim = delimeters[i];
        // 表明中间存在需要解析的简单表达式
#ifdef DEBUG_OP
        std::cout << "Left: " << content.substr(index,
                content.length() - index);
        if (lastDelim) {
            std::cout << " Last Delim: " << lastDelim->second;
        }
        std::cout << std::endl;
#endif
        if (delim.first > index) {
            CHECK_ARGS(delim.second != Not,
                    "Can not connect two conditions with \"!\".");
            CHECK_ARGS(delim.second != LeftBracket,
                    "Lack of logic operator between expressions..");
            if (lastDelim) {
                CHECK_ARGS(lastDelim->second != RightBracket,
                        "Lack of logic operator before expression.");
            }
            configItem->addCondition(new syntax::Node(), &newNode);
            newNode->reduceDepth = reduceDepth;
            exprs->push_back(Expression(content.substr(index,
                    delim.first - index), newNode));
            nodeStack.push_back(newNode);
            CHECK_RET(reduceExpr(&nodeStack, reduceDepth),
                    "Failed to shift-reduce in depth[%d].", reduceDepth);
            if (delim.second == RightBracket) {
                CHECK_ARGS(--reduceDepth > -1,
                        "Lack of \"(\" matches with \")\".");
                CHECK_RET(reduceExpr(&nodeStack, reduceDepth),
                        "Failed to shift-reduce in depth[%d].", reduceDepth);
            } else {
                configItem->addCondition(new syntax::Node(), &newNode);
                newNode->reduceDepth = reduceDepth;
                newNode->leftType = syntax::Expr;
                newNode->rightType = syntax::Expr;
                newNode->leftNode = nodeStack.back();
                switch (delim.second) {
                case Or: newNode->op = new syntax::OperatorOr(); break;
                case And:
                case Semicolon:
                    newNode->op = new syntax::OperatorAnd(); break;
                }
                configItem->addOperator(newNode->op);
                nodeStack.pop_back();
                nodeStack.push_back(newNode);
            }
        // 连续的间隔符
        } else {
            switch (delim.second) {
            case RightBracket:
                if (lastDelim) {
                    CHECK_ARGS(lastDelim->second == LeftBracket ||
                            lastDelim->second == RightBracket,
                            "Lack of expression after logic operator.");
                }
                reduceDepth--;
                break;
            case LeftBracket:
                if (lastDelim) {
                    CHECK_ARGS(lastDelim->second != RightBracket,
                            "Lack of logic operator before expression.");
                }
                reduceDepth++;
                break;
            case Not:
                if (lastDelim) {
                    CHECK_ARGS(lastDelim->second != RightBracket,
                            "Lack of logic operator before expression.");
                }
                configItem->addCondition(new syntax::Node(), &newNode);
                nodeStack.push_back(newNode);
                newNode->reduceDepth = reduceDepth;
                newNode->rightType = syntax::Expr;
                configItem->addOperator(new syntax::OperatorNot(),
                        &(newNode->op));
                break;
            default:
                if (lastDelim) {
                    CHECK_ARGS(lastDelim->second == RightBracket,
                            "Invalid operator or delimeter before %s",
                            "binary logic operator.");
                }
                configItem->addCondition(new syntax::Node(), &newNode);
                newNode->leftType = syntax::Expr;
                newNode->rightType = syntax::Expr;
                newNode->leftNode = nodeStack.back();
                newNode->reduceDepth = reduceDepth;
                switch (delim.second) {
                case Or: newNode->op = new syntax::OperatorOr(); break;
                case And:
                case Semicolon: newNode->op = new syntax::OperatorAnd(); break;
                }
                configItem->addOperator(newNode->op);
                nodeStack.pop_back();
                nodeStack.push_back(newNode);
            }
        }
        lastDelim = &delim;
        if (delim.second == Or || delim.second == And) {
            index = delim.first + 2;
        } else {
            index = delim.first + 1;
        }
    }
    if (index < content.size() - 1) {
#ifdef DEBUG_OP
        std::cout << "Left: " << content.substr(index,
                content.length() - 1);
        if (lastDelim) {
            std::cout << " Last Delim: " << lastDelim->second;
        }
        std::cout << std::endl;
#endif
        configItem->addCondition(new syntax::Node(), &newNode);
        newNode->reduceDepth = reduceDepth;
        exprs->push_back(Expression(content.substr(index,
                 content.size() - 1), newNode));
        nodeStack.push_back(newNode);
        CHECK_RET(reduceExpr(&nodeStack, reduceDepth),
                "Failed to shift-reduce in depth[%d].", reduceDepth);
    }
    CHECK_ARGS(nodeStack.size() == 1, "No node or too many nodes left for %s",
            "main node setting in nodestack");
    if (opType == syntax::Operator::Condition) {
        configItem->conditionMainNode_ = nodeStack.front();
    } else {
        configItem->assignMainNode_ = nodeStack.front();
    }
    CHECK_ARGS(reduceDepth == 0, "Lack left or right bracket in \"%s\"",
            content.c_str());
    return 0;
}

int parseConditions(const std::string& content, ConfigItem* configItem,
        ResourcePool* resourcePool, MifLayer* srcLayer,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::vector<Delimeter> delimeters;
    std::string newContent("");
    CHECK_RET(findDelimeter(syntax::Operator::Condition, content,
            &delimeters, &newContent),
            "Failed to split content with delimeters.");
    if (newContent.length() == 0) {
        configItem->addCondition(nullptr);
        return 0;
    }
    std::vector<Expression> exprs;
    if (delimeters.size() != 0) {
        CHECK_RET(linkExpr(syntax::Operator::Condition, newContent,
                configItem, delimeters, &exprs),
                "Failed split config line into expressions.");
    } else {
        syntax::Node* newNode;
        configItem->addCondition(new syntax::Node(), &newNode);
        exprs.push_back(Expression(newContent, newNode));
        configItem->conditionMainNode_ = newNode;
    }
    for (auto expr : exprs) {
        CHECK_RET(parseExpr(syntax::Operator::Condition, expr.first,
                expr.second, configItem, resourcePool, srcLayer, newGroups),
                "Failed to parse expression \"%s\".", expr.first.c_str());
    }
    return 0;
}

int parseAssigns(const std::string& content, ConfigItem* configItem,
        ResourcePool* resourcePool, MifLayer* srcLayer) {
    std::vector<Delimeter> delimeters;
    std::string newContent("");
    CHECK_RET(findDelimeter(syntax::Operator::Assign, content, &delimeters,
            &newContent), "Failed to split content with delimeters.");
    std::vector<Expression> exprs;
    if (delimeters.size() != 0) {
        CHECK_RET(linkExpr(syntax::Operator::Assign, newContent, configItem,
                delimeters, &exprs),
                "Failed split config line into expressions.");
    } else {
        syntax::Node* newNode;
        configItem->addAssign(new syntax::Node(), &newNode);
        exprs.push_back(Expression(newContent, newNode));
        configItem->assignMainNode_ = newNode;
    }
    for (auto expr : exprs) {
        std::vector<std::pair<std::string, Group**>*> newGroups;
        CHECK_RET(parseExpr(syntax::Operator::Assign, expr.first, expr.second,
                configItem, resourcePool, srcLayer, &newGroups),
                "Failed to parse expression \"%s\".", expr.first.c_str());
        CHECK_ARGS(newGroups.empty(),
                "Do not support group operation in assign expressions.");
    }
    return 0;

}

int parseConfigLine(const std::string& line, ConfigSubGroup* subGroup,
        const int index, ResourcePool* resourcePool, const int layerID,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::vector<std::string> partitions = htk::split(line, "\t");
    CHECK_ARGS(partitions.size() >= 2,
            "Lack of assign expressions for a single line \"%s\".",
            line.c_str());
    if (partitions[1].length() == 0) {
        CHECK_WARN(false,
            "Config line without assign expressions will never work.");
        return 0;
    }
    ConfigItem* configItem = new ConfigItem();
    MifLayer* srcLayer;
    CHECK_RET(resourcePool->getLayerByIndex(&srcLayer, ResourcePool::Input),
            "Failed to get input layer.");
    CHECK_RET(parseConditions(partitions[0], configItem, resourcePool,
            srcLayer, newGroups),
            "Failed to parse condition expressions \"%s\".",
            partitions[0].c_str());
    MifLayer* targetLayer;
    CHECK_RET(resourcePool->getLayerByIndex(&targetLayer, ResourcePool::Output,
            layerID), "Failed to get output layer[%d].", layerID);
    if (targetLayer->isNew()) {
        targetLayer = srcLayer;
    }
    CHECK_RET(parseAssigns(partitions[1], configItem, resourcePool,
            targetLayer), "Failed to parse assign expressions \"%s\".",
            partitions[1].c_str());
    subGroup->group_[index].second = configItem;
    return 0;
}

int groupKeyGenerate(const std::string& str) {
    int sum = 0;
    for (int i = 0; i < str.length(); i++) {
        sum += (str[i] == ' ' ? 0 : str[i]);
    }
    return sum ^ (str.length() * 1234) + sum;
}

int parseGroupArgs(const std::string& content, std::string* layerName,
        std::string* conditions, std::string* oldTagName,
        std::string* newTagName, std::string* tagName) {
    size_t startIndex, endIndex;
    // 获取layerName参数
    startIndex = content.find("(");
    endIndex = content.find(",");
    CHECK_ARGS(endIndex > startIndex, "Can not locate left bracket of group.");
    *layerName = htk::trim(content.substr(startIndex,
            endIndex - startIndex), " ");
    CHECK_ARGS(layerName->length() > 0, "No layer name provided for group.");
    // 获取条件部分
    startIndex = endIndex + 1;
    endIndex = content.find(",", startIndex);
    if (endIndex = std::string::npos) {
        endIndex = content.find("->");
        CHECK_ARGS(endIndex != std::string::npos,
                "Can not find \"->\" for group.");
        endIndex = content.find_last_of(")", endIndex);
        CHECK_ARGS(endIndex != std::string::npos && endIndex > startIndex,
                "Can not locate right bracket of group.");
        *conditions = htk::trim(content.substr(startIndex,
                endIndex - startIndex), " ");
    } else {
        *conditions = htk::trim(content.substr(startIndex,
                endIndex - startIndex), " ");
        // 获取重映射部分参数
        startIndex = endIndex + 1;
        endIndex = content.find(",", startIndex);
        if (endIndex != std::string::npos) {
            *oldTagName = htk::trim(content.substr(startIndex,
                    endIndex - startIndex), " ");
            startIndex = endIndex + 1;
            endIndex = content.find(")", startIndex);
            CHECK_ARGS(endIndex != std::string::npos,
                "Can not locate right bracket of group.");
            *newTagName = htk::trim(content.substr(startIndex,
                    endIndex - startIndex), " ");
        } else {
            startIndex = endIndex + 1;
            endIndex = content.find(")", startIndex);
            *oldTagName = htk::trim(content.substr(startIndex,
                    endIndex - startIndex), " ");
            CHECK_ARGS(oldTagName->length() == 0, "New tag name must be %s",
                    "provided together with old tag name in group.");
        }
    }
    // 获取tagName
    startIndex = content.find("->") + 2;
    endIndex = content.size();
    *tagName = htk::trim(content.substr(startIndex,
            endIndex - startIndex), " ");
    CHECK_ARGS(tagName->length() > 0, "No redirect target name is provided.");
    // 分析参数情况
    CHECK_ARGS((oldTagName->length() && newTagName->length()) ||
            (!oldTagName->length() && !newTagName->length()),
            "New tag name and old tag name must be %s",
            "both provdied or neither provided");
    return 0;
}

int parseGroupInfo(const std::string& content, ResourcePool* resourcePool,
        std::pair<int, Group*>* itemGroup, std::pair<int, Group*>* typeGroup) {
    std::string layerName, conditions, oldTagName, newTagName, tagName;
    CHECK_RET(parseGroupArgs(content, &layerName, &conditions, &oldTagName,
            &newTagName, &tagName), "Failed to parse group arguments.");
    CHECK_RET(resourcePool->getPluginFullPath(layerName, &layerName),
            "Can not find plugin layer \"%s\".", layerName.c_str());
    int staticItemGroupKey = groupKeyGenerate(layerName + conditions);
    int typeGroupKey = groupKeyGenerate(layerName + conditions + tagName);
    // 生成Group信息
    if (oldTagName.length()) {
        // 静态Group的处理
        itemGroup->second = new ItemGroup();
        if (tagName == "POINT") {
            typeGroup->second = new PointGroup();
        } else if (tagName == "LINE") {
            typeGroup->second = new LineGroup();
        } else if (tagName == "AREA") {
            typeGroup->second = new AreaGroup();
        } else {
            typeGroup->second = new TagGroup();
        }
        int matchCnt = resourcePool->findInsertGroup(staticItemGroupKey,
                &(itemGroup->second), typeGroupKey, &(typeGroup->second));
        switch (matchCnt) {
        case 0:
            itemGroup->second->info_ = new Group::GroupInfo();
            itemGroup->second->info_->layerName_ = layerName;
            // 获取外挂表指针并解析条件语句
            MifLayer* pluginLayer;
            CHECK_RET(resourcePool->getLayerByName(&pluginLayer,
                    ResourcePool::Plugin, layerName),
                    "Failed to find layer named as \"%s\".",
                    layerName.c_str());
            itemGroup->second->setLayer(pluginLayer);
            std::vector<std::pair<std::string, Group**>*> newGroup;
            CHECK_RET(parseConditions(conditions,
                    itemGroup->second->info_->configItem_,
                    resourcePool, pluginLayer, &newGroup),
                    "Failed to parse conditions in group.");
            CHECK_ARGS(newGroup.empty(), "New-Condition-Assign %s",
                    "do not support group nested structure.");
            itemGroup->second->parseDone_.signalAll();
            itemGroup->first = staticItemGroupKey;
        case 1:
            typeGroup->first = typeGroupKey;
        case 2: return 0;
        default:
            CHECK_RET(-1, "Failed to find and insert new groups.");
        }
    } else {
        // 动态Group的处理
        itemGroup->second = new ItemGroup(true);
        int dynamicItemGroupKey = groupKeyGenerate(layerName + conditions +
                    oldTagName + newTagName);
        if (resourcePool->findInsertGroup(dynamicItemGroupKey,
                &(itemGroup->second)) > -1) {
            return 0;
        }
        Group* oldStaticGroup;
        if (resourcePool->findGroup(staticItemGroupKey,
                &oldStaticGroup) > -1) {
            oldStaticGroup->parseDone_.wait();
            itemGroup->second->info_ = oldStaticGroup->info_->copy();
            itemGroup->second->setLayer(oldStaticGroup->getLayer());
            itemGroup->second->info_->checkedCnt_ = 0;
        } else {
            itemGroup->second->info_ = new Group::GroupInfo();
            itemGroup->second->info_->layerName_ = layerName;
            // 获取外挂表指针并解析条件语句
            MifLayer* pluginLayer;
            CHECK_RET(resourcePool->getLayerByName(&pluginLayer,
                    ResourcePool::Plugin, layerName),
                    "Failed to find layer named as \"%s\".",
                    layerName.c_str());
            itemGroup->second->setLayer(pluginLayer);
            std::vector<std::pair<std::string, Group**>*> newGroup;
            CHECK_RET(parseConditions(conditions,
                    itemGroup->second->info_->configItem_,
                    resourcePool, pluginLayer, &newGroup),
                    "Failed to parse conditions in group.");
            CHECK_ARGS(newGroup.empty(), "New-Condition-Assign %s",
                    "do not support group nested structure.");
            itemGroup->second->parseDone_.signalAll();
        }
        itemGroup->first = dynamicItemGroupKey;
        itemGroup->second->info_->oldTagName_ = oldTagName;
        itemGroup->second->info_->newTagName_ = newTagName;
        itemGroup->second->info_->tagName_ = tagName;
    }
}

} // namesapce parser

} // namespace consition_assisgn
