//
// Created by kubiii on 11/10/26.
//

#ifndef PIPELINE_SIMULATOR_PIPELINEREGISTERS_H
#define PIPELINE_SIMULATOR_PIPELINEREGISTERS_H
#include <cstdint>
#include "Instruction.h"
namespace pipeline_sim
{
    struct IfIdRegister
    {
        bool valid{false};
        std::uint32_t pc{0};

        std::uint32_t raw_instruction{0};
    };

    struct IdExRegister
    {
        bool valid{false};
        std::uint32_t pc{0};

        DecodedInstruction instruction{};

        std::uint32_t rs1_value{0};
        std::uint32_t rs2_value{0};
    };

    struct ExMemRegister
    {
        bool valid{false};
        std::uint32_t pc{0};

        DecodedInstruction instruction{};
        std::uint32_t alu_result{0};
        std::uint32_t store_value{0};

        bool branch_taken{false};
        std::uint32_t branch_target{0};
    };

    struct MemWbRegister
    {
        bool valid{false};
        std::uint32_t pc{0};

        DecodedInstruction instruction{};

        bool write_enable{false};
        std::uint32_t write_value{0};
    };
}
#endif //PIPELINE_SIMULATOR_PIPELINEREGISTERS_H
