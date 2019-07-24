#include "SyntaxBase.h"
#include "ConditionAssign.h"

namespace condition_assign{

namespace syntax {

DataType getDataType(const std::string& data, std::string* stringVal,
        double* numberVal) {
    const int length = data.length();
    if (length == 0 || (data[0] == "\"" && data[length - 1] == "\"")) {
        if (stringVal != nullptr) {
            *stringVal = data.substr(1, length - 2);
        }
        return String;
    } else {
        if (stringVal != nullptr) {
            *stringVal = data;
        }
        if (!isType<double>(data, numberVal)) {
            return String
        } else {
            return Number;
        }
    }
}

template<typename T>
bool isType(const std::string& data, T* result) {
    std::stringstream streamTemp(data);
    T typeTemp;
    char charTemp;
    T* typeTempPtr = result == nullptr ? &typeTemp : result;
    if (!(streamTemp >> *typeTempPtr)) {
        return String;
    } else if (!(streamTemp >> charTemp)) {
        return String;
    }
    return Number;
}

bool isType<int>(const std::string& data, int* result);
bool isType<double>(const std::string& data, int* result);
bool isType<std::string>(const std::string& data, int* result);

int operatorListInit(const Operator* newOp) {
    operatorList.push_back(newOp);
    return 0;
}

int calculateScore(const std::vector<Node*> nodeVec) {
}

bool satisfyConditions(const std::vector<Node*>& conditions, MifItem* item) {
}

} // namespace syntax

} // namespace condition_assign
