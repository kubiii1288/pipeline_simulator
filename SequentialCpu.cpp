//
// Created by kubiii on 11/10/26.
//

#include "SequentialCpu.h"
#include "Decoder.h"
#include <stdexcept>
namespace pipeline_sim
{
    SequentialCpu::SequentialCpu(std::size_t memory_size) : memory_(memory_size)
    {
    }

    RegisterFile &SequentialCpu::registers() noexcept
    {
        return registers_;
    }

    std::uint32_t SequentialCpu::pc() const noexcept
    {
       return pc_;
    }

    Memory &SequentialCpu::memory() noexcept
    {
       return memory_;
    }

    void SequentialCpu::step()
    {
        std::uint32_t raw = memory().load_word(pc());

        DecodedInstruction instruction = decode(raw);

        std::uint32_t next_pc = pc_ +4;

        switch (instruction.operation) {
            case Operation::Add :
            {
                const std::uint32_t left = registers().read(instruction.rs1);
                const std::uint32_t right = registers().read(instruction.rs2);
                registers().write(instruction.rd, left + right);
                break;
            }
            case Operation::Sub :
            {
                const std::uint32_t left = registers().read(instruction.rs1);
                const std::uint32_t right = registers().read(instruction.rs2);
                registers().write(instruction.rd, left - right);
                break;
            }

            case Operation::Addi:
            {
               const std::uint32_t left = registers().read(instruction.rs1);
               const std::uint32_t immediate = static_cast<std::uint32_t>(instruction.immediate);
               registers().write(instruction.rd, left + immediate);
               break;
            }

            case Operation::Lw:
            {
                const std::uint32_t base = registers().read(instruction.rs1);
                const std::uint32_t offset = static_cast<std::uint32_t>(instruction.immediate);
               const std::uint32_t effective_address = base + offset;
                const std::uint32_t value = memory().load_word(effective_address);
                registers().write(instruction.rd, value);
                break;
            }

            case Operation::Sw :
            {
               const std::uint32_t base = registers().read(instruction.rs1);
               const std::uint32_t offset = static_cast<std::uint32_t>(instruction.immediate);
               const std::uint32_t effective_address = base + offset;
               const std::uint32_t value = registers().read(instruction.rs2);
               memory().store_word(effective_address, value);
               break;
            }

            case Operation::Beq:
            {
               const std::uint32_t left = registers().read(instruction.rs1);
               const std::uint32_t right = registers().read(instruction.rs2);
               if (left == right) {
                   const std::uint32_t offset = static_cast<std::uint32_t>(instruction.immediate);
                   next_pc = pc_ + offset;
               }
               break;
            }
            default:
                    throw std::invalid_argument("Invalid instruction");
        }
        pc_ = next_pc;
    }
} // pipeline_sim