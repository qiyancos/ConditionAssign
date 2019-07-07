#pragma once
#ifndef CONDITIONASSIGN_CONDITION_H__
#define CONDITIONASSIGN_CONDITION_H__

#include <string>
#include <vector>
#include <tuple>

namespace basemap
{
class Condition
{
public:
    enum TYPE { INVALID_TYPE, NUM_TYPE, STR_TYPE };     // 条件类型

    enum OPT {  // 操作类型
        INVALID_OPT,
        NUM_EQ, NUM_GT, NUM_LT, NUM_NE, NUM_GOE, NUM_LOE,
        STR_EQ, STR_NE, STR_START, STR_END, STR_CONTAIN, STR_REGEX,
    };

    // 操作符列表
    typedef std::tuple<OPT, std::string> opt_tuple_t;
    static const std::vector< opt_tuple_t >  NUM_OPT_SYMBOL;
    static const std::vector< opt_tuple_t >  STR_OPT_SYMBOL;

public:
    std::string     key_;
    std::string     value_str_;
    double          value_double_;
    TYPE            type_;
    OPT             opt_;

public:
    Condition();
    virtual ~Condition();
};

typedef std::vector<Condition> ConditionGroup;

class Target
{
public:
    std::string     field_;
    std::string     value_;

public:
    Target();
    virtual ~Target();
};

typedef std::vector<Target> TargetGroup;


class ConfigItem
{
public:
    ConditionGroup condition_group_;
    TargetGroup target_group_;

public:
    ConfigItem() {}
    virtual ~ConfigItem() {}
};

typedef std::vector<ConfigItem> ConfigList;
// ---

// 获取操作类型
Condition::OPT GetConditionOpt(const std::string& condition_str);

// 获取条件类型
Condition::TYPE GetConditionType(Condition::OPT& opt);

// 解析单条件
bool ParseCondition(Condition& res, const std::string& condition_str);

// 解析条件组
bool ParseConditionGroup(ConditionGroup& res, const std::string& group_str);

// 解析单个目标
bool ParseTarget(Target& res, const std::string& target_str);

// 解析目标组
bool ParseTargetGroup(TargetGroup& res, const std::string& group_str);

// 解析配置组
bool ParseConfigGroupByFile(ConfigList& res, const std::string& conf_file);

// 匹配条件
bool MatchCondition(const std::string& value, const Condition& condition);
}

#endif // CONDITIONASSIGN_CONDITION_H__
