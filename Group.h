#ifndef GROUP_H
#define GROUP_H

#include <vector>
#include <map>
#include <iostream>
#include "type_factory.h"

namespace condition_assign {

// 组类型通用父类
class Group {
public:
    struct GroupInfo {
        std::vector<Node*> conditions;
        std::vector<std::string> tagGroup;
        std::string newTag;
    };  
    
    // 未生成实体时存放的解析信息，处理结束后必须为NULL
    GroupInfo* info_ = nullptr;

    // Group的类型
    enum Type {Item, Tag, Point, Link, Area};
    // 获取当前Group的类型
    Type getGroupType();
    static Type getInputType();
    static Type setInputType(Type type);
    
    // 向Group中添加元素的虚函数
    virtual int addElement(const int newElement);
    virtual int addElement(const std::string& newElement);
    virtual int addElement(const wsl::Geometry*);

    // 包含关系的判断
    virtual bool checkOneContain(const std::string& src) {
    virtual bool checkAllContain(const std::string& src);
    // 地理位置包含的判断
    virtual bool checkOneContain(const wsl::Geometry*);
    virtual bool checkAllContain(const wsl::Geometry*);
    // 地理位置被包含的判断
    virtual bool checkOneContained(const wsl::Geometry*);
    virtual bool checkAllContained(const wsl::Geometry*);
    // 地理位置交叉的判断
    virtual bool checkOneIntersect(const wsl::Geometry*);
    virtual bool checkAllIntersect(const wsl::Geometry*);
    // 地理位置相离的判断
    virtual bool checkOneDeparture(const wsl::Geometry*);
    virtual bool checkAllDeparture(const wsl::Geometry*);

    Group(GroupType type);
    virtual ~Group();

protected:
    // 当前Group的类型
    const Type groupType_;
    // 当前输入层的地理抽象类型
    static const Type inputType_;
};

class ItemGroup : public Group {
public:
    ItemGroup();
    // 添加元素
    int addElement(const int newElement);
private:
    // 元素所在层
    const Layer& layer_;
    // 元素组的索引
    std::vector<int> group_;
};

class TagGroup : public Group {
public:
    TagGroup();
    // 添加元素
    int addElement(const std::string& newElement);
    // 检测是否存在于当前的Tag组里面
    bool checkOneContain(const std::string& src);
    bool checkAllContain(const std::string& src);
private:
    std::vector<std::string> group_;
};

class PointGroup : public Group {
public:
    PointGroup();
    // 添加元素
    int addElement(const wsl::Geometry*);

    // 地理位置包含的判断
    bool checkOneContain(const wsl::Geometry*);
    bool checkAllContain(const wsl::Geometry*);
    // 地理位置被包含的判断
    bool checkOneContained(const wsl::Geometry*);
    bool checkAllContained(const wsl::Geometry*);
private:
    std::vector<wsl::Feature<wsl::Point>*> group_;
};

class LinkGroup : public Group {
public:
    LinkGroup();
    int addElement(const wsl::Geometry*);

    // 地理位置包含的判断
    bool checkOneContain(const wsl::Geometry*);
    bool checkAllContain(const wsl::Geometry*);
    // 地理位置被包含的判断
    bool checkOneContained(const wsl::Geometry*);
    bool checkAllContained(const wsl::Geometry*);
    // 地理位置交叉的判断
    bool checkOneIntersect(const wsl::Geometry*);
    bool checkAllIntersect(const wsl::Geometry*);
    // 地理位置相离的判断
    bool checkOneDeparture(const wsl::Geometry*);
    bool checkAllDeparture(const wsl::Geometry*);
private:
    std::vector<wsl::Feature<wsl::Line>*> group_;
};

class AreaGroup : public Group {
public:
    AreaGroup();
    int addElement(const wsl::Geometry*);

    // 地理位置包含的判断
    bool checkOneContain(const wsl::Geometry*);
    bool checkAllContain(const wsl::Geometry*);
    // 地理位置被包含的判断
    bool checkOneContained(const wsl::Geometry*);
    bool checkAllContained(const wsl::Geometry*);
    // 地理位置交叉的判断
    bool checkOneIntersect(const wsl::Geometry*);
    bool checkAllIntersect(const wsl::Geometry*);
    // 地理位置相离的判断
    bool checkOneDeparture(const wsl::Geometry*);
    bool checkAllDeparture(const wsl::Geometry*);
private:
    std::vector<wsl::Feature<wsl::Polygon>*> group_;
};

} // namespace condition_assign

#endif // GROUP_H
