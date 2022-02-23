// This test should always run, reporting no failure
// Author: 2021-03-06 - Ingo Höft
// Last modified: 2021-04-06

#include "gtest/gtest.h"

// for TestSuites linked against the static C library
extern "C" {
	//#include "upnp.h"
}

// two macros to compare values in a range
#define EXPECT_IN_RANGE(VAL, MIN, MAX)	\
	EXPECT_GE((VAL), (MIN));			\
	EXPECT_LE((VAL), (MAX))

#define ASSERT_IN_RANGE(VAL, MIN, MAX)	\
	ASSERT_GE((VAL), (MIN));			\
	ASSERT_LE((VAL), (MAX))


// simple testsuite without fixtures
//----------------------------------
TEST(EmptyTestSuite, empty_gtest)
{
	//GTEST_SKIP();
	//GTEST_SKIP_("to show this feature");

	// SKIP on Github Actions
#ifdef _WIN32
	char* github_action;
	size_t len;
	_dupenv_s(&github_action, &len, "GITHUB_ACTIONS");
#else
	char *github_action = std::getenv("GITHUB_ACTIONS");
#endif
	if(github_action) { GTEST_SKIP()
		<< "  to show this feature";
	}
}


// testsuite with fixtures
//------------------------
class EmptyFixtureTestSuite : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if their bodies would
	// be empty.

	EmptyFixtureTestSuite()
	{
		// You can do set-up work for each test here.
	}

	~EmptyFixtureTestSuite() override
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

		// If the constructor and destructor are not enough for setting up
		// and cleaning up each test, you can define the following methods:

	void SetUp() override
	{
		// Code here will be called immediately after the constructor (right
		// before each test). Have attention to the uppercase 'U' of SetUp().
	}

	void TearDown() override
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Class members declared here can be used by all tests in the test suite
	// for Foo.
};

TEST_F(EmptyFixtureTestSuite, empty_gtest_with_fixture)
{
}


int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
