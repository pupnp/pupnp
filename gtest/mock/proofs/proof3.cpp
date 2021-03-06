// Test with global pointer to the mocked object.
// That is what we need to mock a global free function from a C library.
// ./compile.sh proof*.cpp [-DEXECUTABLE]
// Author: 2021-03-01 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>

class WorkerInterface {
public:
    virtual ~WorkerInterface() {}
    virtual int working() = 0;
};

WorkerInterface* ptrWorkerObj;

class Worker : public WorkerInterface {
public:
    virtual ~Worker() {}
    int working() {
        return 1;
    }
};

class Caller
{
public:
    int calling() {
        int ret = ptrWorkerObj->working();
        std::cout << "workerObj.working returned = " << ret << "\n";
        return ret;
    }
};

#if defined (EXECUTABLE)
int main(int argc, char **argv)
{
    Worker workerObj;
    ptrWorkerObj = &workerObj;

    Caller callerObj;
    return callerObj.calling();
}
#else

class WorkerMock : public WorkerInterface {
public:
    MOCK_METHOD(int, working, ());
};


TEST(MockTestSuite, working)
{
    WorkerMock workerMockObj;
    ptrWorkerObj = &workerMockObj;

    EXPECT_CALL(workerMockObj, working());

    Caller callerObj;
    EXPECT_EQ(callerObj.calling(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
