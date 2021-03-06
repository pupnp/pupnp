// Test side effects with mocked object as parameter to caller program
// ./compile.sh proof*.cpp [-DEXECUTABLE]
// Author: 2021-03-05 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>

class WorkerInterface {
public:
    virtual ~WorkerInterface() {}
    virtual int working(int*) = 0;
};

class Worker : public WorkerInterface {
public:
    virtual ~Worker() {}
    int working(int* setparm) override {
        *setparm = 123;
        return 1;
    };
};

class Caller {
private:
    WorkerInterface* mWorkObj;
    int param;
public:
    Caller(WorkerInterface* pWorkerObj)
        : mWorkObj(pWorkerObj){}

    int calling() {
        int ret = mWorkObj->working(&param);
        std::cout << "workerObj.working returned = " << ret
                  << " and param = " << param << "\n";
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
    MOCK_METHOD(int, working, (int*), (override));
};


TEST(MockTestSuite, working)
{
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DoAll;
    using ::testing::SetArgPointee;

    WorkerMock workerMockObj;
    EXPECT_CALL(workerMockObj, working(_))
        .WillOnce(DoAll(SetArgPointee<0>(247), Return(0)));

    Caller callerObj(&workerMockObj);
    EXPECT_EQ(callerObj.calling(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
