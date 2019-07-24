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

DataType getDataType(const std::string& data, std::string* stringVal,
        double* numberVal) {
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

int operatorListInit(const Operator* newOp) {
    operatorList.push_back(newOp);
    return 0;
}

int calculateScore(const std::vector<Node*> nodeVec) {
    int scoreSum = 0;
    for (Node* node : nodeVec) {
        CHECK_ARGS(node->op != nullptr, "Found node without operator.");
        if (node->value.groupPtr != nullptr) {
            score += (node->op->score() * node->value.groupPtr->size());
        } else {
            score += node->op->score();
        }
    }
    return score;
}

int satisfyConditions(const std::vector<Node*>& conditions, MifItem* item) {
    if (conditions.size() == 0) {
        return true;
    }
    Node* mainNode = conditions[conditions.size() - 1]; 
    CHECK_ARGS(mainNode->nodeType == Expr,
            "Found main node with type \"%s\" but not \"Expression\".",
            getTypeString(mainNode->nodeType).c_str());
    for (Node* node : conditions.size()) {
        CHECK_ARGS(node->op != nullptr, "Found node without operator.");
        CHECK_RET(node->op->process(node, item),
            "Operator process failed in \"%s%s%s\".",
            node->tagName.c_str(), node->op->str().c_str());
    }
    return mainNode->value.exprResult;
}

int applyAssigns(const std::vector<Node*>& assigns, MifItem* item) {
    for (Node* node : assigns.size()) {
        CHECK_ARGS(node->op != nullptr, "Found node without operator.");
        CHECK_RET(node->op->process(node, item),
            "Operator process failed in \"%s%s%s\".",
            node->tagName.c_str(), node->op->str().c_str());
    }
    return 0;
}

} // namespace syntax

} // namespace condition_assign
