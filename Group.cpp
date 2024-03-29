#include "Group.h"
#include "ConditionAssign.h"
#include "Config.h"
#include "ResourcePool.h"

namespace condition_assign {

Group::Type Group::getGeometryType(wsl::Geometry* geometry) {
    wsl::PartType partType = geometry->type();
    switch(partType) {
    case wsl::POINT_TYPE: return Group::Point;
    case wsl::LINE_TYPE: return Group::Line;
    case wsl::POLYGON_TYPE: return Group::Area;
    default: return Group::Item;
    }
}

Group::Group(const Type type, const bool dynamic) : type_(type),
        dynamic_(dynamic) {
    parseDone_.init(0, Semaphore::OnceForAll);
    ready_.init(0, Semaphore::OnceForAll);
}

Group::~Group() {
    if (info_ != nullptr) {
        delete info_;
    }
}

Group::GroupInfo::GroupInfo() : configItem_(new ConfigItem()) {}

Group::GroupInfo::~GroupInfo() {
    delete configItem_;
}

Group::GroupInfo* Group::GroupInfo::copy() {
    GroupInfo* newInfo = new GroupInfo();
    newInfo->layerName_ = layerName_;
    newInfo->configItem_ = new ConfigItem(*configItem_);
    newInfo->oldTagName_ = oldTagName_;
    newInfo->newTagName_ = newTagName_;
    newInfo->tagName_ = tagName_;
    return newInfo;
}

int Group::buildDynamicGroup(Group** groupPtr, MifItem* item) {
    CHECK_RET(-1, "Can not build dynamic from this type of group.");
    return 0;
}

int Group::addElements(const std::vector<int>& newElements) {
    CHECK_RET(-1, "Add multiple mif items is not supported.");
    return 0;
}

int Group::addElements(const std::vector<std::string>& newElements) {
    CHECK_RET(-1, "Add string-type element is not supported.");
    return 0;
}

int Group::addElements(const std::vector<wsl::Geometry*>& newElements) {
    CHECK_RET(-1, "Add geometry-type element is not supported.");
    return 0;
}

int Group::checkOneContain(const std::string& src, bool* result) {
    CHECK_RET(-1, "Check one contain for tag not supported.");
    return 0;
}

int Group::checkAllContain(const std::string& src, bool* result) {
    CHECK_RET(-1, "Check all contain for tag not supported.");
    return 0;
}

int Group::checkOneContain(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check one contain for geometry not supported.");
    return 0;
}

int Group::checkAllContain(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check all contain for geometry not supported.");
    return 0;
}

int Group::checkOneContained(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check one contained for geometry not supported.");
    return 0;
}

int Group::checkAllContained(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check all contained for geometry not supported.");
    return 0;
}

int Group::checkOneIntersect(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check one intersect for geometry not supported.");
    return 0;
}

int Group::checkAllIntersect(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check all intersect for geometry not supported.");
    return 0;
}

int Group::checkOneAtEdge(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check one in contact  for geometry not supported.");
    return 0;
}

int Group::checkAllAtEdge(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check all in contact for geometry not supported.");
    return 0;
}

int Group::checkOneDeparture(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check one departure for geometry not supported.");
    return 0;
}

int Group::checkAllDeparture(const Type inputType, wsl::Geometry* src,
        bool* result) {
    CHECK_RET(-1, "Check all departure for geometry not supported.");
    return 0;
}

ItemGroup::ItemGroup(const bool dynamic) : Group(Item, dynamic) {}

int ItemGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_RET(-1, "Item group can not be initiated with iten group.");
    return 0;
}

int ItemGroup::buildDynamicGroup(Group** groupPtr, MifItem* item) {
    CHECK_ARGS(dynamic_ && info_ != nullptr,
            "Can not find group infomation for dynamic group.");
    ItemGroup* newItemGroup = new ItemGroup(true);
    CHECK_ARGS(layer_ != nullptr, "Plugin layer not provided.");
    newItemGroup->setLayer(layer_);
    std::string oldTagVal;
    CHECK_RET(item->getTagVal(info_->oldTagName_, &oldTagVal),
            "Failed to get value of tag \"%s\" from input layer.",
            info_->oldTagName_.c_str());
    std::vector<std::string> oldTagGroup = htk::split(oldTagVal, "|");
    std::set<std::string> oldTagSet;
    for (const std::string& tagName : oldTagGroup) {
        oldTagSet.insert(htk::trim(tagName, "\""));
    }
    // 首次构建动态Group
    if (!size_) {
        std::lock_guard<std::mutex> groupGuard(groupLock_);
        if (!size_) {
            MifItem* workingItem;
            for (int index = 0; index < layer_->size(); index++) {
                CHECK_RET(layer_->newMifItem(index, nullptr, &workingItem),
                        "Failed to create working mif item for plugin layer.");
                if (satisfyConditions(*(info_->configItem_), workingItem)) {
                    std::string newTagVal;
                    CHECK_RET(workingItem->getTagVal(info_->newTagName_,
                            &newTagVal), "Can not get value of tag \"%s\" %s",
                            info_->newTagName_.c_str(), "from plugin layer.");
                    dynamicGroupMap_[htk::trim(newTagVal, "\"")] = index;
                }
            }
            size_ = dynamicGroupMap_.size();
        }
    }
    std::vector<int> matchIndexes;
    for (const std::string& tagName : oldTagSet) {
        auto mapIter = dynamicGroupMap_.find(tagName);
        if (mapIter != dynamicGroupMap_.end()) {
            matchIndexes.push_back(mapIter->second);
        }
    }
    newItemGroup->addElements(matchIndexes);
    if (info_->tagName_ == "GEOMETRY") {
        *groupPtr = new GeometryGroup();
    } else {
        *groupPtr = new TagGroup();
    }
    newItemGroup->ready_.signalAll();
    CHECK_RET((*groupPtr)->init(*newItemGroup, info_->tagName_),
            "Failed to init from new dynamic item group.");
    delete newItemGroup;
    return 0;
}

int ItemGroup::addElements(const std::vector<int>& newElements) {
    std::lock_guard<std::mutex> groupGuard(groupLock_);
    group_.insert(group_.end(), newElements.begin(), newElements.end());
    size_ += newElements.size();
    return 0;
}

TagGroup::TagGroup() : Group(Tag) {}

int TagGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    const_cast<Semaphore&>(itemGroup.ready_).wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    MifLayer* srcLayer = group.getLayer();
    for (int index : group.group_) {
        std::string tagVal;
        CHECK_RET(srcLayer->getTagVal(tagName, index, &tagVal),
            "Failed to get value of tag \"%s\" from mif layer.",
            tagName.c_str());
        group_.insert(tagVal);
    }
    size_ = group_.size();
    if (!group.isDynamic()) {
        delete group.info_;
        const_cast<ItemGroup&>(group).info_ = nullptr;
    }
    return 0;
}

int TagGroup::addElements(const std::vector<std::string>& newElements) {
    group_.insert(newElements.begin(), newElements.end());
    size_ += newElements.size();
    return 0;
}

int TagGroup::checkOneContain(const std::string& src, bool* result) {
    CHECK_ARGS(src.length() > 0, "Invalid input tag value.");
    *result = (group_.find(src) == group_.end() ? false : true);
    return 0;
}

int TagGroup::checkAllContain(const std::string& src, bool* result) {
    CHECK_ARGS(src.length() > 0, "Invalid input tag value.");
    *result = (group_.find(src) == group_.end() ? false : true);
    return 0;
}

GeometryGroup::GeometryGroup() : Group(Item) {}

GeometryGroup::~GeometryGroup() {
    if (groupRtree_)
    RTreeDestroy(groupRtree_);
}

int GeometryGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    const_cast<Semaphore&>(itemGroup.ready_).wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    MifLayer* srcLayer = group.getLayer();
    type_ = srcLayer->getGeoType();
    wsl::Geometry* geoVal;
    if (!groupRtree_) {
        groupRtree_ = RTreeCreate();
    }
    int elementID = 1;
    for (int index : group.group_) {
        CHECK_RET(srcLayer->getGeometry(&geoVal, index),
            "Failed to get geometry from mif layer in item[%d].", index);
        Rect rect {geoVal->mbr().ll.x(), geoVal->mbr().ll.y(),
                geoVal->mbr().ru.x(), geoVal->mbr().ru.y()};
        RTreeInsertRect(groupRtree_, &rect, elementID++, 0);
        group_.push_back(geoVal);
    }
    size_ = group_.size();
    if (!group.isDynamic()) {
        delete group.info_;
        const_cast<ItemGroup&>(group).info_ = nullptr;
    }
    return 0;
}

int GeometryGroup::addElements(
        const std::vector<wsl::Geometry*>& newElements) {
    group_.insert(group_.end(), newElements.begin(), newElements.end());
    int elementID = size_ + 1;
    if (!groupRtree_) {
        groupRtree_ = RTreeCreate();
    }
    for (wsl::Geometry* element : newElements) {
        Rect rect {element->mbr().ll.x(), element->mbr().ll.y(),
                element->mbr().ru.x(), element->mbr().ru.y()};
        RTreeInsertRect(groupRtree_, &rect, elementID++, 0);
    }
    size_ += newElements.size();
    return 0;
}

int rtreeSearchCallBack(int id, void* arg) {
    std::vector<int>* vecPtr = reinterpret_cast<std::vector<int>*>(arg);
    vecPtr->push_back(id - 1);
    return 1;
}

int GeometryGroup::checkOneContain(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (!size_) {
        return 0;
    }
    CHECK_ARGS(static_cast<int>(inputType) >= static_cast<int>(type_) &&
            (inputType != Line || type_ != Line),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        if (inputType == Area) {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) == wsl::CONTAIN) {
                    *result = true;
                    return 0;
                }
            }
        } else {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) == wsl::INTERSECT) {
                    *result = true;
                    return 0;
                }
            }
        }
    }
    return 0;
}

int GeometryGroup::checkAllContain(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (!size_) {
        return 0;
    }
    CHECK_ARGS(static_cast<int>(inputType) >= static_cast<int>(type_) &&
            (inputType != Line || type_ != Line),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        if (inputType == Area) {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) != wsl::CONTAIN) {
                    *result = false;
                    return 0;
                }
            }
            *result = true;
        } else {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) != wsl::INTERSECT) {
                    *result = false;
                    return 0;
                }
            }
            *result = true;
        }
    }
    return 0;
}

int GeometryGroup::checkOneContained(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (!size_) {
        return 0;
    }
    CHECK_ARGS(static_cast<int>(inputType) <= static_cast<int>(type_) &&
            (inputType != Line || type_ != Line),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        if (type_ == Area) {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(src, target) == wsl::CONTAIN) {
                    *result = true;
                    return 0;
                }
            }
        } else {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(src, target) == wsl::INTERSECT) {
                    *result = true;
                    return 0;
                }
            }
        }
    }
    return 0;
}

int GeometryGroup::checkAllContained(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (!size_) {
        return 0;
    }
    CHECK_ARGS(static_cast<int>(inputType) <= static_cast<int>(type_) &&
            (inputType != Line || type_ != Line),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        if (type_ == Area) {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) != wsl::CONTAIN) {
                    *result = false;
                    return 0;
                }
            }
            *result = true;
        } else {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(target, src) != wsl::INTERSECT) {
                    *result = false;
                    return 0;
                }
            }
            *result = true;
        }
    }
    return 0;
}

int GeometryGroup::checkOneIntersect(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (size_ == 0) {
        return 0;
    }
    CHECK_ARGS(inputType != Point && type_ != Point,
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        for (int checkIndex : checkList) {
            target = group_[checkIndex];
            if (wsl::intersect(src, target) == wsl::INTERSECT) {
                *result = true;
                return 0;
            }
        }
    }
    return 0;
}

int GeometryGroup::checkAllIntersect(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (size_ == 0) {
        return 0;
    }
    CHECK_ARGS(inputType != Point && type_ != Point,
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        for (int checkIndex : checkList) {
            target = group_[checkIndex];
            if (wsl::intersect(src, target) != wsl::INTERSECT) {
                return 0;
            }
            *result = true;
        }
    }
    return 0;
}

int GeometryGroup::checkOneAtEdge(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (size_ == 0) {
        return 0;
    }
    CHECK_ARGS((inputType == Point && type_ == Area) ||
            (inputType == Area && type_ == Point),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        for (int checkIndex : checkList) {
            target = group_[checkIndex];
            if (wsl::intersect(src, target) == wsl::ONE_POINT) {
                *result = true;
                return 0;
            }
        }
    }
    return 0;
}

int GeometryGroup::checkAllAtEdge(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = false;
    if (size_ == 0) {
        return 0;
    }
    CHECK_ARGS((inputType == Point && type_ == Area) ||
            (inputType == Area && type_ == Point),
            "Unsupported geometry type for position processing.");
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        for (int checkIndex : checkList) {
            target = group_[checkIndex];
            if (wsl::intersect(src, target) != wsl::ONE_POINT) {
                return 0;
            }
            *result = true;
        }
    }
    return 0;
}


int GeometryGroup::checkOneDeparture(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = true;
    if (size_ == 0) {
        return 0;
    }
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        if (checkList.size() < size_) {
            return 0;
        } else {
            for (int checkIndex : checkList) {
                target = group_[checkIndex];
                if (wsl::intersect(src, target) == wsl::DEPARTURE) {
                    return 0;
                }
            }
            *result = false;
        }
    }
    return 0;
}

int GeometryGroup::checkAllDeparture(const Type inputType, wsl::Geometry* src,
        bool* result) {
    *result = true;
    if (size_ == 0) {
        return 0;
    }
    Rect srcRect {src->mbr().ll.x(), src->mbr().ll.y(),
            src->mbr().ru.x(), src->mbr().ru.y()};
    std::vector<int> checkList;
    wsl::Geometry* target;
    if (RTreeSearch(groupRtree_, groupRtree_->nodeRoot, &srcRect,
            rtreeSearchCallBack, &checkList)) {
        for (int checkIndex : checkList) {
            target = group_[checkIndex];
            if (wsl::intersect(src, target) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
    }
    return 0;
}

} // namepsace condition_assign
