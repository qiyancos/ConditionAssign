#include "program_helper.h"

#include "htk/str_helpers.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <set>
#include <cmath>
#include <cstdlib>
#include <readline/readline.h>
#include <readline/history.h>

namespace program_helper {

int listDir(const std::string& dir, std::vector<std::string>* files,
        const bool detail) {
    CHECK_EXIT(access(dir.c_str(), R_OK) < 0,
            "Data path \"%s\" not exist or not readable.", dir.c_str());
    DIR* targetDir = opendir(dir.c_str());
    CHECK_ARGS(targetDir != NULL, "Failed to open dir \"%s\".", dir.c_str());
    struct dirent* targetEntry;
    std::string fileName;
    while ((targetEntry = readdir(targetDir)) != NULL) {
        fileName = std::string(targetEntry->d_name);
        if (detail || fileName[0] != '.') {
            files->push_back(std::string(targetEntry->d_name));
        }
    }
    closedir(targetDir);
    return 0;
}

template<typename T>
bool isType(const std::string& data, T* result) {
    std::stringstream streamTemp(data);
    T typeTemp;
    char charTemp;
    T* typeTempPtr = result == nullptr ? &typeTemp : result;
    if (!(streamTemp >> *typeTempPtr)) {
        return false;
    } else if (streamTemp >> charTemp) {
        return false;
    }
    return true;
}

template bool isType<int>(const std::string&, int*);
template bool isType<double>(const std::string&, double*);
template bool isType<std::string>(const std::string&, std::string*);

int executeCommand(const std::string& cmd,
        std::vector<std::string>* stdoutBuffer,
        std::vector<std::string>* stderrBuffer) {
    if (stdoutBuffer) {
        char buffer[1024];   
        char pipeBuffer[1024] {0};   
        FILE *stdoutFile;   
        strcpy(pipeBuffer, cmd.c_str());   
        CHECK_ARGS((stdoutFile = popen(pipeBuffer, "r")) != NULL,
                "Faile to execute command \"%s\" with stdout pipe opened.",
                cmd.c_str());   
        while(fgets(buffer, 1024, stdoutFile) != NULL) {   
            stdoutBuffer->push_back(std::string(buffer));
        }
        pclose(stdoutFile);   
        return 0;
    }
    if (stderrBuffer) {
        CHECK_ARGS(false,
                "Execute command and return with stderr not implemented.");
    }
    if (stderrBuffer && stdoutBuffer) {
        CHECK_ARGS(false,
                "Execute command and return with stderr not implemented.");
    }
    CHECK_ARGS(system(cmd.c_str()) == 0,
            "Failed to execute command \"%s\".", cmd.c_str());
    return 0;
}

int setType(const int lines, const std::vector<int>& typeLength,
        const int terminalWidth, std::vector<int>* columnLength = nullptr) {
    int colNum = 0, lineNum = 0, maxLength = 0;
    int totalLength = 0, partialCol = 1;
    for (int i = 0; i < typeLength.size(); i++) {
        maxLength = std::max(maxLength, typeLength[i]);
        if (lineNum == lines - 1) {
            if ((totalLength += maxLength) > terminalWidth) {
                // 发现溢出
                return -1;
            }
            // 记录某一列的长度
            if (columnLength) {
                columnLength->push_back(maxLength);
            }
            lineNum = 0;
            colNum++;
            maxLength = 0;
            if (i == typeLength.size() - 1) {
                partialCol = 0;
            }
        } else {
            lineNum++;
        }
    }
    if (partialCol) {
        if ((totalLength += maxLength) > terminalWidth) {
            // 发现溢出
            return -1;
        }
        if (columnLength) {
            columnLength->push_back(maxLength);
        }
    }
    return colNum + partialCol;
}

int binarySearchBestLine(const int startLine, const int endLine,
        const std::vector<int>& typeLength, const int terminalWidth) {
    int midLine = (startLine + endLine) >> 1;
    if (startLine == endLine) {
        return startLine;
    } else if (setType(startLine, typeLength, terminalWidth) != -1) {
        return startLine;
    } else if (setType(midLine, typeLength, terminalWidth) != -1){
        return binarySearchBestLine(startLine + 1, midLine, typeLength,
                terminalWidth);
    } else {
        return binarySearchBestLine(midLine + 1, endLine, typeLength,
                terminalWidth);
    }
}

int getTerminalSize(winsize& terminalSize) {
    if (isatty(STDOUT_FILENO) == 0) {
        std::cerr << "Error: No terminal window found." << std::endl;
        return -1;
    }
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminalSize) < 0) {
        std::cerr << "Error: Ioctl TIOCGWINSZ error." << std::endl;
        return -1;
    }
    return 0;
}

std::vector<std::string>* getTypeSetting(
        const std::vector<std::string>& candidates, const bool withID) {
    struct winsize terminalSize;
    // 获取当前的终端大小
    if (getTerminalSize(terminalSize) < 0) {
        return nullptr;
    }
    std::vector<int> typeLength;
    std::vector<std::string> idString;
    std::string tempString;
    for (int i = 0; i <  candidates.size(); i++) {
        tempString = withID ? std::string("[") + std::to_string(i + 1) +
                "] " : "";
        idString.push_back(tempString);
        typeLength.push_back(candidates[i].length() + tempString.length() + 2);
    }
    std::vector<int> columnLength;
    // 使用二分法来确定最佳的行数
    int bestLineNumber = binarySearchBestLine(1, candidates.size(),
            typeLength, terminalSize.ws_col);
    if (bestLineNumber == -1) {
        std::cerr << "Error: Failed to set typesetting." << std::endl;
        return nullptr;
    }
    // 依据最佳行数获取每列长度
    setType(bestLineNumber, typeLength, terminalSize.ws_col, &columnLength);
    // 生成每行的输出信息
    std::vector<std::string>* retVal = new std::vector<std::string>(
            bestLineNumber);
    int lineNow = 0, columnNow = 0;
    for (int i = 0; i < candidates.size(); i++) {
        (*retVal)[lineNow] += idString[i] + candidates[i] + "  ";
        (*retVal)[lineNow].append(columnLength[columnNow] -
                typeLength[i], ' ');
        if (lineNow == bestLineNumber - 1) {
            lineNow = 0;
            columnNow++;
        } else {
            lineNow++;
        }
    }
    return retVal;
}

void printWithTypeSetting(const std::vector<std::string>& candidates,
        const bool withID) {
    std::vector<std::string>* outputs =
            getTypeSetting(candidates, withID);
    if (outputs) {
        for (const std::string& line : *outputs) {
            std::cout << line << std::endl;
        }
        delete outputs;
        return;
    } else {
        return;
    }
}

Progress::Progress(const int totalCount) : totalCount_(totalCount) {
    for (int i = 0; i < 50; i++) {
        progressBar_[i] = '-';
    }
}

void Progress::addProgress(const int newProgress) {
    std::lock_guard<std::mutex> progressGuard(progressLock_);
    progressCount_ += newProgress;
    percentage_ = static_cast<double>(progressCount_ * 100) / totalCount_;
    if (percentage_ >= ((barCount_ + 1) << 1)) {
        progressBar_[barCount_++] = '#';
        dumpProgress();
    }
}

void Progress::dumpProgress() {
    printf("[%s] %d/%d %.2f%%", progressBar_, progressCount_, totalCount_,
            percentage_);
    if (progressCount_ == totalCount_) {
        printf("\n");
    } else {
        printf("\r");
    }
}

SimpleSearchBar* SimpleSearchBar::createSearchBar() {
    std::lock_guard<std::mutex> initGuard(initLock_);
    if (!onlyBar_) {
        onlyBar_ = new SimpleSearchBar();
        return onlyBar_;
    }
    // 如果已经实例化，那么必须等待其他实例化删除才可以create
    return nullptr;
}

SimpleSearchBar* SimpleSearchBar::createSharedSearchBar() {
    // 调用该函数要求使用者不会随意修改属性或者库
    std::lock_guard<std::mutex> initGuard(initLock_);
    if (!onlyBar_) {
        onlyBar_ = new SimpleSearchBar();
    }
    return onlyBar_;
}

int SimpleSearchBar::deleteSearchBar() {
    std::lock_guard<std::mutex> initGuard(initLock_);
    if (onlyBar_) {
        delete onlyBar_;
        return 1;
    } else {
        return 0;
    }
}

int SimpleSearchBar::init(const bool useAll, const bool useNumber,
        const bool useName) {
    CHECK_WARN(useAll || useNumber || useName, "Search Bar will not %s",
            "work well as no search method is given.");
    useAll_ = useAll;
    useNumber_ = useNumber;
    useName_ = useName;
    rl_attempted_completion_over = 0;
    rl_attempted_completion_function = autoComplete;
    return 0;
}

int SimpleSearchBar::setSearchLib(
        const std::vector<std::string>& newSearchLib) {
    searchLib_.clear();
    int index = 0;
    for (const std::string& name :  newSearchLib) {
        searchMap_[name] = index++;
    }
    searchLib_.insert(searchLib_.end(), newSearchLib.begin(),
            newSearchLib.end());
    size_ = newSearchLib.size();
    // 添加指令的自动补全
    searchLib_.push_back(helpInst_);
    searchLib_.push_back(showLibInst_);
    searchLib_.push_back(openNumberInst_);
    searchLib_.push_back(closeNumberInst_);
    if (useAll_) {
        searchLib_.push_back(selectAllInst_);
    }
    return 0;
}

int SimpleSearchBar::search(std::vector<int>* result) {
    CHECK_ARGS(!searchLib_.empty(),
            "Searching in an empty lib is meaningless.");
    char* buffer;
    std::string inputOption;
    std::cout << "You may input \"#Help\" to check help infomation.\n";
    while (1) {
        buffer = readline("Please Enter Your Selection: ");
        CHECK_ARGS(buffer, "Failed to read content for input.");
        inputOption = htk::trim(buffer, " ");
        free(buffer);
        buffer = NULL;
        if (findResult(inputOption, result) > -1) {
            break;
        }
    }
    return 0;
}

void SimpleSearchBar::printHelpInfo() {
    std::cout << "\nYou may use format like \"2-5 8 9\"(Need to open" <<
            " number selection) or \"Name1 Name2\"(Use TAB to complete " <<
            "unfinished name) or \"#All\" to select all the options.\n";
    std::cout << "You can also combine number index with name. But we " <<
            "will take number as option name if there is conflict " <<
            "between option name and number index.\n\n";
    std::cout << "Supported Command:" << std::endl;
    std::cout << "\t" << helpInst_ << ": Print help information.\n";
    std::cout << "\t" << showLibInst_ << ": Print all the options.\n";
    std::cout << "\t" << closeNumberInst_ << ": Disable number index.\n";
    std::cout << "\t" << openNumberInst_ << ": Enable number index.\n\n";
}

int SimpleSearchBar::findResult(const std::string& input, std::vector<int>* results) {
    std::set<int> resultIndex;
    add_history(input.c_str());
    std::vector<std::string> options = htk::split(input, " ", "\t");
    // 特殊指令的解析
    if (options.size() == 1) {
        if (options[0] == helpInst_) {
            printHelpInfo();
            return -1;
        } else if (options[0] == showLibInst_) {
            printWithTypeSetting(searchLib_, true);
            return -1;
        } else if (options[0] == closeNumberInst_) {
            std::cerr << "Number Index has been disabled." << std::endl;
            useNumber_ = false;
            return -1;
        } else if (options[0] == openNumberInst_) {
            std::cerr << "Number Index has been enabled." << std::endl;
            useNumber_ = true;
            return -1;
        }
    }
    // 正常解析
    int numberTemp = 0, endIndex = 0;
    size_t barIndex = 0;
    for (const std::string& option : options) {
        // 查找名字
        if (searchMap_.find(option) != searchMap_.end()) {
            resultIndex.insert(searchMap_[option]);
            continue;
        }
        // 全选解析
        if (useAll_ && option == selectAllInst_) {
            results->clear();
            for (int i = 0; i < size_; i++) {
                results->push_back(i);
            }
            return 0;
        }
        // 编号索引解析
        if (useNumber_) {
            barIndex = option.find("-");
            if (barIndex != std::string::npos) {
                if (isType(option.substr(0, barIndex), &numberTemp) &&
                        isType(option.substr(barIndex + 1, option.size() -
                        barIndex - 1), &endIndex)) {
                    if (numberTemp > 0 && numberTemp <= size_ &&
                            endIndex > 0 && endIndex <= size_) {
                        for (int i = numberTemp - 1; i < endIndex; i++) {
                            resultIndex.insert(i);
                        }
                        continue;
                    }
                }
            } else if (isType(option, &numberTemp)) {
                if (numberTemp > 0 && numberTemp <= size_) {
                    resultIndex.insert(numberTemp - 1);
                    continue;
                }
            }
        }
        CHECK_RET(-1, "Can not parse input \"%s\".", option.c_str());
    }
    results->insert(results->end(), resultIndex.begin(),
            resultIndex.end());
    return 0;
}

char* SimpleSearchBar::findOption(const char* text, int state) {
    static int libIndex;
    if (!state) {
        libIndex = 0;
    }
    const std::vector<std::string>& searchLib = onlyBar_->searchLib_;
    // TODO: 后期需要使用字典树优化
    for (int i = libIndex; i < searchLib.size(); i++) {
        if (searchLib[i].find(text) != std::string::npos) {
            libIndex = i + 1;
            return strdup(searchLib[i].c_str());
        }
    }
    return NULL;
}

char** SimpleSearchBar::autoComplete(const char* text, int start, int end) {
    char** matchList = NULL;
    if (start != end) {
        matchList = rl_completion_matches(text, findOption);
    }
    return matchList;
}

// 初始化静态成员变量
std::mutex SimpleSearchBar::initLock_;
SimpleSearchBar* SimpleSearchBar::onlyBar_;
bool SimpleSearchBar::useAll_;
bool SimpleSearchBar::useNumber_;
bool SimpleSearchBar::useName_;
const std::string SimpleSearchBar::helpInst_ = "#Help";
const std::string SimpleSearchBar::selectAllInst_ = "#All";
const std::string SimpleSearchBar::showLibInst_ = "#Show";
const std::string SimpleSearchBar::closeNumberInst_ = "#NoNumber";
const std::string SimpleSearchBar::openNumberInst_ = "#UseNumber";

int manualSelect(const std::vector<std::string>& srcOptions,
        std::vector<std::string>* selectResult) {
    printWithTypeSetting(srcOptions, true);
    SimpleSearchBar* newSearchBar = SimpleSearchBar::createSearchBar();
    CHECK_RET(newSearchBar->init(true, true, true),
            "Failed to init search bar.");
    CHECK_RET(newSearchBar->setSearchLib(srcOptions),
            "Failed to set serach lib.");
    std::vector<int> result;
    CHECK_RET(newSearchBar->search(&result),
            "Failed to search with SimpleSearchBar");
    for (int id : result) {
        selectResult->push_back(srcOptions[id]);
    }
    SimpleSearchBar::deleteSearchBar();
    return 0;
}

} //namespace program_helper
