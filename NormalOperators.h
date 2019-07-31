#ifndef NORMALOPERATORS_H
#define NORMALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

#define OPREG_NORMAL(Name, Type, Score, String, ...) \
    const std::set<DataType> Operator##Name::dataTypes_ {__VA_ARGS__}; \
    const std::string Operator##Name::str_ = String; \
    const int Operator##Name::score_ = Score; \
    const Operator::OperatorType Operator##Name::type_ = Type; \
    int globalOpReg##Name = operatorListInit(new Operator##Name());

// 定义单个运算符
#define OPDEF(Name) \
    class Operator##Name : public Operator { \
    public: \
        Operator##Name() : Operator() {} \
        ~Operator##Name() = default; \
        std::string str() {return str_;} \
        OperatorType type() {return type_;} \
        int score() { return score_; } \
        \
        int process(Node* node, MifItem* item); \
        \
        int find(Operator** newOperatorPtr, const std::string& content, \
                std::pair<size_t, size_t>* range, std::string* opName) { \
            size_t pos = content.find(str_); \
            *opName = "Operator"#Name""; \
            if (pos == std::string::npos) { \
                return -1; \
            } else { \
                range->first = pos; \
                range->second = pos + str_.length(); \
                *newOperatorPtr = new Operator##Name(); \
                return 0; \
            } \
        } \
        \
        bool isSupported(const DataType type) { \
            if (dataTypes_.find(type) != dataTypes_.end()) { \
                return true; \
            } else if (type == Number && dataTypes_.find(String) != \
                    dataTypes_.end()){ \
                return true; \
            } else { \
                return false; \
            } \
        } \
    \
    private: \
        /* 用于记录当前运算符支持的数据类型 */ \
        static const std::set<DataType> dataTypes_; \
        /* 记录了当前运算符对应的字符串 */ \
        static const std::string str_; \
        /* 用于记录当前运算符的分数 */ \
        static const int score_; \
        /* 用于记录当前运算符的类型 */ \
        static const OperatorType type_; \
    }; \

// 逻辑运算符的声明
OPDEF(Not);
OPDEF(Or);
OPDEF(And);
// 比较运算符声明
OPDEF(Equal);
OPDEF(NotEqual);
// 数字大小比较运算声明
OPDEF(LessEqual);
OPDEF(LessThan);
OPDEF(GreaterEqual);
OPDEF(GreaterThan);
// 字符串运算比较运算声明
OPDEF(Contain);
OPDEF(IsPrefix);
OPDEF(IsSuffix);
OPDEF(RegularExpr);
// Tag包含运算声明
OPDEF(TagContain);
// 地理关系运算声明
OPDEF(GeoContain);
OPDEF(GeoContainAll);
OPDEF(GeoContained);
OPDEF(GeoContainedAll);
OPDEF(GeoIntersect);
OPDEF(GeoIntersectAll);
OPDEF(GeoInContact);
OPDEF(GeoInContactAll);
OPDEF(GeoDeparture);
OPDEF(GeoDepartureAll);
// 赋值相关运算符的声明
OPDEF(Assign);
OPDEF(SelfAdd);

} // namepsace syntax

} // namespace condition_assign

#endif // NORMALOPERATORS_H
