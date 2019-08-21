#include "MifType.h"
#include "ConditionAssign.h"
#include "ExecutorPool.h"

#include <unistd.h>

namespace condition_assign {

double globalDouble = 0.0f;

MifLayer::ItemInfo::ItemInfo() {
#ifdef USE_MIFITEM_CACHE
    tagStringCacheLock_ = new std::mutex();
    tagNumberCacheLock_ = new std::mutex();
#endif
    dynamicGroupCacheLock_ = new std::mutex();
    geometryLock_ = new std::mutex();
}

MifLayer::ItemInfo::~ItemInfo() {
#ifdef USE_MIFITEM_CACHE
    delete tagStringCacheLock_;
    delete tagNumberCacheLock_;
#endif
    delete dynamicGroupCacheLock_;
    delete geometryLock_;
}

MifLayer::MifLayer(const std::string& layerPath,
        MifLayer* copySrcLayer) : layerPath_(layerPath),
        copySrcLayer_(copySrcLayer) {
    ready_.init(0, Semaphore::OnceForAll);
}

int MifLayer::size() {
    return mifSize_;
}

Group::Type MifLayer::getGeoType() {
    return geoType_;
}

int MifLayer::newMifItem(const int index, MifLayer* targetLayer,
        MifItem** newItemPtr) {
    CHECK_ARGS(index > -1 && index < mifSize_,
            "Mif item index[%d] out of bound.", index);
    *newItemPtr = new MifItem(index, this, targetLayer,
            &(itemInfoCache_[index]));
    return 0;
}

MifLayerNew::MifLayerNew(const std::string& layerPath,
        MifLayer* copySrcLayer) : MifLayer(layerPath, copySrcLayer) {}

int MifLayerNew::open() {
    if (ExecutorPool::runParallel_) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not open mif layer with uninitiated layer path.");
        CHECK_ARGS(copySrcLayer_, "Can not find input layer for header copy");
        copySrcLayer_->ready_.wait();
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        mif_.header = copySrcLayer_->mif_.header;
        mif_.header.coordsys = copySrcLayer_->mif_.header.COORDSYS_LL;
        geoType_ = Group::getGeometryType(copySrcLayer_->mif_.data.geo_vec[0]);
        ready_.signalAll();
    }
    return 0;
}

int MifLayerNew::copyLoad() {
    if (copySrcLayer_) {
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        tagColCache_ = copySrcLayer_->tagColCache_;
        tagTypeCache_ = copySrcLayer_->tagTypeCache_;
        mif_.header = copySrcLayer_->mif_.header;
        mif_.header.coordsys = copySrcLayer_->mif_.header.COORDSYS_LL;
        geoType_ = Group::getGeometryType(copySrcLayer_->mif_.data.geo_vec[0]);
        ready_.signalAll();
    }
    return 0;
}

int MifLayerNew::save(const std::string layerPath) {
    const std::string* savePath;
    if (layerPath.length() == 0) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not save unopened mif layer.");
        savePath = &layerPath_;
    } else {
        savePath = &layerPath;
    }
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    CHECK_RET(wgt::wsbl_to_mif(mif_, *savePath),
            "Error occurred while saving mif layer as \"%s\".",
            savePath->c_str());
    return 0;
}

int MifLayerNew::assignWithNumber(const std::string& tagName,
        MifLayer* srcLayer, const int index, const double& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    mif_.mid.push_back(copySrcLayer_->mif_.mid[index]);
    mif_.data.geo_vec.push_back(copySrcLayer_->mif_.data.geo_vec[index] ==
            NULL ? NULL : copySrcLayer_->mif_.data.geo_vec[index]->clone());
    if (tagName != "X" && tagName != "Y") {
        int colID;
        auto colCacheIterator = tagColCache_.find(tagName);
        CHECK_ARGS(colCacheIterator != tagColCache_.end(),
                "Failed to get column id of tag \"%s\".", tagName.c_str());
        colID = colCacheIterator->second;
        int intVal;
        std::string leftValString, valString;
        if (syntax::canRoundToInt(val, &intVal)) {
            valString = std::to_string(intVal);
            CHECK_RET(copySrcLayer_->getTagVal(tagName, index, &leftValString),
                    "Failed to get string value from %s \"%s\".",
                    "copySrcLayer for tag", tagName.c_str());
            int lackZeros = leftValString.length() - valString.length();
            std::string prefix = "";
            if (lackZeros > 0) {
                prefix.append(lackZeros, '0');
            }
            valString = prefix + valString;
        } else {
            valString = std::to_string(val);
        }
        mif_.mid[mifSize_][colID] = valString;
    } else {
        if (tagName == "X") {
            mif_.data.geo_vec[mifSize_]->at(0).at(0).setx(val);
        } else {
            mif_.data.geo_vec[mifSize_]->at(0).at(0).sety(val);
        }
    }
    mifSize_++;
    return 0;
}

int MifLayerNew::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    CHECK_ARGS(tagName != "X" && tagName != "Y",
            "Calling wrong function for coordination exchange.");
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    mif_.mid.push_back(copySrcLayer_->mif_.mid[index]);
    mif_.data.geo_vec.push_back(copySrcLayer_->mif_.data.geo_vec[index] ==
            NULL ? NULL : copySrcLayer_->mif_.data.geo_vec[index]->clone());
    mif_.mid[mifSize_][colID] = val;
    mifSize_++;
    return 0;
}

int MifLayerNew::getTagType(const std::string& tagName,
        syntax::DataType* type, bool isAssign) {
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    if (tagName == "X" || tagName == "Y") {
        *type = syntax::Number;
        return 0;
    }
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        // 查找对应的colID
        int colID;
        int result;
        CHECK_RET(result = checkAddTag(tagName, &colID),
                "Failed to add new column.");
        if (result > 0) {
            if (ExecutorPool::runParallel_) {
                tagTypeCache_[tagName] = syntax::New;
            }
            *type = syntax::New;
            return 0;
        }
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        std::string tagStringVal = copySrcLayer_->mif_.mid[0][colID];
        *type = syntax::getDataType(tagStringVal);
        tagTypeCache_[tagName] = *type;
        return 0;
    }
}

int MifLayerNew::checkAddTag(const std::string& tagName,
        int* colID, bool isAssign) {
    int index;
    std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        index = colCacheIterator->second;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                index = colIterator->second));
        } else {
            // 添加新的column
            mif_.add_column(tagName, "char(64)");
            colIterator = mif_.header.col_name_map.find(lowerTagName);
            CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                "Create new column \"%s\" failed!", tagName.c_str());
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    index = colIterator->second));
            if (colID) {
                *colID = index;
            }
            return 1;
        }
    }
    if (colID) {
        *colID = index;
    }
    return 0;
}

int MifLayerNew::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    CHECK_RET(-1, "This function should never be called.");
    return 0;
}

int MifLayerNew::getTagVal(const std::string& tagName,
        const int index, double* val) {
    CHECK_RET(-1, "This function should never be called.");
    return 0;
}

int MifLayerNew::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_RET(-1, "This function should never be called.");
    return 0;
}

MifLayerNormal::MifLayerNormal(const std::string& layerPath,
        MifLayer* copySrcLayer) :
        MifLayer(layerPath, copySrcLayer) {}

int MifLayerNormal::open() {
    if (!copySrcLayer_) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not open mif layer with uninitiated layer path.");
        CHECK_RET(access(layerPath_.c_str(), R_OK),
                "File \"%s\" exists but not readable.", layerPath_.c_str());
        CHECK_RET(wgt::mif_to_wsbl(layerPath_, mif_),
                "Error occurred while openning mif layer \"%s\".",
                layerPath_.c_str());
        mifSize_ = mif_.mid.size();
        geoType_ = Group::getGeometryType(mif_.data.geo_vec[0]);
        itemInfoCache_.resize(mifSize_);
        ready_.signalAll();
    } else {
        copySrcLayer_->ready_.wait();
        mif_ = copySrcLayer_->mif_;
        mifSize_ = copySrcLayer_->mifSize_;
        geoType_ = Group::getGeometryType(mif_.data.geo_vec[0]);
        tagColCache_ = copySrcLayer_->tagColCache_;
        tagTypeCache_ = copySrcLayer_->tagTypeCache_;
        ready_.signalAll();
    }
    return 0;
}

int MifLayerNormal::copyLoad() {
    // Normal类型的layer不应该执行copyLoad
    CHECK_RET(-1, "Normal mif layer should never running copy load function.");
    return 0;
}

int MifLayerNormal::save(const std::string layerPath) {
    const std::string* savePath;
    if (layerPath.length() == 0) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not save unopened mif layer.");
        savePath = &layerPath_;
    } else {
        savePath = &layerPath;
    }
    CHECK_RET(wgt::wsbl_to_mif(mif_, *savePath),
            "Error occurred while saving mif layer as \"%s\".",
            savePath->c_str());
    return 0;
}

int MifLayerNormal::assignWithNumber(const std::string& tagName,
        MifLayer* srcLayer, const int index, const double& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    if (tagName != "X" && tagName != "Y") {
        int colID;
        auto colCacheIterator = tagColCache_.find(tagName);
        CHECK_ARGS(colCacheIterator != tagColCache_.end(),
                "Failed to get column id of tag \"%s\".", tagName.c_str());
        colID = colCacheIterator->second;
        int intVal;
        std::string leftValString, valString;
        if (syntax::canRoundToInt(val, &intVal)) {
            valString = std::to_string(intVal);
            CHECK_RET(srcLayer->getTagVal(tagName, index, &leftValString),
                    "Failed to get string value from %s \"%s\".",
                    "copySrcLayer for tag", tagName.c_str());
            int lackZeros = leftValString.length() - valString.length();
            std::string prefix = "";
            if (lackZeros > 0) {
                prefix.append(lackZeros, '0');
            }
            valString = prefix + valString;
        } else {
            valString = std::to_string(val);
        }
        mif_.mid[index][colID] = valString;
    } else {
        if (tagName == "X") {
            mif_.data.geo_vec[index]->at(0).at(0).setx(val);
        } else {
            mif_.data.geo_vec[index]->at(0).at(0).sety(val);
        }
    }
    return 0;
}

int MifLayerNormal::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    CHECK_ARGS(tagName != "X" && tagName != "Y",
            "Calling wrong function for coordination exchange.");
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    mif_.mid[index][colID] = val;
    return 0;
}

int MifLayerNormal::getTagType(const std::string& tagName,
        syntax::DataType* type, bool isAssign) {
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    if (tagName == "X" || tagName == "Y") {
        *type = syntax::Number;
        return 0;
    }
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        // 查找对应的colID
        int colID;
        int result;
        CHECK_RET(result = checkAddTag(tagName, &colID, isAssign),
                "Failed to add new column or tag not exist.");
        if (result > 0) {
            if (ExecutorPool::runParallel_) {
                tagTypeCache_[tagName] = syntax::New;
            }
            *type = syntax::New;
            return 0;
        }
        std::string tagStringVal = mif_.mid[0][colID];
        *type = syntax::getDataType(tagStringVal);
        tagTypeCache_[tagName] = *type;
        return 0;
    }
}

int MifLayerNormal::checkAddTag(const std::string& tagName,
        int* colID, bool isAssign) {
    int index;
    std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        index = colCacheIterator->second;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                index = colIterator->second));
        } else if (isAssign) {
            // 添加新的column
            mif_.add_column(tagName, "char(64)");
            colIterator = mif_.header.col_name_map.find(lowerTagName);
            CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                "Create new column \"%s\" failed!", tagName.c_str());
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    index = colIterator->second));
            if (colID) {
                *colID = index;
            }
            return 1;
        } else {
            return 1;
        }
    }
    if (colID) {
        *colID = index;
    }
    return 0;
}

int MifLayerNormal::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    *val = mif_.mid[index][colID];
    return 0;
}

int MifLayerNormal::getTagVal(const std::string& tagName,
        const int index, double* val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    CHECK_ARGS(tagName == "X" || tagName == "Y",
            "Function only support for coordination calculation.");
    if (tagName == "X") {
        *val = mif_.data.geo_vec[index]->at(0).at(0).x();
    } else {
        *val = mif_.data.geo_vec[index]->at(0).at(0).y();
    }
    return 0;
}

int MifLayerNormal::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    *val = mif_.data.geo_vec[index];
    return 0;
}

MifItem::MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer,
        MifLayer::ItemInfo* info) :  srcLayer_(srcLayer),
        targetLayer_(targetLayer), index_(index), info_(info) {}

int MifItem::assignWithNumber(const std::string& tagName,
        const double& val) {
    CHECK_ARGS(targetLayer_ != nullptr,
            "Can not assign number to tag in an empty target layer.");
    CHECK_RET(targetLayer_->assignWithNumber(tagName, srcLayer_, index_, val),
            "Failed to assign number to tag \"%s\".", tagName.c_str());
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(*(info_->tagNumberCacheLock_));
    info_->tagNumberCache[tagName] = val;
#endif
    return 0;
}

int MifItem::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_ARGS(targetLayer_ != nullptr,
            "Can not assign value to tag in an empty target layer.");
    CHECK_RET(targetLayer_->assignWithTag(tagName, index_, val),
            "Failed to assign value to tag \"%s\".", tagName.c_str());
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(*(info_->tagStringCacheLock_));
    info_->tagStringCache[tagName] = val;
#endif
    return 0;
}

int MifItem::getTagVal(const std::string& tagName, std::string* val) {
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(*(info_->tagStringCacheLock_));
    auto cacheIterator = info_->tagStringCache.find(tagName);
    if (cacheIterator != info_->tagStringCache.end()) {
        *val = cacheIterator->second;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        info_->tagStringCache[tagName] = *val;
        return 0;
    }
#else
    CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
            "Failed to get value of tag \"%s\" from mif layer.",
            tagName.c_str());
    return 0;
#endif
}

int MifItem::getTagVal(const std::string& tagName, double* val) {
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(*(info_->tagNumberCacheLock_));
    auto cacheNumberIterator = info_->tagNumberCache.find(tagName);
    if (cacheNumberIterator != info_->tagNumberCache.end()) {
        *val = cacheNumberIterator->second;
        return 0;
    } else if (tagName == "X" || tagName == "Y") {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        info_->tagNumberCache[tagName] = *val;
        return 0;
    } else {
        std::string tagVal;
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Trying to get tag value \"%s\" as a number.", tagVal.c_str());
        info_->tagNumberCache[tagName] = *val;
        return 0;
    }
#else
    if (tagName == "X" || tagName == "Y") {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
    } else {
        std::string tagVal;
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Tag \"%s\" value \"%s\" can not be convert to a number.",
                tagName.c_str(), tagVal.c_str())
    }
    return 0;
#endif
}

int MifItem::getGeometry(wsl::Geometry** val) {
    if (info_->geometry_ != nullptr) {
        *val = info_->geometry_;
        return 0;
    } else {
        std::lock_guard<std::mutex> geoGuard(*(info_->geometryLock_));
        CHECK_RET(srcLayer_->getGeometry(val, index_),
                "Failed to get geometry from mif layer in item[%d].", index_);
        info_->geometry_ = *val;
        // 强制调用_cal_函数
        globalDouble = (*val)->mbr().ll.x();
        return 0;
    }
}

int MifItem::findBuildDynamicGroup(Group** groupPtr, int64_t dynamicGroupKey,
        Group* itemGroup) {
    std::lock_guard<std::mutex> cacheGuard(*(info_->dynamicGroupCacheLock_));
    auto mapIterator = info_->dynamicGroupCache_.find(dynamicGroupKey);
    if (mapIterator != info_->dynamicGroupCache_.end()) {
        *groupPtr = mapIterator->second;
    } else {
        CHECK_RET(itemGroup->buildDynamicGroup(groupPtr, this),             
                "Failed to build dynamic group.");
        info_->dynamicGroupCache_[dynamicGroupKey] = *groupPtr;
    }
    return 0;
}

int MifItem::findInsertProcessResult(bool** resultPtr, int64_t processKey) {
    auto mapIterator = processResultCache_.find(processKey);
    if (mapIterator != processResultCache_.end()) {
        **resultPtr = mapIterator->second;
        return 1;
    } else {
        if (*resultPtr) {
            delete *resultPtr;
        }
        processResultCache_[processKey] = false;
        *resultPtr = &(processResultCache_[processKey]);
        return 0;
    }
}

} // namepsace condition_assign
