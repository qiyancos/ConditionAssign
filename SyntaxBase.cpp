#include "SyntaxBase.h"
#include "ConditionAssign.h"

namespace condition_assign{

namespace syntax {

std::string getTypeString(const DataType type) {
    switch(type) {
    case Number: return "Number";
    case String: return "String";
    case Group: return "Group";
    case Expr: return "Expression";
    }
}

DataType getDataType(const std::string& data, std::string* stringVal = nullptr,
        double* numberVal == nullptr) {
    const int length = data.length();
    if (length == 0 || (data[0] == "\"" && data[length - 1] == "\"")) {
        if (stringVal != nullptr) {
            *stringVal = data.substr(1, length - 2);
        }
        return String;
    } else {
        if (stringVal != nullptr) {
            *stringVal = data;
        }
        if (!isType<double>(data, numberVal)) {
            return String
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
        return String;
    } else if (!(streamTemp >> charTemp)) {
        return String;
    }
    return Number;
}

bool isType<int>(const std::string& data, int* result);
bool isType<double>(const std::string& data, double* result);
bool isType<std::string>(const std::string& data, std::string* result);

int operatorListInit(const Operator* newOp, const bool front = false) {
    if (front) {
        operatorList.push_front(newOp);
    } else {
        operatorList.push_back(newOp);
    }
    return 0;
}

int calculateScore(const std::vector<Node*> nodeVec) {
    int scoreSum = 0;
    for (Node* node : nodeVec) {
        CHECK_ARGS(node->op != nullptr, "Found node without operator.");
        Group* groupPtr = node->value.groupPtr;
        if (groupPtr != nullptr) {
            if (groupPtr->isDynamic() && groupPtr->size() == 0)
                MifLayer* pluginLayer;
                CHECK_ARGS(pluginLayer = groupPtr->getLayer(),
                        "Can not get source layer of dynamic group.");
                score += (node->op->score() * pluginLayer.size());
            } else {
                score += (node->op->score() * groupPtr->size();
            }
        } else {
            score += node->op->score();
        }
    }
    return score;
}

int satisfyConditions(const ConfigItem& configItem, MifItem* item) {
    int result;
    Node* mainNode;
    CHECK_RET(configItem.getMainConditionNode(&mainNode),
            "Failed to get main node of conditions.");
    if (mainNode == nullptr) return 1;
    CHECK_ARGS(mainNode->op != nullptr, "Found main node without operator.");
    CHECK_RET(result = mainNode->op->process(mainNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->tagName.c_str(), node->op->str().c_str(),
            node->value.stringValue.c_str());
    return result;
}

int applyAssigns(const ConfigItem& configItem, MifItem* item) {
    int result;
    Node* mainNode;
    CHECK_RET(configItem.getMainAssignNode(&mainNode),
            "Failed to get main node of conditions.");
    CHECK_ARGS(mainNode->op != nullptr, "Found main node without operator.");
    CHECK_RET(mainNode->op->process(mainNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->tagName.c_str(), node->op->str().c_str(),
            node->value.stringValue.c_str());
    return 0;
}

} // namespace syntax

} // namespace condition_assign
