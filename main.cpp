#include <assert.h>
#include <iostream>

#include "Memory.h"
#include "RegisterFile.h"
using namespace std;
using namespace pipeline_sim;
int main() {
    RegisterFile rf;
    assert(rf.read(1) == 0);
    rf.write(1,42);

    assert(rf.read(1)==42);
    rf.write(0,99);
    assert(rf.read(0) == 0);
    rf.write(31,123);
    assert(rf.read(31) == 123);

    return 0;
}