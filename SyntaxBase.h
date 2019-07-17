#ifndef SYNTAXBASE_H_
#define SYNTAXBASE_H

#include "Group.h"
#include "ResourcePool.h"

#include <string>
#include <vector>
#include <stack>

namespace condition_assign {

namespace syntax {

// 比较浮点数的最大误差
#define MAXERROR 1e-8

// 双目非逻辑运算符的检查过程
#define BINARYOP_CHECK() { \
    CHECK_ARGS(node->leftNode == nullptr && node->rightNode == nullptr, \
            "Bad node-tree structure!"); \
    CHECK_ARGS(node->op.isSupported(node->leftType) && \
            node->op.isSupported(node->rightType) \
            "Unsupported data type!"); \
}

// 用于运算符的注册宏
#define OPREG(Name) { \
    int globalOpReg##Name = operatorListInit(op##Name()); \
}

// 数据类型
enum DataType {Number, String, Group, Expr};
// 获取一个字符串的类型
DataType getDataType(const std::string& data, std::string* stringVal,
        double* numberVal);
// 判断一个数值是给定类型
template<typename T>
bool isType(const std::string& data, T* result);
// 用于比较两个浮点数
inline bool floatEqual(const double a, const double b) {
    return abs(a - b) < MAXERROR;
}
// 用于比较两个浮点数
inline bool floatLessEqual(const double a, const double b) {
    return a < b || abs(a - b) < MAXERROR;
}
// 用于比较两个浮点数
inline bool floatGreaterEqual(const double a, const double b) {
    return a > b || abs(a - b) < MAXERROR;
}

struct Node;

// 运算符类
class Operator {
public:
    // 获取当前运算符对应的字符串
    std::string str();
    // 获取当前运算符的运算评分
    int score();
    // 根据给定的节点数据，计算运算结果
    virtual int process(Node* node, const MifItem& item);
    // 检查给定的数据类型是否支持
    bool isSupported(DataType type);
    // 找到对应操作符的在当前行范围的函数, 范围是左闭右开的
    virtual int find(const std::string& content,
            std::pair<size_t, size_t>* range);
    // 虚析构函数
    virtual ~Operator();

private:
    // 当前运算符支持的数据类型
    const std::set<DataType> dataTypes_;
    // 当前运算符的运算评分
    const int score_;
    // 当前运算符对应的字符串
    const std::string str_;
};

// 配置文件语法分析节点
struct Node {
    // 当前节点所生成的数据类型
    DataType nodeType;
    // 当前节点左右子节点
    Node* leftNode = nullptr;
    Node* rightNode = nullptr;
    // 当前节点左侧的Tag名，如果存在的话
    std::string tagName;
    // 当前节点对应的Op，如果有的话
    Operator* op;
    // 特殊运算符参数的索引
    int opParamIndex;
    // 右值的数据类型
    DataType rightType;
    // 左值的数据类型
    DataType leftType;
    // 右值数据
    struct {
        // 指向对应的组的对象
        Group* groupPtr;
        // 数值
        double numberValue;
        // 字符串
        std::string stringValue;
        // 逻辑表达式结果
        bool exprResult;
    } value;
};

// 所有的运算符注册表
std::vector<Operator> operatorList;
// 用于运算符注册的函数
int operatorListInit(const Operator&& newOp);

} // namesapce syntax

} // namespace condition_assign

#endif // SYNTAXBASE_H
