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
#define FUNCOP_REG(Name, Type, ...) \
    const std::set<DataType> func_op::FuncOperator##Name::dataTypes_ \
            {__VA_ARGS__}; \
    const Operator::OperatorType func_op::FuncOperator##Name::type_ = Type; \
    const int func_op::FuncOperator##Name::id_ = funcOperatorListInit( \
            ""#Name"", new func_op::FuncOperator##Name) + 217;

#define OPREG_SPECIAL(Name, Type, ...) \
    const std::set<DataType> Operator##Name::dataTypes_ {__VA_ARGS__}; \
    const Operator::OperatorType Operator##Name::type_ = Type; \
    const int Operator##Name::id_ = operatorListInit(new Operator##Name());

// 特殊的运算符的声明
class OperatorFunction : public Operator {
public:
    OperatorFunction() : Operator() {}
    ~OperatorFunction() = default;
    OperatorType type() {return type_;}
    std::string str() {return "";}
    int id() {return id_;}
    int process(Node* node, MifItem* item);
    int find(Operator** newOperatorPtr, const std::string& content,
            std::pair<size_t, size_t>* range, std::string* opName);
    bool isSupported(const DataType type) {return false;}

private:
    /* 用于记录当前运算符支持的数据类型 */
    static const std::set<DataType> dataTypes_;
    /* 用于记录当前运算符的唯一ID */
    static const int id_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
};

class OperatorPartialEqual : public Operator {
public:
    OperatorPartialEqual() : Operator() {}
    OperatorPartialEqual(const int startIndex, const int length);
    ~OperatorPartialEqual() = default;
    OperatorType type() {return type_;}
    std::string str();
    int id() {return id_;}
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
    /* 用于记录当前运算符的唯一ID */
    static const int id_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 记录了进行替换的两个关键变量
    int startIndex_, length_;
};

class OperatorReplace : public Operator {
public:
    OperatorReplace() : Operator() {}
    OperatorReplace(const int startIndex, const int length);
    ~OperatorReplace() = default;
    OperatorType type() {return type_;}
    std::string str();
    int id() {return id_;}
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
    /* 用于记录当前运算符的唯一ID */
    static const int id_;
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
    std::string str() {return "<InRange>";}
    int id() {return id_;}
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
    /* 用于记录当前运算符的唯一ID*/
    static const int id_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 记录了进行替换的两个关键变量
    std::set<std::string> rangeOfNum_;
};

class FuncOperatorEmpty : public Operator {
public:
    FuncOperatorEmpty() : Operator() {}
    ~FuncOperatorEmpty() = default;
    OperatorType type() {return type_;}
    std::string str() {return "<Empty>";}
    int id() {return id_;}
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
    /* 用于记录当前运算符的唯一ID*/
    static const int id_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
};

class FuncOperatorSetCoord : public Operator {
public:
    enum Method {Avg};
    FuncOperatorSetCoord() : Operator() {}
    FuncOperatorSetCoord(const Method method);
    ~FuncOperatorSetCoord() = default;
    OperatorType type() {return type_;}
    std::string str() {return "<SetCoord>";}
    int id() {return id_;}
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
    /* 用于记录当前运算符的唯一ID*/
    static const int id_;
    /* 用于记录当前运算符的类型 */
    static const OperatorType type_;
    // 处理的方法
    const Method method_ = Avg;
};

} // namespace func_op

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
