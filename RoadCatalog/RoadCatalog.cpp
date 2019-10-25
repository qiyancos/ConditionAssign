/************************************************************************/
/*              道路连通性分类策略;道路疏密度调整等			            */
/************************************************************************/
#include "ConfigCatalog.h"

int main(int argc,char *argv[])  {
	setlocale(LC_ALL,"Chinese-simplified");

	if (argc < 7) {
		std::cout << "Usage: " << argv[0] << " <ScrDataPath>" <<
                " <TargetDataPath> <PluginDataPath> <ConfigPath> " <<
                "<LogPath> <CityName>" << std::endl;
		return -1;
	}
	
    string inputdir(argv[1]);
	string outputdir(argv[2]);
    string tencentdir(argv[3]);  // 2017.09.20   增加自研数据目录
	string confdir(argv[4]);
	string logdir(argv[5]);
	string cityname(argv[6]);

	sys_log_path(logdir.c_str(), cityname.c_str());

	ConfigCatalog configCatalog(inputdir, outputdir, tencentdir, confdir,
            cityname);
	configCatalog.execute();

	sys_log_flush();
	sys_log_close();

	return 0;
}
