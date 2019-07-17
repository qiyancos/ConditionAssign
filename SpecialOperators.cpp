#include "SpecialOperators.h"
#include "ConditionAssign.h"

namespace condition_assign {

namespace syntax {

int opCondFunc::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            std::string(std::string("Function \"") + funcName_ +
            "\" not defined").c_str());
    std::function<int(Node*, const MifItem&)>& executeFunc = 
            opInternalFuncList[funcName_];
    CHECK_RET(executeFunc(node, item),
            std::string("Failed to execute function [" + funcName_ + "]."));
    return 0;
}

int opCondFunc::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == content::npos || range->second == content::npos ||
            length < 0 || content.find("[") != content::npos) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    funcName_ = content.substr(range->first, length);
    return 0;
}

int opAssignFunc::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            std::string(std::string("Function \"") + funcName_ +
            "\" not defined").c_str());
    std::function<int(Node*, const MifItem&)>& executeFunc = 
            opInternalFuncList[funcName_];
    CHECK_RET(executeFunc(node, item),
            std::string("Failed to execute function [" + funcName_ + "]."));
    return 0;
}

int opAssignFunc::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == content::npos || range->second == content::npos ||
            length < 0 || content.find("[") != content::npos) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    funcName_ = content.substr(range->first, length);
    return 0;
}

int opReplace::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal),
            (std::string("Tag [") + node->tagName +
            "] not found!").c_str());
    CHECK_ARGS(startIdx + length < leftVal.size(),
            "Replace position out of range");
    leftVal.replace(startIdx, length, node->value.stringValue);
    item.assignWithTag(node->tagName, leftVal);
    return 0;
}

int opReplace::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    size_t leftBracketIndex = content.find("[");
    size_t colonIndex = content.find(":");
    size_t rightBracketIndex = content.find("]=");
    if (leftBracketIndex == content::npos || colonIndex == content::npos ||
            rightBracketIndex == content::npos) {
        return -1;
    }
    int startPosLength = colonIndex - leftBracketIndex - 1;
    int lengthLength = rightBracketIndex - colonIndex - 1;
    CHECK_ARGS(startPosLength > 0 && lengthLength > 0,
            "Can not find start position or end position.");
    range->first = leftBracketIndex;
    range->second = rightBracketIndex;
    std::string startIdxStr = content.substr(leftBracketIndex + 1,
            startPosLength);
    std::string lengthStr = content.substr(colonIndex + 1, lengthLength);
    CHECK_ARGS(isType<int>(startIdxStr) && isType<int>(lengthStr));
    startIdx = atoi(startIdxStr.c_str());
    length = atoi(lengthStr.c_str());
    return 0;
}

} // namespace syntax

} // namespace condition_assign
