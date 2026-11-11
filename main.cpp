#include <assert.h>
#include "SequentialCpu.h"
using namespace std;

void testSequentialCPU()
{
    pipeline_sim::SequentialCpu cpu{256};

    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00700113u); // addi x2, x0, 7
    cpu.memory().store_word(8,  0x002081B3u); // add  x3, x1, x2
    cpu.memory().store_word(12, 0x40118233u); // sub  x4, x3, x1
    cpu.memory().store_word(16, 0x04402023u); // sw   x4, 64(x0)
    cpu.memory().store_word(20, 0x04002283u); // lw   x5, 64(x0)
    cpu.memory().store_word(24, 0x00428463u); // beq  x5, x4, 8
    cpu.memory().store_word(28, 0x06300313u); // addi x6, x0, 99
    cpu.memory().store_word(32, 0x02A00313u); // addi x6, x0, 42

    // Seven instructions through BEQ, then the instruction at address 32.
    for (int i = 0; i < 8; ++i) {
        cpu.step();
    }

    assert(cpu.registers().read(0) == 0);
    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 7);
    assert(cpu.registers().read(3) == 12);
    assert(cpu.registers().read(4) == 7);
    assert(cpu.registers().read(5) == 7);
    assert(cpu.registers().read(6) == 42);

    assert(cpu.memory().load_word(64) == 7);
    assert(cpu.pc() == 36);
}
int main() {
    testSequentialCPU();
    return 0;
}