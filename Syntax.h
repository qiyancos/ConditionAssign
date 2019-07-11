#ifndef SYNTAX_H_
#define SYNTAX_H

#include <string>
#include <vector>
#include <tuple>
#include "Group.h"

namespace condition_assign {

namespace syntax {

enum DataType {Num, String, Group, Expr};

class Operator {
public:
    Operator(const std::vector<Datatype>&& dataTypes, const int scoreIn,
            const std::string&& str);
    std::string str() const {
        return str_;
    }
    int score() const {
        return score_;
    }
private:
    const int dataTypes_;
    const int score_;
    const std::string str_;
};

struct Node {
    DataType type;
    Node* leftNode, rightNode;
    std::string tagName;
    syntax::Operator& op;
    union value {
        Group* group;
        int number;
        std::string string;
    };
};

const Operator opNot ({Expr}, 1, "!");
const Operator opOr ({Expr}, 1, "||");
const Operator opAnd ({Expr}, 1, "&&");
const Operator opEqual ({Num, String}, 1, "==");
const Operator opNotEqual ({Num, String}, 1, "!=");
const Operator opLessEqual ({Num}, 1, "<=");
const Operator opLessThan ({Num}, 1, "<");
const Operator opGreaterEqual ({Num}, 1, ">=");
const Operator opGreaterThan ({Num}, 1, ">");
const Operator opContain ({String}, 1, "%=%");
const Operator opIsPrefix ({String}, 1, "%=");
const Operator opIsSuffix ({String}, 1, "=%");
const Operator opRegularExpr ({String}, 1, ":=");
const Operator opTagContain ({Group}, 1, "=<");
const Operator opGeoContain ({Group}, 1, "<[]>");
const Operator opGeoContainAll ({Group}, 1, "&<[]>");
const Operator opGeoContained ({Group}, 1, "[<>]");
const Operator opGeoContainedAll ({Group}, 1, "&[<>]");
const Operator opGeoIntersect ({Group}, 1, "<[>]");
const Operator opGeoIntersectAll ({Group}, 1, "&<[>]");
const Operator opGeoDeparture ({Group}, 1, "<>[]");
const Operator opGeoDepartureAll ({Group}, 1, "&<>[]");

const std::vector<Operator&> operatorList {opNot, opOr, opAnd, opEqual,
        opNotEqual, opLessEqual, opLessThan, opGreaterEqual, opGreaterThan,
        opContain, opIsPrefix, opRegularExpr, opTagContain, opGeoContain,
        opGeoContainAll, opGeoContained, opGeoContainedAll, opGeoIntersect,
        opGeoIntersectAll, opGeoDeparture, opGeoDepartureAll};

} // namesapce syntax

} // namespace condition_assign

#endif // SYNTAX_H
