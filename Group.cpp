#include "Group.h"
#include "ConditionAssign.h"
#include "Config.h"
#include "ResourcePool.h"

namespace condition_assign {

Group::Type Group::inputType_ = Group::Item;

Group::Group(const Type type, const bool dynamic = false) :
        groupType_(type), dynamic_(dynamic) {
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

Group::Type Group::getInputType() {
    return Group::inputType_;
}

int Group::setInputType(const std::string& typeStr) {
    if (typeStr == "POINT" || typeStr.empty()) {
        Group::inputType_ = Point;
    } else if (typeStr == "LINE") {
        Group::inputType_ = Line;
    } else if (typeStr == "AREA") {
        Group::inputType_ = Area;
    } else {
        return -1;
    }
    return 0;
}

Group::Type Group::getGroupType() {
    return groupType_;
}

int Group::buildDynamicGroup(Group** groupPtr, MifItem* item) {
    CHECK_RET(-1, "Can not build dynamic from this type of group.");
}

int Group::addElement(const int newElement) {
    CHECK_RET(-1, "Add mif item is not supported.");
}

int Group::addElement(const std::string& newElement) {
    CHECK_RET(-1, "Add string-type element is not supported.");
}

int Group::addElement(wsl::Geometry* newElement) {
    CHECK_RET(-1, "Add geometry-type element is not supported.");
}

int Group::checkOneContain(const std::string& src, bool* result) {
    CHECK_RET(-1, "Check one contain for tag not supported.");
}

int Group::checkAllContain(const std::string& src, bool* result) {
    CHECK_RET(-1, "Check all contain for tag not supported.");
}

int Group::checkOneContain(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check one contain for geometry not supported.");
}

int Group::checkAllContain(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check all contain for geometry not supported.");
}

int Group::checkOneContained(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check one contained for geometry not supported.");
}

int Group::checkAllContained(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check all contained for geometry not supported.");
}

int Group::checkOneIntersect(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check one intersect for geometry not supported.");
}

int Group::checkAllIntersect(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check all intersect for geometry not supported.");
}

int Group::checkOneInContact(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check one in contact  for geometry not supported.");
}

int Group::checkAllInContact(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check all in contact for geometry not supported.");
}

int Group::checkOneDeparture(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check one departure for geometry not supported.");
}

int Group::checkAllDeparture(wsl::Geometry* src, bool* result) {
    CHECK_RET(-1, "Check all departure for geometry not supported.");
}

ItemGroup::ItemGroup(const bool dynamic = false) : Group(Item, dynamic) {}

int ItemGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_RET(-1, "Item group can not be initiated with iten group.");
}

int ItemGroup::buildDynamicGroup(Group** groupPtr, MifItem* item) {
    CHECK_ARGS(dynamic_ && info_ != nullptr,
            "Can not find group infomation for dynamic group.");
    Group* newItemGroup = new ItemGroup(true);
    CHECK_ARGS(layer_ != nullptr, "Plugin layer not provided.");
    newItemGroup->setLayer(layer_);
    std::string oldTagVal;
    CHECK_RET(item->getTagVal(info_->oldTagName_, &oldTagVal),
            "Failed to get value of tag \"%s\" from input layer.",
            info_->oldTagName_.c_str());
    std::vector<std::string> oldTagGroup = htk::split(oldTagVal, "|");
    std::set<std::string> oldTagMap(oldTagGroup.begin(), oldTagGroup.end());
    if (size_ == 0) {
        MifItem* workingItem;
        for (int index = 0; index < layer_->size(); index++) {
            CHECK_RET(layer_->newMifItem(index, &workingItem, nullptr),
                    "Failed to create working mif item for plugin layer.");
            if (satisfyConditions(*(info_->configItem_), workingItem)) {
                std::string newTagVal;
                CHECK_RET(workingItem->getTagVal(info_->newTagName_,
                        &newTagVal), "Failed to get value of tag \"%s\" %s",
                        info_->newTagName_.c_str(), "from plugin layer.");
                if (oldTagMap.find(newTagVal) != oldTagMap.end()) {
                    newItemGroup->addElement(index);
                }
            }
        }
    } else {
        MifItem* workingItem;
        for (int index : group_) {
            CHECK_RET(layer_->newMifItem(index, &workingItem, nullptr),
                    "Failed to create working mif item for plugin layer.");
            if (satisfyConditions(*(info_->configItem_), workingItem)) {
                std::string newTagVal;
                CHECK_RET(workingItem->getTagVal(info_->newTagName_,
                        &newTagVal), "Failed to get value of tag \"%s\" %s",
                        info_->newTagName_.c_str(), "from plugin layer.");
                if (oldTagMap.find(newTagVal) != oldTagMap.end()) {
                    newItemGroup->addElement(index);
                }
            }
        }
    }
    if (info_->tagName_ == "POINT") {
        *groupPtr = new PointGroup();
    } else if (info_->tagName_ == "LINE") {
        *groupPtr = new LineGroup();
    } else if (info_->tagName_ == "AREA") {
        *groupPtr = new AreaGroup();
    } else {
        *groupPtr = new TagGroup();
    }
    CHECK_RET((*groupPtr)->init(*newItemGroup, info_->tagName_),
            "Failed to init from new dynamic item group.");
    delete newItemGroup;
    return 0;
}

int ItemGroup::addElement(const int newElement) {
    size_++;
    group_.push_back(newElement);
    return 0;
}

TagGroup::TagGroup() : Group(Tag) {}

int TagGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    itemGroup.ready_.wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    for (int index : group.group_) {
        std::string tagVal;
        MifLayer* srcLayer = group.getLayer();
        CHECK_RET(srcLayer->getTagVal(tagName, index, &tagVal),
            "Failed to get value of tag \"%s\" from mif layer.",
            tagName.c_str());
        group_.insert(tagVal);
    }
    size_ = group_.size();
    return 0;
}

int TagGroup::addElement(const std::string& newElement) {
    CHECK_ARGS(newElement.length() == 0, "Trying to add an empty tag.");
    size_++;
    group_.insert(newElement);
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

PointGroup::PointGroup() : Group(Point) {}

int PointGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    itemGroup.ready_.wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    for (int index : group.group_) {
        wsl::Geometry* geoVal;
        MifLayer* srcLayer = group.getLayer();
        CHECK_RET(srcLayer->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer in item[%d].", index);
        group_.push_back(reinterpret_cast<wsl::Feature<wsl::Point>*>(geoVal));
    }
    size_ = group_.size();
    return 0;
}

int PointGroup::addElement(wsl::Geometry* newElement) {
    CHECK_ARGS(newElement != nullptr, "Trying to add an empty point.");
    size_++;
    group_.push_back(reinterpret_cast<wsl::Feature<wsl::Point>*>(newElement));
    return 0;
}

int PointGroup::checkOneContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcPoint, targetPoint) == wsl::ONE_POINT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcLine, targetPoint) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcArea, targetPoint) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int PointGroup::checkAllContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcPoint, targetPoint) != wsl::ONE_POINT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcLine, targetPoint) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcArea, targetPoint) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int PointGroup::checkOneContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcPoint, targetPoint) == wsl::ONE_POINT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int PointGroup::checkAllContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Point>* targetPoint : group_) {
            if (wsl::intersect(srcPoint, targetPoint) != wsl::ONE_POINT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

LineGroup::LineGroup() : Group(Line) {}

int LineGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    itemGroup.ready_.wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    for (int index : group.group_) {
        wsl::Geometry* geoVal;
        MifLayer* srcLayer = group.getLayer();
        CHECK_RET(srcLayer->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer in item[%d].", index);
        group_.push_back(reinterpret_cast<wsl::Feature<wsl::Line>*>(geoVal));
    }
    size_ = group_.size();
    return 0;
}

int LineGroup::addElement(wsl::Geometry* newElement) {
    CHECK_ARGS(newElement != nullptr, "Trying to add an empty line.");
    size_++;
    group_.push_back(reinterpret_cast<wsl::Feature<wsl::Line>*>(newElement));
    return 0;
}


int LineGroup::checkOneContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkAllContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkOneContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcPoint, targetLine) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkAllContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcPoint, targetLine) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkOneIntersect(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) == wsl::INTERSECT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) == wsl::INTERSECT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkAllIntersect(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) != wsl::INTERSECT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) != wsl::INTERSECT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkOneInContact(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            auto geoRelation = wsl::intersect(srcLine, targetLine);
            if (geoRelation == wsl::INTERSECT || geoRelation == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            auto geoRelation = wsl::intersect(srcArea, targetLine);
            if (geoRelation == wsl::INTERSECT || geoRelation == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkAllInContact(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        // 实际上线与线之间的包含关系是不合理的
        // 但是我们这里保留功能，但不建议使用
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            auto geoRelation = wsl::intersect(srcLine, targetLine);
            if (geoRelation != wsl::INTERSECT && geoRelation != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            auto geoRelation = wsl::intersect(srcArea, targetLine);
            if (geoRelation != wsl::INTERSECT && geoRelation != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkOneDeparture(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcPoint, targetLine) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int LineGroup::checkAllDeparture(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcPoint, targetLine) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcLine, targetLine) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Line>* targetLine : group_) {
            if (wsl::intersect(srcArea, targetLine) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

AreaGroup::AreaGroup() : Group(Area) {}

int AreaGroup::init(const Group& itemGroup, const std::string& tagName) {
    CHECK_ARGS(itemGroup.getGroupType() == Item,
            "Cannot expand from group with no item-type.");
    itemGroup.ready_.wait();
    const ItemGroup& group = dynamic_cast<const ItemGroup&>(itemGroup);
    for (int index : group.group_) {
        wsl::Geometry* geoVal;
        MifLayer* srcLayer = group.getLayer();
        CHECK_RET(srcLayer->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer in item[%d].", index);
        group_.push_back(reinterpret_cast<wsl::Feature<wsl::Polygon>*>(
                geoVal));
    }
    return 0;
}

int AreaGroup::addElement(wsl::Geometry* newElement) {
    CHECK_ARGS(newElement != nullptr, "Trying to add an empty area.");
    size_++;
    group_.push_back(reinterpret_cast<wsl::Feature<wsl::Polygon>*>(
            newElement));
    return 0;
}

int AreaGroup::checkOneContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkAllContain(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkOneContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcPoint, targetArea) == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkAllContained(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcPoint, targetArea) != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkOneIntersect(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) == wsl::INTERSECT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) == wsl::INTERSECT) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkAllIntersect(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) != wsl::INTERSECT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) != wsl::INTERSECT) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkOneInContact(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            auto geoRelation = wsl::intersect(srcLine, targetArea);
            if (geoRelation == wsl::INTERSECT || geoRelation == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            auto geoRelation = wsl::intersect(srcArea, targetArea);
            if (geoRelation == wsl::INTERSECT || geoRelation == wsl::CONTAIN) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkAllInContact(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            auto geoRelation = wsl::intersect(srcLine, targetArea);
            if (geoRelation != wsl::INTERSECT && geoRelation != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            auto geoRelation = wsl::intersect(srcArea, targetArea);
            if (geoRelation != wsl::INTERSECT && geoRelation != wsl::CONTAIN) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}


int AreaGroup::checkOneDeparture(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcPoint, targetArea) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) == wsl::DEPARTURE) {
                *result = true;
                return 0;
            }
        }
        *result = false;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

int AreaGroup::checkAllDeparture(wsl::Geometry* src, bool* result) {
    if (group_.size() == 0) {
        *result = false;
        return 0;
    }
    if (Group::inputType_ == Point) {
        wsl::Feature<wsl::Point>* srcPoint =
                reinterpret_cast<wsl::Feature<wsl::Point>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcPoint, targetArea) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Line) {
        wsl::Feature<wsl::Line>* srcLine =
                reinterpret_cast<wsl::Feature<wsl::Line>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcLine, targetArea) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else if (Group::inputType_ == Area) {
        wsl::Feature<wsl::Polygon>* srcArea =
                reinterpret_cast<wsl::Feature<wsl::Polygon>*>(src);
        for (wsl::Feature<wsl::Polygon>* targetArea : group_) {
            if (wsl::intersect(srcArea, targetArea) != wsl::DEPARTURE) {
                *result = false;
                return 0;
            }
        }
        *result = true;
        return 0;
    } else {
        CHECK_ARGS(false, "Unsupported input geometry-type.");
    }
}

} // namepsace condition_assign
