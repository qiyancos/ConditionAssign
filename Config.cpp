#include "Config.h"
#include <htk/str_helpers.h>
#include <htk/file_helpers.h>
#include <iostream>
#include <cmath>

using namespace std;

#define ZERO_VALUE (1e-8)
#define IS_DOUBLE_ZERO(d)  (abs(d) < ZERO_VALUE)

namespace basemap
{
const vector< Condition::opt_tuple_t > Condition::NUM_OPT_SYMBOL = {
    Condition::opt_tuple_t(Condition::NUM_EQ, "=="),
    Condition::opt_tuple_t(Condition::NUM_NE, "<>"),
    Condition::opt_tuple_t(Condition::NUM_GOE, ">="),
    Condition::opt_tuple_t(Condition::NUM_LOE, "<="),
    Condition::opt_tuple_t(Condition::NUM_GT, ">"),
    Condition::opt_tuple_t(Condition::NUM_LT, "<")
};

const vector< Condition::opt_tuple_t > Condition::STR_OPT_SYMBOL = {
    Condition::opt_tuple_t(Condition::STR_CONTAIN, "%=%"),
    Condition::opt_tuple_t(Condition::STR_START, "%="),
    Condition::opt_tuple_t(Condition::STR_END, "=%"),
    Condition::opt_tuple_t(Condition::STR_REGEX, ":="),
    Condition::opt_tuple_t(Condition::STR_NE, "!="),
    Condition::opt_tuple_t(Condition::STR_EQ, "=")
};

Condition::Condition() :
    key_(""),
    value_str_(""),
    value_double_(0),
    type_(INVALID_TYPE),
    opt_(INVALID_OPT)
{
}

Condition::~Condition()
{
}

Condition::OPT GetConditionOpt(const std::string& condition_str)
{
    for (auto& opt_tuple : Condition::NUM_OPT_SYMBOL) {
        if (condition_str.find(get<1>(opt_tuple)) != string::npos) {
            return get<0>(opt_tuple);
        }
    }

    for (auto& opt_tuple : Condition::STR_OPT_SYMBOL) {
        if (condition_str.find(get<1>(opt_tuple)) != string::npos) {
            return get<0>(opt_tuple);
        }
    }

    return Condition::INVALID_OPT;
}

Condition::TYPE GetConditionType(Condition::OPT& opt)
{
    for (auto& opt_tuple : Condition::NUM_OPT_SYMBOL) {
        if (get<0>(opt_tuple) == opt) {
            return Condition::NUM_TYPE;
        }
    }

    for (auto& opt_tuple : Condition::STR_OPT_SYMBOL) {
        if (get<0>(opt_tuple) == opt) {
            return Condition::STR_TYPE;
        }
    }

    return Condition::INVALID_TYPE;
}

bool ParseCondition(Condition& res, const std::string& condition_str)
{
    res.opt_ = GetConditionOpt(condition_str);
    res.type_ = GetConditionType(res.opt_);
    if (Condition::NUM_TYPE == res.type_) {
        for (auto& opt_tuple : Condition::NUM_OPT_SYMBOL) {
            if (res.opt_ == get<0>(opt_tuple)) {
                vector<string> kv = htk::split(condition_str, get<1>(opt_tuple), " ");
                if (2 != kv.size()) return false;
                res.key_ = htk::toLower(htk::trim(kv[0], "\""));
                res.value_double_ = htk::ConvertFromString<double>(htk::trim(kv[1], "\""));
                return true;
            }
        }
    } else if (Condition::STR_TYPE == res.type_) {
        for (auto& opt_tuple : Condition::STR_OPT_SYMBOL) {
            if (res.opt_ == get<0>(opt_tuple)) {
                vector<string> kv = htk::split(condition_str, get<1>(opt_tuple), " ");
                if (2 != kv.size()) return false;
                res.key_ = htk::trim(kv[0], "\"");
                res.value_str_ = htk::trim(kv[1], "\"");
                return true;
            }
        }
    }

    return false;
}

bool ParseConditionGroup(ConditionGroup& res, const std::string& group_str)
{
    vector<string> vec_str = htk::split_with_quot(group_str, ';', '"', " ");
    for (string& str : vec_str) {
        Condition c;
        if (ParseCondition(c, str)) {
            res.push_back(c);
        } else {
            return false;
        }
    }
    return true;
}

bool ParseTarget(Target& res, const std::string& target_str)
{
    vector<string> kv = htk::split(target_str, "=", " ");
    if (2 == kv.size()) {
        res.field_ = htk::toLower(htk::trim(kv[0], "\""));
        res.value_ = htk::trim(kv[1], "\"");
        return true;
    }
    return false;
}

bool ParseTargetGroup(TargetGroup& res, const std::string& group_str)
{
    vector<string> vec_str = htk::split_with_quot(group_str, ';', '"', " ");
    for (string& str : vec_str) {
        Target t;
        if (ParseTarget(t, str)) {
            res.push_back(t);
        } else {
            return false;
        }
    }
    return true;
}

bool ParseConfigGroupByFile(ConfigList& res, const std::string& conf_file)
{
    res.clear();
    vector< vector<string> > vbuff;
    if (htk::SimpleReadCSV(conf_file, vbuff, "\t")) {
        for (auto& row : vbuff) {
            if (row.size() < 2) continue;

            ConfigItem ci;
            if (ParseConditionGroup(ci.condition_group_, row[0]) &&
                ParseTargetGroup(ci.target_group_, row[1])) {
                res.push_back(ci);
            }
        }
    }
    return true;
}

bool MatchCondition(const std::string& value, const Condition& condition)
{
    if (Condition::STR_EQ == condition.opt_) {
        return value == condition.value_str_;
    } else if (Condition::STR_NE == condition.opt_) {
        return value != condition.value_str_;
    } else if (Condition::STR_CONTAIN == condition.opt_) {
        return value.find(condition.value_str_) != string::npos;
    } else if (Condition::STR_START == condition.opt_) {
        return htk::startswith(value, condition.value_str_);
    } else if (Condition::STR_END == condition.opt_) {
        return htk::endswith(value, condition.value_str_);
    } else if (Condition::STR_REGEX == condition.opt_) {
        return htk::RegexSearch(value, condition.value_str_);
    } else if (Condition::NUM_TYPE == condition.type_) {
        // 数值型条件
        if (value.empty()) return false;
        double d_value = atof(value.c_str());
        double diff = d_value - condition.value_double_;

        if (Condition::NUM_EQ == condition.opt_) {
            return IS_DOUBLE_ZERO(diff);
        } else if (Condition::NUM_GOE == condition.opt_) {
            return IS_DOUBLE_ZERO(diff) || d_value > condition.value_double_;
        } else if (Condition::NUM_GT == condition.opt_) {
            return d_value > condition.value_double_;
        } else if (Condition::NUM_LT == condition.opt_) {
            return d_value < condition.value_double_;
        } else if (Condition::NUM_LOE == condition.opt_) {
            return IS_DOUBLE_ZERO(diff) || d_value < condition.value_double_;
        } else if (Condition::NUM_NE == condition.opt_) {
            return !IS_DOUBLE_ZERO(diff);
        }
    }

    return false;
}

Target::Target() :
    field_(""),
    value_("")
{
}

Target::~Target()
{
}
}

