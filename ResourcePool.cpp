#include "ResourcePool.h"

namespace condition_assign {

ResourcePool() {}

int ResourcePool::init(const int targetCnt, const int pluginCnt,
        const int executorCnt) {
    pluginCnt_ = pluginCnt;
    executorCnt_ = executorCnt;
    readyQueue_.resize(executorCnt, std::queue<ExecutorJob*>());
    targetCnt_ = targetCnt;
    configGroup_.resize(targetCnt, new ConfigSubGroup());
    inputLayer_ = new MifLayerReadOnly();
    outputLayers_.resize(targetCnt, new MifLayerReadWrite());
    return 0;
}

int ResourcePool::getConfigSubGroup(int targetID,
        ConfigSubGroup** subGroupPtr) {
    CHECK_ARGS(targetID < targetCnt, "Invalid target ID [%d].", targetID);
    *subGroupPtr = configGroup_[targetID];
    return 0;
}

int ResourcePool::insertGroup(const int key, Group* newGroup) {
    std::lock_guard<std::mutex> mapGuard(groupMapLock_);
    CHECK_ARGS(groupMap_.find(key) == groupMap_.end(),
            "Trying to insert a group with the same key[%d] again.", key);
    groupMap_[key] = newGroup;
    return 0;
}

int ResourcePool::findGroup(const int key, Group** groupPtr){
    std::lock_guard<std::mutex> mapGuard(groupMapLock_);
    auto mapIterator = groupMap_.find(key);
    if (mapIterator == groupMap_.end()) {
        return -1;
    } else {
        *groupPtr = mapIterator->second;
        return 0;
    }
}

int ResourcePool::findInsertGroup(const int itemGroupKey, Group** itemGroupPtr,
        const int typeGroupKey = -1, Group** typeGroupPtr = nullptr) {
    int result = 0;
    std::lock_guard<std::mutex> mapGuard(groupMapLock_);
    auto itemIterator = groupMap_.find(itemGroupKey)
    auto typeIterator = groupMap_.find(typeGroupKey)
    if (mapIterator != groupMap_.end()) {
        result++;
        if (*itemGroupPtr != nullptr) {
            delete *itemGroupPtr;
        }
        *itemGroupPtr = itemIterator.second;
    } else {
        CHECK_ARGS(*itemGroupPtr != nullptr,
                "Can not insert empty item group into group map.");
        groupMap_[itemGroupKey] = *itemGroupPtr;
    }
    if (typeGroupKey != -1) {
        if (typeIterator != groupMap_.end()) {
            CHECK_ARGS(itemIterator != groupMap_.end(),
                "Lack of item group bound with type group.");
            result++;
            if (*typeGroupPtr != nullptr) {
                delete *typeGroupPtr;
            }
            *typeGroupPtr = typeIterator.second;
        } else {
            CHECK_ARGS(*typeGroupPtr != nullptr,
                    "Can not insert empty item group into group map.");
            groupMap_[typeGroupKey] = *typeGroupPtr;
        }
    }
    return result;
}

int ResourcePool::openLayer(const std::string& layerPath,
        const LayerType layerType, const int layerID = -1) {
    if (layerType = Input) {
        CHECK_RET(inputLayer_->open(layerPath),
                "Failed to open input layer \"%s\".", layerPath.c_str());
        std::lock_guard<std::mutex> pluginLayerGuard(pluginLayersLock_);
        pluginLayersMap_[layerPath] = inputLayer_;
        return 0;
    } else if (layerType = Output) {
        outputLayersLock_.lock();
        CHECK_ARGS(outputLayerMap_.find(layerPath) == outputLayerMap_.end(),
                "Trying to reopen output layer \"%s\".", layerPath.c_str());
        CHECK_ARGS(layerID < targetCnt_, "Too much open output layers.");
        outputLayerMap_[layerPath] = layerID;
        CHECK_RET(outputLayers_[layerID]->open(layerPath, inputLayer_),
                "Failed to open output layer \"%s\".", layerPath.c_str());
        outputLayersLock_.unlock();
        return 0;
    } else {
        pluginLayersLock_.lock();
        auto mapIterator = pluginLayersMap_.find(layerPath);
        if (mapIterator != pluginLayersMap_.end()) {
            return 0;
        }
        MifLayer* newLayer = new MifLayerReadOnly();
        pluginLayersMap_[layerPath] = newLayer;
        CHECK_RET(newLayer->open(layerPath),
                "Failed to open plugin layer \"%s\".", layerPath.c_str());
        pluginLayersLock_.unlock();
        return 0;
    }
}

int ResourcePool::getLayerByName(MifLayer** layerPtr,
        const LayerType layerType, const std::string& layerPath,
        int* targetID = nullptr) {
    if (layerType == Input) {
        // 我们建议使用getLayerByIndex而不是该函数获取输入层指针
        CHECK_ARGS(targetIDPtr == nullptr,
                "Can not get target ID for input layer.");
        *layerPtr = inputLayer_;
    } else if (layerType = Output) {
        bool needLock = outputLayerMap_.size() < targetCnt_;
        if (needLock) outputLayersLock_.lock();
        auto mapIterator = outputLayerMap_.find(layerPath);
        CHECK_ARGS(mapIterator != outputLayerMap_.end(),
                "Can not find output layer named as \"%s\".",
                layerPath.c_str());
        *targetID = mapIterator->second;
        *layerPtr = outputLayers_[mapIterator->second];
        if (needLock) outputLayersLock_.unlock();
        return 0;
    } else {
        bool needLock = pluginLayerMap_.size() < targetCnt_;
        if (needLock) pluginLayersLock_.lock();
        auto mapIterator = pluginLayerMap_.find(layerPath);
        if (mapIterator == pluginLayerMap_.end()) {
            std::string completeLayerPath;
            for (auto mapIteratorTemp : pluginLayerMap_) {
                if (mapIteratorTemp->first.find(layerPath)) {
                    completeLayerPath = mapIteratorTemp->first;
                    break;
                }
            }
            CHECK_ARGS(!completeLayerPath.empty(),
                "Can not find plugin layer named as \"%s\".",
                layerPath.c_str());
            *layerPtr = pluginLayers_[completeLayerPath];
        } else {
            *layerPtr = pluginLayers_[mapIterator->second];
        }
        if (needLock) pluginLayersLock_.unlock();
        return 0;
    }
}

int ResourcePool::getLayerByIndex(MifLayer** layerPtr,
        const LayerType layerType, const int targetID = -1) {
    if (layerType = Input) {
        CHECK_ARGS(targetID == -1,
                "Can not find input layer with target ID [%d].", targetID);
        *layerPtr = inputLayer_;
        return 0;
    } else if (layerType = Output) {
        CHECK_ARGS(targetID > -1 && targetID < targetCnt,
                "Invalid targetID [%d].", targetID);
        bool needLock = outputLayerMap_.size() < targetCnt_;
        if (needLock) outputLayersLock_.lock();
        *layerPtr = outputLayers_[targetID];
        if (needLock) outputLayersLock_.unlock();
        return 0;
    } else {
        CHECK_ARGS(false, "Plugin layer cannot be found with layer ID.");
    }
}

int ResourcePool::getPluginFullPath(const std::string& layerName,
        std::string* fullPath) {
    bool needLock = pluginLayerMap_.size() < targetCnt_;
    if (needLock) pluginLayersLock_.lock();
    auto mapIterator = pluginLayerMap_.find(layerName);
    if (mapIterator == pluginLayerMap_.end()) {
        for (auto mapIteratorTemp : pluginLayerMap_) {
            if (mapIteratorTemp->first.find(layerName)) {
                *fullPath = mapIteratorTemp->first;
                if (needLock) pluginLayersLock_.lock();
                return 0;
            }
        }
        return -1;
    } else {
        *fullPath = mapIterator->first;
        if (needLock) pluginLayersLock_.unlock();
        return 0;
    }
}

int ResourcePool::getReadyJob(const int executorID,
        ExecutorJob** jobConsumerPtr) {
    CHECK_ARGS(!readyQueue_[executorID].empty(),
            "No ready job found for executor[%d].", executorID);
    readyQueueLock_[executorID].lock();
    *jobConsumerPtr = readyQueue_[executorID].front();
    CHECK_ARGS(*jobConsumerPtr != nullptr,
            "No job consumer pointer is provided.");
    readyJobCnt_--;
    readyQueue_[executorID].pop();
    readyQueueLock_[executorID].unlock();
    return 0;
}

int ResourcePool::selectReadyJob(std::set<int>* wakeupExecutorID) {
    using vacancy = std::pair<ExecutorJob**, int>;
    std::vector<vacancy> jobVacancies;
    for (auto lock : readyQueueLock_) {
        lock.lock();
    }
    for (int index; index < executorCnt_; index++) {
        std::queue<ExecutorJob*>& que = readyQueue_[index];
        while(que.size() < MAX_READY_QUEUE_SIZE) {
            que.push(nullptr);
            jobVacancies.push_back(vacancy(&(que.back()), index));
        }
    }
    std::lock_guard<std::mutex> candidateGuard(candidateQueueLock_);
    int selected = 0, index = 0;
    while (selected < jobVacancies.size() && !candidateQueue_.empty()) {
        *(jobVacancies[selected].first) = candidateQueue_.front();
        signalExecutorIndexes->insert(jobVacancies[selected++].second);
        readyJobCnt_++;
        candidateQueue_.pop();
    }
    for (auto lock : readyQueueLock_) {
        lock.unlock();
    }
    return 0;
}

int ResourcePool::releaseResource() {
    for (auto configSubGroupPtr : configGroup_) {
        delete configSubGroupPtr;
    }
    for (auto groupMapIterator : groupMap_) {
        delete groupMapIterator->second;
    }
    delete inputLayer_;
    for (auto mapIterator : pluginLayerMap_) {
        delete mapIterator->second;
    }
    for (auto layerPtr : outputLayers_) {
        delete layerPtr;
    }
    for (auto que : readyQueue_) {
        for (auto jobPtr : que) {
            delete jobPtr;
        }
    }
    for (auto job : candidateQueue_) {
        delete job;
    }
}

} // namespace condition_assign
