#include <gtest/gtest.h>
#include <string>
#include "Config.h"

using namespace std;
using namespace basemap;


TEST(ConfigTest, ParseCondition)
{
    string str("key1=value1");
    Condition c;
    ASSERT_TRUE(ParseCondition(c, str));
    EXPECT_EQ(c.type_, Condition::STR_TYPE);
    EXPECT_EQ(c.opt_, Condition::STR_EQ);
    EXPECT_STREQ(c.key_.c_str(), "key1");
    EXPECT_STREQ(c.value_str_.c_str(), "value1");

    str = "key2 = value2";
    ASSERT_TRUE(ParseCondition(c, str));
    EXPECT_EQ(c.type_, Condition::STR_TYPE);
    EXPECT_EQ(c.opt_, Condition::STR_EQ);
    EXPECT_STREQ(c.key_.c_str(), "key2");
    EXPECT_STREQ(c.value_str_.c_str(), "value2");

    str = "key3=\"value3\"";
    ASSERT_TRUE(ParseCondition(c, str));
    EXPECT_EQ(c.type_, Condition::STR_TYPE);
    EXPECT_EQ(c.opt_, Condition::STR_EQ);
    EXPECT_STREQ(c.key_.c_str(), "key3");
    EXPECT_STREQ(c.value_str_.c_str(), "value3");

    str = "key3 : value3";
    EXPECT_FALSE(ParseCondition(c, str));
}

TEST(ConfigTest, ParseConditionGroup)
{
    string str("key1=value1; key2<> 1");
    ConditionGroup cg;
    ASSERT_TRUE(ParseConditionGroup(cg, str));
    ASSERT_EQ(cg.size(), 2);
    EXPECT_STREQ(cg[0].key_.c_str(), "key1");
    EXPECT_STREQ(cg[0].value_str_.c_str(), "value1");
    EXPECT_EQ(cg[0].type_, Condition::STR_TYPE);
    EXPECT_EQ(cg[0].opt_, Condition::STR_EQ);
    EXPECT_STREQ(cg[1].key_.c_str(), "key2");
    EXPECT_DOUBLE_EQ(cg[1].value_double_, 1.0);
}

TEST(ConfigTest, ParseConditionGroupWithQuot)
{
    string str("key1=\";value1\"; key2<> 1");
    ConditionGroup cg;
    ASSERT_TRUE(ParseConditionGroup(cg, str));
    ASSERT_EQ(cg.size(), 2);
    EXPECT_STREQ(cg[0].key_.c_str(), "key1");
    EXPECT_STREQ(cg[0].value_str_.c_str(), ";value1");
    EXPECT_EQ(cg[0].type_, Condition::STR_TYPE);
    EXPECT_EQ(cg[0].opt_, Condition::STR_EQ);
    EXPECT_STREQ(cg[1].key_.c_str(), "key2");
    EXPECT_DOUBLE_EQ(cg[1].value_double_, 1.0);
}

TEST(ConfigTest, ParseTarget)
{
    string str("field1=value1");
    Target t;
    ASSERT_TRUE(ParseTarget(t, str));
    EXPECT_STREQ(t.field_.c_str(), "field1");
    EXPECT_STREQ(t.value_.c_str(), "value1");

    str = "field2 = value2 ";
    ASSERT_TRUE(ParseTarget(t, str));
    EXPECT_STREQ(t.field_.c_str(), "field2");
    EXPECT_STREQ(t.value_.c_str(), "value2");

    str = "field3 : value3";
    EXPECT_FALSE(ParseTarget(t, str));
}

TEST(ConfigTest, ParseTargetGroup)
{
    string str("field1=value1;field2=value2; field3 = value3");
    TargetGroup tg;
    ASSERT_TRUE(ParseTargetGroup(tg, str));
    ASSERT_EQ(tg.size(), 3);
    EXPECT_STREQ(tg[0].field_.c_str(), "field1");
    EXPECT_STREQ(tg[0].value_.c_str(), "value1");
    EXPECT_STREQ(tg[1].field_.c_str(), "field2");
    EXPECT_STREQ(tg[1].value_.c_str(), "value2");
    EXPECT_STREQ(tg[2].field_.c_str(), "field3");
    EXPECT_STREQ(tg[2].value_.c_str(), "value3");
}

TEST(ConfigTest, ParseConfigGroupByFile)
{
    string in_file("conditions.conf");
    ConfigList cg;
    ASSERT_TRUE(ParseConfigGroupByFile(cg, in_file));
    ASSERT_EQ(cg.size(), 4);
    ASSERT_EQ(cg[0].condition_group_.size(), 2);
    ASSERT_EQ(cg[0].target_group_.size(), 1);
    EXPECT_STREQ(cg[0].condition_group_[0].key_.c_str(), "id");
    EXPECT_STREQ(cg[0].condition_group_[0].value_str_.c_str(), "123456");
    EXPECT_STREQ(cg[0].condition_group_[1].key_.c_str(), "rank");
    EXPECT_DOUBLE_EQ(cg[0].condition_group_[1].value_double_, 100);
    EXPECT_EQ(cg[0].condition_group_[1].opt_, Condition::NUM_GT);
}

TEST(ConfigTest, MatchCondition)
{
    string str("key1=value1");
    Condition c;
    ASSERT_TRUE(ParseCondition(c, str));
    EXPECT_TRUE(MatchCondition("value1", c));
    EXPECT_FALSE(MatchCondition("value2", c));

    str = "key2 >= 0.5";
    ASSERT_TRUE(ParseCondition(c, str));
    EXPECT_FALSE(MatchCondition("0.4", c));
    EXPECT_TRUE(MatchCondition("0.5", c));
    EXPECT_TRUE(MatchCondition("0.6", c));
}

// ---

class ConditionTest : public ::testing::TestWithParam<string> {};

TEST_P(ConditionTest, ConditionFormat)
{
    string condition_str = GetParam();
    Condition c;
    ASSERT_TRUE(ParseCondition(c, condition_str));
    EXPECT_STREQ(c.key_.c_str(), "key");
    EXPECT_NE(c.type_, Condition::INVALID_TYPE);
    EXPECT_NE(c.opt_, Condition::INVALID_OPT);
    if (c.type_ == Condition::NUM_TYPE)
    {
        EXPECT_DOUBLE_EQ(c.value_double_, 1.0);
    }
    else
    {
        EXPECT_STREQ(c.value_str_.c_str(), "value");
    }
}

INSTANTIATE_TEST_CASE_P(ConfigTest, ConditionTest,
    testing::Values(
        "key= value",
        "key !=value",
        " \"key\" %=% \"value\"",
        "key %= value",
        "key =% value",
        "key == 1",
        "key <> 1",
        "key <= 1",
        "key >=1 ",
        "key > 1",
        "key < 1"
    )
);
