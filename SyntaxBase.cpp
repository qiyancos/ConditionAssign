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

std::string Operator::str() const {
    return str_;
}

int Operator::score() const {
    return score_;
}

int Operator::process(Node* node, const MifItem& item) const {
    sys_log_println(_ERROR, "Operator function [process] not impelmented!");
    return -1;
}

int Operator::find(const std::string& content,
        std::pair<size_t, size_t>* range) {
    size_t pos = content.find(str_);
    if (pos != content:npos) {
        return -1;
    } else {
        range->first = pos;
        range->second = pos + str_.length();
        return 0;
    }
}

bool Operator::isSupported(DataType type) const {
    if (dataTypes_.find(type) == dataTypes_.end()) {
        return false;
    } else {
        return true;
    }
}

int operatorListInit(const Operator&& newOp) {
    operatorList.push_back(newOp);
    return globalInitTemp + 1;
}

} // namespace syntax

} // namespace condition_assign
