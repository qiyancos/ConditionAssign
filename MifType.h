#ifndef MIFTYPE_H
#define MIFTYPE_H

#include <vector>
#include <map>
#include <string>
#include <set>

namespace condition_assign {

class MifLayer {
public:
    enum AccessType {Read, Write};
    // 依据给定的路径打开Layer
    virtual int open(const std::string& layerPath) = 0;
    // 保存对应的Layer的数据
    virtual int save() = 0;
    // 依据字段名进行赋值操作
    virtual int assignWithTag(const std::string& tagName,
            const int index, const std::string& val) = 0;
    // 获取当前MifItem的某个字段的内容
    virtual int getTagVal(const stg::string& tagName, const int index,
            std::string* val) = 0;
    // 获取当前MifItem地理坐标信息
    virtual int getGeometry(wsl::Geometry** val, const int index) = 0;
    
    // 构造函数
    MifLayer(AccessType type);
    // 虚析构函数
    virtual ~MifLayer() = 0;

protected:
    // 获取对应Layer中Tag的Col索引，对于读操作如果没有会返回-1
    // 对于写操作如果没有会新建，读取或者新建失败会返回-1
    virtual int getTagColID(const std::string& tagName, int* colID,
            AccessType accessType) = 0;

private:
    // 当前MifLayer的处理类型
    const AccessType type_;
    // mif数据锁
    std::mutex mifLock_;
    // Layer数据结构
    wgt::MIF mif_;
    // mif中的item的个数
    int mifSize_;
    // 对应文件的路径
    std::string layerPath_;
    // 缓存Tag名称到对应索引的映射关系锁
    std::mutex tagColCacheLock_;
    // 缓存Tag名称到对应索引的映射关系，快速查找
    std::map<std::string, int> tagColCache_;
}

class MifLayerReadOnly : public MifLayer {
public:
    // 依据给定的路径打开Layer
    int open(const std::string& layerPath);
    // 保存对应的Layer的数据
    int save();
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const int index,
            const std::string& val);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const stg::string& tagName, const int index,
            std::string* val);
    // 获取当前MifItem的某个字段的类型
    int getGeometry(wsl::Geometry** val, const int index);
    
    // 构造函数
    MifLayerReadOnly();
    // 虚构函数
    ~MifLayerReadOnly();

protected:
    // 获取对应Layer中Tag的Col索引，对于读操作如果没有会返回-1
    // 对于写操作如果没有会新建，读取或者新建失败会返回-1
    int getTagColID(const std::string& tagName, int* colID,
            AccessType accessType) = 0;
}

class MifLayerReadWrite : public MifLayer {
public:
    // 依据给定的路径打开Layer
    int open(const std::string& layerPath);
    // 保存对应的Layer的数据
    int save();
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const int index,
            const std::string& val);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const stg::string& tagName, const int index,
            std::string* val);
    // 获取当前MifItem地理坐标信息
    int getGeometry(wsl::Geometry** val, const int index);
    
    // 构造函数
    MifLayerReadWrite();
    // 虚构函数
    ~MifLayerReadWrite();

protected:
    // 获取对应Layer中Tag的Col索引，对于读操作如果没有会返回-1
    // 对于写操作如果没有会新建，读取或者新建失败会返回-1
    int getTagColID(const std::string& tagName, int* colID,
            AccessType accessType) = 0;
}

class MifItem {
public:
    // 构造函数
    MifItem(const MifLayer& layer, const int index);
    // 依据字段名进行赋值操作
    int assignWithTag(const std::string& tagName, const std::string& val);
    // 获取当前MifItem的某个字段的内容
    int getTagVal(const stg::string& tagName, std::string* val);
    // 获取当前MifItem的某个字段的浮点数值
    int getTagVal(const stg::string& tagName, double* val);
    // 获取当前MifItem的某个字段的类型
    int getTagType(const stg::string& tagName, syntax::DataType* type);
    // 获取当前MifItem地理坐标信息
    int getGeometry(wsl::Geometry** val);

private:
    // 当前MifItem所属的MifLayer
    const MifLayer& layer_;
    // 当前MifItem在MifLayer中的索引
    const int inedx_;
    
    // Tag字符串映射缓存的锁
    std::mutex tagStringCacheLock_
    // 缓存的Tag数值(字符串)映射
    std::map<std::string, std::string> tagStringCache_;
    // Tag浮点数映射缓存的锁
    std::mutex tagNumberCacheLock_
    // 缓存的Tag数值(浮点数)映射
    std::map<std::string, double> tagNumberCache_;
    // Tag类型映射缓存的锁
    std::mutex tagTypeCacheLock_
    // 缓存的Tag的数据类型 
    std::map<std::string, syntax::DataType> tagTypeCache_;
    // 缓存的地理坐标信息
    wsl::Geometry* geometry_ = nullptr;
}

} // namespace condition_assign

#endif // MIFTYPE_H
