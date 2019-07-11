#ifndef CONFIG_H_
#define CONFIG_H

#include <string>
#include <vector>
#include <tuple>
#include "Group.h"
#include "Syntax.h"

namespace condition_assign {

class ConfigItem {
public:
private:
    std::vector<syntax::Node> conditions_;
    std::vector<syntax::Node> assigns_;
};

class ConfigGroup {
public:
    ConfigGroup();
    init(const int targetNum);
private:
    std::vector<std::vector<ConfigItem>> group_;
}

class ConfigParser {
public:
    int
};

} // namespace condition_assign

#endif // CONFIG_H
