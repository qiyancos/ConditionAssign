#ifndef SPECIALOPERATORS_H
#define SPECIALOPERATORS_H

#include "SyntaxBase.h"

#include <functional>

namespace condition_assign {

namespace syntax {

// 自由配置运算符内置函数的函数名与函数的映射
extern std::map<std::string, Operator*> funcOperatorList;
// 用于运算符内置函数注册的函数
int funcOperatorListInit(const std::string name, Operator* newFuncOp);

// 用于运算符内置函数注册的宏
#define FUNCOP_REG(Name, Type, Score, ...) \
    const std::set<DataType> func_op::FuncOperator##Name::dataTypes_ \
            {__VA_ARGS__}; \
    const int func_op::FuncOperator##Name::score_ = Score; \
    const Operator::OperatorType func_op::FuncOperator##Name::type_ = Type; \
    int globalInit##Name = funcOperatorListInit(""#Name"", \
            new func_op::FuncOperator##Name);

#define OPREG_SPECIAL(Name, Type, Score, ...) \
    const std::set<DataType> Operator##Name::dataTypes_ {__VA_ARGS__}; \
    const int Operator##Name::score_ = Score; \
    const Operator::OperatorType Operator##Name::type_ = Type; \
    int globalOpReg##Name = operatorListInit(new Operator##Name());

// 特殊的运算符的声明
class OperatorFunction : public Operator {
public:
    OperatorFunction() : Operator() {}
    ~OperatorFunction() = default;
    OperatorType type() {return type_;}
    std::string str() {return "";}
    int score() {return score_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range, std::string* opName);
    bool isSupported(const DataType type) {return false;}

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;
    /* 用于记录当前运算符的分数 */
    static const int score_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
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
            std::pair<size_t, size_t>* range, std::string* opName);
    bool isSupported(const DataType type) {
        if (dataTypes_.find(type) != dataTypes_.end()) {
            return true;
        } else if (type == Number && dataTypes_.find(String) !=
                dataTypes_.end()){
            return true;
        } else {
            return false;
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

namespace func_op {

class FuncOperatorInRange : public Operator {
public:
    FuncOperatorInRange() : Operator() {}
    FuncOperatorInRange(const int startNumber, const int endNumber);
    ~FuncOperatorInRange() = default;
    OperatorType type() {return type_;}
    std::string str() {return "InRange";}
    int score() {return score_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range, std::string* opName);
    bool isSupported(const DataType type) {
        if (dataTypes_.find(type) != dataTypes_.end()) {
            return true;
        } else if (type == Number && dataTypes_.find(String) !=
                dataTypes_.end()){
            return true;
        } else {
            return false;
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
    std::set<std::string> rangeOfNum_;
};

} // namespace func_op

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
