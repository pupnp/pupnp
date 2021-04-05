// Author: 2021-03-06 - Ingo Höft <Ingo@Hoeft-online.de>
// Last modified: 2021-04-06

#include "gtest/gtest.h"

// simple testsuite without fixtures
//----------------------------------
TEST(WorkflowTestSuite, skip_test_on_env_var)
{
    char* github_action = std::getenv("GITHUB_ACTIONS");
    if(github_action) { GTEST_SKIP()
        << "  due to issues with googlemock";
    }

    EXPECT_TRUE(false);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
