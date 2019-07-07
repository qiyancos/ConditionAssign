#include "Layer.h"
#include <tx_common.h>

using namespace std;

namespace basemap
{
bool Layer::Load(const std::string& in_file)
{
    if (in_file.empty()) return false;

    int status = wgt::mif_to_wsbl(in_file, mif_);
    return status >= 0;
}

bool Layer::Save(const std::string& out_file)
{
    if (out_file.empty()) return false;

    int status = wgt::wsbl_to_mif(mif_, out_file);
    return status >= 0;
}

bool Layer::AssignByConfigList(const ConfigList& config_list, int mif_idx)
{
    if (mif_idx < 0 || mif_idx >= mif_.mid.size()) {
        return false;
    }

    for (const ConfigItem& conf : config_list) {
        if (_MatchConditionGroup(conf.condition_group_, mif_idx)) {
            _AssignTargetGroup(conf.target_group_, mif_idx);
            return true;
        }
    }
    return false;
}

bool Layer::_MatchConditionGroup(const ConditionGroup& condition_group, size_t mif_idx)
{
    for (const Condition& condition : condition_group) {
        int pos = GetMifColPos(condition.key_);
        if (pos < 0) return false;

        string mif_value = mif_.mid[mif_idx][pos];
        wgt::trim(mif_value, '"');
        if (!MatchCondition(mif_value, condition)) {
            return false;
        }
    }
    return true;
}

bool Layer::_AssignTargetGroup(const TargetGroup& target_group, size_t mif_idx)
{
    for (const Target& target : target_group) {
        int pos = GetOrAddMifPos(target.field_, "Char(64)");
        mif_.mid[mif_idx][pos] = "\"" + target.value_ + "\"";
    }
    return true;
}

int Layer::GetMifColPos(const std::string& lower_col_name)
{
    auto finder = mif_.header.col_name_map.find(lower_col_name);
    if (finder == mif_.header.col_name_map.end()) {
        return -1;
    }

    return finder->second;
}

int Layer::GetOrAddMifPos(const string& lower_col_name, const string& col_type)
{
    auto finder = mif_.header.col_name_map.find(lower_col_name);
    if (finder == mif_.header.col_name_map.end()) {
        mif_.add_column(lower_col_name, col_type);
        return GetMifColPos(lower_col_name);
    }
    return finder->second;
}
}
