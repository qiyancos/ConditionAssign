#ifndef SPECIALOPERATORS_H
#define SPECIALOPERATORS_H

#include "SyntaxBase.h"

namespace condition_assign {

namespace syntax {

// 自由配置运算符内置函数的函数名与函数的映射
std::map<std::string, std::function<int(Node*, const MifItem&,
        void*)>> opInternalFuncList;
// 用于运算符内置函数注册的函数
int opInternalFuncListInit(const std::string& name,
        const std::function<int(Node*, const MifItem& item, void*)>& func);
// 用于运算符内置函数注册的宏
#define OPFUNC_REG(Name, Func) {\
    int globalInit##Name = opInternalFuncListInit(Name, Func); \
}

// 特殊的运算符的声明
class opCondFunc : public Operator {
public:
    static std::mutex funcNameListLock;
    static std::vector<std::string> funcNameList;
    static std::mutex paramListLock;
    static std::vector<std::string> paramList;
    opCondFunc() : Operator({Num, String, Group}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
};

std::mutex opCondFunc::funcNameListLock = std::mutex();
std::vector<std::string> opCondFunc::funcNameList = std::vector<std::string>();
std::mutex opCondFunc::paramListLock = std::mutex;
std::vector<std::string> opCondFunc::paramList = std::vector<std::string>();

OPREG(CondFunc);

class opAssignFunc : public Operator {
public:
    static std::mutex funcNameListLock;
    static std::vector<std::string> funcNameList;
    static std::mutex paramListLock;
    static std::vector<std::string> paramList;
    opCondFunc() : Operator({Num, String, Group}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
};

std::mutex opAssignFunc::funcNameListLock = std::mutex();
std::vector<std::string> opAssignFunc::funcNameList = std::vector<std::string>();
std::mutex opAssignFunc::paramListLock = std::mutex;
std::vector<std::string> opAssignFunc::paramList = std::vector<std::string>();

OPREG(AssignFunc);

class opReplace : public Operator {
public:
    static std::mutex paramListLock;
    static std::vector<std::pair<int, int>> paramList;
    opCondFunc() : Operator({String}, Score, String){};
    int process(Node* node, const MifItem& item);
    int find(const std::string& content, std::pair<size_t, size_t>* range);
};

std::mutex opRelace::paramListLock = std::mutex();
std::vector<std::pair<int, int>> opReplace::paramList =
        std::vector<std::pair<int, int>>();

OPREG(Replace);

} // namespace syntax

} // namespace condition_assign

#endif // SPECIALOPERATORS_H
