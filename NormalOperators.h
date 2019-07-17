#ifndef NORMALOPERATORS_H
#define NORMALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 定义单个运算符
#define OPDEF(Name, Score, String, ...) { \
    class op##Name : public Operator { \
    public: \
        op##Name() : Operator() {} \
        int score() { return score_; } \
        \
        int process(Node* node, const MifItem& item); \
        \
        int find(const std::string& content, \
                std::pair<size_t, size_t>* range) { \
            size_t pos = content.find(str_); \
            if (pos != content:npos) { \
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
        /* 记录了当前运算符的评分 */ \
        static const int score_; \
        /* 记录了当前运算符对应的字符串 */ \
        static const std::string str_; \
    }; \
    std::set<DataType> op##Name::dataTypes_ {__VA_ARGS__}; \
    int op##Name::score_ = Score; \
    std::string op##Name::str_ = String; \
    /* 注册运算符 */ \
    OPREG(Name); \
}

// 逻辑运算符的声明
OPDEF(Not, 1, "!", Expr);
OPDEF(Or, 1, "||", Expr);
OPDEF(And, 1, "&&", Expr);
// 比较运算符声明
OPDEF(Equal, 1, "==", Number, String);
OPDEF(NotEqual, 1, "!=", Number, String);
// 数字大小比较运算声明
OPDEF(LessEqual, 1, "<=", Number);
OPDEF(LessThan, 1, "<", Number);
OPDEF(GreaterEqual, 1, ">=", Number);
OPDEF(GreaterThan, 1, ">", Number);
// 字符串运算比较运算声明
OPDEF(Contain, 1, "%=%", String);
OPDEF(IsPrefix, 1, "%=", String);
OPDEF(IsSuffix, 1, "=%", String);
OPDEF(RegularExpr, 1, ":=", String);
// Tag包含运算声明
OPDEF(TagContain, 1, "=<", Group);
// 地理关系运算声明
OPDEF(GeoContain, 1, "<[]>", Group);
OPDEF(GeoContainAll, 1, "&<[]>", Group);
OPDEF(GeoContained, 1, "[<>]", Group);
OPDEF(GeoContainedAll, 1, "&[<>]", Group);
OPDEF(GeoIntersect, 1, "<[>]", Group);
OPDEF(GeoIntersectAll, 1, "&<[>]", Group);
OPDEF(GeoInContact, 1, "<[>]", Group);
OPDEF(GeoInContactAll, 1, "&<[>]", Group);
OPDEF(GeoDeparture, 1, "<>[]", Group);
OPDEF(GeoDepartureAll, 1, "&<>[]", Group);
// 赋值相关运算符的声明
OPDEF(Assign, 1, "=", Number, String);
OPDEF(SelfAdd, 1, "+=", Number, String);

} // namepsace syntax

} // namespace condition_assign

#endif // NORMALOPERATORS_H
