#include "NormalOperators.h"
#include "ConditionAssign.h"
#include "MifType.h"
#include "Group.h"

#include <regex>
#include <exception>

namespace condition_assign{

namespace syntax {

int OperatorNot::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(!node->leftNode && node->rightNode,
            "Bad node-tree structure!");
    CHECK_ARGS(node->op->isSupported(node->rightType),
            "Unsupported data type [Right: \"%s\"].",
            getTypeString(node->rightType).c_str());
    CHECK_RET(result = node->rightNode->op->process(node->rightNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->rightNode->tagName.c_str(),
            node->rightNode->op->str().c_str(),
            node->rightNode->value.stringValue.c_str());
    return ! result;
}

int OperatorOr::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(node->leftNode && node->rightNode, "Bad node-tree structure!");
    CHECK_ARGS(node->op->isSupported(node->leftType) &&
            node->op->isSupported(node->rightType),
            "Unsupported data type [Left: \"%s\"] or [Right: \"%s\"]!",
            getTypeString(node->leftType).c_str(),
            getTypeString(node->rightType).c_str());
    CHECK_RET(result = node->leftNode->op->process(node->leftNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->leftNode->tagName.c_str(),
            node->leftNode->op->str().c_str(),
            node->leftNode->value.stringValue.c_str());
    if (result == 1) return 1;
    CHECK_RET(result = node->rightNode->op->process(node->rightNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->rightNode->tagName.c_str(),
            node->rightNode->op->str().c_str(),
            node->rightNode->value.stringValue.c_str());
    return result;
}

int OperatorAnd::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(node->leftNode && node->rightNode, "Bad node-tree structure!");
    CHECK_ARGS(node->op->isSupported(node->leftType) &&
            node->op->isSupported(node->rightType),
            "Unsupported data type [Left: \"%s\"] or [Right: \"%s\"]!",
            getTypeString(node->leftType).c_str(),
            getTypeString(node->rightType).c_str());
    CHECK_RET(result = node->leftNode->op->process(node->leftNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->leftNode->tagName.c_str(),
            node->leftNode->op->str().c_str(),
            node->leftNode->value.stringValue.c_str());
    if (result == 0) return 0;
    CHECK_RET(result = node->rightNode->op->process(node->rightNode, item),
            "Operator process failed in \"%s %s %s\".",
            node->rightNode->tagName.c_str(),
            node->rightNode->op->str().c_str(),
            node->rightNode->value.stringValue.c_str());
    return result;
}

int OperatorEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return floatEqual(leftVal, node->value.numberValue);
    } else if (node->leftType == New) {
        return false;
    } else {
        std::string leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        leftVal = htk::trim(leftVal, "\"");
        return (leftVal == node->value.stringValue);
    }
}

int OperatorNotEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return ! floatEqual(leftVal, node->value.numberValue);
    } else if (node->leftType == New) {
        return false;
    } else {
        std::string leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        leftVal = htk::trim(leftVal, "\"");
        return (leftVal != node->value.stringValue);
    }
}

int OperatorLessEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return floatLessEqual(leftVal, node->value.numberValue);
}

int OperatorLessThan::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return (leftVal < node->value.numberValue);
}

int OperatorGreaterEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return floatGreaterEqual(leftVal, node->value.numberValue);
}

int OperatorGreaterThan::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return (leftVal > node->value.numberValue);
}

int OperatorContain::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    return (leftVal.find(node->value.stringValue) != std::string::npos);
}

int OperatorIsPrefix::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    return htk::startswith(leftVal, node->value.stringValue);
}

int OperatorIsSuffix::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    return htk::endswith(leftVal, node->value.stringValue);
}

int OperatorRegularExpr::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    const std::string regularExpr = node->value.stringValue;
    try {
        const std::regex pattern(regularExpr);
        return std::regex_search(leftVal, pattern);
    } catch (const std::exception& except) {
        CHECK_RET(-1, "Failed to %s \"%s\" in \"%s\". Return info: %s.",
                "search regular expression", regularExpr.c_str(),
                leftVal.c_str(), except.what());
    }
    return 0;
}

int OperatorTagContain::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    leftVal = htk::trim(leftVal, "\"");
    if (groupPtr->isDynamic()) {
        Group* dynamicGroup;
        CHECK_RET(groupPtr->buildDynamicGroup(&dynamicGroup, item),
                "Failed to build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() == Group::Tag,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneContain(leftVal, &result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() == Group::Tag,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneContain(leftVal, &result),
                "Failed to running group-check function.");
    }
    return result;
}

int OperatorGeoContain::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneContain(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneContain(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoContainAll::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkAllContain(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkAllContain(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoContained::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneContained(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneContained(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoContainedAll::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkAllContained(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkAllContained(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoIntersect::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneIntersect(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneIntersect(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoIntersectAll::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkAllIntersect(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkAllIntersect(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoAtEdge::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneAtEdge(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneAtEdge(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoAtEdgeAll::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkAllAtEdge(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkAllAtEdge(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoDeparture::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkOneDeparture(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkOneDeparture(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorGeoDepartureAll::process(Node* node, MifItem* item) {
    bool* result = new bool();
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    Group* dynamicGroup;
    wsl::Geometry* leftVal;
    // 缓存计算检查
    int64_t groupKey;
    if (groupPtr->isDynamic()) {
        groupKey = keyGenerate(groupPtr->info_->tagName_,
                groupPtr->groupKey_);
    } else {
        groupKey = groupPtr->groupKey_;
    }
    if (item->findInsertProcessResult(&result, groupKey * 131 + id_)) {
        return *result;
    }
    Group::Type inputType = item->srcLayer_->getGeoType();
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    groupPtr->ready_.wait();
    if (groupPtr->isDynamic()) {
        CHECK_RET(item->findBuildDynamicGroup(&dynamicGroup, groupKey,
                groupPtr), "Failed to get or build dynamic group.");
        CHECK_ARGS(dynamicGroup->getGroupType() != Group::Tag &&
                dynamicGroup->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(dynamicGroup->checkAllDeparture(inputType, leftVal, result),
                "Failed to running group-check function.");
        delete dynamicGroup;
    } else {
        CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
                groupPtr->getGroupType() != Group::Item,
                "Group type not supported");
        CHECK_RET(groupPtr->checkAllDeparture(inputType, leftVal, result),
                "Failed to running group-check function.");
    }
    return *result;
}

int OperatorAssign::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string newVal = "\"";
    newVal = node->rightType != Number ? newVal +
            node->value.stringValue + "\"" : node->value.stringValue;
    CHECK_RET(item->assignWithTag(node->tagName, newVal),
            "Failed to assign value to tag \"%s\".", node->tagName.c_str());
    return 1;
}

int OperatorSelfAdd::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftValString;
    if (node->leftType != New) {
        CHECK_RET(item->getTagVal(node->tagName, &leftValString),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
    }
    leftValString = htk::trim(leftValString, "\"");
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal = 0.0f;
        std::string prefix = "";
        std::stringstream tempStream;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        leftVal += node->value.numberValue;
        int intLeftVal = static_cast<int>(leftVal);
        if (abs(leftVal - intLeftVal) < 1e-8) {
            tempStream << intLeftVal;
        } else {
            tempStream << leftVal;
        }
        int lackZeros = leftValString.length() - tempStream.str().length();
        if (lackZeros > 0) {
            prefix.append(lackZeros, '0');
        }
        leftValString = prefix + tempStream.str();
    } else {
        leftValString += node->value.stringValue;
        std::string newVal = "\"";
        leftValString = node->leftType != Number ? newVal +
            leftValString + "\"" : leftValString;
    }
    CHECK_RET(item->assignWithTag(node->tagName, leftValString),
            "Failed to assign value to tag \"%s\".", node->tagName.c_str());
    return 1;
}

} // namespace syntax

} // namespace condition_assign
