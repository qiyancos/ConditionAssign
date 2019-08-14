#include "NormalOperators.h"
#include "SpecialOperators.h"

namespace condition_assign {

namespace syntax {

std::vector<Operator*> operatorList;
std::map<std::string, Operator*> funcOperatorList;

// 下面的顺序与运算符优先级密切相关

// 逻辑运算符的声明(因为在parse时生成，所以只初始化，没有注册)
OPINIT_NORMAL(Not, Condition, 1, "!", Expr);
OPINIT_NORMAL(Or, Condition, 1, "||", Expr);
OPINIT_NORMAL(And, Condition, 1, "&&", Expr);

// 函数运算符的声明与注册
FUNCOP_REG(InRange, Condition, 1, Number, String);

// 地理关系运算声明
OPREG_NORMAL(GeoContain, Condition, 1, "<[]>", Empty, GroupType);
OPREG_NORMAL(GeoContainAll, Condition, 1, "&<[]>", Empty, GroupType);
OPREG_NORMAL(GeoContained, Condition, 1, "[<>]", Empty, GroupType);
OPREG_NORMAL(GeoContainedAll, Condition, 1, "&[<>]", Empty, GroupType);
OPREG_NORMAL(GeoIntersect, Condition, 1, "<[>]", Empty, GroupType);
OPREG_NORMAL(GeoIntersectAll, Condition, 1, "&<[>]", Empty, GroupType);
OPREG_NORMAL(GeoAtEdge, Condition, 1, "[>]<", Empty, GroupType);
OPREG_NORMAL(GeoAtEdgeAll, Condition, 1, "&[>]<", Empty, GroupType);
OPREG_NORMAL(GeoDeparture, Condition, 1, "<>[]", Empty, GroupType);
OPREG_NORMAL(GeoDepartureAll, Condition, 1, "&<>[]", Empty, GroupType);

// 正则匹配运算拥有最高的优先级
OPREG_NORMAL(RegularExpr, Condition, 1, ":=", String);
// 比较运算符声明
OPREG_NORMAL(Equal, Condition, 1, "==", New, Number, String);
OPREG_NORMAL(NotEqual, Condition, 1, "!=", New, Number, String);
// 特殊运算符注册(由于是中转函数，其类型没有关系)
OPREG_SPECIAL(Function, Assign, 1, Number, String, GroupType);
// 数字大小比较运算声明
OPREG_NORMAL(LessEqual, Condition, 1, "<=", Number);
OPREG_NORMAL(LessThan, Condition, 1, "<", Number);
OPREG_NORMAL(GreaterEqual, Condition, 1, ">=", Number);
OPREG_NORMAL(GreaterThan, Condition, 1, ">", Number);
// 字符串运算比较运算声明
OPREG_NORMAL(Contain, Condition, 1, "%=%", String);
OPREG_NORMAL(IsPrefix, Condition, 1, "%=", String);
OPREG_NORMAL(IsSuffix, Condition, 1, "=%", String);
// Tag包含运算声明
OPREG_NORMAL(TagContain, Condition, 1, "=<", GroupType);
// 赋值相关运算符的声明
OPREG_NORMAL(SelfAdd, Assign, 1, "+=", New, Number, String);
// 特殊运算符注册
OPREG_SPECIAL(Replace, Assign, 1, String);
// 尽量提升Assign等级加快解析匹配速度
OPREG_NORMAL(Assign, Assign, 1, "=", New, Number, String);

} // namespace syntax

} // namespace condition_assign
