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
        bool load_use_hazard = false;
        if (id_ex_.valid && is_load(id_ex_.instruction.operation) && id_ex_.instruction.rd != 0 && if_id_.valid) {
            const DecodedInstruction waiting_instruction = decode(if_id_.raw_instruction);
            const bool rs1_hazard = uses_rs1(waiting_instruction.operation) && waiting_instruction.rs1 == id_ex_.instruction.rd;
            const bool rs2_hazard = uses_rs2(waiting_instruction.operation) && waiting_instruction.rs2 == id_ex_.instruction.rd;
            load_use_hazard = rs1_hazard || rs2_hazard;
        }

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

            std::uint32_t rs1_operand = id_ex_.rs1_value;
            std::uint32_t rs2_operand = id_ex_.rs2_value;
            const bool can_forward_from_mem_wb = mem_wb_.valid && mem_wb_.write_enable && mem_wb_.instruction.rd != 0;
            if (can_forward_from_mem_wb) {
               if (uses_rs1(id_ex_.instruction.operation) && id_ex_.instruction.rs1 == mem_wb_.instruction.rd) {
                   rs1_operand = mem_wb_.write_value;
               }
                if (uses_rs2(id_ex_.instruction.operation) && id_ex_.instruction.rs2 == mem_wb_.instruction.rd) {
                    rs2_operand = mem_wb_.write_value;
                }
            }

            const bool can_forward_from_ex_mem = ex_mem_.valid && writes_register(ex_mem_.instruction.operation) && !is_load(ex_mem_.instruction.operation) && ex_mem_.instruction.rd != 0;
            if (can_forward_from_ex_mem) {
                if (uses_rs1(id_ex_.instruction.operation) && id_ex_.instruction.rs1 == ex_mem_.instruction.rd) {
                   rs1_operand = ex_mem_.alu_result;
                }
                if (uses_rs2(id_ex_.instruction.operation) && id_ex_.instruction.rs2 == ex_mem_.instruction.rd) {
                    rs2_operand = ex_mem_.alu_result;
                }
            }
            switch (id_ex_.instruction.operation) {
             case Operation::Add :
            {
                next_ex_mem.alu_result = rs1_operand + rs2_operand;
                break;
            }
            case Operation::Sub :
            {
                next_ex_mem.alu_result = rs1_operand - rs2_operand;
                break;
            }

            case Operation::Addi:
            {
                next_ex_mem.alu_result = rs1_operand + static_cast<std::uint32_t>(id_ex_.instruction.immediate);
               break;
            }

            case Operation::Lw:
            {
                next_ex_mem.alu_result = rs1_operand + static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                break;
            }

            case Operation::Sw :
            {
                const std::uint32_t offset = static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                next_ex_mem.alu_result = rs1_operand + offset;
                next_ex_mem.store_value = rs2_operand;
               break;
            }

            case Operation::Beq:
            {
                const std::uint32_t offset = static_cast<std::uint32_t>(id_ex_.instruction.immediate);
                next_ex_mem.branch_taken = rs1_operand == rs2_operand;
                next_ex_mem.branch_target = id_ex_.pc + offset;
               break;
            }
            default:
                    throw std::invalid_argument("Invalid instruction");
            }
        }

        // ID stage: reads the current IF/ID register.
        if (if_id_.valid && !load_use_hazard) {
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

        std::uint32_t next_pc = pc();
        if (load_use_hazard) {
            next_if_id = if_id_;
        } else {
            next_if_id.valid = true;
            next_if_id.pc = pc();
            next_if_id.raw_instruction = memory().load_word(pc());
            next_pc = pc()+4;
        }
        if_id_ = next_if_id;
        id_ex_ = next_id_ex;
        ex_mem_ = next_ex_mem;
        mem_wb_ = next_mem_wb;
        pc_ = next_pc;

    }
} // pipeline_sim