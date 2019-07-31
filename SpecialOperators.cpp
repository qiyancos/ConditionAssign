#include "SpecialOperators.h"
#include "ConditionAssign.h"
#include "MifType.h"

namespace condition_assign {

namespace syntax {

std::map<std::string, std::function<int(Node*,
        MifItem*)>> opInternalFuncList;

OperatorCondFunc::OperatorCondFunc(const std::string& funcName) :
        Operator(), funcName_(funcName) {}

std::string OperatorCondFunc::str() {
    std::string symbol("<");
    symbol += funcName_;
    symbol += ">";
    return symbol;
}

int OperatorCondFunc::process(Node* node, MifItem* item) {
    int result;
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            "Function \"%s\" not defined", funcName_.c_str());
    std::function<int(Node*, MifItem*)>& executeFunc =
            opInternalFuncList[funcName_];
    CHECK_RET(result = executeFunc(node, item),
            "Failed to execute function [%d].", funcName_.c_str());
    return result;
}

int OperatorCondFunc::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "OperatorCondFunc";
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == std::string::npos ||
            range->second == std::string::npos ||
            length < 0 || content.find("[") != std::string::npos) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    *newOperatorPtr = new OperatorCondFunc(content.substr(range->first,
            length));
    return 0;
}

OperatorAssignFunc::OperatorAssignFunc(const std::string& funcName) :
        Operator(), funcName_(funcName) {}

std::string OperatorAssignFunc::str() {
    std::string symbol("<");
    symbol += funcName_;
    symbol += ">";
    return symbol;
}

int OperatorAssignFunc::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            "Function \"%s\" not defined.", funcName_.c_str());
    std::function<int(Node*, MifItem*)>& executeFunc =
            opInternalFuncList[funcName_];
    CHECK_RET(executeFunc(node, item),
            "Failed to execute function [%s].", funcName_.c_str());
    return 0;
}

int OperatorAssignFunc::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "OperatorAssignFunc";
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == std::string::npos ||
            range->second == std::string::npos ||
            length < 0 || content.find("[") != std::string::npos) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    *newOperatorPtr = new OperatorAssignFunc(content.substr(range->first,
            length));
    return 0;
}

OperatorReplace::OperatorReplace(const int startIndex, const int length) :
        Operator(), startIndex_(startIndex), length_(length) {}

std::string OperatorReplace::str() {
    std::stringstream symbol;
    symbol << "[" << startIndex_ << ":";
    symbol << length_ << "]=";
    return symbol.str();
}

int OperatorReplace::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Tag [%s] not found!", node->tagName.c_str());
    CHECK_ARGS(startIndex_ + length_ <= leftVal.size(),
            "Replace position[%d-%d] out of range.",
            startIndex_, startIndex_ + length_ - 1);
    leftVal.replace(startIndex_, length_, node->value.stringValue);
    CHECK_RET(item->assignWithTag(node->tagName, leftVal),
            "Failed to assign new tag value to tag \"%s\".",
            node->tagName.c_str());
    return 0;
}

int OperatorReplace::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "OperatorReplace";
    size_t leftBracketIndex = content.find("[");
    size_t colonIndex = content.find(":");
    size_t rightBracketIndex = content.find("]=");
    if (leftBracketIndex == std::string::npos ||
            colonIndex == std::string::npos ||
            rightBracketIndex == std::string::npos) {
        return -1;
    }
    int startPosLength = colonIndex - leftBracketIndex - 1;
    int lengthLength = rightBracketIndex - colonIndex - 1;
    CHECK_ARGS(startPosLength > 0 && lengthLength > 0,
            "Can not find start position or end position.");
    range->first = leftBracketIndex;
    range->second = rightBracketIndex + 2;
    std::string startIndexString = content.substr(leftBracketIndex + 1,
            startPosLength);
    std::string lengthStr = content.substr(colonIndex + 1, lengthLength);
    int startIndex, length;
    CHECK_ARGS(isType(startIndexString, &startIndex),
            "Start postion \"%s\" can not be converted to number.",
            startIndexString);
    CHECK_ARGS(isType(lengthStr, &length),
            "Length \"%s\" can not be converted to number.",
            startIndexString);\
    *newOperatorPtr = new OperatorReplace(startIndex, length);
    return 0;
}

} // namespace syntax

} // namespace condition_assign
