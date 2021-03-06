// Proof mocking side effects with custom action for output parameter.
// Not working, must be reworked.
// ./compile.sh proof*.cpp [-DEXECUTABLE]
// Author: 2021-03-01 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

int globalValue = 999;

class WorkerInterface {
public:
    virtual ~WorkerInterface() {}
    virtual int working(int*&) = 0;
};

// To mock a free global function later we need a global pointer then
// (not necessary when using classes with inheritance).
WorkerInterface* ptrWorkerObj;

class Worker : public WorkerInterface {
public:
    virtual ~Worker() {}
    int working(int*& out) {
        out = &globalValue;
        return EXIT_SUCCESS;
    }
};

class Caller {
private:
    int* retparm;
public:
    int calling() {
        int ret = ptrWorkerObj->working(retparm);
        std::cout << "workerObj.working returned '" << ret
                  << "' and parameter pointing to '"
                  << *retparm << "'\n";
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
    MOCK_METHOD(int, working, (int*&));
};

// Custom action to return the output parameter
struct ReturnParamPtrRef {
    template <typename T>
    T operator()(T*& arg) {
        arg = &globalValue;
        return EXIT_SUCCESS;
    }
};


TEST(MockTestSuite, working)
{
    using ::testing::_;

    WorkerMock workerMockObj;
    ptrWorkerObj = &workerMockObj;

    EXPECT_CALL(workerMockObj, working(_))
        .WillOnce(ReturnParamPtrRef());

    Caller callerObj;
    EXPECT_EQ(callerObj.calling(), EXIT_SUCCESS);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
