#ifndef SPECIALOPERATORS_H
#define SPECIALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 特殊的运算符的声明
class opCondFunc : public Operator {
public:
    opCondFunc() : Operator({Num, String, Group}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
private:
    std::function<int(Node*, void*)> func_;
};

OPREG(CondFunc);

class opAssignFunc : public Operator {
public:
    opCondFunc() : Operator({Num, String, Group}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
private:
    std::function<int(Node*, void*)> func_;
};

OPREG(AssignFunc);

class opReplace : public Operator {
public:
    opCondFunc() : Operator({String}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
private:
    int startIdx_, endIdx_;
};

OPREG(Replace);

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
