#ifndef NORMALOPERATORS_H
#define NORMALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 定义单个运算符
#define OPDEF(Name, Type, Score, String, ...) { \
    class op##Name : public Operator { \
    public: \
        op##Name() : Operator() {} \
        ~op##Name() == default; \
        std::string str() {return str_;} \
        OperatorType type() {return Type;} \
        int score() { return Score; } \
        \
        Operator* newOperator() { \
            return new op##Name(); \
        } \
        \
        int process(Node* node, MifItem* item); \
        \
        int find(const std::string& content, \
                std::pair<size_t, size_t>* range) { \
            size_t pos = content.find(str_); \
            if (pos != std::string::npos) { \
                return -1; \
            } else { \
                range->first = pos; \
                range->second = pos + str_.length(); \
                return 0; \
            } \
        } \
        \
        bool isSupported(DataType type) const { \
            if (dataTypes_.find(type) == dataTypes_.end()) { \
                return false; \
            } else { \
                return true; \
            } \
        } \
    \
    private: \
        /* 用于记录当前运算符支持的数据类型 */ \
        static const std::set<DataType> dataTypes_; \
        /* 记录了当前运算符对应的字符串 */ \
        static const std::string str_; \
    }; \
    std::set<DataType> op##Name::dataTypes_ {__VA_ARGS__}; \
    std::string op##Name::str_ = String; \
    /* 注册运算符 */ \
    OPREG(Name);
}

// 逻辑运算符的声明
OPDEF(Not, Condition, 1, "!", Expr);
OPDEF(Or, Condition, 1, "||", Expr);
OPDEF(And, Condtion, 1, "&&", Expr);
// 比较运算符声明
OPDEF(Equal, Condition, 1, "==", Number, String);
OPDEF(NotEqual, Condition, 1, "!=", Number, String);
// 数字大小比较运算声明
OPDEF(LessEqual, Condition, 1, "<=", Number);
OPDEF(LessThan, Condition, 1, "<", Number);
OPDEF(GreaterEqual, Condition, 1, ">=", Number);
OPDEF(GreaterThan, Condition, 1, ">", Number);
// 字符串运算比较运算声明
OPDEF(Contain, Condition, 1, "%=%", String);
OPDEF(IsPrefix, Condition, 1, "%=", String);
OPDEF(IsSuffix, Condition, 1, "=%", String);
OPDEF(RegularExpr, Condition, 1, ":=", String);
// 由于正则表达式的特殊性，提升其优先级
OPREG_PRIOR(RegularExpr);
// Tag包含运算声明
OPDEF(TagContain, Condition, 1, "=<", GroupType);
// 地理关系运算声明
OPDEF(GeoContain, Condition, 1, "<[]>", GroupType);
OPDEF(GeoContainAll, Condition, 1, "&<[]>", GroupType);
OPDEF(GeoContained, Condition, 1, "[<>]", GroupType);
OPDEF(GeoContainedAll, Condition, 1, "&[<>]", GroupType);
OPDEF(GeoIntersect, Condition, 1, "<[>]", GroupType);
OPDEF(GeoIntersectAll, Condition, 1, "&<[>]", GroupType);
OPDEF(GeoInContact, Condition, 1, "<[>]", GroupType);
OPDEF(GeoInContactAll, Condition, 1, "&<[>]", GroupType);
OPDEF(GeoDeparture, Condition, 1, "<>[]", GroupType);
OPDEF(GeoDepartureAll, Condition, 1, "&<>[]", GroupType);
// 赋值相关运算符的声明
OPDEF(Assign, Assign, 1, "=", New, Number, String);
OPDEF(SelfAdd, Assign, 1, "+=", New, Number, String);

} // namepsace syntax

} // namespace condition_assign

#endif // NORMALOPERATORS_H
