#include "ResourcePool.h"

namespace condition_assign {

int ResourcePool::init(ExecutorPool::Params params) {
    executorCnt_ = params.executorNum;
    targetCnt_ = params.outputs.size();
    readyQueue_.resize(executorCnt_, std::queue<ExecutorJob*>());
    configGroup_.resize(targetCnt_, new ConfigSubGroup());
    inputLayer_ = new MifLayerReadOnly();
    pluginLayersMap_[params.input] = inputLayer_;
    for (std::string layerPath : params.plugins) {
        pluginLayersMap_[layerPath] = new MifLayerReadWrite();
    }
    outputLayers_.resize(targetCnt_, new MifLayerReadWrite());
    for (int layerID = 0; layerID < targetCnt_; layerID++) {
        outputLayersMap_[params.outputs[layerID]] = layerID;
    }
    return 0;
}

int ResourcePool::getConfigSubGroup(int targetID,
        ConfigSubGroup** subGroupPtr) {
    CHECK_ARGS(targetID < targetCnt_, "Invalid target ID [%d].", targetID);
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
    auto itemIterator = groupMap_.find(itemGroupKey);
    auto typeIterator = groupMap_.find(typeGroupKey);
    if (itemIterator != groupMap_.end()) {
        result++;
        if (*itemGroupPtr != nullptr) {
            delete *itemGroupPtr;
        }
        *itemGroupPtr = itemIterator->second;
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
            *typeGroupPtr = typeIterator->second;
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
    if (layerType == Input) {
        CHECK_RET(inputLayer_->open(layerPath),
                "Failed to open input layer \"%s\".", layerPath.c_str());
        return 0;
    } else if (layerType == Output) {
        int index = -1;
        if (layerID == -1) {
            auto mapIterator = outputLayersMap_.find(layerPath);
            CHECK_ARGS(mapIterator != outputLayersMap_.end(),
                    "Trying to open output layer \"%s\" not registered.",
                    layerPath.c_str());
            index = mapIterator->second;
        }
        CHECK_RET(outputLayers_[index]->open(layerPath, inputLayer_),
                "Failed to open output layer \"%s\".", layerPath.c_str());
        return 0;
    } else {
        auto mapIterator = pluginLayersMap_.find(layerPath);
        if (mapIterator != pluginLayersMap_.end()) {
            return 0;
        }
        CHECK_RET(mapIterator->second->open(layerPath),
                "Failed to open plugin layer \"%s\".", layerPath.c_str());
        return 0;
    }
}

int ResourcePool::getLayerByName(MifLayer** layerPtr,
        const LayerType layerType, const std::string& layerPath,
        int* targetID = nullptr) {
    if (layerType == Input) {
        // 我们建议使用getLayerByIndex而不是该函数获取输入层指针
        CHECK_ARGS(!targetID, "Can not get target ID for input layer.");
        *layerPtr = inputLayer_;
    } else if (layerType == Output) {
        auto mapIterator = outputLayersMap_.find(layerPath);
        CHECK_ARGS(mapIterator != outputLayersMap_.end(),
                "Can not find output layer named as \"%s\".",
                layerPath.c_str());
        *targetID = mapIterator->second;
        *layerPtr = outputLayers_[mapIterator->second];
        return 0;
    } else {
        auto mapIterator = pluginLayersMap_.find(layerPath);
        if (mapIterator == pluginLayersMap_.end()) {
            std::string completeLayerPath;
            CHECK_RET(getPluginFullPath(layerPath, &completeLayerPath),
                "Can not find plugin layer named as \"%s\".",
                layerPath.c_str());
            *layerPtr = pluginLayersMap_[completeLayerPath];
        } else {
            *layerPtr = mapIterator->second;
        }
        return 0;
    }
}

int ResourcePool::getLayerByIndex(MifLayer** layerPtr,
        const LayerType layerType, const int targetID = -1) {
    if (layerType == Input) {
        CHECK_ARGS(targetID == -1,
                "Can not find input layer with target ID [%d].", targetID);
        *layerPtr = inputLayer_;
        return 0;
    } else if (layerType == Output) {
        CHECK_ARGS(targetID > -1 && targetID < targetCnt_,
                "Invalid targetID [%d].", targetID);
        *layerPtr = outputLayers_[targetID];
        return 0;
    } else {
        CHECK_ARGS(false, "Plugin layer cannot be found with layer ID.");
    }
}

int ResourcePool::getPluginFullPath(const std::string& layerName,
        std::string* fullPath) {
    auto mapIterator = pluginLayersMap_.find(layerName);
    if (mapIterator == pluginLayersMap_.end()) {
        for (auto mapIteratorTemp : pluginLayersMap_) {
            if (mapIteratorTemp.first.find(layerName) != std::string::npos) {
                *fullPath = mapIteratorTemp.first;
                return 0;
            }
        }
        return -1;
    } else {
        *fullPath = mapIterator->first;
        return 0;
    }
}

int ResourcePool::getReadyJob(const int executorID,
        ExecutorJob** jobConsumerPtr) {
    CHECK_ARGS(!readyQueue_[executorID].empty(),
            "No ready job found for executor[%d].", executorID);
    std::lock_guard<std::mutex> readyQueueGuard(readyQueueLock_[executorID]);
    *jobConsumerPtr = readyQueue_[executorID].front();
    CHECK_ARGS(*jobConsumerPtr != nullptr,
            "No job consumer pointer is provided.");
    readyJobCnt_--;
    readyQueue_[executorID].pop();
    return 0;
}

int ResourcePool::selectReadyJob(std::set<int>* wakeupExecutorID) {
    using vacancy = std::pair<ExecutorJob**, int>;
    std::vector<vacancy> jobVacancies;
    for (int index = 0; index < readyQueueLock_.size(); index++) {
        readyQueueLock_[index].lock();
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
        wakeupExecutorID->insert(jobVacancies[selected++].second);
        readyJobCnt_++;
        candidateQueue_.pop();
    }
    for (int index = 0; index < readyQueueLock_.size(); index++) {
        readyQueueLock_[index].unlock();
    }
    return 0;
}

ResourcePool::~ResourcePool() {
    for (auto configSubGroupPtr : configGroup_) {
        if (configSubGroupPtr != nullptr) {
            delete configSubGroupPtr;
        }
    }
    for (auto groupMapIterator : groupMap_) {
        if (groupMapIterator.second != nullptr) {
            delete groupMapIterator.second;
        }
    }
    for (auto mapIterator : pluginLayersMap_) {
        delete mapIterator.second;
    }
    for (auto layerPtr : outputLayers_) {
        if (layerPtr != nullptr) {
            delete layerPtr;
        }
    }
    for (auto que : readyQueue_) {
        while (!que.empty()) {
            ExecutorJob* jobPtr = que.front();
            if (jobPtr != nullptr) {
                delete jobPtr;
            }
            que.pop();
        }
    }
    while (!candidateQueue_.empty()) {
        ExecutorJob* jobPtr = candidateQueue_.front();
        if (jobPtr != nullptr) {
            delete jobPtr;
        }
        candidateQueue_.pop();
    }
}

} // namespace condition_assign
