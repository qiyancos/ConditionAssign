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
    // Group的类型
    enum Type {Item, Tag, Point, Line, Area};
    // 解析完毕后的group结构信息
    struct GroupInfo {
        // 所在layer的名称
        std::string layerName;
        // 筛选条件
        std::vector<Node*> conditions;
        // 原始的tag集合
        std::set<std::string> tagGroup;
        // 新的tag名称集合
        std::string tagName;
        // 二次索引的信息
        std::string groupTagName;
    }; 
    
    // 未生成实体时存放的解析信息
    GroupInfo* info_ = nullptr;
    // Group数据是否准备完毕
    Semaphore ready_(0, OnceForAll);

    // 获取当前Group的类型
    Type getGroupType();
    // 获取当前输入的地理类型
    static Type getInputType();
    // 设置当前的输入类型
    static int setInputType(const Type type);
    // 获取当前Group的元素个数
    int size() {return size_;}

    // 基于一个ItemGroup生成类型Group
    virtual int init(const Group& itemGroup, const std::string& tagName) = 0;
    
    // 向Group中添加元素的虚函数
    virtual int addElement(const int newElement);
    virtual int addElement(const std::string& newElement);
    virtual int addElement(wsl::Geometry* newElement);

    // 包含关系的判断
    virtual int checkOneContain(const std::string& src, bool* result) = 0;
    virtual int checkAllContain(const std::string& src, bool* result) = 0;
    // 地理位置包含的判断
    virtual int checkOneContain(wsl::Geometry* src, bool* result)= 0;
    virtual int checkAllContain(wsl::Geometry* src, bool* result) = 0;
    // 地理位置被包含的判断
    virtual int checkOneContained(wsl::Geometry* src, bool* result) = 0;
    virtual int checkAllContained(wsl::Geometry* src, bool* result) = 0;
    // 地理位置交叉的判断
    virtual int checkOneIntersect(wsl::Geometry* src, bool* result) = 0;
    virtual int checkAllIntersect(wsl::Geometry* src, bool* result) = 0;
    // 地理位置接触的判断
    virtual int checkOneInContact(wsl::Geometry* src, bool* result) = 0;
    virtual int checkAllInContact(wsl::Geometry* src, bool* result) = 0;
    // 地理位置相离的判断
    virtual int checkOneDeparture(wsl::Geometry* src, bool* result) = 0;
    virtual int checkAllDeparture(wsl::Geometry* src, bool* result) = 0;

    Group(Type type);
    virtual ~Group();

protected:
    // 当前Group的元素个数
    int size_ = 0;
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
    int init(const Group& itemGroup, const std::string& tagName);
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
    int init(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElement(wsl::Geometry* newElement);

    // 地理位置包含的判断
    int checkOneContain(wsl::Geometry* src, bool* result);
    int checkAllContain(wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(wsl::Geometry* src, bool* result);
    int checkAllContained(wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Point>*> group_;
};

class LineGroup : public Group {
public:
    LineGroup();
    // 基于一个ItemGroup生成LineGroup
    int init(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElement(wsl::Geometry*);

    // 地理位置包含的判断
    int checkOneContain(wsl::Geometry* src, bool* result);
    int checkAllContain(wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(wsl::Geometry* src, bool* result);
    int checkAllContained(wsl::Geometry* src, bool* result);
    // 地理位置交叉的判断
    int checkOneIntersect(wsl::Geometry* src, bool* result);
    int checkAllIntersect(wsl::Geometry* src, bool* result);
    // 地理位置接触的判断
    int checkOneInContact(wsl::Geometry* src, bool* result);
    int checkAllInContact(wsl::Geometry* src, bool* result);
    // 地理位置相离的判断
    int checkOneDeparture(wsl::Geometry* src, bool* result);
    int checkAllDeparture(wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Line>*> group_;
};

class AreaGroup : public Group {
public:
    AreaGroup();
    // 基于ItemGroup构造AreaGroup
    int init(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElement(wsl::Geometry*);

    // 地理位置包含的判断
    int checkOneContain(wsl::Geometry* src, bool* result);
    int checkAllContain(wsl::Geometry* src, bool* result);
    // 地理位置被包含的判断
    int checkOneContained(wsl::Geometry* src, bool* result);
    int checkAllContained(wsl::Geometry* src, bool* result);
    // 地理位置交叉的判断
    int checkOneIntersect(wsl::Geometry* src, bool* result);
    int checkAllIntersect(wsl::Geometry* src, bool* result);
    // 地理位置接触的判断
    int checkOneInContact(wsl::Geometry* src, bool* result);
    int checkAllInContact(wsl::Geometry* src, bool* result);
    // 地理位置相离的判断
    int checkOneDeparture(wsl::Geometry* src, bool* result);
    int checkAllDeparture(wsl::Geometry* src, bool* result);
private:
    std::vector<wsl::Feature<wsl::Polygon>*> group_;
};

} // namespace condition_assign

#endif // GROUP_H
