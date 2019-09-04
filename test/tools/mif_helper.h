#ifndef MIF_HELPER_H
#define MIF_HELPER_H

#include "program_helper.h"

#include <string>
#include <vector>
#include <map>

namespace mif_helper {

struct MifHeader {
    // 版本信息
    int version;
    // 数据字符集标准
    std::string charset;      
    // mid分隔符
    char delimiter;
    // 与数据库有关
    std::vector<int> uniqueVec;   
    // 与数据库有关
    std::vector<int> indexVec;    
    // 当前mif使用的坐标系统类型
    std::string coordsys;
    // 与坐标系转换有关
    std::string transform;    
    
    // mif的字段总数
    int colNum;
    // 所有的字段名称(已转换为小写)
    std::vector<std::string> colNameVec;
    // 所有的字段对应的数据类型
    std::vector<std::string> colTypeVec;
    // 字段到对应的列数的索引
    std::map<std::string, int> colNameMap;

    // 经纬度坐标系统对应的名称
    static const std::string COORDSYS_LL;
    // 墨卡托坐标系统对应的名称
    static const std::string COORDSYS_MC;

    MifHeader() : version(300), charset("WindowsSimpChinese"), delimiter('\t'),
            coordsys(COORDSYS_MC), transform(""), colNum(0) {}
};

// 读取一个Mif文件的头部信息
int loadMifHeader(const std::string& layerPath, MifHeader* mifHeader);

// 对文件名进行标准化修改
int formalizeFileName(std::string* mifFile, std::string* midFile);

// 使用md5检查mif文件相似性
bool checkSameMif(const std::string& file1, const std::string& file2);

// 检查mid文件是否相同
bool checkSameMid(const std::string& file1, const std::string& file2);

class SearchEngine {
public:
    // 构造函数
    SearchEngine(const int threadNum, const bool printDetail,
            const std::string& dataPath,
            const std::vector<std::string>& cityNames,
            const std::vector<std::string>& layerNames) :
            threadNum_(threadNum), printDetail_(printDetail),
            dataPath_(dataPath), cityNames_(cityNames),
            layerNames_(layerNames) {}

    // 从所选城市中获取所有可用的Layer
    static int getAllLayers(const std::string& dataPath,
            const std::vector<std::string>& cityNames,
            std::vector<std::string>* allLayers);

    // 借用awk处理过滤操作
    int process();

    // 汇报统计结果
    int report();

private:
    // 生成对应的匹配条件
    int inputAndParseConfig();

    // 基于原生的数据进行统计
    int processResult();

    // 对不同的Layer生成并执行的指令
    static int worker(SearchEngine* engine, const int startIndex,
            const int endIndex, const int threadId);
    
    // 对不同的Layer生成并执行的指令
    static int executeCommand(SearchEngine* engine, const int startIndex,
            const int endIndex);

private:
    // 完整的统计结果
    std::map<std::string, std::map<std::string,
            std::vector<std::string>>> result_;
    // 统计的结果按城市与Layer
    std::map<std::string, std::map<std::string, int>> sumCityLayer_;
    // 统计的结果按城市的计算
    std::map<std::string, int> sumCity_;
    // 统计的结果按Layer的计算
    std::map<std::string, int> sumLayer_;
    // 总共的数据量
    int totalSum_;

private:
    // 搜索引擎的线程数
    const int threadNum_;
    // 线程的返回状态
    std::vector<int> threadStates_;
    // 搜索引擎的进度条
    program_helper::Progress* progressBar_ = nullptr;

    // 是否需要打印具体的匹配结果
    const bool printDetail_;
    // 搜索数据源路径
    const std::string dataPath_;
    // 需要搜索的城市名集合
    const std::vector<std::string> cityNames_;
    // 需要搜索的Layer名称集合
    const std::vector<std::string> layerNames_;
    // 匹配条件的简单解析结果
    std::vector<std::pair<std::string, std::string>> configs_;
    // 主要的执行队列
    std::vector<std::pair<std::string, std::vector<std::string>*>> jobQueue_;
};

} // namespace mif_helper

#endif // MIF_HELPER_H
