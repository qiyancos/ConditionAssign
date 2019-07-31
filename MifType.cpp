#include "MifType.h"
#include "ConditionAssign.h"

#include <unistd.h>

namespace condition_assign {

MifLayer::MifLayer(AccessType type) : type_(type) {
    ready_.init(0, Semaphore::OnceForAll);
}

MifLayer::~MifLayer() {
    for (auto itemIterator : itemCache_) {
        delete itemIterator.second;
    }
}

int MifLayer::newMifItem(const int index, MifItem** newItemPtr,
        MifLayer* targetLayer) {
    ready_.wait();
    CHECK_ARGS(index > -1 && index < mifSize_,
            "Mif item index[%d] out of bound.", index);
    std::lock_guard<std::mutex> cacheGuard(itemCacheLock_);
    auto itemIterator = itemCache_.find(index);
    if (itemIterator == itemCache_.end()){
        *newItemPtr = new MifItem(index, this, targetLayer);
        itemCache_[index] = *newItemPtr;
    } else {
        *newItemPtr = itemIterator->second;
    }
    return 0;
}

MifLayerReadWrite::MifLayerReadWrite() : MifLayer(MifLayer::Write) {}

int MifLayerReadWrite::open(const std::string& layerPath,
        MifLayer* input = nullptr) {
    CHECK_ARGS(layerPath.length() != 0, "Can not open with empty layer path.");
    CHECK_ARGS(layerPath_.length() == 0,
            "Trying to reopen mif layer with old path \"%s\" %s \"%s\".",
            layerPath_.c_str(), "and new path", layerPath.c_str());
    if (access(layerPath.c_str(), W_OK) < 0) {
        CHECK_ARGS(access(layerPath.c_str(), 0) < 0,
                "File \"%s\" exists but not writable.", layerPath.c_str());
        CHECK_ARGS(input != nullptr,
                "Can not find input layer for header copy");
        layerPath_ = layerPath;
        input->ready_.wait();
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        mif_.header = input->mif_.header;
        mif_.header.coordsys = input->mif_.header.COORDSYS_LL;
        ready_.signalAll();
        newLayer_ = true;
        return 0;
    }
    layerPath_ = layerPath;
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while openning mif layer \"%s\".",
            layerPath.c_str());
    mifSize_ = mif_.mid.size();
    ready_.signalAll();
    return 0;
}

int MifLayerReadWrite::save(std::string layerPath = "") {
    std::string* savePath;
    if (layerPath.length() == 0) {
        CHECK_ARGS(layerPath_.length() != 0,
                "Can not save unopened mif layer.")
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

int MifLayerReadWrite::newItemWithTag(MifLayer* input, const int index,
        const std::string& tagName, const std::string& val) {
    ready_.wait();
    mifLock_.lock();
    mif_.mid.push_back(input->mif_.mid[index]);
    mif_.data.geo_vec.push_back(input->mif_.data.geo_vec[index] == NULL ?
            NULL : input->mif_.data.geo_vec[index]->clone());
    mifSize_++;
    mifLock_.unlock();
    CHECK_RET(assignWithTag(tagName, mifSize_ - 1, val),
            "Failed to assign value to tag \"%s\" in mif item [%d].",
            tagName.c_str(), mifSize_ - 1);
    return 0;
}

int MifLayerReadWrite::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    ready_.wait();
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index of tag \"%s\" for write.",
            tagName.c_str());
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    mif_.mid[index][colID] = val;
    return 0;
}

int MifLayerReadWrite::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    ready_.wait();
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        std::string tagStringVal;
        if (getTagVal(tagName, 0, &tagStringVal) < 0) {
            tagTypeCache_[tagName] = syntax::New;
            *type = syntax::New;
            return 0;
        } else {
            *type = syntax::getDataType(tagStringVal);
            tagTypeCache_[tagName] = *type;
            return 0;
        }
    }
}

int MifLayerReadWrite::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    ready_.wait();
    int colID;
    if (getTagColID(tagName, &colID, MifLayer::Write) < 0) {
        return -1;
    }
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    *val = mif_.mid[index][colID];
    return 0;
}

int MifLayerReadWrite::getGeometry(wsl::Geometry** val, const int index) {
    ready_.wait();
    CHECK_ARGS(index < mifSize_ && index >= 0,
            "Index[%d] out of bound.", index);
    std::lock_guard<std::mutex> mifGuard(mifLock_);
    *val = mif_.data.geo_vec[index];
    return 0;
}

int MifLayerReadWrite::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
    ready_.wait();
    std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        *colID = colCacheIterator->second;
        return 0;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        std::lock_guard<std::mutex> mifGuard(mifLock_);
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    *colID = colIterator->second));
            return 0;
        } else {
            if (accessType == Read) {
                return -1;
            } else {
                mif_.add_column(tagName, "Char(64)");
                colIterator = mif_.header.col_name_map.find(lowerTagName);
                CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                        "Create new column \"%s\" failed!", tagName.c_str());
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        *colID = colIterator->second));
                return 0;
            }
        }
    }
}

MifLayerReadOnly::MifLayerReadOnly() : MifLayer(MifLayer::Read) {}

int MifLayerReadOnly::open(const std::string& layerPath,
        MifLayer* input = nullptr) {
    CHECK_ARGS(layerPath.length() != 0,
            "Can not open with empty layer path.");
    CHECK_ARGS(layerPath_.length() == 0,
            "Trying to reopen mif layer with old path \"%s\" %s \"%s\".",
            layerPath_.c_str(), "and new path", layerPath.c_str());
    CHECK_RET(access(layerPath.c_str(), R_OK),
            "File \"%s\" exists but not readable.", layerPath.c_str());
    layerPath_ = layerPath;
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while openning mif layer \"%s\".",
            layerPath.c_str());
    mifSize_ = mif_.mid.size();
    ready_.signalAll();
    return 0;
}

int MifLayerReadOnly::save(std::string layerPath = "") {
    CHECK_RET(-1, "Read-only layer do not need to be saved.");
}

int MifLayerReadOnly::newItemWithTag(MifLayer* input, const int index,
        const std::string& tagName, const std::string& val) {
    CHECK_RET(-1, "Read-only layer do not support adding new item.");
}

int MifLayerReadOnly::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_RET(-1, "Read-only layer do not support assign option.");
}

int MifLayerReadOnly::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    ready_.wait();
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        std::string tagStringVal;
        if (getTagVal(tagName, 0, &tagStringVal) < 0) {
            tagTypeCache_[tagName] = syntax::New;
            *type = syntax::New;
            return 0;
        } else {
            *type = syntax::getDataType(tagStringVal);
            tagTypeCache_[tagName] = *type;
            return 0;
        }
    }
}

int MifLayerReadOnly::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    ready_.wait();
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index of tag \"%s\".", tagName.c_str());
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    *val = mif_.mid[index][colID];
    return 0;
}

int MifLayerReadOnly::getGeometry(wsl::Geometry** val, const int index) {
    ready_.wait();
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    *val = mif_.data.geo_vec[index];
    return 0;
}

int MifLayerReadOnly::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
    ready_.wait();
    std::lock_guard<std::mutex> tagColCacheGuard(tagColCacheLock_);
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        *colID = colCacheIterator->second;
        return 0;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    *colID = colIterator->second));
            return 0;
        } else {
            return -1;
        }
    }
}

MifItem::MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer) :
        srcLayer_(srcLayer), targetLayer_(targetLayer), index_(index) {}

int MifItem::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_ARGS(targetLayer_ != nullptr,
            "Can not assign value to tag in an empty target layer.");
    if (targetLayer_->isNew()) {
        CHECK_RET(targetLayer_->newItemWithTag(srcLayer_, index_, tagName,
                val), "Failed to assign value to tag \"%s\" %s",
                tagName.c_str(), "mif layer in a new mif item.");
    } else {
        CHECK_RET(targetLayer_->assignWithTag(tagName, index_, val),
                "Failed to assign value to tag \"%s\".", tagName.c_str());
    }
    /*
    // 我们不跟踪的新的Tag，因为并行的机制决定了并不能使用他们
    tagStringCacheLock_.lock();
    tagStringCache_[tagName] = val;
    tagStringCacheLock_.unlock();
    */
    return 0;
}

int MifItem::getTagVal(const std::string& tagName, std::string* val) {
    std::lock_guard<std::mutex> tagStringCacheGuard(tagStringCacheLock_);
    auto cacheIterator = tagStringCache_.find(tagName);
    if (cacheIterator != tagStringCache_.end()) {
        *val = cacheIterator->second;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        tagStringCache_[tagName] = *val;
        return 0;
    }
}

int MifItem::getTagVal(const std::string& tagName, double* val) {
    std::lock_guard<std::mutex> tagNumberCacheGuard(tagNumberCacheLock_);
    auto cacheNumberIterator = tagNumberCache_.find(tagName);
    if (cacheNumberIterator != tagNumberCache_.end()) {
        *val = cacheNumberIterator->second;
        return 0;
    } else {
        std::string tagVal;
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Trying to get tag value \"%s\" as a number.", tagVal.c_str());
        tagNumberCache_[tagName] = *val;
        return 0;
    }
}

int MifItem::getGeometry(wsl::Geometry** val) {
    if (geometry_ != nullptr) {
        *val = geometry_;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getGeometry(val, index_),
                "Failed to get geometry from mif layer in item[%d].", index_);
        geometry_ = *val;
        return 0;
    }
}

} // namepsace condition_assign
