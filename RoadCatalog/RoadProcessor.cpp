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

ConfigCatalog::ConfigCatalog(string inputdir, string outputdir, string tencentdir, string confdir, string cityname, string _incrementpath)
{
    strCityName = cityname;
    strInputPath = inputdir + "/" + cityname;
    strOutputPath = outputdir + "/" + cityname;
    strTencentPath = tencentdir;
    strConfPath = confdir;
}

ConfigCatalog::~ConfigCatalog()
{}

bool ConfigCatalog::execute()
{
    LoadKindClassAdjust(strConfPath);

    // 城市级别特殊分类处理
    if (strCityName != "china")
    {
        sys_log_println(_INFORANK, "process C_R catalog\n");
        mapid2dirvec.clear();
        string mapid2dirvec_file = strConfPath + "/mapid2citydir";
        LoadMapid2dirConf(mapid2dirvec_file, mapid2dirvec);
        string layername = "C_R";
        string infile = strInputPath + "/" + layername;
        string outfile = strOutputPath + "/" + layername;
        string backPolygonfile = strInputPath + "/" + "C_BackPolygon";
        if (Road_CatalogEx(infile, outfile, layername, backPolygonfile) < 0)
        {
            sys_log_println(_ERROR, "road: %s error\n", infile.c_str());
            return false;
        }

        //// 道路疏密度,将道路稀少地区的道路的分类提升,保留原有的catalog到新字段oldcatalog
        // sys_log_println(_INFORANK, "process C_R road_upgrade\n");
        // string roadFile = strOutputPath + "/C_R";
        // string nodeFile = strInputPath + "/C_N";
        // string roadupconfFile = strConfPath + "/road_upgrade.conf";
        // RoadLevelUpgrade(roadFile, nodeFile, roadupconfFile);

        // 2017.09.20   精细化区域面道路主干道处理
        //  - 数据端提供一个外挂表, 通过外挂表的标识字段修改关联link的catalog
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
    sys_log_println(_INFORANK, "finish to catalog all layers\n");
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
        if (strItem[i] == '"')        // 识别到引号, 切换引号状态
        {
            quot_status ^= true;
        }

        if (!quot_status && (strItem[i] == seprator[0]))        // 非引号状态下才识别分隔符
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
    int col_constST = roadTemp.get_col_pos("const_st");        // 供用信息
    int col_uflag =  roadTemp.get_col_pos("uflag");     //城市标识，区分市区和非市区
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
        //    str_kind_class = "0c";
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

    // step3 匝道（IC、JCT、匝道、提前左转、提前右转、掉头口、主辅出入口) kindclass等于所连接道路（排除行人道路、轮渡）的kindclass的次低级
    ProcessRampKindClass(road_Mif, in_path);
    sys_log_println(_INFORANK, "Road_CatalogEx step3 finish\r\n");

    // step4 服务区、停车区字段a值处理：如连接匝道，按匝道a值，如只连接高快速，则a值为06
    ProcessSAKindClass(road_Mif, in_path);
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
//         if(wgt::trim(road_Mif.mid[index][col_pathname],'"') == "" )
//         {
//             road_Mif.mid[index][col_pathname] = road_Mif.mid[index][col_sign_name];
//         }
        bool isConstruction = constST == "4" || constST == "3";
        bool isUrban = build_in_flag == "01";

        if (isWalkstreet(kind))
        {    // 步行街
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
            //             else if((road_Mif.mid[index][col_sign_name][1]=='G'&& IsNum(road_Mif.mid[index][col_sign_name],2,road_Mif.mid[index][col_sign_name].size()-3)))
            //             {
            //                 // 编号为"G"开头或者为空
            //                 catalog += "01";
            // 
            //             }
            else if (!sign_name.empty() && sign_name[1] == 'S'&& IsNum(sign_name, 2, sign_name.size() - 3))
            {
                // 编号为"S"开头或者为空
                catalog += "02";
            }
            else if (isRamp(kind))
            {    // JCT
                catalog += "05";
            }
            else
            {    // 其他
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
            {    // JCT
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
            {    // JCT
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
            {    // JCT
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
            {    // JCT
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
            {    // JCT
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
            {    // JCT
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
        if (rattr == "0b"    // 匝道
            || rattr == "12"    // 提前右转
            || rattr == "15"    // 提前左转
            || rattr == "16"    // 掉头口
            || rattr == "17"    // 主辅路出入口
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
        if (rattr == "05"            // IC
            || rattr == "03")        // JCT
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
        if (rattr == "0b")    // 匝道
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
        if (rattr == "07"        // 服务区
            || rattr == "06"    // 停车区
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
        if (rattr == "04")        // 交叉点内link
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
        if (rattr == "0f")        // 隧道
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
            rattr == "1b")        // 移动式桥
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
        if (rattr == "0a")        // 辅路
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
        if (rattr == "17")        // 主辅路出入口
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
        if (rattr != "0a" && rattr2 == "09")        // 步行街
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
        if (rattr1 == ignoreAttr && rattr1 == "01")        // 忽略的道路属性
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
        if (rattr2 == ignoreAttr && rattr2 == "01")        // 忽略的道路属性
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
        if (rattr == "18")            // 虚拟链接路
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
    string path = city_path.substr(0, city_path.find_last_of('/'));        // 用于后面城市扩展  ../data/01_basic/05_Recatalog
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

    map<string, int> linkid2indexmap;            // linkid ==> pos
    map<string, vector<string> > nodeid2lids;    // nodeid ==> linkids
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

    map<string, string> citydirlist;        // 已经加载进来的城市
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

            map<int, string> kindclassmap_reliefroad;    // 连接的辅路kindclass值
            map<int, string> kindclassmap_direct;        // 与当前link直接连接的其他kindclass值
            
            for(int i=0;i<2;i++)
            {
                map<int, string> kindclassmap;                // 连接的kindclass值
                map<int, int> directindex_map;                // 直连的index pos
                map<int, int> directindexrlink_map;            // 直连的交叉点内link
                set<int> linkvisited;    // 已经访问的link
                queue<LinkItem> linkindexQueue;            // 存放road的下标

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
                                        //    if(direction_connect == "2" && snodeid_connect == snodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //    if(direction_connect == "3" && enodeid_connect == snodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //}
                                        //if(direction == "3")
                                        //{
                                        //    if(direction_connect == "2" && enodeid_connect == snodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //    if(direction_connect == "3" && snodeid_connect == snodeid)
                                        //    {
                                        //        continue;
                                        //    }
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
                                        //    if(direction_connect == "2" && enodeid_connect == enodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //    if(direction_connect == "3" && snodeid_connect == enodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //}
                                        //if(direction == "3")
                                        //{
                                        //    if(direction_connect == "2" && snodeid_connect == enodeid)
                                        //    {
                                        //        continue;
                                        //    }
                                        //    if(direction_connect == "3" && enodeid_connect == enodeid)
                                        //    {
                                        //        continue;
                                        //    }
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
    wgt::MIF C_N_Mif;
    string city_path = in_path.substr(0, in_path.find_last_of('/'));
    string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
    string path = city_path.substr(0, city_path.find_last_of('/'));        // 用于后面城市扩展  ../data/01_basic/05_Recatalog
    string node_path = city_path + "/C_N";

    if (wgt::mif_to_wsbl(node_path, C_N_Mif) < 0)
    {
        sys_log_println(_ERROR, "ProcessSAKindClass read C_N mif error! %s\n", node_path.c_str());
        return;
    }
    // col index
    int col_linkmapid = road_Mif.get_col_pos("mapid");
    int col_linkid = road_Mif.get_col_pos("id");
    int col_kindclass = road_Mif.get_col_pos("kind_class");
    int col_kind = road_Mif.get_col_pos("kind");
    int col_snodeid = road_Mif.get_col_pos("snodeid");
    int col_enodeid = road_Mif.get_col_pos("enodeid");

    int col_nodemapid = C_N_Mif.get_col_pos("mapid");
    int col_nodeid = C_N_Mif.get_col_pos("id");
    int col_adjoin_nid = C_N_Mif.get_col_pos("adjoin_nid");
    int col_node_lid = C_N_Mif.get_col_pos("node_lid");

    map<string, int> linkid2indexmap;            // linkid ==> pos
    map<string, vector<string> > nodeid2lids;    // nodeid ==> linkids

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

    map<string, string> citydirlist;        // 已经加载进来的城市
    citydirlist[cur_city] = "";

    nSize = road_Mif.mid.size();
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
            // 找出与这个SA相连的路段
            vector<int> linkposvec;
            map<string, string> linkvisited;    // 已经访问的link
            queue<int> linkindexQueue;            // 存放road的下标
            linkindexQueue.push(index);
            while (linkindexQueue.size() != 0)
            {
                int pos = linkindexQueue.front();
                linkindexQueue.pop();
                string linkid = road_Mif.mid[pos][col_linkid];
                string kind = road_Mif.mid[pos][col_kind];
                string kindclass = road_Mif.mid[pos][col_kindclass];
                wgt::trim(linkid, '"');
                wgt::trim(kind, '"');
                wgt::trim(kindclass, '"');
                linkvisited[linkid] = "";        // 添加到 visited
                if (!isSA(kind))
                {
                    linkposvec.push_back(pos);
                }
                else
                {
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
                            if (linkvisited.find(lid) == linkvisited.end())
                            {
                                map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
                                if (iter2 != linkid2indexmap.end())
                                {
                                    linkindexQueue.push(iter2->second);
                                }
                                else
                                {
                                    // 如果扩展后还是没找到呢
                                    sys_log_println(_ASSERT, "ProcessSAKindClass can not find lid: %s \n", lid.c_str());
                                }
                            }
                        }
                    }
                    else
                    {
                        sys_log_println(_ASSERT, "ProcessSAKindClass can not find snodeid: %s \n", snodeid.c_str());
                    }
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    iter_find = nodeid2lids.find(enodeid);
                    if (iter_find != nodeid2lids.end())
                    {
                        vector<string>& res = iter_find->second;
                        for (vector<string>::iterator iter1 = res.begin(); iter1 != res.end(); iter1++)
                        {
                            string lid = *iter1;
                            if (linkvisited.find(lid) == linkvisited.end())
                            {
                                map<string, int>::iterator iter2 = linkid2indexmap.find(lid);
                                if (iter2 != linkid2indexmap.end())
                                {
                                    linkindexQueue.push(iter2->second);
                                }
                                else
                                {
                                    // 如果扩展后还是没找到呢
                                    sys_log_println(_ASSERT, "ProcessSAKindClass can not find lid: %s \n", lid.c_str());
                                }
                            }
                        }
                    }
                    else
                    {
                        sys_log_println(_ASSERT, "ProcessRampKindClass can not find enodeid: %s \n", enodeid.c_str());
                    }
                }
            }
            // 遍历连接的非SA路段
            bool ishighway = true;    // 连接是否为高快速
            bool isram = false;        // 连接是否为匝道
            bool isused = true;    // 连接的道路是否使用
            for (vector<int>::iterator iter = linkposvec.begin(); iter != linkposvec.end(); ++iter)
            {
                int pos = *iter;
                string kindclass = road_Mif.mid[pos][col_kindclass];
                string kind = road_Mif.mid[pos][col_kind];
                wgt::trim(kind, '"');
                wgt::trim(kindclass, '"');
                if (kindclass != "00" && kindclass != "01")        // 高快速
                {
                    ishighway = false;
                }
                if (isRamp(kind))
                {
                    isram = true;
                    rampkindclass = kindclass;
                }
                if (kindclass == "0c")
                {
                    isused = false;
                    break;
                }
            }
            if (!isram && ishighway)
            {
                rampkindclass = "06";
            }
            if (isused == false)
            {
                rampkindclass = "0c";
            }
            road_Mif.mid[index][col_kindclass] = rampkindclass;
        }
    }
}

void ConfigCatalog::ProcessRLinkKindClass(wgt::MIF& road_Mif, string in_path)
{
    wgt::MIF C_N_Mif;
    wgt::MIF C_R_Mif = road_Mif;    // 因为后面会根据mapid扩展MIF,此处直接拷贝
    string city_path = in_path.substr(0, in_path.find_last_of('/'));
    string cur_city = city_path.substr(city_path.find_last_of('/') + 1);
    string path = city_path.substr(0, city_path.find_last_of('/'));        // 用于后面城市扩展  ../data/01_basic/05_Recatalog
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

    map<string, int> linkid2indexmap;            // linkid ==> pos
    map<string, vector<string> > nodeid2lids;    // nodeid ==> linkids
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

    map<string, string> citydirlist;        // 已经加载进来的城市
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

            bool flag = true;                    // 是否"仅连接高快速和服务区，停车区、匝道"
            map<int, string> kindclassmap;        // 连接的kindclass值



            for(int i=0;i<2;i++)
            {
                map<int, string> kindclassmapEx;        // 连接的kindclass值

                map<int, string> kindclassmapSameFuncClass; //同FuncClass的连接的kindclass值

                set<LinkItem> linkvisited;    // 已经访问的link
                queue<LinkItem> linkindexQueue;            // 存放road的下标

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
    string path = city_path.substr(0, city_path.find_last_of('/'));        // 用于后面城市扩展  ../data/01_basic/05_Recatalog
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

    map<string, int> linkid2indexmap;            // linkid ==> pos
    map<string, vector<string> > nodeid2lids;    // nodeid ==> linkids

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

    map<string, string> citydirlist;        // 已经加载进来的城市
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

            queue<int> linkindexQueue;            // 存放road的下标
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
    string path = city_path.substr(0, city_path.find_last_of('/'));        // 用于后面城市扩展  ../data/01_basic/05_Recatalog
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

    map<string, int> linkid2indexmap;            // linkid ==> pos
    map<string, vector<string> > nodeid2lids;    // nodeid ==> linkids

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
    

    for (int kindclassProcess=3; kindclassProcess<=9; kindclassProcess++)
    {
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

                queue<int> linkindexQueue;            // 存放road的下标
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
                                wgt::trim(rampkindclass, '"');
                                wgt::trim(build_in_flag, '"');
                                wgt::trim(length, '"');
                                int kindclass = strtol(rampkindclass.c_str(),NULL,16);

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
                                wgt::trim(rampkindclass, '"');
                                wgt::trim(build_in_flag, '"');
                                wgt::trim(length, '"');
                                int kindclass = strtol(rampkindclass.c_str(),NULL,16);

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

                if (group.length < 2 && (group.connected_min_kind_class <= 9 || group.connected_min_kind_class == 11) && !group.connected_ferry)
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
    int nSize_pre = C_R_Mif.mid.size();        // 原来的size

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

int ConfigCatalog::RoadLevelUpgrade(string roadFile, string nodeFile, string roadupconfFile)
{
    // 这个配置是可以多重配置
    typedef struct _roadUpConfig
    {
        std::map<string, string> layerCatalog;
        std::map<string, string> upCatalog;
        float limitLen;
        int empty_block_handle;        // 1:全部提升 0:不处理
        string upValue;
    }roadUpConfig;

    roadUpConfig confs[10];
    int curCount = 0;
    int isStart = 0;
    int layerType = -1;    // layerCatalog:0; upCatalog:1; limitLen:2
    {
        std::string strline;
        vector<string> splitItems;
        std::ifstream infile(roadupconfFile.data());
        while (infile.good() && !infile.eof())
        {
            strline.clear();
            getline(infile, strline);
            wgt::trim(strline, '\r');    //换行符
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
