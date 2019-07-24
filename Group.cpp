#include "Group.h"
#include "ConditionAssign.h"

namespace condition_assign {

Group::Group(Type type) : type_(type) {}

Type Group::getGroupType() {
    return groupType_;
}

static Type getInputType() {
    return Group::inputType_;
}

static int setInputType(const Type type) {
    Group::inputType_ = type;
    return 0;
}

virtual int addElement(const int newElement) {
    CHECK_RET(-1, "Add mif item is not supported.");
}

virtual int addElement(const std::string& newElement) {
    CHECK_RET(-1, "Add string-type element is not supported.");
}

virtual int addElement(const wsl::Geometry* newElement) {
    CHECK_RET(-1, "Add geometry-type element is not supported.");
}

ItemGroup::ItemGroup(MifLayer* layer) : Group(Item), layer_(layer) {}

int ItemGroup::addElement(const int newElement) {
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
        CHECK_RET(group.layer_->getTagVal(tagName, index, &tagVal),
            "Failed to get tag value from mif layer.");
        group_.insert(tagVal);
    }
    return 0;
}

int TagGroup::addElement(const std::string& newElement) {
    CHECK_RET(newElement.length() == 0, "Trying to add an empty tag.");
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
        CHECK_RET(group.layer_->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer.");
        group_.insert(reinterpret_cast<wsl::Feature<wsl::Point>*>(geoVal));
    }
    return 0;
}

int PointGroup::checkOneContain(const wsl::Geometry* src, bool* result) {
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

int PointGroup::checkAllContain(const wsl::Geometry* src, bool* result) {
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

int PointGroup::checkOneContained(const wsl::Geometry* src, bool* result) {
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

int PointGroup::checkAllContained(const wsl::Geometry* src, bool* result) {
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
        *result = true
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
        CHECK_RET(group.layer_->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer.");
        group_.insert(reinterpret_cast<wsl::Feature<wsl::Line>*>(geoVal));
    }
    return 0;
}

int LineGroup::checkOneContain(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkAllContain(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkOneContained(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkAllContained(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkOneIntersect(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkAllIntersect(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkOneInContact(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkAllContact(const wsl::Geometry* src, bool* result) {
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

int LineGroup::checkOneDeparture(const wsl::Geometry* src, bool* result) {
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
            if (wsl::intersect(srcArea, targetLine) == wsl::Departure) {
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

int LineGroup::checkAllDeparture(const wsl::Geometry* src, bool* result) {
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
            if (wsl::intersect(srcArea, targetLine) != wsl::Departure) {
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
        CHECK_RET(group.layer_->getGeometry(index, &geoVal),
            "Failed to get geometry from mif layer.");
        group_.insert(reinterpret_cast<wsl::Feature<wsl::Polygon>*>(geoVal));
    }
    return 0;
}

int AreaGroup::checkOneContain(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkAllContain(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkOneContained(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkAllContained(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkOneIntersect(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkAllIntersect(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkOneInContact(const wsl::Geometry* src, bool* result) {
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

int AreaGroup::checkAllInContact(const wsl::Geometry* src, bool* result) {
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


int AreaGroup::checkOneDeparture(const wsl::Geometry* src, bool* result) {
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
            if (wsl::intersect(srcArea, targetArea) == wsl::Departure) {
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

int LineGroup::checkAllDeparture(const wsl::Geometry* src, bool* result) {
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
            if (wsl::intersect(srcArea, targetArea) != wsl::Departure) {
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
