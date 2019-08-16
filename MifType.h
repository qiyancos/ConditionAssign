#ifndef MIFTYPE_H
#define MIFTYPE_H

#include "Semaphore.h"
#include "SyntaxBase.h"
#include "Group.h"

#include <vector>
#include <map>
#include <string>

namespace condition_assign {

// 是否使用MifItem的Cache优化
// #define USE_MIFITEM_CACHE
// 下面的变量用于解决地理库不支持多线程的问题
extern double globalDouble;

class MifLayer {
public:
    class ItemInfo {
    public:
        // 构造函数
        ItemInfo();
        //析构函数
        ~ItemInfo();

#ifdef USE_MIFITEM_CACHE
        // 缓存的Tag数值(字符串)映射锁
        std::mutex* tagStringCacheLock_;
        // 缓存的Tag数值(字符串)映射
        std::map<std::string, std::string> tagStringCache_;
        // 缓存的Tag数值(浮点数)映射锁
        std::mutex* tagNumberCacheLock_;
        // 缓存的Tag数值(浮点数)映射
        std::map<std::string, double> tagNumberCache_;
#endif
        // 动态Group缓存的锁
        std::mutex* dynamicGroupCacheLock_;
        // 动态Group的缓存，放置多次重建
        std::map<int64_t, Group*> dynamicGroupCache_;
        // 计算地理位置时的锁
        std::mutex* geometryLock_;
        // 缓存的地理坐标信息
        wsl::Geometry* geometry_ = nullptr;
    };

public:
    // 构造函数
    MifLayer(const std::string& layerPath, MifLayer* copySrcLayer = nullptr);
    // 虚析构函数
    virtual ~MifLayer() = default;
    
    // 获取当前layer的大小
    int size();
    // 设置当前Layer的属性为输入
    void setAsInput() {isInput = true;}
    // 设置当前Layer的属性为输出
    void setAsOutput() {isOutput = true;}
    // 设置当前Layer的属性为外挂表
    void setAsPlugin() {isPlugin = true;}
    // 获取当前Layer对应的地理类型
    Group::Type getGeoType();
    
    // 获取当前Layer给定索引的mifitem
    int newMifItem(const int index, MifLayer* targetLayer,
            MifItem** newItemPtr);
    
    // 依据路径打开Layer
    virtual int open() = 0;
    // 依据拷贝源进行复制加载
    virtual int copyLoad() = 0;
    // 保存对应的Layer的数据
    virtual int save(const std::string layerPath = "") = 0;
    // 对于坐标或者普通的字段进行数值赋值
    virtual int assignWithNumber(const std::string& tagName, const int index,
            const double& val) = 0;
    // 依据字段名进行赋值操作
    virtual int assignWithTag(const std::string& tagName, const int index,
            const std::string& val) = 0;
    // 获取当前MifItem的某个字段的类型(操作带锁)
    virtual int getTagType(const std::string& tagName,
            syntax::DataType* type, bool isAssign = false) = 0;
    // 检查当前是否有某一个Tag，如果没有则会添加
    virtual int checkAddTag(const std::string& tagName,
            int* colID = nullptr, bool isAssign = false) = 0;
    // 获取当前MifItem的某个字段的内容
    virtual int getTagVal(const std::string& tagName, const int index,
            std::string* val) = 0;
    // 获取当前MifItem的某个字段的数值
    virtual int getTagVal(const std::string& tagName, const int index,
            double* val) = 0;
    // 获取当前MifItem地理坐标信息
    virtual int getGeometry(wsl::Geometry** val, const int index) = 0;
    // 判断当前的MifLayer是不是新打开的
    virtual bool isNew() {return false;}

public:
    // Layer数据结构
    wgt::MIF mif_;
    // mif中的item的个数
    int mifSize_;
    // 当前MifLayer是否打开
    Semaphore ready_;
    // 缓存Tag名称到对应索引的映射关系，快速查找
    std::map<std::string, int> tagColCache_;
    // 缓存的Tag的数据类型
    std::map<std::string, syntax::DataType> tagTypeCache_;

protected:
    // mif数据锁
    std::mutex mifLock_;
    // 复制载入对应的MifLayer
    MifLayer* copySrcLayer_;
    // 对应文件的路径
    std::string layerPath_;
    
    // 缓存Tag名称到对应索引的映射关系锁
    std::mutex tagColCacheLock_;
    // Tag类型映射缓存的锁
    std::mutex tagTypeCacheLock_;
    
    // MifItem对应的缓存
    std::vector<ItemInfo> itemInfoCache_;
    
    // 当前Layer的地理类型
    Group::Type geoType_;
    // 当前Layer是否是一个输入
    bool isInput = false;
    // 当前Layer是否是一个输出
    bool isOutput = false;
    // 当前Layer是否是一个外挂
    bool isPlugin = false;
};

// 新生成的Layer，一定是输出Layer，所有操作均带锁
class MifLayerNew : public MifLayer {
public:
    // 构造函数
    MifLayerNew(const std::string& layerPath,
            MifLayer* copySrcLayer = nullptr);
    // 虚构函数
    ~MifLayerNew() = default;

    // 依据给定的路径打开Layer
    int open();
    // 依据拷贝源进行复制加载
    int copyLoad();
    // 保存对应的Layer的数据
    int save(const std::string layerPath = "");
    // 对于坐标或者普通的字段进行数值赋值
    int assignWithNumber(const std::string& tagName, const int index,
            const double& val);
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const int index,
            const std::string& val);
    // 获取当前MifItem的某个字段的类型
    int getTagType(const std::string& tagName, syntax::DataType* type,
            bool isAssign = false);
    // 检查当前是否有某一个Tag，如果没有则会添加
    int checkAddTag(const std::string& tagName, int* colID = nullptr,
            bool isAssign = false);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const std::string& tagName, const int index,
            std::string* val);
    // 获取当前MifItem的某个字段的数值
    int getTagVal(const std::string& tagName, const int index,
            double* val);
    // 获取当前MifItem地理坐标信息
    int getGeometry(wsl::Geometry** val, const int index);
    // 判断当前的MifLayer是不是新打开的
    virtual bool isNew() {return true;}
};

// 原本就存在的Layer(内部操作一般无锁)
class MifLayerNormal : public MifLayer {
public:
    // 构造函数
    MifLayerNormal(const std::string& layerPath,
            MifLayer* copySrcLayer = nullptr);
    // 虚构函数
    ~MifLayerNormal() = default;

    // 依据给定的路径打开Layer
    int open();
    // 依据拷贝源进行复制加载
    int copyLoad();
    // 保存对应的Layer的数据
    int save(const std::string layerPath = "");
    // 对于坐标或者普通的字段进行数值赋值
    int assignWithNumber(const std::string& tagName, const int index,
            const double& val);
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const int index,
            const std::string& val);
    // 获取当前MifItem的某个字段的类型
    int getTagType(const std::string& tagName, syntax::DataType* type,
            bool isAssign = false);
    // 检查当前是否有某一个Tag，如果没有则会添加
    int checkAddTag(const std::string& tagName, int* colID = nullptr,
            bool isAssign = false);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const std::string& tagName, const int index,
            std::string* val);
    // 获取当前MifItem的某个字段的数值
    int getTagVal(const std::string& tagName, const int index,
            double* val);
    // 获取当前MifItem的某个字段的类型
    int getGeometry(wsl::Geometry** val, const int index);
};

class MifItem {
public:
    // 构造函数
    MifItem(const int index, MifLayer* srcLayer, MifLayer* targetLayer,
            MifLayer::ItemInfo* info);
    // 析构函数
    ~MifItem() = default;

    // 依据字段名进数值行赋值操作
    int assignWithNumber(const std::string& tagName, const double& val);
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const std::string& val);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const std::string& tagName, std::string* val);
    // 获取当前MifItem的某个字段的浮点数值
    int getTagVal(const std::string& tagName, double* val);
    // 获取当前MifItem地理坐标信息
    int getGeometry(wsl::Geometry** val);
    // 查找是否存在一个动态组，没有则会建立并插入动态组
    int findBuildDynamicGroup(Group** groupPtr, int64_t dynamicGroupKey,
            Group* itemGroup);
    // 查找已有的运算结果
    int findInsertProcessResult(bool** resultPtr, int64_t processKey);

public:
    // 当前MifItem所属的MifLayer
    MifLayer* srcLayer_ = nullptr;
    // 当前MifItem对应的目标MifLayer
    MifLayer* targetLayer_ = nullptr;
    // 当前MifItem在MifLayer中的索引
    const int index_;

private:
    // 用于缓存复杂运算的运算结果
    std::map<int64_t, bool> processResultCache_;
    // 当前MifItem的info
    MifLayer::ItemInfo* info_ = nullptr;
};

} // namespace condition_assign

#endif // MIFTYPE_H
