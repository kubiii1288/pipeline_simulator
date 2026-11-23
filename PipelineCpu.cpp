//
// Created by kubiii on 11/12/26.
//

#include "PipelineCpu.h"

#include <stdexcept>

#include "Decoder.h"

namespace pipeline_sim
{
    PipelineCpu::PipelineCpu(std::size_t memory_size): memory_(memory_size) {}

    Memory &PipelineCpu::memory() noexcept
    {
        return memory_;
    }

    std::uint32_t PipelineCpu::pc() const noexcept
    {
        return pc_;
    }

    RegisterFile &PipelineCpu::registers() noexcept
    {
        return registers_;
    }

    void PipelineCpu::tick()
    {
        // start with bubbles for the next cycle
        IfIdRegister next_if_id{};
        IdExRegister next_id_ex{};
        ExMemRegister next_ex_mem{};
        MemWbRegister next_mem_wb{};


        // WB stage: commit the oldest instruction
        if (mem_wb_.valid && mem_wb_.write_enable) {
            registers().write(mem_wb_.instruction.rd, mem_wb_.write_value);
        }

        // MEM stage: reads the current EX/MEM and fill next MEM/WB
        if (ex_mem_.valid) {
            next_mem_wb.valid = true;
            next_mem_wb.pc = ex_mem_.pc;
            next_mem_wb.instruction = ex_mem_.instruction;

            switch (ex_mem_.instruction.operation) {
                case Operation::Add:
                case Operation::Sub:
                case Operation::Addi:
                {
                   next_mem_wb.write_enable = true;
                   next_mem_wb.write_value = ex_mem_.alu_result;
                   break;
                }
                case Operation::Lw:
                {
                    next_mem_wb.write_enable = true;
                    next_mem_wb.write_value = memory().load_word(ex_mem_.alu_result);
                    break;
                }
                case Operation::Sw:
                {
                    memory().store_word(ex_mem_.alu_result, ex_mem_.store_value);
                   break;
                }
                case Operation::Beq:
                {
                    break;
                }

                default: throw std::invalid_argument("invalid instruction");
            }
        }

        // EX stage: reads the current ID/EX register
        if (id_ex_.valid) {
            next_ex_mem.valid = true;
            next_ex_mem.pc = id_ex_.pc;
            next_ex_mem.instruction = id_ex_.instruction;

            switch (id_ex_.instruction.operation) {
             case Operation::Add :
            {
                next_ex_mem.alu_result = id_ex_.rs1_value + id_ex_.rs2_value;
                break;
            }
            case Operation::Sub :
            {
                next_ex_mem.alu_result = id_ex_.rs1_value - id_ex_.rs2_value;
                break;
            }

            case Operation::Addi:
            {
                next_ex_mem.alu_result = id_ex_.rs1_value + static_cast<std::uint32_t>(id_ex_.instruction.immediate);
               break;
            }

            case Operation::Lw:
            {
                next_ex_mem.alu_result = id_ex_.rs1_value + static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                break;
            }

            case Operation::Sw :
            {
                const std::uint32_t offset = static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                next_ex_mem.alu_result = id_ex_.rs1_value + offset;
                next_ex_mem.store_value = id_ex_.rs2_value;
               break;
            }

            case Operation::Beq:
            {
                const std::uint32_t offset = static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                next_ex_mem.branch_taken = id_ex_.rs1_value == id_ex_.rs2_value;
                next_ex_mem.branch_target = id_ex_.pc + offset;
               break;
            }
            default:
                    throw std::invalid_argument("Invalid instruction");
            }
        }

        // ID stage: reads the current IF/ID register.
        if (if_id_.valid) {
            const DecodedInstruction instruction = decode(if_id_.raw_instruction);
            if (instruction.operation == Operation::Invalid) {
                throw std::invalid_argument("invalid instruction");
            }
            // setup for ID/EX
            next_id_ex.valid = true;
            next_id_ex.pc = if_id_.pc;
            next_id_ex.instruction = instruction;
            next_id_ex.rs1_value = registers().read(instruction.rs1);
            next_id_ex.rs2_value = registers().read(instruction.rs2);
        }

        // IF stage: create the next IF/ID value
        next_if_id.valid = true;
        next_if_id.pc = pc();
        next_if_id.raw_instruction = memory().load_word(pc());

        const std::uint32_t next_pc = pc() +4;

        if_id_ = next_if_id;
        id_ex_ = next_id_ex;
        ex_mem_ = next_ex_mem;
        mem_wb_ = next_mem_wb;
        pc_ = next_pc;

    }
} // pipeline_sim