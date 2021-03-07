#include "gtest/gtest.h"
#include "gmock/gmock.h"

extern "C" {
    int myStrerror();
}

class strerrorInterface {
public:
    virtual ~strerrorInterface() {}
    virtual char* strerror(int) = 0;
};

class strerrorMock : public strerrorInterface {
public:
    virtual ~strerrorMock() override {}
    MOCK_METHOD(char*, strerror, (int), (override));
};

// We need a pointer to the Mock Object here in the global mocked function
// so we can return its result. The Mock Object must be defined later in the
// test body, otherwise we get a memory leak error. As far as the Mock Object
// is defined, we set this pointer to it.
strerrorMock* ptrStrerrorMockObj = nullptr;
char* strerror(int error_number)
{
    std::cout << "mocked strerror function called\n";
    return ptrStrerrorMockObj->strerror(error_number);
}


TEST(MockTestSuite, strerror)
{
    char response[] = "mocked strerror function";

    strerrorMock strerrorMockObj;
    // Set global pointer to this Mock Object.
    ptrStrerrorMockObj = &strerrorMockObj;

    EXPECT_CALL(strerrorMockObj, strerror(0))
        .WillOnce(::testing::Return(response));

    EXPECT_EQ(myStrerror(), 0);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
