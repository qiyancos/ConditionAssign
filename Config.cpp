#include "Config.h"
#include "ConditionAssign.h"

namespace condition_assign {

int ConfigItem::score() {
    if (score_ == -1)
        int tempScore = 0;
        CHECK_RET(tempScore = calculateScore(conditions_),
                "Failed to calculate score of conditions.");
        score_ += tempScore;
        CHECK_RET(tempScore = calculateScore(assigns_),
                "Failed to calculate score of conditions.");
        score_ += tempScore;
    }
    return score_;
}

int ConfigItem::addOperator(syntax::Operator* op) {
    operators_.push_back(op);
    return 0;
}

int ConfigItem::addCondition(syntax::Node* newCondition) {
    conditions_.push_back(newCondition);
    return 0;
}

int ConfigItem::addAssign(syntax::Node* newAssign) {
    assigns_.push_back(newAssign);
    return 0;
}

ConfigItem::~ConfigItem() {
    for (syntax::Node* node : conditions_) {
        delete node;
    }
    for (syntax::Node* node : assigns_) {
        delete node;
    }
    for (syntax::Operator* op : operators_) {
        delete op;
    }
}

int parseConfigLine(const std::string& line, ConfigSubGroup* subGroup,
        ResourcePool* resourcePool,
        std::vector<std::pair<std::string, Group**>*>* newGroups) {
    std::stack<char> bracketStack;
    // TODO
}

int parseGroupInfo(const std::string& content, ResourcePool* resourcePool,
        std::pair<int, Group*>* itemGroup, std::pair<int, Group*>* typeGroup) {
}

} // namespace consition_assisgn
