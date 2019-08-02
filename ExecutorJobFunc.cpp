#include "ExecutorJobFunc.h"
#include "ConditionAssign.h"
#include "Group.h"

namespace condition_assign {

namespace job_func {

int loadLayer(void* param, const int executorID) {
    TEST(executorID);
    LoadLayerParam* paramPtr = reinterpret_cast<LoadLayerParam*>(param);
    CHECK_RET(paramPtr->resourcePool->openLayer(*(paramPtr->layerPath),
            paramPtr->layerType, paramPtr->layerID),
            "Failed to open layer \"%s\".", paramPtr->layerPath->c_str());
    return 0;
}

int saveLayer(void* param, const int executorID) {
    TEST(executorID);
    SaveLayerParam* paramPtr = reinterpret_cast<SaveLayerParam*>(param);
    MifLayer* layer;
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&layer,
            ResourcePool::Output, paramPtr->layerID),
            "Can not find output layer[%d].", paramPtr->layerID);
    CHECK_RET(layer->save(), "Failed to save output layer[%d].",
            paramPtr->layerID);
    return 0;
}

int parseConfigFile(void* param, const int executorID) {
    TEST(executorID);
    ParseConfigFileParam* paramPtr =
            reinterpret_cast<ParseConfigFileParam*>(param);
    std::ifstream configFileStream(paramPtr->filePath->c_str());
    CHECK_ARGS(configFileStream, "Failed to open config file \"%s\".",
            paramPtr->filePath->c_str());
    std::string content;
    std::vector<std::string>* fullContent = new std::vector<std::string>();
    while (getline(configFileStream, content)) {
        if (content.length() > 0 && htk::trim(content, " ")[0] != '#') {
            fullContent->push_back(content);
        }
    }
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
        newJobs.push_back(new ExecutorJob(ExecutorJob::ParseConfigLines,
                new ParseConfigLinesParam {fullContent, startIndex,
                lineCount, paramPtr->layerID, paramPtr->resourcePool}));
        startIndex += lineCount;
    }
    newJobs.push_back(new ExecutorJob(ExecutorJob::ParseConfigLines,
            new ParseConfigLinesParam {fullContent, edgeCount, totalCount -
            edgeCount, paramPtr->layerID, paramPtr->resourcePool}));
    MifLayer* targetLayer;
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&targetLayer,
            ResourcePool::Output, paramPtr->layerID),
            "Failed to get layer bind with this config file.")
    MifLayer* srcLayer;
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&srcLayer,
            ResourcePool::Input), "Failed to get intput layer.")
    srcLayer->ready_.wait();
    targetLayer->ready_.wait();
    std::lock_guard<std::mutex> lockGuard(
            paramPtr->resourcePool->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        paramPtr->resourcePool->candidateQueue_.push(job);
    }
    paramPtr->resourcePool->newCandidateJob_->signalAll();
    return 0;
}

int parseConfigLines(void* param, const int executorID) {
    TEST(executorID);
    ParseConfigLinesParam* paramPtr =
            reinterpret_cast<ParseConfigLinesParam*>(param);
    int index = paramPtr->startIndex;
    int lineCount = paramPtr->lineCount;
    ConfigSubGroup* subGroup;
    CHECK_RET(paramPtr->resourcePool->getConfigSubGroup(paramPtr->layerID,
            &subGroup), "Failed to get config sub group for layer[%d]",
            paramPtr->layerID);
    std::vector<std::pair<std::string, Group**>*> newGroups;
    while (lineCount--) {
        CHECK_RET(parser::parseConfigLine((*(paramPtr->fullContent))[index],
                subGroup, paramPtr->resourcePool, paramPtr->layerID,
                &newGroups), "Failed to parse single line \"%s\" %s",
                (*(paramPtr->fullContent))[index].c_str(), "in config file.");
        index++;
    }
    std::vector<ExecutorJob*> newJobs;
    for (std::pair<std::string, Group**>* groupInfo : newGroups) {
        // 新建buildGroup的相关工作项
        newJobs.push_back(new ExecutorJob(ExecutorJob::ParseGroup,
                new ParseGroupParam {groupInfo, paramPtr->resourcePool}));
    }
    std::vector<std::pair<int, int>> rangesInJob;
    subGroup->readyCnt_ += paramPtr->lineCount;
    if (subGroup->readyCnt_ == paramPtr->fullContent->size()) {
        // 当前配置文件的所有内容均已经解析完毕
        int scoreSum = 0, startIndex = 0, index = 0;
        for (ConfigItem* configItem : subGroup->group_) {
            int itemScore = 0;
            CHECK_RET(itemScore = configItem->score(),
                    "Failed to get config item's score.");
            scoreSum += itemScore;
            if (scoreSum > MAX_SCORE_SUM_PER_JOB) {
                rangesInJob.push_back(std::pair<int, int>(startIndex,
                        ++index));
                startIndex = index;
                scoreSum = 0;
            }
        }
        if (scoreSum < (MAX_SCORE_SUM_PER_JOB >> 1)) {
            startIndex = rangesInJob.empty() ? 0 : rangesInJob.back().first;
        }
        rangesInJob.push_back(std::pair<int, int>(startIndex,
                subGroup->group_.size()));
        delete paramPtr->fullContent;
    } else {
        return 0;
    }
    MifLayer* srcLayer;
    MifLayer* targetLayer;
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&srcLayer,
            ResourcePool::Input), "Failed to get input mif layer.");
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&targetLayer,
            ResourcePool::Output, paramPtr->layerID),
            "Failed to get output mif_layer-%d.", paramPtr->layerID);
    int itemCount = targetLayer->size();
    for (std::pair<int, int> range : rangesInJob) {
        for (int index = 0; index < itemCount; index++) {
            newJobs.push_back(new ExecutorJob(ExecutorJob::ProcessMifItem,
                    new ProcessMifItemParam {index, srcLayer, targetLayer,
                    subGroup, range.first, range.second - range.first}));
        }
    }
    std::lock_guard<std::mutex> candidateQueueGuard(
            paramPtr->resourcePool->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        paramPtr->resourcePool->candidateQueue_.push(job);
    }
    paramPtr->resourcePool->newCandidateJob_->signalAll();
    return 0;
}

int parseGroup(void* param, const int executorID) {
    TEST(executorID);
    // 第一个整数为-1表示当前Group是否已经存在并注册
    using GroupPair = std::pair<int, Group*>;
    ParseGroupParam* paramPtr = reinterpret_cast<ParseGroupParam*>(param);
    GroupPair itemGroup(-1, nullptr), typeGroup(-1, nullptr);
    CHECK_RET(parser::parseGroupInfo(paramPtr->groupInfo->first,
            paramPtr->resourcePool, &itemGroup, &typeGroup),
            "Failed to parse group infomation [%s]",
            paramPtr->groupInfo->first);
    // 判断解析group的结果
    if (typeGroup.second == nullptr) {
        *(paramPtr->groupInfo->second) = itemGroup.second;
        delete paramPtr->groupInfo;
        return 0;
    } else {
        *(paramPtr->groupInfo->second) = typeGroup.second;
        delete paramPtr->groupInfo;
        // 分发任务
        std::vector<ExecutorJob*> newJobs;
        if (itemGroup.first > -1) {
            CHECK_ARGS(typeGroup.first > 0,
                    "Type group found but item group not found.");
            MifLayer* pluginLayer;
            CHECK_RET(paramPtr->resourcePool->getLayerByName(&pluginLayer,
                    ResourcePool::Plugin, itemGroup.second->info_->layerName_),
                    "Failed to found plugin layer \"%s\"",
                    itemGroup.second->info_->layerName_.c_str());

            int itemCount = itemGroup.second->info_->configItem_->score();
            itemCount = MAX_SCORE_SUM_PER_JOB / itemCount;
            itemCount = itemCount == 0 ? 1 : itemCount;
            int startIndex = 0;
            int totalCount = pluginLayer->size();
            int edgeCount = totalCount / itemCount * itemCount;
            if (totalCount - edgeCount <= (itemCount >> 1)) {
                edgeCount -= itemCount;
            }
            while (startIndex < edgeCount) {
                newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                        new BuildGroupParam {pluginLayer,itemGroup.second,
                        typeGroup.second, startIndex, itemCount,
                        paramPtr->resourcePool}));
                startIndex += itemCount;
            }
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                new BuildGroupParam {pluginLayer, itemGroup.second,
                typeGroup.second, edgeCount, totalCount -edgeCount,
                paramPtr->resourcePool}));
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                    new BuildGroupParam {pluginLayer, itemGroup.second,
                    typeGroup.second, -1, 0, paramPtr->resourcePool}));
        } else if (typeGroup.first > -1) {
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                    new BuildGroupParam {nullptr, itemGroup.second,
                    typeGroup.second, -1, 0, paramPtr->resourcePool}));
        }
        std::lock_guard<std::mutex> candidateGuard(
                paramPtr->resourcePool->candidateQueueLock_);
        for (ExecutorJob* job : newJobs) {
            paramPtr->resourcePool->candidateQueue_.push(job);
        }
        paramPtr->resourcePool->newCandidateJob_->signalAll();
    }
    return 0;
}

int buildGroup(void* param, const int executorID) {
    TEST(executorID);
    BuildGroupParam* paramPtr = reinterpret_cast<BuildGroupParam*>(param);
    if (paramPtr->startIndex == -1) {
        CHECK_RET(paramPtr->typeGroup->init(*(paramPtr->itemGroup),
                paramPtr->typeGroup->info_->tagName_),
                "Failed to init type group from item group.");
        CHECK_ARGS(!paramPtr->typeGroup->info_,
                "Type group should not have group info while building.");
        paramPtr->typeGroup->ready_.signalAll();
    } else {
        Group::GroupInfo* groupInfo = paramPtr->itemGroup->info_;
        CHECK_ARGS(groupInfo, "Group info should be available for building.");
        std::vector<int> passedIndex;
        int index = paramPtr->startIndex;
        int result = 0, totalCount = paramPtr->itemCount;
        MifItem* workingItem;
        while (totalCount--) {
            CHECK_RET(paramPtr->pluginLayer->newMifItem(index,
                    &workingItem, nullptr),
                    "Failed to create new mif item while building group.");
            result = satisfyConditions(*(groupInfo->configItem_),
                    workingItem);
            CHECK_RET(result, "Failed to check conditions in mif item.");
            if (result) {
                passedIndex.push_back(index);
            }
            index++;
        }
        for (int newIndex : passedIndex) {
            paramPtr->itemGroup->addElement(newIndex);
        }
        groupInfo->checkedCnt_ += totalCount;
        if (groupInfo->checkedCnt_ == paramPtr->pluginLayer->size()) {
            delete groupInfo;
            paramPtr->itemGroup->info_ = nullptr;
            paramPtr->itemGroup->ready_.signalAll();
        }
    }
    return 0;
}

int processMifItem(void* param, const int executorID) {
    TEST(executorID);
    ProcessMifItemParam* paramPtr =
            reinterpret_cast<ProcessMifItemParam*>(param);
    MifItem* workingItem;
#ifdef DEBUG_OP
    std::cout << ">>Process Mif Item: " << paramPtr->mifItemIndex
            << std::endl;
#endif
    CHECK_RET(paramPtr->srcLayer->newMifItem(paramPtr->mifItemIndex,
            &workingItem, paramPtr->targetLayer),
            "Failed to create new mif item while processing mif item.");
    std::vector<ConfigItem*>& subGroup = paramPtr->subGroup->group_;
    int configIndex = paramPtr->startIndex;
    int result = 0;
    while (paramPtr->itemCount--) {
#ifdef DEBUG_OP
        std::cout << "--Process Config Item: " << configIndex << std::endl;
#endif
        ConfigItem* configItem = subGroup[configIndex++];
        result = satisfyConditions(*configItem, workingItem);
        CHECK_RET(result, "Failed to check conditions in mif item.");
        if (result) {
#ifdef DEBUG_OP
            std::cout << "++Match!" << std::endl;
#endif
            CHECK_RET(applyAssigns(*configItem, workingItem),
                    "Failed apply assign expr to mif item.");
        }
    }
    return 0;
}

} // namespace job_func

} // namespace condition_assign
