// Test with mocked object as parameter to caller program
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

class Worker : public WorkerInterface {
public:
    virtual ~Worker() {}
    int working() {
        return 1;
    };
};

class Caller
{
private:
    WorkerInterface* mWorkObj;
public:
    Caller(WorkerInterface* pWorkerObj)
        : mWorkObj(pWorkerObj){}

    int calling() {
        int ret = mWorkObj->working();
        std::cout << "workerObj.working returned = " << ret << "\n";
        return ret;
    }
};

#if defined (EXECUTABLE)
int main(int argc, char **argv)
{
    Worker workerObj;
    Caller callerObj(&workerObj);
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
    EXPECT_CALL(workerMockObj, working());

    Caller callerObj(&workerMockObj);
    EXPECT_EQ(callerObj.calling(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
