// Mock network interface structures
// ./compile_all.sh
// Author: 2021-03-06 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../../tools/ifaddrs.cpp"

extern "C" {
    int mygetifaddrs();
}

// --- mock getifaddrs -------------------------------------
class getifaddrsInterface {
public:
    virtual ~getifaddrsInterface() {}
    virtual int getifaddrs(struct ifaddrs**) = 0;
};

class GetifaddrsMock : public getifaddrsInterface {
public:
    virtual ~GetifaddrsMock() {}
    MOCK_METHOD(int, getifaddrs, (struct ifaddrs**), (override));
};

// Pointer Will be initialized later in the test body
// For further information look at https://stackoverflow.com/a/66498073/5014688
getifaddrsInterface* ptrGetifaddrsMockObj = nullptr;
int getifaddrs(struct ifaddrs** ifap) {
    return ptrGetifaddrsMockObj->getifaddrs(ifap);
}

// --- mock freeifaddrs ------------------------------------
class FreeifaddrsInterface {
public:
    virtual ~FreeifaddrsInterface() {}
    virtual void freeifaddrs(struct ifaddrs*) = 0;
};

class FreeifaddrsMock : public FreeifaddrsInterface {
public:
    virtual ~FreeifaddrsMock() {}
    MOCK_METHOD(void, freeifaddrs, (struct ifaddrs*), (override));
};

FreeifaddrsInterface* ptrFreeifaddrsMockObj = nullptr;
void freeifaddrs(struct ifaddrs* ifap) {
    return ptrFreeifaddrsMockObj->freeifaddrs(ifap);
}


TEST(MockTestSuite, getifaddrs_and_freeifaddrs)
{
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DoAll;
    using ::testing::SetArgPointee;

    struct ifaddrs* ifaddr = nullptr;

    Ifaddr4 ifaddr4;
    ifaddr = ifaddr4.get();
    //ifaddr->ifa_name = (char*)"mock";

    GetifaddrsMock getifaddrsMockObj;
    ptrGetifaddrsMockObj = &getifaddrsMockObj;

    FreeifaddrsMock freeifaddrsMockObj;
    ptrFreeifaddrsMockObj = &freeifaddrsMockObj;

    EXPECT_CALL(getifaddrsMockObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));

    EXPECT_CALL(freeifaddrsMockObj, freeifaddrs(ifaddr))
        .Times(1);

    EXPECT_EQ(mygetifaddrs(), EXIT_SUCCESS);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
