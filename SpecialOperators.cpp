#include "SpecialOperators.h"
#include "ConditionAssign.h"
#include "MifType.h"

namespace condition_assign {

namespace syntax {

int funcOperatorListInit(const std::string name, Operator* newFuncOp) {
    funcOperatorList[name] = newFuncOp;
    return 0;
}

int OperatorFunction::process(Node* node, MifItem* item) {
    CHECK_RET(-1, "Process function of operator Function should %s.",
            "never be called.");
    return 0;
}

int OperatorFunction::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    size_t leftBound = content.find("<");
    size_t rightBound = content.find(">");
    range->first = leftBound;
    range->second = rightBound;
    int length = rightBound - leftBound - 1;
    if (leftBound == std::string::npos || rightBound == std::string::npos ||
            length < 0) {
        return 0;
    }
    CHECK_ARGS(length, "No function name is given in this expression.");
    std::string funcName = content.substr(leftBound + 1,
            rightBound - leftBound - 1);
    auto funcOpIterator = funcOperatorList.find(funcName);
    CHECK_ARGS(funcOpIterator != funcOperatorList.end(),
            "Unknown function name \"%s\".", funcName.c_str());
    return funcOpIterator->second->find(newOperatorPtr, content, range,
            opName);
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
    return 1;
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
        return 0;
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
    CHECK_ARGS(isType(startIndexString, &startIndex_),
            "Start postion \"%s\" can not be converted to number.",
            startIndexString.c_str());
    CHECK_ARGS(isType(lengthStr, &length_),
            "Length \"%s\" can not be converted to number.",
            startIndexString.c_str());\
    *newOperatorPtr = new OperatorReplace(startIndex_, length_);
    return 1;
}

namespace func_op {

FuncOperatorInRange::FuncOperatorInRange(const int startNumber,
        const int endNumber) {
    int number = startNumber;
    while (number <= endNumber) {
        rangeOfNum_.insert(std::to_string(number++));
    }
}

int FuncOperatorInRange::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Tag [%s] not found!", node->tagName.c_str());
    return rangeOfNum_.find(leftVal) != rangeOfNum_.end();
}

int FuncOperatorInRange::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "FuncOperatorInRange";
    std::string arguments = content.substr(range->second + 1,
            content.size() - range->second - 1);
    size_t minusIndex = arguments.find("-");
    CHECK_ARGS(minusIndex != std::string::npos, "Can not find %s \"%s\".",
            "delimeter in function arguments", arguments.c_str());
    std::string startNumberStr = arguments.substr(0, minusIndex);
    std::string endNumberStr = arguments.substr(minusIndex + 1,
            arguments.size() - minusIndex - 1);
    int startNumber, endNumber;
    CHECK_ARGS(isType(startNumberStr, &startNumber),
            "Start number part \"%s\" in arguments is not a integer.",
            startNumberStr.c_str());
    CHECK_ARGS(isType(endNumberStr, &endNumber),
            "End number part \"%s\" in arguments is not a integer.",
            endNumberStr.c_str());
    *newOperatorPtr = new FuncOperatorInRange(startNumber, endNumber);
    return 1;
}

} // namespace func_op

} // namespace syntax

} // namespace condition_assign
