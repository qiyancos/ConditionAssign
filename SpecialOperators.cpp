#include "SpecialOperators.h"

namespace condition_assign {

namespace syntax {

int opCondFunc::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    funcNameListLock.lock();
    std::string funcName = funcNameList[node->opParamsIndex];
    funcNameListLock.unlock();
    CHECK_ARGS(opInternalFuncList.find(funcName) != opInternalFuncList.end(),
            std::string(std::string("Function \"") + funcName +
            "\" not defined").c_str());
    paramListLock.lock();
    void* funcParam = paramList[node->opParamIndex];
    paramListLock.unlock();
    std::function<int(Node*, const MifItem&, void*)>& executeFunc = 
            opInternalFuncList[funcName];
    CHECK_RET(executeFunc(node, item, funcParam));
    return 0;
}

int opCondFunc::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == content::npos || range->second == content::npos) {
        return -1;
    }
    if (length < 0) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    paramListLock.lock();
    paramList.push_back(content.substr(range->first, length));
    paramListWriteLock.unlock();
    return 0;
}

int opAssignFunc::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    funcNameListLock.lock();
    std::string funcName = funcNameList[node->opParamsIndex];
    funcNameListLock.unlock();
    CHECK_ARGS(opInternalFuncList.find(funcName) != opInternalFuncList.end(),
            std::string(std::string("Function \"") + funcName +
            "\" not defined").c_str());
    paramListLock.lock();
    void* funcParam = paramList[node->opParamIndex];
    paramListLock.unlock();
    std::function<int(Node*, const MifItem&, void*)>& executeFunc = 
            opInternalFuncList[funcName];
    CHECK_RET(executeFunc(node, item, funcParam));
    return 0;
}

int opAssignFunc::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    range->first = content.find("<");
    range->second = content.find(">");
    int length = range->second - range->first - 1;
    if (range->first == content::npos || range->second == content::npos) {
        return -1;
    }
    if (length < 0) {
        return -1;
    }
    CHECK_ARGS(!length, "No function name is given in condition expression.");
    paramListLock.lock();
    paramList.push_back(content.substr(range->first, length));
    paramListWriteLock.unlock();
    return 0;
}

int opReplace::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    item.getTagVal(node->tagName, &leftVal);
    paramListLock.lock();
    const std::pair<int, int>& funcParam = paramList[node->opParamIndex];
    paramListLock.unlock();
    CHECKARGS(funcParam.second < leftVal.size(),
            "Replace position out of range");
    leftVal.replace(funcParam.first, funcParam.second,
            node->value.stringValue);
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
    std::string startPos = content.substr(leftBracketIndex + 1,
            startPosLength);
    std::string length = content.substr(colonIndex + 1, lengthLength);
    CHECK_ARGS(isType<int>(startPos) && isType<int>(length));
    paramListLock.lock();
    paramList.push_back(std::pair<int, int>(atoi(startPos.c_str()),
            atoi(length.c_str())));
    return 0;
}

} // namespace syntax

} // namespace condition_assign
