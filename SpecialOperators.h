#ifndef SPECIALOPERATORS_H
#define SPECIALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 自由配置运算符内置函数的函数名与函数的映射
std::map<std::string, std::function<int(Node*,
        MifItem*)>> opInternalFuncList;
// 用于运算符内置函数注册的函数
int opInternalFuncListInit(const std::string& name,
        const std::function<int(Node*, MifItem* item)>& func);
// 用于运算符内置函数注册的宏
#define OPFUNC_REG(Name, Func) {\
    int globalInit##Name = opInternalFuncListInit(Name, Func); \
}

// 特殊的运算符的声明
class opCondFunc : public Operator {
public:
    opCondFunc() : Operator() {}
    OperatorType type() {return Condition;}
    std::string str();
    int score() {return 1;}
    Operator* newOperator() {return new opCondFunc();}
    int process(Node* node, MifItem* item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
    bool isSupported(DataType type) const {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;

private:
    // 记录了函数名
    std::string funcName_;
};

std::set<DataType> opCondFunc::dataTypes_ {Number, String, Group};

OPREG(CondFunc);

class opAssignFunc : public Operator {
public:
    opAssignFunc() : Operator() {}
    OperatorType type() {return Assign;}
    std::string str();
    int score() {return 1;}
    Operator* newOperator() {return new opAssignFunc();}
    int process(Node* node, MifItem* item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
    bool isSupported(DataType type) const {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;

private:
    // 记录了函数名
    std::string funcName_;
};

std::set<DataType> opAssignFunc::dataTypes_ {Number, String, Group};

OPREG(AssignFunc);

class opReplace : public Operator {
public:
    opReplace() : Operator() {}
    OperatorType type() {return Assign;}
    std::string str();
    int score() {return 1;}
    Operator* newOperator() {return new opReplace();}
    int process(Node* node, MifItem* item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
    bool isSupported(DataType type) const {
        if (dataTypes_.find(type) == dataTypes_.end()) {
            return false;
        } else {
            return true;
        }
    }

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;

private:
    // 记录了进行替换的两个关键变量
    int startIndex_, length_;
};

std::set<DataType> opReplace::dataTypes_ {String};

OPREG(Replace);

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
