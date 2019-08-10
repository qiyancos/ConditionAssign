#include "ResourcePool.h"

namespace condition_assign {

int ResourcePool::init(const ExecutorPool::Params& params,
        Semaphore* newCandidateJob,
        std::map<std::string, ExecutorPool::LayerInfo>* layerInfo) {
    inputSize_ = params.inputs.size();
    configSize_ = params.configs.size();
    outputSize_ = params.outputs.size();
    // 初始化Layer
    CHECK_RET(initRunningModel(params, layerInfo),
            "Failed to init layers based on current running model.");
    // 其他内容的初始化
    newCandidateJob_ = newCandidateJob;
    executorCount_ = params.executorNum;
    targetCount_ = params.outputs.size();
    readyQueue_.resize(executorCount_, std::deque<ExecutorJob*>());
    for (int i = 0; i < executorCount_; i++) {
        readyQueueLock_.push_back(new std::mutex());
    }
    // 初始化串行场景依赖信号量和配置文件信息
    CHECK_RET(initConfigGroup(params, *layerInfo),
            "Failed to init config group.");
    return 0;
}

int ResourcePool::initRunningModel(const ExecutorPool::Params& params,
        std::map<std::string, ExecutorPool::LayerInfo>* layerInfo) {
    int index = 0;
    int sharedID = 0;
    int uniqueID = 0;
    std::map<std::string, int> readOnlyLayers;
    std::map<std::string, int> outputLayers;
    for (const std::string& input : params.inputs) {
        if (layerInfo->find(input) == layerList.end()) {
            (*layerInfo)[input] = ExecutorPool::LayerInfo();
        }
        if (readOnlyLayers[input] != readOnlyLayers.end()) {
            layers_.push_back(new MifLayerNormal(input));
            idMapping_[uniqueID] = sharedID++;
        } else {
            idMapping_[uniqueID] = readOnlyLayers[input];
        }
        if (ExecutorPool::runParallel_) {
            readOnlyLayers[input] = idMapping_[uniqueID];
        }
        uniqueID++;
        (*layerInfo)[input].inputIndexes.push_back(index++);
    }
    index = 0;
    for (const std::string& plugin : params.plugins) {
        if (layerInfo->find(plugin) == layerList.end()) {
            (*layerInfo)[plugin] = ExecutorPool::LayerInfo();
        }
        if (readOnlyLayers[plugin] != readOnlyLayers.end()) {
            layers_.push_back(new MifLayerNormal(plugin));
            idMapping_[uniqueID] = sharedID++;
        } else {
            idMapping_[uniqueID] = readOnlyLayers[plugin];
        }
        readOnlyLayers[plugin] = idMapping_[uniqueID++];
        (*layerInfo)[plugin].pluginIndexes.push_back(index++);
    }
    index = 0;
    if (ExecutorPool::runParallel_) {
        for (const std::string& output : params.outputs) {
            if (layerInfo->find(output) == layerList.end()) {
                (*layerInfo)[output] = ExecutorPool::LayerInfo();
            }
            // 没有注册当前输出层
            if (outputLayers.find(output) == outputLayers.end()) {
                if (htk::endswith(output, "*NEW*")) {
                    MifLayer* copySrcLayer = layers_[idMapping_[index]];
                    layers_.push_back(new MifLayerNew(output.substr(0,
                            output.size() - 5), copySrcLayer));
                    idMapping_[uniqueID] = sharedID++;
                } else {
                    std::string input = params.inputs[index];
                    if (layerInfo[input].inputIndexes == 1 &&
                            layerInfo[input].inputIndexes[0] == index) {
                        idMapping_[uniqueID] = idMapping_[index];
                    } else {
                        MifLayer* copySrcLayer = layers_[idMapping_[index]];
                        layers_.push_back(new MifLayerNormal(output,
                                copySrcLayer));
                        idMapping_[uniqueID] = sharedID++;
                    }
                }
            // 已经注册了当前输出层
            } else {
                idMapping_[uniqueID] = outputLayers[output];
            }
            outputLayers[output] = idMapping_[uniqueID++];
            (*layerInfo)[output].outputIndexes.push_back(index++);
        }
    } else {
        for (const std::string& output : params.outputs) {
            if (layerInfo->find(output) == layerList.end()) {
                (*layerInfo)[output] = ExecutorPool::LayerInfo();
            }
            // 没有注册当前输出层
            if (outputLayers.find(output) == outputLayers.end()) {
                if (htk::endswith(output, "*NEW*")) {
                    MifLayer* copySrcLayer = layers_[idMapping_[0]];
                    layers_.push_back(new MifLayerNew(output.substr(0,
                            output.size() - 5), copySrcLayer));
                    idMapping_[uniqueID] = sharedID++;
                } else {
                    std::string input = params.inputs[0];
                    MifLayer* copySrcLayer = layers_[idMapping_[0]];
                    layers_.push_back(new MifLayerNormal(output,
                            copySrcLayer));
                    idMapping_[uniqueID] = sharedID++;
                }
            // 已经注册了当前输出层
            } else {
                idMapping_[uniqueID] = outputLayers[output];
            }
            outputLayers[output] = idMapping_[uniqueID++];
            (*layerInfo)[output].outputIndexes.push_back(index++);
        }
    }
    CHECK_RET(initLayers(params, readOnlyLayers, *layerInfo),
            "Faile to init layers' properties.");
    return 0;
}

int ResourcePool::initLayers(const ExecutorPool::Params& params,
        const std::map<std::string, int>& readOnlyLayers,
        const std::map<std::string, ExecutorPool::LayerInfo>& layerInfo) {
    index = 0;
    if (!ExecutorPool::runParallel_) {
        if (layerInfo[params.inputs[0]].outputIndexes.size() > 1) {
            layers_[0]->setWithItemCache();
            CHECK_RET(layers_[0]->setGeoType(params.geoTypes[0]),
                    "Failed to set geometry type for layer \"%s\".",
                    params.inputs[0]);
        }
    }
    for (auto readOnlyLayer : readOnlyLayers) {
        // 设置输入层的地理类型
        const ExecutorPool::LayerInfo& info = layerInfo[readOnlyLayer.first];
        if (info.inputIndexes.size() + info.pluginIndexes.size() > 1 ||
                info.pluginIndexes.size() > 0) {
            layers_[readOnlyLayer.second]->setWithItemCache();
        }
        if (!info.inputIndexes.empty()) {
            CHECK_RET(layers_[readOnlyLayer.second]->setGeoType(
                    params.geoTypes[info.inputIndexes[0]])
                    "Failed to set geometry type for layer \"%s\".",
                    readOnlyLayer.first);
        }
    }
    return 0;
}

int ResourcePool::initConfigGroup(const ExecutorPool::Params& params,
        const std::map<std::string, ExecutorPool::LayerInfo>& layerInfo) {
    // 基于输入类型初始化ConfigGroup
    std::vector<int> savePoints(configSize_, 0);
    if (!ExecutorPool::runParallel_) {
        // 保存点生成
        int uniqueID = inputSize_ + pluginSize_;
        std::map<int, int> saveTag;
        for (int i = outputSize_ - 1; i >= 0; i--) {
            int sharedID = idMapping_[uniqueID];
            if (saveTag.find(sharedID) == saveTag.end()){
                savePoints[i] = i;
                saveTag[sharedID] = i;
            } else {
                savePoints[i] = saveTag[idMapping_[uniqueID]];
            }
        }
    }
    CHECK_RET(configGroup_.init(configSize_, targetCount_, savePoints, this),
            "Failed to init config group");
    return 0;
}

int ResourcePool::getConfigSubGroup(int targetID,
        ConfigSubGroup** subGroupPtr) {
    CHECK_ARGS(targetID < outputSize_, "Invalid target ID [%d].", targetID);
    *subGroupPtr = configGroup_.group_[targetID];
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

int ResourcePool::getLayersCount() {
    return layers_.size();
}

int ResourcePool::getLayerByName(MifLayer** layerPtr,
        const LayerType layerType, const std::string& layerPath,
        int* sharedID = nullptr) {
    if (layerType == Input) {
        auto mapIterator = inputLayersMap_.find(layerPath);
        CHECK_ARGS(mapIterator != inputLayersMap_.end(),
                "Can not find input layer named as \"%s\".",
                layerPath.c_str());
        if (sharedID) {
            *sharedID = mapIterator->second;
        }
        *layerPtr = layers_[mapIterator->second];
        return 0;
    } else if (layerType == Output) {
        auto mapIterator = outputLayersMap_.find(layerPath);
        CHECK_ARGS(mapIterator != outputLayersMap_.end(),
                "Can not find output layer named as \"%s\".",
                layerPath.c_str());
        if (sharedID) {
            *sharedID = mapIterator->second;
        }
        *layerPtr = layers_[mapIterator->second];
        return 0;
    } else {
        auto mapIterator = pluginLayersMap_.find(layerPath);
        if (mapIterator == pluginLayersMap_.end()) {
            std::string completeLayerPath;
            CHECK_RET(getPluginFullPath(layerPath, &completeLayerPath),
                "Can not find plugin layer named as \"%s\".",
                layerPath.c_str());
            int layerID = pluginLayersMap_[completeLayerPath];
            if (sharedID) {
                *sharedID = layerID;
            }
            *layerPtr = layers_[layerID];
        } else {
            if (sharedID) {
                *sharedID = mapIterator->second;
            }
            *layerPtr = layers_[mapIterator->second];
        }
        return 0;
    }
}

int ResourcePool::getLayerByIndex(MifLayer** layerPtr,
        const LayerType layerType, const int index) {
    int sharedID = -1;
    CHECK_RET(getSharedIDByIndex(layerType, index, &sharedID),
            "Failed to get shared id for given layer type and index.");
    *layerPtr = layers_[sharedID];
    return 0;
}

int ResourcePool::getSharedIDByIndex(const LayerType layerType, const int index,
        int* sharedID) {
    int sharedID = -1;
    if (layerType == Input) {
        CHECK_ARGS(index > -1 && index < inputSize_,
                "Invalid index[%d] for input layer.", index);
        if (ExecutorPool::runParallel) {
            *sharedID = idMapping_[index];
        } else {
            *sharedID = idMapping_[0];
        }
    } else if (layerType == Output) {
        CHECK_ARGS(index > -1 && index < outputSize_,
                "Invalid index[%d] for output layer.", index);
        *sharedID = idMapping_[index + inputSize_ + pluginSize_];
    } else {
        CHECK_ARGS(index > -1 && index < pluginSize_,
                "Invalid index[%d] for plugin layer.", index);
        *sharedID = idMapping_[index + inputSize_];
    }
    return 0;
}

int ResourcePool::getPluginFullPath(const std::string& layerName,
        std::string* fullPath) {
    for (auto mapIteratorTemp : pluginLayersMap_) {
        if (mapIteratorTemp.first.find(layerName) != std::string::npos) {
            *fullPath = mapIteratorTemp.first;
            return 0;
        }
    }
    return -1;
}

int ResourcePool::getOutputFullPath(const int index, std::string** fullPath) {
    CHECK_ARGS(index > -1 && index < outputSize_,
            "Index[%d] of output layer out of bound.", index);
    *fullPath = &(outputLayersPath_[index]);
    return 0;
}

int ResourcePool::getReadyJob(const int executorID,
        ExecutorJob** jobConsumerPtr) {
    TEST(executorID);
    std::lock_guard<std::mutex> readyQueueGuard(
            *(readyQueueLock_[executorID]));
    TEST(executorID);
    if (readyQueue_[executorID].empty()) {
        TEST(executorID);
        return 0;
    }
    CHECK_ARGS(*jobConsumerPtr == nullptr,
            "Job consumer already have available job.");
    *jobConsumerPtr = readyQueue_[executorID].front();
    CHECK_ARGS(*jobConsumerPtr != nullptr,
            "No job consumer pointer is provided.");
    readyJobCount_--;
    readyQueue_[executorID].pop_front();
    TEST(executorID);
    return 1;
}

int ResourcePool::selectReadyJob(std::set<int>* wakeupExecutorID) {
    TEST("rc");
    std::lock_guard<std::mutex> candidateGuard(candidateQueueLock_);
    if (candidateQueue_.empty()) {
        return 0;
    }
    using vacancy = std::pair<ExecutorJob**, int>;
    std::vector<vacancy> jobVacancies;
    for (int index = executorCount_ - 1; index >= 0; index--) {
        readyQueueLock_[index]->lock();
    }
#ifdef DEBUG_JOB
    std::cout << "Before select: ";
    for (auto que : readyQueue_) {
        std::cout << que.size() << " ";
    }
    std::cout << std::endl;
#endif
    TEST("rc");
    int maxReadySize = candidateQueue_.size() + readyJobCount_;
    int avgSize = maxReadySize / executorCount_;
    maxReadySize = maxReadySize % executorCount_ > 0 ? avgSize + 1 : avgSize;
    maxReadySize = std::min(MAX_READY_QUEUE_SIZE, maxReadySize);
    for (int index; index < executorCount_; index++) {
        std::deque<ExecutorJob*>& que = readyQueue_[index];
        while(que.size() < maxReadySize) {
            que.push_back(nullptr);
            jobVacancies.push_back(vacancy(&(que.back()), index));
        }
    }
    if (jobVacancies.empty()) {
        for (int index = 0; index < executorCount_; index++) {
            readyQueueLock_[index]->unlock();
        }
        return 0;
    }
    int selected = 0, index = 0;
    while (selected < jobVacancies.size()) {
        if (candidateQueue_.empty()) {
            readyQueue_[jobVacancies[selected++].second].pop_back();
        } else {
            *(jobVacancies[selected].first) = candidateQueue_.front();
            wakeupExecutorID->insert(jobVacancies[selected++].second);
            readyJobCount_++;
            candidateQueue_.pop();
        }
    }
#ifdef DEBUG_JOB
    std::cout << "After select:  ";
    for (auto que : readyQueue_) {
        std::cout << que.size() << " ";
    }
    std::cout << std::endl;
#endif
    for (int index = 0; index < executorCount_; index++) {
        readyQueueLock_[index]->unlock();
    }
    TEST("rc");
    return 0;
}

ResourcePool::~ResourcePool() {
    for (auto groupMapIterator : groupMap_) {
        if (groupMapIterator.second != nullptr) {
            delete groupMapIterator.second;
        }
    }
    for (auto layerPtr : layers_) {
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
            que.pop_front();
        }
    }
    while (!candidateQueue_.empty()) {
        ExecutorJob* jobPtr = candidateQueue_.front();
        if (jobPtr != nullptr) {
            delete jobPtr;
        }
        candidateQueue_.pop();
    }
    for (std::mutex* lock : readyQueueLock_) {
        delete lock;
    }
    for (Semaphore* semaphore : sependencySignals_) {
        delete semaphore;
    }
}

} // namespace condition_assign
