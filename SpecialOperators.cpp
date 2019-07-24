#include "SpecialOperators.h"
#include "ConditionAssign.h"

namespace condition_assign {

namespace syntax {

std::string opCondFunc::str() {
    std::string symbol("<");
    symbol += funcName_;
    symbol += ">";
    return symbol;
}

int opCondFunc::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            "Function \"%s\" not defined", funcName_.c_str());
    std::function<int(Node*, MifItem*)>& executeFunc = 
            opInternalFuncList[funcName_];
    CHECK_RET(executeFunc(node, item),
            "Failed to execute function [%d].", funcName_.c_str());
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

std::string opAssignFunc::str() {
    std::string symbol("<");
    symbol += funcName_;
    symbol += ">";
    return symbol;
}

int opAssignFunc::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    CHECK_ARGS(opInternalFuncList.find(funcName_) != opInternalFuncList.end(),
            "Function \"%s\" not defined.", funcName_.c_str());
    std::function<int(Node*, MifItem*)>& executeFunc = 
            opInternalFuncList[funcName_];
    CHECK_RET(executeFunc(node, item),
            "Failed to execute function [%s].", funcName_.c_str());
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

std::string opReplace::str() {
    std::stringstream symbol;
    symbol << "[" << startIndex_ << ":";
    symbol << length_ << "]=";
    return symbol.str();
}

int opReplace::process(Node* node, MifItem* item) {
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
    std::string startIndex_Str = content.substr(leftBracketIndex + 1,
            startPosLength);
    std::string lengthStr = content.substr(colonIndex + 1, lengthLength);
    CHECK_ARGS(isType(startIndex_Str, &startIndex_),
            "Start postion \"%s\" can not be converted to number.",
            startIndex_Str);
    CHECK_ARGS(isType(lengthStr, &length_),
            "Length \"%s\" can not be converted to number.",
            startIndex_Str);
    return 0;
}

} // namespace syntax

} // namespace condition_assign
