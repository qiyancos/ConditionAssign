/************************************************************************/
/*                道路连通性分类策略;道路疏密度调整等				    */
/************************************************************************/
#include "RoadCatalog.h"
#include "RoadProcessor.h"

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, "Chinese-simplified");

	if (argc < 7) {
		std::cout << "Usage: " << argv[0] << " <ScrDataPath>" <<
                " <TargetDataPath> <PluginDataPath> <ConfigPath> " <<
                "<LogPath> <CityName>" << std::endl;
		return -1;
	}
	std::string inputdir(argv[1]);
	std::string outputdir(argv[2]);
    std::string tencentdir(argv[3]);  // 2017.09.20   增加自研数据目录
	std::string confdir(argv[4]);
	std::string logdir(argv[5]);
	std::string cityname(argv[6]);

	sys_log_path(logdir.c_str(), cityname.c_str());

	RoadProcessor roadProcessor(inputdir, outputdir, tencentdir,
            confdir, cityname);
	roadProcessor.execute();

	sys_log_flush();
	sys_log_close();

	return 0;
}
