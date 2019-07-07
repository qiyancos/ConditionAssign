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

    // ����ͼ��
    bool Load(const std::string& in_file);

    // ����ͼ��
    bool Save(const std::string& out_file);

    // ���������б����������ֶ�ֵ, �����ConfigList��Ӧ���������б�
    bool AssignByConfigList(const ConfigList& config_list, int mif_idx);

    // ��ȡMIF�ֶ�����(ԭ��MIF.get_col_posЧ�ʱȽϵ�, ���ﵥ��ʵ��һ���򻯰�)
    int GetMifColPos(const std::string& lower_col_name);
    int GetOrAddMifPos(const std::string& lower_col_name,
        const std::string& col_type = "char(64)");

private:

    // ƥ��������, �����ConditionGroup��Ӧ���������е�������
    bool _MatchConditionGroup(const ConditionGroup& condition_group, size_t mif_idx);

    // Ŀ����ֵ, �����TargetGroup��Ӧ���������е�Ŀ����
    bool _AssignTargetGroup(const TargetGroup& target_group, size_t mif_idx);

};


}


#endif // CONDITIONASSIGN_LAYER_H__
