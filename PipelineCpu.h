//
// Created by kubiii on 11/12/26.
//

#ifndef PIPELINE_SIMULATOR_PIPELINECPU_H
#define PIPELINE_SIMULATOR_PIPELINECPU_H

#include "Memory.h"
#include "PipelineRegisters.h"
#include "RegisterFile.h"
namespace pipeline_sim
{
    class PipelineCpu
    {
    public:
        explicit PipelineCpu(std::size_t memory_size);

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
    };
} // pipeline_sim

#endif //PIPELINE_SIMULATOR_PIPELINECPU_H
