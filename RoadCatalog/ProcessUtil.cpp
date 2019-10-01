#include "ProcessUtil.h"
#include <stdio.h>  
#include <string.h>  
#include <queue>
#include <cctype>

namespace process_util {

using namespace wgt;

bool ConfigCatalog::LoadKindClassAdjust(const string& configPath,
        std::map<std::string, std::string>* kindClassAdjustMap) {
    str_t file_name = configPath + "/kindclass_adjust.txt";
    str_t strline;
    std::vector<str_t> splitItems;
    std::ifstream infile(file_name.data());
    while (infile.good() && !infile.eof()) {
        strline.clear();
        getline(infile, strline);
        if (strline.empty()) {
            continue;
        }
        strline = wgt::trim(strline, " ");
        strline = wgt::trim_right(strline, "\r");
        if (strline.at(0) == '#') {
            continue;
        }
        sys_splitString(strline, splitItems, ',');
        if (splitItems.size() < 4) {
            continue;
        }
        (*kindClassAdjustMap)[splitItems[0] + "_" + splitItems[1] + "_" +
                splitItems[2]] = splitItems[3];
    }
    return true;
}

void LoadMapid2dirConf(const std::string configPath,
        std::map<std::string, std::vector<std::string>>* mapid2dirvec) {
    std::ifstream confFile(configPath.c_str());
    if (confFile.is_open() == false) {
        return;
    }
    std::string line;
    while (confFile.good() && confFile.eof() == false) {
        getline(confFile, line);
        line = wgt::trim(line, " ");
        //���Ϊ�ջ���#��ͷ  
        if (line.empty() || wgt::startswith(line, "#")) {
            continue;
        }
        std::vector<std::string> splitItems;
        bool parseReult = ParseString(splitItems, line, "\t", false);
        if (parseReult == false || splitItems.size() < 2) {
            continue;
        }
        int itemSize = splitItems.size();
        std::string mapid = splitItems[0];
        std::vector<std::string> dirvec;
        for (int i = 1; i < itemSize; i++)
        {
            dirvec.push_back(splitItems[i]);
        }
        (*mapid2dirvec)[mapid] = dirvec;
    }
    confFile.close();
}

std::string AdjustmentKindClass(std::string str_AdminCode,
        std::string str_kind_class, std::string str_funcclass,
        const std::map<std::string, std::string>& kindClassAdjustMap) {
    str_AdminCode = str_AdminCode.substr(0, 4) + "00";
    std::map<std::string, std::string>::iterator iter =
            kindClassAdjustMap.find(str_AdminCode + "_" + str_kind_class +
            "_" + str_funcclass);
    if (iter != kindClassAdjustMap.end()) {
        str_kind_class = iter->second;
    } else {
        iter = kindClassAdjustMap.find("_" + str_kind_class + "_" +
                str_funcclass);
        if (iter != kindClassAdjustMap.end()) {
            str_kind_class = iter->second;
        }
    }
    return str_kind_class;
}

std::string GetRoadLevelFromKind(std::string str_kind) {
    int level = 100;
    std::string strlevel = "";
    std::vector<string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (const std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string roadLevel = kind.substr(0, 2);
        int curlevel = (int)strtol(roadLevel.c_str(), NULL, 16);
        if (curlevel < level) {
            level = curlevel;
            strlevel = roadLevel;
        }
    }
    return strlevel;
}

bool ParseString(std::vector<std::string> &vectorItems,
        std::string strItem, std::string seprator, bool botrimSeperator) {
    strItem = wgt::trim(strItem, " ");
    if (strItem.empty() == true) {
        return false;
    }

    unsigned start = 0;
    bool quot_status = false;
    std::string sub_str;
    for (size_t i = 0; i < strItem.size(); ++i) {
        if (strItem[i] == '"') {
            // ʶ������, �л�����״̬ 
            quot_status ^= true;
        }

        if (!quot_status && (strItem[i] == seprator[0])) {
            // ������״̬�²�ʶ��ָ���
            sub_str = strItem.substr(start, i - start);
            if (botrimSeperator) {
                sub_str = wgt::trim(sub_str, seprator);
                if (!sub_str.empty()) {
                    vectorItems.push_back(sub_str);
                }
            } else {
                vectorItems.push_back(sub_str);
            }
            start = i + 1;
        }

        // ��β����
        if ((i == strItem.size() - 1) && (strItem[i] != seprator[0])) {
            sub_str = strItem.substr(start, i + 1 - start);
            if (botrimSeperator) {
                sub_str = wgt::trim(sub_str, seprator);
                if (!sub_str.empty()) {
                    vectorItems.push_back(sub_str);
                }
            } else {
                vectorItems.push_back(sub_str);
            }
        }
    }

    return vectorItems.size() > 0;
}

double GetPointDistance(const Point2& pt1, const Point2& pt2) {
    return sqrt((pt2.x - pt1.x) * (pt2.x - pt1.x) + (pt2.y - pt1.y) *
            (pt2.y - pt1.y));
}

double GetLinkLength(wsl::Geometry* geoLine) {
    //wsl::Geometry* geo_ptr = rtic_mif.data.geo_vec[idx];
    wsl::Line& line_feat = ((wsl::Feature<wsl::Line>*)geoLine)->at(0);
    int iSizeLine = line_feat.size();
    vector<Point2> vecPts;
    for (int i = 0; i < iSizeLine; i++) {
        const wsl::Point& pnt = line_feat.at(i);
        wsl::coor::dpoint_t ll_pt(pnt.x(), pnt.y());
        wsl::coor::dpoint_t meter_pnt = wsl::coor::ll2mc(ll_pt);

        Point2 pt;
        pt.x = meter_pnt.x;
        pt.y = meter_pnt.y;
        vecPts.push_back(pt);
    }

    double lineLength = 0;
    for (int i = 0; i + 1 < iSizeLine; i++) {
        lineLength += GetPointDistance(vecPts[i], vecPts[i + 1]);
    }
    return lineLength;
}

std::string fixQuotMain2(std::string& str) {
    if (str.empty()) {
        return str;
    }
    if (str[0] == '\"' && str[str.size() - 1] == '\"') {
        str = str.substr(1, str.length() - 2);
    }
    return str;
}

bool IsNum(const std::string& str, size_t s = 0, size_t len) {
    size_t size = ((len == string::npos) || (len + s > str.size())) ?
            str.size() : len + s;
    for (size_t index = s; index < size; index++) {
        if (!isdigit(str.at(index))) {
            return false;
        }
    }
    return true;
}

bool isValidBrandName(const std::string& name, const char type) {
    if (name[1] != type) return false;
    int start = 2, size = name.size() - 3;
    if (type == 'X' || type == 'Y') {
        if (isupper(name[start])) {
            ++start;
            --size;
        }
        if (isupper(name[start])) {
            ++start;
            --size;
        }
    }
    if (type == 'G') {
        if (name[start] == 'X' || name[start] == 'V') {
            ++start;
            --size;
        }
    }
    return (IsNum(name, start, size));
}

bool isRamp(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    std::map<std::string, std::string> kind2map;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        kind2map[rattr] = "";
        if (rattr == "0b"    // �ѵ�
            || rattr == "12"    // ��ǰ��ת
            || rattr == "15"    // ��ǰ��ת
            || rattr == "16"    // ��ͷ��
            || rattr == "17"    // ����·�����
            ) {
            return true;
        }
    }
    // IC��JCT��Ҫͬʱ����: [03��0b��03��17��JCT����·��]
    // �� [05��0b��05��17��IC����·��]
    if (kind2map.find("03") != kind2map.end() &&
            (kind2map.find("0b") != kind2map.end() ||
            kind2map.find("17") != kind2map.end())) {
        return true;
    }
    if (kind2map.find("05") != kind2map.end() &&
            (kind2map.find("0b") != kind2map.end() ||
            kind2map.find("17") != kind2map.end())) {
        return true;
    }
    // IC�ҷǷ�գ���Ϊ���ѵ�
    if (kind2map.find("05") != kind2map.end() &&
            kind2map.find("0c") == kind2map.end()) {
        return true;
    }
    return false;
}

bool isJCT(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    std::map<std::string, std::string> kind2map;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        kind2map[rattr] = "";
    }
    // IC��JCT��Ҫͬʱ����: [03��0b��03��17��JCT����·��] ��
    // [05��0b��05��17��IC����·��]
    if (kind2map.find("03") != kind2map.end() &&
            (kind2map.find("0b") != kind2map.end() ||
            kind2map.find("17") != kind2map.end())) {
        return true;
    }
    return false;
}

bool isICJCT(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        std::string kind = *iter;
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "05"            // IC
            || rattr == "03")        // JCT
        {
            return true;
        }
    }
    return false;
}

bool isOnlyRamp(std::string str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        std::string kind = *iter;
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "0b") {    // �ѵ�
            return true;
        }
    }
    return false;
}

bool isSA(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "07"        // ������
            || rattr == "06" ) {    // ͣ����
            return true;
        }
    }
    return false;
}

bool isRLink(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "04") {       // �������link
            return true;
        }
    }
    return false;
}

bool isTunnel(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "0f") {       // ���
            return true;
        }
    }
    return false;
}

bool isBridge(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "08" ||  // �̶���
            rattr == "1b") {       // �ƶ�ʽ��
            return true;
        }
    }
    return false;
}

bool isFulu(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "0a") {       // ��·
            return true;
        }
    }
    return false;
}

bool isZhuFuluConnected(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "17") {       // ����·�����
            return true;
        }
    }
    return false;
}

bool isWalkStreet(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(0, 2);
        std::string rattr2 = kind.substr(2, 2);
        // SW�Ὣ�󲿷��ֶɵ�kind����Ϊ"0a09"
        if (rattr != "0a" && rattr2 == "09") {       // ���н�
            return true;
        }
    }
    return false;
}

bool isSameKind(const std::string& str_kind1, const std::string& str_kind2,
        const std::string& ignoreAttr) {
    if (!isSameKindClass(str_kind1, str_kind2)) {
        return false;
    }
    std::vector<std::string> kindvec1;
    std::vector<std::string> kindvec2;
    sys_splitString(str_kind1, kindvec1, '|');
    sys_splitString(str_kind2, kindvec2, '|');

    std::set<std::string> kindset1;
    for (std::string& kind1 : kindvec1) {
        if (kind1.size() != 4) {
            continue;
        }
        std::string rattr1 = kind1.substr(2, 2);
        if (rattr1 == ignoreAttr && rattr1 == "01") {      // ���Եĵ�·����
            continue;
        }
        kindset1.insert(kind1);
    }

    std::set<std::string> kindset2;
    for (std::string& kind2 : kindvec2) {
        if (kind2.size() != 4)
        {
            continue;
        }
        std::string rattr2 = kind2.substr(2, 2);
        if (rattr2 == ignoreAttr && rattr2 == "01") {      // ���Եĵ�·����
            continue;
        }
        kindset2.insert(kind2);
    }

    return kindset1 == kindset2;
}

bool isSameKindClass(const std::string& str_kind1,
        const std::string& str_kind2) {
    return GetRoadLevelFromKind(str_kind1) == GetRoadLevelFromKind(str_kind2);
}

bool needDelete(const std::string& str_kind) {
    std::vector<std::string> kindvec;
    sys_splitString(str_kind, kindvec, '|');
    for (std::string& kind : kindvec) {
        if (kind.size() != 4) {
            continue;
        }
        std::string rattr = kind.substr(2, 2);
        if (rattr == "18") {          // ��������·
            return true;
        }
    }
    return false;
}

} // namespace process_util
