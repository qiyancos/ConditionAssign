#include "program_helper.h"

class SearchEngine {
public:
    // 构造函数
    SearchEngine(const bool printDetail, const std::string& dataPath,
            const std::vector<std::string>& cityNames,
            const std::vector<std::string>& layerNames) :
            printDetail_(printDetail), dataPath_(dataPath),
            cityNames_(cityNames), layerNames_(layerNames) {}
    // 从所选城市中获取所有可用的Layer
    static int getAllLayers(const std::string& dataPath,
            const std::vector<std::string>& cityNames,
            std::vector<std::string>* allLayers) {
    }
    // 借用awk处理过滤操作
    int process() {
        std::cout << "Please input the condition for searching:" << std::endl;
    }
    // 汇报统计结果
    int report() {
    }

private:
    // 是否需要打印具体的匹配结果
    const bool printDetail_;
    // 搜索数据源路径
    const std::string dataPath_;
    // 需要搜索的城市名集合
    const std::vector<std::string> cityNames_;
    // 需要搜索的Layer名称集合
    const std::vector<std::string> layerNames_;
};

int selectCity(const std::string& dataPath,
        std::vector<std::string>* cityNames) {
    std::vector<std::string> allCityNames;
    CHECK_RET(program_helper::listDir(dataPath, &allCityNames),
            "Failed to list files and dirs in \"%s\".", dataPath.c_str());
    CHECK_RET(program_helper::manualSelect(allCityNames, cityNames),
            "Failed to select target cities.");
    return 0;
}

int selectLayer(const std::string& dataPath,
        const std::vector<std::string>& cityNames,
        std::vector<std::string>* layerNames) {
    std::vector<std::string> allLayerNames;
    CHECK_RET(SearchEngine::getAllLayers(dataPath, cityNames, &allLayerNames),
            "Failed to find available layer in \"%s\".", dataPath.c_str());
    CHECK_RET(program_helper::manualSelect(allLayerNames, layerNames),
            "Failed to select target layers.");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3 && argc != 2) {
        std::cerr << "Usage: " << argv[0]  << " [OPTION] <path_of_data>" <<
                std::endl;
        std::cerr << "       Root path should hold the directory of" <<
                "different cities." << std::endl;
        std::cerr << "       -d, --detail\t\tPrint detail for match info." <<
                std::endl;
        return 0;
    }
    bool printDetail = false;
    std::string dataPath(argv[1]);
    if (argc == 3) {
        if (dataPath == "-d" || dataPath == "--detail") {
            printDetail = true;
            dataPath = argv[2];
        } else {
            std::cerr << "Unknown arguments: " << argv[1] << std::endl;
            return -1;
        }
    }
    std::vector<std::string> cityNames;
    std::vector<std::string> layerNames;
    CHECK_RET(selectCity(dataPath, &cityNames), "Fail to select city names.");
    CHECK_RET(selectLayer(dataPath, cityNames, &layerNames),
            "Fail to select layer names.");
    SearchEngine engine(printDetail, dataPath, cityNames, layerNames);
    engine.process();
    engine.report();
}
