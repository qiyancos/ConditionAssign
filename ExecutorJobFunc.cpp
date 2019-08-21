#include "ExecutorJobFunc.h"
#include "ConditionAssign.h"
#include "Group.h"

namespace condition_assign {

namespace job {

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
    CHECK_RET(resourcePool_->getLayerBySharedID(&layer, sharedID_),
            "Failed to get output layer of shared id[%d].", sharedID_);
    CHECK_RET(layer->save(savePath_),
            "Failed to save output layer[%d] to path \"%s\".",
            sharedID_, savePath_.c_str());
    return 0;
}

int ParseConfigFileJob::process(const int executorID) {
    TEST(executorID);
    // 设置Config子组
    std::vector<MifLayer*>* targetLayers = new std::vector<MifLayer*>();
    std::vector<MifLayer*>* srcLayers = new std::vector<MifLayer*>();
    std::vector<ConfigSubGroup*>* subGroups =
            new std::vector<ConfigSubGroup*>();
    MifLayer* layer;
    ConfigSubGroup* subGroup;
    for (int configIndex : configIndexes_) {
        // 初始化subGroup的路径信息
        CHECK_RET(resourcePool_->getConfigSubGroup(configIndex, &subGroup),
                "Failed to get config sub group for layer[%d]", configIndex);
        subGroup->filePath_ = &(filePath_);
        CHECK_RET(resourcePool_->getOutputFullPath(subGroup->id_,
                &(subGroup->savePath_)), "Failed to get target %s \"%s\".",
                "layer path of config file", filePath_.c_str());
        subGroups->push_back(subGroup);
        // 获取对应的inputlayer和outputLayer
        CHECK_RET(resourcePool_->getLayerBySharedID(&layer,
                subGroup->srcLayerID_), "Failed to get %s[%d].",
                "input layer bind with this config file", configIndex);
        srcLayers->push_back(layer);
        CHECK_RET(resourcePool_->getLayerBySharedID(&layer,
                subGroup->targetLayerID_), "Failed to get %s[%d].",
                "output layer bind with this config file", configIndex);
        targetLayers->push_back(layer);
    }
    TEST(executorID);
    // 打开并逐行读取配置文件
    std::ifstream configFileStream(filePath_.c_str());
    CHECK_ARGS(configFileStream, "Failed to open config file \"%s\".",
            filePath_.c_str());
    std::string content;
    std::vector<std::pair<std::string, int>>* fullContent =
            new std::vector<std::pair<std::string,int>>();
    int lineNumber = 1;
    while (getline(configFileStream, content)) {
        if (content.length() > 0 && htk::trim(htk::trim(content, " "), "\t")[0]
                != '#') {
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
                startIndex, lineCount, subGroups, srcLayers,
                targetLayers, resourcePool_));
        startIndex += lineCount;
    }
    newJobs.push_back(new ParseConfigLinesJob(filePath_, fullContent,
            edgeCount, totalCount - edgeCount, subGroups, srcLayers,
            targetLayers, resourcePool_));
    // 在串行模式下需要执行copyLoad
    if (!ExecutorPool::runParallel_ && (*targetLayers)[0]->isNew()) {
        (*targetLayers)[0]->copyLoad();
    }
    for (MifLayer* srcLayer : *targetLayers) {
        srcLayer->ready_.wait();
    }
    for (MifLayer* targetLayer : *targetLayers) {
        targetLayer->ready_.wait();
    }
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
    int lineCount = lineCount_;
    ConfigSubGroup* subGroup = (*subGroups_)[0];
    std::vector<std::pair<std::string, Group**>*> newGroups;
    while (lineCount--) {
        std::pair<std::string, int>& thisLine =
                (*fullContent_)[index];
        CHECK_RET(parser::parseConfigLine(thisLine.first, subGroup,
                srcLayers_, targetLayers_, index, resourcePool_, &newGroups),
                "Failed to parse single line \"%s\"%s\"%s\" [line: %d]",
                thisLine.first.c_str(), " in config file ",
                filePath_.c_str(), thisLine.second);
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
    if (subGroup->readyCount_ + lineCount_ == fullContent_->size()) {
        MifLayer *srcLayer, *targetLayer;
        ConfigSubGroup* subGroupNow;
        for (index = 0; index < subGroups_->size(); index++) {
            srcLayer = (*srcLayers_)[index];
            targetLayer = (*targetLayers_)[index];
            subGroupNow = (*subGroups_)[index];
            int startIndex = 0;
            int itemCount = MAX_ITEM_PER_JOB;
            int totalCount = srcLayer->size() ;
            int edgeCount = totalCount / itemCount * itemCount;
            if (totalCount - edgeCount < (itemCount >> 1)) {
                edgeCount -= itemCount;
                edgeCount = edgeCount < 0 ? 0 : edgeCount;
            }
            while (startIndex < edgeCount) {
                resourcePool_->jobCache_[subGroupNow->id_].push_back(
                        new ProcessMifItemsJob(srcLayer, targetLayer,
                        subGroupNow, startIndex, itemCount, resourcePool_));
                startIndex += itemCount;
            }
            resourcePool_->jobCache_[subGroupNow->id_].push_back(
                    new ProcessMifItemsJob(srcLayer, targetLayer,
                    subGroupNow, edgeCount, totalCount - edgeCount,
                    resourcePool_));
        }
        // 判断是否可以转移ProcessMifItemJob
        if (resourcePool_->parseGroupJobCount_ == 0) {
            for (int i = 0; i < resourcePool_->jobCache_.size(); i++) {
                newJobs.insert(newJobs.end(),
                        resourcePool_->jobCache_[i].begin(),
                        resourcePool_->jobCache_[i].end());
                resourcePool_->jobCache_[i].clear();
            }
        }
        delete targetLayers_;
        delete srcLayers_;
        delete subGroups_;
    }
    subGroup->readyCount_ += lineCount_;
    resourcePool_->jobCacheLock_.unlock();
    if (!newJobs.empty()) {
        std::lock_guard<std::mutex> candidateQueueGuard(
                resourcePool_->candidateQueueLock_);
        for (ExecutorJob* job : newJobs) {
            resourcePool_->candidateQueue_.push(job);
        }
        resourcePool_->newCandidateJob_->signalAll();
    }
    return 0;
}

int ParseGroupJob::process(const int executorID) {
    TEST(executorID);
    // 第一个整数为-1表示当前Group是否已经存在并注册
    using GroupPair = std::pair<int64_t, Group*>;
    GroupPair itemGroup(-1, nullptr), typeGroup(-1, nullptr);
    CHECK_RET(parser::parseGroupInfo(groupInfo_->first,
            resourcePool_, &itemGroup, &typeGroup),
            "Failed to parse group \"%s\".", groupInfo_->first.c_str());
    // 判断解析group的结果是否为Dynamic Group
    if (typeGroup.second == nullptr) {
        *(groupInfo_->second) = itemGroup.second;
        delete groupInfo_;
        std::lock_guard<std::mutex> jobCacheGuard(
                resourcePool_->jobCacheLock_);
        if (--resourcePool_->parseGroupJobCount_ == 0) {
            std::vector<ExecutorJob*> newJobs;
            for (int i = 0; i < resourcePool_->jobCache_.size(); i++) {
                newJobs.insert(newJobs.end(),
                        resourcePool_->jobCache_[i].begin(),
                        resourcePool_->jobCache_[i].end());
                resourcePool_->jobCache_[i].clear();
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
    } else {
        *(groupInfo_->second) = typeGroup.second;
        delete groupInfo_;
        // 分发任务
        std::vector<ExecutorJob*> newJobs;
        if (itemGroup.first > -1) {
            CHECK_ARGS(typeGroup.first > 0,
                    "Type group found but item group not found.");
            MifLayer* pluginLayer;
            CHECK_RET(resourcePool_->getLayerByName(&pluginLayer,
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
            newJobs.push_back(new BuildGroupJob(pluginLayer, itemGroup.second,
                typeGroup.second, edgeCount, totalCount -edgeCount,
                resourcePool_));
            newJobs.push_back(new BuildGroupJob(pluginLayer, itemGroup.second,
                    typeGroup.second, -1, 0, resourcePool_));
        } else if (typeGroup.first > -1) {
            newJobs.push_back(new BuildGroupJob(nullptr, itemGroup.second,
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
        CHECK_RET(typeGroup_->init(*(itemGroup_), itemGroup_->info_->tagName_),
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
        std::vector<int> matchIndexes;
        while (totalCount--) {
            CHECK_RET(pluginLayer_->newMifItem(index, nullptr, &workingItem),
                    "Failed to create new mif item while building group.");
            result = satisfyConditions(*(groupInfo->configItem_),
                    workingItem);
            CHECK_RET(result, "Failed to check conditions in mif item.");
            if (result) {
                matchIndexes.push_back(index);
            }
            delete workingItem;
            index++;
        }
        itemGroup_->addElements(matchIndexes);
        if (groupInfo->checkedCount_ + itemCount_ == pluginLayer_->size()) {
            itemGroup_->ready_.signalAll();
        }
        groupInfo->checkedCount_ += itemCount_;
    }
    std::lock_guard<std::mutex> jobCacheGuard(resourcePool_->jobCacheLock_);
    if (resourcePool_->parseGroupJobCount_ == 0) {
        std::vector<ExecutorJob*> newJobs;
        for (int i = 0; i < resourcePool_->jobCache_.size(); i++) {
            newJobs.insert(newJobs.end(),
                    resourcePool_->jobCache_[i].begin(),
                    resourcePool_->jobCache_[i].end());
            resourcePool_->jobCache_[i].clear();
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
            *(subGroup_->group_);
    const int totalConfigCount = configItemGroup.size();
    while (itemCount--) {
#ifdef DEBUG_OP
        std::cout << ">>Process Mif Item: " << itemIndex + 1 << "/" <<
                srcLayer_->size() << std::endl;
#endif
        CHECK_RET(srcLayer_->newMifItem(itemIndex++, targetLayer_,
                &workingItem), "Failed to create %s",
                "new mif item while processing mif item.");
        for (int configIndex = 0; configIndex < totalConfigCount;
                configIndex++) {
            int result = 0;
#ifdef DEBUG_OP
            std::cout << "--Process Config Item: " << configIndex << std::endl;
#endif
            ConfigItem* configItem = configItemGroup[configIndex].second;
            if (!configItem) continue;
            result = satisfyConditions(*configItem, workingItem);
            CHECK_RET(result, "Failed to %s \"%s\"[line: %d] %s [%d].",
                    "check conditions from config file",
                    subGroup_->filePath_->c_str(),
                    configItemGroup[configIndex].first,
                    "for mifitem", itemIndex);
            if (result) {
#ifdef DEBUG_OP
                std::cout << "++Match!" << std::endl;
#endif
#ifdef DEBUG_MATCH_INFO
                {
                    std::lock_guard<std::mutex> debugMatchInfoGuard(
                            debugMatchInfoLock);
                    debugMatchInfoStream << "Condition in file \"" <<
                            *(subGroup_->filePath_) << "\" [line: " <<
                            configItemGroup[configIndex].first << 
                            "] matched for mifitem[" << itemIndex <<
                            "] in layer \"" << srcLayer_->layerPath_ <<
                            "\"." << std::endl;
                }
#endif
                CHECK_RET(applyAssigns(*configItem, workingItem),
                        "Failed apply assign expr to mif item.");
                break;
            }
        }
        delete workingItem;
    }
    if ((subGroup_->processedCount_ += itemCount_) == srcLayer_->size()) {
        std::vector<ExecutorJob*> newJobs;
        if (*(subGroup_->finishedFileCount_) + 1 >=
                subGroup_->savePoint_) {
            newJobs.push_back(new SaveLayerJob(subGroup_->targetLayerID_,
                    *(subGroup_->savePath_), resourcePool_));
        }
        if (!ExecutorPool::runParallel_ && 
                !(resourcePool_->parseConfigFileJobs_.empty())) {
            newJobs.push_back(resourcePool_->parseConfigFileJobs_.front());
            resourcePool_->parseConfigFileJobs_.pop();
        }
        std::lock_guard<std::mutex> candidateQueueGuard(
                resourcePool_->candidateQueueLock_);
        if (!newJobs.empty()) {
            for (ExecutorJob* job : newJobs) {
                resourcePool_->candidateQueue_.push(job);
            }
            resourcePool_->newCandidateJob_->signalAll();
        }
        (*(subGroup_->finishedFileCount_))++;
    }
    return 0;
}

} // namespace job

} // namespace condition_assign
