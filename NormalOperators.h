#ifndef NORMALOPERATORS_H
#define NORMALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 定义单个运算符
#define OPDEF(Name, Score, String, ...) { \
    class op##Name : public Operator { \
    public: \
        op##Name() : Operator({__VA_ARGS__}, Score, String){}; \
        int process(Node* node, const MifItem& item); \
    }; \
    OPREG(Name);
}

// 逻辑运算符的声明
OPDEF(Not, 1, "!", Expr);
OPDEF(Or, 1, "||", Expr);
OPDEF(And, 1, "&&", Expr);
// 比较运算符声明
OPDEF(Equal, 1, "==", Num, String);
OPDEF(NotEqual, 1, "!=", Num, String);
// 数字大小比较运算声明
OPDEF(LessEqual, 1, "<=", Num);
OPDEF(LessThan, 1, "<", Num);
OPDEF(GreaterEqual, 1, ">=", Num);
OPDEF(GreaterThan, 1, ">", Num);
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
OPDEF(GeoDeparture, 1, "<>[]", Group);
OPDEF(GeoDepartureAll, 1, "&<>[]", Group);
// 赋值相关运算符的声明
OPDEF(Assign, 1, "=", Num, String);
OPDEF(SelfAdd, 1, "+=", Num, String);

} // namepsace syntax

} // namespace condition_assign

#endif // NORMALOPERATORS_H
