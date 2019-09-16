#include "SpecialOperators.h"
#include "ConditionAssign.h"
#include "MifType.h"

namespace condition_assign {

namespace syntax {

int funcOperatorListInit(const std::string name, Operator* newFuncOp) {
    funcOperatorList[name] = newFuncOp;
    return funcOperatorList.size();
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
    range->second = rightBound + 1;
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

OperatorPartialEqual::OperatorPartialEqual(const int startIndex,
        const int length) : Operator(), startIndex_(startIndex),
        length_(length) {}

std::string OperatorPartialEqual::str() {
    std::stringstream symbol;
    symbol << "[" << startIndex_ << ":";
    symbol << length_ << "]==";
    return symbol.str();
}

int OperatorPartialEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Tag [%s] not found!", node->tagName.c_str());
    if (startIndex_ + length_ > leftVal.size()) {
        return 0;
    }
    return leftVal.substr(startIndex_, length_) == node->value.stringValue;
}

int OperatorPartialEqual::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "OperatorPartialEqual";
    size_t leftBracketIndex = content.find("[");
    size_t colonIndex = content.find(":");
    size_t rightBracketIndex = content.find("]==");
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
    range->second = rightBracketIndex + 3;
    std::string startIndexString = content.substr(leftBracketIndex + 1,
            startPosLength);
    std::string lengthStr = content.substr(colonIndex + 1, lengthLength);
    CHECK_ARGS(isType(startIndexString, &startIndex_),
            "Start postion \"%s\" can not be converted to number.",
            startIndexString.c_str());
    CHECK_ARGS(isType(lengthStr, &length_),
            "Length \"%s\" can not be converted to number.",
            startIndexString.c_str());\
    *newOperatorPtr = new OperatorPartialEqual(startIndex_, length_);
    return 1;
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
    if (node->leftType == New) {
        return false;
    }
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Tag [%s] not found!", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    return rangeOfNum_.find(leftVal) != rangeOfNum_.end();
}

int FuncOperatorInRange::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "FuncOperatorInRange";
    const std::string arguments = content.substr(range->second + 1,
            content.size() - range->second - 1);
    size_t minusIndex = arguments.find("-");
    CHECK_ARGS(minusIndex != std::string::npos, "Can not find %s \"%s\".",
            "delimiter in function arguments", arguments.c_str());
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

int FuncOperatorEmpty::process(Node* node, MifItem* item) {
    // 空函数应保证对NEW类型输出起作用
    if (item->targetLayer_->isNew()) {
        CHECK_RET(item->addAsNewItem(),
                "Failed to add new mif item to new mif layer");
    }
    return 1;
}

int FuncOperatorEmpty::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "FuncOperatorEmpty";
    *newOperatorPtr = new FuncOperatorEmpty();
    return 1;
}

FuncOperatorSetCoord::FuncOperatorSetCoord(const Method method) :
        method_(method) {}

int FuncOperatorSetCoord::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double newX, newY;
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup = nullptr;
    int64_t groupKey = keyGenerate(groupPtr->info_->tagName_,
            groupPtr->groupKey_);
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        groupPtr = dynamicGroup;
    }
    CHECK_ARGS(item->srcLayer_->getGeoType() == Group::Point,
            "Unsupported layer geometry type for coord setting.");
    CHECK_ARGS(groupPtr->getGroupType() == Group::Point,
            "Unsupported group geometry type for coord setting.");
    std::string tagName = "x";
    switch(method_) {
    case Avg:
        newX = newY = 0.0;
        for (auto& geometry :
                reinterpret_cast<GeometryGroup*>(groupPtr)->group_) {
            newX += geometry->at(0).at(0).x();
            newY += geometry->at(0).at(0).y();
        }
        if (groupPtr->size()) {
            newX /= groupPtr->size();
            newY /= groupPtr->size();
        } else {
            newX = newY = 0.0f;
        }
        break;
    case FullAvg:
        CHECK_RET(item->getTagVal(tagName, &newX),
                "Failed to get x coord of processing item.");
        tagName = "y";
        CHECK_RET(item->getTagVal(tagName, &newY),
                "Failed to get x coord of processing item.");
        for (auto& geometry :
                reinterpret_cast<GeometryGroup*>(groupPtr)->group_) {
            newX += geometry->at(0).at(0).x();
            newY += geometry->at(0).at(0).y();
        }
        newX /= (groupPtr->size() + 1);
        newY /= (groupPtr->size() + 1);
        break;
    }
    tagName = "x";
    CHECK_RET(item->assignWithNumber(tagName, newX), "Failed to set x coord.");
    tagName = "y";
    CHECK_RET(item->assignWithNumber(tagName, newY), "Failed to set y coord.");
    return 1;
}

int FuncOperatorSetCoord::find(Operator** newOperatorPtr,
        const std::string& content, std::pair<size_t, size_t>* range,
        std::string* opName) {
    *opName = "FuncOperatorSetCoord";
    const std::string arguments = content.substr(range->second + 1,
            content.size() - range->second - 1);
    const size_t argDelimIndex = arguments.find("@");
    Method method = Avg;
    if (argDelimIndex != std::string::npos) {
        const std::string methodStr = arguments.substr(argDelimIndex + 1);
        if (methodStr == "FULLAVG" || methodStr == "FullAvg") {
            method = FullAvg;
        } else {
            CHECK_ARGS(methodStr == "AVG" || methodStr == "Avg",
                    "Unrecognized method \"%s\".", methodStr.c_str());
        }
    }
    *newOperatorPtr = new FuncOperatorSetCoord(method);
    return 1;
}
} // namespace func_op

} // namespace syntax

} // namespace condition_assign
