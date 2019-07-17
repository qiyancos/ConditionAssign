#include "MifType.h"
#include "ConditionAssign.h"

namespace condition_assign {

MifLayer(AccessType type) : type_(type) {}

MifLayerReadWrite() : MifLayer(MifLayer::Write) {}

int MifLayerReadWrite::open(const syd::string& layerPath) {
    if (layerPath.empty()) {
        return -1;
    }
    layerPath_ = layerPath;
    mifLock_.lock();
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while running [wgt::mif_to_wsbl].");
    mifSize_ = mif_.mid.size();
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::save() {
    mifLock_.lock();
    CHECK_RET(wgt::wsbl_to_mif(mif_, layerPath_),
            "Error occurred while running [wgt::wsbl_to_mif].");
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::assignWithTag(const std::string& tagName,
        const int index, const std::string& val) {
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index."); 
    CHECK_ARGS((index < mifSize_ && index >= 0), "Index out of bound.");
    mifLock_->lock();
    mif_->mid[index][colID] = val;
    mifLock_->unlock();
    return 0;
}

int MifLayerReadWrite::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index."); 
    CHECK_ARGS((index < mifSize_ && index >= 0), "Index out of bound.");
    mifLock_->lock();
    *val = mif_->mid[index][colID];
    mifLock_->unlock();
    return 0;
}

int MifLayerReadWrite::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS((index < mifSize_ && index >= 0), "Index out of bound.");
    mifLock_.lock();
    val = mif_.data.geo_vec[index];
    mifLock_.unlock();
    return 0;
}

int MifLayerReadWrite::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
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
                        "Create new column failed!");
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


int MifLayerReadOnly::open(const syd::string& layerPath) {
    CHECK_RET(wgt::mif_to_wsbl(layerPath, mif_),
            "Error occurred while running [wgt::mif_to_wsbl].");
    return 0;
}

int MifLayerReadOnly::save() {
    CHECK_RET(wgt::wsbl_to_mif(mif_, layerPath_),
            "Error occurred while running [wgt::wsbl_to_mif].");
    return 0;
}

int MifLayerReadOnly::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_RET(-1, "Read-only layers do not support assign option.");
}

int MifLayerReadOnly::getTagVal(const std::string& tagName,
        const int index, std::string* val) {
    int colID;
    CHECK_RET(getTagColID(tagName, &colID, MifLayer::Write),
            "Failed to get column index."); 
    CHECK_ARGS((index < mifSize_ && index >= 0), "Index out of bound.");
    *val = mif_->mid[index][colID];
    return 0;
}

int MifLayerReadOnly::getGeometry(wsl::Geometry** val, const int index) {
    CHECK_ARGS((index < mifSize_ && index >= 0), "Index out of bound.");
    val = mif_.data.geo_vec[index];
    return 0;
}

int MifLayerReadOnly::getTagColID(const std::string& tagName, int* colID,
        AccessType accessType) {
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

MifItem(const MifLater& layer, const int index) : layer_(layer),
        index_(index) {}

int MifItem::assignWithTag(const std::string& tagName,
        const std::string& val) {
    CHECK_RET(layer_.assignWithTag(tagName, index_, val),
            "Failed to assign value to mif layer.");
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
        CHECK_RET(layer_.getTagVal(tagName, index_, val),
                "Failed to get tag value from mif layer.");
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
        CHECK_RET(getTagVal(tagName, &tagVal),
                "Failed to get tag value as string.");
        CHECK_ARGS(syntax::isType(tagVal, val),
                "Trying to get tag value as a number");
        tagNumberCache_[tagName] = *val;
        tagNumberCacheLock_.unlock();
        return 0;
    }
}

int MifItem::getTagType(const std::string& tagName, syntax::DataType* type) {
    tagTypeCacheLock_.lock();
    auto cacheIterator = tagTypeCache_.find(tagName);
    if (cacheIterator != tagTypeCache_.end()) {
        *type = cacheIterator->second;
        tagTypeCacheLock_.unlock();
        return 0;
    } else {
        std::string tagStringVal;
        CHECK_RET(getTagVal(tagName, &tagStringVal),
                "Failed to get tag value as string");
        double tagNumberVal;
        if (isType(tagVal, &tagNumberVal)) {
            tagNumberCacheLock_.lock();
            tagNumberCache[tagName] = tagNumberVal;
            tagNumberCacheLock_.unlock();
            tagStringCacheLock_.lock();
            tagStringCache[tagName] = tagStringVal;
            tagStringCacheLock_.unlock();
            tagTypeCache[tagName] = syntax::Number;
            tagTypeCacheLock_.unlock();
            *type = syntax::Number;
            return 0;
        } else {
            tagStringCacheLock_.lock();
            tagStringCache[tagName] = tagStringVal;
            tagStringCacheLock_.unlock();
            tagTypeCache[tagName] = syntax::String;
            tagTypeCacheLock_.unlock();
            *type = syntax::String;
            return 0;
        }
    }
}

int MifItem::getGeometry(wsl::Geometry** val) {
    if (geometry_ != nullptr) {
        *val = geometry_;
        return 0;
    } else {
        CHECK_RET(layer_.getGeometry(val, index_),
                "Failed to get geometry from mif layer.");
        geometry_ = *val;
        return 0;
    }
}

} // namepsace condition_assign
