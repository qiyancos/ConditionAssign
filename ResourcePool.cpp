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
        dependencySignals_.push_back(new Semaphore(0, Semaphore::OnceForAll));
    }
    // 初始化串行场景依赖信号量和配置文件信息
    dependencySignals_[executorCount_ - 1]->signalAll();
    CHECK_RET(initConfigGroup(params, layerList),
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
        if (inputSize_ > 1) {
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
    if (inputSize > 1) {
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
    }
    auto needCacheEnd = needCacheLayers.end();
    index = 0;
    for (auto layerIterator : layerList) {
        layers_.push_back(new MifLayerNormal(needCacheLayers.find(
                layerIterator.first) != needCacheEnd));
        // 设置输入层的地理类型
        if (layerIterator.second.inputIndex != -1) {
            CHECK_RET(layers_.back().setGeoType(params.geoTypes[
                    layerIterator.second.inputIndex]),
                    "Failed to set geometry type for layer \"%s\".",
                    layerIterator.first);
        }
        layerList.second.layerID = index;
        if (layerIterator.second.pluginIndex != -1) {
            pluginLayersMap_[layerIterator.first] = index++;
        } else {
            portLayersMap_[layerIterator.first] = index++;
        }
    }
    return 0;
}

int ResourcePool::initConfigGroup(const ExecutorPool::Params& params,
        const std::map<std::string, ExecutorPool::LayerInfo>* layerList) {
    // 基于输入类型初始化ConfigGroup
    std::vector<std::vector<int>> layerDependency;
#ifdef USE_COMPLICATE_DEPENDENCY
    // 依赖关系的检查
    std::map<std::string, int> lastIndex;
    int inputLastIndex = -1;
    int anotherLastIndex;
    bool isInputLayer;
    if (!needWarnOutputAsInput) {
        for (int i = 0; i < params.outputs.size(); i++) {
            temp = &(params.outputs[i]);
            anotherLastIndex = -1;
            isInputlayer = *temp == *inputLayer;
            if (!isInputLayer && lastIndex.find(*temp) != lastIndex.end()) {
                anotherLastIndex = lastIndex[*temp];
            }
            layerDependency.push_back(std::vector<int>(1,
                    std::max(anotherLastIndex, inputLastIndex)));
            lastIndex[*temp] = i;
            inputLastIndex = isInputLayer ? i : inputLastIndex;
        }
    }
#else
    // 全部设置为强依赖
    if (!needWarnOutputAsInput) {
        for(int i = 0; i < params.outputs.size(); i++) {
            layerDependency.push_back(std::vector<int>(1, i - 1));
        }
    }
#endif
    int srcLayerID = layerList[params.inputs[0]].layerID;
    bool singleInput = params.inputs.size() == 1;
    for (int i = 0; i < targetCount_; i++) {
        if (singleInput) {
            layerDependency[i].second.push_back(srcLayerID);
        } else {
            layerDependency[i].second.push_back(
                    layerList[params.inputs[i]].layerID);
        }
        layerDependency[i].second.push_back(
                layerList[params.outputs[i]].layerID);
    }
    CHECK_RET(configGroup_.init(params.configs.size(), targetCount_,
            dependencySignals_, layerDependency),
            "Failed to init config group");
    return 0;
}

int ResourcePool::getConfigSubGroup(int targetID,
        ConfigSubGroup** subGroupPtr) {
    CHECK_ARGS(targetID < targetCount_, "Invalid target ID [%d].", targetID);
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
        const LayerType layerType, const int inputLayerID,
        const int layerID = -1) {
    CHECK_ARGS(layerID > -1 && layerID < layers_.size(),
            "Layer ID [%d] out of bound.", layerID);
    CHECK_ARGS(inputLayerID > -1 && inputLayerID < layers_.size(),
            "Input layer ID [%d] out of bound.", inputLayerID);
    if (layerType == Output) {
        if (access(layerPath.c_str(), 0) < 0) {
            delete layers_[layerID];
            layers_[layerID] = new MifLayerNew();
        }
        CHECK_RET(layers_[layerID]->open(layerPath, layers_[inputLayerID]),
                "Failed to open output layer \"%s\".", layerPath.c_str());
    } else {
        CHECK_RET(layers_[layerID]->open(layerPath),
                "Failed to open output layer \"%s\".", layerPath.c_str());
    }
    return 0;
}

int ResourcePool::getLayerByName(MifLayer** layerPtr,
        const LayerType layerType, const std::string& layerPath,
        int* targetID = nullptr) {
    if (layerType != Plugin) {
        auto mapIterator = portLayersMap_.find(layerPath);
        if (layerType == Input) {
            CHECK_ARGS(mapIterator != portLayersMap_.end(),
                    "Can not find output layer named as \"%s\".",
                    layerPath.c_str());
        } else {
            CHECK_ARGS(mapIterator != portLayersMap_.end(),
                    "Can not find input layer named as \"%s\".",
                    layerPath.c_str());
        }
        if (targetID) {
            *targetID = mapIterator->second;
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
            if (targetID) {
                *targetID = layerID;
            }
            *layerPtr = layers_[layerID];
        } else {
            if (targetID) {
                *targetID = mapIterator->second;
            }
            *layerPtr = layers_[mapIterator->second];
        }
        return 0;
    }
}

int ResourcePool::getLayerByIndex(MifLayer** layerPtr,
        const LayerType layerType, const int targetID) {
    CHECK_ARGS(targetID > -1 && targetID < layers_.size(),
                "Invalid targetID [%d].", targetID);
    *layerPtr = layers_[targetID];
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
