#include "NormalOperators.h"

namespace condition_assign{

namespace syntax {

// 双目非逻辑运算符的检查过程
#define BINARYOP_CHECK() { \
    CHECK_ARGS(node->leftNode == nullptr && node->rightNode == nullptr, \
            "Bad node-tree structure!"); \
    CHECK_ARGS(node->op.isSupported(node->leftType) && \
            node->op.isSupported(node->rightType) \
            "Unsupported data type!"); \
}

int opNot::process(Node* node, const MifItem& item) {
    CHECK_ARGS(node->leftNode == nullptr && node->rightNode != nullptr, \
            "Bad node-tree structure!"); \
    CHECK_ARGS(node->op.isSupported(node->leftType), \
            "Unsupported data type!"); \
    node->value.exprResult = ! node->rightNode->value.exprResult;
    return 0;
}

int opOr::process(Node* node, const MifItem& item) {
    CHECK_ARGS(node->leftNode != nullptr && node->rightNode != nullptr,
            "Bad node-tree structure!");
    CHECK_ARGS(node->op.isSupported(node->leftType) &&
            node->op.isSupported(node->rightType),
            "Unsupported data type!");
    node->value.exprResult = node->leftNode->value.exprResult ||
            node->rightNode->value.exprResult;
    return 0;
}

int opAnd::process(Node* node, const MifItem& item) {
    CHECK_ARGS(node->leftNode != nullptr && node->rightNode != nullptr,
            "Bad node-tree structure!");
    CHECK_ARGS(node->op.isSupported(node->leftType) &&
            node->op.isSupported(node->rightType),
            "Unsupported data type!");
    node->value.exprResult = node->leftNode->value.exprResult &&
            node->rightNode->value.exprResult;
    return 0;
}

int opEqual::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Num) {
        double leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        node->value.exprResult = floatEqual(leftVal, node->value.numberValue);
    } else {
        std::string leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        node->value.exprResult = (leftVal == node->value.stringValue);
    }
    return 0;
}

int opNotEqual::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    if (node->leftType == node->rightType && node->leftType == Num) {
        double leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        node->value.exprResult = ! floatEqual(leftVal,
                node->value.numberValue);
    } else {
        std::string leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        node->value.exprResult = (leftVal != node->value.stringValue);
    }
    return 0;
}

int opLessEqual::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = floatLessEqual(leftVal, node->value.numberValue);
    return 0;
}

int opLessThan::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = (leftVal < node->value.numberValue);
    return 0;
}

int opGreaterEqual::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = floatGreaterEqual(leftVal,
            node->value.numberValue);
    return 0;
}

int opGreaterThan::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    double leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = (leftVal > node->value.numberValue);
    return 0;
}

int opContain::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = (node->value.stringValue.find(leftVal) !=
            node->value.stringValue::npos);
    return 0;
}

int opIsPrefix::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = htk::startswith(leftVal,
            node->value.stringValue);
    return 0;
}

int opIsSuffix::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = htk::endswith(leftVal, node->value.stringValue);
    return 0;
}

int opRegularExpr::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = htk::RegexSearch(leftVal,
            node->value.stringValue);
    return 0;
}

int opTagContain::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() == Group::Tag,
            "Group type not supported");
    std::string leftVal;
    CHECK_RET(item.getTagVal(node->tagName, &leftVal));
    node->value.exprResult = groupPtr->checkOneContain(leftVal);
    return 0;
}

int opGeoContain::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkOneContain(leftVal);
    return 0;
}

int opGeoContainAll::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkAllContain(leftVal);
    return 0;
}

int opGeoContained::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkOneContained(leftVal);
    return 0;
}

int opGeoContainedAll::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkAllContained(leftVal);
    return 0;
}

int opGeoIntersect::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkOneIntersect(leftVal);
    return 0;
}

int opGeoIntersectAll::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkAllIntersect(leftVal);
    return 0;
}

int opGeoDeparture::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkOneDeparture(leftVal);
    return 0;
}

int opGeoDepartureAll::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    Group* groupPtr = node->value.groupPtr;
    CHECK_ARGS(groupPtr->info_ == nullptr, "Group data not ready");
    CHECK_ARGS(groupPtr->getGroupType() != Group::Tag &&
            groupPtr->getGroupType() != Group::Item,
            "Group type not supported");
    CHECK_ARGS(groupPtr->getInputType() != Group::Point,
            "Only contain(ed) functions support point-type mif item!");
    wsl::Geometry* leftVal;
    CHECK_RET(item.getGeometry(&leftVal));
    node->value.exprResult = groupPtr->checkAllDeparture(leftVal);
    return 0;
}

int opAssign::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    CHECK_RET(item.assignWithTag(node->tagName, node->value.stringValue);
    return 0;
}

int opSelfAdd::process(Node* node, const MifItem& item) {
    BINARYOP_CHECK();
    std::stringstream tempStream;
    if (node->leftType == node->rightType && node->leftType == Num) {
        double leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        int intLeftVal = static_cast<int>(leftVal + node->value.numberValue);
        if (abs(leftVal - intLeftVal) < 1e-8) {
            tempStream << intLeftVal;
        } else {
            tempStream << leftVal;
        }
    } else {
        std::string leftVal;
        CHECK_RET(item.getTagVal(node->tagName, &leftVal));
        leftVal += node->value.stringValue;
        tempStream << leftVal;
    }
    CHECK_RET(item.assignWithTag(node->tagName, tempStream.str());
    return 0;
}

} // namespace syntax

} // namespace condition_assign
