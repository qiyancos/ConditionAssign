#include "NormalOperators.h"
#include "SpecialOperators.h"

namespace condition_assign {

namespace syntax {

std::vector<Operator*> operatorList;
std::map<std::string, Operator*> funcOperatorList;

// 下面的顺序与运算符优先级密切相关

// 逻辑运算符的声明(因为在parse时生成，所以只初始化，没有注册)
OPINIT_NORMAL(Not, Condition, "!", Expr);
OPINIT_NORMAL(Or, Condition, "||", Expr);
OPINIT_NORMAL(And, Condition, "&&", Expr);

// 函数运算符的声明与注册
FUNCOP_REG(InRange, Condition, Number, String);

// 地理关系运算声明(极高优先级防止匹配group内的运算符)
OPREG_NORMAL(GeoContain, Condition, "<[]>", Empty, GroupType);
OPREG_NORMAL(GeoContainAll, Condition, "&<[]>", Empty, GroupType);
OPREG_NORMAL(GeoContained, Condition, "[<>]", Empty, GroupType);
OPREG_NORMAL(GeoContainedAll, Condition, "&[<>]", Empty, GroupType);
OPREG_NORMAL(GeoIntersect, Condition, "<[>]", Empty, GroupType);
OPREG_NORMAL(GeoIntersectAll, Condition, "&<[>]", Empty, GroupType);
OPREG_NORMAL(GeoAtEdge, Condition, "[>]<", Empty, GroupType);
OPREG_NORMAL(GeoAtEdgeAll, Condition, "&[>]<", Empty, GroupType);
OPREG_NORMAL(GeoDeparture, Condition, "<>[]", Empty, GroupType);
OPREG_NORMAL(GeoDepartureAll, Condition, "&<>[]", Empty, GroupType);

// 正则匹配运算拥有最高的优先级
OPREG_NORMAL(RegularExpr, Condition, ":=", String);
// 比较运算符声明
OPREG_NORMAL(Equal, Condition, "==", New, Number, String);
OPREG_NORMAL(NotEqual, Condition, "!=", New, Number, String);
// 特殊运算符注册(由于是中转函数，其类型没有关系)
OPREG_SPECIAL(Function, Assign, Number, String, GroupType);
// 数字大小比较运算声明
OPREG_NORMAL(LessEqual, Condition, "<=", Number);
OPREG_NORMAL(LessThan, Condition, "<", Number);
OPREG_NORMAL(GreaterEqual, Condition, ">=", Number);
OPREG_NORMAL(GreaterThan, Condition, ">", Number);
// 字符串运算比较运算声明
OPREG_NORMAL(Contain, Condition, "%=%", String);
OPREG_NORMAL(IsPrefix, Condition, "%=", String);
OPREG_NORMAL(IsSuffix, Condition, "=%", String);
// Tag包含运算声明
OPREG_NORMAL(TagContain, Condition, "=<", GroupType);
// 赋值相关运算符的声明
OPREG_NORMAL(SelfAdd, Assign, "+=", New, Number, String);
// 特殊运算符注册
OPREG_SPECIAL(PartialEqual, Condition, String);
OPREG_SPECIAL(Replace, Assign, String);
// 尽量提升Assign等级加快解析匹配速度
OPREG_NORMAL(Assign, Assign, "=", New, Number, String);

} // namespace syntax

} // namespace condition_assign
