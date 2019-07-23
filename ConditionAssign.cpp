#include "ConditionAssign.h"
#include "Executor.h"
#include "conf_helper.h"

using namespace condition_asssign;

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Chinese-simplified");
    if (argc < 9) {
        std::cout << "Usage: ./ConditionAssign";
        for (std::string argName : conf_helper::argList) {
            std::cout << " <" << argName << ">";
        }
        std::cout << std::endl;
        return 0;
    }

    std::string maxExecutor, logDir, inputLayer, inputGeoType;
    std::vector<std::string> confFiles, outputLayers, pluginlayers;
    conf_helper::ConfArgParser argParser(argc, argv);

    CHECK_EXIT(argParser.findArgByName("MaxExecutor", &maxExecutor),
            "Can not find argument MaxExecutor!");
    CHECK_EXIT(argParser.findArgByName("LogPath", &logDir),
            "Can not find argument LogPath!");
    CHECK_EXIT(argParser.findArgByName("ConfPath", &confFiles),
            "Can not find argument ConfPath!");
    CHECK_EXIT(argParser.findArgByName("SourceLayer", &inputLayer),
            "Can not find argument SourceLayer!");
    CHECK_EXIT(argParser.findArgByName("TargetLayer", &outputLayers),
            "Can not find argument TargetLayers!");
    CHECK_EXIT(argParser.findArgByName("PluginLayer", &pluginLayers),
            "Can not find argument PluginLayers!");
    CHECK_EXIT(argParser.findArgByName("SourceGeoType", &inputGeoType),
            "Can not find argument SourceGeoType!");
    int executorNum = atoi(maxExecutor);

    time_t start = 0, end = 0;
    time(&start);

    // …Ë÷√»’÷æ
    char date_str[256];
    strftime(date_str, sizeof(date_str), "%Y%m%d", localtime(&start));
    sys_log_path(log_dir.c_str(), date_str);

    sys_log_println(_INFORANK, "=====================================\n");
    sys_log_println(_INFORANK, "          [ ConditionAssign ]        \n");
    sys_log_println(_INFORANK, "-- InputLayer: [%s]\n", inputLayer.c_str());
    sys_log_println(_INFORANK, "-- OutputLayers: [%s]\n", outputLayers.c_str());
    sys_log_println(_INFORANK, "-- ThreadNum: [%d]\n", executorNum);
    sys_log_println(_INFORANK, "-- LogDir: [%s]\n", logDir.c_str());

    ExecutorPool::Params poolParams;
    poolParams.executorNum = executorNum;
    poolParams.input = inputLayer;
    poolParams.geoType = inputGeoType;
    poolParams.outputs = outputLayers;
    poolParams.plugins = pluginLayers;
    poolParams.configs = confFiles;
    
    ExecutorPool mainPool(poolParams);
    CHECK_RET(mainPool.init(), "ExecutorPool failed to init.");

    sys_log_println(_INFORANK, "Processing...\n");
    CHECK_EXIT(mainPool.execute(),
            "Error: errors occurred while processing data");

    time(&end);
    sys_log_println(_INFORANK, "Process Finished. Cost time = %ds\n",
            (end - start));
    sys_log_println(_INFORANK, "=====================================\n\n");
    sys_log_close();
    return 0;
}
