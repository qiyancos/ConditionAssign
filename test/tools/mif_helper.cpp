#include "mif_helper.h"
#include "program_helper.h"

#include "md5_helper.h"
#include "spatial-base.h"
#include "type_factory.h"
#include "htk/str_helpers.h"

#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <fstream>
#include <thread>

namespace mif_helper {

const std::string MifHeader::COORDSYS_LL = "CoordSys Earth Projection 1, 0";
const std::string MifHeader::COORDSYS_MC = "CoordSys NonEarth Units \"m\" ";

int loadMifHeader(const std::string& layerPath, MifHeader* mifHeader) {
    std::string keyword;
    std::vector<std::string> words;
    int colCnt(-3);
    std::ifstream mifStream(layerPath.c_str());
    CHECK_ARGS(mifStream, "Failed to open mif layer file \"%s\".",
            layerPath.c_str());
    for (std::string line; getline(mifStream, line);) {
        line = htk::trim(line, " ");
        if (line.size() == 0) {
            continue;
        }

        words = htk::split(line, " ");
        keyword = htk::toLower(words[0]);
        if (colCnt < -2) {
            if (keyword == "version") {
                CHECK_ARGS(words.size() == 2,
                        "Version line \"%s\" has not 2 word.", line.c_str());
                mifHeader->version = atoi(words[1].c_str());
            }
            if (keyword == "charset") {
                CHECK_ARGS(words.size() == 2,
                        "Charset line \"%s\" has not 2 word.", line.c_str());
                mifHeader->charset = htk::trim(words[1], "\"");
            }
            if (keyword == "delimiter") {
                CHECK_ARGS(words.size() == 2,
                        "Delimiter line \"%s\" has not 2 word.", line.c_str());
                mifHeader->delimiter = words[1][1]; // "," ;
            }
            if (keyword == "unique") {
                size_t wordCnt = words.size();
                for (size_t i = 1; i < wordCnt; i++) {
                    mifHeader->uniqueVec.push_back(atoi(
                            htk::trim(words[i], ",").c_str()));
                }
            }
            if (keyword == "index") {
                size_t wordCnt = words.size();
                for (size_t i = 1; i < wordCnt; i++) {
                    mifHeader->indexVec.push_back(atoi(
                            htk::trim(words[i], ",").c_str()));
                }
            }
            if (keyword == "coordsys") {
                mifHeader->coordsys = line;
            }
            if (keyword == "projection") {
                mifHeader->coordsys += " " + line;
            }
            if (keyword == "transform") {
                mifHeader->transform = line;
            }
        }

        if (keyword == "columns") {
            CHECK_ARGS(words.size() == 2,
                    "Columns line \"%s\" has not 2 word.", line.c_str());
            mifHeader->colNum = atoi(words[1].c_str());
            colCnt = -2;
        }
        if (colCnt >= -2) {
            ++colCnt;
        }
        if (colCnt >= 0 && colCnt < mifHeader->colNum) {
            CHECK_ARGS(words.size() == 2, "Columns type line \"%s\" %s.",
                    line.c_str(), "has less than 2 words");
            CHECK_ARGS(keyword != "data" && keyword != "none",
                    "Invalid column type conf, need %d but find only %d.",
                    mifHeader->colNum, colCnt);
            mifHeader->colTypeVec.push_back(line.substr(words[0].size() + 1));
            std::string tagName = htk::toLower(words[0]);
            mifHeader->colNameVec.push_back(tagName);
            CHECK_ARGS(mifHeader->colNameMap.find(tagName) ==
                    mifHeader->colNameMap.end(), "Column \"%s\" redefined.",
                    words[0].c_str());
            mifHeader->colNameMap[tagName] = colCnt;
        }
        if (keyword == "data" || keyword == "none") {
            break;
        }
    }
    return 0;
}

int formalizeFileName(std::string* mifFile, std::string* midFile) {
    std::string suffix = mifFile->substr(mifFile->size() - 4, 4);
    if (suffix == ".mif") {
        *midFile = mifFile->substr(0, mifFile->size() - 4) + ".mid";
    } else if (suffix == ".MIF") {
        *midFile = mifFile->substr(0, mifFile->size() - 4) + ".MID";
    } else {
        *midFile = *mifFile + ".mid";
        *mifFile += ".mif";
    }
    return 0;
}

bool checkSameMif(const std::string& file1, const std::string& file2) {
#ifdef STRICT_DIFF
    std::string md5sum1 = md5file(file1);
    std::string md5sum2 = md5file(file2);
#else
    char buffer[1024];
    FILE* file1Ptr = fopen(file1.c_str(), "r");
    std::string keyWord;
    while (1) {
        fgets(buffer, 1024, file1Ptr);
        keyWord = htk::trim(htk::trim(std::string(buffer), " "), "\n");
        if (keyWord == "data" || keyWord == "Data") {
            break;
        }
    }
    FILE* file2Ptr = fopen(file2.c_str(), "r");
    while (1) {
        fgets(buffer, 1024, file2Ptr);
        keyWord = htk::trim(htk::trim(std::string(buffer), " "), "\n");
        if (keyWord == "data" || keyWord == "Data") {
            break;
        }
    }
    std::string md5sum1 = md5file(file1Ptr);
    std::string md5sum2 = md5file(file2Ptr);
#endif
    if (md5sum1 == md5sum2) {
        std::cout << "Mif file match!" << std::endl;
        return true;
    } else {
        std::cout << "Mif File not match [" << md5sum1 << ":" <<
                md5sum2 << "]!" << std::endl;
        return false;
    }
#ifndef STRICT_DIFF
    fclose(file1Ptr);
    fclose(file2Ptr);
#endif
}

bool checkSameMid(const std::string& file1, const std::string& file2) {
    wgt::MIF mif1, mif2;
    std::cout << "Loading mid file..." << std::endl;
    if (wgt::mif_to_wsbl(file1, mif1) < 0) {
        std::cout << "Failed to open file " << file1 << std::endl;
        exit(1);
    }
    if (wgt::mif_to_wsbl(file2, mif2) < 0) {
        std::cout << "Failed to open file " << file2 << std::endl;
        exit(1);
    }
    int mifSize = mif1.mid.size();
    if (mifSize != mif2.mid.size()) {
        std::cout << "Mif size mismatch [" << mifSize << ":" <<
                mif2.mid.size() << "]!" << std::endl;
        return false;
    }
    int colSize = mif1.mid[0].size();
#ifdef STRICT_DIFF
    if (colSize != mif2.mid[0].size()) {
        std::cout << "Column size mismatch [" << colSize << ":" <<
                mif2.mid[0].size() << "]!" << std::endl;
        return false;
    }
#else
    std::cout << "Loss check will not compare number of tags." << std::endl;
#endif
    std::cout << "Start checking mid file." << std::endl;
    program_helper::Progress progressCounter(mifSize);
#ifdef STRICT_DIFF
    for (int mifIndex = 0; mifIndex < mifSize; mifIndex++) {
        for (int colID = 0; colID < colSize; colID++) {
            std::string tagVal1 = htk::trim(mif1.mid[mifIndex][colID], "\"");
            std::string tagVal2 = htk::trim(mif2.mid[mifIndex][colID], "\"");
            if (tagVal1 != tagVal2) {
                std::cout << "Tag mismatch [\"" << tagVal1 << "\" : \"" <<
                        tagVal2 << "\"] in mif item (" << mifIndex + 1 <<
                        ":" << colID + 1 << ")." << std::endl;
                return false;
            }
        }
        progressCounter.addProgress(1);
    }
#else
    const std::vector<std::string> checkTag {"catalog", "name"};
    std::vector<std::pair<int, int>> checkTagIndex;
    for (auto& tag : checkTag) {
        std::pair<int, int> result;
        result.first = mif1.header.col_name_map.find(tag) == 
                mif1.header.col_name_map.end() ? -1 :
                mif1.header.col_name_map[tag];
        result.second = mif2.header.col_name_map.find(tag) == 
                mif2.header.col_name_map.end() ? -1 :
                mif2.header.col_name_map[tag];
        checkTagIndex.push_back(result);
    }
    for (int mifIndex = 0; mifIndex < mifSize; mifIndex++) {
        for (auto& colID : checkTagIndex) {
            if (colID.first + colID.second < 0) {
                if (colID.first + colID.second == -2) {
                    continue;
                } else {
                    std::cout << "Tag not all exist in both mif file." << std::endl;
                    return false;
                }
            }
            std::string tagVal1 = htk::trim(mif1.mid[mifIndex][colID.first], "\"");
            std::string tagVal2 = htk::trim(mif2.mid[mifIndex][colID.second], "\"");
            if (tagVal1 != tagVal2) {
                std::cout << "Tag mismatch [\"" << tagVal1 << "\" : \"" <<
                        tagVal2 << "\"] in mif item (" << mifIndex + 1 <<
                        ", " << colID.first + 1 << ":" << colID.second + 1 <<
                        ")." << std::endl;
                return false;
            }
        }
        progressCounter.addProgress(1);
    }   
#endif
    return true;
}

int SearchEngine::getAllLayers(const std::string& dataPath,
        const std::vector<std::string>& cityNames,
        std::vector<std::string>* allLayers) {
    std::set<std::string> layerSet;
    std::string dirPath;
    for (const std::string& city : cityNames) {
        dirPath = dataPath + "/" + city;
        std::vector<std::string> layerInCity;
        CHECK_RET(program_helper::listDir(dirPath, &layerInCity),
                "Failed to get layers in city \"%s\".", city.c_str());
        for (const std::string& name : layerInCity) {
            if (htk::endswith(name, ".mif") ||
                    htk::endswith(name, ".MIF")) {
                layerSet.insert(name);
            }
        }
    }
    allLayers->insert(allLayers->end(), layerSet.begin(), layerSet.end());
    return 0;
}

int SearchEngine::inputAndParseConfig() {
    while (1) {
        std::string input;
        std::cout << "Please input the condition for searching:\n";
        std::cin >> input;
        if (input.length() > 0) {
            std::vector<std::string> exprs = htk::split(input, ";");
            for (const std::string& expr : exprs) {
                std::vector<std::string> exprParts = htk::split(expr, "=");
                if (exprParts.size() != 2) {
                    std::cerr << "Error: miss operator in expr \"" <<
                            expr << "\"." << std::endl;
                    continue;
                }
                std::string tagName = htk::toLower(htk::trim(htk::trim(
                        exprParts[0], " "), "\""));
                std::string rightVal = htk::trim(exprParts[1], " ");
                configs_.push_back(std::pair<std::string, std::string>(
                        tagName, rightVal));
            }
            break;
        }
    }
    return 0;
}

int SearchEngine::process() {
    inputAndParseConfig();
    std::cout << std::endl;
    for (const std::string& cityName : cityNames_) {
        result_[cityName] = std::map<std::string, std::vector<std::string>>();
        std::map<std::string, std::vector<std::string>>& cityMap =
                result_[cityName];
        for (const std::string& layerName : layerNames_) {
            cityMap[layerName] = std::vector<std::string>();
            jobQueue_.push_back(std::pair<std::string,
                    std::vector<std::string>*>(dataPath_ + "/" + cityName +
                    "/" + layerName, &(cityMap[layerName])));
        }
    }
    int startIndex = 0, endIndex;
    int totalJobCnt = jobQueue_.size();
    progressBar_ = new program_helper::Progress(totalJobCnt);
    int avgJobCnt = totalJobCnt / threadNum_;
    avgJobCnt = avgJobCnt == 0 ? 1 : avgJobCnt;
    std::vector<std::thread> threadList;
    threadStates_.resize(threadNum_);
    std::cout << "-- Start processing... " << std::endl;
    for (int i = 0; i < threadNum_; i++) {
        endIndex = startIndex + avgJobCnt;
        if (i == threadNum_ - 1) {
            endIndex = totalJobCnt;
        }
        threadList.push_back(std::thread(worker, this, startIndex,
                endIndex, i));
        startIndex = endIndex;
        if (endIndex == totalJobCnt) {
            break;
        }
    }
    for (auto& thread : threadList) {
        thread.join();
    }
    delete progressBar_;
    int index = 0;
    for (const int states : threadStates_) {
        CHECK_RET(states,
                "Thread-%d encountered with error while running.", index);
        index++;
    }
    CHECK_RET(processResult(), "Failed to process search result.");
    return 0;
}

int SearchEngine::report() {
    std::cout << "\nFinal Report: " << std::endl;
    std::cout << "================================================\n";
    std::cout << "-- Layer Detail: " << std::endl;
    for (auto iterCity : sumCityLayer_) {
        std::cout << "    " << iterCity.first << ":\n";
        for (auto iterLayer : iterCity.second) {
            if (iterLayer.second == -1) {
                std::cout << "        " << iterLayer.first << "\tNot exist.\n";
            } else {
                std::cout << "        " << iterLayer.first << "\t" <<
                        iterLayer.second << std::endl;
            }
        }
    }
    std::cout << "-- City Summary: " << std::endl;
    for (auto iterCity : sumCity_) {
        std::cout << "    " << iterCity.first << "\t" <<
                iterCity.second << std::endl;
    }
    std::cout << "-- Layer Summary: " << std::endl;
    for (auto iterLayer : sumLayer_) {
        std::cout << "    " << iterLayer.first << "\t" <<
                iterLayer.second << std::endl;
    }
    std::cout << "-- Total: " << totalSum_ << std::endl;
    std::cout << "================================================\n";
    if (printDetail_) {
        std::cout << "-- Detail Report:" << std::endl;
        for (auto iterCity : result_) {
            std::cout << "-- " << iterCity.first << ":\n";
            for (auto iterLayer : iterCity.second) {
                std::cout << "-- " << iterLayer.first << ":\n";
                for (std::string& indexStr : iterLayer.second) {
                    indexStr = htk::trim(htk::trim(indexStr, "\n"), " ");
                }
                program_helper::printWithTypeSetting(iterLayer.second);
                std::cout << std::endl;
            }
        }
        std::cout << "================================================\n";
    }
    return 0;
}

int SearchEngine::processResult() {
    totalSum_ = 0;
    for (auto iterCity : result_) {
        sumCity_[iterCity.first] = 0;
        sumCityLayer_[iterCity.first] = std::map<std::string, int>();
        for(auto iterLayer : iterCity.second) {
            if (sumLayer_.find(iterLayer.first) == sumLayer_.end()) {
                sumLayer_[iterLayer.first] = 0;
            }
            int sumTemp = iterLayer.second.size();
            sumCityLayer_[iterCity.first][iterLayer.first] = sumTemp;
            if (sumTemp == 1 && iterLayer.second[0] == "-1") {
                sumCityLayer_[iterCity.first][iterLayer.first] = -1;
                sumTemp = 0;
            }
            sumLayer_[iterLayer.first] += sumTemp;
            sumCity_[iterCity.first] += sumTemp;
        }
        totalSum_ += sumCity_[iterCity.first];
    }
    return 0;
}

int SearchEngine::worker(SearchEngine* engine, const int startIndex,
        const int endIndex, const int threadID) {
    engine->threadStates_[threadID] = executeCommand(engine, startIndex,
            endIndex);
    return 0;
}

int SearchEngine::executeCommand(SearchEngine* engine, const int startIndex,
        const int endIndex) {
    for (int i = startIndex; i < endIndex; i++) {
        const std::string& layerPath = engine->jobQueue_[i].first;
        std::vector<std::string>* result = engine->jobQueue_[i].second;
        if (access(layerPath.c_str(), R_OK) < 0) {
            result->push_back("-1");
            engine->progressBar_->addProgress(1);
            continue;
        }
        CHECK_ARGS(result->empty(),
                "Result should always be empty before process.");
        // 载入对应mifLayer的Header信息
        std::map<std::string, int> tagMap;
        MifHeader mifHeader;
        CHECK_RET(loadMifHeader(layerPath, &mifHeader),
                "Failed to parse header in mif layer \"%s\".",
                layerPath.c_str());
        // 依据config信息生成awk指令
        std::string finalCommand = std::string("awk -F \"") +
                mifHeader.delimiter + "\" '{if (";
        bool tagNotExist = false;
        for (const auto& config : engine->configs_) {
            if (mifHeader.colNameMap.find(config.first) ==
                    mifHeader.colNameMap.end()) {
                engine->progressBar_->addProgress(1);
                tagNotExist = true;
            } else {
                if (finalCommand[finalCommand.size() - 1] != '(') {
                    finalCommand += " && ";
                }
                finalCommand += std::string("$") +
                        std::to_string(mifHeader.colNameMap[config.first] + 1)
                        + " ~ \"" + config.second + "\" ";
            }
        }
        if (tagNotExist) {
            continue;
        }
        std::string midPath;
        if (htk::endswith(layerPath, ".mif")) {
            midPath = layerPath.substr(0, layerPath.length() - 4) + ".mid";
        } else {
            midPath = layerPath.substr(0, layerPath.length() - 4) + ".MID";
        }
        finalCommand += ") print NR}' " + midPath;
        // 执行指令并收取结果
        CHECK_RET(program_helper::executeCommand(finalCommand, result),
                "Failed to execute command \"%s\".", finalCommand.c_str());
        engine->progressBar_->addProgress(1);
    }
    return 0;
}

} // namespace mif_helper
