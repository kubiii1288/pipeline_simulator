//
// Created by kubiii on 11/10/26.
//

#ifndef PIPELINE_SIMULATOR_SEQUENTIALCPU_H
#define PIPELINE_SIMULATOR_SEQUENTIALCPU_H
#include <cstddef>
#include <cstdint>

#include "RegisterFile.h"
#include "Memory.h"

namespace pipeline_sim
{
    class SequentialCpu
    {
    public:
        explicit SequentialCpu(std::size_t memory_size);
        void step();
        std::uint32_t pc() const noexcept;
        RegisterFile& registers() noexcept;
        Memory& memory() noexcept;
    private:
        RegisterFile registers_;
        Memory memory_;
        std::uint32_t pc_{0};
    };
} // pipeline_sim

#endif //PIPELINE_SIMULATOR_SEQUENTIALCPU_H
