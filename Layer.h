#pragma once
#ifndef CONDITIONASSIGN_LAYER_H__
#define CONDITIONASSIGN_LAYER_H__

#include <vector>
#include <string>
#include <type_factory.h>
#include "Config.h"


namespace basemap
{
class Layer
{
public:
    wgt::MIF    mif_;

public:
    Layer() {}
    virtual ~Layer() {}

    // 加载图层
    bool Load(const std::string& in_file);

    // 保存图层
    bool Save(const std::string& out_file);

    // 根据配置列表设置设置字段值, 这里的ConfigList对应整个配置列表
    bool AssignByConfigList(const ConfigList& config_list, int mif_idx);

    // 获取MIF字段索引(原生MIF.get_col_pos效率比较低, 这里单独实现一个简化版)
    int GetMifColPos(const std::string& lower_col_name);
    int GetOrAddMifPos(const std::string& lower_col_name,
        const std::string& col_type = "char(64)");

private:

    // 匹配条件域, 这里的ConditionGroup对应单行配置中的条件组
    bool _MatchConditionGroup(const ConditionGroup& condition_group, size_t mif_idx);

    // 目标域赋值, 这里的TargetGroup对应单行配置中的目标组
    bool _AssignTargetGroup(const TargetGroup& target_group, size_t mif_idx);

};


}


#endif // CONDITIONASSIGN_LAYER_H__
