#ifndef ROADCATALOG_H
#define ROADCATALOG_H
/*************************************************
                == RoadCatalog ==
              Update on: 2019/09/18
              Create on: 2018/08/27
*************************************************/

#include "htk/str_helpers.h"
#include <stdint.h>
#include <locale>
#include <iostream>

#ifdef USE_TIMER
#include <chrono>
#endif  // USE_TIMER

// 下面定义了四个检查函数
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

// 计时测试使用的宏，计算构造和析构间的时间（微秒）
#ifdef USE_TIMER

#define TIME_STAMP() \
    timer::printTimeStamp(__func__, __FILE__, __LINE__);

#define TIMER() \
    timer newTimer(__func__, __FILE__, __LINE__);

class timer {
public:
    timer(const std::string funcName, const std::string fileName,
            const int lineCount) : funcName_(funcName), fileName_(fileName),
            lineCount_(lineCount) {
        start_ = std::chrono::system_clock::now();
    }

    ~timer() {
        end_ = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_ - start_);
        std::cout << "[" << funcName_ << "-" << lineCount_ << "] In (";
        std::cout << funcName_ << ") Total time: ";
        std::cout << static_cast<double>(duration.count());
        std::cout << " Micro Second." << std::endl;
    }

    static void printTimeStamp(const std::string funcName,
            const std::string fileName, const int line) {
        struct timeval tv;  
        gettimeofday(&tv,NULL);
        int min = tv.tv_sec / 60 % 60;
        double second = tv.tv_sec % 60;
        second += static_cast<double>(tv.tv_usec) / 1000000;
        std::cout << "[" << fileName << ":" << line << "] In func <" <<
                funcName << ">, time: " << min << ":" << second << std::endl;
    }

private:
    const std::string funcName_, fileName_;
    const int lineCount_;
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point end_;
};

#endif // USE_TIMER

#endif // ROADCATALOG_H
