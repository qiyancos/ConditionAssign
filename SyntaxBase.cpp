#include "SyntaxBase.h"
#include "ConditionAssign.h"
#include "Group.h"
#include "MifType.h"
#include "Config.h"

namespace condition_assign{

namespace syntax {

// https://blog.csdn.net/g1036583997/article/details/51910598
// 这里使用的是BKDRHash算法
int64_t keyGenerate(const std::string& str, int64_t oldHash) {
/*
    register int64_t hash = oldHash;
    int index = 0;
    while(index < str.length()) {
        hash = hash * 131 + str[index++];
    }
    return abs(hash);
*/
    register int64_t hash = oldHash;
    int index = 0;
    if (oldHash != 0) {
        while (index < str.length()) {
            hash = hash * 131 + str[index++];
        }
    } else {
        // 这里的逻辑是为了处理Group特殊设置的
        while (index < str.length() && str[index] != ' ') {
            hash += str[index++];
        }
        hash = (hash ^ (hash * 131)) + hash;
    }
    return abs(hash);
}

std::string getTypeString(const DataType type) {
    switch(type) {
    case Empty: return "Empty";
    case New: return "New";
    case Number: return "Number";
    case String: return "String";
    case GroupType: return "Group";
    case Expr: return "Expression";
    }
    return "Unknown Type";
}

DataType getDataType(const std::string data, std::string* stringVal,
        double* numberVal) {
    const int length = data.length();
    if (length == 0 || (data[0] == '\"' && data[length - 1] == '\"')) {
        if (stringVal != nullptr) {
            *stringVal = data.substr(1, length - 2);
        }
        return String;
    } else {
        if (stringVal != nullptr) {
            *stringVal = data;
        }
        if (!isType<double>(data, numberVal)) {
            return String;
        } else {
            return Number;
        }
    }
}

template<typename T>
bool isType(const std::string& data, T* result) {
    std::stringstream streamTemp(data);
    T typeTemp;
    char charTemp;
    T* typeTempPtr = result == nullptr ? &typeTemp : result;
    if (!(streamTemp >> *typeTempPtr)) {
        return false;
    } else if (streamTemp >> charTemp) {
        return false;
    }
    return true;
}

template bool isType<int>(const std::string&, int*);
template bool isType<double>(const std::string&, double*);
template bool isType<std::string>(const std::string&, std::string*);

int operatorListInit(Operator* newOp) {
    operatorList.push_back(newOp);
    return operatorList.size();
}

/*
// 用于计算计算强度的函数，已关闭
int calculateScore(const std::vector<Node*>& nodeVec) {
    int score = 0;
    for (Node* node : nodeVec) {
        CHECK_ARGS(node->op != nullptr, "Found node without operator.");
        Group* groupPtr = node->value.groupPtr;
        if (groupPtr != nullptr) {
            if (groupPtr->isDynamic() && groupPtr->size() == 0) {
                MifLayer* pluginLayer = groupPtr->getLayer();
                CHECK_ARGS(pluginLayer,
                        "Can not get source layer of dynamic group.");
                score += (node->op->score() * pluginLayer->size());
            } else {
                score += (node->op->score() * groupPtr->size());
            }
        } else {
            score += node->op->score();
        }
    }
    return score;
}
*/

} // namespace syntax

int satisfyConditions(const ConfigItem& configItem, MifItem* item) {
    int result;
    syntax::Node* mainNode = configItem.conditionMainNode_;
    if (!mainNode) {
        return true;
    }
    CHECK_ARGS(mainNode->op, "Found main node without operator.");
    CHECK_RET(result = mainNode->op->process(mainNode, item),
            "Operator process failed in \"%s %s %s\".",
            mainNode->tagName.c_str(), mainNode->op->str().c_str(),
            mainNode->value.stringValue.c_str());
    return result;
}

int applyAssigns(const ConfigItem& configItem, MifItem* item) {
    syntax::Node* mainNode = configItem.assignMainNode_;
    CHECK_ARGS(mainNode, "Failed to get main node of assign expressions.");
    CHECK_ARGS(mainNode->op != nullptr, "Found main node without operator.");
    CHECK_RET(mainNode->op->process(mainNode, item),
            "Operator process failed in \"%s %s %s\".",
            mainNode->tagName.c_str(), mainNode->op->str().c_str(),
            mainNode->value.stringValue.c_str());
    return 0;
}

} // namespace condition_assign
