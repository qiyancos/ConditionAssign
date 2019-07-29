#ifndef SPECIALOPERATORS_H
#define SPECIALOPERATORS_H

#include "SyntaxBase.h"

#include <functional>

namespace condition_assign {

namespace syntax {

// 自由配置运算符内置函数的函数名与函数的映射
extern std::map<std::string, std::function<int(Node*,
        MifItem*)>> opInternalFuncList;
// 用于运算符内置函数注册的函数
int opInternalFuncListInit(const std::string& name,
        const std::function<int(Node*, MifItem* item)>& func);
// 用于运算符内置函数注册的宏
#define OPFUNC_REG(Name, Func) \
    int globalInit##Name = opInternalFuncListInit(Name, Func); \

#define OPREG_SPECIAL(Name, Type, Score, ...) \
    const std::set<DataType> Operator##Name::dataTypes_ {__VA_ARGS__}; \
    const int Operator##Name::score_ = Score; \
    const Operator::OperatorType Operator##Name::type_ = Type; \
    int globalOpReg##Name = operatorListInit(new Operator##Name());

// 特殊的运算符的声明
class OperatorCondFunc : public Operator {
public:
    OperatorCondFunc() : Operator() {}
    OperatorCondFunc(const std::string& funcName);
    ~OperatorCondFunc() = default;
    OperatorType type() {return type_;}
    std::string str();
    int score() {return score_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range);
    bool isSupported(const DataType type) {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;
    /* 用于记录当前运算符的分数 */
    static const int score_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 记录了函数名
    std::string funcName_;
};

class OperatorAssignFunc : public Operator {
public:
    OperatorAssignFunc() : Operator() {}
    OperatorAssignFunc(const std::string& funcName);
    ~OperatorAssignFunc() = default;
    OperatorType type() {return type_;}
    std::string str();
    int score() {return score_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range);
    bool isSupported(const DataType type) {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;
    /* 用于记录当前运算符的分数 */
    static const int score_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 记录了函数名
    std::string funcName_;
};

class OperatorReplace : public Operator {
public:
    OperatorReplace() : Operator() {}
    OperatorReplace(const int startIndex, const int length);
    ~OperatorReplace() = default;
    OperatorType type() {return type_;}
    std::string str();
    int score() {return score_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range);
    bool isSupported(const DataType type) {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;
    /* 用于记录当前运算符的分数 */
    static const int score_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 记录了进行替换的两个关键变量
    int startIndex_, length_;
};

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
