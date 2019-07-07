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
    enum TYPE { INVALID_TYPE, NUM_TYPE, STR_TYPE };     // ��������

    enum OPT {  // ��������
        INVALID_OPT,
        NUM_EQ, NUM_GT, NUM_LT, NUM_NE, NUM_GOE, NUM_LOE,
        STR_EQ, STR_NE, STR_START, STR_END, STR_CONTAIN, STR_REGEX,
    };

    // �������б�
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

// ��ȡ��������
Condition::OPT GetConditionOpt(const std::string& condition_str);

// ��ȡ��������
Condition::TYPE GetConditionType(Condition::OPT& opt);

// ����������
bool ParseCondition(Condition& res, const std::string& condition_str);

// ����������
bool ParseConditionGroup(ConditionGroup& res, const std::string& group_str);

// ��������Ŀ��
bool ParseTarget(Target& res, const std::string& target_str);

// ����Ŀ����
bool ParseTargetGroup(TargetGroup& res, const std::string& group_str);

// ����������
bool ParseConfigGroupByFile(ConfigList& res, const std::string& conf_file);

// ƥ������
bool MatchCondition(const std::string& value, const Condition& condition);
}

#endif // CONDITIONASSIGN_CONDITION_H__
