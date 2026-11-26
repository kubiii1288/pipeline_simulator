#include <assert.h>

#include "PipelineCpu.h"
#include "SequentialCpu.h"
#include <utility>
#include <memory>
#include <cstdint>
#include <iostream>
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
    assert(counters.branch_misprediction == 1);
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
    assert(counters.branch_misprediction == 0);
}

void testCorrectTakenPrediction()
{
    pipeline_sim::PipelineCpu cpu{
        256, std::make_unique<pipeline_sim::AlwaysTakenPredictor>()};

    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00500113u); // addi x2, x0, 5
    cpu.memory().store_word(8,  0x00208663u); // beq  x1, x2, 12
    cpu.memory().store_word(12, 0x06300193u); // addi x3, x0, 99 (skipped)
    cpu.memory().store_word(16, 0x05800213u); // addi x4, x0, 88 (skipped)
    cpu.memory().store_word(20, 0x02A00293u); // addi x5, x0, 42 (target)

    for (std::uint32_t address = 24; address <= 36; address += 4) {
        cpu.memory().store_word(address, 0x00000013u); // NOP
    }

    for (int cycle = 0; cycle < 8; ++cycle) {
        cpu.tick();
    }

    const auto& rows = cpu.trace();
    assert(rows.size() == 8);
    assert(rows[3].if_pc == 20u);         // Cycle 4 fetches the target
    assert(rows[4].ex_pc == 8u);          // Cycle 5 resolves the branch
    assert(!rows[4].branch_redirected);  // The prediction was correct
    assert(cpu.counters().branch_misprediction == 0);

    assert(cpu.registers().read(3) == 0);
    assert(cpu.registers().read(4) == 0);
    assert(cpu.registers().read(5) == 42);
}

void testIncorrectTakenPrediction()
{
    pipeline_sim::PipelineCpu cpu{
        256, std::make_unique<pipeline_sim::AlwaysTakenPredictor>()};

    cpu.memory().store_word(0,  0x00500093u); // addi x1, x0, 5
    cpu.memory().store_word(4,  0x00600113u); // addi x2, x0, 6
    cpu.memory().store_word(8,  0x00208663u); // beq  x1, x2, 12 (not taken)
    cpu.memory().store_word(12, 0x06300193u); // addi x3, x0, 99 (fallthrough)
    cpu.memory().store_word(16, 0x05800213u); // addi x4, x0, 88
    cpu.memory().store_word(20, 0x02A00293u); // addi x5, x0, 42 (predicted target)

    for (std::uint32_t address = 24; address <= 32; address += 4) {
        cpu.memory().store_word(address, 0x00000013u); // NOP
    }

    for (int cycle = 0; cycle < 11; ++cycle) {
        cpu.tick();
    }

    const auto& rows = cpu.trace();
    assert(rows.size() == 11);
    assert(rows[3].if_pc == 20u);        // Cycle 4 follows the prediction
    assert(rows[4].ex_pc == 8u);         // Cycle 5 resolves the BEQ
    assert(rows[4].branch_redirected);
    assert(rows[5].if_pc == 12u);        // Cycle 6 fetches the fallthrough
    assert(cpu.counters().branch_misprediction == 1);

    assert(cpu.registers().read(3) == 99);
    assert(cpu.registers().read(4) == 88);
    assert(cpu.registers().read(5) == 0);
}

void testBimodalPredictorLearns()
{
    pipeline_sim::BimodalPredictor predictor;
    constexpr std::uint32_t branch_pc = 24u;

    const auto first = predictor.predict(branch_pc);
    assert(!first.taken);
    assert(first.metadata == 6u);

    predictor.update(branch_pc, first, true);

    const auto second = predictor.predict(branch_pc);
    assert(second.taken);
}

void testBimodalCountdownLoop()
{
    pipeline_sim::PipelineCpu cpu{
        256, std::make_unique<pipeline_sim::BimodalPredictor>()};

    cpu.memory().store_word(0,  0x00300093u); // addi x1, x0, 3
    cpu.memory().store_word(4,  0xFFF08093u); // addi x1, x1, -1
    cpu.memory().store_word(8,  0x00008463u); // beq  x1, x0, +8 (exit to 16)
    cpu.memory().store_word(12, 0xFE000CE3u); // beq  x0, x0, -8 (loop to 4)
    cpu.memory().store_word(16, 0x02A00113u); // addi x2, x0, 42

    for (std::uint32_t address = 20; address <= 32; address += 4) {
        cpu.memory().store_word(address, 0x00000013u); // NOP
    }

    for (int cycle = 0; cycle < 18; ++cycle) {
        cpu.tick();
    }

    assert(cpu.registers().read(1) == 0);
    assert(cpu.registers().read(2) == 42);

    const auto& counters = cpu.counters();
    assert(counters.branches == 5);
    assert(counters.branch_misprediction == 2);
}

void testGshareTrainsSavedSlot()
{
    pipeline_sim::GsharePredictor predictor;

    const auto older = predictor.predict(0u);
    const auto saved = predictor.predict(24u);
    assert(saved.metadata == 6u);
    assert(!saved.taken);

    predictor.update(0u, older, true);  // History becomes 1.
    assert(predictor.predict(24u).metadata == 7u);

    predictor.update(24u, saved, true);  // Train slot 6. History becomes 3.

    const auto probe = predictor.predict(20u);
    assert(probe.metadata == 6u);
    assert(probe.taken);
}

void testGshareCountdownLoop()
{
    pipeline_sim::PipelineCpu cpu{
        256, std::make_unique<pipeline_sim::GsharePredictor>()};

    cpu.memory().store_word(0,  0x00300093u); // addi x1, x0, 3
    cpu.memory().store_word(4,  0xFFF08093u); // addi x1, x1, -1
    cpu.memory().store_word(8,  0x00008463u); // beq  x1, x0, +8 (exit to 16)
    cpu.memory().store_word(12, 0xFE000CE3u); // beq  x0, x0, -8 (loop to 4)
    cpu.memory().store_word(16, 0x02A00113u); // addi x2, x0, 42

    for (std::uint32_t address = 20; address <= 32; address += 4) {
        cpu.memory().store_word(address, 0x00000013u); // NOP
    }

    for (int cycle = 0; cycle < 22; ++cycle) {
        cpu.tick();
    }

    assert(cpu.registers().read(1) == 0);
    assert(cpu.registers().read(2) == 42);

    const auto& counters = cpu.counters();
    assert(counters.branches == 5);
    assert(counters.branch_misprediction == 4);
}

void testCorrelatedBranchProgram()
{
    pipeline_sim::SequentialCpu cpu{256};

    cpu.memory().store_word(0,  0x00400093u); // addi x1, x0, 4
    cpu.memory().store_word(4,  0x00100113u); // addi x2, x0, 1
    cpu.memory().store_word(8,  0x403101B3u); // sub  x3, x2, x3
    cpu.memory().store_word(12, 0x00018463u); // beq  x3, x0, +8
    cpu.memory().store_word(16, 0x00120213u); // addi x4, x4, 1
    cpu.memory().store_word(20, 0x00130313u); // addi x6, x6, 1
    cpu.memory().store_word(24, 0x00138393u); // addi x7, x7, 1
    cpu.memory().store_word(28, 0x00018463u); // beq  x3, x0, +8
    cpu.memory().store_word(32, 0x00128293u); // addi x5, x5, 1
    cpu.memory().store_word(36, 0xFFF08093u); // addi x1, x1, -1
    cpu.memory().store_word(40, 0x00008463u); // beq  x1, x0, +8  → 48
    cpu.memory().store_word(44, 0xFC000EE3u); // beq  x0, x0, -36 → 8
    cpu.memory().store_word(48, 0x02A00413u); // addi x8, x0, 42

    for (int step = 0; step < 100 && cpu.registers().read(8) != 42; ++step) {
        cpu.step();
    }

    assert(cpu.registers().read(8) == 42); // Reached the completion marker
    assert(cpu.registers().read(1) == 0);  // Finished all four iterations
    assert(cpu.registers().read(4) == 2);  // Count A
    assert(cpu.registers().read(5) == 2);  // Count B
}

pipeline_sim::PerformanceCounters runCorrelatedBranchBenchmark(
    std::unique_ptr<pipeline_sim::BranchPredictor> predictor)
{
    pipeline_sim::PipelineCpu cpu{256, std::move(predictor)};

    cpu.memory().store_word(0,  0x3E800093u); // addi x1, x0, 1000
    cpu.memory().store_word(4,  0x00100113u); // addi x2, x0, 1
    cpu.memory().store_word(8,  0x403101B3u); // sub  x3, x2, x3
    cpu.memory().store_word(12, 0x00018463u); // beq  x3, x0, +8
    cpu.memory().store_word(16, 0x00120213u); // addi x4, x4, 1
    cpu.memory().store_word(20, 0x00130313u); // addi x6, x6, 1
    cpu.memory().store_word(24, 0x00138393u); // addi x7, x7, 1
    cpu.memory().store_word(28, 0x00018463u); // beq  x3, x0, +8
    cpu.memory().store_word(32, 0x00128293u); // addi x5, x5, 1
    cpu.memory().store_word(36, 0xFFF08093u); // addi x1, x1, -1
    cpu.memory().store_word(40, 0x00008463u); // beq  x1, x0, +8 → 48
    cpu.memory().store_word(44, 0xFC000EE3u); // beq  x0, x0, -36 → 8
    cpu.memory().store_word(48, 0x02A00413u); // addi x8, x0, 42

    for (std::uint32_t address = 52; address <= 64; address += 4) {
        cpu.memory().store_word(address, 0x00000013u); // NOP
    }

    for (int tick = 0; tick < 30'000 && cpu.registers().read(8) != 42; ++tick) {
        cpu.tick();
    }

    assert(cpu.registers().read(8) == 42);
    assert(cpu.registers().read(1) == 0);
    assert(cpu.registers().read(4) == 500);
    assert(cpu.registers().read(5) == 500);
    assert(cpu.counters().retired_instructions == 9'002);

    return cpu.counters();
}

void testCorrelatedBranchBenchmark()
{
    const auto baseline = runCorrelatedBranchBenchmark(
        std::make_unique<pipeline_sim::AlwaysNotTakenPredictor>());
    assert(baseline.branch_misprediction == 2'000);
    assert(baseline.cycles == 13'006);

    const auto bimodal = runCorrelatedBranchBenchmark(
        std::make_unique<pipeline_sim::BimodalPredictor>());
    assert(bimodal.branch_misprediction == 1'002);
    assert(bimodal.cycles == 11'010);

    const auto gshare = runCorrelatedBranchBenchmark(
    std::make_unique<pipeline_sim::GsharePredictor>());
    const auto print_result = [](const char* name,
                                 const pipeline_sim::PerformanceCounters& counters) {
        const double cpi =
            static_cast<double>(counters.cycles) / counters.retired_instructions;

        std::cout << name << ": " << counters.cycles << " cycles, "
                  << counters.branch_misprediction << " mispredictions, "
                  << cpi << " CPI\n";
    };

    print_result("Always not taken (baseline)", baseline);
    print_result("Bimodal", bimodal);
    print_result("Gshare", gshare);
}
void testCountdownBenchmark()
{
    const auto run = [](const char* name,
                        std::unique_ptr<pipeline_sim::BranchPredictor> predictor) {
        pipeline_sim::PipelineCpu cpu{256, std::move(predictor)};

        cpu.memory().store_word(0,  0x3E800093u); // addi x1, x0, 1000
        cpu.memory().store_word(4,  0xFFF08093u); // addi x1, x1, -1
        cpu.memory().store_word(8,  0x00008463u); // beq x1, x0, +8  → 16
        cpu.memory().store_word(12, 0xFE000CE3u); // beq x0, x0, -8  → 4
        cpu.memory().store_word(16, 0x02A00113u); // addi x2, x0, 42

        for (std::uint32_t address = 20; address <= 32; address += 4) {
            cpu.memory().store_word(address, 0x00000013u); // NOP
        }

        for (int tick = 0; tick < 10'000 && cpu.registers().read(2) != 42; ++tick) {
            cpu.tick();
        }

        assert(cpu.registers().read(2) == 42);
        assert(cpu.registers().read(1) == 0);

        const auto& counters = cpu.counters();
        assert(counters.retired_instructions == 3'001);

        const double cpi =
            static_cast<double>(counters.cycles) / counters.retired_instructions;

        std::cout << name << ": " << counters.cycles << " cycles, "
                  << counters.branch_misprediction << " mispredictions, "
                  << cpi << " CPI\n";
    };

    run("Countdown / Always not taken",
        std::make_unique<pipeline_sim::AlwaysNotTakenPredictor>());
    run("Countdown / Bimodal",
        std::make_unique<pipeline_sim::BimodalPredictor>());
    run("Countdown / Gshare",
        std::make_unique<pipeline_sim::GsharePredictor>());
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
    testCorrectTakenPrediction();
    testIncorrectTakenPrediction();
    testBimodalPredictorLearns();
    testBimodalCountdownLoop();
    testGshareTrainsSavedSlot();
    testGshareCountdownLoop();
    testCorrelatedBranchProgram();
    testCorrelatedBranchBenchmark();
    testCountdownBenchmark();
    return 0;
}