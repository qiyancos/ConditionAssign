#include "ProcessUtil.h"

struct RoadColIndex {
	int col_mapid;
	int col_linkid;
	int col_snodeid;
	int col_enodeid;
	int col_catalog;
	int col_kind_class;
	int col_length;
	int col_funcclass;
	int col_name;
};

struct NodeColIndex {
	int col_nodeid;
	int col_adjoin_nid;
	int col_nodelid;
};

struct RoadBlockInfo {
	string mapid;
    // mapid下所有的link索引
	std::vector<int> alllinkIndex;
    // 在layer中已经配置的link索引
	std::vector<int> layerlinkIndex;
};

// 筛选合适的一个link: rank angle length
struct score {
	int pos;
	int point;	// 总得分
	int kind_class;
	float angle;
	float length;
};

struct compareRoadBlocklinkSize {
	bool operator() (const RoadBlockInfo& feature1,
            const RoadBlockInfo& feature2) {
		return (feature1.layerlinkIndex.size() <
                feature2.layerlinkIndex.size());
	}
};

struct compareScoreByKindClass {
	bool operator() (const score& feature1, const score& feature2) {
		return (feature1.kind_class < feature2.kind_class);
	}
};

struct compareScoreByAngle {
	bool operator() (const score& feature1, const score& feature2) {
		return (feature1.angle < feature2.angle);
	}
};

struct compareScoreByLength {
	bool operator() (const score& feature1, const score& feature2) {
		return (feature1.length < feature2.length);
	}
};

struct compareScoreByPoint {
	bool operator() (const score& feature1, const score& feature2) {
		return (feature1.point > feature2.point);
	}
};

// 判断给定的数值是否在对应的向量中
bool isInIntVector(std::vector<int>& all, int temp);

// 判断一个道路块是否在所有给定的道路块中
bool isInAllBlock(std::vector<RoadBlockInfo>& allBlock, std::string mapid,
        RoadBlockInfo& block);

// 初始化地图数据
void initmapdata();

// 处理道路link
void processLink(int idx, std::vector<int>& upgradedLink, wgt::MIF& result,
        int flag);

// 对link进行扩展
int link_expand(int idx, std::string nid, bool is_start,
        std::vector<int>& upgradedLink, std::vector<int>& cur_upgradedLink,
        std::queue<int>& baklinks, int& flag);

// 寻找和当前link邻接的link
int find_adjoin_link(int idx, string nid, bool is_start,
        std::vector<int>& upgradedLink, std::vector<int>& cur_upgradedLink,
        std::vector<score>& res_vec, int& flag);

// 计算道路之间的角度
double calculate_angle(Feature<Line>* link1, Feature<Line>* link2,
        bool is_start1, bool is_start2);

// 道路等级提升
void RoadUpgrade(wgt::MIF& plink, wgt::MIF pNlink,
        const std::map<std::string, std::string>& layerCatalog, 
        const std::map<std::string, std::string>& upCatalog, 
        const std::string& upValue, float limitLen, int empty_block_handle);

