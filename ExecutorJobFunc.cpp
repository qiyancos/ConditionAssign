#include "ExecutorJobFunc.h"
#include "ConditionAssign.h"

namespace condition_assign {

namespace job_func {

int loadLayer(void* param) {
    LoadLayerParams* paramPtr = reinterpret_cast<LoadLayerParams*>(param);
    CHECK_RET(paramPtr->resourcePool->openLayer(*(paramPtr->layerPath),
            paramPtr->layerType, paramPtr->layerID),
            "Failed to open layer \"%s\".", paramPtr->layerPath->c_str());
    return 0;
}

int saveLayer(void* param) {
    SaveLayerParam* paramPtr = reinterpret_cast<SaveLayerParam*>(param);
    MifLayer* layer;
    CHECK_RET(paramPtr->resourcePool->getLayer(&layer, ResourcePool::Output,
            *(paramPtr->layerPath)),
            "Can not find layer with layer path \"%s\".",
            paramPtr->layerPath->c_str());
    CHECK_RET(layer->save(), "Failed to save layer \"%s\".",
            paramPtr->layerPath->c_str());
    return 0;
}

int parseConfigFile(void* param) {
    ParseConfigFileParam* paramPtr =
            reinterpret_cast<ParseConfigFileParam*>(param);
    std::ifstream configFileStream(paramPtr->filePath);
    CHECK_ARGS(configFileStream, "Failed to open config file \"%s\".",
            paramPtr->filePath.c_str());
    std::string content;
    std::vector<std::string>* fullContent = new std::vector<std::string>();
    while (getline(configFileStream, content)) {
        fullContent->push_back(content);
    }
    int startIndex = 0;
    int lineCount = MAX_LINE_PER_JOB;
    int totalCnt = fullContent->size() ;
    int edgeCount = totalCount / lineCount * lineCount;
    if (totalCount - edgeCount < (lineCount >> 1)) {
        edgeCount -= lineCount;
    }
    std::vector<ExecutorJob*> newJobs;
    while (startIndex < edgeCount) {
        newJobs.push_back(new ExecutorJob(ExecutorJob::ParseConfigLines,
                new ParseConfigLinesParams {fullContent, startIndex,
                lineCount, paramPtr->layerID, paramPtr->resourcePool}));
        startIndex += lineCount;
    }
    newJobs.push_back(new ExecutorJob(ExecutorJob::ParseConfigLines,
            new ParseConfigLinesParams {fullContent, edgeCount, totalCount -
            edgeCount, paramPtr->layerID, paramPtr->resourcePool}));
    std::lock_guard<std::mutex> lockGuard(
            paramPtr->resourcePool->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        paramPtr->resourcePool->candidateQueue_.push(job);
    }
    paramPtr->resourcePool->newCandidateJob.signalAll();
    return 0;
}

int parseConfigLines(void* param) {
    ParseConfigLineParam* paramPtr =
            reinterpret_cast<ParseConfigLineParam*>(param);
    int index = paramPtr->startIndex;
    int lineCount = paramPtr->lineCount;
    SubConfigGroup* subGroup;
    CHECK_RET(paramPtr->resourcePool->getConfigSubGroup(paramPtr->layerID,
            &subGroup), "Failed to get config sub group for layer[%d]",
            paramPtr->layerID);
    std::vector<std::pair<std::string, Group**>*> newGroups;
    while (lineCount--) {
        CHECK_RET(parser::parseConfigLine((*(paramPtr->fullContent))[index],
                subGroup, paramPtr->resourcePool, &newGroups),
                "Failed to parse single line in config file. [%s]",
                (*(paramPtr->fullContent))[index].c_str());
        index++;
    }
    std::vector<ExecutorJob*> newJobs;
    for (std::pair<std::string, Group**>* groupInfo : newGroups) {
        // 新建buildGroup的相关工作项
        newJobs.push_back(new ExecutorJob(ExecutorJob::ParseGroup,
                new ParseGroupParams {groupInfo, paramPtr->resourcePool}));
    }
    std::vector<std::pair<int, int>> rangesInJob;
    if (subGroup->readyCnt += lineCount == paramPtr->fullContent->size()) {
        // 当前配置文件的所有内容均已经解析完毕
        int scoreSum = 0, startIndex = 0, index = 0;
        for (ConfigItem configItem : subGroup->group) {
            int itemScore = 0;
            CHECK_RET(itemScore = configItem.score(),
                    "Failed to get config item's score.");
            scoreSum += itemScore;
            if (scoreSum > MAX_SCORE_SUM_PER_JOB) {
                rangesInJob.push_back(std::pair<int, int>(startIndex,
                        ++index));
                startIndex = index;
                scoreSum = 0;
                if (subGroup->readyCnt - startIndex < MIN_CONFIGITEM_PER_JOB) {
                    rangesInJob.push_back(std::pair<int, int>(startIndex,
                            subGroup->readyCnt - 1));
                    break;
                }
            }
        }
        delete paramPtr->fullContent;
    }
    MifLayer* srcLayer, targetLayer;
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&srcLayer,
            ResourcePool::Input), "Failed to get input mif layer.");
    CHECK_RET(paramPtr->resourcePool->getLayerByIndex(&targetLayer,
            ResourcePool::Output, paramPtr->layerID),
            "Failed to get output mif_layer-%d.", paramPtr->layerID);
    int itemCount = targetLayer->size();
    for (std::pair<int, int> range : rangesInJob) {
        for (int index = 0; index < itemCount; index++) {
            newJobs.push_back(new ExecutorJob(ExecutorJob::ProcessMifItem,
                    new ProcessMifItemParams {index, srcLayer, targetLayer,
                    subGroup, range.first, range.second - range.first}));
        }
    }
    std::lock_guard<std::mutex> candidateQueueGuard(
            paramPtr->resourcePool->candidateQueueLock_);
    for (ExecutorJob* job : newJobs) {
        paramPtr->resourcePool->candidateQueue_.push(job);
    }
    return 0;
}

int parseGroup(void* param) {
    // 第一个整数为-1表示当前Group是否已经存在并注册
    using GroupPair = std::pair<int, Group*>;
    ParseGroupParam* paramPtr = reinterpret_cast<ParseGroupParam*>(param);
    GroupPair itemGroup(-1, nullptr), typeGroup(-1, nullptr);
    CHECK_RET(parser::parseGroupInfo(paramPtr->groupInfo.first,
            paramPtr->resourcePool, &itemGroup, &typeGroup),
            "Failed to parse group infomation [%s]",
            paramPtr->groupInfo.first);
    // 判断解析group的结果
    if (typeGroup.second == nullptr) {
        *(paramPtr->groupInfo.second) = itemGroup.second;
        delete paramPtr->groupInfo;
        return 0;
    } else {
        *(paramPtr->groupInfo.second) = typeGroup.second;
        delete paramPtr->groupInfo;
        // 分发任务
        std::vector<ExecutorJob*> newJobs;
        if (itemGroup.first > -1) {
            CHECK_ARGS(typeGroup.first > 0,
                    "Type group found but item group not found.");
            MifLayer* pluginLayer;
            CHECK_RET(paramPtr->resourcePool->getLayerByName(&pluginLayer,
                    ResourcePool::Plugin, itemGroup.second->info_->layerName),
                    "Failed to found plugin layer \"%s\"",
                    itemGroup.second->info_->layerName.c_str());
            
            int itemCount = syntax::calculateScore(
                    itemGroup.second->info_->configItem.score());
            itemCount /= MAX_SCORE_SUM_PER_JOB;
            itemCount = itemCount == 0 ? 1 : itemCount;
            int startIndex = 0;
            int totalCnt = pluginLayer->size();
            int edgeCount = totalCount / itemCount * itemCount;
            if (totalCount - edgeCount <= (lineCount >> 1)) {
                edgeCount -= lineCount;
            }
            
            while (startIndex < edgeCount) {
                newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                        new BuildGroupParams {pluginLayer,itemGroup.second,
                        typeGroup.second, startIndex, itemCount,
                        paramPtr->resourcePool}));
                startIndex += itemCount;
            }
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                new BuildGroupParams {pluginLayer, itemGroup.second,
                typeGroup.second, edgeCount, totalCount -edgeCount,
                paramPtr->resourcePool}));
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                    new BuildGroupParams {pluginLayer, itemGroup.second,
                    typeGroup.second, -1, 0, paramPtr->resourcePool}));
        } else if (typeGroup.first > -1) {
            newJobs.push_back(new ExecutorJob(ExecutorJob::BuildGroup,
                    new BuildGroupParams {nullptr, itemGroup.second,
                    typeGroup.second, -1, 0, paramPtr->resourcePool}));
        }
        std::lock_guard<std::mutex> candidateGuard(
                paramPtr->resourcePool->candidateQueueLock_);
        for (ExecutorJob* job : newJobs) {
            paramPtr->resourcePool->candidateQueue_.push(job);
        }
        paramPtr->resourcePool->newCandidateJob.signalAll();
    }
    return 0;
}    

int buildGroup(void* param) {
    BuildGroupParam* paramPtr = reinterpret_cast<BuildGroupParam*>(param);
    if (paramPtr->startIndex == -1) {
        typeGroup.second->init(*itemGroup, typeGroup->info_->tagName);
        CHECK_ARGS(paramPtr->typeGroup->info_ == nullptr,
                "Type group should not have group info while building.");
        paramPtr->typeGroup->ready_.signalAll();
    } else {
        CHECK_ARGS(Group::GroupInfo* groupInfo = paramPtr->itemGroup->info_ !=
                nullptr, "Group info should be available for building.");
        std::vector<int> passedIndex;
        int index = paramPtr->startIndex;
        int result = 0, totalCount = paramPtr->itemCount;
        MifItem* workingItem;
        while (totalCount--) {
            CHECK_RET(paramPtr->pluginLayer->newMifItem(index,
                    &workingItem, nullptr),
                    "Failed to create new mif item while building group.");
            result = syntax::satisfyConditions(groupInfo->configItem,
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
        groupInfo->checkedCnt += totalCount;
        if (groupInfo->checkedCnt == pluginLayer->size()) {
            delete groupInfo;
            paramPtr->itemGroup->info_ = nullptr;
            paramPtr->itemGroup->ready_.signalAll();
        }
    }
    return 0;
}

int processMifItem(void* param) {
    ProcessMifItemParam* paramPtr =
            reinterpret_cast<ProcessMifItemParam*>(param);
    MifItem* workingItem;
    CHECK_RET(srcLayer->newMifItem(paramPtr->index, &workingItem,
            paramPtr->targetLayer),
            "Failed to create new mif item while processing mif item.");
    std::vector<ConfigItem>& subGroup = paramPtr->subGroup->group;
    int configIndex = startIndex;
    int result = 0;
    while (paramPtr->itemCount--) {
        ConfigItem& configItem = subGroup[configIndex++];
        result = syntax::satisfyConditions(configItem, workingItem);
        CHECK_RET(result, "Failed to check conditions in mif item.");
        if (result) {
            CHECK_RET(syntax::applyAssigns(configItem.assigns_,
                    workingItem), "Failed apply assign expr to mif item.");
        }
    }
    return 0;
}

} // namespace job_func

} // namespace condition_assign
