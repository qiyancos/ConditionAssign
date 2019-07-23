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
        std::string tagName;
    };  
    
    // 未生成实体时存放的解析信息，处理结束后必须为NULL
    GroupInfo* info_ = nullptr;

    // Group的类型
    enum Type {Item, Tag, Point, Line, Area};
    // 获取当前Group的类型
    Type getGroupType();
    static Type getInputType();
    static int setInputType(const Type type);
    
    // 向Group中添加元素的虚函数
    virtual int addElement(const int newElement);
    virtual int addElement(const std::string& newElement);
    virtual int addElement(const wsl::Geometry* newElement);

    // 包含关系的判断
    virtual int checkOneContain(const std::string& src, bool* result) = 0;
    virtual int checkAllContain(const std::string& src, bool* result) = 0;
    // 地理位置包含的判断
    virtual int checkOneContain(const wsl::Geometry* src, bool* result)= 0;
    virtual int checkAllContain(const wsl::Geometry* src, bool* result) = 0;
    // 地理位置被包含的判断
    virtual int checkOneContained(const wsl::Geometry* src,
            bool* result) = 0;
    virtual int checkAllContained(const wsl::Geometry* src,
            bool* result) = 0;
    // 地理位置交叉的判断
    virtual int checkOneIntersect(const wsl::Geometry* src,
            bool* result) = 0;
    virtual int checkAllIntersect(const wsl::Geometry* src,
            bool* result) = 0;
    // 地理位置接触的判断
    virtual int checkOneInContact(const wsl::Geometry* src,
            bool* result) = 0;
    virtual int checkAllInContact(const wsl::Geometry* src,
            bool* result) = 0;
    // 地理位置相离的判断
    virtual int checkOneDeparture(const wsl::Geometry* src,
            bool* result) = 0;
    virtual int checkAllDeparture(const wsl::Geometry* src,
            bool* result) = 0;

    Group(Type type);
    virtual ~Group();

protected:
    // 当前Group的类型
    const Type groupType_;
    // 当前输入层的地理抽象类型
    static Type inputType_;
};

Group::Type Group::inputType_ = Group::Item;

class ItemGroup : public Group {
public:
    ItemGroup(MifLayer* layer);
    // 添加元素
    int addElement(const int newElement);
    // 元素所在层
    MifLayer* layer_;
    // 元素组的索引
    std::vector<int> group_;
};

class TagGroup : public Group {
public:
    TagGroup();
    // 基于ItemGroup生成一个TagGroup
    TagGroup(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElement(const std::string& newElement);
    // 检测是否存在于当前的Tag组里面
    int checkOneContain(const std::string& src, bool* result);
    int checkAllContain(const std::string& src, bool* result);
private:
    std::set<std::string> group_;
};

class PointGroup : public Group {
public:
    PointGroup();
    // 基于一个ItemGroup生成PointGroup 
    PointGroup(const Group& itemGroup);
    // 添加元素
    int addElement(const wsl::Geometry* newElement);

    // 地理位置包含的判断
    int checkOneContain(const wsl::Geometry* src, bool* result);
    int checkAllContain(const wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(const wsl::Geometry* src, bool* result);
    int checkAllContained(const wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Point>*> group_;
};

class LineGroup : public Group {
public:
    LineGroup();
    // 基于一个ItemGroup生成LineGroup
    LineGroup(const Group& itemGroup);
    // 添加元素
    int addElement(const wsl::Geometry*);

    // 地理位置包含的判断
    int checkOneContain(const wsl::Geometry* src, bool* result);
    int checkAllContain(const wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(const wsl::Geometry* src, bool* result);
    int checkAllContained(const wsl::Geometry* src, bool* result);
    // 地理位置交叉的判断
    int checkOneIntersect(const wsl::Geometry* src, bool* result);
    int checkAllIntersect(const wsl::Geometry* src, bool* result);
    // 地理位置接触的判断
    int checkOneInContact(const wsl::Geometry* src, bool* result);
    int checkAllInContact(const wsl::Geometry* src, bool* result);
    // 地理位置相离的判断
    int checkOneDeparture(const wsl::Geometry* src, bool* result);
    int checkAllDeparture(const wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Line>*> group_;
};

class AreaGroup : public Group {
public:
    AreaGroup();
    // 基于ItemGroup构造AreaGroup
    AreaGroup(const Group& itemGroup);
    // 添加元素
    int addElement(const wsl::Geometry*);

    // 地理位置包含的判断
    int checkOneContain(const wsl::Geometry* src, bool* result);
    int checkAllContain(const wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(const wsl::Geometry* src, bool* result);
    int checkAllContained(const wsl::Geometry* src, bool* result);
    // 地理位置交叉的判断
    int checkOneIntersect(const wsl::Geometry* src, bool* result);
    int checkAllIntersect(const wsl::Geometry* src, bool* result);
    // 地理位置接触的判断
    int checkOneInContact(const wsl::Geometry* src, bool* result);
    int checkAllInContact(const wsl::Geometry* src, bool* result);
    // 地理位置相离的判断
    int checkOneDeparture(const wsl::Geometry* src, bool* result);
    int checkAllDeparture(const wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Polygon>*> group_;
};

} // namespace condition_assign

#endif // GROUP_H
