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
    std::cout << "Please select the cities in which you want to search:\n";
    CHECK_RET(program_helper::manualSelect(allCityNames, cityNames),
            "Failed to select target cities.");
    std::cout << "-- The following cities have been selected:\n";
    program_helper::printWithTypeSetting(*cityNames);
    std::cout << std::endl;
    return 0;
}

int selectLayer(const std::string& dataPath,
        const std::vector<std::string>& cityNames,
        std::vector<std::string>* layerNames) {
    std::vector<std::string> allLayerNames;
    CHECK_RET(mif_helper::SearchEngine::getAllLayers(dataPath,
            cityNames, &allLayerNames),
            "Failed to find available layer in \"%s\".", dataPath.c_str());
    std::cout << "Please select the layers in which you want to search:\n";
    CHECK_RET(program_helper::manualSelect(allLayerNames, layerNames),
            "Failed to select target layers.");
    std::cout << "-- The following layers have been selected:\n";
    program_helper::printWithTypeSetting(*layerNames);
    std::cout << std::endl;
    return 0;
}

void printHelpInfo(const std::string binaryPath) {
    std::cerr << "Usage: " << binaryPath  << " [OPTION]" << 
            " <path_of_data>" << std::endl;
    std::cerr << "       -d, --detail\t\tPrint detail for match info." <<
            std::endl;
    std::cerr << "Root path should hold the directory of" <<
            " different cities." << std::endl;
    return 0;
}        

int main(int argc, char** argv) {
    if (argc == 2 && (std::string(argv[1]) == "-h" ||
            std::string(argv[1]) == "--help")) {
        printHelpInfo(argv[0]);
        return 0;
    }
    if (argc != 4 && argc != 3) {
        std::cerr << "Error: Too few or too many arguments." << std::endl;
        printHelpInfo(argv[0]);
        return -1;
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
    CHECK_RET(engine.process(), "Search engine process failed.");
    CHECK_RET(engine.report(), "Search engine report failed.");
}
