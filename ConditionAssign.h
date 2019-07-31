#ifndef CONDITIONASSIGN_H
#define CONDITIONASSIGN_H
/*************************************************
== ConditionAssign ==
Update on: 2019/07/10
Create on: 2018/08/27

根据图层属性条件设置字段值

支持将几个连续的条件配置组装成一个配置流(配置间用 "|" 分隔)
这样只进行一次I/O操作, 在内存中依次处理各个配置, 减少时间损耗

*************************************************/

#include "htk/str_helpers.h"

#include <tx_common.h>
#include <tx_platform.h>

#include <locale>
#include <iostream>

#ifdef DEBUG
#include <time.h>
#include <unistd.h>
#include <ctime>
#include <sstream>
#include <utility>
#include <fstream>
extern std::vector<std::ofstream> debugStream;
extern std::string debugLogDir;
#endif

namespace condition_assign {

#define CHECK_EXIT(expr, info, ...) { \
    int errCode = expr; \
    if (errCode < 0) { \
        sys_log_println(_ERROR, (std::string(__FILE__) + "[%d]: " + info + \
                " In [%s]." + " Error code = %d\n").c_str(), \
                __LINE__, ##__VA_ARGS__, __func__, errCode); \
        exit(errCode); \
    } \
}

#define CHECK_RET(expr, info, ...) { \
    int retCode = expr; \
    if (retCode < 0) { \
        sys_log_println(_ERROR, (std::string(__FILE__) + "[%d]: " + info + \
                " In [%s]." + " Return %d\n").c_str(), \
                __LINE__, ##__VA_ARGS__, __func__, retCode); \
        return retCode; \
    } \
}

#define CHECK_ARGS(expr, info, ...) { \
    if (!(expr)) { \
        sys_log_println(_ERROR, (std::string(__FILE__) + "[%d]: " + info + \
                " In [%s].\n").c_str(), __LINE__, ##__VA_ARGS__, __func__); \
        return -1; \
    } \
}

#ifdef DEBUG
inline int getDebugIndex(const std::string content) {
    if (content == "main") {
        return 0;
    } else if (content == "rc") {
        return 1;
    }
    return 0;
}

inline int getDebugIndex(const int id) {
    return id + 2;
}

#define TEST(String) {\
    time_t now; \
    time(&now); \
    struct tm *nowTime; \
    nowTime=localtime(&now); \
    std::ofstream& streamOut = debugStream[getDebugIndex(String)]; \
    streamOut << "Test line:" << int(__LINE__) << " within func["; \
    streamOut << __func__ << "] in file[" << __FILE__; \
    streamOut << "] at time [" << nowTime->tm_mon + 1 << "/"; \
    streamOut << nowTime->tm_mday << " " << nowTime->tm_hour << ":"; \
    streamOut << nowTime->tm_min << ":" << nowTime->tm_sec << "]."; \
    streamOut << std::endl; \
}
#else
#define TEST(String)
#endif

} // namespace condition_assign

#endif // CONDITIONASSIGN_H
