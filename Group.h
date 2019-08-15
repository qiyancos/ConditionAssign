#ifndef GROUP_H
#define GROUP_H

#include "type_factory.h"
#include "Semaphore.h"

#include <vector>
#include <atomic>
#include <map>
#include <iostream>

namespace condition_assign {

class ConfigItem;
class MifLayer;
class MifItem;

// 组类型通用父类
class Group {
public:
    // Group的类型
    enum Type {Item, Tag, Point, Line, Area};
    // 解析完毕后的group结构信息
    class GroupInfo {
    public:
        // 构造函数
        GroupInfo();
        // 析构函数
        ~GroupInfo();
        // 生成一个当前GroupInfo的拷贝
        GroupInfo* copy();

    public:
        // 已经检查过的元素个数
        std::atomic<int> checkedCount_ {0};
        // 所在layer的名称
        std::string layerName_;
        // 筛选条件
        ConfigItem* configItem_;
        // 原始的tag集合
        std::string oldTagName_;
        // 新映射tag集合
        std::string newTagName_;
        // 索引的tag名称集合
        std::string tagName_;
    };

    // 获取当前的Geometry信息
    static Type getGeometryType(wsl::Geometry* geometry);
    // 获取当前Group的类型
    Type getGroupType() const {return type_;}
    // 设置当前Group的layer指针
    void setLayer(MifLayer* layer) {layer_ = layer;}
    // 获取当前Group对应的layer
    MifLayer* getLayer() const {return layer_;}
    // 获取当前Group的元素个数
    int size() const {return size_;}
    // 判断当前的
    bool isDynamic() const {return dynamic_;}

    // 基于一个ItemGroup生成类型Group
    virtual int init(const Group& itemGroup, const std::string& tagName) = 0;
    // 动态Group的构建
    virtual int buildDynamicGroup(Group** groupPtr, MifItem* item);

    // 向Group中添加元素的虚函数
    virtual int addElements(const std::vector<int>& newElements);
    virtual int addElements(const std::vector<std::string>& newElements);
    virtual int addElements(const std::vector<wsl::Geometry*>& newElements);

    // 包含关系的判断
    virtual int checkOneContain(const std::string& src, bool* result);
    virtual int checkAllContain(const std::string& src, bool* result);
    // 地理位置包含的判断
    virtual int checkOneContain(const Type inputType, wsl::Geometry* src,
            bool* result);
    virtual int checkAllContain(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置被包含的判断
    virtual int checkOneContained(const Type inputType, wsl::Geometry* src,
            bool* result);
    virtual int checkAllContained(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置交叉的判断
    virtual int checkOneIntersect(const Type inputType, wsl::Geometry* src,
            bool* result);
    virtual int checkAllIntersect(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置接触的判断
    virtual int checkOneAtEdge(const Type inputType, wsl::Geometry* src,
            bool* result);
    virtual int checkAllAtEdge(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置相离的判断
    virtual int checkOneDeparture(const Type inputType, wsl::Geometry* src,
            bool* result);
    virtual int checkAllDeparture(const Type inputType, wsl::Geometry* src,
            bool* result);

    // 构造函数
    Group(const Type type, const bool dynamic = false);
    // 析构函数
    virtual ~Group();

public:
    // 未生成实体时存放的解析信息
    GroupInfo* info_ = nullptr;
    // Group的内容是否解析完毕
    Semaphore parseDone_;
    // Group数据是否准备完毕
    Semaphore ready_;
    // 当前Group的key值
    int64_t groupKey_;

protected:
    // 元素所在层
    MifLayer* layer_ = nullptr;
    // 当前Group的类型
    Type type_;
    // 当前Group是不是一个动态Group
    bool dynamic_ = false;
    // 当前Group的元素个数
    int size_ = 0;
};

class ItemGroup : public Group {
public:
    ItemGroup(const bool dynamic = false);
    // 基于一个ItemGroup生成类型Group
    int init(const Group& itemGroup, const std::string& tagName);
    // 动态Group的构建
    int buildDynamicGroup(Group** groupPtr, MifItem* item);
    // 原子性添加元素
    int addElements(const std::vector<int>& newElements);

public:
    // 元素组的锁
    std::mutex groupLock_;
    // 元素组的索引
    std::vector<int> group_;
};

class TagGroup : public Group {
public:
    TagGroup();
    // 基于ItemGroup生成一个TagGroup
    int init(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElements(const std::vector<std::string>& newElements);
    // 检测是否存在于当前的Tag组里面
    int checkOneContain(const std::string& src, bool* result);
    int checkAllContain(const std::string& src, bool* result);

private:
    std::set<std::string> group_;
};

class GeometryGroup : public Group {
public:
    GeometryGroup();
    // 基于ItemGroup构造AreaGroup
    int init(const Group& itemGroup, const std::string& tagName);
    // 添加元素
    int addElements(const std::vector<wsl::Geometry*>& newElements);

    // 地理位置包含的判断
    int checkOneContain(const Type inputType, wsl::Geometry* src,
            bool* result);
    int checkAllContain(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置被包含的判断
    int checkOneContained(const Type inputType, wsl::Geometry* src,
            bool* result);
    int checkAllContained(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置交叉的判断
    int checkOneIntersect(const Type inputType, wsl::Geometry* src,
            bool* result);
    int checkAllIntersect(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置接触的判断
    int checkOneAtEdge(const Type inputType, wsl::Geometry* src,
            bool* result);
    int checkAllAtEdge(const Type inputType, wsl::Geometry* src,
            bool* result);
    // 地理位置相离的判断
    int checkOneDeparture(const Type inputType, wsl::Geometry* src,
            bool* result);
    int checkAllDeparture(const Type inputType, wsl::Geometry* src,
            bool* result);

private:
    std::vector<wsl::Geometry*> group_;
};

} // namespace condition_assign

#endif // GROUP_H
