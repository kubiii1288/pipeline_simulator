#include <assert.h>

#include "PipelineCpu.h"
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

void testNegativeOffsetsAndBranches()
{
    pipeline_sim::SequentialCpu cpu{256};

    cpu.memory().store_word(0,  0x08000513u); // addi x10, x0, 128
    cpu.memory().store_word(4,  0xFFF00093u); // addi x1, x0, -1
    cpu.memory().store_word(8,  0xFE152E23u); // sw x1, -4(x10)
    cpu.memory().store_word(12, 0xFFC52103u); // lw x2, -4(x10)
    cpu.memory().store_word(16, 0x00008463u); // beq x1, x0, 8 (not taken)
    cpu.memory().store_word(20, 0xFF800193u); // addi x3, x0, -8
    cpu.memory().store_word(24, 0xFE108EE3u); // beq x1, x1, -4 (taken)

    for (int i = 0; i < 7; ++i) {
        cpu.step();
    }

    assert(cpu.registers().read(1) == 0xFFFFFFFFu);
    assert(cpu.registers().read(2) == 0xFFFFFFFFu);
    assert(cpu.registers().read(3) == 0xFFFFFFF8u);

    assert(cpu.memory().load_word(124) == 0xFFFFFFFFu);

    // Final branch at address 24 jumps backward to address 20.
    assert(cpu.pc() == 20);
}
void testBasicPipeline()
{
    pipeline_sim::PipelineCpu cpu{256};

    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00700113u); // addi x2, x0, 7
    cpu.memory().store_word(8,  0x00900193u); // addi x3, x0, 9

    // addi x0, x0, 0 acts as a NOP.
    cpu.memory().store_word(12, 0x00000013u);
    cpu.memory().store_word(16, 0x00000013u);
    cpu.memory().store_word(20, 0x00000013u);
    cpu.memory().store_word(24, 0x00000013u);

    for (int cycle = 0; cycle < 7; ++cycle) {
        cpu.tick();
    }
    const auto& rows = cpu.trace();
    assert(rows.size() == 7);
    assert(rows[0].cycle == 1);
    assert(rows[0].if_pc == 0u);
    assert(!rows[0].wb_pc.has_value());

    assert(rows[1].id_pc == 0u);
    assert(rows[2].ex_pc == 0u);
    assert(rows[3].mem_pc == 0u);
    assert(rows[4].wb_pc == 0u);

    assert(cpu.registers().read(0) == 0);
    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 7);
    assert(cpu.registers().read(3) == 9);
    assert(cpu.pc() == 28);
}

void testExMemForwarding()
{
    pipeline_sim::PipelineCpu cpu{256};

    cpu.memory().store_word(0, 0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4, 0x00108133u); // add  x2, x1, x1

    cpu.memory().store_word(8,  0x00000013u);
    cpu.memory().store_word(12, 0x00000013u);
    cpu.memory().store_word(16, 0x00000013u);
    cpu.memory().store_word(20, 0x00000013u);

    for (int cycle = 0; cycle < 6; ++cycle) {
        cpu.tick();
    }

    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 10);
}

void testMemWbForwarding()
{
    pipeline_sim::PipelineCpu cpu(256);
    cpu.memory().store_word(0, 0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4, 0x00700193u); // addi x3, x0, 7
    cpu.memory().store_word(8, 0x00108133u); // add  x2, x1, x1
    cpu.memory().store_word(12, 0x00000013u);
    cpu.memory().store_word(16, 0x00000013u);
    cpu.memory().store_word(20, 0x00000013u);
    cpu.memory().store_word(24, 0x00000013u);
    for (int cycle = 0; cycle < 7; ++cycle) {
        cpu.tick();
    }

    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 10);
    assert(cpu.registers().read(3) == 7);
}

void testLoadUseStall()
{
    pipeline_sim::PipelineCpu cpu{256};

    cpu.memory().store_word(64, 21);          // Data to load

    cpu.memory().store_word(0, 0x04002083u);  // lw  x1, 64(x0)
    cpu.memory().store_word(4, 0x00108133u);  // add x2, x1, x1
    cpu.memory().store_word(8,  0x00000013u);
    cpu.memory().store_word(12, 0x00000013u);
    cpu.memory().store_word(16, 0x00000013u);
    cpu.memory().store_word(20, 0x00000013u);
    for (int cycle = 0; cycle < 7; ++cycle) {
        cpu.tick();
    }
    const auto& rows = cpu.trace();
    assert(rows.size() == 7);

    const auto& stall = rows[2];  // Cycle 3
    assert(stall.load_use_stall);
    assert(stall.ex_pc == 0u);    // LW still runs in EX
    assert(!stall.id_pc.has_value());
    assert(!stall.if_pc.has_value());

    const auto& resume = rows[3]; // Cycle 4
    assert(!resume.load_use_stall);
    assert(resume.mem_pc == 0u);  // LW advances to MEM
    assert(resume.id_pc == 4u);   // The dependent ADD finally decodes

    assert(cpu.registers().read(1) == 21);
    assert(cpu.registers().read(2) == 42);
    assert(cpu.pc() == 24);

    const auto& counters = cpu.counters();

    assert(counters.cycles == 7);
    assert(counters.retired_instructions == 2);
    assert(counters.load_use_stalls == 1);
    assert(counters.branches == 0);
    assert(counters.taken_branches == 0);

}

void testTakenBranchFlush()
{
    pipeline_sim::PipelineCpu cpu(256);
    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00500113u); // addi x2, x0, 5
    cpu.memory().store_word(8,  0x00208663u); // beq  x1, x2, 12
    cpu.memory().store_word(12, 0x06300193u); // addi x3, x0, 99
    cpu.memory().store_word(16, 0x05800213u); // addi x4, x0, 88
    cpu.memory().store_word(20, 0x02A00293u); // addi x5, x0, 42
    cpu.memory().store_word(24,  0x00000013u);
    cpu.memory().store_word(28, 0x00000013u);
    cpu.memory().store_word(32, 0x00000013u);
    cpu.memory().store_word(36, 0x00000013u);

    for (int cycle = 0; cycle < 10; ++cycle) {
        cpu.tick();
    }

    const auto& rows = cpu.trace();
    assert(rows.size() == 10);

    const auto& redirect = rows[4];  // Cycle 5
    assert(redirect.ex_pc == 8u);    // BEQ resolves in EX
    assert(redirect.branch_redirected);
    assert(!redirect.id_pc.has_value());
    assert(!redirect.if_pc.has_value());

    assert(!rows[5].branch_redirected);
    assert(rows[5].if_pc == 20u);    // Cycle 6 fetches the target


    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 5);
    assert(cpu.registers().read(3) == 0);  // Flushed
    assert(cpu.registers().read(4) == 0);  // Flushed
    assert(cpu.registers().read(5) == 42); // Branch target executed

    const auto& counters = cpu.counters();

    assert(counters.cycles == 10);
    assert(counters.retired_instructions == 4);
    assert(counters.load_use_stalls == 0);
    assert(counters.branches == 1);
    assert(counters.taken_branches == 1);
}

void testNotTakenBranch()
{
    pipeline_sim::PipelineCpu cpu{256};

    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00600113u); // addi x2, x0, 6
    cpu.memory().store_word(8,  0x00208663u); // beq  x1, x2, 12
    cpu.memory().store_word(12, 0x06300193u); // addi x3, x0, 99
    cpu.memory().store_word(16, 0x05800213u); // addi x4, x0, 88
    cpu.memory().store_word(20, 0x00000013u);
    cpu.memory().store_word(24, 0x00000013u);
    cpu.memory().store_word(28, 0x00000013u);
    cpu.memory().store_word(32, 0x00000013u);

    for (int cycle = 0; cycle < 9; ++cycle) {
        cpu.tick();
    }

    assert(cpu.registers().read(1) == 5);
    assert(cpu.registers().read(2) == 6);
    assert(cpu.registers().read(3) == 99);
    assert(cpu.registers().read(4) == 88);
    assert(cpu.pc() == 36);

    const auto& counters = cpu.counters();

    assert(counters.cycles == 9);
    assert(counters.retired_instructions == 5);
    assert(counters.load_use_stalls == 0);
    assert(counters.branches == 1);
    assert(counters.taken_branches == 0);
}
int main() {
    testSequentialCPU();
    testNegativeOffsetsAndBranches();
    testBasicPipeline();
    testExMemForwarding();
    testMemWbForwarding();
    testLoadUseStall();
    testTakenBranchFlush();
    testNotTakenBranch();
    return 0;
}