#ifndef SYNTAXBASE_H
#define SYNTAXBASE_H

#include "type_factory.h"
#include "ConditionAssign.h"

#include <string>
#include <vector>
#include <list>
#include <stack>

namespace condition_assign {

class Group;
class MifItem;
class ConfigItem;

namespace syntax {

// 比较浮点数的最大误差
#define MAXERROR 1e-8

// 双目非逻辑运算符的检查过程
#define BINARYOP_CHECK() { \
    CHECK_ARGS(!node->leftNode && !node->rightNode, \
            "Bad node-tree structure!"); \
    CHECK_ARGS(node->op->isSupported(node->leftType) && \
            node->op->isSupported(node->rightType), \
            "Unsupported data type [Left: \"%s\"] or [Right: \"%s\"]!", \
            getTypeString(node->leftType).c_str(), \
            getTypeString(node->rightType).c_str()); \
}

// 用于生成一个字符串的hash数值
int64_t keyGenerate(const std::string& str, int64_t oldHash = 0);
// 数据类型
enum DataType {Empty, New, Number, String, GroupType, Expr};
// debug使用获取数据类型对应的字符串
std::string getTypeString(const DataType type);
// 获取一个字符串的类型
DataType getDataType(const std::string data, const bool strictCheck,
        std::string* stringVal = nullptr, double* numberVal = nullptr);
// 判断一个数值是给定类型
template<typename T> bool isType(const std::string& data, T* result);

// 用于判断一个浮点数是否可以约分为整数
inline bool canRoundToInt(const double val, int* intVal) {
    int intTemp = static_cast<int>(val);
    if (abs(val - intTemp) > 0.999) {
        *intVal = ++intTemp;
        return abs(val - intTemp) < MAXERROR;
    } else {
        *intVal = intTemp;
        return abs(val - intTemp) < MAXERROR;
    }
}

// 用于比较两个浮点数
inline bool floatEqual(const double a, const double b) {
    return fabs(a - b) < MAXERROR;
}
// 用于比较两个浮点数
inline bool floatLessEqual(const double a, const double b) {
    return a < b || fabs(a - b) < MAXERROR;
}
// 用于比较两个浮点数
inline bool floatGreaterEqual(const double a, const double b) {
    return a > b || fabs(a - b) < MAXERROR;
}

struct Node;

// 运算符类
class Operator {
public:
    // 运算符的类型，类型可以辅助解析
    enum OperatorType {Condition, Assign};
    // 获取当前运算符的运算评分
    virtual int id() = 0;
    // 获取当前运算符的类型
    virtual OperatorType type() = 0;
    // 根据给定的节点数据，计算运算结果
    virtual int process(Node* node, MifItem* item) = 0;
    // 检查给定的数据类型是否支持
    virtual bool isSupported(const DataType type) = 0;
    // 找到对应操作符的在当前行范围的函数, 范围是左闭右开的
    virtual int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range, std::string* opName) = 0;
    // 获取当前运算符的字符串形式
    virtual std::string str() = 0;
    
    // 构造函数
    Operator() = default;
    // 虚析构函数
    virtual ~Operator() = default;
};

// 配置文件语法分析节点
struct Node {
    // 归约深度，用于解析
    int reduceDepth = -1;
    // 当前节点左右子节点
    Node* leftNode = nullptr;
    Node* rightNode = nullptr;
    // 当前节点左侧的Tag名，如果存在的话
    std::string tagName = "";
    // 当前节点对应的Op，如果有的话
    Operator* op = nullptr;
    // 右值的数据类型
    DataType rightType;
    // 左值的数据类型
    DataType leftType;
    // 右值数据
    struct {
        // 指向对应的组的对象
        Group* groupPtr = nullptr;
        // 数值
        double numberValue = 0.0f;
        // 字符串
        std::string stringValue = "";
    } value;
};

// 所有的运算符注册表
extern std::vector<Operator*> operatorList;
// 用于运算符注册的函数
int operatorListInit(Operator* newOp);

// 计算一个节点向量的分数
// int calculateScore(const std::vector<Node*>& nodeVec);

} // namesapce syntax

// 判断一个给定MifItem是否满足条件
int satisfyConditions(const ConfigItem& configItem, MifItem* item);
// 对MifItem执行赋值操作
int applyAssigns(const ConfigItem& configItem, MifItem* item);

} // namespace condition_assign

#endif // SYNTAXBASE_H
