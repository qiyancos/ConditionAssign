#include "ResourcePool.h"

namespace condition_assign {

ResourcePool() {}

int ResourcePool::init(const int targetCnt, const int pluginCnt,
        const int executorCnt, const int candidateQueueCnt) {
    pluginCnt_ = pluginCnt;
    executorCnt_ = executorCnt;
    readyQueue_.resize(executorCnt, std::queue<ExecutorJob*>());
    targetCnt_ = targetCnt;
    configGroup_.resize(targetCnt, new ConfigSubGroup());
    candidateQueueCnt_ = candidateCnt;
    candidateQueue_.resize(candidateCnt, std::queue<ExecutorJob*>());
    inputLayer_ = new MifLayerReadOnly();
    outputLayers_.resize(targetCnt, new MifLayerReadWrite());
    return 0;
}

int ResourcePool::getConfigSubGroup(int targetID, ConfigSubGroup** subGroupPtr) {
    CHECK_ARGS(targetID < targetCnt, "Invalid target ID.");
    *subGroupPtr = configGroup_[targetID];
    return 0;
}

int ResourcePool::newGroup(Group** newGroupPtr, Group* newGroup) {
    groupsLock_.lock();
    groups_.push_back(newGroup);
    groupsLock_.unlock();
    *newGroupPtr = newGroup;
    return 0;
}

int ResourcePool::insertGroup(const std::string& key, Group* newGroup) {
    groupMapLock_.lock();
    CHECK_ARGS(groupMap_.find(key) == groupMap_.end(),
            "Trying to insert the same group again.");
    groupMap_[key] = newGroup;
    groupMapLock_.unlock();
    return 0;
}

int ResourcePool::findGroup(const std::string& key, Group** groupPtr){
    groupMapLock_.lock();
    auto mapIterator = groupMap_.find(key);
    if (mapIterator == groupMap_.end()) {
        groupMapLock_.unlock();
        return -1;
    } else {
        *groupPtr = mapIterator->second;
        groupMapLock_.unlock();
        return 0;
    }
}

int ResourcePool::openLayer(const std::string& layerPath,
        const LayerType layerType, const int layerID = -1) {
    if (layerType = Input) {
        CHECK_RET(inputLayer_->open(layerPath),
                (std::string("Failed to open input layer \"") +
                layerPath + "\".").c_str());
        return 0;
    } else if (layerType = Output) {
        outputLayersLock_.lock();
        CHECK_ARGS(outputLayerMap_.find(layerPath) ==
                outputLayerMap_.end(),
                (std::string("Trying to reopen output layer \"") +
                layerPath + "\".").c_str());
        CHECK_ARGS(layerID < targetCnt_, "Too much open output layers.");
        outputLayerMap_[layerPath] = layerID;
        CHECK_RET(outputLayers_[layerID]->open(layerPath, inputLayer_),
                (std::string("Failed to open output layer \"") +
                layerPath + "\".").c_str());
        outputLayersLock_.unlock();
        return 0;
    } else {
        pluginLayersLock_.lock();
        CHECK_ARGS(pluginLayerMap_.find(layerPath) ==
                pluginLayerMap_.end(),
                (std::string("Trying to reopen plugin layer \"") +
                layerPath + "\".").c_str());
        MifLayer* newLayer = new MifLayerReadOnly();
        pluginLayers_[layerPath] = newLayer;
        CHECK_RET(newLayer->open(layerPath),
                (std::string("Failed to open output layer \"") +
                layerPath + "\".").c_str());
        pluginLayersLock_.unlock();
        return 0;
    }
}

int ResourcePool::getLayerByName(MifLayer** layerPtr,
        const LayerType layerType, const std::string& layerPath,
        int* targetID = nullptr) {
    if (layerType == Input) {
        // 我们建议使用getLayerByIndex而不是该函数获取输入层指针
        CHECK_ARGS(targetIDPtr == nullptr, "No target ID for input layer.");
        *layerPtr = inputLayer_;
    } else if (layerType = Output) {
        bool needLock = outputLayerMap_.size() < targetCnt_;
        if (needLock) outputLayersLock_.lock();
        auto mapIterator = outputLayerMap_.find(layerPath);
        CHECK_ARGS(mapIterator != outputLayerMap_.end(),
                (std::string("Can not find output layer named as \"") +
                layerPath + "\".").c_str());
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
                (std::string("Can not find plugin layer named as \"") +
                layerPath + "\".").c_str());
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
                "Can not find input layer with target id.");
        *layerPtr = inputLayer_;
        return 0;
    } else if (layerType = Output) {
        CHECK_ARGS(targetID > -1 && targetID < targetCnt,
                "Invalid targetID.");
        bool needLock = outputLayerMap_.size() < targetCnt_;
        if (needLock) outputLayersLock_.lock();
        *layerPtr = outputLayers_[targetID];
        if (needLock) outputLayersLock_.unlock();
        return 0;
    } else {
        CHECK_ARGS(false, "Plugin layer cannot be found with index.");
    }
}

int ResourcePool::getReadyJob(const int executorID,
        ExecutorJob** jobConsumerPtr) {
    std::stringstream tempStream;
    CHECK_ARGS(!readyQueue_[executorID].empty(),
            (tempStream << "No ready job found for executor[" <<
            executorID << "].").str().c_str());
    readyQueueLock_[executorID].lock();
    *jobConsumerPtr = readyQueue_[executorID].front();
    CHECK_ARGS(*jobConsumerPtr != nullptr,
            "Error occourred while get ready job.");
    readyJobCnt_--;
    readyQueue_[executorID].pop();
    readyQueueLock_[executorID].unlock();
    return 0;
}

int ResourcePool::selectReadyJob(std::set<int>* wakeupExecutorID) {
    using vacacy = std::pair<ExecutorJob**, int>;
    std::vector<vacacy> jobVacacies;
    for (auto lock : readyQueueLock_) {
        lock.lock();
    }
    for (int index; index < executorCnt_; index++) {
        std::queue<ExecutorJob*>& que = readyQueue_[index];
        while(que.size() < MAX_READY_QUEUE_SIZE) {
            que.push(nullptr);
            jobVacacies.push_back(vacacy(&(que.back()), index));
        }
    }
    candidateQueueLock_.lock();
    int selected = 0, index = 0;
    while (selected < jobVacacies.size()) {
        if (!candidateQueue_[index].empty()) {
            *(jobVacacies[selected].first) = candidateQueue_[index].front();
            signalExecutorIndexes->insert(jobVacacies[selected++].second);
            readyJobCnt_++;
            candidateJobCnt_--;
            candidateQueue_[index].pop();
        }
        if (++index == candidateQueueCnt_) {
            if (candidateJobCnt_ == 0) {
                break;
            } else {
                index = 0;
            }
        }
    }
    candidateQueueLock_.unlock();
    for (auto lock : readyQueueLock_) {
        lock.unlock();
    }
    return 0;
}

int ResourcePool::releaseResource() {
    for (auto configSubGroupPtr : configGroup_) {
        delete configSubGroupPtr;
    }
    for (auto groupPtr : groups_) {
        delete groupPtr;
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
    for (auto que : candidateQueue_) {
        for (auto jobPtr : que) {
            delete jobPtr;
        }
    }
}

} // namespace condition_assign
