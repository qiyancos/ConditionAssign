#include <gtest/gtest.h>
#include <string>
#include "Layer.h"
#include "Config.h"

using namespace std;
using namespace basemap;


class LayerTest : public ::testing::Test
{
protected:
    Layer layer;

    virtual void SetUp()
    {
        layer.Load("C_POI");
    }

    virtual void TearDown()
    {
        layer.Save("C_POI_OUT");
    }
};

TEST_F(LayerTest, Load)
{
    ASSERT_EQ(layer.mif_.mid.size(), 8);
    
    int col_catalog = layer.mif_.get_col_pos("catalog");
    ASSERT_EQ(col_catalog, 3);
    EXPECT_STREQ(layer.mif_.mid[0][col_catalog].c_str(), "0207");
}

TEST_F(LayerTest, Save)
{
    ASSERT_TRUE(layer.Save("C_POI_OUT"));
}

TEST_F(LayerTest, AssignByConfigGroup)
{
    ConfigList cg;
    ASSERT_TRUE(ParseConfigGroupByFile(cg, "conditions.conf"));
    ASSERT_EQ(layer.mif_.mid.size(), 8);
    ASSERT_FALSE(layer.AssignByConfigList(cg, 0));
    ASSERT_TRUE(layer.AssignByConfigList(cg, 1));
    ASSERT_TRUE(layer.AssignByConfigList(cg, 7));
    EXPECT_STREQ(layer.mif_.mid[0][3].c_str(), "0207");
    EXPECT_STREQ(layer.mif_.mid[0][11].c_str(), "\"\"");
    EXPECT_STREQ(layer.mif_.mid[1][3].c_str(), "\"0101\"");
    EXPECT_STREQ(layer.mif_.mid[1][11].c_str(), "\"1\"");
    EXPECT_STREQ(layer.mif_.mid[7][11].c_str(), "\"2\"");
}