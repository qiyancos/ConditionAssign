#pragma once
#ifndef CONDITIONASSIGN_PROCESSER_H__
#define CONDITIONASSIGN_PROCESSER_H__

#include <string>
#include <mutex>
#include "Config.h"
#include "Layer.h"

namespace basemap
{

class Processer
{
public:
    int LoadConfig(const std::string& conf_file);

    int LoadInput(const std::string& input_file);

    int run(int thread_num = 1);

    int Save(const std::string& output_file);

private:
    ConfigList _config_group_;
    Layer _layer_;
    std::mutex _mutex_;

    void _Worker(int& idx, const int& total);

    void _SyncMifColumn();
};
}


#endif // CONDITIONASSIGN_PROCESSER_H__
