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
#include "MD5.h"

using namespace std;
using namespace wgt;

#define BLANK  " "
#define TABLE  "\t"
#define NOTE "#"
#define SEMICOLON ";"
#define EQUAL "="
#define NOTEQUAL "!="
#define FIX "%"
#define NOT "!"
#define OR "|"
#define SRCFILE "srcfile"
#define GUIDFIELDNAME "guid"
#define NAMEFIELDNAME "name"
#define ALLCONDITION "all"
#define OTHERS "others"
#define ENTERCODE 13	// 回车符
#define DATAVERSION "15summer"
#define NOPOSITION -1

#define MAINROADCATALOG "0C070101DD"


enum EnumConditionLogic
{
	none,
	fieldequal,
	notequal,
	prefix,
	suffix,
	contain
};

typedef struct 
{
  string field;
  string value;
} Field;

typedef struct
{
	Field field;
	EnumConditionLogic enumCondition;
} CellCondition;

typedef struct 
{
	vector<CellCondition> conditions;
	vector<Field> assigns;
	vector<string> targetlayername;  //配置的目标图层名称  
} ConfigueItem;

typedef struct 
{
	string key;
	int time;
} KeyWordTime;

typedef map<string, vector<ConfigueItem> > ConfigLayers;
typedef map<string, vector<KeyWordTime> > LayerKeys;
typedef map<string, vector<string> > LayerFilds;

typedef struct Link_Item_S
{
	int index;
	string nodeid;

	Link_Item_S& operator=(const Link_Item_S& crf)
	{
		if (&crf == this)
		{ 
			return *this;
		}
		index = crf.index;
		nodeid = crf.nodeid;

		return *this;
	}

	bool operator < (const Link_Item_S& crf) const     
	{  
		if (index != crf.index)
			return index < crf.index;
		return nodeid < crf.nodeid;      
	}
} LinkItem;

typedef struct Group_S
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

string Wstring2String(const wstring &wstr);

wstring String2Wstring(const string &str);

class ConfigCatalog 
{
public:
	ConfigCatalog(string inputdir, string outputdir, string tencentdir,
            string confdir, string cityname);
	~ConfigCatalog();

	bool execute();
	bool LoadConfigFile(string filepath);
	bool LoadKindClassAdjust(string& conf_dir);
	bool Kind2Catalog3(string layername, wgt::MIF& mifLayer, vector<ConfigueItem>& layerConfigItems);
	int Road_Catalog(string in_path, string out_path, string layername);
	void ProcessRoadDirection(wgt::MIF& InMif);
	int Road_CatalogEx(string in_path, string out_path,string layername,string backPolygonfile);
	int RoadLevelUpgrade(string roadFile, string nodeFile, string roadupconfFile);
	void LoadMapid2dirConf(string confpath, map<string, vector<string> >& mapid2dirvec);
	void ExpandCityDir(wgt::MIF& C_R_Mif, map<string, int>& linkid2indexmap, map<string, vector<string> >& nodeid2lids, string path);
	// int Process_CommunityArea(string in_path, string out_path, string layername, vector<string> poi_pathVec);
	int Process_SubwayST(string line_path, string st_path);
	int Process_TrafficLight(string infile, string outfile);
	int Process_Railway(string line_path);

	void SmoothRoadCatalog(wgt::MIF& mifRoadLayer);
	bool SmoothRoadItemCatalog(wgt::MIF& mifRoadLayer, const int& iMifIndex, map<string, vector<int> >& mapIndex, const int& indexSnode, const int& indexEnode, const int& indexCatalog);
	void WriteLog(string strFileName, map<string, string>& map);
	double GetPointDistance(const Point2& pt1, const Point2& pt2);
	double GetLinkLength(wsl::Geometry* geoLine);

    /*************************************************
    // Method: RecatalogMainRoad
    // Description: 根据外挂表标识字段更改映射的linkcatalog
    // Returns: 
    // Parameter: 
    *************************************************/
    int RecatalogMainRoad(const string & input_mif, const string & output_mif, const string & plug_mif, const string &new_catalog);

protected:
	bool ParseSingleConfigueLine(string line);
	bool ParseSingleItems(string layer, string strSrcFile, string condition, string assign, string strTargetLayername);
	bool ParseString(vector<string>& vectorItems, string strItem, string seprator, bool botrimseperator);
	bool ConverToCondition(vector<CellCondition>& vecConditon, vector<string> items);
	void ConverToAssignFields(vector<Field> & vecAssign, vector<string> items);

	bool CatalogSingleLineConfig(wgt::MIF &inmif, size_t index, ConfigueItem configItems);
	bool MeetCondition(wgt::MIF &inmif, size_t index, CellCondition condition);
	bool MeetCondition2(wgt::MIF &inmif, size_t index, CellCondition condition);
	void AssignConfigFieldsValue(wgt::MIF &inmif, size_t index, vector<Field> fields);
	void AssignField(wgt::MIF &inmif, size_t index, Field singlefield);

	void GetMD5(string mifname,int id ,string &randstr);
	void RecordLayerKeyCondition(string layer, string key);

	void RecordLayerFields(string layer, string fieldname);
	void CheckFiledNames(wgt::MIF &inmif, string layername);
	void GetAvailableIndex(const string strLayer, wgt::MIF& mifLayer, int index, map<int, string>& mapIndex);

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
	// increment
	/*
    void LoadIncrementInfo();
	void RecordIncrementInfo(wsl::Geometry* geo_ptr, string layername);
	void WriteIncrementInfo();
    */
	int ProcessPOI(string strInputPath, const string& strInputAOI, string strOutputPath);
private:
	string strInputPath;
	string strOutputPath;
    string strTencentPath;
	string strConfPath;
	string strCityName;
	
    // 增量更新
	/*
    bool bIncrement;
	string m_incrementlayer_file;	// layer
	string m_incrementmeshinfo_file;// meshinfo.mif
	vector<string> m_incrementlayer_vec;
	wgt::MIF m_incrementmeshinfo_mif;
    */

    ConfigLayers layersConfig;
	LayerKeys layerKeys;
	LayerFilds layerFields;

	map<string, map<string, vector<int> > > layerKeyIndexs3;
	map<string, vector<ConfigueItem> > layerConfigs3;
	map<string, vector<string> > mapid2dirvec;

	map<string, string> kindClassAdjustMap;

	//map<string, string> roadCatalogTest;
};
#endif // __CONFIGCATALOG__H_
