#include "ConditionAssign.h"
#include "ExecutorPool.h"
#include "conf_helper.h"

using namespace condition_assign;

#ifdef DEBUG
std::string debugLogDir;
std::vector<std::ofstream> debugStream;
#endif

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Chinese-simplified");
    if (argc < 9) {
        std::cout << "Usage: " << std::string(argv[0]);
        for (std::string argName : conf_helper::argList) {
            std::cout << " <" << argName << ">";
        }
        std::cout << std::endl;
        return 0;
    }

    std::string maxExecutor, logDir;
    std::vector<std::string> inputLayers, configFiles, inputGeoTypes,
            outputLayers, pluginLayers;
    conf_helper::ConfArgParser argParser(argc, argv);

    CHECK_EXIT(argParser.findArgByName("MaxExecutor", &maxExecutor),
            "Can not find argument MaxExecutor!");
    CHECK_EXIT(argParser.findArgByName("LogPath", &logDir),
            "Can not find argument LogPath!");
    CHECK_EXIT(argParser.findArgByName("ConfPath", &configFiles),
            "Can not find argument ConfPath!");
    CHECK_EXIT(argParser.findArgByName("SourceLayer", &inputLayers),
            "Can not find argument SourceLayer!");
    CHECK_EXIT(argParser.findArgByName("TargetLayer", &outputLayers),
            "Can not find argument TargetLayers!");
    CHECK_EXIT(argParser.findArgByName("PluginLayer", &pluginLayers),
            "Can not find argument PluginLayers!");
    CHECK_EXIT(argParser.findArgByName("InputGeoType", &inputGeoTypes),
            "Can not find argument InputGeoTypes!");
    int executorNum = atoi(maxExecutor.c_str());

    time_t start = 0, end = 0;
    time(&start);

    // ������־
    char date_str[256];
    strftime(date_str, sizeof(date_str), "%Y%m%d", localtime(&start));
    sys_log_path(logDir.c_str(), date_str);
#ifdef DEBUG
    debugLogDir = logDir;
#endif

    sys_log_println(_INFORANK, "=====================================\n");
    sys_log_println(_INFORANK, "          [ ConditionAssign ]        \n");
    for (int index = 0; index < inputLayers.size(); index++) {
        sys_log_println(_INFORANK, "-- InputLayers: [%d]-[%s]\n",
                index + 1, inputLayers[index].c_str());
    }
    for (int index = 0; index < configFiles.size(); index++) {
        sys_log_println(_INFORANK, "-- ConfigPath: [%d]-[%s]\n",
                index + 1, configFiles[index].c_str());
    }
    for (int index = 0; index < outputLayers.size(); index++) {
        sys_log_println(_INFORANK, "-- OutputLayers: [%d]-[%s]\n",
                index + 1, outputLayers[index].c_str());
    }
    sys_log_println(_INFORANK, "-- ThreadNum: [%d]\n", executorNum);
    sys_log_println(_INFORANK, "-- LogDir: [%s]\n", logDir.c_str());

    ExecutorPool::Params poolParams;
    poolParams.executorNum = executorNum;
    poolParams.input = inputLayers;
    poolParams.outputs = outputLayers;
    poolParams.plugins = pluginLayers;
    poolParams.geoTypes = inputGeoTypes;
    poolParams.configs = configFiles;
    
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
    setlocale(LC_ALL, "C");
    for (auto regOp : syntax::operatorList) {
        delete regOp;
    }
    return 0;
}
