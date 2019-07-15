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
    int open(const std::string& layerPath);
    // 保存对应的Layer的数据
    int save();
    // 获取对应Layer中Tag的Col索引，对于读操作如果没有会返回-1
    // 对于写操作如果没有会新建，读取或者新建失败会返回-1
    int getTagColID(const std::string& tagName, AccessType accessType);
private:
    // Layer数据结构
    wgt::MIF mif_;
    // 缓存Tag名称到对应索引的映射关系，快速查找
    std::map<std::string, int> tagColCache_;
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
    // 判断当前MifItem的某一个Tag是否兼容给定的类型
    bool isTagTypeCompatable(const std::string& tagName,
            const syntax::DataType type);
private:
    // 当前MifItem所属的MifLayer
    const MifLayer& layer_;
    // 当前MifItem在MifLayer中的索引
    const int inedx_;
    // 缓存的Tag数值(字符串)映射
    std::map<std::string, std::string> tagStringCache_;
    // 缓存的Tag数值(浮点数)映射
    std::map<std::string, double> tagNumberCache_;
    // 缓存的Tag的数据类型 
    std::map<std::string, syntax::DataType> tagTypeCache_;
    // 缓存的地理坐标信息
    wsl::Geometry* geometry_ = nullptr;
}

} // namespace condition_assign

#endif // MIFTYPE_H
