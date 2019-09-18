#ifndef __CONFIGCATALOG__H_
#define __CONFIGCATALOG__H_
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "type_factory.h"
#include "spatial-base.h"
#include "tx_platform.h"
#include "tx_common.h"

using namespace wgt;

#define MAINROADCATALOG "0C070101DD"

typedef struct
{
    int kind_class;
    int connected_min_kind_class;
    bool connected_ferry;
    set<int> indexs;
    double length;
} Group;

typedef struct _Point2
{
    double x;
    double y;

}Point2;

typedef struct _RoadCatalogExistItem
{
    string strCatalog;
    int time;
}RoadCatalogExistItem;

void RoadUpgrade(wgt::MIF& plink, wgt::MIF pNlink, std::map<std::string, std::string> layerCatalog, std::map<std::string, std::string> upCatalog, std::string upValue, float limitLen, int empty_block_handle);

class ConfigCatalog 
{
public:
    ConfigCatalog(string inputdir, string outputdir, string tencentdir, string confdir, string cityname, string _incrementpath);
    ~ConfigCatalog();

    bool execute();
    bool LoadKindClassAdjust(string& conf_dir);
    int Road_Catalog(string in_path, string out_path, string layername);
    void ProcessRoadDirection(wgt::MIF& InMif);
    int Road_CatalogEx(string in_path, string out_path,string layername,string backPolygonfile);
    int RoadLevelUpgrade(string roadFile, string nodeFile, string roadupconfFile);
    void LoadMapid2dirConf(string confpath, map<string, vector<string> >& mapid2dirvec);
    void ExpandCityDir(wgt::MIF& C_R_Mif, map<string, int>& linkid2indexmap, map<string, vector<string> >& nodeid2lids, string path);

    void SmoothRoadCatalog(wgt::MIF& mifRoadLayer);
    bool SmoothRoadItemCatalog(wgt::MIF& mifRoadLayer, const int& iMifIndex, map<string, vector<int> >& mapIndex, const int& indexSnode, const int& indexEnode, const int& indexCatalog);
    double GetPointDistance(const Point2& pt1, const Point2& pt2);
    double GetLinkLength(wsl::Geometry* geoLine);

    int RecatalogMainRoad(const string & input_mif, const string & output_mif, const string & plug_mif, const string &new_catalog);

protected:
    bool ParseString(vector<string>& vectorItems, string strItem, string seprator, bool botrimseperator);

    string GetRoadLevelFromKind(string str_kind);
    string AdjustmentKindClass(string str_AdminCode, string str_kind_class, string str_funcclass);

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
    void ProcessRampKindClass(wgt::MIF& road_Mif, string in_path);
    void ProcessSAKindClass(wgt::MIF& road_Mif, string in_path);
    void ProcessRLinkKindClass(wgt::MIF& road_Mif, string in_path);
    void ProcessBuildInFlag(wgt::MIF& road_Mif, string in_path);
    void CheckKindClassConnectivity(wgt::MIF& road_Mif, string in_path);

private:
    string strInputPath;
    string strOutputPath;
    string strTencentPath;
    string strConfPath;
    string strCityName;

    map<string, vector<string> > mapid2dirvec;
    map<string, string> kindClassAdjustMap;
};

#endif // __CONFIGCATALOG__H_
