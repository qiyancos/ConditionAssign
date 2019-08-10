#include "ExecutorJobFunc.h"
#include "ConditionAssign.h"
#include "Group.h"

namespace condition_assign {

namespace job_func {

int LoadLayerJob::process(const int executorID) {
    TEST(executorID);
    MifLayer* layer;
    CHECK_RET(resourcePool_->getLayerBySharedID(&layer, sharedID_),
            "Failed to get layer of shared id[%d].", sharedID_);
    CHECK_RET(layer->open(),
            "Failed to open layer with sahred id[%s].", sharedID_);
    return 0;
}

int SaveLayerJob::process(const int executorID) {
    TEST(executorID);
    MifLayer* layer;
    CHECK_RET(paramPtr->resourcePool->getLayerBySharedID(&layer, sharedID_),
            "Failed to get output layer of shared id[%d].", sharedID_);
    CHECK_RET(layer->save(savePath_),
            "Failed to save output layer[%d] to path \"%s\".",
            sahredID_, savePath_.c_str());
    return 0;
}

int ParseConfigFileJob::process(const int executorID) {
    TEST(executorID);
    // 设置Config子组
    ConfigSubGroup* subGroup;
    CHECK_RET(resourcePool_->getConfigSubGroup(configIndex_, &subGroup),
            "Failed to get config sub group for layer[%d]", configIndex_);
    // 打开并逐行读取配置文件
    subGroup->filePath_ = &(filePath_);
    CHECK_RET(resourcePool_->getOutputFullPath(subGroup->id_,
            &(subGroup->savePath_)), "Failed to get target layer %s \"%s\".",
            "path of config file", filePath_.c_str());
    std::ifstream configFileStream(filePath.c_str());
    CHECK_ARGS(configFileStream, "Failed to open config file \"%s\".",
            filePath.c_str());
    std::string content;
    std::vector<std::pair<std::string, int>>* fullContent =
            new std::vector<std::pair<std::string,int>>();
    int lineNumber = 1;
    while (getline(configFileStream, content)) {
        if (content.length() > 0 && htk::trim(content, " ")[0] != '#') {
            fullContent->push_back(std::pair<std::string, int>(
                    content, lineNumber));
            subGroup->group_->push_back(std::pair<int, ConfigItem*>(lineNumber,
                    nullptr));
        }
        lineNumber++;
    }
    // 任务划分
    int startIndex = 0;
    int lineCount = MAX_LINE_PER_JOB;
    int totalCount = fullContent->size() ;
    int edgeCount = totalCount / lineCount * lineCount;
    if (totalCount - edgeCount < (lineCount >> 1)) {
        edgeCount -= lineCount;
        edgeCount = edgeCount < 0 ? 0 : edgeCount;
    }
    std::vector<ExecutorJob*> newJobs;
    while (startIndex < edgeCount) {
        newJobs.push_back(new ParseConfigLinesJob(filePath_, fullContent,
                startIndex, lineCount, subGroup, resourcePool_));
        startIndex += lineCount;
    }
    newJobs.push_back(new ParseConfigLinesJob(filePath_, fullContent,
            edgeCount, totalCount - edgeCount, subGroup, resourcePool_));
    MifLayer* targetLayer;
    CHECK_RET(resourcePool_->getLayerByIndex(&targetLayer,
            ResourcePool::Output, configIndex_), "Failed to get layer %s[%d].",
            "bind with this config file", configIndex_);
    // 在串行模式下需要执行copyLoad
    if (!ExecutorPool::runParallel_) {
        targetLayer->copyLoad();
    }
    MifLayer* srcLayer;
    CHECK_RET(resourcePool_->getLayerByIndex(&srcLayer, ResourcePool::Input,
            configIndex_), "Failed to get intput layer %s[%d].",
            "bind with this config file", configIndex_);
    srcLayer->ready_.wait();
    targetLayer->ready_.wait();
    std::lock_guard<std::mutex> lockGuard(resourcePool_->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        resourcePool_->candidateQueue_.push(job);
    }
    resourcePool_->newCandidateJob_->signalAll();
    return 0;
}

int ParseConfigLinesJob::process(const int executorID) {
    TEST(executorID);
    int index = startIndex_;
    int lineCount = lineCount;
    std::vector<std::pair<std::string, Group**>*> newGroups;
    while (lineCount--) {
        std::pair<std::string, int>& thisLine =
                (*fullContent_)[index];
        CHECK_RET(parser::parseConfigLine(thisLine.first,
                subGroup_, index, resourcePool_, &newGroups),
                "Failed to parse single line \"%s\"%s\"%s\" [line: %d]",
                thisLine.first.c_str(), " in config file ",
                filePath.c_str(), thisLine.second);
        index++;
    }
    std::vector<ExecutorJob*> newJobs;
    for (std::pair<std::string, Group**>* groupInfo : newGroups) {
        // 新建buildGroup的相关工作项
        newJobs.push_back(new ParseGroupJob(groupInfo, resourcePool_));
    }
    std::vector<std::pair<int, int>> rangesInJob;
    resourcePool_->jobCacheLock_.lock();
    resourcePool_->parseGroupJobCount_ += newJobs.size();
    // 当前配置文件的所有内容均已经解析完毕
    if (subGroup_->readyCount_ + lineCount_ == fullContent_->size()) {
        MifLayer* srcLayer;
        CHECK_RET(resourcePool_->getLayerBySharedID(&srcLayer,
                subGroup_->srcLayerID_), "Failed to get %s[%d].",
                "input mif layer with shared id", subGroup_->srcLayerID_);
        MifLayer* targetLayer;
        CHECK_RET(resourcePool_->getLayerBySharedID(&targetLayer,
                subGroup_->targetLayerID_), "Failed to get %s[%d].",
                "output mif layer with shared id", subGroup->targetLayerID_);
        int startIndex = 0;
        int itemCount = MAX_ITEM_PER_JOB;
        int totalCount = srcLayer->size() ;
        int edgeCount = totalCount / itemCount * itemCount;
        if (totalCount - edgeCount < (itemCount >> 1)) {
            edgeCount -= itemCount;
            edgeCount = edgeCount < 0 ? 0 : edgeCount;
        }
        while (startIndex < edgeCount) {
            resourcePool_->jobCache_[subGroup_->id_].push_back(
                    ProcessMifItemsJob(srcLayer, targetLayer,
                    subGroup_, startIndex, itemCount));
            startIndex += itemCount;
        }
        resourcePool_->jobCache_[subGroup_->id_].push_back(
                ProcessMifItemsJob(srcLayer, targetLayer,
                subGroup_, edgeCount, totalCount - edgeCount));
        if (resourcePool_->parseGroupJobCount_ == 0) {
            for (int i = 0; i < resourcePool_->jobCache_.size(); i++) {
                newJobs.insert(newJobs.end(), resourcePool_->jobCache_.begin(),
                        resourcePool_->jobCache_.end());
                resourcePool_->jobCache_.clear();
            }
        }
    }
    subGroup_->readyCount_ += lineCount_;
    resourcePool_->jobCacheLock_.unlock();
    std::lock_guard<std::mutex> candidateQueueGuard(
            resourcePool_->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        resourcePool_->candidateQueue_.push(job);
    }
    resourcePool_->newCandidateJob_->signalAll();
    return 0;
}

int ParseGroupJob::process(const int executorID) {
    TEST(executorID);
    // 第一个整数为-1表示当前Group是否已经存在并注册
    using GroupPair = std::pair<int, Group*>;
    GroupPair itemGroup(-1, nullptr), typeGroup(-1, nullptr);
    CHECK_RET(parser::parseGroupInfo(groupInfo_->first,
            resourcePool_, &itemGroup, &typeGroup),
            "Failed to parse group \"%s\".", groupInfo_->first);
    // 判断解析group的结果
    if (typeGroup.second == nullptr) {
        *(groupInfo_->second) = itemGroup.second;
        delete groupInfo_;
        std::lock_guard<std::mutex> jobCacheGuard(
                resourcePool_->jobCacheLock_);
        resourcePool_->parseGroupJobCount_--;
        return 0;
    } else {
        *(groupInfo_->second) = typeGroup.second;
        delete groupInfo_;
        // 分发任务
        std::vector<ExecutorJob*> newJobs;
        if (itemGroup.first > -1) {
            CHECK_ARGS(typeGroup.first > 0,
                    "Type group found but item group not found.");
            MifLayer* pluginLayer;
            CHECK_RET(resourcePool->getLayerByName(&pluginLayer,
                    ResourcePool::Plugin, itemGroup.second->info_->layerName_),
                    "Failed to found plugin layer \"%s\"",
                    itemGroup.second->info_->layerName_.c_str());

            int itemCount = MAX_ITEM_PER_JOB;
            int startIndex = 0;
            int totalCount = pluginLayer->size();
            int edgeCount = totalCount / itemCount * itemCount;
            if (totalCount - edgeCount <= (itemCount >> 1)) {
                edgeCount -= itemCount;
            }
            while (startIndex < edgeCount) {
                newJobs.push_back(new BuildGroupJob(pluginLayer,
                        itemGroup.second, typeGroup.second, startIndex,
                        itemCount, resourcePool_));
                startIndex += itemCount;
            }
            newJobs.push_back(new BuildGroup(pluginLayer, itemGroup.second,
                typeGroup.second, edgeCount, totalCount -edgeCount,
                resourcePool_));
            newJobs.push_back(new BuildGroupJob(pluginLayer, itemGroup.second,
                    typeGroup.second, -1, 0, resourcePool_));
        } else if (typeGroup.first > -1) {
            newJobs.push_back(new BuildGroup(nullptr, itemGroup.second,
                    typeGroup.second, -1, 0, resourcePool_));
        }
        std::lock_guard<std::mutex> candidateGuard(
                resourcePool_->candidateQueueLock_);
        for (ExecutorJob* job : newJobs) {
            resourcePool_->candidateQueue_.push(job);
        }
        resourcePool_->newCandidateJob_->signalAll();
    }
    std::lock_guard<std::mutex> jobCacheGuard(
            resourcePool_->jobCacheLock_);
    resourcePool_->parseGroupJobCount_--;
    return 0;
}

int BuildGroupJob::process(const int executorID) {
    TEST(executorID);
    if (startIndex_ == -1) {
        CHECK_RET(typeGroup_->init(*(itemGroup_), typeGroup_->info_->tagName_),
                "Failed to init type group from item group.");
        CHECK_ARGS(!typeGroup_->info_,
                "Type group should not have group info while building.");
        typeGroup_->ready_.signalAll();
    } else {
        Group::GroupInfo* groupInfo = itemGroup_->info_;
        CHECK_ARGS(groupInfo, "Group info should be available for building.");
        int index = startIndex_;
        int result = 0, totalCount = itemCount_;
        MifItem* workingItem;
        while (totalCount--) {
#ifdef USE_MIFITEM_CACHE
            CHECK_RET(pluginLayer_->newMifItem(index, &workingItem, nullptr),
                    "Failed to create new mif item while building group.");
#else
            workingItem = new MifItem(index, pluginLayer_, nullptr);
#endif
            result = satisfyConditions(*(groupInfo->configItem_),
                    workingItem);
            CHECK_RET(result, "Failed to check conditions in mif item.");
            if (result) {
                itemGroup_->addElement(index);
            }
#ifndef USE_MIFITEM_CACHE
            delete workingItem;
#endif
            index++;
        }
        groupInfo->checkedCount_ += totalCount;
        if (groupInfo->checkedCount_ == paramPtr->pluginLayer->size()) {
            delete groupInfo;
            paramPtr->itemGroup->info_ = nullptr;
            paramPtr->itemGroup->ready_.signalAll();
        }
    }
    std::lock_guard<std::mutex> jobCacheGuard(resourcePool_->jobCacheLock_);
    if (resourcePool_->parseGroupJobCount_ == 0) {
        std::vector<ExecutorJob*> newJobs;
        for (int i = 0; i < resourcePool_->jobCache_.size(); i++) {
            newJobs.insert(newJobs.end(), resourcePool_->jobCache_.begin(),
                    resourcePool_->jobCache_.end());
            resourcePool_->jobCache_.clear();
        }
        if (!newJobs.empty()) {
            std::lock_guard<std::mutex> candidateGuard(
                    resourcePool_->candidateQueueLock_);
            for (ExecutorJob* job : newJobs) {
                resourcePool_->candidateQueue_.push(job);
            }
            resourcePool_->newCandidateJob_->signalAll();
        }
    }
    return 0;
}

int ProcessMifItemsJob::process(const int executorID) {
    // TIMER();
    TEST(executorID);
    MifItem* workingItem;
    int itemIndex = startIndex_;
    int itemCount = itemCount_;
    std::vector<std::pair<int, ConfigItem*>>& configItemGroup =
            subGroup_->group_;
    const int totalConfigCount = configItemGroup.size();
    while (itemCount--) {
#ifdef DEBUG_OP
        std::cout << ">>Process Mif Item: " << itemIndex << std::endl;
#endif
#ifdef USE_MIFITEM_CACHE
        if (srcLayer_->withItemCache()) {
            CHECK_RET(srcLayer_->newMifItem(itemIndex++, &workingItem,
                    targetLayer_), "Failed to create %s",
                    "new mif item while processing mif item.");
        } else {
            workingItem = new MifItem(itemIndex++, srcLayer_, targetLayer_);
        }
#else
        workingItem = new MifItem(itemIndex++, srcLayer_, targetLayer_);
#endif
        for (int configIndex = 0; configIndex < totalConfigCount;
                configIndex++) {
            int result = 0;
#ifdef DEBUG_OP
            std::cout << "--Process Config Item: " << configIndex << std::endl;
#endif
            ConfigItem* configItem = configItemGroup[configIndex].second;
            if (!configItem) continue;
            result = satisfyConditions(*configItem, workingItem);
            CHECK_RET(result, "Failed to %s \"%s\"[line: %d].",
                    "check conditions from config file",
                    subGroup_->filePath_->c_str(),
                    configItemGroup[configIndex].first);
            if (result) {
#ifdef DEBUG_OP
                std::cout << "++Match!" << std::endl;
#endif
                CHECK_RET(applyAssigns(*configItem, workingItem),
                        "Failed apply assign expr to mif item.");
                break;
            }
        }
#ifdef USE_MIFITEM_CACHE
        if (!srcLayer_->withItemCache()) {
            delete workingItem;
        }
#else
        delete workingItem;
#endif
        if (subGroup_->readyCount_ + itemCount_ == srcLayer_->size()) {
            std::vector<ExecutorJob*> newJobs;
            if (!ExecutorPool::runParallel_ && 
                    !(resourcePool_->parseConfgFileJobs_.empty()) {
                newJobs.push_back(resourcePool->parseConfigFileJobs_.front());
                resourcePool->parseConfigFileJobs_.pop();
            }
            if (*(subGroup_->finishedFileCount_) + 1 >= subGroup_->savePoint_) {
                newJobs.push_back(new SaveLayerJob(subGroup_->targetLayerID_,
                        subGroup_->savePath_, resourcePool_));
            }
            std::lock_guard<std::mutex> candidateQueueGuard(
                    resourcePool_->candidateQueueLock_);
            if (!newJobs.empty()) {
                for (ExecutorJob* job : newJobs) {
                    resourcePool_->candidateQueue_.push(job);
                }
                resourcePool_->newCandidateJob_->signalAll();
            }
            *(subGroup_->finishedFileCount_)++;
        }
        subGroup_->readyCount_ += itemCount_;
    }
    return 0;
}

} // namespace job_func

} // namespace condition_assign
