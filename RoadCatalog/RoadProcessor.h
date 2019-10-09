#ifndef ROADPROCESSOR_H
#define ROADPROCESSOR_H

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

struct Group {
    int kind_class;
    int connected_min_kind_class;
    bool connected_ferry;
    std::set<int> indexs;
    double length;
};

struct RoadCatalogExistItem {
    std::string strCatalog;
    int time;
};

struct LinkItem {
	int index;
	std::string nodeid;

	LinkItem& operator=(const LinkItem& crf) {
		if (&crf == this) { 
			return *this;
		}
		index = crf.index;
		nodeid = crf.nodeid;

		return *this;
	}

	bool operator < (const LinkItem& crf) const {  
		if (index != crf.index)
			return index < crf.index;
		return nodeid < crf.nodeid;      
	}
};

class RoadProcessor {
public:
    // 构造函数
    RoadProcessor(const std::string& inputdir, const std::string& outputdir,
            const std::string& tencentdir, const std::string& confdir,
            const std::string& cityname);
    // 析构函数
    ~RoadProcessor() = default;
    
    // 主执行线程
    bool execute();

private:
    // 道路分类的主进程
    int Road_CatalogEx(const std::string& in_path,
            const std::string& out_path, const std::string& layername,
            const std::string& backPolygonfile);
    
    // 处理外挂主路的catalog设置
    int RecatalogMainRoad(const std::string & input_mif,
            const std::string& output_mif, const std::string& plug_mif,
            const std::string& new_catalog);

    // 根据最终的kindclass和其他信息确定道路分类
    int Road_Catalog(const std::string& in_path, const std::string& out_path,
            const std::string& layername);
    
    // 处理道路的反向操作，反转逆序道路
    void ProcessRoadDirection(wgt::MIF& InMif);

    // 处理匝道的KindClass
    void ProcessRampKindClass(wgt::MIF& road_Mif, const std::string& in_path);
    
    // 处理SA服务区的kindclass
    void ProcessSAKindClass(wgt::MIF& road_Mif, const std::string& in_path);
    
    // 处理交叉点内Link的kindclass
    void ProcessRLinkKindClass(wgt::MIF& road_Mif, const std::string& in_path);
    
    // 设置建成区内道路的标志位
    void ProcessBuildInFlag(wgt::MIF& road_Mif, const std::string& in_path);
    
    // kindclass平滑调整
    void CheckKindClassConnectivity(wgt::MIF& road_Mif, std::string in_path);
    
    // 扩展到其他城市的连接道路
    void ExpandCityDir(wgt::MIF& C_R_Mif,
            std::map<std::string, int>& linkid2indexmap,
            std::map<std::string, std::vector<std::string>>& nodeid2lids,
            const std::string& path);
    
    // 道路等级的提升
    int RoadLevelUpgrade(const std::string& roadFile,
            const std::string& nodeFile, const std::string& roadupconfFile);

    // 对道路的catalog进行再次平滑
    void SmoothRoadCatalog(wgt::MIF& mifRoadLayer);

    // 对单个道路执行平滑操作
    bool SmoothRoadItemCatalog(wgt::MIF& mifRoadLayer, const int mifIndex,
            std::map<std::string, std::vector<int>>& mapIndex,
            const int indexSnode, const int indexEnode,
            const int indexCatalog);

private:
    // 输入数据的路径
    std::string strInputPath;
    // 输出数据的路径
    std::string strOutputPath;
    // 自研数据的路径
    std::string strTencentPath;
    // 配置文件的路径
    std::string strConfPath;
    // 当前处理的城市名称
    std::string strCityName;

    // 图幅id和目录的关系索引
    std::map<std::string, std::vector<std::string> > mapid2dirvec;
    // kindclass调整的映射关系
    std::map<std::string, std::string> kindClassAdjustMap;
};

#endif // ROADPROCESSOR_H
