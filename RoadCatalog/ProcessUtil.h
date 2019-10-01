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

// 基础点类型
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

// 根据给定的map查找kindclass调整的map对应的kind_class
std::string AdjustmentKindClass(std::string str_AdminCode,
        std::string str_kind_class, std::string str_funcclass,
        const std::map<std::string, std::string>& kindClassAdjustMap);

// 获取道路kind对应的最大道路等级
std::string GetRoadLevelFromKind(const std::string& str_kind);

// 用于解析kindclass的配置文件
// 分隔字符串, 当分隔符包含在引号中时, 当做普通字符串
// 为支持宽字符设置的全局字符编码会造成输出数字带千分符逗号, 暂时使用string处理
bool ParseString(std::vector<std::string>& vectorItems,
        std::string strItem, std::string seprator, bool botrimseperator);

// 获取两个点之间的距离
double GetPointDistance(const Point2& pt1, const Point2& pt2);

// 获取某一个link的长度
double GetLinkLength(wsl::Geometry* geoLine);

// 根据是否有双引号提取字符串数值(去除字符串的前后双引号)
std::string fixQuotMain2(std::string &str);

// 判断一个字符串是否是一个数字(只是判断全数字，s给定判断长度)
bool IsNum(const std::string& str, size_t s = 0,
        size_t len = std::string::npos);

// 依据道路名称判断是否合法
bool isValidBrandName(const std::string& name, const char type);

// 传入sw的kind，判断当前是否为匝道(IC、JCT、匝道、
// 提前左转、提前右转、掉头口、主辅出入口
bool isRamp(const std::string& str_kind);

// 判断是否是一个JCT连接路
bool isJCT(const std::string& str_kind);

// 判断是否是一个IC/JCT连接路
bool isICJCT(const std::string& str_kind);

// 判断是否仅仅是一个匝道
bool isOnlyRamp(const std::string& str_kind);

// 是否是一个服务区
bool isSA(const std::string& str_kind);

// 是否是交叉点内link
bool isRLink(const std::string& str_kind);

// 是否是隧道
bool isTunnel(const std::string& str_kind);

// 是否是桥
bool isBridge(const std::string& str_kind);

// 是否是辅路
bool isFulu(const std::string& str_kind);

// 是否是主辅路出入口
bool isZhuFuluConnected(const std::string& str_kind);

// 是否是步行街
bool isWalkStreet(const std::string& str_kind);

// 判断两个link的king是否相似
bool isSameKind(const std::string& str_kind1, const std::string& str_kind2,
        const std::string& ignoreAttr);

// 判断两个道路的道路等级是否一致
bool isSameKindClass(string str_kind1, string str_kind2);

// 判断道路是否可以删除
bool needDelete(const std::string&& str_kind);

} // namespace process_util

#endif // PROCESSUTIL_H
