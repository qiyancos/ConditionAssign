#include "ConfigCatalog.h"
#include <stdio.h>  
#include <string.h>  
#include <queue>
#include <cctype>

bool IsNum(const string& str, size_t s = 0, size_t len = string::npos)
{
	size_t size = ((len == string::npos) || (len + s > str.size()))
		? str.size() : len + s;

	for (size_t index = s; index < size; index++)
	{
		if (!isdigit(str.at(index)))
			return false;
	}
	return true;
}

template <char type>
bool isValidBrandName(std::string name) {
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

bool isInVector(vector<string>& vec, string value)
{
	for (vector<string>::iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{
		if (value == *iter)
		{
			return true;
		}
	}
	return false;
}

string fix_quot_main2(std::string &str)
{
	if (str.empty())
	{
		return str;
	}
	if (*str.begin() == '\"' && *str.rbegin() == '\"')
	{
		str = str.substr(1, str.length() - 2);
	}
	return str;
} 

ConfigCatalog::ConfigCatalog(string inputdir, string outputdir, string tencentdir, string confdir, string cityname)
{
	strCityName = cityname;
	strInputPath = inputdir + "/" + cityname;
	strOutputPath = outputdir + "/" + cityname;
    strTencentPath = tencentdir;
	strConfPath = confdir;
    /*
	bIncrement = false;
	if (_incrementpath != "")
	{ // 增量处理
		m_incrementlayer_file = _incrementpath + "/layer";
		m_incrementmeshinfo_file= _incrementpath + "/meshinfo";
		bIncrement = true;
		LoadIncrementInfo();
	}
    */
}

ConfigCatalog::~ConfigCatalog()
{}

bool ConfigCatalog::execute()
{
	// 全量更新删除已有目录
    /*
	if (!bIncrement)
	{
		sys_deletedir(strOutputPath.c_str());
		if (!sys_folderexist(strOutputPath.c_str()) && !sys_mkdir(strOutputPath.c_str(), 0777))
		{
			sys_log_println(_ERROR, "create directory failed %s\n", strOutputPath.c_str());
			return false;
		}
	}
	// load catalog_config.conf	
	string catalogconfdir = strConfPath + "/catalog_config.conf";
	bool boLoad = LoadConfigFile(catalogconfdir);
	if (boLoad == false || layerConfigs3.size() == 0)
	{
		sys_log_println(_ERROR, "catalog load config file fail\n");
		return false;
	}
    */

	LoadKindClassAdjust(strConfPath);

    /*
	if (strCityName != "china")
	{
		string strInputPOI = strInputPath + "/C_POI";
		string strInputAOI = strInputPath + "/C_CommunityArea";
		string strOutputPath = strInputPOI;
		ProcessPOI(strInputPOI,strInputAOI,strOutputPath);
	}

	// 遍历catalog_config.conf,完成配置图层的映射
	{
		string strlayername = "";
		string strLayerPath = "";
		map<string, vector<ConfigueItem> >::iterator iteLayer;
		for (iteLayer = layerConfigs3.begin(); iteLayer != layerConfigs3.end(); iteLayer++)
		{
			strlayername = iteLayer->first;
			vector<ConfigueItem>& configItems = iteLayer->second;
			strLayerPath = strInputPath + "/" + strlayername;
			// check layer B_* for china; C_* for city except C_AdmArea
			if (strlayername != "C_AdmArea" && 
				((wgt::startswith(strlayername, "B_") && strCityName != "china") || (wgt::startswith(strlayername, "C_") && strCityName == "china")))
			{
				continue;
			}
			// if bIncrement check layer 
            if (bIncrement && !isInVector(m_incrementlayer_vec, strlayername))
			{
				continue;
			}

			sys_log_println(_INFORANK, "begin to catalog layer:%s\n", strLayerPath.c_str());
			if (layerKeys.find(strlayername) == layerKeys.end())  // 表明没有配置条件  
			{
				continue;
			}
			wgt::MIF inmif;
			if (wgt::mif_to_wsbl(strLayerPath, inmif) < 0)
			{	
				sys_log_println(_WARN, "load mif fail:%s\n", strLayerPath.c_str());
				continue;
			}
			CheckFiledNames(inmif, strlayername);
			bool boresult = Kind2Catalog3(strlayername, inmif, configItems);
			if (!boresult)
			{
				sys_log_println(_WARN, "catalog,layer %s fail\n", strlayername.c_str());
			}
			sys_log_println(_INFORANK, "finish to catalog layer:%s\n", strLayerPath.c_str());
		}
	}
    */

	// 城市级别特殊分类处理
	if (strCityName != "china")
	{
		// 增加红绿灯C_TrafficLight图层的生成（红绿灯路口居中）
		/*
        if (!bIncrement || isInVector(m_incrementlayer_vec, "C_N"))
		{
			sys_log_println(_INFORANK, "process C_TrafficLight\n");
			string infile = strInputPath + "/C_N";
			string outfile = strOutputPath + "/C_TrafficLight";
			Process_TrafficLight(infile, outfile);
		}
        */

		// if (!bIncrement || isInVector(m_incrementlayer_vec, "C_R"))
		{
			sys_log_println(_INFORANK, "process C_R catalog\n");
			mapid2dirvec.clear();
			string mapid2dirvec_file = strConfPath + "/mapid2citydir";
			LoadMapid2dirConf(mapid2dirvec_file, mapid2dirvec);
			string layername = "C_R";
			string infile = strInputPath + "/" + layername;
			string  outfile = strOutputPath + "/" + layername;
			string backPolygonfile = strInputPath + "/" + "C_BackPolygon";
			if (Road_CatalogEx(infile, outfile, layername, backPolygonfile) < 0)
			{
				sys_log_println(_ERROR, "road: %s error\n", infile.c_str());
				return false;
			}
		}

		//// 道路疏密度,将道路稀少地区的道路的分类提升,保留原有的catalog到新字段oldcatalog
		//if (!bIncrement || isInVector(m_incrementlayer_vec, "C_R"))
		//{
		//	sys_log_println(_INFORANK, "process C_R road_upgrade\n");
		//	string roadFile = strOutputPath + "/C_R";
		//	string nodeFile = strInputPath + "/C_N";
		//	string roadupconfFile = strConfPath + "/road_upgrade.conf";
		//	RoadLevelUpgrade(roadFile, nodeFile, roadupconfFile);
		//}

        // 2017.09.20   精细化区域面道路主干道处理
        //  - 数据端提供一个外挂表, 通过外挂表的标识字段修改关联link的catalog
        // if (!bIncrement || isInVector(m_incrementlayer_vec, "C_R"))
        {
            sys_log_println(_INFORANK, "process C_R main road\n");
            string input_layer = strOutputPath + "/C_R";
            string output_layer = strOutputPath + "/C_R";
            string plug_layer = strTencentPath + "/" + strCityName + "/mainroad";
            string new_catalog = MAINROADCATALOG;
            int rev = RecatalogMainRoad(input_layer, output_layer, plug_layer, new_catalog);
            if (rev < 0)
            {
                sys_log_println(_WARN, "main road process failed.\n");
            }
            else
            {
                sys_log_println(_INFORANK, "succeed. cnt = %d\n", rev);
            }
        }

		/** C_POI乡镇和村庄类点特殊处理
		*1、村庄类位于水系面中 && 后缀名为"桥"、"隧道",分类修改为010A0C02FF(桥:代表)
		*2、村庄类位于水系面中 其他分类修改为01FF0303(水系名称)
		*3、村庄类位于建成区中 分类映射为*XX无效化
		**/
		/*
        if (!bIncrement || isInVector(m_incrementlayer_vec, "C_POI"))
		{
			sys_log_println(_INFORANK, "process C_POI water poi\n");
			string layername = "C_BackPolygon";
			string layername2 = "C_POI";
			string cbackpolygon_file = strOutputPath + "/" + layername;
			string cpoi_file = strOutputPath + "/" + layername2;

			vector<wsl::Feature<wsl::Polygon>* > BuildUpArea;
			vector<wsl::Feature<wsl::Polygon>* > WaterArea;
			wgt::MIF backpolygonMif;
			if (wgt::mif_to_wsbl(cbackpolygon_file, backpolygonMif) < 0)
			{
				sys_log_println(_INFORANK, "load mif fail:%s\n", cbackpolygon_file.c_str());
				return false;
			}
			int col_catalog = backpolygonMif.get_col_pos("catalog");
			if (col_catalog < 0)
			{
				sys_log_println(_ERROR, "get col catalog error, %s\n", cbackpolygon_file.c_str());
				return false;
			}
			for (int i=0; i<backpolygonMif.mid.size(); i++)
			{
				string catalog_str = backpolygonMif.mid[i][col_catalog];
				wgt::trim(catalog_str, '"');
				if (catalog_str == "0809") // 建成区
				{
					wsl::Feature<wsl::Polygon>* feat_ptr = (wsl::Feature<wsl::Polygon>*)backpolygonMif.data.geo_vec[i];
					BuildUpArea.push_back(feat_ptr);
				}
				else if (wgt::startswith(catalog_str, "09010105") || catalog_str == "09010105" || catalog_str == "09010205") // 水系(河流,湖泊等)
				{
					wsl::Feature<wsl::Polygon>* feat_ptr = (wsl::Feature<wsl::Polygon>*)backpolygonMif.data.geo_vec[i];
					WaterArea.push_back(feat_ptr);
				}
			}
			//打开POI表，对于类别为0206和0207的做筛选
			wgt::MIF poiMif;
			if (wgt::mif_to_wsbl(cpoi_file, poiMif) < 0)
			{
				sys_log_println(_INFORANK, "load mif fail:%s\n", cpoi_file.c_str());
				return false;
			}
			col_catalog = poiMif.get_col_pos("catalog");
			int col_name = poiMif.get_col_pos("name");
			if (col_catalog < 0 || col_name < 0)
			{
				sys_log_println(_ERROR, "get col catalog name error, %s\n", cpoi_file.c_str());
				return false;
			}
			for (int i=0; i<poiMif.mid.size(); i++)
			{
				string& catalog_str = poiMif.mid[i][col_catalog];
				wgt::trim(catalog_str, '"');
				wsl::Feature<wsl::Point>* feat_ptr = (wsl::Feature<wsl::Point>*) poiMif.data.geo_vec[i];
				if (catalog_str == "0206" || catalog_str == "0207")	// 乡镇|村庄
				{
					for (int j=0; j<WaterArea.size(); j++)
					{
						wsl::Feature<wsl::Polygon>* feat_ptr2 = WaterArea[j];
						if (wsl::intersect(feat_ptr, feat_ptr2) == wsl::CONTAIN)
						{
							string name_str = poiMif.mid[i][col_name];
							wgt::trim(name_str, '"');
							if (wgt::endswith(name_str, "桥") || wgt::endswith(name_str, "隧道"))
							{
								catalog_str = "010A0C02FF";		// 桥:代表
							}
							else
							{
								catalog_str = "01FF0303";		// 水系名称
							}
							break;
						}
					}
					for (int j=0; j<BuildUpArea.size(); j++)
					{
						wsl::Feature<wsl::Polygon>* feat_ptr2 = BuildUpArea[j];
						if (wsl::intersect(feat_ptr, feat_ptr2) == wsl::CONTAIN)
						{
							catalog_str = catalog_str + "XX";	// 无效化
							break;
						}
					}
				}
			}
			if (wgt::wsbl_to_mif(poiMif, cpoi_file) != 0)
			{
				sys_log_println(_INFORANK, "write back mif fail:%s \n", cpoi_file.c_str());
				return false;
			}
		}
        */

		/**
		* C_CommunityArea(有名区域面)
		* 1.读取C_POI.mid,关联id与master_id字段,确定区域面的父子关系
		* 2.catalog分类: 父节点(090301) \ 子节点(090302) \ 父子节点(090303) \ 其他(090300)
		*/
		//if (!bIncrement || isInVector(m_incrementlayer_vec, "C_CommunityArea"))
		//{
		//	sys_log_println(_INFORANK, "process C_CommunityArea relationship\n");
		//	string layername = "C_CommunityArea";
		//	string infile = strInputPath + "/" + layername;
		//	string outfile = strOutputPath + "/" + layername;
		//	string poifile = strInputPath + "/C_POI";
		//	//string manualpoifile = strInputPath + "/C_ManualPOI";
		//	vector<string> poifileVec;
		//	poifileVec.push_back(poifile);
		//	//poifileVec.push_back(manualpoifile);
		//	string communitymiffile = infile + ".mif";
		//	if (sys_folderexist(communitymiffile.c_str()))
		//	{
		//		if (Process_CommunityArea(infile, outfile, layername, poifileVec) < 0)
		//		{
		//			sys_log_println(_ERROR, "process C_CommunityArea: %s error\n", infile.c_str());
		//			return false;
		//		}
		//	}
		//}

		/**
		* 增加处理地铁站catalog
		* 在建地铁站+在建地铁线
		**/
        /*
		if (!bIncrement || isInVector(m_incrementlayer_vec, "C_SubwayLine") || isInVector(m_incrementlayer_vec, "C_SubwayST"))
		{
			sys_log_println(_INFORANK, "process C_SubwayLine & C_SubwayST under construction\n");
			string subwaylinefile = strOutputPath + "/C_SubwayLine";
			string subwaystfile = strOutputPath + "/C_SubwayST";
			string subwaylinemiffile = subwaylinefile + ".mif";
			string subwaystfilemiffile = subwaystfile + ".mif";
			if (sys_folderexist(subwaylinemiffile.c_str()) || sys_folderexist(subwaystfilemiffile.c_str()))
			{
				if (Process_SubwayST(subwaylinefile, subwaystfile))
				{
					sys_log_println(_ERROR, "Process_SubwayST %s \n", subwaystfile.c_str());
				}
			}			
		}
        */
		
        /**
		* 增加处理铁路线catalog
		* 区分高铁
		**/
		/*
        if (!bIncrement || isInVector(m_incrementlayer_vec, "C_Railway"))
		{	cout<<"ffff"<<endl;
			sys_log_println(_INFORANK, "process C_Railway \n");
			string raillinefile = strOutputPath + "/C_Railway";
			string raillinemiffile = raillinefile + ".mif";
			if (sys_folderexist(raillinemiffile.c_str()))
			{
				if (Process_Railway(raillinefile))
				{
					sys_log_println(_ERROR, "raillinefile %s \n", raillinefile.c_str());
				}
			}			
		}else{
			cout<<"wwwwww"<<endl;
		}
        */
	}
	// WriteIncrementInfo();
	sys_log_println(_INFORANK, "finish to catalog all layers\n");
}

int ConfigCatalog::Process_Railway(string line_path)
{
	wgt::MIF mifRail;
	int ret = -1;
	ret = wgt::mif_to_wsbl(line_path,mifRail);
	printf("king");
	if (ret <= -1)
	{
		
		return ret;
	}
	int col_name = mifRail.get_col_pos("name");
	int col_catalog = mifRail.get_col_pos("catalog");
	if (col_name < 0 || col_catalog < 0)
	{
		return -1;
	}
	int nSize = mifRail.mid.size();
	for (int i =0;i< nSize; i++)
	{
		string name = mifRail.mid[i][col_name];
		cout<<"king1"<<name<<endl;
		if(name.find("高速")!=string::npos||name.find("城际")!=string::npos || name.find("客运专线")!=string::npos)
		{
			string new_catalog = "\"05010201\"";
			mifRail.mid[i][col_catalog] = new_catalog;
			cout<<"king3ok"<<endl;
		}
	}
	printf("king2");
	return wgt::wsbl_to_mif(mifRail, line_path);
}
bool ConfigCatalog::LoadConfigFile(std::string filepath)
{
	if (filepath.empty() == true)
	{
		return false;
	}
	ifstream confFile(filepath.c_str());
	if (confFile.is_open() == false)
	{
		return false;
	}
	string line;
	while (confFile.good() && confFile.eof() == false)
	{
		getline(confFile, line);
		line = wgt::trim(line, BLANK);

#ifdef LINUX
		line = wgt::trim_right(line, ENTERCODE);
#endif

		if (line.empty() || wgt::startswith(line, NOTE))   //如果为空或者#开头  
		{
			continue;
		}

		ParseSingleConfigueLine(line);
	}
	confFile.close();
	return true;
}

bool ConfigCatalog::LoadKindClassAdjust(string& conf_dir)
{	  	
	str_t file_name = conf_dir + "/kindclass_adjust.txt";
	
	str_t strline;
	std::vector<str_t> splitItems;
	std::ifstream infile( file_name.data() );
	while (infile.good() && !infile.eof())
	{
		strline.clear();
		getline(infile, strline);
		if (strline.empty()) 
		{
			continue;
		}
		strline = wgt::trim(strline, " ");
		strline = wgt::trim_right(strline, "\r");

		if (strline.at(0)=='#')
		{
			continue;
		}
		sys_splitString(strline, splitItems, ',');
		if (splitItems.size() < 4) 
		{
			continue;
		}

		kindClassAdjustMap[splitItems[0] + "_" + splitItems[1] + "_" + splitItems[2]] = splitItems[3];
	}
	return true;
}

int ConfigCatalog::RecatalogMainRoad(const string & input_mif, const string & output_mif, const string & plug_mif, const string & new_catalog)
{
    int cnt = 0;

    // 1. 载入外挂表, 记录标识主干道的link
    set<string> main_link;
    {
        wgt::MIF mif;
        int rev = wgt::mif_to_wsbl(plug_mif, mif);
        TX_CHECK_MIF_READ_RET(rev, plug_mif);

        int pos_id = mif.get_col_pos("id");
        int pos_rgrade = mif.get_col_pos("rgrade");
        TX_CHECK_COL_ID(pos_id, plug_mif);
        TX_CHECK_COL_ID(pos_rgrade, plug_mif);

        size_t mif_size = mif.mid.size();
        for (size_t i = 0; i < mif_size; ++i)
        {
            string id = mif.mid[i][pos_id];
            string rgrade = mif.mid[i][pos_rgrade];
            wgt::trim(id, '"');
            wgt::trim(rgrade, '"');

            if (rgrade == "1")  main_link.insert(id);
        }
    }

    // 2. 遍历道路表, 修改catalog
    wgt::MIF mif;
    int rev = wgt::mif_to_wsbl(input_mif, mif);
    TX_CHECK_MIF_READ_RET(rev, input_mif);

    int pos_id = mif.get_col_pos("id");
    int pos_catalog = mif.get_col_pos("catalog");
    TX_CHECK_COL_ID(pos_id, input_mif);
    TX_CHECK_COL_ID(pos_catalog, input_mif);

    size_t mif_size = mif.mid.size();
    for (size_t i = 0; i < mif_size; ++i)
    {
        string id = mif.mid[i][pos_id];
        string catalog = mif.mid[i][pos_catalog];
        wgt::trim(id, '"');
        wgt::trim(catalog, '"');

        if (main_link.count(id) > 0)    // 匹配主干道linkid, 修改catalog
        {
            mif.mid[i][pos_catalog] = new_catalog;
            ++cnt;
        }
    }

    // 保存mif
    if (wgt::wsbl_to_mif(mif, output_mif) < 0)
    {
        return -3;
    }
    
    return cnt;
}

bool ConfigCatalog::ParseSingleConfigueLine(std::string line)
{
	if (line.empty() || wgt::startswith(line, NOTE))
	{
		return false;
	}
	vector<string> splitItems;
	bool parseReult = ParseString(splitItems, line, TABLE, false);
	if (parseReult == false)
	{
		return false;
	}
	int itemSize = splitItems.size();
	if (itemSize <= 0)
	{
		return false;
	}

	string strLayers = splitItems[0];
	string strSrcFile;
	string strCondition;
	string strAssign;
	string strTargetLayername;
	if (itemSize > 1)
	{
		strSrcFile = splitItems[1];
	}
	if (itemSize > 2)
	{
		strCondition = splitItems[2];
	}
	if (itemSize > 3)
	{
		strAssign = splitItems[3];
	}
	if (itemSize > 4)
	{
		strTargetLayername = splitItems[4];
	}

	return ParseSingleItems(strLayers, strSrcFile, strCondition, strAssign, strTargetLayername);
}


// 分隔字符串, 当分隔符包含在引号中时, 当做普通字符串
// 为支持宽字符设置的全局字符编码会造成输出数字带千分符逗号, 暂时使用string处理
bool ConfigCatalog::ParseString(std::vector<string> &vectorItems, std::string strItem, std::string seprator, bool botrimSeperator)
{
	strItem = wgt::trim(strItem, BLANK);
	if (strItem.empty() == true)
	{
		return false;
	}

	unsigned start = 0;
	bool quot_status = false;
	string sub_str;
	for (size_t i = 0; i < strItem.size(); ++i)
	{
		if (strItem[i] == '"')		// 识别到引号, 切换引号状态
		{
			quot_status ^= true;
		}

		if (!quot_status && (strItem[i] == seprator[0]))		// 非引号状态下才识别分隔符
		{

			sub_str = strItem.substr(start, i - start);
			if (botrimSeperator)
			{
				sub_str = wgt::trim(sub_str, seprator);
				if (!sub_str.empty())
				{
					vectorItems.push_back(sub_str);
				}
			}
			else {
				vectorItems.push_back(sub_str);
			}
			start = i + 1;
		}

		// 结尾处理
		if ((i == strItem.size() - 1) && (strItem[i] != seprator[0]))
		{
			sub_str = strItem.substr(start, i + 1 - start);
			if (botrimSeperator)
			{
				sub_str = wgt::trim(sub_str, seprator);
				if (!sub_str.empty())
				{
					vectorItems.push_back(sub_str);
				}
			}
			else {
				vectorItems.push_back(sub_str);
			}
		}
	}

	return vectorItems.size() > 0;

}


//// 分隔字符串, 当分隔符包含在引号中时, 当做普通字符串
//bool ConfigCatalog::ParseString(std::vector<string> &vectorItems, std::string strItem, std::string seprator, bool botrimSeperator)
//{
//	strItem = wgt::trim(strItem, BLANK);
//	if (strItem.empty() == true)
//	{
//		return false;
//	}
//
//	// 转换为宽字符处理避免中英混合出现乱码
//	wstring wstrItem = String2Wstring(strItem);
//	unsigned start = 0;
//	bool quot_status = false;
//	string sub_str;
//	for (size_t i = 0; i < wstrItem.size(); ++i)
//	{
//		if (wstrItem[i] == '"')		// 识别到引号, 切换引号状态
//		{
//			quot_status ^= true;
//		}
//
//		if (!quot_status && (wstrItem[i] == seprator[0]))		// 非引号状态下才识别分隔符
//		{
//
//			sub_str = Wstring2String(wstrItem.substr(start, i - start));
//			if (botrimSeperator)
//			{
//				sub_str = wgt::trim(sub_str, seprator);
//				if (!sub_str.empty())
//				{
//					vectorItems.push_back(sub_str);
//				}
//			}
//			else {
//				vectorItems.push_back(sub_str);
//			}
//			start = i + 1;
//		}
//
//		// 结尾处理
//		if ((i == wstrItem.size() - 1) && (wstrItem[i] != seprator[0]))
//		{
//			sub_str = Wstring2String(wstrItem.substr(start, i + 1 - start));
//			if (botrimSeperator)
//			{
//				sub_str = wgt::trim(sub_str, seprator);
//				if (!sub_str.empty())
//				{
//					vectorItems.push_back(sub_str);
//				}
//			}
//			else {
//				vectorItems.push_back(sub_str);
//			}
//		}
//	}
//
//	return vectorItems.size() > 0;
//
//}


//bool ConfigCatalog::ParseString(std::vector<string> &vectorItems, std::string strItem, std::string seprator, bool botrimSeperator)
//{
	//strItem = wgt::trim(strItem, BLANK);
//	if (strItem.empty() == true)
//	{
//		return false;
//	}
//	vector<string> splitItems;
//
	//wgt::split(strItem, splitItems, seprator[0]);
//	int isize = splitItems.size();
//	if (isize == 0)
//	{
//		return false;
//	}
//
//	for (int i = 0; i < isize; i++)
//	{
//		string strTem;
//		strTem = splitItems[i];
//		if (botrimSeperator)
//		{
//			strTem = wgt::trim(strTem, seprator);
//			if (strTem.empty())
//			{
//				continue;
//			}
//		}
//		/*strTem = wgt::trim(strTem, BLANK);
//		strTem = wgt::trim(strTem, seprator);
//		if (strTem.empty() == true)
//		{
//			continue;
//		}*/
//		vectorItems.push_back(strTem);
//	}
//	return true;
//}

bool ConfigCatalog::ParseSingleItems(std::string layer, string strSrcFile, std::string condition, std::string assign, string strTargetLayername)
{
	if (layer.empty() == true)
	{
		return false;
	}
	vector<string> veclayers;
	vector<string> vecCondition;
	vector<string> vecAssign;
	vector<string> vecTargetlayers;

	bool parseResult = false;
	parseResult = ParseString(veclayers, layer, SEMICOLON, true);
	if (parseResult == false)
	{
		return false;
	}
	ParseString(vecCondition, condition, SEMICOLON, true);
	ParseString(vecAssign, assign, SEMICOLON, true);

	bool parseResult1 = false;
	parseResult1 = ParseString(vecTargetlayers, strTargetLayername, SEMICOLON, true);
	if (parseResult1 == false)
	{
		vecTargetlayers.resize(1);
		vecTargetlayers[0] = "";
	}

	strSrcFile = wgt::trim(strSrcFile, BLANK[0]);
	if (!strSrcFile.empty())
	{
		strSrcFile = wgt::tolower(strSrcFile);
		string strSrc = SRCFILE;
		strSrc = strSrc + "=" + strSrcFile;
		vecCondition.push_back(strSrc);
	}

	vector<CellCondition> conditions;
	bool bocondition = ConverToCondition(conditions, vecCondition);
	if (!bocondition)
	{ //条件无效，不是没有条件   
		return false;
	}

	vector<Field> fields;
	ConverToAssignFields(fields, vecAssign);

	int iconditionSize = conditions.size();

	int ilayersize = veclayers.size();
	string layername;
	for (int i = 0; i < ilayersize; i++)
	{
		layername = veclayers[i];
		if (layername.empty() == true)
		{
			continue;
		}

		ConfigueItem confiItem;
		confiItem.assigns = fields;
		confiItem.conditions = conditions;
		confiItem.targetlayername = vecTargetlayers;

		//记得重新构造索引，因为有可能有分号和逗号 

		int iConditionSize = conditions.size();

		bool boHasEauql = false;
		string strRecordFieldName;
		string strEaqualFieldValue;
		vector<string> vecSplitValue;
		if (iConditionSize == 0)
		{
			strRecordFieldName = ALLCONDITION;
		}
		else
		{
			for (int k = 0; k < iConditionSize; k++)
			{
				if (conditions[k].enumCondition == fieldequal)  //如果是相等的  
				{
					strRecordFieldName = conditions[k].field.field;
					strEaqualFieldValue = conditions[k].field.value;
					vecSplitValue.clear();
					if (strEaqualFieldValue.find(",", 0) != NOPOSITION)
					{
						ParseString(vecSplitValue, strEaqualFieldValue, ",", true);
					}
					else if (strEaqualFieldValue.find(SEMICOLON, 0) != NOPOSITION)
					{
						ParseString(vecSplitValue, strEaqualFieldValue, SEMICOLON, true);
					}
					else
					{
						vecSplitValue.push_back(strEaqualFieldValue);
					}
					boHasEauql = true;
					break;
				}
			}

			if (!boHasEauql)
			{
				strRecordFieldName = OTHERS;
			}
		}

		layerConfigs3[layername].push_back(confiItem);
		if (boHasEauql)  //设置了条件的，而且找到等号的索引的  
		{
			int isizeSplit = vecSplitValue.size();
			string strKeyAndValue;
			for (int j = 0; j < isizeSplit; j++)
			{
				strKeyAndValue = strRecordFieldName + "=" + vecSplitValue[j];
				layerKeyIndexs3[layername][strKeyAndValue].push_back(layerConfigs3[layername].size() - 1);
			}
		}
		else
		{
			layerKeyIndexs3[layername][strRecordFieldName].push_back(layerConfigs3[layername].size() - 1);
		}

		RecordLayerKeyCondition(layername, strRecordFieldName);
	}

	for (int i = 0; i < ilayersize; i++)
	{
		layername = veclayers[i];
		if (layername.empty())
		{
			continue;
		}
		int ifieldSize = fields.size();
		for (int f = 0; f < ifieldSize; f++)
		{
			RecordLayerFields(layername, fields[f].field);
		}
	}
	return true;
}

bool ConfigCatalog::ConverToCondition(std::vector<CellCondition> &vecConditon, std::vector<string> items)
{
	int isize = items.size();
	if (isize == 0)
	{
		return true;
	}
	string strItem;
	for (int i = 0; i < isize; i++)
	{
		strItem = items[i];
		strItem = wgt::trim(strItem, BLANK);
		if (strItem.empty() == true)
		{
			continue;
		}
		string strSeperator;
		bool bonotequal = false;
		bool boequal = false;
		bool bofix = false;

		if (strItem.find(NOTEQUAL, 0) != NOPOSITION)
		{
			bonotequal = true;
		}
		if (strItem.find(EQUAL, 0) != NOPOSITION)
		{
			boequal = true;
		}
		if (strItem.find(FIX, 0) != NOPOSITION)
		{
			bofix = true;
		}

		vector<string> vecSubItems;
		ParseString(vecSubItems, strItem, EQUAL, true);
		if (vecSubItems.size() != 2)
		{
			sys_log_println(_WARN, "parse catalog config condition,can not match: %s !\n", strItem.c_str());
			return false;
		}
		CellCondition condition;
		if (bofix == true) //前缀或者后缀，或者包含关系     
		{
			bool boPrefix = wgt::startswith(vecSubItems[0], FIX);
			bool boSuffix = wgt::endswith(vecSubItems[0], FIX);
			if (boPrefix && boSuffix)
			{
				vecSubItems[0] = wgt::trim_left(vecSubItems[0], FIX);
				vecSubItems[0] = wgt::trim_right(vecSubItems[0], FIX);
				condition.enumCondition = contain;
			}
			else if (boPrefix)
			{
				vecSubItems[0] = wgt::trim_left(vecSubItems[0], FIX);
				condition.enumCondition = prefix;
			}
			else if (suffix)
			{
				vecSubItems[0] = wgt::trim_right(vecSubItems[0], FIX);
				condition.enumCondition = suffix;
			}
			else
			{
				sys_log_println(_WARN, "parse catalog config condition,can not match: %s !\n", strItem.c_str());
				continue;
			}
			/*if (wgt::startswith(vecSubItems[0], FIX) == true)
			{
				vecSubItems[0] = wgt::trim_left(vecSubItems[0], FIX);
				condition.enumCondition = prefix;
			}
			else if(wgt::endswith(vecSubItems[0], FIX) == true)
			{
				vecSubItems[0] = wgt::trim_right(vecSubItems[0], FIX);
				condition.enumCondition = suffix;
			}
			else
			{
				sys_log_println(_WARN, "parse catalog config condition,can not match: %s !\n",strItem.c_str());
				continue;
			}*/
		}
		else if (bonotequal == true) //不等于  
		{
			vecSubItems[0] = wgt::trim_right(vecSubItems[0], NOT);
			condition.enumCondition = notequal;
		}
		else //等于 
		{
			condition.enumCondition = fieldequal;
		}
		condition.field.field = wgt::tolower(vecSubItems[0]);
		//condition.field.value = wgt::toupper(vecSubItems[1]);
		condition.field.value = wgt::tolower(wgt::trim(vecSubItems[1], '"'));

		vecConditon.push_back(condition);
	}
	return true;
}

void ConfigCatalog::ConverToAssignFields(std::vector<Field> &vecAssign, std::vector<string> items)
{
	int isize = items.size();
	if (isize <= 0)
	{
		return;
	}

	string strItem;
	for (int i = 0; i < isize; i++)
	{
		strItem = items[i];
		strItem = wgt::trim(strItem, " ");
		if (strItem.empty() == true)
		{
			continue;
		}
		vector<string> vecSubItems;
		ParseString(vecSubItems, strItem, EQUAL, true);
		if (vecSubItems.size() != 2)
		{
			sys_log_println(_WARN, "parse catalog config assign,err:%s,size=%d\n", strItem.c_str(), vecSubItems.size());
			continue;
		}
		Field fieldAssign;
		fieldAssign.field = vecSubItems[0];
		fieldAssign.value = wgt::toupper(vecSubItems[1]);
		vecAssign.push_back(fieldAssign);
	}
}

bool ConfigCatalog::Kind2Catalog3(string layername, wgt::MIF& mifLayer, vector<ConfigueItem>& layerConfigItems)
{
	Field filedGuid;
	filedGuid.field = GUIDFIELDNAME;

	Field filedName;
	filedName.field = NAMEFIELDNAME;

	map<string, wgt::MIF> targetLayers;
	//bool IsSaveFlag = 1;
	vector<std::string> vecTargetLayers;
	string strTargetLayer;
	string strTargetPath;

	//int iCount = 0;

	int  pos_name = mifLayer.get_col_pos("name");
	int  pos_catalog = mifLayer.get_col_pos("catalog");

	size_t mid_size(mifLayer.mid.size());
	map<int, string> mapIndex;
	if (mid_size == 0) //空图层时需要直接输出空
	{
		strTargetPath = strOutputPath + "/" + layername;
		int ifieldIndex = mifLayer.get_col_pos("name");
		if (ifieldIndex == -1)
		{
			mifLayer.add_column("name", "char(64)", mifLayer.header.col_num);
		}
		ifieldIndex = mifLayer.get_col_pos("catalog");
		if (ifieldIndex == -1)
		{
			mifLayer.add_column("catalog", "char(64)", mifLayer.header.col_num);
		}
		ifieldIndex = mifLayer.get_col_pos("guid");
		if (ifieldIndex == -1)
		{
			mifLayer.add_column("guid", "char(64)", mifLayer.header.col_num);
		}

		wgt::check_err("write back to ouput mif", wgt::wsbl_to_mif(mifLayer, strTargetPath));
	}

	int iConfigSize = layerConfigItems.size();
	for (size_t i = 0; i < mid_size; i++)
	{
		if (pos_catalog != -1) //如果原表有catalog字段，则将该字段先清空后再赋值
		{
			mifLayer.mid[i][pos_catalog] = "";
		}

		//赋值guid 字段 
		string guid;
		GetMD5(layername, i, guid);
		filedGuid.value = guid;
		AssignField(mifLayer, i, filedGuid);

		if (pos_name == -1)
		{
			filedName.value = "";
		}
		else
		{
			filedName.value = mifLayer.mid[i][pos_name];
		}

		AssignField(mifLayer, i, filedName);

		//int IsSaveFlag = 0;
		bool boHasAssign = false;
		mapIndex.clear();
		GetAvailableIndex(layername, mifLayer, i, mapIndex);
		for (int k = 0; k < iConfigSize; k++)
		{
			if (mapIndex.find(k) == mapIndex.end())
			{
				continue;
			}
			ConfigueItem& item = layerConfigItems[k];
			bool boMeet = CatalogSingleLineConfig(mifLayer, i, item);
			if (!boMeet)
			{
				continue;
			}

			AssignConfigFieldsValue(mifLayer, i, item.assigns);
			boHasAssign = true;
			vecTargetLayers = item.targetlayername;
			break;
		}
		/*if (!boHasAssign)
		{
			continue;
		}*/
		if (vecTargetLayers.size() == 0)
		{
			vecTargetLayers.push_back("");
		}

		int sizeTargetLayer = vecTargetLayers.size();
		for (int k = 0; k < sizeTargetLayer; k++)
		{
			strTargetLayer = vecTargetLayers[k];
			//如果是留在原来图层的，不再重新新建一个图层，耗内存  
			if (strTargetLayer.empty())  //如果要更换图层名称图层  
			{
				strTargetLayer = layername;
			}
			//if (IsSaveFlag == 0) //未找到对应的分类映射，可以考虑不保存该数据 
			//{
			//	continue;
			//}

			if (targetLayers.find(strTargetLayer) == targetLayers.end())
			{
				wgt::MIF newMifLayer;
				int flag = -1;
				/*
                if (!bIncrement)
				{
					strTargetPath = strOutputPath + "/" + strTargetLayer;
					flag = wgt::mif_to_wsbl(strTargetPath, newMifLayer);
				}
                */
				if (flag < 0)
				{
					newMifLayer.header = mifLayer.header;
					newMifLayer.header.coordsys = mifLayer.header.COORDSYS_LL;
				}
				targetLayers[strTargetLayer] = newMifLayer;
			}

			targetLayers[strTargetLayer].mid.push_back(mifLayer.mid[i]);
			if (mifLayer.data.geo_vec[i] != NULL)
			{
				targetLayers[strTargetLayer].data.geo_vec.push_back(mifLayer.data.geo_vec[i]->clone());
			}
			else
			{
				targetLayers[strTargetLayer].data.geo_vec.push_back(NULL);
			}
			//iCount++;

			if (i == mid_size - 1)
			{
				sys_log_println(_INFORANK, "finish feature :%d\n", i);
			}
		}

	}

	map<string, wgt::MIF>::iterator iteTargetLayer;
	for (iteTargetLayer = targetLayers.begin(); iteTargetLayer != targetLayers.end(); iteTargetLayer++)
	{
		strTargetLayer = iteTargetLayer->first;
		strTargetPath = strOutputPath + "/" + strTargetLayer;
		wgt::check_err("write back to ouput mif", wgt::wsbl_to_mif(iteTargetLayer->second, strTargetPath));
		/*
        if (bIncrement && strTargetLayer != layername)
		{
			// 适用一些图层layer名称修改的,例如：C_N -> C_TrafficLight
			RecordIncrementInfo(NULL, strTargetLayer);
		}
        */
	}
	targetLayers.clear();

	return true;
}

void ConfigCatalog::GetAvailableIndex(const string strLayer, wgt::MIF& mifLayer, int index, map<int, string>& mapIndex)
{
	mapIndex.clear();
	int isize = mifLayer.mid.size();
	if (index >= isize || index < 0)
	{
		return;
	}
	if (layerKeys.find(strLayer) == layerKeys.end())
	{
		return;
	}
	if (layerKeyIndexs3.find(strLayer) == layerKeyIndexs3.end())
	{
		return;
	}
	vector<KeyWordTime>& keyword = layerKeys[strLayer];
	int iKeySize = keyword.size();
	string strKeyWord;
	string strFieldValue;

	for (int i = 0; i < iKeySize; i++)
	{
		strKeyWord = keyword[i].key;
		strFieldValue = strKeyWord;
		if (strKeyWord != ALLCONDITION && strKeyWord != OTHERS)
		{
			int ifieldIndex = mifLayer.get_col_pos(strKeyWord);
			if (ifieldIndex == -1)
			{
				if (strLayer.find("C_Pedestrian") != string::npos)
				{//如果是C_Pedestrian图层，暂时不提示field找不到提示 
					continue;
				}
				string output = strLayer + "," + strKeyWord;
				sys_log_println(_WARN, "laye no field:%s\n", output.c_str());
				//IsSaveFlag = 0;
				continue;
			}
			string fieldValue = wgt::tolower(mifLayer.mid[index][ifieldIndex]);

			fieldValue = wgt::trim(fieldValue, ' ');
			fieldValue = fix_quot_main2(fieldValue);
			//多kind需要拆解，只取第一个
			vector<std::string >fieldVec;
			wgt::split(fieldValue, fieldVec, '|');
			string fieldValuenew = fieldVec[0];

			strFieldValue = strKeyWord + "=" + fieldValuenew; //只构造key=value的
		}

		map<string, vector<int> >::iterator iteFound = layerKeyIndexs3[strLayer].find(strFieldValue);
		if (iteFound == layerKeyIndexs3[strLayer].end())
		{
			continue;
		}
		vector<int>& vecIndex = layerKeyIndexs3[strLayer][strFieldValue];
		int isizeIndex = vecIndex.size();
		for (int i = 0; i < isizeIndex; i++)
		{
			if (mapIndex.find(vecIndex[i]) == mapIndex.end())
			{
				mapIndex.insert(pair<int ,string>(vecIndex[i], ""));
			}
		}
	}


}

bool ConfigCatalog::CatalogSingleLineConfig(wgt::MIF &inmif, size_t index, ConfigueItem configItems)
{
	int iconditionSize = configItems.conditions.size();
	if (iconditionSize <= 0)
	{
		return true;
	}

	bool boMeet = true;
	for (int i = 0; i < iconditionSize; i++)
	{
		boMeet = MeetCondition2(inmif, index, configItems.conditions[i]);
		if (boMeet == false)
		{
			break;
		}
	}
	return boMeet;
}

bool ConfigCatalog::MeetCondition(wgt::MIF &inmif, size_t index, CellCondition condition)
{
	string strFieldName = condition.field.field;
	if (strFieldName.empty() == true)
	{
		return false;
	}
	int ifieldIndex = inmif.get_col_pos(strFieldName);
	if (ifieldIndex == -1)
	{
		return false;
	}
	string fieldValue = wgt::tolower(inmif.mid[index][ifieldIndex]);
	fieldValue = wgt::trim(fieldValue, ' ');
	fieldValue = fix_quot_main2(fieldValue);

	vector<string> vecItems;
	ParseString(vecItems, fieldValue, OR, true);
	if (vecItems.size() > 0)
	{
		fieldValue = vecItems[0];
	}

	string configValue = condition.field.value;
	configValue = wgt::tolower(configValue);

	vector<string> vecConfig;
	//如果包含有逗号或者分号  
	if (configValue.find(",", 0) != NOPOSITION)
	{
		ParseString(vecConfig, configValue, ",", true);
	}
	else if (configValue.find(SEMICOLON, 0) != NOPOSITION)
	{
		ParseString(vecConfig, configValue, SEMICOLON, true);
	}
	else
	{
		vecConfig.push_back(configValue);
	}

	bool boMeet = false;

	int vecConfigSize = vecConfig.size();
	string strCongSingleItem;
	for (int i = 0; i < vecConfigSize; i++)
	{
		strCongSingleItem = vecConfig[i];
		if (condition.enumCondition == fieldequal)
		{
			if (strCongSingleItem == fieldValue)
			{
				boMeet = true;
				break;
			}
		}
		else if (condition.enumCondition == notequal)
		{
			if (strCongSingleItem != fieldValue)
			{
				boMeet = true;
				break;
			}
		}
		else if (condition.enumCondition == contain)
		{
			if (fieldValue.find(strCongSingleItem) != NOPOSITION)
			{
				boMeet = true;
				break;
			}
		}
		else if (condition.enumCondition == prefix)
		{
			if (wgt::startswith(fieldValue, strCongSingleItem) == true)
			{
				boMeet = true;
				break;
			}
		}
		else if (condition.enumCondition == suffix)
		{
			if (wgt::endswith(fieldValue, strCongSingleItem) == true)
			{
				boMeet = true;
				break;
			}
		}

	}


	return boMeet;
}


bool ConfigCatalog::MeetCondition2(wgt::MIF &inmif, size_t index, CellCondition condition)
{
	string strFieldName = condition.field.field;
	if (strFieldName.empty() == true)
	{
		return false;
	}
	int ifieldIndex = inmif.get_col_pos(strFieldName);
	if (ifieldIndex == -1)
	{
		return false;
	}
	string strfieldValue = wgt::tolower(inmif.mid[index][ifieldIndex]);
	strfieldValue = wgt::trim(strfieldValue, ' ');
	strfieldValue = fix_quot_main2(strfieldValue);

	vector<string> vecItems;
	string strSeprator = "";
	/*if (strfieldValue.find(SEMICOLON) != NOPOSITION)
	{
		strSeprator = SEMICOLON;
	}*/
	if (strfieldValue.find(OR) != NOPOSITION)
	{
		strSeprator = OR;
	}
	else
	{
		strSeprator = "";
	}
	if (!strSeprator.empty())
	{
		ParseString(vecItems, strfieldValue, strSeprator, true);
	}
	else
	{
		vecItems.push_back(strfieldValue);
	}

	string configValue = condition.field.value;
	configValue = wgt::tolower(configValue);

	vector<string> vecConfig;
	//如果包含有逗号或者分号  
	if (configValue.find(",", 0) != NOPOSITION)
	{
		ParseString(vecConfig, configValue, ",", true);
	}
	/*else if (configValue.find(SEMICOLON, 0) != NOPOSITION)
	{
		ParseString(vecConfig, configValue, SEMICOLON, true);
	}*/
	else
	{
		vecConfig.push_back(configValue);
	}

	bool boMeet = false;

	int vecConfigSize = vecConfig.size();
	string strCongSingleItem;

	int vecValueSize = vecItems.size();
	string fieldValue;
	for (int k = 0; k < vecValueSize; k++)
	{
		fieldValue = vecItems[k];
		for (int i = 0; i < vecConfigSize; i++)
		{
			strCongSingleItem = vecConfig[i];
			if (condition.enumCondition == fieldequal)
			{
				if (strCongSingleItem == fieldValue)
				{
					boMeet = true;
					break;
				}
			}
			else if (condition.enumCondition == notequal)
			{
				if (strCongSingleItem != fieldValue)
				{
					boMeet = true;
					break;
				}
			}
			else if (condition.enumCondition == contain)
			{
				if (fieldValue.find(strCongSingleItem) != NOPOSITION)
				{
					boMeet = true;
					break;
				}
			}
			else if (condition.enumCondition == prefix)
			{
				if (wgt::startswith(fieldValue, strCongSingleItem) == true)
				{
					boMeet = true;
					break;
				}
			}
			else if (condition.enumCondition == suffix)
			{
				if (wgt::endswith(fieldValue, strCongSingleItem) == true)
				{
					boMeet = true;
					break;
				}
			}
		}

		if (boMeet)
		{
			break;
		}
	}

	return boMeet;
}

void ConfigCatalog::AssignConfigFieldsValue(wgt::MIF &inmif, size_t index, std::vector<Field> fields)
{
	int ifieldSize = fields.size();
	if (ifieldSize <= 0)
	{
		return;
	}
	string fieldname;
	string fieldvalue;
	int ifieldIndex = -1;
	string oldValue;
	Field singleField;
	for (int i = 0; i < ifieldSize; i++)
	{
		singleField = fields[i];
		AssignField(inmif, index, singleField);
	}
}

void ConfigCatalog::AssignField(wgt::MIF &inmif, size_t index, Field singlefield)
{
	string fieldname = singlefield.field;
	string fieldvalue = singlefield.value;

	//如果是空间坐标字段
	if (fieldname == "x")
	{
		float x = atof(fieldvalue.c_str());
		inmif.data.geo_vec[index]->at(0).at(0).setx(x);
		return;
	}
	else if (fieldname == "y")
	{
		float y = atof(fieldvalue.c_str());
		inmif.data.geo_vec[index]->at(0).at(0).sety(y);
		return;
	}


	int ifieldIndex = inmif.get_col_pos(fieldname);
	if (ifieldIndex == -1)
	{
		if (fieldname == "catalog")
		{
			inmif.add_column(fieldname, "char(32)", inmif.header.col_num);
			ifieldIndex = inmif.get_col_pos(fieldname);
		}
		else
		{
			inmif.add_column(fieldname, "char(64)", inmif.header.col_num);
			ifieldIndex = inmif.get_col_pos(fieldname);
		}

	}
	if (ifieldIndex == -1)
	{
		sys_log_println(_WARN, "catalog assign value,add field name fail: %s !\n", fieldname.c_str());
		return;
	}

	if (wgt::startswith(fieldvalue, "\""))
	{
		inmif.mid[index][ifieldIndex] = fieldvalue;
	}
	else
	{
		inmif.mid[index][ifieldIndex] = "\"" + fieldvalue + "\"";
	}

}

void ConfigCatalog::GetMD5(std::string mifname, int id, std::string &randstr)
{
	char strtemp[128];
#ifdef LINUX
	snprintf(strtemp, 128, "%s%s%s%d", DATAVERSION, strCityName.c_str(), mifname.c_str(), id);
#else
	sprintf_s(strtemp, "%s%s%s%d", DATAVERSION, strCityName.c_str(), mifname.c_str(), id);
#endif

	MD5 m_md5temp;
	char blankmd5[33];
	m_md5temp.md5_passwd(strtemp, blankmd5, sizeof(strtemp));
	//printf("blankmd5 is %s\n",blankmd5);
	randstr = blankmd5;
}

void ConfigCatalog::RecordLayerKeyCondition(std::string layer, std::string key)
{
	layer = wgt::trim(layer, BLANK);
	if (layer.empty())
	{
		return;
	}
	key = wgt::tolower(key);

	vector<KeyWordTime>& vectorKeys = layerKeys[layer];
	int isize = vectorKeys.size();
	bool hasExit = false;
	for (int i = 0; i < isize; i++)
	{
		if (vectorKeys[i].key == key)
		{
			if (key == ALLCONDITION)
			{
				vectorKeys[i].time = 0;  //保证all的，即没有设置条件的，在最后
			}
			else if (key == OTHERS)
			{
				vectorKeys[i].time = 1;
			}
			else
			{
				vectorKeys[i].time++;
			}

			hasExit = true;
			break;
		}
	}
	if (hasExit)
	{
		return;
	}
	KeyWordTime keytime;
	keytime.key = key;
	keytime.time = 1;

	vectorKeys.push_back(keytime);
}

void ConfigCatalog::RecordLayerFields(std::string layer, std::string fieldname)
{
	layer = wgt::trim(layer, BLANK);
	if (layer.empty())
	{
		return;
	}
	fieldname = wgt::tolower(fieldname);

	vector<string>& vectorFields = layerFields[layer];
	int isize = vectorFields.size();

	bool hasExit = false;
	for (int i = 0; i < isize; i++)
	{
		if (vectorFields[i] == fieldname)
		{
			hasExit = true;
			break;
		}
	}
	if (hasExit)
	{
		return;
	}
	vectorFields.push_back(fieldname);
}

void ConfigCatalog::CheckFiledNames(wgt::MIF &inmif, std::string layername)
{
	if (layername.empty())
	{
		return;
	}
	vector<string>& vecFields = layerFields[layername];
	int ifieldSize = vecFields.size();
	if (ifieldSize == 0)
	{
		return;
	}
	string strFieldName;
	for (int i = 0; i < ifieldSize; i++)
	{
		strFieldName = vecFields[i];
		strFieldName = wgt::tolower(strFieldName);
		if (strFieldName.empty() || strFieldName == "x" || strFieldName == "y")
			continue;
		int ifieldIndex = inmif.get_col_pos(strFieldName);
		if (ifieldIndex >= 0)
			continue;
		if (strFieldName == "catalog")
		{
			inmif.add_column(strFieldName, "char(32)", inmif.header.col_num);
		}
		else
		{
			inmif.add_column(strFieldName, "char(64)", inmif.header.col_num);
		}

	}
}
int ConfigCatalog::Road_Catalog(string in_path, string out_path, string layername)
{
	wgt::MIF fn;

	ostringstream pclass;
	ostringstream outfile;

	if (wgt::mif_to_wsbl(in_path, fn) < 0)
	{
		std::cerr << "open " << in_path << " failed" << std::endl;
		return -1;
	}

	//Road_Rank(fn, in_path); //赋道路rank

	int pos_kind = fn.get_col_pos("kind");
	int pos_pathclass = fn.get_col_pos("funcclass");
	//int pos_name = fn.get_col_pos("pathname");
	int pos_name = fn.get_col_pos("sign_name");
	int pos_pathname = fn.get_col_pos("pathname");
	//int pos_rank = fn.get_col_pos("rank");	
	if (pos_kind == -1 || pos_pathclass == -1 || pos_name == -1 || pos_pathname == -1)
	{
		sys_log_println(_ERROR, "get kind pathclass pathname rank failed %s\r\n", in_path.c_str());
		return -1;
	}
	int pos_guid = fn.get_col_pos("guid");
	if (pos_guid == -1)
	{
		wgt::check_err("add guid", fn.add_column("guid", "char(64)", fn.header.col_num));
		pos_guid = fn.get_col_pos("guid");
		if (pos_guid == -1)
		{
			sys_log_println(_ERROR, "get rank field failed %s\r\n", in_path.c_str());
			return -1;

		}
	}
	int pos_catalog = fn.get_col_pos("catalog");
	if (pos_catalog == -1)
	{
		wgt::check_err("add catalog", fn.add_column("catalog", "Char(30)", fn.header.col_num));
		pos_catalog = fn.get_col_pos("catalog");
		if (pos_catalog == -1)
		{
			sys_log_println(_ERROR, "get catalog field failed %s\r\n", in_path.c_str());
			return -1;
		}
	}
	string passcode;
	size_t fsz(fn.mid.size());
	for (size_t i = 0; i < fsz; ++i)
	{
		//增加名称关联，如果pathname为空，而signname不为空，则拷贝signname到pathname
		if (wgt::trim(fn.mid[i][pos_pathname], '"') == "")
		{
			fn.mid[i][pos_pathname] = fn.mid[i][pos_name];
		}

		RouteKinds rk(fn.mid[i][pos_kind]);

		//c_ferryline
		if (rk.road_class() == "0a")
		{
			fn.mid[i][pos_catalog] = "\"0C06\"";
			//赋值guid
			string  randstr = "";
			//m_catalog->GetRandStr(16,randstr,fsz-i);
			//fn.mid[i][pos_guid]="\"SW_R"+randstr+"\"";
			//m_catalog->GetGuidStr("C_R",i,randstr);
			GetMD5(layername, i, randstr);
			fn.mid[i][pos_guid] = "\"" + randstr + "\"";

			continue;
		}

		string temp(fn.mid[i][pos_pathclass]);
		passcode = "0C0" + temp.substr(1, temp.length() - 2);

		//虚拟内部路可以不选取，直接过滤掉
		if (rk.is<0x18>())
		{
			passcode += "14";
		}

		//区域内道路属性,需要优先选取
		else if (rk.is_road_in_area() || rk.is<0x19>() || rk.is<0x1a>())
		{
			passcode += rk.is_ud_dep() ? "12" : "11";
		}
		//jct
		else if (rk.is_jct())
		{
			if (rk.road_class() == "00") //高速
			{
				passcode = "0C080301";
			}
			else if (rk.road_class() == "01") //城高
			{
				passcode = "0C080302";
			}
			else
			{
				sys_log_println(_ERROR, "KIND ERROR %s\r\n", rk.attr_str().c_str());
			}
		}
		else if (rk.is_sa() || rk.is_pa()) //SAPA
		{
			if (rk.road_class() == "00") //高速
			{
				passcode = "0C080401";
			}
			else if (rk.road_class() == "01") //城高
			{
				passcode = "0C080402";
			}
			else
			{
				sys_log_println(_ERROR, "KIND ERROR %s\r\n", rk.attr_str().c_str());
			}
		}
		//环岛
		else if (rk.is_island())
		{
			passcode += "01";
		}
		//辅路
		else if (rk.is_side_road())
		{
			passcode += "02";
		}
		//ic
		else if (rk.is_ic() && !rk.is_ud_dep())
		{
			passcode += "04";
		}
		//ramp
		else if (rk.is_ramp() /*|| rk.is_jct()*/ || rk.is_turn_right())
		{
			passcode += "05";
		}
		//inlink
		else if (rk.is_in_link() && !rk.is_ud_dep())
		{
			passcode += "06";
		}
		else if (rk.road_class() == "0b")//种别：行人道路
		{
			passcode += "13";
		}

		//双线 高速			
		else if (rk.is_ud_dep())
		{
			if (rk.road_class() == "00")
			{
				if (fn.mid[i][pos_name][1] == 'G'&& IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080104";
				else if (fn.mid[i][pos_name][1] == 'S'&& IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080103";
				else
					passcode = "0c080101";
			}
			else if (rk.road_class() == "01")
			{
				if (fn.mid[i][pos_name][1] == 'G'&& IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080204";
				else if (fn.mid[i][pos_name][1] == 'S'&& IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080203";
				else
					passcode = "0c080201";
			}
			//双线 国道
			else if (fn.mid[i][pos_name][1] == 'G')
			{
				if (fn.mid[i][pos_name].size() == 6
					&& IsNum(fn.mid[i][pos_name], 2, 3)) //"G101" 国道
				{
					passcode += "08";
				}
				else //G101辅路
				{
					passcode += "0B";
				}
			}
			//双线 省道
			else if (fn.mid[i][pos_name][1] == 'S')
			{
				if (fn.mid[i][pos_name].size() == 6
					&& IsNum(fn.mid[i][pos_name], 2, 3)) //"S101" 省道
				{
					passcode += "09";
				}
				else //S101线
				{
					passcode += "0B";
				}
			}
			//双线 县道
			// else if(fn.mid[i][pos_name][1]  == 'X' && IsNum(fn.mid[i][pos_name],2,fn.mid[i][pos_name].size()-3))
			else if (isValidBrandName<'X'>(fn.mid[i][pos_name]))
			{
				passcode += "0A";
			}
			//双线 其他
			else
			{
				passcode += "0B";
			}
		}
		//单线 高速
		else
		{
			if (rk.road_class() == "00")
			{
				if (fn.mid[i][pos_name][1] == 'G'&&IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080104";
				else if (fn.mid[i][pos_name][1] == 'S'&&IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080103";
				else
					passcode = "0c080102";
			}
			else if (rk.road_class() == "01")
			{
				if (fn.mid[i][pos_name][1] == 'G'&&IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080204";
				else if (fn.mid[i][pos_name][1] == 'S'&&IsNum(fn.mid[i][pos_name], 2, fn.mid[i][pos_name].size() - 3))
					passcode = "0c080203";
				else
					passcode = "0c080202";
			}
			//单线 国道
			else if (fn.mid[i][pos_name][1] == 'G')
			{
				if (fn.mid[i][pos_name].size() == 6
					&& IsNum(fn.mid[i][pos_name], 2, 3)) //"G101" 省道
				{
					passcode += "0D";
				}
				else //G101辅路
				{
					passcode += "0B";
				}
			}
			//单线 省道
			else if (fn.mid[i][pos_name][1] == 'S')
			{
				if (fn.mid[i][pos_name].size() == 6
					&& IsNum(fn.mid[i][pos_name], 2, 3)) //"S101" 省道
				{
					passcode += "0E";
				}
				else //S101线
				{
					passcode += "0B";
				}
			}
			//单线 县道
			// else if( fn.mid[i][pos_name][1] == 'X' && IsNum(fn.mid[i][pos_name],2,fn.mid[i][pos_name].size()-3))
			else if (isValidBrandName<'X'>(fn.mid[i][pos_name]))
			{
				passcode += "0F";
			}
			//单线 其他
			else
			{
				passcode += "10";
			}
		}

		fn.mid[i][pos_catalog] = "\"" + wgt::toupper(passcode) + "\"";

		//赋值guid
		string  randstr = "";
		//m_catalog->GetRandStr(16,randstr,fsz-i);
		//fn.mid[i][pos_guid]="\"SW_R"+randstr+"\"";
		GetMD5(layername, i, randstr);
		fn.mid[i][pos_guid] = "\"" + randstr + "\"";
	}
	wgt::check_err("Write back to Road MIF/MID", wgt::wsbl_to_mif(fn, out_path));
	return 0;
}

bool In_built_up_Areas(std::vector<wsl::Geometry*>& areas, wsl::Geometry* line)
{
	for (size_t k=0; k<areas.size(); k++)
	{
		int inter_ret = wsl::intersect(line, areas[k]);
		if ( inter_ret==wsl::CONTAIN || inter_ret==wsl::INTERSECT )
		{
			return true;
		}
	}

	return false;	
}

void ConfigCatalog::ProcessRoadDirection(wgt::MIF& InMif)
{
	int dirID = InMif.get_col_pos("direction");
	int snodID = InMif.get_col_pos("snodeid");
	int enodID = InMif.get_col_pos("enodeid");
	if (dirID == -1 || snodID == -1 || enodID == -1)
	{
		sys_log_println(_ERROR, "ProcessRoadDirection C_R snodeid enodeid error!\n");
	}

	for (int i=0 ;i<InMif.data.geo_vec.size(); i++) //经纬度转莫卡托
	{
		wsl::Geometry* geo = InMif.data.geo_vec[i];
		int part = geo->size();

		string dirstr = InMif.mid[i][dirID];
		int dir = atoi(wgt::trim(dirstr,'"').c_str());
		if (dir == 3)  // 将方向为3的坐标进行逆序存储
		{
			InMif.mid[i][dirID] = "2";
			string snodestr = InMif.mid[i][snodID];			 
			string enodestr = InMif.mid[i][enodID];
			InMif.mid[i][enodID] = snodestr;
			InMif.mid[i][snodID] = enodestr;

			for (int j=0;j<part;j++)
			{
				int pcount =(*geo)[j].size();
				vector<wsl::coor::dpoint_t> tempdpvector ;
				tempdpvector.clear();
				for (int k =0 ;k< pcount; k++)
				{
					wsl::coor::dpoint_t ptMeterLonglat,ptLonglat;
					ptLonglat.x = geo->at(j).at(k).x();
					ptLonglat.y =  geo->at(j).at(k).y();
					tempdpvector.push_back(ptLonglat);
				}
				for (int k=0 ; k<pcount;k++)
				{
					wsl::coor::dpoint_t tempdp = tempdpvector [pcount-1-k];
					geo->at(j).at(k).setx(tempdp .x);
					geo->at(j).at(k).sety( tempdp.y);
				}
			}
		}
	}
}

int ConfigCatalog::Road_CatalogEx(string in_path, string out_path, string layername, string backPolygonfile)
{
	// step1: add "kind_class"字段(char(2))
	wgt::MIF road_Mif, roadTemp;
	if (wgt::mif_to_wsbl(in_path, roadTemp) < 0)
	{
		sys_log_println(_ERROR, "Road_CatalogEx read road mif error! %s\n", in_path.c_str());
		return -1;
	}

	std::vector<wsl::Geometry*> built_up_geo_vec;
	wgt::MIF backTemp;
	if (wgt::mif_to_wsbl(backPolygonfile, backTemp) > 0)
	{
		int col_backkind = backTemp.get_col_pos("kind");
		if (col_backkind != -1)
		{
			int nSizeback = backTemp.mid.size();
			for (int index = 0; index < nSizeback; index++)
			{
				string str_kind = backTemp.mid[index][col_backkind];
				wgt::trim(str_kind, '"');
				if (str_kind == "0133") //建成区
				{
					built_up_geo_vec.push_back(backTemp.data.geo_vec[index]->clone());
				}
			}
			wsl::clear_geovec(backTemp.data.geo_vec);
			backTemp.mid.clear();
		}
	}

	// 反转逆序的道路
	ProcessRoadDirection(roadTemp);
	
	int col_admincodel = roadTemp.get_col_pos("AdminCodeL");
	int col_kind = roadTemp.get_col_pos("kind");
	int col_funclass = roadTemp.get_col_pos("funcclass");
	int col_sign_name = roadTemp.get_col_pos("sign_name");
	int col_pathname = roadTemp.get_col_pos("pathname");
	int col_through = roadTemp.get_col_pos("through");
	int col_constST = roadTemp.get_col_pos("const_st");		// 供用信息
	int col_uflag =  roadTemp.get_col_pos("uflag");	 //城市标识，区分市区和非市区
	if (col_kind == -1 || col_funclass == -1 || col_sign_name == -1 || col_through == -1 || col_constST == -1)
	{
		sys_log_println(_ERROR, "Road_CatalogEx C_R col_kind col_funcclass error!\n");
		return -1;
	}
	int col_kindclass = roadTemp.get_col_pos("kind_class");
	if (col_kindclass == -1)
	{
		wgt::check_err("add kind_class", roadTemp.add_column("kind_class", "Char(2)", roadTemp.header.col_num));
		col_kindclass = roadTemp.get_col_pos("kind_class");
		if (col_kindclass == -1)
		{
			sys_log_println(_ERROR, "get catalog field failed %s\r\n", in_path.c_str());
			return -1;
		}
	}
	int col_build_in_flag = roadTemp.get_col_pos("build_in_flag");
	if (col_build_in_flag == -1)
	{
		wgt::check_err("add build_in_flag", roadTemp.add_column("build_in_flag", "Char(2)", roadTemp.header.col_num));
		col_build_in_flag = roadTemp.get_col_pos("build_in_flag");
		if (col_build_in_flag == -1)
		{
			sys_log_println(_ERROR, "get catalog field failed %s\r\n", in_path.c_str());
			return -1;
		}
	}
	sys_log_println(_INFORANK, "Road_CatalogEx step1 finish\r\n");

	road_Mif.header = roadTemp.header;
	// add 删除道路属性: 虚拟链接路(18)
	int nSizeTemp = roadTemp.mid.size();
	for (int index = 0; index < nSizeTemp; index++)
	{
		string str_kind = roadTemp.mid[index][col_kind];
		string str_through = roadTemp.mid[index][col_through];
		wgt::trim(str_kind, '"');
		wgt::trim(str_through, '"');
		if (!(needDelete(str_kind) || ((str_kind == "060f" || str_kind == "080f") && str_through == "0")))
		{
			road_Mif.mid.push_back(roadTemp.mid[index]);
			road_Mif.data.geo_vec.push_back(roadTemp.data.geo_vec[index]->clone());
		}
	}
	wsl::clear_geovec(roadTemp.data.geo_vec);
	roadTemp.mid.clear();
	//end
	// step2: 根据kind值计算kind_class 
	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_kind_class = "", str_kind_classBak = "";
		string str_AdminCodeL = road_Mif.mid[index][col_admincodel];
		string str_kinds = road_Mif.mid[index][col_kind];
		string str_funcclass = road_Mif.mid[index][col_funclass];
		string str_constST = road_Mif.mid[index][col_constST];
		wgt::trim(str_kinds, '"');
		wgt::trim(str_funcclass, '"');
		wgt::trim(str_constST, '"');
		str_kind_class = GetRoadLevelFromKind(str_kinds);
		str_kind_classBak = str_kind_class;
		// 根据funcclass调整kind_class
		str_kind_class = AdjustmentKindClass(str_AdminCodeL, str_kind_class, str_funcclass);

		// 施工道路，kind_class为"OC"
		// 2016.3.22 SW新增3未供用
		//if (str_constST == "4" || str_constST == "3")
		//{
		//	str_kind_class = "0c";
	//}
		road_Mif.mid[index][col_kindclass] = str_kind_class;

		string str_build_in_flag = "";
		if ( In_built_up_Areas(built_up_geo_vec, road_Mif.data.geo_vec[index]) )
		{ 
			str_build_in_flag = "01";
		}
		else
		{
			str_build_in_flag = "02";
		}
		road_Mif.mid[index][col_build_in_flag] = str_build_in_flag;
		//road_Mif.mid[index][col_build_in_flag] = "01";
	}
	sys_log_println(_INFORANK, "Road_CatalogEx step2 finish\r\n");

	// step3 服务区、停车区降级
	ProcessSAKindClass(road_Mif, in_path);
	sys_log_println(_INFORANK, "Road_CatalogEx step3 finish\r\n");

	// step4 匝道（IC、JCT、匝道、提前左转、提前右转、掉头口、主辅出入口) kindclass等于所连接道路（排除行人道路、轮渡）的kindclass的次低级
	ProcessRampKindClass(road_Mif, in_path);
	sys_log_println(_INFORANK, "Road_CatalogEx step4 finish\r\n");

	CheckKindClassConnectivity(road_Mif, in_path);

	ProcessBuildInFlag(road_Mif, in_path);

	// step6 catalog值
	int col_guid = road_Mif.get_col_pos("guid");
	if (col_guid == -1)
	{
		wgt::check_err("add guid", road_Mif.add_column("guid", "char(64)", road_Mif.header.col_num));
		col_guid = road_Mif.get_col_pos("guid");
		if (col_guid == -1)
		{
			sys_log_println(_ERROR, "get col_guid field failed %s\r\n", in_path.c_str());
			return -1;

		}
	}
	int col_catalog = road_Mif.get_col_pos("catalog");
	if (col_catalog == -1)
	{
		wgt::check_err("add catalog", road_Mif.add_column("catalog", "Char(30)", road_Mif.header.col_num));
		col_catalog = road_Mif.get_col_pos("catalog");
		if (col_catalog == -1)
		{
			sys_log_println(_ERROR, "get catalog field failed %s\r\n", in_path.c_str());
			return -1;
		}
	}

	nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string catalog = "0C";
		string kind = road_Mif.mid[index][col_kind];
		string kind_class = road_Mif.mid[index][col_kindclass];
		string constST = road_Mif.mid[index][col_constST];
		string sign_name =  road_Mif.mid[index][col_sign_name];
		string build_in_flag =  road_Mif.mid[index][col_build_in_flag];
		wgt::trim(kind, '"');
		wgt::trim(kind_class, '"');
		wgt::trim(constST, '"');
		wgt::trim(build_in_flag, '"');

		//增加名称关联，如果pathname为空，而signname不为空，则拷贝signname到pathname
// 		if(wgt::trim(road_Mif.mid[index][col_pathname],'"') == "" )
// 		{
// 			road_Mif.mid[index][col_pathname] = road_Mif.mid[index][col_sign_name];
// 		}
		bool isConstruction = constST == "4" || constST == "3";
		bool isUrban = build_in_flag == "01";

		if (isWalkstreet(kind))
		{	// 步行街
			catalog += "0803";
			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else if (kind_class == "00")
		{
			// 高速
			catalog += "01";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "03";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "04";
			}
			// 			else if((road_Mif.mid[index][col_sign_name][1]=='G'&& IsNum(road_Mif.mid[index][col_sign_name],2,road_Mif.mid[index][col_sign_name].size()-3)))
			// 			{
			// 				// 编号为"G"开头或者为空
			// 				catalog += "01";
			// 
			// 			}
			else if (!sign_name.empty() && sign_name[1] == 'S'&& IsNum(sign_name, 2, sign_name.size() - 3))
			{
				// 编号为"S"开头或者为空
				catalog += "02";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{	// 其他
				catalog += "01";
			}

			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else if (kind_class == "01")
		{
			// 快速
			catalog += "02";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "04";
			}
			else
			{
				// 其他
				catalog += "01";
			}

			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else if (kind_class == "02")
		{
			// 国道
			catalog += "03";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isFulu(kind))
			{
				// 辅路
				catalog += "04";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{
				// 其他
				catalog += "01";
			}
						
			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else if (kind_class == "03")
		{
			// 省道
			catalog += "04";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isFulu(kind))
			{
				// 辅路
				catalog += "04";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{
				// 其他
				catalog += "01";
			}

			if (isUrban)
			{
				if (isConstruction)
				{
					catalog += "03";
				}
				else
				{
					catalog += "01";
				}
			}
			else
			{
				if (isConstruction)
				{
					catalog += "04";
				}
				else
				{
					catalog += "02";
				}
			}
		}
		else if (kind_class == "04")
		{
			// 县道
			catalog += "05";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isFulu(kind))
			{
				// 辅路
				catalog += "04";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{
				// 其他
				catalog += "01";
			}

			if (isUrban)
			{
				if (isConstruction)
				{
					catalog += "03";
				}
				else
				{
					catalog += "01";
				}
			}
			else
			{
				if (isConstruction)
				{
					catalog += "04";
				}
				else
				{
					catalog += "02";
				}
			}
		}
		else if (kind_class == "06")
		{
			// 乡镇村道
			catalog += "06";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isFulu(kind))
			{
				// 辅路
				catalog += "04";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{
				// 其他
				catalog += "01";
			}

			if (isUrban)
			{
				if (isConstruction)
				{
					catalog += "03";
				}
				else
				{
					catalog += "01";
				}
			}
			else
			{
				if (isConstruction)
				{
					catalog += "04";
				}
				else
				{
					catalog += "02";
				}
			}
		}
		else if (kind_class == "08" || kind_class == "09")
		{
			// 其他道路
			catalog += "07";
			if (isTunnel(kind))
			{
				// 隧道
				catalog += "02";
			}
			else if (isBridge(kind))
			{
				// 桥
				catalog += "03";
			}
			else if (isFulu(kind))
			{
				// 辅路
				catalog += "04";
			}
			else if (isRamp(kind))
			{	// JCT
				catalog += "05";
			}
			else
			{
				// 其他
				catalog += "01";
			}

			if (isUrban)
			{
				if (isConstruction)
				{
					catalog += "03";
				}
				else
				{
					catalog += "01";
				}
			}
			else
			{
				if (isConstruction)
				{
					catalog += "04";
				}
				else
				{
					catalog += "02";
				}
			}
		}
		else if (kind_class == "0a" || 
			kind_class == "0c")
		{
			// 轮渡
			catalog += "0801";
		}
		else if (kind_class == "0b")
		{
			// 行人道路
			catalog += "0802";
			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else if (kind_class == "0d")
		{
			// 自行车专用道
			catalog += "0904";
			if (isConstruction)
			{
				catalog += "03";
			}
			else
			{
				catalog += "01";
			}
		}
		else
		{
			catalog = "FFFFFF";
			sys_log_println(_ERROR, "no catalog kind:%s kind_class:%s\n", kind.c_str(), kind_class.c_str());
		}

		road_Mif.mid[index][col_catalog] = catalog;

		//赋值guid
		string  randstr = "";
		//m_catalog->GetRandStr(16,randstr,fsz-i);
		//fn.mid[i][pos_guid]="\"SW_R"+randstr+"\"";
		GetMD5(layername, index, randstr);
		road_Mif.mid[index][col_guid] = "\"" + randstr + "\"";
	}

	SmoothRoadCatalog(road_Mif);

	// write new C_R data
	return wgt::wsbl_to_mif(road_Mif, out_path) == 0;
}

// 从kind中获取最大的roadlevel字段
string ConfigCatalog::GetRoadLevelFromKind(string str_kind)
{
	int level = 100;
	string strlevel = "";
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string roadLevel = kind.substr(0, 2);
		int curlevel = (int)strtol(roadLevel.c_str(), NULL, 16);
		if (curlevel < level)
		{
			level = curlevel;
			strlevel = roadLevel;
		}
	}
	return strlevel;
}

string ConfigCatalog::AdjustmentKindClass(string str_AdminCode, string str_kind_class, string str_funcclass)
{
	str_AdminCode = str_AdminCode.substr(0, 4) + "00";
	map<string, string>::iterator iter = kindClassAdjustMap.find(str_AdminCode + "_" + str_kind_class + "_" + str_funcclass);
	if (iter != kindClassAdjustMap.end())
	{
		str_kind_class = iter->second;
	}
	else
	{
		iter = kindClassAdjustMap.find("_" + str_kind_class + "_" + str_funcclass);
		if (iter != kindClassAdjustMap.end())
		{
			str_kind_class = iter->second;
		}
	}
	return str_kind_class;
}

// 传入sw的kind，判断当前是否为匝道(IC、JCT、匝道、提前左转、提前右转、掉头口、主辅出入口)
bool ConfigCatalog::isRamp(string str_kind)
{
	vector<string> kindvec;
	map<string, string> kind2map;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		kind2map[rattr] = "";
		if (rattr == "0b"	// 匝道
			|| rattr == "12"	// 提前右转
			|| rattr == "15"	// 提前左转
			|| rattr == "16"	// 掉头口
			|| rattr == "17"	// 主辅路出入口
			)
		{
			return true;
		}
	}
	// IC和JCT需要同时满足: [03、0b或03、17（JCT连接路）]或[05、0b或05、17（IC连接路）]
	if (kind2map.find("03") != kind2map.end() && (kind2map.find("0b") != kind2map.end() || kind2map.find("17") != kind2map.end()))
	{
		return true;
	}
	if (kind2map.find("05") != kind2map.end() && (kind2map.find("0b") != kind2map.end() || kind2map.find("17") != kind2map.end()))
	{
		return true;
	}

	// IC且非封闭，认为是匝道
	if (kind2map.find("05") != kind2map.end() && kind2map.find("0c") == kind2map.end())
	{
		return true;
	}
	return false;
}

bool ConfigCatalog::isJCT(string str_kind)
{
	vector<string> kindvec;
	map<string, string> kind2map;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		kind2map[rattr] = "";
	}
	// IC和JCT需要同时满足: [03、0b或03、17（JCT连接路）]或[05、0b或05、17（IC连接路）]
	if (kind2map.find("03") != kind2map.end() && (kind2map.find("0b") != kind2map.end() || kind2map.find("17") != kind2map.end()))
	{
		return true;
	}
	return false;
}

bool ConfigCatalog::isICJCT(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "05"			// IC
			|| rattr == "03")		// JCT
		{
			return true;
		}
	}
	return false;
}

bool ConfigCatalog::isOnlyRamp(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "0b")	// 匝道
		{
			return true;
		}
	}
	return false;
}

// 是否是服务区
bool ConfigCatalog::isSA(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "07"		// 服务区
			|| rattr == "06"	// 停车区
			)
		{
			return true;
		}
	}
	return false;
}

// 是否是交叉点内link
bool ConfigCatalog::isRLink(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "04")		// 交叉点内link
		{
			return true;
		}
	}
	return false;
}

// 是否是隧道
bool ConfigCatalog::isTunnel(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "0f")		// 隧道
		{
			return true;
		}
	}
	return false;
}

// 是否是“桥”
bool ConfigCatalog::isBridge(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "08" ||  // 固定桥
			rattr == "1b")		// 移动式桥
		{
			return true;
		}
	}
	return false;
}

bool ConfigCatalog::isFulu(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "0a")		// 辅路
		{
			return true;
		}
	}
	return false;
}

// 主辅路出入口
bool ConfigCatalog::isZhuFuluConnected(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "17")		// 主辅路出入口
		{
			return true;
		}
	}
	return false;
}

// 是否是步行街
bool ConfigCatalog::isWalkstreet(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(0, 2);
		string rattr2 = kind.substr(2, 2);
		// SW会将大部分轮渡的kind制作为"0a09"
		if (rattr != "0a" && rattr2 == "09")		// 步行街
		{
			return true;
		}
	}
	return false;
}


// kind是否相似
bool ConfigCatalog::isSameKind(string str_kind1, string str_kind2, string ignoreAttr)
{
	if (!isSameKindClass(str_kind1,str_kind2))
	{
		return false;
	}
	vector<string> kindvec1;
	vector<string> kindvec2;
	sys_splitString(str_kind1, kindvec1, '|');
	sys_splitString(str_kind2, kindvec2, '|');

	set<string> kindset1;
	for (vector<string>::iterator iter1 = kindvec1.begin(); iter1 != kindvec1.end(); ++iter1)
	{
		string kind1 = *iter1;
		if (kind1.size() != 4)
		{
			continue;
		}
		string rattr1 = kind1.substr(2, 2);
		if (rattr1 == ignoreAttr && rattr1 == "01")		// 忽略的道路属性
		{
			continue;
		}
		kindset1.insert(kind1);
	}

	set<string> kindset2;
	for (vector<string>::iterator iter2 = kindvec2.begin(); iter2 != kindvec2.end(); ++iter2)
	{
		string kind2 = *iter2;
		if (kind2.size() != 4)
		{
			continue;
		}
		string rattr2 = kind2.substr(2, 2);
		if (rattr2 == ignoreAttr && rattr2 == "01")		// 忽略的道路属性
		{
			continue;
		}
		kindset2.insert(kind2);
	}

	return kindset1 == kindset2;
}

bool ConfigCatalog::isSameKindClass(string str_kind1, string str_kind2r)
{
	return GetRoadLevelFromKind(str_kind1) == GetRoadLevelFromKind(str_kind2r);
}

// 删除道路
bool ConfigCatalog::needDelete(string str_kind)
{
	vector<string> kindvec;
	sys_splitString(str_kind, kindvec, '|');
	for (vector<string>::iterator iter = kindvec.begin(); iter != kindvec.end(); ++iter)
	{
		string kind = *iter;
		if (kind.size() != 4)
		{
			continue;
		}
		string rattr = kind.substr(2, 2);
		if (rattr == "18")			// 虚拟链接路
		{
			return true;
		}
	}
	return false;
}

void ConfigCatalog::ProcessRampKindClass(wgt::MIF& road_Mif, string in_path)
{
	wgt::MIF C_N_Mif;
	string city_path = in_path.substr(0, in_path.find_last_of('/'));
	string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
	string path = city_path.substr(0, city_path.find_last_of('/'));		// 用于后面城市扩展  ../data/01_basic/05_Recatalog
	string node_path = city_path + "/C_N";
	if (wgt::mif_to_wsbl(node_path, C_N_Mif) < 0)
	{
		sys_log_println(_ERROR, "ProceeRampKindClass read C_N mif error! %s\n", node_path.c_str());
		return;
	}
	// col index
	int col_linkmapid = road_Mif.get_col_pos("mapid");
	int col_linkid = road_Mif.get_col_pos("id");
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	int col_kind = road_Mif.get_col_pos("kind");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");
	int col_direction= road_Mif.get_col_pos("direction");
	int col_funcclass = road_Mif.get_col_pos("funcclass");

	int col_nodemapid = C_N_Mif.get_col_pos("mapid");
	int col_nodeid = C_N_Mif.get_col_pos("id");
	int col_adjoin_nid = C_N_Mif.get_col_pos("adjoin_nid");
	int col_node_lid = C_N_Mif.get_col_pos("node_lid");
	if (col_linkmapid == -1 || col_linkid == -1 || col_kindclass == -1 || col_kind == -1 || col_snodeid == -1 || col_enodeid == -1 ||
		col_nodemapid == -1 || col_nodeid == -1 || col_adjoin_nid == -1 || col_node_lid == -1)
	{
		sys_log_println(_ERROR, "ProcessRampKindClass MIF col error!\n");
		return;
	}

	map<string, int> linkid2indexmap;			// linkid ==> pos
	map<string, vector<string> > nodeid2lids;	// nodeid ==> linkids
	map<string, string> nodeid2adjoinnodeid;   // nodeid ==> adjoinnodeid

	int nSize2 = C_N_Mif.mid.size();
	for (int index = 0; index < nSize2; index++)
	{
		string str_nodeid = C_N_Mif.mid[index][col_nodeid];
		string str_adjoinid = C_N_Mif.mid[index][col_adjoin_nid];
		string str_linkids = C_N_Mif.mid[index][col_node_lid];
		wgt::trim(str_nodeid, '"');
		wgt::trim(str_adjoinid, '"');
		wgt::trim(str_linkids, '"');

		vector<string> res;
		sys_splitString(str_linkids, res, '|');

		map<string, vector<string> >::iterator ir1 = nodeid2lids.find(str_nodeid);
		map<string, vector<string> >::iterator ir2 = nodeid2lids.find(str_adjoinid);
		if (ir1 != nodeid2lids.end())
		{
			for (int i = 0; i < res.size(); i++)
			{
				ir1->second.push_back(res[i]);
			}
		}
		else
		{
			nodeid2lids[str_nodeid] = res;
		}
		if (str_adjoinid != "" && str_adjoinid != "0")
		{
			if (ir2 != nodeid2lids.end())
			{
				for (int i = 0; i < res.size(); i++)
				{
					ir2->second.push_back(res[i]);
				}
			}
			else
			{
				nodeid2lids[str_adjoinid] = res;
			}

			nodeid2adjoinnodeid[str_nodeid] = str_adjoinid;
		}
	}
	C_N_Mif.mid.clear();
	C_N_Mif.data.geo_vec.clear();

	map<string, string> citydirlist;		// 已经加载进来的城市
	citydirlist[cur_city] = "";

	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_linkid, '"');
		if (linkid2indexmap.find(str_linkid) != linkid2indexmap.end())
		{
			sys_log_println(_ASSERT, "linkid redefined %s\n", str_linkid.c_str());
		}
		linkid2indexmap[str_linkid] = index;
	}


	// 通过nodeid来找出所有与此匝道连接的道路
	nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string rampkindclass = road_Mif.mid[index][col_kindclass];
		string str_kinds = road_Mif.mid[index][col_kind];
		string rampfuncclass = road_Mif.mid[index][col_funcclass];
		wgt::trim(str_kinds, '"');
		wgt::trim(rampkindclass, '"');
		wgt::trim(rampfuncclass, '"');
		if (isRamp(str_kinds))
		{
			map<int, string> forwardkindclassmap;  //向前连接的kindclass值
			map<int, string> backwardkindclassmap; //向后连接的kindclass值

			map<int, string> kindclassmap_reliefroad;	// 连接的辅路kindclass值
			map<int, string> kindclassmap_direct;		// 与当前link直接连接的其他kindclass值
			
			for(int i=0;i<2;i++)
			{
				map<int, string> kindclassmap;				// 连接的kindclass值
				map<int, int> directindex_map;				// 直连的index pos
				map<int, int> directindexrlink_map;			// 直连的交叉点内link
				set<int> linkvisited;	// 已经访问的link
				queue<LinkItem> linkindexQueue;			// 存放road的下标

				LinkItem linkItem;
				linkItem.index = index;
				if(i==0)
				{
					// 第一遍从终点找
					linkItem.nodeid = road_Mif.mid[index][col_enodeid];
				}
				else
				{
					// 第二遍从起点找
					linkItem.nodeid = road_Mif.mid[index][col_snodeid];
				}
				wgt::trim(linkItem.nodeid, '"');

				linkindexQueue.push(linkItem);
				linkvisited.insert(linkItem.index);

				while (linkindexQueue.size() != 0)
				{
					LinkItem linkItem = linkindexQueue.front();
					linkindexQueue.pop();
					int pos = linkItem.index;
					if (pos >= road_Mif.mid.size())
					{
						sys_log_println(_ERROR, "pos(%d) >= C_R_Mif.mid.size(%d)\n", pos, road_Mif.mid.size());
					}

					string nodeid = linkItem.nodeid;
					string linkid = road_Mif.mid[pos][col_linkid];
					string kind = road_Mif.mid[pos][col_kind];
					string kindclass = road_Mif.mid[pos][col_kindclass];
					string funcclass = road_Mif.mid[pos][col_funcclass];
					wgt::trim(linkid, '"');
					wgt::trim(kind, '"');
					wgt::trim(kindclass, '"');
					wgt::trim(funcclass, '"');


					if (atoi(funcclass.c_str()) > atoi(rampfuncclass.c_str()))
					{
						continue;
					}

					if (!isRamp(kind))
					{
						// 排除 行人道路 和 轮渡
						if (kindclass != "0a" && kindclass != "0b" && kindclass != "0c")
						{
							// 有效路段
							kindclassmap[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
							if (isFulu(kind) && isZhuFuluConnected(str_kinds) && isSameKind(str_kinds,kind,"17"))
							{
								kindclassmap_reliefroad[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
							}
							// 直连link
							if (directindex_map.find(pos) != directindex_map.end())
							{
								kindclassmap_direct[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
							}
						}
					}
					else
					{	
						// JCT单独计算
						if (isJCT(str_kinds) && !isJCT(kind))
						{
							continue;
						}
						string mapid = road_Mif.mid[pos][col_linkmapid];
						string snodeid = road_Mif.mid[pos][col_snodeid];
						string enodeid = road_Mif.mid[pos][col_enodeid];
						string direction = road_Mif.mid[pos][col_direction];
						wgt::trim(mapid, '"');
						wgt::trim(snodeid, '"');
						wgt::trim(enodeid, '"');
						wgt::trim(direction, '"');

						if (nodeid == snodeid)
						{
							map<string, vector<string> >::iterator iter_find = nodeid2lids.find(snodeid);
							if (iter_find != nodeid2lids.end())
							{
								vector<string>& res = iter_find->second;
								for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
								{
									string lid = *iter1;
									if (linkid == lid)
									{
										continue;
									}

									map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
									if (iter2 != linkid2indexmap.end())
									{
										string snodeid_connect = road_Mif.mid[iter2->second][col_snodeid];
										string enodeid_connect = road_Mif.mid[iter2->second][col_enodeid];
										string direction_connect = road_Mif.mid[iter2->second][col_direction];
										wgt::trim(snodeid_connect, '"');
										wgt::trim(enodeid_connect, '"');
										wgt::trim(direction_connect, '"');

										//if(direction == "2")
										//{
										//	if(direction_connect == "2" && snodeid_connect == snodeid)
										//	{
										//		continue;
										//	}
										//	if(direction_connect == "3" && enodeid_connect == snodeid)
										//	{
										//		continue;
										//	}
										//}
										//if(direction == "3")
										//{
										//	if(direction_connect == "2" && enodeid_connect == snodeid)
										//	{
										//		continue;
										//	}
										//	if(direction_connect == "3" && snodeid_connect == snodeid)
										//	{
										//		continue;
										//	}
										//}

										LinkItem linkItem;
										linkItem.index = iter2->second;
										if( snodeid_connect == nodeid || nodeid2adjoinnodeid[snodeid_connect] == nodeid || nodeid2adjoinnodeid[nodeid] == snodeid_connect)
										{
											// 起点相同
											linkItem.nodeid = enodeid_connect;
										}
										else
										{
											// 终点相同
											linkItem.nodeid = snodeid_connect;
										}
										
										if (linkvisited.find(linkItem.index) == linkvisited.end())
										{
											linkindexQueue.push(linkItem);
											linkvisited.insert(linkItem.index);
										}

										// 当前所求link直连的非连接道路的link的kindclass
										if (index == pos || directindexrlink_map.find(pos) != directindexrlink_map.end())
										{
											directindex_map[iter2->second] = iter2->second;
											// 当前是否为交叉点内link
											string curkind = road_Mif.mid[iter2->second][col_kind];
											wgt::trim(curkind, '"');
											if (isRLink(curkind))
											{
												directindexrlink_map[iter2->second] = iter2->second;
											}
										}										
									}
									else
									{
										// 如果扩展后还是没找到呢
										sys_log_println(_ASSERT, "ProcessRampKindClass can not find lid: %s \n", lid.c_str());
									}
								}
							}
							else
							{
								sys_log_println(_ASSERT, "ProcessRampKindClass can not find snodeid: %s \n", snodeid.c_str());
							}
						}
						//////////////////////////////////////////////////////////////////////////////////////////
						if (nodeid == enodeid)
						{
							map<string, vector<string> >::iterator iter_find = nodeid2lids.find(enodeid);
							if (iter_find != nodeid2lids.end())
							{
								vector<string>& res = iter_find->second;
								for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
								{
									string lid = *iter1;
									
									map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
									if (iter2 != linkid2indexmap.end())
									{
										string snodeid_connect = road_Mif.mid[iter2->second][col_snodeid];
										string enodeid_connect = road_Mif.mid[iter2->second][col_enodeid];
										string direction_connect = road_Mif.mid[iter2->second][col_direction];
										wgt::trim(snodeid_connect, '"');
										wgt::trim(enodeid_connect, '"');
										wgt::trim(direction_connect, '"');

										//if(direction == "2")
										//{
										//	if(direction_connect == "2" && enodeid_connect == enodeid)
										//	{
										//		continue;
										//	}
										//	if(direction_connect == "3" && snodeid_connect == enodeid)
										//	{
										//		continue;
										//	}
										//}
										//if(direction == "3")
										//{
										//	if(direction_connect == "2" && snodeid_connect == enodeid)
										//	{
										//		continue;
										//	}
										//	if(direction_connect == "3" && enodeid_connect == enodeid)
										//	{
										//		continue;
										//	}
										//}

										LinkItem linkItem;
										linkItem.index = iter2->second;
										if( snodeid_connect == nodeid || nodeid2adjoinnodeid[snodeid_connect] == nodeid || nodeid2adjoinnodeid[nodeid] == snodeid_connect)
										{
											// 起点相同
											linkItem.nodeid = enodeid_connect;
										}
										else
										{
											// 终点相同
											linkItem.nodeid = snodeid_connect;
										}
																					
										if (linkvisited.find(linkItem.index) == linkvisited.end())
										{
											linkindexQueue.push(linkItem);
											linkvisited.insert(linkItem.index);
										}

										// 当前所求link直连的非连接道路的link的kindclass
										if (index == pos || directindexrlink_map.find(pos) != directindexrlink_map.end())
										{
											directindex_map[iter2->second] = iter2->second;
											// 当前是否为交叉点内link
											string curkind = road_Mif.mid[iter2->second][col_kind];
											wgt::trim(curkind, '"');
											if (isRLink(curkind))
											{
												directindexrlink_map[iter2->second] = iter2->second;
											}
										}
									}
									else
									{
										// 如果扩展后还是没找到呢
										sys_log_println(_ASSERT, "ProcessRampKindClass can not find lid: %s \n", lid.c_str());
									}
								}
							}
							else
							{
								sys_log_println(_ASSERT, "ProcessRampKindClass can not find enodeid: %s \n", enodeid.c_str());
							}
						}
					}
				}

				if(i==0)
				{
					forwardkindclassmap = kindclassmap;
				}
				else
				{
					backwardkindclassmap = kindclassmap;
				}
			}

			map<int, string> kindclassmapall;
			kindclassmapall.insert(forwardkindclassmap.begin(),forwardkindclassmap.end());
			kindclassmapall.insert(backwardkindclassmap.begin(),backwardkindclassmap.end());


			int kindclassvalue = -1;
			if (kindclassmapall.size() == 1)
			{
				kindclassvalue = kindclassmapall.begin()->first;
			}
			else if (kindclassmapall.size() > 1)
			{
				kindclassvalue = (++kindclassmapall.begin())->first;
			}

			if (!forwardkindclassmap.empty())
			{
				if (forwardkindclassmap.begin()->first > kindclassvalue)
				{
					kindclassvalue = forwardkindclassmap.begin()->first;
				}
			}
			if (!backwardkindclassmap.empty())
			{
				if (backwardkindclassmap.begin()->first > kindclassvalue)
				{
					kindclassvalue = backwardkindclassmap.begin()->first;
				}
			}

			if(kindclassvalue != -1)
			{
				rampkindclass = kindclassmapall[kindclassvalue];
			}

			// JCT kindclass 最低不低于快速（01）
			// 20151030 mody JCT只连接高速或者快速路，则对应为"00"或"01",如都连接，则为"01"
			if (isJCT(str_kinds))
			{
				if (rampkindclass != "00" && rampkindclass != "01")
				{
					rampkindclass = "01";
				}
			}
			// 主辅路出入口,选择辅路的最高级link处理
			if (isZhuFuluConnected(str_kinds) && kindclassmap_reliefroad.size() > 0)
			{
				rampkindclass = kindclassmap_reliefroad.begin()->second;
			}

			road_Mif.mid[index][col_kindclass] = rampkindclass;
		}
	}
}

void ConfigCatalog::ProcessSAKindClass(wgt::MIF& road_Mif, string in_path)
{
	// col index
	int col_linkmapid = road_Mif.get_col_pos("mapid");
	int col_linkid = road_Mif.get_col_pos("id");
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	int col_kind = road_Mif.get_col_pos("kind");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");

	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string rampkindclass = road_Mif.mid[index][col_kindclass];
		string str_kinds = road_Mif.mid[index][col_kind];
		string linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_kinds, '"');
		wgt::trim(rampkindclass, '"');
		wgt::trim(linkid, '"');
		if (isSA(str_kinds))
		{
			// 直接降级
			rampkindclass = "06";
			road_Mif.mid[index][col_kindclass] = rampkindclass;
		}
	}
}

void ConfigCatalog::ProcessRLinkKindClass(wgt::MIF& road_Mif, string in_path)
{
	wgt::MIF C_N_Mif;
	wgt::MIF C_R_Mif = road_Mif;	// 因为后面会根据mapid扩展MIF,此处直接拷贝
	string city_path = in_path.substr(0, in_path.find_last_of('/'));
	string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
	string path = city_path.substr(0, city_path.find_last_of('/'));		// 用于后面城市扩展  ../data/01_basic/05_Recatalog
	string node_path = city_path + "/C_N";

	if (wgt::mif_to_wsbl(node_path, C_N_Mif) < 0)
	{
		sys_log_println(_ERROR, "ProcessRLinkKindClass read C_N mif error! %s\n", node_path.c_str());
		return;
	}
	// col index
	int col_linkmapid = road_Mif.get_col_pos("mapid");
	int col_linkid = road_Mif.get_col_pos("id");
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	int col_kind = road_Mif.get_col_pos("kind");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");
	int col_pathname = road_Mif.get_col_pos("pathname");
	int col_direction= road_Mif.get_col_pos("direction");
	int col_funcclass = road_Mif.get_col_pos("funcclass");

	int col_nodemapid = C_N_Mif.get_col_pos("mapid");
	int col_nodeid = C_N_Mif.get_col_pos("id");
	int col_adjoin_nid = C_N_Mif.get_col_pos("adjoin_nid");
	int col_node_lid = C_N_Mif.get_col_pos("node_lid");

	map<string, int> linkid2indexmap;			// linkid ==> pos
	map<string, vector<string> > nodeid2lids;	// nodeid ==> linkids
	map<string, string> nodeid2adjoinnodeid;   // nodeid ==> adjoinnodeid

	int nSize2 = C_N_Mif.mid.size();
	for (int index = 0; index < nSize2; index++)
	{
		string str_nodeid = C_N_Mif.mid[index][col_nodeid];
		string str_adjoinid = C_N_Mif.mid[index][col_adjoin_nid];
		string str_linkids = C_N_Mif.mid[index][col_node_lid];
		wgt::trim(str_nodeid, '"');
		wgt::trim(str_adjoinid, '"');
		wgt::trim(str_linkids, '"');

		vector<string> res;
		sys_splitString(str_linkids, res, '|');

		map<string, vector<string> >::iterator ir1 = nodeid2lids.find(str_nodeid);
		map<string, vector<string> >::iterator ir2 = nodeid2lids.find(str_adjoinid);
		if (ir1 != nodeid2lids.end())
		{
			for (int i = 0; i < res.size(); i++)
			{
				ir1->second.push_back(res[i]);
			}
		}
		else
		{
			nodeid2lids[str_nodeid] = res;
		}
		if (str_adjoinid != "" && str_adjoinid != "0")
		{
			if (ir2 != nodeid2lids.end())
			{
				for (int i = 0; i < res.size(); i++)
				{
					ir2->second.push_back(res[i]);
				}
			}
			else
			{
				nodeid2lids[str_adjoinid] = res;
			}
			nodeid2adjoinnodeid[str_nodeid] = str_adjoinid;
		}
	}
	C_N_Mif.mid.clear();
	C_N_Mif.data.geo_vec.clear();

	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_linkid, '"');
		if (linkid2indexmap.find(str_linkid) != linkid2indexmap.end())
		{
			sys_log_println(_ASSERT, "linkid redefined %s\n", str_linkid.c_str());
		}
		linkid2indexmap[str_linkid] = index;
	}

	map<string, string> citydirlist;		// 已经加载进来的城市
	citydirlist[cur_city] = "";

	nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string rampkindclass = road_Mif.mid[index][col_kindclass];
		string str_kinds = road_Mif.mid[index][col_kind];
		string rampfuncclass = road_Mif.mid[index][col_funcclass];
		wgt::trim(str_kinds, '"');
		wgt::trim(rampkindclass, '"');
		wgt::trim(rampfuncclass, '"');
		if (isRLink(str_kinds))
		{
			map<int, string> forwardkindclassmap;  //向前连接的kindclass值
			map<int, string> backwardkindclassmap; //向后连接的kindclass值

			map<int, string> forwardkindclassmapSameFuncClass;  //向前连接的kindclass值
			map<int, string> backwardkindclassmapSameFuncClass; //向后连接的kindclass值

			bool flag = true;					// 是否"仅连接高快速和服务区，停车区、匝道"
			map<int, string> kindclassmap;		// 连接的kindclass值



			for(int i=0;i<2;i++)
			{
				map<int, string> kindclassmapEx;		// 连接的kindclass值

				map<int, string> kindclassmapSameFuncClass; //同FuncClass的连接的kindclass值

				set<LinkItem> linkvisited;	// 已经访问的link
				queue<LinkItem> linkindexQueue;			// 存放road的下标

				LinkItem linkItem;
				linkItem.index = index;
				if(i==0)
				{
					// 第一遍从终点找
					linkItem.nodeid = C_R_Mif.mid[index][col_enodeid];
				}
				else
				{
					// 第二遍从起点找
					linkItem.nodeid = C_R_Mif.mid[index][col_snodeid];
				}
				wgt::trim(linkItem.nodeid, '"');

				linkindexQueue.push(linkItem);
				linkvisited.insert(linkItem);

				while (linkindexQueue.size() != 0)
				{
					LinkItem linkItem = linkindexQueue.front();
					linkindexQueue.pop();
					int pos = linkItem.index;

					string nodeid = linkItem.nodeid;
					string linkid = C_R_Mif.mid[pos][col_linkid];
					string kind = C_R_Mif.mid[pos][col_kind];
					string kindclass = C_R_Mif.mid[pos][col_kindclass];
					string funcclass = C_R_Mif.mid[pos][col_funcclass];
					wgt::trim(linkid, '"');
					wgt::trim(kind, '"');
					wgt::trim(kindclass, '"');
					wgt::trim(funcclass, '"');

					if (!isRLink(kind))
					{
						// 仅连接高快速和服务区，停车区、匝道的交叉点内Link
						if (kindclass == "00" || kindclass == "01" || isSA(kind) || isRamp(kind))
						{
							// 有效路段
							kindclassmap[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
						}
						else
						{
							flag = false;
							kindclassmapEx[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
							if (rampfuncclass == funcclass)
							{
								kindclassmapSameFuncClass[(int)strtol(kindclass.c_str(), NULL, 16)] = kindclass;
							}
						}
					}
					else 
					{
						if(kind != str_kinds)	
						{
							continue;
						}

						string mapid = C_R_Mif.mid[pos][col_linkmapid];
						string snodeid = C_R_Mif.mid[pos][col_snodeid];
						string enodeid = C_R_Mif.mid[pos][col_enodeid];
						string direction = C_R_Mif.mid[pos][col_direction];
						wgt::trim(mapid, '"');
						wgt::trim(snodeid, '"');
						wgt::trim(enodeid, '"');
						wgt::trim(direction, '"');

						if (nodeid == snodeid)
						{
							map<string, vector<string> >::iterator iter_find = nodeid2lids.find(snodeid);
							if (iter_find == nodeid2lids.end())
							{
								// 没找到，需要加载其他城市
								map<string, vector<string> >::iterator it = mapid2dirvec.find(mapid);
								if (it != mapid2dirvec.end())
								{
									vector<string>& cityvec = it->second;
									for (int i = 0; i < cityvec.size(); i++)
									{
										string city = cityvec[i];
										if (city != cur_city && citydirlist.find(city) == citydirlist.end())
										{
											string other_path = path + "/" + city;
											citydirlist[city] = "";
											ExpandCityDir(C_R_Mif, linkid2indexmap, nodeid2lids, other_path);
											iter_find = nodeid2lids.find(snodeid);
											if (iter_find != nodeid2lids.end())
											{
												break;
											}
										}
									}
								}
							}
							if (iter_find != nodeid2lids.end())
							{
								vector<string>& res = iter_find->second;
								for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
								{
									string lid = *iter1;
									if (linkid == lid)
									{
										continue;
									}
									map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
									if (iter2 == linkid2indexmap.end())
									{
										// 没找到，需要加载其他城市
										map<string, vector<string> >::iterator it = mapid2dirvec.find(mapid);
										if (it != mapid2dirvec.end())
										{
											vector<string>& cityvec = it->second;
											for (int i = 0; i < cityvec.size(); i++)
											{
												string city = cityvec[i];
												if (city != cur_city && citydirlist.find(city) == citydirlist.end())
												{
													string other_path = path + "/" + city;
													citydirlist[city] = "";
													ExpandCityDir(C_R_Mif, linkid2indexmap, nodeid2lids, other_path);
													iter2 = linkid2indexmap.find(lid);
													if (iter2 != linkid2indexmap.end())
													{
														break;
													}
												}
											}
										}
									}
									if (iter2 != linkid2indexmap.end())
									{
										string snodeid_connect = C_R_Mif.mid[iter2->second][col_snodeid];
										string enodeid_connect = C_R_Mif.mid[iter2->second][col_enodeid];
										string direction_connect = C_R_Mif.mid[iter2->second][col_direction];
										wgt::trim(snodeid_connect, '"');
										wgt::trim(enodeid_connect, '"');
										wgt::trim(direction_connect, '"');

										if(direction == "2")
										{
											if(direction_connect == "2" && snodeid_connect == snodeid)
											{
												continue;
											}
											if(direction_connect == "3" && enodeid_connect == snodeid)
											{
												continue;
											}
										}
										if(direction == "3")
										{
											if(direction_connect == "2" && enodeid_connect == snodeid)
											{
												continue;
											}
											if(direction_connect == "3" && snodeid_connect == snodeid)
											{
												continue;
											}
										}

										LinkItem linkItem;
										linkItem.index = iter2->second;
										if( snodeid_connect == nodeid || nodeid2adjoinnodeid[snodeid_connect] == nodeid || nodeid2adjoinnodeid[nodeid] == snodeid_connect)
										{
											// 起点相同
											linkItem.nodeid = enodeid_connect;
										}
										else
										{
											// 终点相同
											linkItem.nodeid = snodeid_connect;
										}
										
										if (linkvisited.find(linkItem) == linkvisited.end())
										{
											linkindexQueue.push(linkItem);
											linkvisited.insert(linkItem);
										}
									}
									else
									{
										// 如果扩展后还是没找到呢
										sys_log_println(_ASSERT, "ProcessRLinkKindClass can not find lid: %s \n", lid.c_str());
									}
								}
							}
							else
							{
								sys_log_println(_ASSERT, "ProcessSAKindClass can not find snodeid: %s \n", snodeid.c_str());
							}
						}
						////////////////////////////////////////////////////////////////////////////////////////////
						if (nodeid == enodeid)
						{
							map<string, vector<string> >::iterator iter_find = nodeid2lids.find(enodeid);
							if (iter_find == nodeid2lids.end())
							{
								// 没找到，需要加载其他城市
								map<string, vector<string> >::iterator it = mapid2dirvec.find(mapid);
								if (it != mapid2dirvec.end())
								{
									vector<string>& cityvec = it->second;
									for (int i = 0; i < cityvec.size(); i++)
									{
										string city = cityvec[i];
										if (city != cur_city && citydirlist.find(city) == citydirlist.end())
										{
											string other_path = path + "/" + city;
											citydirlist[city] = "";
											ExpandCityDir(C_R_Mif, linkid2indexmap, nodeid2lids, other_path);
											iter_find = nodeid2lids.find(enodeid);
											if (iter_find != nodeid2lids.end())
											{
												break;
											}
										}
									}
								}
							}
							if (iter_find != nodeid2lids.end())
							{
								vector<string>& res = iter_find->second;
								for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
								{
									string lid = *iter1;
									if (linkid == lid)
									{
										continue;
									}
									map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
									if (iter2 == linkid2indexmap.end())
									{
										// 没找到，需要加载其他城市
										map<string, vector<string> >::iterator it = mapid2dirvec.find(mapid);
										if (it != mapid2dirvec.end())
										{
											vector<string>& cityvec = it->second;
											for (int i = 0; i < cityvec.size(); i++)
											{
												string city = cityvec[i];
												if (city != cur_city && citydirlist.find(city) == citydirlist.end())
												{
													string other_path = path + "/" + city;
													citydirlist[city] = "";
													ExpandCityDir(C_R_Mif, linkid2indexmap, nodeid2lids, other_path);
													iter2 = linkid2indexmap.find(lid);
													if (iter2 != linkid2indexmap.end())
													{
														break;
													}
												}
											}
										}
									}
									if (iter2 != linkid2indexmap.end())
									{
										string snodeid_connect = C_R_Mif.mid[iter2->second][col_snodeid];
										string enodeid_connect = C_R_Mif.mid[iter2->second][col_enodeid];
										string direction_connect = C_R_Mif.mid[iter2->second][col_direction];
										wgt::trim(snodeid_connect, '"');
										wgt::trim(enodeid_connect, '"');
										wgt::trim(direction_connect, '"');

										if(direction == "2")
										{
											if(direction_connect == "2" && enodeid_connect == enodeid)
											{
												continue;
											}
											if(direction_connect == "3" && snodeid_connect == enodeid)
											{
												continue;
											}
										}
										if(direction == "3")
										{
											if(direction_connect == "2" && snodeid_connect == enodeid)
											{
												continue;
											}
											if(direction_connect == "3" && enodeid_connect == enodeid)
											{
												continue;
											}
										}

										LinkItem linkItem;
										linkItem.index = iter2->second;
										if( snodeid_connect == nodeid || nodeid2adjoinnodeid[snodeid_connect] == nodeid || nodeid2adjoinnodeid[nodeid] == snodeid_connect)
										{
											// 起点相同
											linkItem.nodeid = enodeid_connect;
										}
										else
										{
											// 终点相同
											linkItem.nodeid = snodeid_connect;
										}
																					
										if (linkvisited.find(linkItem) == linkvisited.end())
										{
											linkindexQueue.push(linkItem);
											linkvisited.insert(linkItem);
										}
									}
									else
									{
										// 如果扩展后还是没找到呢
										sys_log_println(_ASSERT, "ProcessSAKindClass can not find lid: %s \n", lid.c_str());
									}
								}
							}
						}
					}
				}

				if(i==0)
				{
					forwardkindclassmap = kindclassmapEx;
					forwardkindclassmapSameFuncClass = kindclassmapSameFuncClass;
				}
				else
				{
					backwardkindclassmap = kindclassmapEx;
					backwardkindclassmapSameFuncClass = kindclassmapSameFuncClass;
				}
			}

			// 取所有连接路段的kindclass的次高级别
			if (flag)
			{
				if (kindclassmap.size() == 1)
				{
					rampkindclass = kindclassmap.begin()->second;
				}
				else if (kindclassmap.size() >= 2)
				{
					rampkindclass = (++kindclassmap.begin())->second;
				}
			}
			else
			{
				map<int, string> kindclassmapall;
				kindclassmapall.insert(forwardkindclassmap.begin(),forwardkindclassmap.end());
				kindclassmapall.insert(backwardkindclassmap.begin(),backwardkindclassmap.end());

				map<int, string> kindclassmapallSameFuncClass;
				kindclassmapallSameFuncClass.insert(forwardkindclassmapSameFuncClass.begin(),forwardkindclassmapSameFuncClass.end());
				kindclassmapallSameFuncClass.insert(backwardkindclassmapSameFuncClass.begin(),backwardkindclassmapSameFuncClass.end());


				int kindclassvalue = (int)strtol(rampkindclass.c_str(), NULL, 16);
				if (kindclassmapall.find(kindclassvalue) == kindclassmapall.end() && kindclassmapall.size() >= 1)
				{
					kindclassvalue = kindclassmapall.begin()->first;
				}
							
				if (!forwardkindclassmap.empty())
				{
					if (forwardkindclassmap.begin()->first > kindclassvalue)
					{
						kindclassvalue = forwardkindclassmap.begin()->first;
					}
				}
				if (!backwardkindclassmap.empty())
				{
					if (backwardkindclassmap.begin()->first > kindclassvalue)
					{
						kindclassvalue = backwardkindclassmap.begin()->first;
					}
				}

				// 取同funclass所有连接路段的kindclass的级别
				if (!kindclassmapallSameFuncClass.empty())
				{
					kindclassvalue = kindclassmapallSameFuncClass.begin()->first;

					if (!forwardkindclassmapSameFuncClass.empty())
					{
						if (forwardkindclassmapSameFuncClass.begin()->first > kindclassvalue)
						{
							kindclassvalue = forwardkindclassmapSameFuncClass.begin()->first;
						}
					}
					if (!backwardkindclassmapSameFuncClass.empty())
					{
						if (backwardkindclassmapSameFuncClass.begin()->first > kindclassvalue)
						{
							kindclassvalue = backwardkindclassmapSameFuncClass.begin()->first;
						}
					}
				}
				
				if(kindclassvalue != -1)
				{
					rampkindclass = kindclassmapall[kindclassvalue];
				}
			}
			if (rampkindclass != "0c")
			{
				// 内Link不应该取"0c"
				road_Mif.mid[index][col_kindclass] = rampkindclass;
			}
		}
	}
}

void ConfigCatalog::ProcessBuildInFlag(wgt::MIF& road_Mif, string in_path)
{
	wgt::MIF C_N_Mif;
	string city_path = in_path.substr(0, in_path.find_last_of('/'));
	string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
	string path = city_path.substr(0, city_path.find_last_of('/'));		// 用于后面城市扩展  ../data/01_basic/05_Recatalog
	string node_path = city_path + "/C_N";

	if (wgt::mif_to_wsbl(node_path, C_N_Mif) < 0)
	{
		sys_log_println(_ERROR, "ProcessRLinkKindClass read C_N mif error! %s\n", node_path.c_str());
		return;
	}
	// col index
	int col_linkmapid = road_Mif.get_col_pos("mapid");
	int col_linkid = road_Mif.get_col_pos("id");
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	int col_kind = road_Mif.get_col_pos("kind");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");
	int col_pathname = road_Mif.get_col_pos("pathname");
	int col_build_in_flag = road_Mif.get_col_pos("build_in_flag");
	int col_length = road_Mif.get_col_pos("length");
	

	int col_nodemapid = C_N_Mif.get_col_pos("mapid");
	int col_nodeid = C_N_Mif.get_col_pos("id");
	int col_adjoin_nid = C_N_Mif.get_col_pos("adjoin_nid");
	int col_node_lid = C_N_Mif.get_col_pos("node_lid");

	map<string, int> linkid2indexmap;			// linkid ==> pos
	map<string, vector<string> > nodeid2lids;	// nodeid ==> linkids

	int nSize2 = C_N_Mif.mid.size();
	for (int index = 0; index < nSize2; index++)
	{
		string str_nodeid = C_N_Mif.mid[index][col_nodeid];
		string str_adjoinid = C_N_Mif.mid[index][col_adjoin_nid];
		string str_linkids = C_N_Mif.mid[index][col_node_lid];
		wgt::trim(str_nodeid, '"');
		wgt::trim(str_adjoinid, '"');
		wgt::trim(str_linkids, '"');

		vector<string> res;
		sys_splitString(str_linkids, res, '|');

		map<string, vector<string> >::iterator ir1 = nodeid2lids.find(str_nodeid);
		map<string, vector<string> >::iterator ir2 = nodeid2lids.find(str_adjoinid);
		if (ir1 != nodeid2lids.end())
		{
			for (int i = 0; i < res.size(); i++)
			{
				ir1->second.push_back(res[i]);
			}
		}
		else
		{
			nodeid2lids[str_nodeid] = res;
		}
		if (str_adjoinid != "" && str_adjoinid != "0")
		{
			if (ir2 != nodeid2lids.end())
			{
				for (int i = 0; i < res.size(); i++)
				{
					ir2->second.push_back(res[i]);
				}
			}
			else
			{
				nodeid2lids[str_adjoinid] = res;
			}
		}
	}
	C_N_Mif.mid.clear();
	C_N_Mif.data.geo_vec.clear();

	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_linkid, '"');
		if (linkid2indexmap.find(str_linkid) != linkid2indexmap.end())
		{
			sys_log_println(_ASSERT, "linkid redefined %s\n", str_linkid.c_str());
		}
		linkid2indexmap[str_linkid] = index;
	}

	map<string, string> citydirlist;		// 已经加载进来的城市
	citydirlist[cur_city] = "";

	nSize = road_Mif.mid.size();
	int groupIndex = 0;
	
	vector<int> visited(nSize);

	for (int index = 0; index < nSize; index++)
	{
		string rampkindclass = road_Mif.mid[index][col_kindclass];
		string str_kinds = road_Mif.mid[index][col_kind];
		string ramppathname = road_Mif.mid[index][col_pathname];
		string build_in_flag = road_Mif.mid[index][col_build_in_flag];
		string length = road_Mif.mid[index][col_length];
		wgt::trim(str_kinds, '"');
		wgt::trim(rampkindclass, '"');
		wgt::trim(build_in_flag, '"');
		wgt::trim(length, '"');
		int kindclass = strtol(rampkindclass.c_str(),NULL,16);
		int build_in_flag_v = strtol(build_in_flag.c_str(),NULL,16);
		if (visited[index] == 0)
		{
			if(build_in_flag == "02")
			{
				continue;
			}
			if(kindclass <4 )
			{
				continue;
			}

			visited[index] = groupIndex;

			queue<int> linkindexQueue;			// 存放road的下标
			linkindexQueue.push(index);
			while (linkindexQueue.size() != 0)
			{
				int pos = linkindexQueue.front();
				linkindexQueue.pop();
				string linkid = road_Mif.mid[pos][col_linkid];
				string kind = road_Mif.mid[pos][col_kind];
				string str_kindclassparent = road_Mif.mid[pos][col_kindclass];
				string pathname = road_Mif.mid[pos][col_pathname];
				wgt::trim(linkid, '"');
				wgt::trim(kind, '"');
				wgt::trim(str_kindclassparent, '"');
				int kindclassparent = strtol(str_kindclassparent.c_str(),NULL,16);

				

				string mapid = road_Mif.mid[pos][col_linkmapid];
				string snodeid = road_Mif.mid[pos][col_snodeid];
				string enodeid = road_Mif.mid[pos][col_enodeid];
				wgt::trim(mapid, '"');
				wgt::trim(snodeid, '"');
				wgt::trim(enodeid, '"');
				map<string, vector<string> >::iterator iter_find = nodeid2lids.find(snodeid);
				if (iter_find != nodeid2lids.end())
				{
					vector<string>& res = iter_find->second;
					for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
					{
						string lid = *iter1;
						map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
						if (iter2 != linkid2indexmap.end())
						{
							int index = iter2->second;
															
							string rampkindclassconnected = road_Mif.mid[index][col_kindclass];
							//string build_in_flag = road_Mif.mid[index][col_build_in_flag];
							string length = road_Mif.mid[index][col_length];
							wgt::trim(rampkindclassconnected, '"');
							//wgt::trim(build_in_flag, '"');
							wgt::trim(length, '"');
							int kindclassconnected = strtol(rampkindclassconnected.c_str(),NULL,16);

							if (visited[index] == 0)
							{
								if(kindclassconnected == kindclassparent)
								{
									visited[index] = 1;								
									linkindexQueue.push(index);
								}
							}
						}
						
					}
				}	


				iter_find = nodeid2lids.find(enodeid);
				if (iter_find != nodeid2lids.end())
				{
					vector<string>& res = iter_find->second;
					for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
					{
						string lid = *iter1;
						map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
						if (iter2 != linkid2indexmap.end())
						{
							int index = iter2->second;
															
							string rampkindclassconnected = road_Mif.mid[index][col_kindclass];
							//string build_in_flag = road_Mif.mid[index][col_build_in_flag];
							string length = road_Mif.mid[index][col_length];
							wgt::trim(rampkindclassconnected, '"');
							//wgt::trim(build_in_flag, '"');
							wgt::trim(length, '"');
							int kindclassconnected = strtol(rampkindclassconnected.c_str(),NULL,16);

							if (visited[index] == 0)
							{
								if(kindclassconnected == kindclassparent)
								{
									visited[index] = 1;
									linkindexQueue.push(index);
								}
							}

						}
						
					}
				}	
			}
		}
	}

	
	for (int index=0;index<visited.size();index++)
	{
		if (visited[index] == 1)
		{
			road_Mif.mid[index][col_build_in_flag] = "01";
		}
	}
}

void ConfigCatalog::CheckKindClassConnectivity(wgt::MIF& road_Mif, string in_path)
{
	wgt::MIF C_N_Mif;
	string city_path = in_path.substr(0, in_path.find_last_of('/'));
	string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
	string path = city_path.substr(0, city_path.find_last_of('/'));		// 用于后面城市扩展  ../data/01_basic/05_Recatalog
	string node_path = city_path + "/C_N";

	if (wgt::mif_to_wsbl(node_path, C_N_Mif) < 0)
	{
		sys_log_println(_ERROR, "ProcessRLinkKindClass read C_N mif error! %s\n", node_path.c_str());
		return;
	}
	// col index
	int col_linkmapid = road_Mif.get_col_pos("mapid");
	int col_linkid = road_Mif.get_col_pos("id");
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	int col_kind = road_Mif.get_col_pos("kind");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");
	int col_pathname = road_Mif.get_col_pos("pathname");
	int col_build_in_flag = road_Mif.get_col_pos("build_in_flag");
	int col_length = road_Mif.get_col_pos("length");
	int col_const_st = road_Mif.get_col_pos("const_st");
	

	int col_nodemapid = C_N_Mif.get_col_pos("mapid");
	int col_nodeid = C_N_Mif.get_col_pos("id");
	int col_adjoin_nid = C_N_Mif.get_col_pos("adjoin_nid");
	int col_node_lid = C_N_Mif.get_col_pos("node_lid");

	map<string, int> linkid2indexmap;			// linkid ==> pos
	map<string, vector<string> > nodeid2lids;	// nodeid ==> linkids

	int nSize2 = C_N_Mif.mid.size();
	for (int index = 0; index < nSize2; index++)
	{
		string str_nodeid = C_N_Mif.mid[index][col_nodeid];
		string str_adjoinid = C_N_Mif.mid[index][col_adjoin_nid];
		string str_linkids = C_N_Mif.mid[index][col_node_lid];
		wgt::trim(str_nodeid, '"');
		wgt::trim(str_adjoinid, '"');
		wgt::trim(str_linkids, '"');

		vector<string> res;
		sys_splitString(str_linkids, res, '|');

		map<string, vector<string> >::iterator ir1 = nodeid2lids.find(str_nodeid);
		map<string, vector<string> >::iterator ir2 = nodeid2lids.find(str_adjoinid);
		if (ir1 != nodeid2lids.end())
		{
			for (int i = 0; i < res.size(); i++)
			{
				ir1->second.push_back(res[i]);
			}
		}
		else
		{
			nodeid2lids[str_nodeid] = res;
		}
		if (str_adjoinid != "" && str_adjoinid != "0")
		{
			if (ir2 != nodeid2lids.end())
			{
				for (int i = 0; i < res.size(); i++)
				{
					ir2->second.push_back(res[i]);
				}
			}
			else
			{
				nodeid2lids[str_adjoinid] = res;
			}
		}
	}
	C_N_Mif.mid.clear();
	C_N_Mif.data.geo_vec.clear();

	int nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_linkid, '"');
		if (linkid2indexmap.find(str_linkid) != linkid2indexmap.end())
		{
			sys_log_println(_ASSERT, "linkid redefined %s\n", str_linkid.c_str());
		}
		linkid2indexmap[str_linkid] = index;
	}

	nSize = road_Mif.mid.size();

	int groupIndex = 0;
	

	for (int kindclassProcess=2; kindclassProcess<=9; kindclassProcess++)
	{
		vector<int> visited(nSize);

		for (int index = 0; index < nSize; index++)
		{
			string rampkindclass = road_Mif.mid[index][col_kindclass];
			string str_kinds = road_Mif.mid[index][col_kind];
			string ramppathname = road_Mif.mid[index][col_pathname];
			string build_in_flag = road_Mif.mid[index][col_build_in_flag];
			string length = road_Mif.mid[index][col_length];
			string const_st = road_Mif.mid[index][col_const_st];
			wgt::trim(str_kinds, '"');
			wgt::trim(rampkindclass, '"');
			wgt::trim(build_in_flag, '"');
			wgt::trim(length, '"');
			wgt::trim(const_st, '"');

			if (const_st == "4" || const_st == "3")
			{
				// 施工道路不处理
				continue;
			}

			int kindclass = strtol(rampkindclass.c_str(),NULL,16);
			if (visited[index] == 0)
			{
				if(kindclass != kindclassProcess)
				{
					continue;
				}
				groupIndex ++;
				Group group;
				group.kind_class = strtol(rampkindclass.c_str(),NULL,16);
				group.connected_min_kind_class = 0xFF;
				group.connected_ferry = false;
				group.length = atof(length.c_str());
				group.indexs.insert(index);
				visited[index] = groupIndex;

				queue<int> linkindexQueue;			// 存放road的下标
				linkindexQueue.push(index);
				while (linkindexQueue.size() != 0)
				{
					int pos = linkindexQueue.front();
					linkindexQueue.pop();
					string linkid = road_Mif.mid[pos][col_linkid];
					string kind = road_Mif.mid[pos][col_kind];
					string kindclass = road_Mif.mid[pos][col_kindclass];
					string pathname = road_Mif.mid[pos][col_pathname];
					wgt::trim(linkid, '"');
					wgt::trim(kind, '"');
					wgt::trim(kindclass, '"');

					string mapid = road_Mif.mid[pos][col_linkmapid];
					string snodeid = road_Mif.mid[pos][col_snodeid];
					string enodeid = road_Mif.mid[pos][col_enodeid];
					wgt::trim(mapid, '"');
					wgt::trim(snodeid, '"');
					wgt::trim(enodeid, '"');
					map<string, vector<string> >::iterator iter_find = nodeid2lids.find(snodeid);
					if (iter_find != nodeid2lids.end())
					{
						vector<string>& res = iter_find->second;
						for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
						{
							string lid = *iter1;
							map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
							if (iter2 != linkid2indexmap.end())
							{
								int index = iter2->second;
																
								string rampkindclass = road_Mif.mid[index][col_kindclass];
								string build_in_flag = road_Mif.mid[index][col_build_in_flag];
								string length = road_Mif.mid[index][col_length];
								string const_st = road_Mif.mid[index][col_const_st];
								wgt::trim(rampkindclass, '"');
								wgt::trim(build_in_flag, '"');
								wgt::trim(length, '"');
								wgt::trim(const_st, '"');
								int kindclass = strtol(rampkindclass.c_str(),NULL,16);

								if (const_st == "4" || const_st == "3")
								{
									// 施工道路不处理
									continue;
								}

								if (kindclass == group.kind_class)
								{
									if (visited[index] == 0)
									{
										visited[index] = groupIndex;
										group.indexs.insert(index);
										group.length += atof(length.c_str());
										linkindexQueue.push(index);
									}
								}
								else 
								{
									group.connected_min_kind_class = min(group.connected_min_kind_class,kindclass);
									if (kindclass == 0x0A)
									{
										group.connected_ferry = true;
									}
								}
							}
							
						}
					}	


					iter_find = nodeid2lids.find(enodeid);
					if (iter_find != nodeid2lids.end())
					{
						vector<string>& res = iter_find->second;
						for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
						{
							string lid = *iter1;
							map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
							if (iter2 != linkid2indexmap.end())
							{
								int index = iter2->second;
																
								string rampkindclass = road_Mif.mid[index][col_kindclass];
								string build_in_flag = road_Mif.mid[index][col_build_in_flag];
								string length = road_Mif.mid[index][col_length];
								string const_st = road_Mif.mid[index][col_const_st];
								wgt::trim(rampkindclass, '"');
								wgt::trim(build_in_flag, '"');
								wgt::trim(length, '"');
								wgt::trim(const_st, '"');
								int kindclass = strtol(rampkindclass.c_str(),NULL,16);

								if (const_st == "4" || const_st == "3")
								{
									// 施工道路不处理
									continue;
								}
								if (kindclass == group.kind_class)
								{
									if (visited[index] == 0)
									{
										visited[index] = groupIndex;
										group.indexs.insert(index);
										group.length += atof(length.c_str());
										linkindexQueue.push(index);
									}
								}
								else
								{
									group.connected_min_kind_class = min(group.connected_min_kind_class,kindclass);
									if (kindclass == 0x0A)
									{
										group.connected_ferry = true;
									}
								}
							}
							
						}
					}	
				}

				if (group.length < 0.5 && (group.connected_min_kind_class <= 9 || group.connected_min_kind_class == 11) && !group.connected_ferry)
				{
					if (group.connected_min_kind_class > group.kind_class)
					{
						for (set<int>::iterator iter = group.indexs.begin(); iter != group.indexs.end(); iter++)
						{
							int index = *iter;
							char str_kind_class[32];   
#ifdef WIN32
							sprintf_s(str_kind_class,32,"%02X",group.connected_min_kind_class);   
#else
							snprintf(str_kind_class,32,"%02X",group.connected_min_kind_class);   
#endif

							road_Mif.mid[index][col_kindclass] = wgt::tolower(str_kind_class);
						
							string linkid = road_Mif.mid[index][col_linkid];
						}
					}
				}
			}

		}
	}
}

void ConfigCatalog::LoadMapid2dirConf(string confpath, map<string, vector<string> >& mapid2dirvec)
{
	ifstream confFile(confpath.c_str());
	if (confFile.is_open() == false)
	{
		return;
	}
	string line;
	while (confFile.good() && confFile.eof() == false)
	{
		getline(confFile, line);
		line = wgt::trim(line, BLANK);
		if (line.empty() || wgt::startswith(line, NOTE))   //如果为空或者#开头  
		{
			continue;
		}
		vector<string> splitItems;
		bool parseReult = ParseString(splitItems, line, TABLE, false);
		if (parseReult == false || splitItems.size() < 2)
		{
			continue;
		}
		int itemSize = splitItems.size();
		string mapid = splitItems[0];
		vector<string> dirvec;
		for (int i = 1; i < itemSize; i++)
		{
			dirvec.push_back(splitItems[i]);
		}
		mapid2dirvec[mapid] = dirvec;
	}
	confFile.close();
}

void ConfigCatalog::ExpandCityDir(wgt::MIF& C_R_Mif, map<string, int>& linkid2indexmap, map<string, vector<string> >& nodeid2lids, string path)
{
	sys_log_println(_INFORANK, "begin ExpandCityDir %s C_R_Mif.mid.size(%d)\n", path.c_str(), C_R_Mif.mid.size());
	int nSize_pre = C_R_Mif.mid.size();		// 原来的size

	wgt::MIF road_Mif, node_Mif;
	string node_path = path + "/C_N";
	if (wgt::mif_to_wsbl(node_path, node_Mif) < 0)
	{
		sys_log_println(_ERROR, "ExpandCityDir read C_N mif error! %s\n", node_path.c_str());
		return;
	}
	string road_path = path + "/C_R";
	if (wgt::mif_to_wsbl(road_path, road_Mif) < 0)
	{
		sys_log_println(_ERROR, "ExpandCityDir read C_R mif error! %s\n", road_path.c_str());
		return;
	}
	int nSize = road_Mif.mid.size();
	int col_kind = road_Mif.get_col_pos("kind");
	int col_funclass = road_Mif.get_col_pos("funcclass");
	if (col_kind == -1 || col_funclass == -1)
	{
		sys_log_println(_ERROR, "Road_CatalogEx C_R col_kind col_funcclass error!\n");
		return;
	}
	int col_kindclass = road_Mif.get_col_pos("kind_class");
	if (col_kindclass == -1)
	{
		wgt::check_err("add catalog", road_Mif.add_column("kind_class", "Char(2)", road_Mif.header.col_num));
		col_kindclass = road_Mif.get_col_pos("kind_class");
		if (col_kindclass == -1)
		{
			sys_log_println(_ERROR, "get catalog field failed %s\r\n", road_path.c_str());
			return;
		}
		// step2: 根据kind值计算kind_class 
		for (int index = 0; index < nSize; index++)
		{
			string str_kind_class = "";
			string str_kinds = road_Mif.mid[index][col_kind];
			string str_funcclass = road_Mif.mid[index][col_funclass];
			wgt::trim(str_kinds, '"');
			wgt::trim(str_funcclass, '"');
			str_kind_class = GetRoadLevelFromKind(str_kinds);
			// 根据funcclass调整kind_class
			if (str_kind_class == "03" && str_funcclass == "1")
			{
				str_kind_class = "02";
			}
			if (str_kind_class == "03" && (str_funcclass == "4" || str_funcclass == "5"))
			{
				str_kind_class = "04";
			}
			if (str_kind_class == "04" && (str_funcclass == "2" || str_funcclass == "3"))
			{
				str_kind_class = "03";
			}
			if (str_kind_class == "06" && (str_funcclass == "2" || str_funcclass == "3"))
			{
				str_kind_class = "03";
			}
			if (str_kind_class == "06" && str_funcclass == "4")
			{
				str_kind_class = "04";
			}
			road_Mif.mid[index][col_kindclass] = str_kind_class;
		}
	}

	// col index
	int col_linkid = road_Mif.get_col_pos("id");
	int col_snodeid = road_Mif.get_col_pos("snodeid");
	int col_enodeid = road_Mif.get_col_pos("enodeid");

	int col_nodeid = node_Mif.get_col_pos("id");
	int col_adjoin_nid = node_Mif.get_col_pos("adjoin_nid");
	int col_node_lid = node_Mif.get_col_pos("node_lid");

	int nSize2 = node_Mif.mid.size();
	for (int index = 0; index < nSize2; index++)
	{
		string str_nodeid = node_Mif.mid[index][col_nodeid];
		string str_adjoinid = node_Mif.mid[index][col_adjoin_nid];
		string str_linkids = node_Mif.mid[index][col_node_lid];
		wgt::trim(str_nodeid, '"');
		wgt::trim(str_adjoinid, '"');
		wgt::trim(str_linkids, '"');

		vector<string> res;
		sys_splitString(str_linkids, res, '|');

		map<string, vector<string> >::iterator ir1 = nodeid2lids.find(str_nodeid);
		map<string, vector<string> >::iterator ir2 = nodeid2lids.find(str_adjoinid);
		if (ir1 != nodeid2lids.end())
		{
			for (int i = 0; i < res.size(); i++)
			{
				ir1->second.push_back(res[i]);
			}
		}
		else
		{
			nodeid2lids[str_nodeid] = res;
		}
		if (str_adjoinid != "" && str_adjoinid != "0")
		{
			if (ir2 != nodeid2lids.end())
			{
				for (int i = 0; i < res.size(); i++)
				{
					ir2->second.push_back(res[i]);
				}
			}
			else
			{
				nodeid2lids[str_adjoinid] = res;
			}
		}
	}

	nSize = road_Mif.mid.size();
	for (int index = 0; index < nSize; index++)
	{
		string str_linkid = road_Mif.mid[index][col_linkid];
		wgt::trim(str_linkid, '"');
		if (linkid2indexmap.find(str_linkid) != linkid2indexmap.end())
		{
			sys_log_println(_ASSERT, "linkid redefined %s\n", str_linkid.c_str());
		}
		linkid2indexmap[str_linkid] = index + nSize_pre;
		C_R_Mif.data.geo_vec.push_back(road_Mif.data.geo_vec[index]->clone());
		C_R_Mif.mid.push_back(road_Mif.mid[index]);
	}
	sys_log_println(_INFORANK, "finish ExpandCityDir %s C_R_Mif.size(%d)\n", path.c_str(), C_R_Mif.mid.size());
}

/*
int ConfigCatalog::Process_CommunityArea(string in_path, string out_path, string layername, vector<string> poi_pathVec)
{
	map<string, string> childid2masteridMap;	// C_POI与C_ManualPOI
	map<string, string> incrementpoiMap;		// 变更的poi id
	{
		for (vector<string>::iterator itor = poi_pathVec.begin(); itor != poi_pathVec.end(); ++itor)
		{
			string poi_path = *itor;
			wgt::MIF poi_Mif;
			if (wgt::mif_to_wsbl(poi_path, poi_Mif) < 0)
			{
				sys_log_println(_ERROR, "Process_CommunityArea read poi mif error! %s\n", poi_path.c_str());
				return -1;
			}
			int col_id = poi_Mif.get_col_pos("id");
			int col_masterid = poi_Mif.get_col_pos("master_id");
			int col_update_flag = poi_Mif.get_col_pos("update_flag");	// 字段:new,modify,delete,空为全量
			if (col_id == -1 || col_masterid == -1)
			{
				sys_log_println(_ERROR, "Process_CommunityArea C_POI col_id col_masterid col_update_flag error!\n");
				return -1;
			}
			if (bIncrement && col_update_flag == -1)
			{
				sys_log_println(_ERROR, "Process_CommunityArea C_POI col_id col_masterid col_update_flag error!\n");
				return -1;
			}
			int nSize = poi_Mif.mid.size();
			for (int i = 0; i < nSize; ++i)
			{
				string str_id = poi_Mif.mid[i][col_id];
				string str_masterid = poi_Mif.mid[i][col_masterid];
				wgt::trim(str_id, '"');
				wgt::trim(str_masterid, '"');
				if (str_id != "" && str_masterid != "")
				{
					childid2masteridMap[str_id] = str_masterid;
				}
				if (bIncrement)
				{
					string str_update_flag = poi_Mif.mid[i][col_update_flag];
					wgt::trim(str_update_flag, '"');
					if (str_update_flag != INCREMENT_TYPE_DEFAULT)
					{
						incrementpoiMap[str_id] = str_update_flag;
					}
				}
				
			}
		}
	}
	// 读取C_CommunityArea,使用childid2masteridMap来确定父子关系
	wgt::MIF communityarea_Mif;
	if (wgt::mif_to_wsbl(in_path, communityarea_Mif) < 0)
	{
		sys_log_println(_ERROR, "Process_CommunityArea read C_CommunityArea mif error! %s\n", in_path.c_str());
		return -1;
	}
	int col_poiid = communityarea_Mif.get_col_pos("poi_id");
	if (col_poiid == -1)
	{
		sys_log_println(_ERROR, "Process_CommunityArea C_CommunityArea col_poiid error!\n");
		return -1;
	}

	// 索引 poiid ==> index
	map<string, int> poiid2index;
	map<string, vector<string> > poiid2parentid;	// poiid ==> parentid 一般只有唯一的parentid
	map<string, vector<string> > poiid2childid;		// poiid ==> childid 多个childid
	int nSize = communityarea_Mif.mid.size();
	for (int index = 0; index < nSize; ++index)
	{
		string str_poiid = communityarea_Mif.mid[index][col_poiid];
		wgt::trim(str_poiid, '"');
		poiid2index[str_poiid] = index;
	}
	// loop
	for (int index = 0; index < nSize; ++index)
	{
		string str_poiid = communityarea_Mif.mid[index][col_poiid];
		wgt::trim(str_poiid, '"');

		map<string, string>::iterator itor_find = childid2masteridMap.find(str_poiid);
		if (itor_find != childid2masteridMap.end())
		{
			string str_masterid = itor_find->second;
			if (poiid2index.find(str_masterid) == poiid2index.end())
			{
				sys_log_println(_WARN, "Process_CommunityArea can not find masterid:%s in C_CommunityArea\n", str_masterid.c_str());
				continue;
			}
			map<string, vector<string> >::iterator iter;
			// 记录该poiid对应的parent的poiid
			iter = poiid2parentid.find(str_poiid);
			if (iter != poiid2parentid.end())
			{
				vector<string>& idvec = iter->second;
				if (false == isInVector(idvec, str_masterid))
				{
					idvec.push_back(str_masterid);
				}
			}
			else
			{
				vector<string> idvec;
				idvec.push_back(str_masterid);
				poiid2parentid[str_poiid] = idvec;
			}
			// 记录parent的poiid对应的childid
			iter = poiid2childid.find(str_masterid);
			if (iter != poiid2childid.end())
			{
				vector<string>& idvec = iter->second;
				if (false == isInVector(idvec, str_poiid))
				{
					idvec.push_back(str_poiid);
				}
			}
			else
			{
				vector<string> idvec;
				idvec.push_back(str_poiid);
				poiid2childid[str_masterid] = idvec;
			}
		}
	} // end look

	wgt::MIF dst_Mif;
	dst_Mif.header = communityarea_Mif.header;
	wgt::check_err("add parent_poiid", dst_Mif.add_column("parent_poiid", "Char(60)", dst_Mif.header.col_num));
	wgt::check_err("add child_poiid", dst_Mif.add_column("child_poiid", "Char(610)", dst_Mif.header.col_num));
	wgt::check_err("add catalog", dst_Mif.add_column("catalog", "Char(32)", dst_Mif.header.col_num));
	wgt::check_err("add guid", dst_Mif.add_column("guid", "Char(64)", dst_Mif.header.col_num));
	int col_parent_poiid = dst_Mif.get_col_pos("parent_poiid");
	int col_child_poiid = dst_Mif.get_col_pos("child_poiid");
	int col_catalog = dst_Mif.get_col_pos("catalog");
	int col_guid = dst_Mif.get_col_pos("guid");
	int col_update_flag = dst_Mif.get_col_pos("update_flag");
	if (bIncrement && col_update_flag < 0)
	{
		sys_log_println(_ERROR, "Process_CommunityArea get col_update_flag error! %s\n", in_path.c_str());
		return -1;
	}

	int index = 0;	// 记录新记录的序号(guid生成使用)
	for (int kk = 0; kk < nSize; ++kk)
	{
		vector<string> attrVec = communityarea_Mif.mid[kk];
		string str_poiid = attrVec[col_poiid];
		wgt::trim(str_poiid, '"');

		bool haveParent = false;
		bool haveChild = false;
		map<string, vector<string> >::iterator iter_find, iter_find2;
		iter_find = poiid2parentid.find(str_poiid);
		iter_find2 = poiid2childid.find(str_poiid);
		// 若当前自身poiid变更(删除),删除所有主子关系
		if (bIncrement && incrementpoiMap.find(str_poiid) != incrementpoiMap.end())
		{
			vector<string> attrVecNew = attrVec;
			if (incrementpoiMap.find(str_poiid)->second == INCREMENT_TYPE_DELETE)
			{
				attrVecNew[col_update_flag] = INCREMENT_TYPE_MODIFY;
				attrVecNew.push_back("");				// parent_poiid
				attrVecNew.push_back("");		// child_poiid
				attrVecNew.push_back("\"090300\"");		// catalog
				string  guidstr = "";
				GetMD5(layername, index, guidstr);
				attrVecNew.push_back("\"" + guidstr + "\"");	// guid

				dst_Mif.mid.push_back(attrVecNew);
				dst_Mif.data.geo_vec.push_back(communityarea_Mif.data.geo_vec[kk]->clone());
				RecordIncrementInfo(communityarea_Mif.data.geo_vec[kk], layername);
				index++;
				continue;
			}
		}

		// 父子节点
		if (iter_find != poiid2parentid.end() && iter_find2 != poiid2childid.end())
		{
			vector<string> attrVecNew = attrVec;

			vector<string>& ids = iter_find->second;
			if (ids.size() > 1)
			{
				sys_log_println(_WARN, "Process_CommunityArea poiid2parentid size > 1\n");
			}
			string str_parentid = ids[0];
			if (bIncrement && incrementpoiMap.find(str_parentid) != incrementpoiMap.end())
			{
				// 父poi已经删除
				if (incrementpoiMap.find(str_parentid)->second == INCREMENT_TYPE_DELETE)
				{
					str_parentid = "";
				}
				attrVecNew[col_update_flag] = INCREMENT_TYPE_MODIFY;
			}

			vector<string>& ids2 = iter_find2->second;
			string str_childid = "";
			for (int i = 0; i < ids2.size(); i++)
			{
				string nameTemp = ids2[i];
				if (bIncrement && incrementpoiMap.find(nameTemp) != incrementpoiMap.end())
				{
					// 父poi已经删除
					if (incrementpoiMap.find(nameTemp)->second == INCREMENT_TYPE_DELETE)
					{
						nameTemp = "";
					}
					attrVecNew[col_update_flag] = INCREMENT_TYPE_MODIFY;
					if (nameTemp == "")
					{
						continue;
					}
				}
				if (str_childid != "")
				{
					str_childid.append("|");
				}
				str_childid.append(nameTemp);
			}
			attrVecNew.push_back(str_parentid);		// parent_poiid
			attrVecNew.push_back(str_childid);		// child_poiid
			attrVecNew.push_back("\"090303\"");		// catalog
			string guidstr = "";
			GetMD5(layername, index, guidstr);
			attrVecNew.push_back("\"" + guidstr + "\"");	// guid

			dst_Mif.mid.push_back(attrVecNew);
			dst_Mif.data.geo_vec.push_back(communityarea_Mif.data.geo_vec[kk]->clone());
			if (bIncrement && attrVecNew[col_update_flag] == INCREMENT_TYPE_MODIFY)
			{
				RecordIncrementInfo(communityarea_Mif.data.geo_vec[kk], layername);
			}
			index++;
			haveParent = true;
			haveChild = true;
		}
		// 是否为子节点(有没有parent_poiid)
		else if (iter_find != poiid2parentid.end())
		{
			vector<string> attrVecNew = attrVec;

			vector<string>& ids = iter_find->second;
			if (ids.size() > 1)
			{
				sys_log_println(_WARN, "Process_CommunityArea poiid2parentid size > 1\n");
			}
			string str_parentid = ids[0];
			if (bIncrement && incrementpoiMap.find(str_parentid) != incrementpoiMap.end())
			{
				// 父poi已经删除
				if (incrementpoiMap.find(str_parentid)->second == INCREMENT_TYPE_DELETE)
				{
					str_parentid = "";
				}
				attrVecNew[col_update_flag] = INCREMENT_TYPE_MODIFY;
			}
			
			attrVecNew.push_back(str_parentid);		// parent_poiid
			attrVecNew.push_back("");				// child_poiid
			attrVecNew.push_back("\"090302\"");		// catalog
			string  guidstr = "";
			GetMD5(layername, index, guidstr);
			attrVecNew.push_back("\"" + guidstr + "\"");	// guid

			dst_Mif.mid.push_back(attrVecNew);
			dst_Mif.data.geo_vec.push_back(communityarea_Mif.data.geo_vec[kk]->clone());
			if (bIncrement && attrVecNew[col_update_flag] == INCREMENT_TYPE_MODIFY)
			{
				RecordIncrementInfo(communityarea_Mif.data.geo_vec[kk], layername);
			}
			index++;
			haveParent = true;
		}
		// 是否为父节点(有没有child_poiid)
		else if (iter_find2 != poiid2childid.end())
		{
			vector<string> attrVecNew = attrVec;

			vector<string>& ids2 = iter_find2->second;
			string str_childid = "";
			for (int i = 0; i < ids2.size(); i++)
			{
				string nameTemp = ids2[i];
				if (bIncrement && incrementpoiMap.find(nameTemp) != incrementpoiMap.end())
				{
					// 父poi已经删除
					if (incrementpoiMap.find(nameTemp)->second == INCREMENT_TYPE_DELETE)
					{
						nameTemp = "";
					}
					attrVecNew[col_update_flag] = INCREMENT_TYPE_MODIFY;
					if (nameTemp == "")
					{
						continue;
					}
				}
				if (str_childid != "")
				{
					str_childid.append("|");
				}
				str_childid.append(nameTemp);
			}
			attrVecNew.push_back("");				// parent_poiid
			attrVecNew.push_back(str_childid);		// child_poiid
			attrVecNew.push_back("\"090301\"");		// catalog
			string  guidstr = "";
			GetMD5(layername, index, guidstr);
			attrVecNew.push_back("\"" + guidstr + "\"");	// guid

			dst_Mif.mid.push_back(attrVecNew);
			dst_Mif.data.geo_vec.push_back(communityarea_Mif.data.geo_vec[kk]->clone());
			if (bIncrement && attrVecNew[col_update_flag] == INCREMENT_TYPE_MODIFY)
			{
				RecordIncrementInfo(communityarea_Mif.data.geo_vec[kk], layername);
			}
			index++;
			haveChild = true;
		}

		if (!haveParent && !haveChild)
		{
			vector<string> attrVecNew = attrVec;
			attrVecNew.push_back("");				// parent_poiid
			attrVecNew.push_back("");		// child_poiid
			attrVecNew.push_back("\"090300\"");		// catalog
			string  guidstr = "";
			GetMD5(layername, index, guidstr);
			attrVecNew.push_back("\"" + guidstr + "\"");	// guid

			dst_Mif.mid.push_back(attrVecNew);
			dst_Mif.data.geo_vec.push_back(communityarea_Mif.data.geo_vec[kk]->clone());
			index++;
		}
	}
	// write new data
	return wgt::wsbl_to_mif(dst_Mif, out_path) == 0;
}
*/

int ConfigCatalog::RoadLevelUpgrade(string roadFile, string nodeFile, string roadupconfFile)
{
	// 这个配置是可以多重配置
	typedef struct _roadUpConfig
	{
		std::map<string, string> layerCatalog;
		std::map<string, string> upCatalog;
		float limitLen;
		int empty_block_handle;		// 1:全部提升 0:不处理
		string upValue;
	}roadUpConfig;

	roadUpConfig confs[10];
	int curCount = 0;
	int isStart = 0;
	int layerType = -1;	// layerCatalog:0; upCatalog:1; limitLen:2
	{
		std::string strline;
		vector<string> splitItems;
		std::ifstream infile(roadupconfFile.data());
		while (infile.good() && !infile.eof())
		{
			strline.clear();
			getline(infile, strline);
			wgt::trim(strline, '\r');	//换行符
			if (strline.empty())
			{
				continue;
			}
			if (strline.at(0) == '#')
			{
				continue;
			}
			if (strline == "CONFIG")
			{
				isStart = 1;
				continue;
			}
			if (strline == "END")
			{
				isStart = 0;
				layerType = -1;
				curCount++;
				continue;
			}

			if (strline == "layerCatalog" && isStart == 1)
			{
				layerType = 0;
				continue;
			}
			else if (strline == "upCatalog" && isStart == 1)
			{
				layerType = 1;
				continue;
			}
			else if (strline == "limitLen" && isStart == 1)
			{
				layerType = 2;
				continue;
			}
			else if (strline == "EMPTY_BLOCK_HANDLE" && isStart == 1)
			{
				layerType = 3;
				continue;
			}
			else if (strline == "upValue" && isStart == 1)
			{
				layerType = 4;
				continue;
			}
			sys_splitString(strline, splitItems, '#');
			for (vector<string>::iterator it = splitItems.begin(); it != splitItems.end(); ++it)
			{
				switch (layerType)
				{
				case 0:
					confs[curCount].layerCatalog[*it] = "";
					break;
				case 1:
					confs[curCount].upCatalog[*it] = "";
					break;
				case 2:
					confs[curCount].limitLen = atof((*it).c_str());
					break;
				case 3:
					confs[curCount].empty_block_handle = atoi((*it).c_str());
					break;
				case 4:
					confs[curCount].upValue = *it;
				}
			}
		}
	}
	wgt::MIF roadMif, nodeMif;
	if (wgt::mif_to_wsbl(roadFile, roadMif) < 0)
	{
		sys_log_println(_ERROR, "RoadLevelUpgrade read c_r mif error! %s\n", roadFile.c_str());
		return -1;
	}
	if (wgt::mif_to_wsbl(nodeFile, nodeMif) < 0)
	{
		sys_log_println(_ERROR, "RoadLevelUpgrade read c_n mif error! %s\n", nodeFile.c_str());
		return -1;
	}
	// 增加oldcatalog字段,赋值
	int col_catalog = roadMif.get_col_pos("catalog");
	if (col_catalog == -1)
	{
		sys_log_println(_ERROR, "RoadLevelUpgrade c_r col catalog error!\n");
		return -1;
	}
	int col_oldcatalog = roadMif.get_col_pos("oldcatalog");
	if (col_oldcatalog == -1)
	{
		wgt::check_err("add catalog", roadMif.add_column("oldcatalog", "Char(30)", roadMif.header.col_num));
		col_oldcatalog = roadMif.get_col_pos("oldcatalog");
		if (col_oldcatalog == -1)
		{
			sys_log_println(_ERROR, "get oldcatalog field failed\n");
			return -1;
		}
	}
	int nSize = roadMif.mid.size();
	for (int i = 0; i < nSize; i++)
	{
		roadMif.mid[i][col_oldcatalog] = roadMif.mid[i][col_catalog];
	}

	for (int i = 0; i < curCount; ++i)
	{
		roadUpConfig& conf = confs[i];
		sys_log_println(_ASSERT, "layerCatalog.size:%d upCatalog.size:%d limitLen:%f\n", conf.layerCatalog.size(), conf.upCatalog.size(), conf.limitLen);
		RoadUpgrade(roadMif, nodeMif, conf.layerCatalog, conf.upCatalog, conf.upValue, conf.limitLen, conf.empty_block_handle);
	}
	// write new data
	wgt::check_err("Write back to C_R MIF/MID", wgt::wsbl_to_mif(roadMif, roadFile));
}

int ConfigCatalog::Process_TrafficLight(string infile, string outfile)
{
	// 读取C_N表，根据关联红绿灯找出关联的mainnode路口
	// 红绿灯坐标点居中

	wgt::MIF mifNode, mifTrafficLight;
	if (wgt::mif_to_wsbl(infile, mifNode) <= -1)
	{
		sys_log_println(_ERROR, "Process_TrafficLight read %s failed\n", infile.c_str());
		return -1;
	}
	int nSize = mifNode.mid.size();
	int col_id = mifNode.get_col_pos("id");
	int col_subnodeid = mifNode.get_col_pos("subnodeid");
	int col_light = mifNode.get_col_pos("light_flag");
	if (col_id < 0 || col_subnodeid < 0 || col_light < 0)
	{
		sys_log_println(_ERROR, "no id subnodeid light_flag %s\n", infile.c_str());
		return -1;
	}
	map<string, wsl::coor::dpoint_t> id2pointmap;	// nodeid -> point坐标
	map<string, vector<string> > id2subnodevecmap;	// nodeid -> nodeidvec路口组合
	for (int i=0; i<nSize; ++i)
	{
		string id = mifNode.mid[i][col_id];
		string subnodeid = mifNode.mid[i][col_subnodeid];
		string light_flag = mifNode.mid[i][col_light];
		wgt::trim(id, '"');
		wgt::trim(subnodeid, '"');
		wgt::trim(light_flag, '"');
		if (subnodeid != "0" && subnodeid != "")
		{
			vector<string> subnodeidvec;
			sys_splitString(subnodeid, subnodeidvec, '|');
			subnodeidvec.push_back(id);
			int nodesize = subnodeidvec.size();
			for (int j=0; j<nodesize; ++j)
			{
				id2subnodevecmap[subnodeidvec[j]] = subnodeidvec;
			}
		}
		wsl::Geometry* geo_ptr = mifNode.data.geo_vec[i];
		id2pointmap[id] = wsl::coor::dpoint_t(geo_ptr->at(0).at(0).x(), geo_ptr->at(0).at(0).y());
	}

	if (wgt::mif_to_wsbl(outfile, mifTrafficLight) <= -1)
	{
		sys_log_println(_ERROR, "Process_TrafficLight read %s failed\n", outfile.c_str());
		return -1;
	}
	// 纠正坐标
	nSize = mifTrafficLight.mid.size();
	col_id = mifTrafficLight.get_col_pos("id");
	for (int i=0; i<nSize; ++i)
	{
		string id = mifTrafficLight.mid[i][col_id];
		wgt::trim(id, '"');
		map<string, vector<string> >::iterator itor = id2subnodevecmap.find(id);
		if (itor == id2subnodevecmap.end())
		{
			continue;
		}
		vector<string>& subnodevec = itor->second;
		// 坐标居中
		wsl::coor::dpoint_t centerPt;
		int nodesize = subnodevec.size();
		int count = 0;	// 有效点个数
		for (int j=0; j<nodesize; ++j)
		{
			map<string, wsl::coor::dpoint_t>::iterator itor_pos = id2pointmap.find(subnodevec[j]);
			if (itor_pos != id2pointmap.end())
			{
				centerPt.x += itor_pos->second.x;
				centerPt.y += itor_pos->second.y;
				count++;
			}		
		}
		if (count != 0)
		{
			centerPt.x /= count;
			centerPt.y /= count;
			mifTrafficLight.data.geo_vec[i]->at(0).at(0).setx(centerPt.x);
			mifTrafficLight.data.geo_vec[i]->at(0).at(0).sety(centerPt.y);
		}
	}

	wgt::wsbl_to_mif(mifTrafficLight, outfile);
	return 1;
}

int ConfigCatalog::Process_SubwayST(string line_path, string st_path)
{
	// 找出地铁站关联的地铁线，根据地铁线的开通状态，来修正地铁站的分类
	// 在建地铁站+在建地铁线 010A0304FF
	// status:1开通 0停运 2在建 3规划
	wgt::MIF mifSubwayLine, mifSubwayST;
	map<string, string> subwaylinemap;
	subwaylinemap.clear();
	int ret = -1;
	ret = wgt::mif_to_wsbl(line_path, mifSubwayLine);
	if (ret <= -1)
	{
		return ret;
	}
	int col_lineid = mifSubwayLine.get_col_pos("line_id");
	int col_linestatus = mifSubwayLine.get_col_pos("status");
	if (col_lineid < 0 || col_linestatus < 0)
	{
		return -1;
	}
	int nSize = mifSubwayLine.mid.size();
	for (int i = 0; i < nSize; ++i)
	{
		string lineid = mifSubwayLine.mid[i][col_lineid];
		string status = mifSubwayLine.mid[i][col_linestatus];
		wgt::trim(lineid, '"');
		wgt::trim(status, '"');
		if (status != "1")
		{
			subwaylinemap[lineid] = "";
		}
	}
	// subwayST
	ret = wgt::mif_to_wsbl(st_path, mifSubwayST);
	if (ret <= -1)
	{
		return ret;
	}
	col_lineid = mifSubwayST.get_col_pos("line_id");
	col_linestatus = mifSubwayST.get_col_pos("status");
	int col_catalog = mifSubwayST.get_col_pos("catalog");
	if (col_lineid < 0 || col_linestatus < 0 || col_catalog < 0)
	{
		return -1;
	}
	nSize = mifSubwayST.mid.size();
	for (int i = 0; i < nSize; ++i)
	{
		string lineid = mifSubwayST.mid[i][col_lineid];
		string status = mifSubwayST.mid[i][col_linestatus];
		wgt::trim(lineid, '"');
		wgt::trim(status, '"');
		if (status != "1" && subwaylinemap.find(lineid) != subwaylinemap.end())
		{
			string new_catalog = "\"010A0304\"";
			mifSubwayST.mid[i][col_catalog] = new_catalog;
		}
	}
	return wgt::wsbl_to_mif(mifSubwayST, st_path);
}

/*
void ConfigCatalog::LoadIncrementInfo()
{
	// m_incrementlayer_file
	FILE *fp = fopen(m_incrementlayer_file.c_str(), "rb");
	if (fp == NULL)
	{
		m_incrementlayer_vec.clear();
		return;
	}
	char szBuffer[64];
	for ( ; fgets(szBuffer, 64, fp) != 0; )
	{
		if (szBuffer[0] == '#')
		{
			continue;
		}
		char *value = strtok(szBuffer, "\t\r\n");
		m_incrementlayer_vec.push_back(value);
	}
	fclose(fp);
	if (m_incrementlayer_vec.size() == 0)
	{
		sys_log_println(_WARN, "increment layer size == 0, %s\n", m_incrementlayer_file.c_str());
	}	
	// m_incrementmeshinfo_file
	if (wgt::mif_to_wsbl(m_incrementmeshinfo_file, m_incrementmeshinfo_mif) < 0)
	{
		m_incrementlayer_vec.clear();
		return;
	}
}

void ConfigCatalog::RecordIncrementInfo(wsl::Geometry* geo_ptr, string layername)
{
	if (bIncrement == false)
	{
		return;
	}
	if (!isInVector(m_incrementlayer_vec, layername))
	{
		m_incrementlayer_vec.push_back(layername);
	}
	if (geo_ptr == NULL)
	{
		return;
	}
	double minlon, minlat, maxlon, maxlat;
	int part_size = geo_ptr->size();
	for (int j=0; j<part_size; j++)
	{
		int pcount = (*geo_ptr)[j].size();
		for (int k=0; k<pcount; k++)
		{
			double x = geo_ptr->at(j).at(k).x();
			double y = geo_ptr->at(j).at(k).y();
			if (j==0 && k==0)
			{
				minlon = maxlon = x;
				minlat = maxlat = y;
			}
			minlon = minlon < x ? minlon : x;
			maxlon = maxlon > x ? maxlon : x;
			minlat = minlat < y ? minlat : y;
			maxlat = maxlat > y ? maxlat : y;
		}
	}
	wsl::Part_t part;
	wsl::PartType partType = geo_ptr->type();
	switch (partType)
	{
	case wsl::POINT_TYPE:
		{
			part.push_back(wsl::Point_t(minlon-0.00001, minlat-0.00001));
			part.push_back(wsl::Point_t(minlon+0.00001, minlat-0.00001));
			part.push_back(wsl::Point_t(minlon+0.00001, minlat+0.00001));
			part.push_back(wsl::Point_t(minlon-0.00001, minlat+0.00001));
		}
		break;
	case wsl::LINE_TYPE:
	case wsl::POLYGON_TYPE:
		{
			part.push_back(wsl::Point_t(minlon, minlat));
			part.push_back(wsl::Point_t(maxlon, minlat));
			part.push_back(wsl::Point_t(maxlon, maxlat));
			part.push_back(wsl::Point_t(minlon, maxlat));
		}
		break;
	}
	wsl::Geometry_t geo;
	geo.resize(1);
	geo.at(0) = part;
	m_incrementmeshinfo_mif.data.geo_vec.push_back(new wsl::Feature<wsl::Polygon>(geo));
	vector<string> mid_vec;
	mid_vec.push_back("");			// mapid
	mid_vec.push_back(layername);	// layer
	char temp[10];
#ifdef LINUX
	snprintf(temp,10,"%lf",minlon);
#else
	sprintf_s(temp,"%lf",minlon);
#endif
	mid_vec.push_back(temp);	// minlon
#ifdef LINUX
	snprintf(temp,10,"%lf",minlat);
#else
	sprintf_s(temp,"%lf",minlat);
#endif
	mid_vec.push_back(temp);	// minlat
#ifdef LINUX
	snprintf(temp,10,"%lf",maxlon);
#else
	sprintf_s(temp,"%lf",maxlon);
#endif
	mid_vec.push_back(temp);	// maxlon
#ifdef LINUX
	snprintf(temp,10,"%lf",maxlat);
#else
	sprintf_s(temp,"%lf",maxlat);
#endif
	mid_vec.push_back(temp);	// maxlat
	mid_vec.push_back("");
	m_incrementmeshinfo_mif.mid.push_back(mid_vec);
}

void ConfigCatalog::WriteIncrementInfo()
{
	if (bIncrement)
	{
		ofstream out_stream(m_incrementlayer_file.c_str(), ios::out);
		for (int i=0; i<m_incrementlayer_vec.size(); ++i)
		{
			out_stream <<  m_incrementlayer_vec[i] << endl;
		}
		out_stream.flush();
		out_stream.close();
		// meshinfo
		wgt::wsbl_to_mif(m_incrementmeshinfo_mif, m_incrementmeshinfo_file);
	}
}
*/

void ConfigCatalog::SmoothRoadCatalog(wgt::MIF& mifRoadLayer)
{
	int iSizeRoad = mifRoadLayer.mid.size();
	if (iSizeRoad <= 0)
	{
		return;
	}
	int index_snodeid = mifRoadLayer.get_col_pos("snodeid");
	int index_enodeid = mifRoadLayer.get_col_pos("enodeid");

	if (index_enodeid < 0 || index_enodeid < 0)
	{
		sys_log_println(_ERROR, "snodeid and enodeid field index error\n");
		return;
	}
	map<string, vector<int> > mapNodeIdIndex;
	string strSnodeId;
	string strEnodeId;
	for (int i = 0; i < iSizeRoad; i++)
	{
		strSnodeId = mifRoadLayer.mid[i][index_snodeid];
		strSnodeId = wgt::trim(strSnodeId, '"');
		strSnodeId = wgt::trim(strSnodeId, ' ');

		vector<int>& vecInt = mapNodeIdIndex[strSnodeId];
		vecInt.push_back(i);

		strEnodeId = mifRoadLayer.mid[i][index_enodeid];
		strEnodeId = wgt::trim(strEnodeId, '"');
		strEnodeId = wgt::trim(strEnodeId, ' ');

		vector<int>& vecInt2 = mapNodeIdIndex[strEnodeId];
		vecInt2.push_back(i);
	}

	int index_catalog = mifRoadLayer.get_col_pos("catalog");
	int index_kind = mifRoadLayer.get_col_pos("kind");
	string strKind;
	for (int i = 0; i < iSizeRoad; i++)
	{
		strKind = mifRoadLayer.mid[i][index_kind];
		strKind = wgt::trim(strKind, '"');
		if (isRLink(strKind))
		{
			continue;
		}
		SmoothRoadItemCatalog(mifRoadLayer, i, mapNodeIdIndex, index_snodeid, index_enodeid, index_catalog);
	}

	/*string strLog = strOutputPath + "/roadcatalog.log";
	WriteLog(strLog, roadCatalogTest);*/
}

bool ConfigCatalog::SmoothRoadItemCatalog(wgt::MIF& mifRoadLayer, const int& iMifIndex, map<string, vector<int> >& mapIndex, const int& indexSnode, const int& indexEnode, const int& indexCatalog)
{
	if (iMifIndex < 0 || indexSnode < 0
		|| indexEnode < 0 || indexCatalog < 0)
	{
		return false;
	}

	string strCatalog = mifRoadLayer.mid[iMifIndex][indexCatalog];
	strCatalog = wgt::trim(strCatalog, '"');
	strCatalog = wgt::trim(strCatalog, ' ');
	string strOrigiDiffer = strCatalog.substr(2, 2);
	int differOrigi = -1;
	int timeExist = -1;
	if (isdigit(strOrigiDiffer[0]) && isdigit(strOrigiDiffer[1]))
	{
		differOrigi = atoi(strOrigiDiffer.c_str());
	}

	string strCatalog4 = strCatalog.substr(0, 4);
	strCatalog4 = wgt::toupper(strCatalog4);
	if (strCatalog4 == "0C08")
	{
		return false;
	}

	string strSnodeId = mifRoadLayer.mid[iMifIndex][indexSnode];
	strSnodeId = wgt::trim(strSnodeId, '"');
	strSnodeId = wgt::trim(strSnodeId, ' ');

	int index_id = mifRoadLayer.get_col_pos("id");
	string strId = mifRoadLayer.mid[iMifIndex][index_id];
	strId = wgt::trim(strId, '"');
	strId = wgt::trim(strId, ' ');

	int indexOthers = -1;
	string strCatalogOther;
	string strCatalogOther4;
	map<string, vector<RoadCatalogExistItem> > mapSnodeCatalog;
	if (mapIndex.find(strSnodeId) != mapIndex.end())
	{
		vector<int>& vecIndex = mapIndex[strSnodeId];
		int iSizeSnode = vecIndex.size();
		for (int i = 0; i < iSizeSnode; i++)
		{
			indexOthers = vecIndex[i];
			if (indexOthers == iMifIndex)
			{
				continue;
			}
			if (indexOthers < 0)
			{
				continue;
			}
			strCatalogOther = mifRoadLayer.mid[indexOthers][indexCatalog];
			strCatalogOther = wgt::trim(strCatalogOther, '"');
			strCatalogOther = wgt::trim(strCatalogOther, ' ');
			strCatalogOther4 = strCatalogOther.substr(0, 4);

			if (strCatalog4 == strCatalogOther4)
			{
				return false;
			}
			/*map<string, vector<RoadCatalogExistItem> >::iterator iteSnodeCatalog = mapSnodeCatalog.find(strCatalogOther4);
			if (iteSnodeCatalog != mapSnodeCatalog.end())
			{
				mapSnodeCatalog.erase(iteSnodeCatalog);
				continue;
			}
			mapSnodeCatalog[strCatalogOther4] = strCatalogOther;*/

			vector<RoadCatalogExistItem>& vecItems = mapSnodeCatalog[strCatalogOther4];
			RoadCatalogExistItem item;
			item.strCatalog = strCatalogOther;
			item.time = 1;
			vecItems.push_back(item);
		}
	}

	string strEnodeId = mifRoadLayer.mid[iMifIndex][indexEnode];
	strEnodeId = wgt::trim(strEnodeId, '"');
	strEnodeId = wgt::trim(strEnodeId, ' ');

	string strNewCatalog = "";
	int differ = 100;
	bool boFoundNew = false;
	if (mapIndex.find(strEnodeId) != mapIndex.end())
	{
		vector<int>& vecIndex = mapIndex[strEnodeId];
		int iSizeEnode = vecIndex.size();
		for (int i = 0; i < iSizeEnode; i++)
		{
			indexOthers = vecIndex[i];
			if (indexOthers == iMifIndex)
			{
				continue;
			}
			if (indexOthers < 0)
			{
				continue;
			}
			strCatalogOther = mifRoadLayer.mid[indexOthers][indexCatalog];
			strCatalogOther = wgt::trim(strCatalogOther, '"');
			strCatalogOther = wgt::trim(strCatalogOther, ' ');
			strCatalogOther4 = strCatalogOther.substr(0, 4);

			if (strCatalog4 == strCatalogOther4)
			{
				return false;
			}
			if (mapSnodeCatalog.find(strCatalogOther4) == mapSnodeCatalog.end())
			{
				continue;
			}
			
			wsl::Geometry* geo_ptr = mifRoadLayer.data.geo_vec[iMifIndex];
			if (GetLinkLength(geo_ptr) > 50)
			{
				continue;
			}

			int temExist = mapSnodeCatalog[strCatalogOther4].size();
			temExist = temExist % 2;

			if (temExist == 0)
			{
				continue;
			}

			/*if (temExist == 1 && timeExist == 0)
			{
				strNewCatalog = strCatalogOther;
				boFoundNew = true;
				timeExist = temExist;
				continue;
			}*/

			string strDiffer = strCatalogOther.substr(2, 2);
			int differTem = -1;
			if (isdigit(strDiffer[0]) && isdigit(strDiffer[1]))
			{
				differTem = atoi(strDiffer.c_str());
			}

			if (differOrigi != -1 && differTem != -1)
			{
				int tem = abs(differOrigi) - abs(differTem);
				tem = abs(tem);
				if (tem < differ || differ == 100)
				{
					strNewCatalog = strCatalogOther;
					differ = tem;
					boFoundNew = true;
					//timeExist = temExist;
				}
			}
			else
			{
				strNewCatalog = strCatalogOther;
				boFoundNew = true;
				//timeExist = temExist;
			}

		}
	}
	if (boFoundNew)
	{
		mifRoadLayer.mid[iMifIndex][indexCatalog] = strNewCatalog;
		
		/*string strTem = strCatalog + "_" + strNewCatalog;
		roadCatalogTest[strId] = strTem;*/
	}
	return boFoundNew;
}

void ConfigCatalog::WriteLog(string strFileName, map<string, string>& mapLog)
{
	if (strFileName.empty())
	{
		return;
	}
	sys_log_println(_INFORANK, "begin to write log\n");
	sys_log_println(_INFORANK, "size = %d\n", mapLog.size());

	ofstream outFile;// (strFileName.c_str(), ios::out);
	outFile.open(strFileName.c_str(), ios::out);
	map<string, string>::iterator iteMap = mapLog.begin();
	while (iteMap != mapLog.end())
	{
		outFile << iteMap->first.c_str() << ":" << iteMap->second.c_str() << endl;
		iteMap++;
	}
	outFile.close();
	sys_log_println(_INFORANK, "end to write log\n");

}

double ConfigCatalog::GetPointDistance(const Point2& pt1, const Point2& pt2)
{
	return sqrt((pt2.x - pt1.x)*(pt2.x - pt1.x) + (pt2.y - pt1.y)*(pt2.y - pt1.y));
}

double ConfigCatalog::GetLinkLength(wsl::Geometry* geoLine)
{
	//wsl::Geometry* geo_ptr = rtic_mif.data.geo_vec[idx];
	wsl::Line& line_feat = ((wsl::Feature<wsl::Line>*)geoLine)->at(0);
	int iSizeLine = line_feat.size();
	vector<Point2> vecPts;
	for (int i = 0; i < iSizeLine; i++)
	{
		const wsl::Point& pnt = line_feat.at(i);
		wsl::coor::dpoint_t ll_pt(pnt.x(), pnt.y());
		wsl::coor::dpoint_t meter_pnt = wsl::coor::ll2mc(ll_pt);

		Point2 pt;
		pt.x = meter_pnt.x;
		pt.y = meter_pnt.y;
		vecPts.push_back(pt);
	}

	double lineLength = 0;
	for (int i = 0; i + 1 < iSizeLine; i++)
	{
		lineLength += GetPointDistance(vecPts[i], vecPts[i + 1]);
	}
	return lineLength;
}

wstring String2Wstring(const string & str)
{
	string curLocale = setlocale(LC_ALL, NULL);
	setlocale(LC_ALL, "chs");

	const char* _Source = str.c_str();
	size_t _Dsize = str.size() + 1;
	wchar_t *_Dest = new wchar_t[_Dsize];
	wmemset(_Dest, 0, _Dsize);
	mbstowcs(_Dest, _Source, _Dsize);
	wstring result = _Dest;
	delete[]_Dest;

	setlocale(LC_ALL, curLocale.c_str());

	return result;
}

string Wstring2String(const wstring & wstr)
{
	string curLocale = setlocale(LC_ALL, NULL);

	setlocale(LC_ALL, "chs");

	const wchar_t* _Source = wstr.c_str();
	size_t _Dsize = 2 * wstr.size() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);
	string result = _Dest;
	delete[]_Dest;

	setlocale(LC_ALL, curLocale.c_str());

	return result;
}

int ConfigCatalog::ProcessPOI(string strInputPath, const string& strInputAOI, string strOutputPath)
{
    if (strInputPath.empty() || strOutputPath.empty())
    {
        return 0;
    }

	map<string, string> id2aoi_area;
    map<string, string> id2category_code;

	// aoi_area
	{
		wgt::MIF aoi_mif;
		int nSize = wgt::mif_to_wsbl(strInputAOI, aoi_mif);
		if (nSize < 0)
		{
			sys_log_println(_WARN, "load %s faild.\n", strInputAOI.c_str());
		}

		int col_id = aoi_mif.get_col_pos("poi_id");
		int col_area = aoi_mif.get_col_pos("shape_area");
		if (col_id >= 0 && col_area >= 0)
		{
			for (size_t i = 0; i < nSize; ++i)
			{
				string id = aoi_mif.mid[i][col_id]; wgt::trim(id, '"');
				string area = aoi_mif.mid[i][col_area]; wgt::trim(area, '"');

				if (!area.empty())
				{
					id2aoi_area.insert(pair<string, string>(id, area));
				}
			}
		}
		else
		{
			sys_log_println(_WARN, "%s column error.\n", strInputAOI.c_str());
		}
	}

    // category_code
    {
        wgt::MIF poi_mif;
        if (wgt::mif_to_wsbl(strInputPath, poi_mif) < 0)
        {
            sys_log_println(_ERROR, "open %s error!\n", strInputPath.c_str());
            return 0;
        }
        int pos_id = poi_mif.get_col_pos("id");
        int pos_category_code = poi_mif.get_col_pos("category_code");
        if (pos_id < 0 || pos_category_code < 0)
        {
            sys_log_println(_ERROR, "%s get col error!\n", strInputPath.c_str());
            return 0;
        }
        for (size_t i = 0; i < poi_mif.mid.size(); ++i)
        {
            string id = poi_mif.mid[i][pos_id];
            string category_code = poi_mif.mid[i][pos_category_code];
            wgt::trim(id, '"');
            wgt::trim(category_code, '"');
            if (!id.empty() && !category_code.empty())
            {
                id2category_code.insert(make_pair(id, category_code));
            }
        }
    }

    // 2018.01.24   目前不再有过滤楼号的操作, 采取本地更新策略, 节省时间
    wgt::MIF poi_mif;
    if (wgt::mif_to_wsbl(strInputPath, poi_mif) < 0)
    {
        sys_log_println(_ERROR, "open %s error!\n", strInputPath.c_str());
        return 0;
    }
	int pos_id = poi_mif.get_col_pos("id");
    int pos_master_id = poi_mif.get_col_pos("master_id");
    
	int pos_aoiarea = poi_mif.get_col_pos("aoi_area");
	if (pos_aoiarea < 0)
	{
		poi_mif.add_column("aoi_area", "Char(16)");
		pos_aoiarea = poi_mif.get_col_pos("aoi_area");
	}

    int pos_master_category_code = poi_mif.get_col_pos("master_category_code");
    if (pos_master_category_code < 0)
    {
        poi_mif.add_column("master_category_code", "char(16)");
        pos_master_category_code = poi_mif.get_col_pos("master_category_code");
    }

    if (pos_id < 0 || pos_master_id < 0)
    {
        sys_log_println(_ERROR, "poi get col err\n");
        return 0;
    }
    
    for (size_t i = 0; i < poi_mif.mid.size(); ++i)
    {
		string id = poi_mif.mid[i][pos_id];
		wgt::trim(id, '"');
		string master_id = poi_mif.mid[i][pos_master_id];
		wgt::trim(master_id, '"');
		// aoi_area关联
		if (id2aoi_area.find(id) != id2aoi_area.end())
		{
			poi_mif.mid[i][pos_aoiarea] = id2aoi_area[id];
		}

        // master_category_code
        if (id2category_code.find(master_id) != id2category_code.end())
        {
            poi_mif.mid[i][pos_master_category_code] = "\"" + id2category_code[master_id] + "\"";
        }
    }

	return wgt::wsbl_to_mif(poi_mif, strOutputPath) == 0;
}
