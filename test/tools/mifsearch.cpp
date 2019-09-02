#include "program_helper.h"
#include "mif_helper.h"

#include <iostream>
#include <string>
#include <vector>

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
    CHECK_RET(mif_helper::SearchEngine::getAllLayers(dataPath,
            cityNames, &allLayerNames),
            "Failed to find available layer in \"%s\".", dataPath.c_str());
    CHECK_RET(program_helper::manualSelect(allLayerNames, layerNames),
            "Failed to select target layers.");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 4 && argc != 3) {
        std::cerr << "Usage: " << argv[0]  << " <thread number> [OPTION]" << 
                " <path_of_data>" << std::endl;
        std::cerr << "       -d, --detail\t\tPrint detail for match info." <<
                std::endl;
        std::cerr << "\nRoot path should hold the directory of" <<
                "different cities." << std::endl;
        return 0;
    }
    bool printDetail = false;
    const int threadNum = atoi(argv[1]);
    CHECK_ARGS(threadNum > 0, "Illegal thread number \"%s\".", argv[1]);
    std::string dataPath(argv[2]);
    if (argc == 4) {
        if (dataPath == "-d" || dataPath == "--detail") {
            printDetail = true;
            dataPath = argv[3];
        } else {
            std::cerr << "Unknown arguments: " << argv[2] << std::endl;
            return -1;
        }
    }
    std::vector<std::string> cityNames;
    std::vector<std::string> layerNames;
    CHECK_RET(selectCity(dataPath, &cityNames), "Fail to select city names.");
    CHECK_RET(selectLayer(dataPath, cityNames, &layerNames),
            "Fail to select layer names.");
    mif_helper::SearchEngine engine(threadNum, printDetail, dataPath,
            cityNames, layerNames);
    engine.process();
    engine.report();
}
