// Checking pointer and reference handling returned by a function parameter
// g++ -std=c++11 -pedantic-errors -Wall proof?.cpp
// Author: 2021-02-28 - Ingo Höft <Ingo@Hoeft-online.de>

#include <iostream>

struct Struc {
    struct Struc* next;   // pointer to next item
    const char* name;
};


class Worker {
private:
    const char* struc2name = "second entry";
    Struc struc2 = {nullptr, struc2name};
    const char* struc1name = "first entry";
    Struc struc1 = {&struc2, struc1name};
public:
    int working(Struc*& out) {
        out = &struc1;
        return 0;
    }
};


int main(int argc, char **argv)
{
    Struc* retparm;

    Worker workerObj;
    workerObj.working(retparm);
    std::cout << "retparm: '" << retparm << "'\n"
              << "retparm->name:       '" << retparm->name << "'\n"
              << "retparm->next->name: '" << retparm->next->name << "'\n";
    return 0;
}
