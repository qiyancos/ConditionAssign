#ifndef CONDITIONASSIGN_H
#define CONDITIONASSIGN_H
/*************************************************
== ConditionAssign ==
Update on: 2019/07/10
Create on: 2018/08/27

����ͼ���������������ֶ�ֵ

֧�ֽ���������������������װ��һ��������(���ü��� "|" �ָ�)
����ֻ����һ��I/O����, ���ڴ������δ����������, ����ʱ�����

*************************************************/

#include "htk/str_helpers.h"

#include <tx_common.h>
#include <tx_platform.h>

#include <locale>
#include <iostream>

#ifdef USE_TIMER
#include <chrono>
#endif  // USE_TIMER

#ifdef DEBUG
#include <time.h>
#include <unistd.h>
#include <ctime>
#include <sstream>
#include <utility>
#include <fstream>

extern std::vector<std::ofstream> debugStream;
extern std::string debugLogDir;
#endif // DEBUG

namespace condition_assign {

// ���涨�����ĸ���麯��
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

#define CHECK_WARN(expr, info, ...) { \
    if (!(expr)) { \
        sys_log_println(_WARN, (std::string(__FILE__) + "[%d]: " + info + \
                " In [%s].\n").c_str(), __LINE__, ##__VA_ARGS__, __func__); \
    } \
}

#define CHECK_ARGS(expr, info, ...) { \
    if (!(expr)) { \
        sys_log_println(_ERROR, (std::string(__FILE__) + "[%d]: " + info + \
                " In [%s].\n").c_str(), __LINE__, ##__VA_ARGS__, __func__); \
        return -1; \
    } \
}

// ��ʱ����ʹ�õĺ꣬���㹹����������ʱ�䣨΢�룩
#ifdef USE_TIMER

#define TIMER() \
    timer newTimer(__func__, __FILE__, __LINE__);

class timer {
public:
    timer(const std::string funcName, const std::string fileName,
            const int lineCnt) : funcName_(funcName), fileName_(fileName),
            lineCnt_(lineCnt) {
        start_ = std::chrono::system_clock::now();
    }

    ~timer() {
        end_ = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_ - start_);
        std::cout << "[" << funcName_ << "-" << lineCnt_ << "] In (";
        std::cout << funcName_ << ") Total time: ";
        std::cout << static_cast<double>(duration.count());
        std::cout << " Micro Second." << std::endl;
    }

private:
    const std::string funcName_, fileName_;
    const int lineCnt_;
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point end_;
};

#endif // USE_TIMER

// ���߳�debug��صĺ�
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

#else // DEBUG

#define TEST(String)

#endif // DEBUG

} // namespace condition_assign

#endif // CONDITIONASSIGN_H
