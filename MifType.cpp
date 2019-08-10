#include "MifType.h"
#include "ConditionAssign.h"

#include <unistd.h>

namespace condition_assign {

MifLayer::MifLayer(const std::string& layerPath,
        MifLayer* copySrcLayer = nullptr) : layerPath_(layerPath),
        copySrcLayer_(copySrcLayer) {
    ready_.init(0, Semaphore::OnceForAll);
}

MifLayer::~MifLayer() {
    for (auto itemIterator : itemCache_) {
        delete itemIterator.second;
    }
}

int MifLayer::size() {
    return mifSize_;
}

bool MifLayer::withItemCache() {
    return withItemCache_;
}

int MifLayer::setGeoType(const std::string& typeStr) {
    if (typeStr == "POINT" || typeStr == "NULL") {
        geoType_ = Group::Point;
    } else if (typeStr == "LINE") {
        geoType_ = Group::Line;
    } else if (typeStr == "AREA") {
        geoType_ = Group::Area;
    } else {
        CHECK_RET(-1, "Unknown geometry type \"%s\".", typeStr.c_str());
    }
    return 0;
}

Group::Type MifLayer::getGeoType() {
    return geoType_;
}

int MifLayer::newMifItem(const int index, MifLayer* targetLayer,
        MifItem** newItemPtr) {
    CHECK_ARGS(index > -1 && index < mifSize_,
            "Mif item index[%d] out of bound.", index);
#ifdef USE_MIFITEM_CACHE
    CHECK_ARGS(withCache_,
            "Can not get new mif item from miflayer without item cache.");
    *newItemPtr = new MifItem(index, this, targetLayer,
            &(itemInfoCache_[index]));
#else
    *newItemPtr = new MifItem(index, this, targetlayer);
#endif
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
        ready_.signalAll();
    }
    return 0;
}

int MifLayerNew::copyLoad() {
    if (copySrcLayer_) {
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        tagColCache_ = copySrcLayer_->tagColCache_;
        tagTypeCahce_ = copySrcLayer_->tagTypeCache_;
        mif_.header = copySrcLayer_->mif_.header;
        mif_.header.coordsys = copySrcLayer_->mif_.header.COORDSYS_LL;
        ready_.signalAll();
    }
    return 0;
}

int MifLayerNew::save(std::string layerPath = "") {
    std::string* savePath;
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

int MifLayerNew::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    mif_.mid.push_back(copySrcLayer_->mif_.mid[index]);
    mif_.data.geo_vec.push_back(copySrcLayer_->mif_.data.geo_vec[index] ==
            NULL ? NULL : copySrcLayer_->mif_.data.geo_vec[index]->clone());
    mif_.mid[mifSize][colID] = val;
    mifSize_++;
    return 0;
}

int MifLayerNew::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        // 查找对应的colID
        int colID;
        std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
        auto colCacheIterator = tagColCache_.find(tagName);
        if (colCacheIterator != tagColCache_.end()) {
            colID = colCacheIterator->second;
        } else {
            std::string lowerTagName = htk::toLower(tagName);
            std::lock_guard<std::mutex> mifGuard(mifLock_);
            auto colIterator = mif_.header.col_name_map.find(lowerTagName);
            if (colIterator != mif_.header.col_name_map.end()) {
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        colID = colIterator->second));
            } else {
                // 添加新的column
                mif_.add_column(tagName, "char(64)");
                colIterator = mif_.header.col_name_map.find(lowerTagName);
                CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                        "Create new column \"%s\" failed!", tagName.c_str());
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        colID = colIterator->second));
                tagTypeCache_[tagName] = syntax::New;
                *type = syntax::New;
                return 0;
            }
        }
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        std::string tagStringVal = copySrcLayer_->mif_.mid[0][colID];
        *type = syntax::getDataType(tagStringVal);
        tagTypeCache_[tagName] = *type;
        return 0;
    }
}

int MifLayerNew::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    *val = mif_.mid[index][colID];
    return 0;
}

int MifLayerNew::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS(index < mifSize_ && index >= 0,
            "Index[%d] out of bound.", index);
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    *val = mif_.data.geo_vec[index];
    return 0;
}

MifLayerNormal::MifLayerNormal(const bool withCache) : MifLayer(withCache) {}

int MifLayerNormal::open() {
    if (!copySrcLayer) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not open mif layer with uninitiated layer path.");
        CHECK_RET(access(layerPath.c_str(), R_OK),
                "File \"%s\" exists but not readable.", layerPath.c_str());
        CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
                "Error occurred while openning mif layer \"%s\".",
                layerPath.c_str());
        mifSize_ = mif_.mid.size();
#ifdef USE_MIFITEM_CACHE
        if (withCache_) {
            itemInfoCache_.resize(mifSize);
        }
#endif
        ready_.signalAll();
    } else {
        copySrcLayer_->ready_.wait();
        mif_ = copySrcLayer->mif_;
        mifSize_ = copySrcLayer_->mifSize_;
        tagColCache_ = copySrcLayer_->tagColCache_;
        tagTypeCache_ = copySrcLayer->tagTypeCache_;
        ready_.signalAll();
    }
    return 0;
}

int MiLayerNormal::copyLoad() {
    // Normal类型的layer不应该执行copyLoad
    CHECK_RET(-1, "Normal mif layer should never running copy load function.");
}

int MifLayerNormal::save(std::string layerPath = "") {
    std::string* savePath;
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

int MifLayerNormal::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    int colID;
    auto colCacheIterator = tagColCache_.find(tagName);
    CHECK_ARGS(colCacheIterator != tagColCache_.end(),
            "Failed to get column id of tag \"%s\".", tagName.c_str());
    colID = colCacheIterator->second;
    mif_.mid[mifSize][colID] = val;
    mifSize_++;
    return 0;
}

int MifLayerNormal::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        // 查找对应的colID
        int colID;
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
        auto colCacheIterator = tagColCache_.find(tagName);
        if (colCacheIterator != tagColCache_.end()) {
            colID = colCacheIterator->second;
        } else {
            std::string lowerTagName = htk::toLower(tagName);
            auto colIterator = mif_.header.col_name_map.find(lowerTagName);
            if (colIterator != mif_.header.col_name_map.end()) {
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        colID = colIterator->second));
            } else {
                // 添加新的column
                mif_.add_column(tagName, "char(64)");
                colIterator = mif_.header.col_name_map.find(lowerTagName);
                CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                        "Create new column \"%s\" failed!", tagName.c_str());
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        colID = colIterator->second));
                tagTypeCache_[tagName] = syntax::New;
                *type = syntax::New;
                return 0;
            }
        }
        std::string tagStringVal = mif_.mid[0][colID];
        *type = syntax::getDataType(tagStringVal);
        tagTypeCache_[tagName] = *type;
        return 0;
    }
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

int MifLayerNormal::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    *val = mif_.data.geo_vec[index];
    return 0;
}

MifItem::MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer) :
        index_(index), srcLayer_(srcLayer), targetLayer_(targetLayer) {
#ifdef USE_MIFITEM_CACHE
    info_ = new MifLayer::ItemInfo();
    newInfo_ = true;
#endif
}

MifItem::MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer,
        MifLayer::ItemInfo* info) : index_(index), srcLayer_(srcLayer),
        targetLayer_(targetLayer), info_(info) {}

MifItem::~MifItem() {
    if (newInfo_) {
        delete info_;
    }
}

int MifItem::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_ARGS(targetLayer_ != nullptr,
            "Can not assign value to tag in an empty target layer.");
    CHECK_RET(targetLayer_->assignWithTag(tagName, index_, val),
            "Failed to assign value to tag \"%s\".", tagName.c_str());
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(info_->tagStringCacheLock_);
    info->tagStringCache_[tagName] = val;
#endif
    return 0;
}

int MifItem::getTagVal(const std::string& tagName, std::string* val) {
#ifdef USE_MIFITEM_CACHE
    std::lock_guard<std::mutex> cacheGuard(info_->tagStringCacheLock_);
    auto cacheIterator = info_->tagStringCache_.find(tagName);
    if (cacheIterator != info_->tagStringCache_.end()) {
        *val = cacheIterator->second;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        info_->tagStringCache_[tagName] = *val;
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
    std::lock_guard<std::mutex> cacheGuard(info->tagNumberCacheLock_);
    auto cacheNumberIterator = info_->tagNumberCache_.find(tagName);
    if (cacheNumberIterator != info_->tagNumberCache_.end()) {
        *val = info_->cacheNumberIterator->second;
        return 0;
    } else {
        std::string tagVal;
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Trying to get tag value \"%s\" as a number.", tagVal.c_str());
        info_->tagNumberCache_[tagName] = *val;
        return 0;
    }
#else
    std::string tagVal;
    CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
            "Failed to get value of tag \"%s\" from mif layer.",
            tagName.c_str());
    *val = atof(tagVal.c_str());
    return 0;
#endif
}

int MifItem::getGeometry(wsl::Geometry** val) {
    if (info_->geometry_ != nullptr) {
        *val = geometry_;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getGeometry(val, index_),
                "Failed to get geometry from mif layer in item[%d].", index_);
        info_->geometry_ = *val;
        return 0;
    }
}

} // namepsace condition_assign
