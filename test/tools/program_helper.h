#ifndef PROGRAM_HELPER_H
#define PRPGRAM_HELPER_H

#include <htk/str_helpers.h>

#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <mutex>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <cmath>

#define CHECK_EXIT(expr, info, ...) { \
    int errCode = expr; \
    if (errCode < 0) { \
        fprintf(stderr, (std::string("[ERROR] ") + __FILE__ + "[%d]: " + \
                info + " In [%s]." + " Error code = %d\n").c_str(), \
                __LINE__, ##__VA_ARGS__, __func__, errCode); \
        exit(errCode); \
    } \
}

#define CHECK_RET(expr, info, ...) { \
    int retCode = expr; \
    if (retCode < 0) { \
        fprintf(stderr, (std::string("[ERROR] ") + __FILE__ + "[%d]: " + \
                info + " In [%s]." + " Return %d\n").c_str(), \
                __LINE__, ##__VA_ARGS__, __func__, retCode); \
        return retCode; \
    } \
}

#define CHECK_WARN(expr, info, ...) { \
    if (!(expr)) { \
        fprintf(stderr, (std::string("[WARN] ") + __FILE__ + "[%d]: " + \
            info + " In [%s].\n").c_str(), __LINE__, \
            ##__VA_ARGS__, __func__); \
    } \
}

#define CHECK_ARGS(expr, info, ...) { \
    if (!(expr)) { \
        fprintf(stderr, (std::string("[ERROR] ") + __FILE__ + "[%d]: " + \
                info + " In [%s].\n").c_str(), __LINE__, \
                ##__VA_ARGS__, __func__); \
        return -1; \
    } \
}

namespace program_helper {

// 基于终端大小的自动排版打印函数
void printWithTypeSetting(const std::vector<std::string>& candidates,
        const bool withID = false);

template<typename T>
bool isType(const std::string& data, T* result);

// 多线程支持的自适应进度条显示类
class Progress {
public:
    // 构造函数
    Progress(const int totalCount);
    // 添加新的进度数值
    void addProgress(const int newProgress);

private:
    // 打印信息
    void dumpProgress();

    // 支持多线程进度条
    std::mutex progressLock_;
    // 当前进度的百分比
    double percentage_ = 0;
    // 当前要写的barCount
    int barCount_ = 0;
    // 进度条
    char progressBar_[51] {0};
    // 当前进度的实际数值
    int progressCount_ = 0;
    // 总进度数值
    const int totalCount_;
};


// 帶自动补全的搜索输入栏（唯一实例，多线程安全）
class SimpleSearchBar {
public:
    // 唯一创建函数
    static SimpleSearchBar* createSearchBar();
    // 唯一共享创建函数
    static SimpleSearchBar* createSharedSearchBar();
    // 唯一的删除函数
    static int deleteSearchBar();
    
    // 初始化函数
    int init(const bool useAll = true, const bool useNumber = false,
            const bool useName = true);
    // 设置字符串搜索库
    int setSearchLib(const std::vector<std::string>& newSearchLib);
    // 开启搜索栏，最后会返回匹配的索引
    int search(std::vector<int>* result);

private:
    // 打印帮助信息
    void printHelpInfo();
    // 依据输入的选项进行筛选
    int findResult(const std::string& input, std::vector<int>* results);
    
    // 获取一个候选项的函数
    static char* findOption(const char* text, int state);
    // 自动补全函数
    static char** autoComplete(const char* text, int start, int end);

private:
    // 多线程的构造锁
    static std::mutex initLock_;
    // 唯一的实例化指针
    static SimpleSearchBar* onlyBar_;
    // 是否激活ALL关键字
    static bool useAll_;
    // 是否使用序号查询（支持查询但是并不自动补全）
    static bool useNumber_;
    // 是否使用一般的名称补全
    static bool useName_;
    // 当前实际备选项的个数
    int size_;
    // 用于在最后查找选项的索引
    std::map<std::string, int> searchMap_;
    // 进行快速查找的一维向量
    std::vector<std::string> searchLib_;
    
    // 特殊指令的编码
    // 帮助选项
    static const std::string helpInst_;
    // 选择所有
    static const std::string selectAllInst_;
    // 显示选择的库
    static const std::string showLibInst_;
    // 关闭编号选择
    static const std::string closeNumberInst_;
    // 开启编号选择
    static const std::string openNumberInst_;
};

// 获取当前文件夹下的所有文件/文件夹名
int listDir(const std::string& dir, std::vector<std::string>* files,
        const bool detail = false);

// 基于之搜索栏的选择函数
int manualSelect(const std::vector<std::string>& srcOptions,
        std::vector<std::string>* selectResult);

} //namespace program_helper

#endif // PROGRAM_HELPER_H
