#ifndef PROCESSUTIL_H
#define PROCESSUTIL_H
#include <string>
#include <vector>
#include <map>
#include "type_factory.h"
#include "spatial-base.h"
#include "tx_platform.h"
#include "tx_common.h"

namespace process_util {

struct Point2{
    double x;
    double y;
};

// 加载kindclass的相关配置文件
bool LoadKindClassAdjust(const std::string& configPath,
        std::map<std::string, std::string>* kingClassAdjustMap);

// 加载MAPID到目录的映射配置文件
void LoadMapid2dirConf(const std::string& configPath,
        std::map<std::string, std::vector<std::string>>* mapid2dirvec);

// TODO
string AdjustmentKindClass(string str_AdminCode, string str_kind_class, string str_funcclass);
std::string GetRoadLevelFromKind(const std::string& str_kind);
bool ParseString(vector<string>& vectorItems, string strItem, string seprator, bool botrimseperator);
double GetPointDistance(const Point2& pt1, const Point2& pt2);
double GetLinkLength(wsl::Geometry* geoLine);
std::string fix_quot_main2(std::string &str);

bool IsNum(const string& str, size_t s = 0, size_t len = string::npos);
template <char type>
bool isValidBrandName(std::string name);
bool isRamp(string str_kind);
bool isJCT(string str_kind);
bool isICJCT(string str_kind);
bool isOnlyRamp(string str_kind);
bool isSA(string str_kind);
bool isRLink(string str_kind);
bool isTunnel(string str_kind);
bool isBridge(string str_kind);
bool isFulu(string str_kind);
bool isZhuFuluConnected(string str_kind);
bool isWalkstreet(string str_kind);
bool isSameKind(string str_kind1, string str_kind2, string ignoreAttr);
bool isSameKindClass(string str_kind1, string str_kind2);
bool needDelete(string str_kind);

} // namespace process_util

#endif // PROCESSUTIL_H
