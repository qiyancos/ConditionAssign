#include "Processer.h"
#include "Config.h"
#include "Layer.h"
#include <tx_common.h>
#include <list>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <functional>

using namespace std;

namespace basemap
{

int Processer::LoadConfig(const std::string& conf_file)
{
    if (ParseConfigGroupByFile(_config_group_, conf_file)) {
        // 多线程环境下补充字段可能发生段错误
        // 这里每次加载配置后将目标字段同步更新到MIF对象中
        _SyncMifColumn();

        return _config_group_.size();
    }
    return -1;
}

int Processer::LoadInput(const std::string& input_file)
{
    if (_layer_.Load(input_file)) {
        sys_log_println(_INFORANK, "mif layer size = %d\n", _layer_.mif_.mid.size());
        return _layer_.mif_.mid.size();
    }
    return -1;
}

int Processer::run(int thread_num)
{
    int idx = 0;
    int total = _layer_.mif_.mid.size();

    list<thread> threads;
    for (int i = 0; i < thread_num; ++i) {
        threads.push_back(thread(bind(&Processer::_Worker, this, std::ref(idx), total)));
    }
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}

int Processer::Save(const std::string& output_file)
{
    if (_layer_.Save(output_file)) {
        return 0;
    }
    return -1;
}

void Processer::_Worker(int& idx, const int& total)
{
    while (true) {
        _mutex_.lock();
        int cur_idx = idx;
        if (idx >= total) {
            _mutex_.unlock();
            break;
        }
        ++idx;
        _mutex_.unlock();

        // run
        _layer_.AssignByConfigList(_config_group_, cur_idx);
    }
}

void Processer::_SyncMifColumn()
{
    for (auto& conf : _config_group_) {
        for (auto& rule : conf.target_group_) {
            _layer_.GetOrAddMifPos(rule.field_);
        }
    }
}
}

