#include "NormalOperators.h"
#include "ConditionAssign.h"

namespace condition_assign{

namespace syntax {

int opNot::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(node->leftNode == nullptr && node->rightNode != nullptr, \
            "Bad node-tree structure!"); \
    CHECK_ARGS(node->op.isSupported(node->leftType), \
            "Unsupported data type."); \
    CHECK_RET(result = node->rightNode->op->process(node->rightNode, item),
            "Operator process failed in \"%s %s %s\".",                         
            node->rightNode->tagName.c_str(),
            node->rightNode->op->str().c_str(),
            node->rightNode->value.stringValue.c_str());
    return ! result;
}

int opOr::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(node->leftNode != nullptr && node->rightNode != nullptr,
            "Bad node-tree structure!");
    CHECK_ARGS(node->op.isSupported(node->leftType) &&
            node->op.isSupported(node->rightType),
            "Unsupported data type!");
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
    return rightResult;
}

int opAnd::process(Node* node, MifItem* item) {
    int result;
    CHECK_ARGS(node->leftNode != nullptr && node->rightNode != nullptr,
            "Bad node-tree structure!");
    CHECK_ARGS(node->op.isSupported(node->leftType) &&
            node->op.isSupported(node->rightType),
            "Unsupported data type!");
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
    return rightResult;
}

int opEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return floatEqual(leftVal, node->value.numberValue);
    } else {
        std::string leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return (leftVal == node->value.stringValue);
    }
}

int opNotEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return ! floatEqual(leftVal, node->value.numberValue);
    } else {
        std::string leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        return (leftVal != node->value.stringValue);
    }
}

int opLessEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return floatLessEqual(leftVal, node->value.numberValue);
}

int opLessThan::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return (leftVal < node->value.numberValue);
}

int opGreaterEqual::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return floatGreaterEqual(leftVal, node->value.numberValue);
}

int opGreaterThan::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal,
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return (leftVal > node->value.numberValue);
}

int opContain::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return (node->value.stringValue.find(leftVal) != std::string::npos);
}

int opIsPrefix::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return htk::startswith(leftVal, node->value.stringValue);
}

int opIsSuffix::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return htk::endswith(leftVal, node->value.stringValue);
}

int opRegularExpr::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_ARGS(node->value.stringValue.substr(0, 2) == "^(",
            "Regular expression format not good.");
    CHECK_ARGS(htk::endswith(node->value.stringValue, ")$"),
            "Regular expression format not good.");
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    return htk::RegexSearch(leftVal, node->value.stringValue);
}

int opTagContain::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() == Group::Tag,
            "Group type not supported");
    std::string leftVal;
    CHECK_RET(item->getTagVal(node->tagName, &leftVal),
            "Can not get value of tag \"%s\".", node->tagName.c_str());
    CHECK_RET(groupPtr->checkOneContain(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoContain::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkOneContain(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoContainAll::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkAllContain(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoContained::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkOneContained(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoContainedAll::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkAllContained(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoIntersect::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkOneIntersect(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoIntersectAll::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkAllIntersect(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoInContact::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkOneInContact(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoInContactAll::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkAllInContact(leftVal, &result)),
            "Failed to running group-check function.");
    return result;
}

int opGeoDeparture::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkOneDeparture(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opGeoDepartureAll::process(Node* node, MifItem* item) {
    bool result;
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    groupPtr->ready_.wait();
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item->getGeometry(&leftVal),
            "Failed to get mif item geometry info.");
    CHECK_RET(groupPtr->checkAllDeparture(leftVal, &result),
            "Failed to running group-check function.");
    return result;
}

int opAssign::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    CHECK_RET(item->assignWithTag(node->tagName, node->value.stringValue),
            "Failed to assign value to tag \"%s\".", node->tagName.c_str());
    return 0;
}

int opSelfAdd::process(Node* node, MifItem* item) {
    BINARYOP_CHECK();
    std::stringstream tempStream;
    if (node->leftType == node->rightType && node->leftType == Number) {
        double leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        int intLeftVal = static_cast<int>(leftVal + node->value.numberValue);
        if (abs(leftVal - intLeftVal) < 1e-8) {
            tempStream << intLeftVal;
        } else {
            tempStream << leftVal;
        }
    } else {
        std::string leftVal;
        CHECK_RET(item->getTagVal(node->tagName, &leftVal),
                "Can not get value of tag \"%s\".", node->tagName.c_str());
        leftVal += node->value.stringValue;
        tempStream << leftVal;
    }
    CHECK_RET(item->assignWithTag(node->tagName, tempStream.str()),
            "Failed to assign value to tag \"%s\".", node->tagName.c_str());
    return 0;
}

} // namespace syntax

} // namespace condition_assign
