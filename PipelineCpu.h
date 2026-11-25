//
// Created by kubiii on 11/12/26.
//

#ifndef PIPELINE_SIMULATOR_PIPELINECPU_H
#define PIPELINE_SIMULATOR_PIPELINECPU_H

#include "Memory.h"
#include "PipelineRegisters.h"
#include "RegisterFile.h"
#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>
#include "BranchPredictor.h"
#include <memory>
#include <utility>
namespace pipeline_sim
{
    struct PerformanceCounters
    {
        std::uint64_t cycles{0};
        std::uint64_t retired_instructions{0};
        std::uint64_t load_use_stalls{0};
        std::uint64_t branches{0};
        std::uint64_t taken_branches{0};
        std::uint64_t branch_misprediction{0};

    };
    struct CycleTrace
    {
        std::uint64_t cycle{0};
        std::optional<std::uint32_t> if_pc;
        std::optional<std::uint32_t> id_pc;
        std::optional<std::uint32_t> ex_pc;
        std::optional<std::uint32_t> mem_pc;
        std::optional<std::uint32_t> wb_pc;
        bool load_use_stall{false};
        bool branch_redirected{false};
    };
    class PipelineCpu
    {
    public:
        explicit PipelineCpu(std::size_t memory_size, std::unique_ptr<BranchPredictor> predictor = std::make_unique<AlwaysNotTakenPredictor>());
        const PerformanceCounters& counters() const noexcept;
        const std::vector<CycleTrace>& trace() const noexcept;
        void tick();

        std::uint32_t pc() const noexcept;
        RegisterFile& registers() noexcept;
        Memory& memory() noexcept;
    private:
        RegisterFile registers_;
        Memory memory_;
        std::uint32_t pc_{0};
        IfIdRegister if_id_{};
        IdExRegister id_ex_{};
        ExMemRegister ex_mem_{};
        MemWbRegister mem_wb_{};
        PerformanceCounters counters_{};
        std::vector<CycleTrace> traces_{};
        std::unique_ptr<BranchPredictor> predictor_;
    };
} // pipeline_sim

#endif //PIPELINE_SIMULATOR_PIPELINECPU_H
