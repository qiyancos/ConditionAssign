#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <set>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <queue>

#include "ConfigCatalog.h"

#include "type_factory.h"
#include "spatial-base.h"
#include "tx_common.h"

using namespace wgt;
using namespace wsl;
using namespace std;

typedef struct _RoadColIndex
{
	int col_mapid;
	int col_linkid;
	int col_snodeid;
	int col_enodeid;
	int col_catalog;
	int col_kind_class;
	int col_length;
	int col_funcclass;
	int col_name;
}RoadColIndex;

typedef struct _NodeColIndex
{
	int col_nodeid;
	int col_adjoin_nid;
	int col_nodelid;
}NodeColIndex;

typedef struct _RoadBlockInfo
{
	string mapid;
	vector<int> alllinkIndex;		// mapid下所有的link索引
	vector<int> layerlinkIndex;		// 在layer中已经配置的link索引
}RoadBlockInfo;

// 筛选合适的一个link: rank angle length
typedef struct _score
{
	int pos;
	int point;	// 总得分
	int kind_class;
	float angle;
	float length;
}score;

struct compareRoadBlocklinkSize
{
	bool operator()(const RoadBlockInfo& feature1, const RoadBlockInfo& feature2)
	{
		return (feature1.layerlinkIndex.size() < feature2.layerlinkIndex.size());
	}
};

struct compareScoreByKindClass
{
	bool operator()(const score& feature1, const score& feature2)
	{
		return (feature1.kind_class < feature2.kind_class);
	}
};
struct compareScoreByAngle
{
	bool operator()(const score& feature1, const score& feature2)
	{
		return (feature1.angle < feature2.angle);
	}
};
struct compareScoreByLength
{
	bool operator()(const score& feature1, const score& feature2)
	{
		return (feature1.length < feature2.length);
	}
};
struct compareScoreByPoint
{
	bool operator()(const score& feature1, const score& feature2)
	{
		return (feature1.point > feature2.point);
	}
};

bool isInIntVector(std::vector<int>& all, int temp);
bool isInAllBlock(vector<RoadBlockInfo>& allBlock, string mapid, RoadBlockInfo& block);
void initmapdata();
void processLink(int idx, vector<int>& upgradedLink, wgt::MIF& result, int flag);
int link_expand(int idx, string nid, bool is_start, vector<int>& upgradedLink, vector<int>& cur_upgradedLink, queue<int>& baklinks, int& flag);
int find_adjoin_link(int idx, string nid, bool is_start, vector<int>& upgradedLink, vector<int>& cur_upgradedLink, vector<score>& res_vec, int& flag);
double calculate_angle(Feature<Line>* link1, Feature<Line>* link2, bool is_start1, bool is_start2);

// global variable
wgt::MIF roadmif;
wgt::MIF nodemif;
RoadColIndex roadcol;					// C_R column index
NodeColIndex nodecol;					// C_N column index
map<string, int> linkid2index;			// C_R中id与index索引
map<string, set<int> > snid2indexset;	// C_R中snodeid与set<index>
map<string, set<int> > enid2indexset;	// C_R中enodeid与set<index>
map<string, string> nid2adjoinid;		// C_N中邻接点映射
vector<int> all_layerlinkIndex;			// 已经在layer中配置的link，所有block的
vector<int> all_uplinkIndex;		// 需要提升的link索引(从vector中筛选)
float LimitLength = 0.0;
string upMiddleValue;				// 提升的catalog中间值"05"

void initmapdata()
{
	linkid2index.clear();
	snid2indexset.clear();
	enid2indexset.clear();
	nid2adjoinid.clear();
	all_layerlinkIndex.clear();
	all_uplinkIndex.clear();
	LimitLength = 0.0;
	upMiddleValue = "";
}

void RoadUpgrade(wgt::MIF& plink, wgt::MIF pNlink, std::map<std::string, std::string> layerCatalog, std::map<std::string, std::string> upCatalog, std::string upValue, float limitLen, int empty_block_handle)
{
	// all block info  split by mapid
	vector<RoadBlockInfo> roadblockVec;
	vector<int> upgradedLink;	// 已经提升的link索引
	// copy mif
	roadmif = plink;
	nodemif = pNlink;
	int nRsize = roadmif.mid.size();
	int nNsize = nodemif.mid.size();
	//col pos
	roadcol.col_mapid = roadmif.get_col_pos("mapid");
	roadcol.col_linkid = roadmif.get_col_pos("id");
	roadcol.col_snodeid = roadmif.get_col_pos("snodeid");
	roadcol.col_enodeid = roadmif.get_col_pos("enodeid");
	roadcol.col_catalog = roadmif.get_col_pos("catalog");
	roadcol.col_kind_class = roadmif.get_col_pos("kind_class");
	roadcol.col_length = roadmif.get_col_pos("length");
	roadcol.col_funcclass = roadmif.get_col_pos("funcclass");
	roadcol.col_name = roadmif.get_col_pos("pathname");

	nodecol.col_nodeid = nodemif.get_col_pos("id");
	nodecol.col_adjoin_nid = nodemif.get_col_pos("adjoin_nid");
	nodecol.col_nodelid = nodemif.get_col_pos("node_lid");

	// map init
	initmapdata();
	upMiddleValue = upValue;
	LimitLength = limitLen;

	for (int i=0; i<nNsize; ++i)
	{
		string nodeid = nodemif.mid[i][nodecol.col_nodeid];
		string adjoin_nid = nodemif.mid[i][nodecol.col_adjoin_nid];
		wgt::trim(nodeid, '"');
		wgt::trim(adjoin_nid, '"');
		if (adjoin_nid != "" && adjoin_nid != "0")
		{
			nid2adjoinid[nodeid] = adjoin_nid;
		}		
	}
	for (int i=0; i<nRsize; ++i)
	{
		string str_mapid = roadmif.mid[i][roadcol.col_mapid];
		string str_catalog = roadmif.mid[i][roadcol.col_catalog];
		wgt::trim(str_mapid, '"');
		wgt::trim(str_catalog, '"');
		RoadBlockInfo block;

		bool flag = false;
		for (vector<RoadBlockInfo>::iterator itor=roadblockVec.begin(); itor!=roadblockVec.end(); ++itor)
		{
			RoadBlockInfo& info = *itor;
			if (info.mapid == str_mapid)
			{
				info.alllinkIndex.push_back(i);
				if (layerCatalog.find(str_catalog)!=layerCatalog.end())
				{
					info.layerlinkIndex.push_back(i);
					all_layerlinkIndex.push_back(i);
				}
				if (upCatalog.find(str_catalog)!=upCatalog.end())
				{
					all_uplinkIndex.push_back(i);
				}
				flag = true;
				break;
			}
		}
		if (flag == false)
		{
			block.mapid = str_mapid;
			block.alllinkIndex.push_back(i);
			if (layerCatalog.find(str_catalog)!=layerCatalog.end())
			{
				block.layerlinkIndex.push_back(i);
				all_layerlinkIndex.push_back(i);
			}
			if (upCatalog.find(str_catalog)!=upCatalog.end())
			{
				all_uplinkIndex.push_back(i);
			}
			roadblockVec.push_back(block);
		}
		linkid2index[wgt::trim(roadmif.mid[i][roadcol.col_linkid], '"')] = i;
		snid2indexset[wgt::trim(roadmif.mid[i][roadcol.col_snodeid],'"')].insert(i);
		enid2indexset[wgt::trim(roadmif.mid[i][roadcol.col_enodeid],'"')].insert(i);
	}
	// 依次排序
	std::sort( roadblockVec.begin(), roadblockVec.end(), compareRoadBlocklinkSize());

	int nblockCount = roadblockVec.size();
	int limitCount = nblockCount/2;
	// 处理排序后的block内link，将有些link提升
	for (vector<RoadBlockInfo>::iterator itor=roadblockVec.begin(); itor!=roadblockVec.end(),limitCount>0; ++itor,limitCount--)
	{
		RoadBlockInfo& block = *itor;
		// block内没有layerlink，则全部提升
		if (block.layerlinkIndex.size() == 0 && empty_block_handle == 1)
		{
			for (vector<int>::iterator itor_index=block.alllinkIndex.begin(); itor_index!=block.alllinkIndex.end(); ++itor_index)
			{
				if (isInIntVector(block.layerlinkIndex, *itor_index) || isInIntVector(upgradedLink, *itor_index) || !isInIntVector(all_uplinkIndex, *itor_index))
				{
					continue;
				}
				processLink(*itor_index, upgradedLink, plink, 1);
			}
		}
		else
		{	// 针对有些数据不足的block，考虑在all_layerlinkIndex的基础上，查找连接的link
			for (vector<int>::iterator itor_index=block.layerlinkIndex.begin(); itor_index!=block.layerlinkIndex.end(); ++itor_index)
			{
				int pos = *itor_index;
				string snodeid = roadmif.mid[pos][roadcol.col_snodeid];
				string enodeid = roadmif.mid[pos][roadcol.col_enodeid];
				vector<int> cur_upgradedLink;
				vector<score> same_vec;
				int flag = 0;
				int ret = find_adjoin_link(pos, snodeid, 1, upgradedLink, cur_upgradedLink, same_vec, flag);
				if (ret > 0)
				{
					processLink(same_vec[0].pos, upgradedLink, plink, 0);
				}
				ret = find_adjoin_link(pos, enodeid, 0, upgradedLink, cur_upgradedLink, same_vec, flag);
				if (ret > 0)
				{
					processLink(same_vec[0].pos, upgradedLink, plink, 0);
				}
			}
		}
	}
}

bool isInIntVector(std::vector<int>& all, int temp)
{
	for (std::vector<int>::iterator itor=all.begin(); itor!=all.end(); ++itor)
	{
		if (*itor == temp)
		{
			return true;
		}
	}
	return false;
}

bool isInAllBlock(vector<RoadBlockInfo>& allBlock, string mapid, RoadBlockInfo& block)
{
	for (vector<RoadBlockInfo>::iterator itor=allBlock.begin(); itor!=allBlock.end(); ++itor)
	{
		RoadBlockInfo& info = *itor;
		if (info.mapid == mapid)
		{
			block = *itor;
			return true;
		}
	}
	return false;
}

// flag参数用于区分link截止条件
// 1: 所有link	0: 与layer中的link连接
void processLink(int idx, vector<int>& upgradedLink, wgt::MIF& result, int flag)
{
	/** 处理link时，需要以此link的snode与enode遍历
	* 当连接道路为 layerindex中的link
	* 可能处理多个block中的link
	*/
	vector<int> cur_upgradedLink;	// 本次需要提升的link index
	queue<int> baklinks;			// 与一个link关联的link数可能>1,用于存放没有选择的link，备选
	cur_upgradedLink.push_back(idx);
	string linkid = roadmif.mid[idx][roadcol.col_linkid];
	string snodeid = roadmif.mid[idx][roadcol.col_snodeid];
	string enodeid = roadmif.mid[idx][roadcol.col_enodeid];
	wgt::trim(snodeid, '"');
	wgt::trim(enodeid, '"');
	float alllen = 0.0; 
	// 以snodeid查找
	link_expand(idx, snodeid, 1, upgradedLink, cur_upgradedLink, baklinks, flag);
	for (vector<int>::iterator itor=cur_upgradedLink.begin(); itor!=cur_upgradedLink.end(); ++itor)
	{
		alllen += atof(wgt::trim(roadmif.mid[*itor][roadcol.col_length], '"').c_str());
	}
	link_expand(idx, enodeid, 0, upgradedLink, cur_upgradedLink, baklinks, flag);
	for (vector<int>::iterator itor=cur_upgradedLink.begin(); itor!=cur_upgradedLink.end(); ++itor)
	{
		alllen += atof(wgt::trim(roadmif.mid[*itor][roadcol.col_length], '"').c_str());
	}
	if (alllen < LimitLength)
	{
		cur_upgradedLink.clear();
		while (!baklinks.empty())
			baklinks.pop();
	}
	for (vector<int>::iterator itor=cur_upgradedLink.begin(); itor!=cur_upgradedLink.end(); ++itor)
	{
		int tbc_idx = *itor;
		upgradedLink.push_back(tbc_idx);
		// 提升catalog值
		string cur_catalog = result.mid[*itor][roadcol.col_catalog];
		if (upMiddleValue != "")
		{
			string new_catalog = cur_catalog.substr(0,2) + upMiddleValue + cur_catalog.substr(4);
			result.mid[*itor][roadcol.col_catalog] = new_catalog;
		}
	}
	cur_upgradedLink.clear();
	// 处理孤立link的情况
	if (flag == 1)
	{	// 孤立的link

		while (flag==1 && !baklinks.empty())
		{
			int first = baklinks.front();
			baklinks.pop();
			cur_upgradedLink.push_back(first);
			string linkid = roadmif.mid[first][roadcol.col_linkid];
			string snodeid = roadmif.mid[first][roadcol.col_snodeid];
			string enodeid = roadmif.mid[first][roadcol.col_enodeid];
			wgt::trim(snodeid, '"');
			wgt::trim(enodeid, '"');
			link_expand(first, snodeid, 1, upgradedLink, cur_upgradedLink, baklinks, flag);
			link_expand(first, enodeid, 0, upgradedLink, cur_upgradedLink, baklinks, flag);

			for (vector<int>::iterator itor=cur_upgradedLink.begin(); itor!=cur_upgradedLink.end(); ++itor)
			{
				int tbc_idx = *itor;
				upgradedLink.push_back(tbc_idx);
				// 提升catalog值
				string cur_catalog = result.mid[*itor][roadcol.col_catalog];
				if (upMiddleValue != "")
				{
					string new_catalog = cur_catalog.substr(0,2) + upMiddleValue + cur_catalog.substr(4);
					result.mid[*itor][roadcol.col_catalog] = new_catalog;
				}
			}
			cur_upgradedLink.clear();
		}
	}
}

// flag为1,即表明是从空快查找link，这样遍历后，link可能是孤立的，与layer中配置的link未连接
// 因此增加baklinks，记录备选
// 同时，当查找的link已经与layerlink连接，将flag置为0
int link_expand(int idx, string nid, bool is_start, vector<int>& upgradedLink, vector<int>& cur_upgradedLink, queue<int>& baklinks, int& flag)
{
	vector<score> same_vec;
	int ret = find_adjoin_link(idx, nid, is_start, upgradedLink, cur_upgradedLink, same_vec, flag);
	if (ret <= 0)
	{
		return ret;
	}
	int tbc_idx = same_vec[0].pos;
	if (same_vec.size() > 1)
	{
		// 参考评分
		int base_point = 0;
		std::sort( same_vec.begin(), same_vec.end(), compareScoreByKindClass());
		int kindclassTemp = same_vec[0].kind_class;
		for (int m=0; m<ret; m++)
		{
			if (same_vec[m].kind_class != kindclassTemp)
			{
				kindclassTemp = same_vec[m].kind_class;
				base_point += 1;
			}
			same_vec[m].point+=base_point;
		}
		base_point = 0;
		std::sort( same_vec.begin(), same_vec.end(), compareScoreByAngle());
		float angleTemp = same_vec[0].angle;
		for (int m=0; m<ret; m++)
		{
			if (same_vec[m].angle != angleTemp)
			{
				angleTemp = same_vec[m].angle;
				base_point += 0.8;
			}
			same_vec[m].point+=base_point;
		}
		base_point = 0;
		std::sort( same_vec.begin(), same_vec.end(), compareScoreByLength());
		float lengthTemp = same_vec[0].length;
		for (int m=0; m<ret; m++)
		{
			if (same_vec[m].length != lengthTemp)
			{
				lengthTemp = same_vec[0].length;
				base_point += 0.5;
			}
			same_vec[m].point+=base_point;
		}
		// pathname funcclass
		string cur_pathname = roadmif.mid[idx][roadcol.col_name];
		string cur_funcclass = roadmif.mid[idx][roadcol.col_funcclass];
		wgt::trim(cur_pathname, '"');
		wgt::trim(cur_funcclass, '"');
		for (int m=0; m<ret; m++)
		{
			string tbc_pathname = roadmif.mid[same_vec[m].pos][roadcol.col_name];
			string tbc_funcclass = roadmif.mid[same_vec[m].pos][roadcol.col_funcclass];
			wgt::trim(tbc_pathname, '"');
			wgt::trim(tbc_funcclass, '"');
			if (cur_pathname == tbc_pathname)
			{
				same_vec[m].point += 2*ret;
			}
			if (cur_funcclass == tbc_funcclass)
			{
				same_vec[m].point += ret;
			}
			// nid
			if (is_start == 1)
			{ // 优先取tbc中enodeid为nid的
				string tbc_nid = roadmif.mid[m][roadcol.col_enodeid];
				wgt::trim(tbc_nid, '"');
				if (tbc_nid == nid)
				{
					same_vec[m].point += ret*1.5;
				}
			}
			else
			{
				string tbc_nid = roadmif.mid[m][roadcol.col_snodeid];
				wgt::trim(tbc_nid, '"');
				if (tbc_nid == nid)
				{
					same_vec[m].point += ret*1.5;
				}
			}
		}
		// 按照评分排序
		std::sort( same_vec.begin(), same_vec.end(), compareScoreByPoint());
		tbc_idx = same_vec[0].pos;
		if (flag == 1)
		{	// 需要将其他关联的link备份
			for (int m=0; m<ret; m++)
			{
				if (same_vec[m].pos != tbc_idx)
				{
					baklinks.push(same_vec[m].pos);
				}
			}
		}

	}
	if (tbc_idx < 0)
	{ // 下标不可能为负
		return -100;	
	}
	cur_upgradedLink.push_back(tbc_idx);	// 添加到已经提升的vector中
	// next expand
	string tbc_snid(roadmif.mid[tbc_idx][roadcol.col_snodeid]);
	string tbc_enid(roadmif.mid[tbc_idx][roadcol.col_enodeid]);
	string tbc_adj_snid(nid2adjoinid[tbc_snid]);
	string tbc_adj_enid(nid2adjoinid[tbc_enid]);
	if (tbc_snid == nid)
	{
		link_expand(tbc_idx, tbc_enid, 0, upgradedLink, cur_upgradedLink, baklinks, flag);
	}
	else if (tbc_enid == nid)
	{
		link_expand(tbc_idx, tbc_snid, 1, upgradedLink, cur_upgradedLink, baklinks, flag);
	}
	else if (tbc_adj_snid == nid)
	{
		link_expand(tbc_idx, tbc_enid, 0, upgradedLink, cur_upgradedLink, baklinks, flag);
	}
	else if (tbc_adj_enid == nid) 
	{
		link_expand(tbc_idx, tbc_snid, 1, upgradedLink, cur_upgradedLink, baklinks, flag);
	}
	else
	{
		return -1;
	}
	return 0;
}

int find_adjoin_link(int idx, string nid, bool is_start, vector<int>& upgradedLink, vector<int>& cur_upgradedLink, vector<score>& res_vec, int& flag)
{
	string cur_nodeid(nid);
	res_vec.clear();
	set<int>::iterator lset_it, lset_end;
//	if (is_start)
	{	// start node
		lset_it = enid2indexset[cur_nodeid].begin();
		lset_end = enid2indexset[cur_nodeid].end();
		for (; lset_it != lset_end; ++lset_it) 
		{
			int conn_idex = *lset_it;
			if (!isInIntVector(all_uplinkIndex, conn_idex))
			{
				continue;
			}			
			// 当前link是否满足条件
			if (conn_idex!=idx && !isInIntVector(all_layerlinkIndex, conn_idex) && !isInIntVector(upgradedLink, conn_idex) && !isInIntVector(cur_upgradedLink, conn_idex))
			{
				score msco;
				msco.pos = conn_idex;
				msco.point = 0;
				msco.kind_class = atoi(roadmif.mid[msco.pos][roadcol.col_kind_class].c_str());
				msco.length = atof(wgt::trim(roadmif.mid[msco.pos][roadcol.col_length], '"').c_str());
				msco.angle = calculate_angle((Feature<Line>*)roadmif.data.geo_vec[msco.pos],
					(Feature<Line>*)roadmif.data.geo_vec[idx], cur_nodeid == roadmif.mid[msco.pos][roadcol.col_snodeid], is_start);
				res_vec.push_back(msco);
			}
			if (isInIntVector(all_layerlinkIndex, conn_idex))
			{
				flag = 0;
			}			
		}
		lset_it = snid2indexset[cur_nodeid].begin();
		lset_end = snid2indexset[cur_nodeid].end();
		for (; lset_it != lset_end; ++lset_it) 
		{
			int conn_idex = *lset_it;
			if (!isInIntVector(all_uplinkIndex, conn_idex))
			{
				continue;
			}
			// 当前link是否满足条件
			if (conn_idex!=idx && !isInIntVector(all_layerlinkIndex, conn_idex) && !isInIntVector(upgradedLink, conn_idex)  && !isInIntVector(cur_upgradedLink, conn_idex))
			{
				score msco;
				msco.pos = conn_idex;
				msco.point = 0;
				msco.kind_class = atoi(roadmif.mid[msco.pos][roadcol.col_kind_class].c_str());
				msco.length = atof(wgt::trim(roadmif.mid[msco.pos][roadcol.col_length], '"').c_str());
				msco.angle = calculate_angle((Feature<Line>*)roadmif.data.geo_vec[msco.pos],
					(Feature<Line>*)roadmif.data.geo_vec[idx], cur_nodeid == roadmif.mid[msco.pos][roadcol.col_snodeid], is_start);
				res_vec.push_back(msco);
			}
			if (isInIntVector(all_layerlinkIndex, conn_idex))
			{
				flag = 0;
			}
		}
		// adjoin_id
		if (nid2adjoinid.find(cur_nodeid) != nid2adjoinid.end())
		{
			string adj_id(nid2adjoinid[cur_nodeid]);
			lset_it = enid2indexset[adj_id].begin();
			lset_end = enid2indexset[adj_id].end();
			for (; lset_it != lset_end; ++lset_it) 
			{
				int conn_idex = *lset_it;
				if (!isInIntVector(all_uplinkIndex, conn_idex))
				{
					continue;
				}
				// 当前link是否满足条件
				if (conn_idex!=idx && !isInIntVector(all_layerlinkIndex, conn_idex) && !isInIntVector(upgradedLink, conn_idex) && !isInIntVector(cur_upgradedLink, conn_idex))
				{
					score msco;
					msco.pos = conn_idex;
					msco.point = 0;
					msco.kind_class = atoi(roadmif.mid[msco.pos][roadcol.col_kind_class].c_str());
					msco.length = atof(wgt::trim(roadmif.mid[msco.pos][roadcol.col_length], '"').c_str());
					msco.angle = calculate_angle((Feature<Line>*)roadmif.data.geo_vec[msco.pos],
						(Feature<Line>*)roadmif.data.geo_vec[idx], cur_nodeid == roadmif.mid[msco.pos][roadcol.col_snodeid], is_start);
					res_vec.push_back(msco);
				}
				if (isInIntVector(all_layerlinkIndex, conn_idex))
				{
					flag = 0;
				}
			}
			lset_it = snid2indexset[adj_id].begin();
			lset_end = snid2indexset[adj_id].end();
			for (; lset_it != lset_end; ++lset_it) 
			{
				int conn_idex = *lset_it;
				if (!isInIntVector(all_uplinkIndex, conn_idex))
				{
					continue;
				}
				// 当前link是否满足条件
				if (conn_idex!=idx && !isInIntVector(all_layerlinkIndex, conn_idex) && !isInIntVector(upgradedLink, conn_idex) && !isInIntVector(cur_upgradedLink, conn_idex))
				{
					score msco;
					msco.pos = conn_idex;
					msco.point = 0;
					msco.kind_class = atoi(roadmif.mid[msco.pos][roadcol.col_kind_class].c_str());
					msco.length = atof(wgt::trim(roadmif.mid[msco.pos][roadcol.col_length], '"').c_str());
					msco.angle = calculate_angle((Feature<Line>*)roadmif.data.geo_vec[msco.pos],
						(Feature<Line>*)roadmif.data.geo_vec[idx], cur_nodeid == roadmif.mid[msco.pos][roadcol.col_snodeid], is_start);
					res_vec.push_back(msco);
				}
				if (isInIntVector(all_layerlinkIndex, conn_idex))
				{
					flag = 0;
				}
			}
		}
	}
	return res_vec.size();
}

double calculate_angle(Feature<Line>* link1, Feature<Line>* link2, bool is_start1, bool is_start2)
{
	Line& line1 = link1->at(0);
	Line& line2 = link2->at(0);

	size_t size1 = line1.size();
	size_t size2 = line2.size();

	const Point& p11 = is_start1 ? line1.at(0) : line1.at(size1 - 1);
	const Point& p12 = is_start1 ? line1.at(1) : line1.at(size1 - 2);
	const Point& p21 = is_start2 ? line2.at(0) : line2.at(size2 - 1);
	const Point& p22 = is_start2 ? line2.at(1) : line2.at(size2 - 2);

	return std::fabs( angle(p11, p12, p21, p22) );
}

