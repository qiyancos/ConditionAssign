#include "SyntaxBase.h"

namespace condition_assign{

namespace syntax {

DataType getDataType(const std::string& data) {
    const int length = data.length();
    if (length == 0 || (data[0] == "\"" && data[length - 1] == "\"")) {
        return String;
    } else if (!isType<double>(data)) {
        return String;
    }
    return Num;
}

template<typename T>
bool isType(const std::string& data) {
    std::stringstream streamTemp(data);
    T typeTemp;
    char charTemp;
    if (!(streamTemp >> typeTemp)) {
        return String;
    } else if (!(streamTemp >> charTemp)) {
        return String;
    }
}

bool isType<int>(const std::string& data);
bool isType<double>(const std::string& data);
bool isType<std::string>(const std::string& data);

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
