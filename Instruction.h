//
// Created by kubiii on 11/10/26.
//

#ifndef PIPELINE_SIMULATOR_INSTRUCTION_H
#define PIPELINE_SIMULATOR_INSTRUCTION_H
#include <cstdint>

namespace pipeline_sim
{
    enum class Operation
    {
        Invalid,
        Add,
        Sub,
        Addi,
        Lw,
        Sw,
        Beq
    };

    struct DecodedInstruction
    {
        Operation operation{Operation::Invalid};
        std::uint32_t raw{0};

        std::uint8_t rd{0};
        std::uint8_t rs1{0};
        std::uint8_t rs2{0};
        std::int32_t immediate{0};

    };
    bool uses_rs1(Operation op) noexcept;
    bool uses_rs2(Operation op) noexcept;
    bool writes_register(Operation op) noexcept;
    bool is_load(Operation op) noexcept;
}
#endif //PIPELINE_SIMULATOR_INSTRUCTION_H
