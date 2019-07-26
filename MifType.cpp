#include "MifType.h"
#include "ConditionAssign.h"

namespace condition_assign {

MifLayer::MifLayer(AccessType type) : type_(type) {}

int MifLayer::newMifItem(const int index, MifItem** newItem,
        MifLayer* targetLayer) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not create new mif item from unopened mif layer.");
    CHECK_ARGS(index > -1 && index < mifSize_,
            "Mif item index[%d] out of bound.", index);
    std::lock_guard<std::mutex> cacheGuard(itemCacheLock_);
    auto itemIterator = itemCache_.find(index);
    if (itemIterator == itemCache_.end()){
        *newItem = new MifItem(this, targetLayer, indexi);
        itemCache_[index] = *newItem;
    } else {
        *newItem = itemIterator->second;
    }
    return 0;
}

MifLayerReadWrite::MifLayerReadWrite() : MifLayer(MifLayer::Write) {}

int MifLayerReadWrite::open(const std::string& layerPath,
        MifLayer* input = nullptr) {
    CHECK_ARGS(layerPath.empty(), "Can not open with empty layer path.");
    CHECK_ARGS(!layerPath_.empty(),
            "Trying to reopen mif layer with old path \"%s\" %s \"%s\".",
            layerPath_.c_str(), "and new path", layerPath_.c_str());
    if (access(layerPath.c_str(), W_OK) < 0) {
        CHECK_ARGS(access(layerPath.c_str(), 0) < 0,
                "File \"%s\" exists but not writable.", layerPath.c_str());
        CHECK_ARGS(input != nullptr,
                "Can not find input layer for header copy");
        layerPath_ = layerPath;
        input->opened_.wait();
        mifLock_.lock();
        mif_.header = input->mif_.header;
        mif_.header.corrdsys = input->mif_.header.COORDSYS_LL;
        opened_.signalAll();
        mifLock_.unlock();
        newLayer_ = true;
        return 0;
    }
    layerPath_ = layerPath;
    mifLock_.lock();
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while openning mif layer \"%s\".",
            layerPath.c_str());
    mifSize_ = mif_.mid.size();
    opened_.signalAll();
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::save(std::string layerPath = "") {
    std::string* savePath;
    if (layerPath.empty()) {
        CHECK_ARGS(!layerPath_.empty(), "Can not save unopened mif layer.")
        savePath = &layerPath_;
    } else {
        savePath = &layerPath_;
    }
    mifLock_.lock();
    CHECK_RET(wgt::wsbl_to_mif(mif_, *savePath),
            "Error occurred while saving mif layer as \"%s\".",
            savePath->c_str());
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::newItemWithTag(MifLayer* input, const int index,
        const std::string& tagName, const std::string val) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    mifLock_->lock();
    mif_.mid.push_back(input->mif_.mid[index]);
    mif_.data.geo_vec.push_back(input->mif_.data.geo_vec[index] == NULL ?
            NULL : input->mif_.data.geo_vec[index]->clone());
    mifSize_++;
    mifLock_->unlock();
    CHECK_RET(assignWithTag(tagName, mifSize_ - 1, val),
            "Failed to assign value to tag \"%s\" in mif item [%d].",
            tagName.c_str(), mifSize - 1);
    return 0;
}

int MifLayerReadWrite::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index of tag \"%s\" for write.",
            tagName.c_str()); 
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    mifLock_->lock();
    mif_->mid[index][colID] = val;
    mifLock_->unlock();
    return 0;
}

int MifLayerReadWrite::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    CHECK_ARGS(false,
            "We did not expect you will get tag type from output layer.");
}

int MifLayerReadWrite::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index of tag \"%s\".", tagName.c_str()); 
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    mifLock_->lock();
    *val = mif_->mid[index][colID];
    mifLock_->unlock();
    return 0;
}

int MifLayerReadWrite::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    CHECK_ARGS(index < mifSize_ && index >= 0,
            "Index[%d] out of bound.", index);
    mifLock_.lock();
    val = mif_.data.geo_vec[index];
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    tagColCacheLock_.lock();
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        colID = colCacheIterator->second;
        tagColCacheLock_.unlock();
        return 0;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        mifLock_.lock();
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    colID = colIterator->second));
            tagColCacheLock_.unlock();
            mifLock_.unlock();
            return 0;
        } else {
            tagColCacheLock_.unlock();
            if (accessType == Read) {
                mifLock_.unlock();
                return -1;
            } else {
                mif_.add_column(tagName, "Char(64)");
                colIterator = mif_.header.col_name_map.find(lowerTagName);
                CHECK_RET(colIterator != mif_.header.col_name_map.end(),
                        "Create new column \"%s\" failed!", tagName.c_str());
                mifLock_.unlock();
                tagColCacheLock_.lock();
                tagColCache_.insert(std::pair<std::string, int>(tagName,
                        colID = colIterator->second));
                tagColCacheLock_.unlock();
                return 0;
            }
        }
    }
}

MifLayerReadOnly::MifLayerReadOnly() : MifLayer(MifLayer::Read) {}

int MifLayerReadOnly::open(const syd::string& layerPath,
        MifLayer* input = nullptr) {
    CHECK_ARGS(layerPath.empty(), "Can not open with empty layer path.");
    CHECK_ARGS(!layerPath_.empty(),
            "Trying to reopen mif layer with old path \"%s\" %s \"%s\".",
            layerPath_, "and new path", layerPath_.c_str());
    CHECK_RET(access(layerPath.c_str(), R_OK),
            "File \"%s\" exists but not readable.", layerPath.c_str());
    layerPath = layerPath;
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while openning mif layer \"%s\".",
            layerPath.c_str());
    opened_.signalAll();
    return 0;
}

int MifLayerReadOnly::save(std::string layerPath = "") {
    CHECK_RET(-1, "Read-only layer do not need to be saved.");
}

int MifLayerReadOnly::newItemWithTag(MifLayer* input, const int index,
        const std::string& tagName, const std::string val) {
    CHECK_RET(-1, "Read-only layer do not support adding new item.");
}

int MifLayerReadOnly::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_RET(-1, "Read-only layer do not support assign option.");
}

int MifLayerReadOnly::getTagType(const std::string& tagName,
        syntax::DataType* type) {
    CHECK_ARGS(mifSize_ > 0, "No available mif item for judging data type.");
    std::lock_guard<std::mutex> typeCacheGuard(tagTypeCacheLock_);
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        return 0;
    } else {
        std::string tagStringVal;
        if (srcLayer_->getTagVal(tagName, 0, &tagStringVal) < 0) {
            tagTypeCache[tagName] = syntax::New;
            *type = syntax::New;
            return 0;
        } else {
            *type = syntax::getDataType(tagVal);
            tagTypeCache[tagName] = *type;
            return 0;
        }
    }
}

int MifLayerReadOnly::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index of tag \"%s\".", tagName.c_str()); 
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    *val = mif_->mid[index][colID];
    return 0;
}

int MifLayerReadOnly::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    CHECK_ARGS((index < mifSize_ && index >= 0),
            "Index[%d] out of bound.", index);
    val = mif_.data.geo_vec[index];
    return 0;
}

int MifLayerReadOnly::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
    CHECK_ARGS(!layerPath_.empty(),
            "Can not operate with unopened mif layer.")
    tagColCacheLock_.lock();
    auto colCacheIterator = tagColCache_.find(tagName);
    if (colCacheIterator != tagColCache_.end()) {
        colID = colCacheIterator->second;
        tagColCacheLock_.unlock();
        return 0;
    } else {
        std::string lowerTagName = htk::toLower(tagName);
        auto colIterator = mif_.header.col_name_map.find(lowerTagName);
        if (colIterator != mif_.header.col_name_map.end()) {
            tagColCache_.insert(std::pair<std::string, int>(tagName,
                    colID = colIterator->second));
            tagColCacheLock_.unlock();
            return 0;
        } else {
            tagColCacheLock_.unlock();
            return -1;
        }
    }
}

MifItem::MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer) :
        srcLayer_(srcLayer), targetLayer_(targetLayer), index_(index),
        sameLayer_(targetLayer != nullptr && *srcLayer == *targetLayer) {}

int MifItem::assignWithTag(const std::string& tagName,
        const std::string& val) {
    if (!sameLayer_) {
        CHECK_RET(targetLayer_->newItemWithTag(srcLayer_, index_, tagName, val),
                "Failed to assign value to tag \"%s\" %s",
                tagName.c_str(), "mif layer in a new mif item.");
    } else {
        CHECK_RET(targetLayer_->assignWithTag(tagName, index_, val),
                "Failed to assign value to tag \"%s\".", tagName.c_str());
    }
    // 我们不跟踪的新的Tag，因为并行的机制决定了并不能使用他们
    /*
    tagStringCacheLock_.lock();
    tagStringCache_[tagName] = val;
    tagStringCacheLock_.unlock();
    */
    return 0;
}

int MifItem::getTagVal(const std::string& tagName, std::string* val) {
    tagStringCacheLock_.lock();
    auto cacheIterator = tagStringCache_.find(tagName);
    if (cacheIterator != tagStringCache_.end()) {
        tagStringCacheLock_.unlock();
        *val = cacheIterator->second;
        return 0;
    } else {
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, val),
                "Failed to get value of tag \"%s\" from mif layer.",
                tagName.c_str());
        tagStringCache_[tagName] = *val;
        tagStringCacheLock_.unlock();
        return 0;
    }
}

int MifItem::getTagVal(const std::string& tagName, double* val) {
    tagNumberCacheLock_.lock();
    auto cacheNumberIterator = tagNumberCache_.find(tagName);
    if (cacheNumberIterator != tagNumberCache_.end()) {
        tagNumberCacheLock_.unlock();
        *val = cacheNumberIterator->second;
        return 0;
    } else {
        std::string tagVal;
        CHECK_RET(srcLayer_->getTagVal(tagName, index_, &tagVal),
                "Failed to get value of tag \"%s\" from mif layer.",
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Trying to get tag value \"%s\" as a number.", tagVal.c_str());
        tagNumberCache_[tagName] = *val;
        tagNumberCacheLock_.unlock();
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
